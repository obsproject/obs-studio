#include <widgets/AeriumAccountWidget.hpp>

#include <QAction>
#include <QApplication>
#include <QCryptographicHash>
#include <QDesktopServices>
#include <QDialog>
#include <QEventLoop>
#include <QJsonDocument>
#include <QLabel>
#include <QKeyEvent>
#include <QMainWindow>
#include <QMenu>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPushButton>
#include <QUrlQuery>

#include <cstdio>
#include <cstring>
#include <deque>
#include <mutex>
#include <stdexcept>

namespace {
QByteArray savedToken;
std::mutex credentialMutex;
bool canSave = true;

void Require(bool condition, const char *message)
{
	if (!condition) {
		throw std::runtime_error(message);
	}
}

void Wait(const std::function<bool()> &predicate, int timeout = 8000)
{
	if (predicate()) {
		return;
	}
	QEventLoop loop;
	QTimer check;
	QTimer deadline;
	check.setInterval(5);
	deadline.setSingleShot(true);
	QObject::connect(&check, &QTimer::timeout, &loop, [&] {
		if (predicate()) {
			loop.quit();
		}
	});
	QObject::connect(&deadline, &QTimer::timeout, &loop, &QEventLoop::quit);
	check.start();
	deadline.start(timeout);
	loop.exec();
	Require(predicate(), "Timed out waiting for widget state");
}

QByteArray Saved()
{
	std::lock_guard guard(credentialMutex);
	return savedToken;
}

QJsonObject Tokens(char suffix = 'r')
{
	return {{"access_token", QString(43, QChar('a'))},
		{"refresh_token", QString(43, QChar(suffix))},
		{"expires_in", 900}};
}

QJsonObject Profile()
{
	return {{"user",
		 QJsonObject{{"login", "vastonline"}, {"display_name", "VastOnline"}, {"twitch_id", "476597509"}}}};
}

struct Expected {
	QString path;
	int status;
	QJsonObject body;
	int delay = 0;
	std::function<void(const QNetworkRequest &, const QJsonObject &)> verify;
};

class Reply : public QNetworkReply {
	QByteArray payload;
	qsizetype offset = 0;

public:
	Reply(const QNetworkRequest &request, const Expected &expected, QObject *parent) : QNetworkReply(parent)
	{
		setRequest(request);
		setUrl(request.url());
		open(QIODevice::ReadOnly);
		payload = QJsonDocument(expected.body).toJson();
		setAttribute(QNetworkRequest::HttpStatusCodeAttribute, expected.status);
		QTimer::singleShot(expected.delay, this, [this] {
			setFinished(true);
			emit readyRead();
			emit finished();
		});
	}
	void abort() override {}
	qint64 bytesAvailable() const override { return payload.size() - offset + QNetworkReply::bytesAvailable(); }
	qint64 readData(char *data, qint64 size) override
	{
		const auto count = std::min(size, static_cast<qint64>(payload.size() - offset));
		if (!count) {
			return -1;
		}
		memcpy(data, payload.constData() + offset, count);
		offset += count;
		return count;
	}
};

class Network : public QNetworkAccessManager {
public:
	std::deque<Expected> expected;
	int requests = 0;
	QString failure;

protected:
	QNetworkReply *createRequest(Operation operation, const QNetworkRequest &request, QIODevice *outgoing) override
	{
		requests++;
		Expected next{request.url().path(), 503, {}};
		if (expected.empty()) {
			failure = "Unexpected network request";
		} else {
			next = expected.front();
			expected.pop_front();
			try {
				Require(operation == PostOperation, "Unexpected method");
				Require(request.url().scheme() == "https" && request.url().host() == "api.aerium.tv",
					"Wrong API origin");
				Require(request.url().path() == next.path, "Unexpected API path");
				Require(request.attribute(QNetworkRequest::RedirectPolicyAttribute).toInt() ==
						QNetworkRequest::ManualRedirectPolicy,
					"Token-bearing request allowed redirects");
				const auto body = QJsonDocument::fromJson(outgoing->readAll()).object();
				if (next.verify) {
					next.verify(request, body);
				}
			} catch (const std::exception &error) {
				failure = error.what();
			}
		}
		return new Reply(request, next, this);
	}
};

class Browser : public QObject {
	Q_OBJECT
public:
	QUrl opened;
public slots:
	void Open(const QUrl &url) { opened = url; }
};

QPushButton *Button(AeriumAccountWidget &widget)
{
	return widget.findChild<QPushButton *>("aeriumAccountButton");
}

QPushButton *Cancel(AeriumAccountWidget &widget)
{
	return widget.findChild<QPushButton *>("aeriumLoginCancel");
}

void SignedOut(AeriumAccountWidget &widget)
{
	Wait([&] { return Button(widget)->isEnabled() && Button(widget)->text() == "Sign in with Twitch"; });
}

void Clean(Network &network)
{
	Require(network.failure.isEmpty(), qPrintable(network.failure));
	Require(network.expected.empty(), "Expected requests not made");
}
} // namespace

