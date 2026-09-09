#include "AeriumAccountWidget.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QCryptographicHash>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDialog>
#include <QJsonDocument>
#include <QMenu>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QUrlQuery>

#include <algorithm>
#include <chrono>
#include <cstring>

#include "moc_AeriumAccountWidget.cpp"

namespace {
const QString apiOrigin = QStringLiteral("https://api.aerium.tv");

class LoginWindow : public QDialog {
public:
	explicit LoginWindow(QWidget *parent) : QDialog(parent, Qt::Dialog | Qt::FramelessWindowHint)
	{
		setWindowModality(Qt::WindowModal);
	}
	void reject() override {}

protected:
	void closeEvent(QCloseEvent *event) override { event->ignore(); }
};

bool ValidToken(const QByteArray &token)
{
	static const QRegularExpression pattern(QStringLiteral("^[A-Za-z0-9_-]{43}$"));
	return pattern.match(QString::fromLatin1(token)).hasMatch();
}

QByteArray RandomVerifier()
{
	QByteArray bytes(32, '\0');
	for (qsizetype index = 0; index < bytes.size(); index += 4) {
		const quint32 value = QRandomGenerator::system()->generate();
		memcpy(bytes.data() + index, &value, sizeof(value));
	}
	return bytes.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
}
} // namespace

AeriumAccountWidget::AeriumAccountWidget(QWidget *parent, QNetworkAccessManager *manager, QWidget *host)
	: QWidget(parent),
	  loginHost(host)
{
	network = manager ? manager : new QNetworkAccessManager(this);
	credentialWorker.setMaxThreadCount(1);
	setObjectName("aeriumAccountWidget");
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	auto *layout = new QHBoxLayout(this);
	layout->setContentsMargins(12, 4, 12, 4);
	auto *brand = new QLabel("Aerium", this);
	brand->setObjectName("aeriumBrand");
	layout->addWidget(brand);
	statusLabel = new QLabel(this);
	statusLabel->setObjectName("aeriumAccountStatus");
	statusLabel->setTextFormat(Qt::PlainText);
	statusLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	statusLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
	layout->addWidget(statusLabel, 1);
	cancelButton = new QPushButton(tr("Cancel"), this);
	cancelButton->setObjectName("aeriumLoginCancel");
	cancelButton->hide();
	layout->addWidget(cancelButton);
	accountButton = new QPushButton(tr("Sign in with Twitch"), this);
	accountButton->setObjectName("aeriumAccountButton");
	accountButton->setFixedWidth(240);
	accountButton->setProperty("class", "button-primary");
	layout->addWidget(accountButton);
	accountMenu = new QMenu(host ? host : this);
	auto *retry = accountMenu->addAction(tr("Reconnect Twitch"));
	retry->setObjectName("aeriumReconnect");
	connect(retry, &QAction::triggered, this, &AeriumAccountWidget::RefreshSession);
	auto *signOut = accountMenu->addAction(tr("Sign out"));
	signOut->setObjectName("aeriumSignOut");
	connect(signOut, &QAction::triggered, this, &AeriumAccountWidget::SignOut);
	connect(accountButton, &QPushButton::clicked, this, [this] {
		if (!busy && username.isEmpty()) {
			if (refreshToken.isEmpty()) {
				StartLogin();
			} else {
				RefreshSession();
			}
		}
	});
	connect(cancelButton, &QPushButton::clicked, this, &AeriumAccountWidget::CancelLogin);
	pollTimer.setSingleShot(true);
	sessionTimer.setSingleShot(true);
	connect(&pollTimer, &QTimer::timeout, this, &AeriumAccountWidget::PollLogin);
	connect(&sessionTimer, &QTimer::timeout, this, &AeriumAccountWidget::RefreshSession);
	if (loginHost) {
		CreateLoginWindow();
		loginHost->installEventFilter(this);
	}
	QTimer::singleShot(0, this, &AeriumAccountWidget::RestoreSession);
}

AeriumAccountWidget::~AeriumAccountWidget()
{
	credentialWorker.waitForDone();
	delete loginWindow.data();
}

