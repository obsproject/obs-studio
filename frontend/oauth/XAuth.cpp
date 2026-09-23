#include "XAuth.hpp"

#include <oauth/AuthListener.hpp>
#include <utility/RemoteTextThread.hpp>
#include <utility/XApiWrappers.hpp>
#include <utility/obf.h>
#include <widgets/OBSBasic.hpp>

#include <qt-wrappers.hpp>
#include <ui-config.h>

#include <QCryptographicHash>
#include <QDesktopServices>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QThread>
#include <QUrl>
#include <QUrlQuery>

#include <json11.hpp>

#include "moc_XAuth.cpp"

#define X_AUTH_URL "https://api.x.com/2/oauth2/authorize"
#define X_TOKEN_URL "https://api.x.com/2/oauth2/token"
#define X_SCOPE_VERSION 2
#define X_STATE_LENGTH 32
#define X_VERIFIER_LENGTH 64

using namespace json11;

static const char allowedChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
static const int allowedCount = static_cast<int>(sizeof(allowedChars) - 1);
static const char verifierChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~";
static const int verifierCount = static_cast<int>(sizeof(verifierChars) - 1);

static void DeleteCookies() {}

static bool LoadClient(std::string &clientId, std::string &secret)
{
	clientId = X_CLIENTID;
	secret = X_SECRET;
	if (!clientId.empty()) {
		deobfuscate_str(&clientId[0], X_CLIENTID_HASH);
	}
	if (!secret.empty()) {
		deobfuscate_str(&secret[0], X_SECRET_HASH);
	}
	return !clientId.empty() && !secret.empty();
}

static QString BasicAuthorization(const std::string &clientId, const std::string &secret)
{
	const QByteArray id = QUrl::toPercentEncoding(QString::fromStdString(clientId));
	const QByteArray key = QUrl::toPercentEncoding(QString::fromStdString(secret));
	const QByteArray token = (id + ":" + key).toBase64();
	return QString::fromLatin1("Authorization: Basic ") + QString::fromLatin1(token);
}

static QString RandomToken(const char *alphabet, int alphabetCount, int length)
{
	QString out;
	out.reserve(length);
	QRandomGenerator *rng = QRandomGenerator::system();
	for (int i = 0; i < length; i++) {
		out.append(QLatin1Char(alphabet[rng->bounded(0, alphabetCount)]));
	}
	return out;
}