AeriumCredentials::Result AeriumCredentials::Read()
{
	return {true, Saved()};
}
AeriumCredentials::Result AeriumCredentials::Write(const QByteArray &token)
{
	std::lock_guard guard(credentialMutex);
	if (canSave) {
		savedToken = token;
	}
	return {canSave, {}};
}
AeriumCredentials::Result AeriumCredentials::Remove()
{
	std::lock_guard guard(credentialMutex);
	savedToken.clear();
	return {true, {}};
}

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	Browser browser;
	QDesktopServices::setUrlHandler("https", &browser, "Open");
	try {
		{
			Network network;
			QMainWindow host;
			AeriumAccountWidget widget(&host, &network, &host);
			host.resize(1000, 720);
			host.show();
			widget.resize(900, 48);
			widget.show();
			SignedOut(widget);
			auto *overlay = host.findChild<QDialog *>("aeriumLoginWindow");
			Wait([&] { return overlay && overlay->isVisible(); });
			Require(QApplication::activeModalWidget() == overlay, "Login did not block the host window");
			Require(overlay->size() == host.size(), "Login does not cover the host window");
			QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
			QApplication::sendEvent(overlay, &escape);
			overlay->close();
			Require(overlay->isVisible(), "Escape or Close bypassed login");
			host.resize(860, 640);
			host.move(100, 80);
			Wait([&] { return overlay->geometry() == QRect(host.mapToGlobal(QPoint(0, 0)), host.size()); });
			overlay->grab().save("/tmp/aerium-login-overlay.png");
			Require(network.requests == 0, "Signed-out startup used network");
			widget.grab().save("/tmp/aerium-account-signed-out.png");
			const QString id(43, QChar('i'));
			QString challenge;
			network.expected.push_back(
				{"/v1/auth/desktop",
				 201,
				 {{"request_id", id},
				  {"authorization_url", "https://api.aerium.tv/v1/auth/twitch?request=" + id}},
				 0,
				 [&](const auto &, const auto &body) {
					 challenge = body.value("code_challenge").toString();
				 }});
			network.expected.push_back(
				{"/v1/auth/exchange", 200, Tokens(), 0, [&](const auto &, const auto &body) {
					 const auto verifier = body.value("code_verifier").toString().toLatin1();
					 Require(verifier.size() == 43, "Verifier is not 256-bit base64url");
					 Require(QString::fromLatin1(
							 QCryptographicHash::hash(verifier, QCryptographicHash::Sha256)
								 .toBase64(QByteArray::Base64UrlEncoding |
									   QByteArray::OmitTrailingEquals)) ==
							 challenge,
						 "Handoff proof mismatch");
				 }});
			network.expected.push_back({"/v1/session/validate", 200, Profile(), 100});
			overlay->findChild<QPushButton *>("aeriumOverlayLogin")->click();
			Wait([&] { return network.requests == 3; });
			Require(overlay->isVisible(), "Login unlocked before account validation");
			Wait([&] { return Button(widget)->text() == "VastOnline"; });
			Require(!overlay->isVisible(), "Verified login did not dismiss overlay");
			Require(QUrlQuery(browser.opened).queryItemValue("request") == id, "Browser was not opened");
			Require(Saved() == QByteArray(43, 'r'), "Refresh credential was not saved");
			Require(Button(widget)->menu() != nullptr, "Account menu missing");
			Button(widget)->menu()->popup(Button(widget)->mapToGlobal(QPoint(0, Button(widget)->height())));
			Wait([&] { return Button(widget)->menu()->isVisible(); });
			Button(widget)->menu()->hide();
			widget.grab().save("/tmp/aerium-account-signed-in.png");
			Clean(network);
			std::puts(
				"PASS modal coverage, resize, dismissal prevention, verified login, and top-right username");
		}
		{
			Network network;
			network.expected.push_back({"/v1/session/refresh", 200, Tokens('s')});
			network.expected.push_back({"/v1/session/validate", 200, Profile(), 100});
			QMainWindow host;
			AeriumAccountWidget widget(&host, &network, &host);
			host.resize(1000, 720);
			host.show();
			auto *overlay = host.findChild<QDialog *>("aeriumLoginWindow");
			Wait([&] { return network.requests == 2; });
			Require(overlay->isVisible(), "Restore unlocked before validation");
			Wait([&] { return Button(widget)->text() == "VastOnline"; });
			Require(!overlay->isVisible(), "Saved session did not dismiss login");
			Require(Saved() == QByteArray(43, 's'), "Restored session was not rotated");
			network.expected.push_back({"/v1/session/refresh", 503, {}, 100});
			Button(widget)->menu()->findChild<QAction *>("aeriumReconnect")->trigger();
			Require(Button(widget)->text() == "VastOnline", "Background reconnect hid username");
			Wait([&] {
				return widget.findChild<QLabel *>("aeriumAccountStatus")->toolTip().contains("offline");
			});
			Require(Saved() == QByteArray(43, 's'), "Service outage discarded saved credential");
			network.expected.push_back({"/v1/session/logout", 204, {}});
			Button(widget)->menu()->findChild<QAction *>("aeriumSignOut")->trigger();
			SignedOut(widget);
			Require(overlay->isVisible(), "Sign-out did not restore the login gate");
			Wait([&] { return Saved().isEmpty(); });
			Clean(network);
			std::puts(
				"PASS restart restores, outage preserves username/session, and sign-out clears credentials");
		}
		{
			AeriumCredentials::Write(QByteArray(43, 'r'));
			Network network;
			network.expected.push_back({"/v1/session/refresh", 401, {}});
			QMainWindow host;
			AeriumAccountWidget widget(&host, &network, &host);
			host.resize(1000, 720);
			host.show();
			Wait([&] { return network.requests == 1 && Saved().isEmpty(); });
			SignedOut(widget);
			Require(host.findChild<QDialog *>("aeriumLoginWindow")->isVisible(),
				"Rejected session bypassed login");
			Clean(network);
			std::puts("PASS rejected saved session returns to sign-in");
		}
		{
			AeriumCredentials::Write(QByteArray(43, 'r'));
			Network network;
			network.expected.push_back({"/v1/session/refresh", 200, Tokens('t'), 100});
			network.expected.push_back({"/v1/session/logout", 204, {}});
			network.expected.push_back({"/v1/session/logout", 204, {}});
			QMainWindow host;
			AeriumAccountWidget widget(&host, &network, &host);
			host.resize(1000, 720);
			host.show();
			Wait([&] { return network.requests == 1; });
			host.findChild<QPushButton *>("aeriumOverlayCancel")->click();
			Wait([&] { return network.requests == 3 && Saved().isEmpty(); });
			SignedOut(widget);
			Require(host.findChild<QDialog *>("aeriumLoginWindow")->isVisible(),
				"Cancelled login dismissed overlay");
			Clean(network);
			std::puts("PASS cancellation ignores and revokes late rotated session");
		}
		{
			Network network;
			AeriumAccountWidget widget(nullptr, &network);
			SignedOut(widget);
			const QUrl previous = browser.opened;
			network.expected.push_back({"/v1/auth/desktop",
						    201,
						    {{"request_id", QString(43, QChar('i'))},
						     {"authorization_url", "https://evil.example/login"}}});
			Button(widget)->click();
			Wait([&] { return network.requests == 1 && Button(widget)->isEnabled(); });
			Require(browser.opened == previous, "Untrusted browser URL was opened");
			Clean(network);
			std::puts("PASS malicious login URL rejected");
		}
		{
			Network network;
			QMainWindow host;
			AeriumAccountWidget widget(&host, &network, &host);
			host.resize(1000, 720);
			host.show();
			SignedOut(widget);
			auto *overlay = host.findChild<QDialog *>("aeriumLoginWindow");
			auto *login = overlay->findChild<QPushButton *>("aeriumOverlayLogin");
			network.expected.push_back({"/v1/auth/desktop", 503, {}});
			login->click();
			Wait([&] { return network.requests == 1 && login->isEnabled(); });
			Require(overlay->isVisible(), "Service failure dismissed login");
			const QString id(43, QChar('i'));
			network.expected.push_back(
				{"/v1/auth/desktop",
				 201,
				 {{"request_id", id},
				  {"authorization_url", "https://api.aerium.tv/v1/auth/twitch?request=" + id}}});
			network.expected.push_back({"/v1/auth/exchange", 403, {}});
			login->click();
			Wait([&] { return network.requests == 3 && login->isEnabled(); });
			Require(overlay->isVisible(), "Declined Twitch consent dismissed login");
			Require(overlay->findChild<QLabel *>("aeriumOverlayStatus")->text().contains("declined"),
				"Overlay did not display login error");
			overlay->findChild<QPushButton *>("aeriumOverlayQuit")->click();
			Wait([&] { return !host.isVisible() && !overlay->isVisible(); });
			Clean(network);
			std::puts("PASS service failure, denied consent, retry, and quit without bypassing login");
		}
		{
			AeriumCredentials::Write(QByteArray(43, 'r'));
			canSave = false;
			Network network;
			network.expected.push_back({"/v1/session/refresh", 200, Tokens('u')});
			network.expected.push_back({"/v1/session/validate", 200, Profile()});
			AeriumAccountWidget widget(nullptr, &network);
			Wait([&] { return Button(widget)->text() == "VastOnline"; });
			Require(widget.findChild<QLabel *>("aeriumAccountStatus")->toolTip().contains("Keychain"),
				"Failed secure save not reported");
			Clean(network);
			std::puts("PASS Keychain failure keeps credentials in memory and reports it");
		}
		QDesktopServices::unsetUrlHandler("https");
		return 0;
	} catch (const std::exception &error) {
		std::fprintf(stderr, "FAIL: %s\n", error.what());
		return 1;
	}
}

#include "account-test.moc"