void AeriumAccountWidget::CreateLoginWindow()
{
	loginWindow = new LoginWindow(loginHost);
	loginWindow->setObjectName("aeriumLoginWindow");
	loginWindow->setWindowTitle(tr("Sign in to Aerium"));
	auto *layout = new QVBoxLayout(loginWindow);
	layout->setContentsMargins(32, 32, 32, 24);
	layout->setSpacing(16);
	layout->addStretch();
	auto *title = new QLabel("Aerium", loginWindow);
	title->setObjectName("aeriumLoginTitle");
	layout->addWidget(title, 0, Qt::AlignHCenter);
	auto *subtitle = new QLabel(tr("Twitch sign-in"), loginWindow);
	layout->addWidget(subtitle, 0, Qt::AlignHCenter);
	layout->addSpacing(24);
	loginButton = new QPushButton(tr("Sign in with Twitch"), loginWindow);
	loginButton->setObjectName("aeriumOverlayLogin");
	loginButton->setProperty("class", "button-primary");
	loginButton->setFixedWidth(280);
	loginButton->setMinimumHeight(44);
	loginButton->setDefault(true);
	layout->addWidget(loginButton, 0, Qt::AlignHCenter);
	loginStatus = new QLabel(loginWindow);
	loginStatus->setObjectName("aeriumOverlayStatus");
	loginStatus->setTextFormat(Qt::PlainText);
	loginStatus->setWordWrap(true);
	loginStatus->setAlignment(Qt::AlignCenter);
	loginStatus->setMinimumHeight(64);
	loginStatus->setFixedWidth(480);
	layout->addWidget(loginStatus, 0, Qt::AlignHCenter);
	loginCancelButton = new QPushButton(tr("Cancel sign-in"), loginWindow);
	loginCancelButton->setObjectName("aeriumOverlayCancel");
	loginCancelButton->setAutoDefault(false);
	layout->addWidget(loginCancelButton, 0, Qt::AlignHCenter);
	layout->addStretch();
	auto *quit = new QPushButton(tr("Quit Aerium"), loginWindow);
	quit->setObjectName("aeriumOverlayQuit");
	quit->setAutoDefault(false);
	layout->addWidget(quit, 0, Qt::AlignRight);
	connect(loginButton, &QPushButton::clicked, accountButton, &QPushButton::click);
	connect(loginCancelButton, &QPushButton::clicked, cancelButton, &QPushButton::click);
	connect(quit, &QPushButton::clicked, loginHost, &QWidget::close);
}

void AeriumAccountWidget::UpdateLoginWindow()
{
	if (!loginWindow || !loginHost) {
		return;
	}
	loginButton->setText(accountButton->text());
	loginButton->setEnabled(accountButton->isEnabled());
	loginCancelButton->setVisible(!cancelButton->isHidden());
	loginStatus->setText(statusLabel->toolTip());
	const bool required = username.isEmpty() && loginHost->isVisible() && !loginHost->isMinimized();
	if (required) {
		loginWindow->setGeometry(QRect(loginHost->mapToGlobal(QPoint(0, 0)), loginHost->size()));
		if (!loginWindow->isVisible()) {
			loginWindow->show();
			loginButton->setFocus();
		}
	} else {
		loginWindow->hide();
	}
}

bool AeriumAccountWidget::eventFilter(QObject *watched, QEvent *event)
{
	if (watched == loginHost &&
	    (event->type() == QEvent::Show || event->type() == QEvent::Hide || event->type() == QEvent::Move ||
	     event->type() == QEvent::Resize || event->type() == QEvent::WindowStateChange)) {
		QTimer::singleShot(0, this, &AeriumAccountWidget::UpdateLoginWindow);
	}
	return QWidget::eventFilter(watched, event);
}

void AeriumAccountWidget::SetStatus(const QString &text)
{
	statusLabel->setToolTip(text);
	statusLabel->setAccessibleName(text);
	statusLabel->setText(
		statusLabel->fontMetrics().elidedText(text, Qt::ElideRight, std::max(0, statusLabel->width())));
	UpdateLoginWindow();
}

void AeriumAccountWidget::SetBusy(const QString &text, bool cancellable)
{
	busy = true;
	if (!username.isEmpty()) {
		cancelButton->hide();
		SetStatus(text);
		return;
	}
	accountButton->setMenu(nullptr);
	accountButton->setText(text);
	accountButton->setEnabled(false);
	cancelButton->setVisible(cancellable);
	UpdateLoginWindow();
}

void AeriumAccountWidget::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);
	SetStatus(statusLabel->toolTip());
}

void AeriumAccountWidget::CredentialTask(std::function<AeriumCredentials::Result()> work, CredentialHandler callback)
{
	const auto current = generation;
	credentialWorker.start([this, current, work = std::move(work), callback = std::move(callback)] {
		const auto result = work();
		QMetaObject::invokeMethod(
			this,
			[this, current, result, callback] {
				if (current == generation) {
					callback(result);
				}
			},
			Qt::QueuedConnection);
	});
}

