#include "auth-youtube.hpp"

#include <iostream>
#include <QMessageBox>
#include <QThread>
#include <vector>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QUrl>
#include <QRandomGenerator>

#ifdef WIN32
#include <windows.h>
#include <shellapi.h>

#pragma comment(lib, "shell32")
#endif

#include "auth-listener.hpp"
#include "obs-app.hpp"
#include "qt-wrappers.hpp"
#include "ui-config.h"
#include "youtube-api-wrappers.hpp"
#include "window-basic-main.hpp"
#include "obf.h"

#ifdef BROWSER_AVAILABLE
#include "window-dock-browser.hpp"
#endif

using namespace json11;

/* ------------------------------------------------------------------------- */
#define YOUTUBE_AUTH_URL "https://accounts.google.com/o/oauth2/v2/auth"
#define YOUTUBE_TOKEN_URL "https://www.googleapis.com/oauth2/v4/token"
#define YOUTUBE_SCOPE_VERSION 1
#define YOUTUBE_API_STATE_LENGTH 32
#define SECTION_NAME "YouTube"

#define YOUTUBE_CHAT_PLACEHOLDER_URL \
	"https://obsproject.com/placeholders/youtube-chat"
#define YOUTUBE_CHAT_POPOUT_URL \
	"https://www.youtube.com/live_chat?is_popout=1&dark_theme=1&v=%1"

#define YOUTUBE_CHAT_DOCK_NAME "ytChat"