static QString CodeChallenge(const QString &verifier)
{
	const QByteArray digest = QCryptographicHash::hash(verifier.toUtf8(), QCryptographicHash::Sha256);
	return QString::fromLatin1(digest.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

static QString TokenError(const Json &json)
{
	const std::string error = json["error"].string_value();
	const std::string description = json["error_description"].string_value();
	if (!description.empty()) {
		return QString::fromStdString(description);
	}
	if (!error.empty()) {
		return QString::fromStdString(error);
	}
	return QString::fromStdString(json["detail"].string_value());
}

void RegisterXAuth()
{
	OAuth::RegisterOAuth(
		xServiceDef, []() { return std::make_shared<XApiWrappers>(xServiceDef); }, XAuth::Login, DeleteCookies);
}

XAuth::XAuth(const Def &d) : OAuthStreamKey(d) {}

QString XAuth::RedirectUri()
{
	return QString("http://127.0.0.1:%1/callback").arg(XOAuthRedirectPort);
}

QString XAuth::GenerateState() const
{
	return RandomToken(allowedChars, allowedCount, X_STATE_LENGTH);
}

bool XAuth::RetryLogin()
{
	return false;
}

void XAuth::SaveInternal()
{
	OAuth::SaveInternal();
	OBSBasic *main = OBSBasic::Get();
	config_set_string(main->Config(), service(), "Username", QT_TO_UTF8(username));
}

bool XAuth::LoadInternal()
{
	OBSBasic *main = OBSBasic::Get();
	if (!OAuth::LoadInternal()) {
		return false;
	}
	const char *name = config_get_string(main->Config(), service(), "Username");
	username = name ? QString::fromUtf8(name) : QString();
	firstLoad = false;
	return true;
}

bool XAuth::ExchangeCode(const QString &code, const QString &redirectUri, const QString &verifier)
{
	std::string clientId;
	std::string secret;
	if (!LoadClient(clientId, secret)) {
		return false;
	}

	QUrlQuery form;
	form.addQueryItem("grant_type", "authorization_code");
	form.addQueryItem("code", code);
	form.addQueryItem("redirect_uri", redirectUri);
	form.addQueryItem("client_id", QString::fromStdString(clientId));
	form.addQueryItem("code_verifier", verifier);
	const QByteArray body = form.query(QUrl::FullyEncoded).toUtf8();

	std::string output;
	std::string error;
	long status = 0;
	const std::string authorization = BasicAuthorization(clientId, secret).toStdString();
	const bool success = GetRemoteFile(X_TOKEN_URL, output, error, &status, "application/x-www-form-urlencoded",
					   "POST", body.constData(), {authorization}, nullptr, 15, false);

	if (!success || output.empty()) {
		blog(LOG_WARNING, "X token exchange failed: %s", error.c_str());
		return false;
	}

	std::string parseError;
	Json json = Json::parse(output, parseError);
	if (!parseError.empty()) {
		blog(LOG_WARNING, "X token exchange returned invalid JSON");
		return false;
	}
	if (status >= 400 || !json["error"].string_value().empty()) {
		blog(LOG_WARNING, "X token exchange rejected: %s", QT_TO_UTF8(TokenError(json)));
		return false;
	}

	token = json["access_token"].string_value();
	refresh_token = json["refresh_token"].string_value();
	if (token.empty() || refresh_token.empty()) {
		return false;
	}

	int expires = json["expires_in"].int_value();
	if (expires <= 0) {
		expires = (int)json["expires_in"].number_value();
	}
	if (expires <= 0) {
		expires = 7200;
	}
	expire_time = (uint64_t)time(nullptr) + (uint64_t)expires;
	currentScopeVer = X_SCOPE_VERSION;
	return true;
}

bool XAuth::RefreshToken()
{
	if (refresh_token.empty()) {
		return false;
	}

	std::string clientId;
	std::string secret;
	if (!LoadClient(clientId, secret)) {
		return false;
	}

	QUrlQuery form;
	form.addQueryItem("grant_type", "refresh_token");
	form.addQueryItem("refresh_token", QString::fromStdString(refresh_token));
	form.addQueryItem("client_id", QString::fromStdString(clientId));
	const QByteArray body = form.query(QUrl::FullyEncoded).toUtf8();

	std::string output;
	std::string error;
	long status = 0;
	const std::string authorization = BasicAuthorization(clientId, secret).toStdString();
	const bool success = GetRemoteFile(X_TOKEN_URL, output, error, &status, "application/x-www-form-urlencoded",
					   "POST", body.constData(), {authorization}, nullptr, 15, false);
	if (!success || output.empty()) {
		blog(LOG_WARNING, "X token refresh failed: %s", error.c_str());
		return false;
	}

	std::string parseError;
	Json json = Json::parse(output, parseError);
	if (!parseError.empty() || status >= 400 || !json["error"].string_value().empty()) {
		blog(LOG_WARNING, "X token refresh rejected");
		return false;
	}

	const std::string access = json["access_token"].string_value();
	if (access.empty()) {
		return false;
	}
	token = access;
	const std::string rotated = json["refresh_token"].string_value();
	if (!rotated.empty()) {
		refresh_token = rotated;
	}
	int expires = json["expires_in"].int_value();
	if (expires <= 0) {
		expires = (int)json["expires_in"].number_value();
	}
	if (expires <= 0) {
		expires = 7200;
	}
	expire_time = (uint64_t)time(nullptr) + (uint64_t)expires;
	return true;
}

std::shared_ptr<Auth> XAuth::Login(QWidget *owner, const std::string &)
{
	AuthListener server(owner, XOAuthRedirectPort);
	if (server.GetPort() != XOAuthRedirectPort) {
		QMessageBox::warning(owner, QTStr("X.Auth.WaitingAuth.Title"),
				     QTStr("X.Auth.PortInUse").arg(XOAuthRedirectPort));
		return nullptr;
	}

	std::string clientId;
	std::string secret;
	if (!LoadClient(clientId, secret)) {
		return nullptr;
	}

	auto auth = std::make_shared<XApiWrappers>(xServiceDef);
	const QString redirectUri = RedirectUri();
	const QString state = auth->GenerateState();
	const QString verifier = RandomToken(verifierChars, verifierCount, X_VERIFIER_LENGTH);
	server.SetState(state);

	QUrl url(X_AUTH_URL);
	QUrlQuery query;
	query.addQueryItem("response_type", "code");
	query.addQueryItem("client_id", QString::fromStdString(clientId));
	query.addQueryItem("redirect_uri", redirectUri);
	// users.read resolves GET /2/users/me. The Livestream path :user_id must be that id.
	query.addQueryItem("scope", "broadcast.read broadcast.write users.read offline.access");
	query.addQueryItem("state", state);
	query.addQueryItem("code_challenge", CodeChallenge(verifier));
	query.addQueryItem("code_challenge_method", "S256");
	QString encoded = query.toString(QUrl::FullyEncoded);
	encoded.replace('+', "%20");
	url.setQuery(encoded);

	QMessageBox dlg(owner);
	dlg.setWindowFlags(dlg.windowFlags() & ~Qt::WindowCloseButtonHint);
	dlg.setWindowTitle(QTStr("X.Auth.WaitingAuth.Title"));
	const QString text = QTStr("X.Auth.WaitingAuth.Text")
				     .arg(QString("<a href='%1'>X OAuth</a>").arg(url.toString(QUrl::FullyEncoded)));
	dlg.setText(text);
	dlg.setTextFormat(Qt::RichText);
	dlg.setStandardButtons(QMessageBox::StandardButton::Cancel);
#if defined(__APPLE__) && QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
	dlg.setOption(QMessageBox::Option::DontUseNativeDialog);
#endif

	QString authCode;
	connect(&dlg, &QMessageBox::buttonClicked, &dlg, [&](QAbstractButton *) { dlg.reject(); });
	connect(&server, &AuthListener::ok, &dlg, [&dlg, &authCode](QString code) {
		authCode = code;
		dlg.accept();
	});
	connect(&server, &AuthListener::fail, &dlg, [&dlg]() { dlg.reject(); });

	const QString openUrl = url.toString(QUrl::FullyEncoded);
	QScopedPointer<QThread> thread(
		CreateQThread([openUrl]() { QDesktopServices::openUrl(QUrl(openUrl, QUrl::StrictMode)); }));
	thread->start();

#if defined(__APPLE__) && QT_VERSION >= QT_VERSION_CHECK(6, 5, 0) && QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
	const bool nativeDialogs = qApp->testAttribute(Qt::AA_DontUseNativeDialogs);
	App()->setAttribute(Qt::AA_DontUseNativeDialogs, true);
	dlg.exec();
	App()->setAttribute(Qt::AA_DontUseNativeDialogs, nativeDialogs);
#else
	dlg.exec();
#endif

	if (dlg.result() == QMessageBox::Cancel || dlg.result() == QDialog::Rejected || authCode.isEmpty()) {
		return nullptr;
	}

	bool exchanged = false;
	auto exchange = [&]() {
		exchanged = auth->ExchangeCode(authCode, redirectUri, verifier);
	};
	ExecThreadedWithoutBlocking(exchange, QTStr("Auth.Authing.Title"), QTStr("Auth.Authing.Text").arg("X"));
	if (!exchanged) {
		QMessageBox::warning(owner, QTStr("Auth.AuthFailure.Title"),
				     QTStr("Auth.AuthFailure.Text").arg("X", "token", "rejected"));
		return nullptr;
	}

	auto finish = [&]() {
		auth->FetchIdentity();
	};
	ExecThreadedWithoutBlocking(finish, QTStr("Auth.LoadingChannel.Title"),
				    QTStr("Auth.LoadingChannel.Text").arg("X"));

	config_t *config = OBSBasic::Get()->Config();
	config_set_string(config, "X", "Username", QT_TO_UTF8(auth->Username()));
	config_save_safe(config, "tmp", nullptr);
	return auth;
}