void AeriumAccountWidget::Request(const QString &path, const QJsonObject &body, const QByteArray &bearer,
				  ResponseHandler callback)
{
	QNetworkRequest request(QUrl(apiOrigin + path));
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
	request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
	request.setAttribute(QNetworkRequest::CookieLoadControlAttribute, QNetworkRequest::Manual);
	request.setAttribute(QNetworkRequest::CookieSaveControlAttribute, QNetworkRequest::Manual);
	request.setTransferTimeout(std::chrono::seconds(20));
	if (!bearer.isEmpty()) {
		request.setRawHeader("Authorization", "Bearer " + bearer);
	}
	auto *reply = network->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
	const auto current = generation;
	connect(reply, &QNetworkReply::readyRead, reply, [reply] {
		if (reply->bytesAvailable() > 65536) {
			reply->abort();
		}
	});
	connect(reply, &QNetworkReply::finished, this, [this, reply, current, path, callback] {
		const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
		const auto document = QJsonDocument::fromJson(reply->readAll());
		const int retry = std::clamp(reply->rawHeader("Retry-After").toInt(), 5, 60);
		reply->deleteLater();
		if (current != generation) {
			if (status == 200 && (path == "/v1/auth/exchange" || path == "/v1/session/refresh")) {
				Revoke(document.object().value("refresh_token").toString().toLatin1());
			}
			return;
		}
		callback(status, document.object(), retry);
	});
}

void AeriumAccountWidget::RestoreSession()
{
	SetBusy(tr("Checking account..."), true);
	CredentialTask(AeriumCredentials::Read, [this](const auto &result) {
		busy = false;
		cancelButton->hide();
		accountButton->setEnabled(true);
		accountButton->setText(tr("Sign in with Twitch"));
		UpdateLoginWindow();
		if (!result.success) {
			SetStatus(tr("Could not read Keychain. Sign in to retry."));
			return;
		}
		if (ValidToken(result.token)) {
			refreshToken = result.token;
			remembered = true;
			RefreshSession();
		}
	});
}

void AeriumAccountWidget::StartLogin()
{
	++generation;
	verifier = RandomVerifier();
	requestId.clear();
	loginClock.start();
	pollDelay = 5000;
	SetBusy(tr("Opening Twitch..."), true);
	SetStatus({});
	const auto challenge = QCryptographicHash::hash(verifier, QCryptographicHash::Sha256)
				       .toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
	Request("/v1/auth/desktop", {{"code_challenge", QString::fromLatin1(challenge)}}, {},
		[this](int status, const QJsonObject &body, int) {
			if (status != 201) {
				CancelLogin();
				SetStatus(status == 429 ? tr("Too many attempts. Try again shortly.")
							: tr("Sign-in service unavailable. Try again."));
				return;
			}
			requestId = body.value("request_id").toString();
			const QUrl url(body.value("authorization_url").toString());
			if (!ValidToken(requestId.toLatin1()) || url.scheme() != "https" ||
			    url.host() != "api.aerium.tv" || url.port(-1) != -1 || !url.userInfo().isEmpty() ||
			    url.path() != "/v1/auth/twitch" || QUrlQuery(url).queryItemValue("request") != requestId ||
			    !QDesktopServices::openUrl(url)) {
				CancelLogin();
				SetStatus(tr("Could not open Twitch sign-in. Try again."));
				return;
			}
			SetBusy(tr("Waiting for Twitch..."), true);
			pollTimer.start(5000);
		});
}

void AeriumAccountWidget::PollLogin()
{
	if (loginClock.elapsed() >= 600000) {
		CancelLogin();
		SetStatus(tr("Sign-in expired. Try again."));
		return;
	}
	Request("/v1/auth/exchange", {{"request_id", requestId}, {"code_verifier", QString::fromLatin1(verifier)}}, {},
		[this](int status, const QJsonObject &body, int retry) {
			if (status == 200) {
				verifier.clear();
				requestId.clear();
				AcceptSession(body);
			} else if (status == 202 || status == 429 || status == 503) {
				pollDelay = status == 202 ? 5000
							  : std::min(60000, std::max(retry * 1000, pollDelay * 2));
				pollTimer.start(pollDelay);
			} else {
				CancelLogin();
				SetStatus(status == 403 ? tr("Sign-in declined or account not in the private beta.")
							: tr("Sign-in interrupted. Try again."));
			}
		});
}