static const char allowedChars[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
static const int allowedCount = static_cast<int>(sizeof(allowedChars) - 1);
/* ------------------------------------------------------------------------- */

static inline void OpenBrowser(const QString auth_uri)
{
	QUrl url(auth_uri, QUrl::StrictMode);
	QDesktopServices::openUrl(url);
}

void RegisterYoutubeAuth()
{
	for (auto &service : youtubeServices) {
		OAuth::RegisterOAuth(
			service,
			[service]() {
				return std::make_shared<YoutubeApiWrappers>(
					service);
			},
			YoutubeAuth::Login, []() { return; });
	}
}

YoutubeAuth::YoutubeAuth(const Def &d)
	: OAuthStreamKey(d),
	  section(SECTION_NAME)
{
}

YoutubeAuth::~YoutubeAuth()
{
	if (!uiLoaded)
		return;

#ifdef BROWSER_AVAILABLE
	OBSBasic *main = OBSBasic::Get();

	main->RemoveDockWidget(YOUTUBE_CHAT_DOCK_NAME);
	chat = nullptr;
#endif
}

bool YoutubeAuth::RetryLogin()
{
	return true;
}

void YoutubeAuth::SaveInternal()
{
	OBSBasic *main = OBSBasic::Get();
	config_set_string(main->Config(), service(), "DockState",
			  main->saveState().toBase64().constData());

	const char *section_name = section.c_str();
	config_set_string(main->Config(), section_name, "RefreshToken",
			  refresh_token.c_str());
	config_set_string(main->Config(), section_name, "Token", token.c_str());
	config_set_uint(main->Config(), section_name, "ExpireTime",
			expire_time);
	config_set_int(main->Config(), section_name, "ScopeVer",
		       currentScopeVer);
}

static inline std::string get_config_str(OBSBasic *main, const char *section,
					 const char *name)
{
	const char *val = config_get_string(main->Config(), section, name);
	return val ? val : "";
}

bool YoutubeAuth::LoadInternal()
{
	OBSBasic *main = OBSBasic::Get();

	const char *section_name = section.c_str();
	refresh_token = get_config_str(main, section_name, "RefreshToken");
	token = get_config_str(main, section_name, "Token");
	expire_time =
		config_get_uint(main->Config(), section_name, "ExpireTime");
	currentScopeVer =
		(int)config_get_int(main->Config(), section_name, "ScopeVer");
	firstLoad = false;
	return implicit ? !token.empty() : !refresh_token.empty();
}

#ifdef BROWSER_AVAILABLE
static const char *ytchat_script = "\
const obsCSS = document.createElement('style');\
obsCSS.innerHTML = \"#panel-pages.yt-live-chat-renderer {display: none;}\
yt-live-chat-viewer-engagement-message-renderer {display: none;}\";\
document.querySelector('head').appendChild(obsCSS);";
#endif

void YoutubeAuth::LoadUI()
{
	if (uiLoaded)
		return;

#ifdef BROWSER_AVAILABLE
	if (!cef)
		return;

	OBSBasic::InitBrowserPanelSafeBlock();
	OBSBasic *main = OBSBasic::Get();

	QCefWidget *browser;

	QSize size = main->frameSize();
	QPoint pos = main->pos();

	chat = new YoutubeChatDock();
	chat->setObjectName(YOUTUBE_CHAT_DOCK_NAME);
	chat->resize(300, 600);
	chat->setMinimumSize(200, 300);
	chat->setWindowTitle(QTStr("Auth.Chat"));
	chat->setAllowedAreas(Qt::AllDockWidgetAreas);

	browser = cef->create_widget(chat, YOUTUBE_CHAT_PLACEHOLDER_URL,
				     panel_cookies);
	browser->setStartupScript(ytchat_script);

	chat->SetWidget(browser);
	main->AddDockWidget(chat, Qt::RightDockWidgetArea);

	chat->setFloating(true);
	chat->move(pos.x() + size.width() - chat->width() - 50, pos.y() + 50);

	if (firstLoad) {
		chat->setVisible(true);
	} else {
		const char *dockStateStr = config_get_string(
			main->Config(), service(), "DockState");
		QByteArray dockState =
			QByteArray::fromBase64(QByteArray(dockStateStr));

		if (main->isVisible() || !main->isMaximized())
			main->restoreState(dockState);
	}
#endif

	uiLoaded = true;
}

void YoutubeAuth::SetChatId(const QString &chat_id,
			    const std::string &api_chat_id)
{
#ifdef BROWSER_AVAILABLE
	QString chat_url = QString(YOUTUBE_CHAT_POPOUT_URL).arg(chat_id);

	if (chat && chat->cefWidget) {
		chat->cefWidget->setURL(chat_url.toStdString());
		chat->SetApiChatId(api_chat_id);
	}
#else
	UNUSED_PARAMETER(chat_id);
	UNUSED_PARAMETER(api_chat_id);
#endif
}

void YoutubeAuth::ResetChat()
{
#ifdef BROWSER_AVAILABLE
	if (chat && chat->cefWidget) {
		chat->cefWidget->setURL(YOUTUBE_CHAT_PLACEHOLDER_URL);
	}
#endif
}

QString YoutubeAuth::GenerateState()
{
	char state[YOUTUBE_API_STATE_LENGTH + 1];
	QRandomGenerator *rng = QRandomGenerator::system();
	int i;

	for (i = 0; i < YOUTUBE_API_STATE_LENGTH; i++)
		state[i] = allowedChars[rng->bounded(0, allowedCount)];
	state[i] = 0;

	return state;
}

// Static.
std::shared_ptr<Auth> YoutubeAuth::Login(QWidget *owner,
					 const std::string &service)
{
	QString auth_code;
	AuthListener server;

	auto it = std::find_if(youtubeServices.begin(), youtubeServices.end(),
			       [service](auto &item) {
				       return service == item.service;
			       });
	if (it == youtubeServices.end()) {
		return nullptr;
	}
	const auto auth = std::make_shared<YoutubeApiWrappers>(*it);

	QString redirect_uri =
		QString("http://127.0.0.1:%1").arg(server.GetPort());

	QMessageBox dlg(owner);
	dlg.setWindowFlags(dlg.windowFlags() & ~Qt::WindowCloseButtonHint);
	dlg.setWindowTitle(QTStr("YouTube.Auth.WaitingAuth.Title"));

	std::string clientid = YOUTUBE_CLIENTID;
	std::string secret = YOUTUBE_SECRET;
	deobfuscate_str(&clientid[0], YOUTUBE_CLIENTID_HASH);
	deobfuscate_str(&secret[0], YOUTUBE_SECRET_HASH);

	QString state;
	state = auth->GenerateState();
	server.SetState(state);

	QString url_template;
	url_template += "%1";
	url_template += "?response_type=code";
	url_template += "&client_id=%2";
	url_template += "&redirect_uri=%3";
	url_template += "&state=%4";
	url_template += "&scope=https://www.googleapis.com/auth/youtube";
	QString url = url_template.arg(YOUTUBE_AUTH_URL, clientid.c_str(),
				       redirect_uri, state);

	QString text = QTStr("YouTube.Auth.WaitingAuth.Text");
	text = text.arg(
		QString("<a href='%1'>Google OAuth Service</a>").arg(url));

	dlg.setText(text);
	dlg.setTextFormat(Qt::RichText);
	dlg.setStandardButtons(QMessageBox::StandardButton::Cancel);

	connect(&dlg, &QMessageBox::buttonClicked, &dlg,
		[&](QAbstractButton *) {
#ifdef _DEBUG
			blog(LOG_DEBUG, "Action Cancelled.");
#endif
			// TODO: Stop server.
			dlg.reject();
		});

	// Async Login.
	connect(&server, &AuthListener::ok, &dlg,
		[&dlg, &auth_code](QString code) {
#ifdef _DEBUG
			blog(LOG_DEBUG, "Got youtube redirected answer: %s",
			     QT_TO_UTF8(code));
#endif
			auth_code = code;
			dlg.accept();
		});
	connect(&server, &AuthListener::fail, &dlg, [&dlg]() {
#ifdef _DEBUG
		blog(LOG_DEBUG, "No access granted");
#endif
		dlg.reject();
	});

	auto open_external_browser = [url]() { OpenBrowser(url); };
	QScopedPointer<QThread> thread(CreateQThread(open_external_browser));
	thread->start();

	dlg.exec();
	if (dlg.result() == QMessageBox::Cancel ||
	    dlg.result() == QDialog::Rejected)
		return nullptr;

	if (!auth->GetToken(YOUTUBE_TOKEN_URL, clientid, secret,
			    QT_TO_UTF8(redirect_uri), YOUTUBE_SCOPE_VERSION,
			    QT_TO_UTF8(auth_code), true)) {
		return nullptr;
	}

	config_t *config = OBSBasic::Get()->Config();
	config_remove_value(config, "YouTube", "ChannelName");

	ChannelDescription cd;
	if (auth->GetChannelDescription(cd))
		config_set_string(config, "YouTube", "ChannelName",
				  QT_TO_UTF8(cd.title));

	config_save_safe(config, "tmp", nullptr);
	return auth;
}

#ifdef BROWSER_AVAILABLE
void YoutubeChatDock::SetWidget(QCefWidget *widget_)
{
	lineEdit = new LineEditAutoResize();
	lineEdit->setVisible(false);
	lineEdit->setMaxLength(200);
	lineEdit->setPlaceholderText(QTStr("YouTube.Chat.Input.Placeholder"));
	sendButton = new QPushButton(QTStr("YouTube.Chat.Input.Send"));
	sendButton->setVisible(false);

	chatLayout = new QHBoxLayout();
	chatLayout->setContentsMargins(0, 0, 0, 0);
	chatLayout->addWidget(lineEdit, 1);
	chatLayout->addWidget(sendButton);

	QVBoxLayout *layout = new QVBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(widget_, 1);
	layout->addLayout(chatLayout);

	QWidget *widget = new QWidget();
	widget->setLayout(layout);
	setWidget(widget);

	QWidget::connect(lineEdit, SIGNAL(returnPressed()), this,
			 SLOT(SendChatMessage()));
	QWidget::connect(sendButton, SIGNAL(pressed()), this,
			 SLOT(SendChatMessage()));

	cefWidget.reset(widget_);
}

void YoutubeChatDock::SetApiChatId(const std::string &id)
{
	this->apiChatId = id;
	QMetaObject::invokeMethod(this, "EnableChatInput",
				  Qt::QueuedConnection);
}

void YoutubeChatDock::SendChatMessage()
{
	const QString message = lineEdit->text();
	if (message == "")
		return;

	OBSBasic *main = OBSBasic::Get();
	YoutubeApiWrappers *apiYouTube(
		dynamic_cast<YoutubeApiWrappers *>(main->GetAuth()));

	ExecuteFuncSafeBlock([&]() {
		lineEdit->setText("");
		lineEdit->setPlaceholderText(
			QTStr("YouTube.Chat.Input.Sending"));
		if (apiYouTube->SendChatMessage(apiChatId, message)) {
			os_sleep_ms(3000);
		} else {
			QString error = apiYouTube->GetLastError();
			apiYouTube->GetTranslatedError(error);
			QMetaObject::invokeMethod(
				this, "ShowErrorMessage", Qt::QueuedConnection,
				Q_ARG(const QString &, error));
		}
		lineEdit->setPlaceholderText(
			QTStr("YouTube.Chat.Input.Placeholder"));
	});
}

void YoutubeChatDock::ShowErrorMessage(const QString &error)
{
	QMessageBox::warning(this, QTStr("YouTube.Chat.Error.Title"),
			     QTStr("YouTube.Chat.Error.Text").arg(error));
}

void YoutubeChatDock::EnableChatInput()
{
	lineEdit->setVisible(true);
	sendButton->setVisible(true);
}
#endif