void AeriumAccountWidget::AcceptSession(const QJsonObject &tokens)
{
	const auto access = tokens.value("access_token").toString().toLatin1();
	const auto refresh = tokens.value("refresh_token").toString().toLatin1();
	if (!ValidToken(access) || !ValidToken(refresh) || tokens.value("expires_in").toInt() != 900) {
		Revoke(refresh);
		ForgetSession(tr("Invalid session response. Sign in again."));
		return;
	}
	accessToken = access;
	refreshToken = refresh;
	SetBusy(tr("Signing in..."), true);
	CredentialTask([refresh] { return AeriumCredentials::Write(refresh); },
		       [this](const auto &result) {
			       remembered = result.success;
			       ValidateSession();
		       });
}

void AeriumAccountWidget::ValidateSession()
{
	Request("/v1/session/validate", {}, accessToken, [this](int status, const QJsonObject &body, int retry) {
		if (status == 200) {
			const auto user = body.value("user").toObject();
			const auto login = user.value("login").toString();
			static const QRegularExpression loginPattern("^[A-Za-z0-9_]{1,25}$");
			if (!loginPattern.match(login).hasMatch()) {
				Revoke(refreshToken);
				ForgetSession(tr("Invalid account response. Sign in again."));
				return;
			}
			const auto display = user.value("display_name").toString();
			username = display.compare(login, Qt::CaseInsensitive) == 0 ? display : login;
			ShowAccount();
			SetStatus(remembered ? QString()
					     : tr("Signed in for this session; Keychain could not save it."));
			sessionTimer.start(12 * 60 * 1000);
		} else if (status == 401 || status == 403) {
			ForgetSession(tr("Twitch session ended. Sign in again."));
		} else {
			RetryLater(tr("Account offline. Reconnecting..."), std::max(30, retry));
		}
	});
}

void AeriumAccountWidget::RefreshSession()
{
	if (!ValidToken(refreshToken) || busy) {
		return;
	}
	sessionTimer.stop();
	SetBusy(tr("Connecting Twitch..."), true);
	Request("/v1/session/refresh", {{"refresh_token", QString::fromLatin1(refreshToken)}}, {},
		[this](int status, const QJsonObject &body, int retry) {
			if (status == 200) {
				AcceptSession(body);
			} else if (status == 429 || status == 503) {
				RetryLater(tr("Account offline. Reconnecting..."), std::max(30, retry));
			} else {
				ForgetSession(tr("Session could not be restored. Sign in again."));
			}
		});
}

void AeriumAccountWidget::ShowAccount()
{
	busy = false;
	cancelButton->hide();
	accountButton->setEnabled(true);
	accountButton->setText(accountButton->fontMetrics().elidedText(username, Qt::ElideRight, 200));
	accountButton->setToolTip(username);
	accountButton->setAccessibleName(username);
	accountButton->setMenu(accountMenu);
	UpdateLoginWindow();
}

void AeriumAccountWidget::RetryLater(const QString &message, int seconds)
{
	busy = false;
	if (!username.isEmpty()) {
		ShowAccount();
	} else {
		accountButton->setText(tr("Reconnect Twitch"));
		accountButton->setEnabled(true);
		cancelButton->show();
	}
	SetStatus(message);
	sessionTimer.start(seconds * 1000);
}

void AeriumAccountWidget::Revoke(const QByteArray &token)
{
	if (ValidToken(token)) {
		Request("/v1/session/logout", {}, token, [](int, const QJsonObject &, int) {});
	}
}

void AeriumAccountWidget::ForgetSession(const QString &message)
{
	++generation;
	pollTimer.stop();
	sessionTimer.stop();
	verifier.clear();
	requestId.clear();
	accessToken.clear();
	refreshToken.clear();
	username.clear();
	remembered = false;
	busy = false;
	accountButton->setMenu(nullptr);
	accountButton->setText(tr("Sign in with Twitch"));
	accountButton->setToolTip({});
	accountButton->setAccessibleName(tr("Sign in with Twitch"));
	accountButton->setEnabled(true);
	cancelButton->hide();
	SetStatus(message);
	CredentialTask(AeriumCredentials::Remove, [this](const auto &result) {
		if (!result.success) {
			SetStatus(tr("Could not remove saved session from Keychain."));
		}
	});
}

void AeriumAccountWidget::CancelLogin()
{
	SignOut();
}

void AeriumAccountWidget::SignOut()
{
	const auto token = refreshToken;
	ForgetSession({});
	if (ValidToken(token)) {
		Request("/v1/session/logout", {}, token, [this](int status, const QJsonObject &, int) {
			if (status != 204) {
				SetStatus(tr("Signed out locally. Server session revocation could not be confirmed."));
			}
		});
	}
}
