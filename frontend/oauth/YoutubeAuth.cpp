#include "YoutubeAuth.hpp"

#ifdef BROWSER_AVAILABLE
#include <docks/YouTubeChatDock.hpp>
#endif
#include <oauth/AuthListener.hpp>
#include <utility/YoutubeApiWrappers.hpp>
#include <utility/obf.h>
#include <widgets/OBSBasic.hpp>

#include <qt-wrappers.hpp>
#include <ui-config.h>

#include <QDesktopServices>
#include <QRandomGenerator>

#include "moc_YoutubeAuth.cpp"

#define YOUTUBE_AUTH_URL "https://accounts.google.com/o/oauth2/v2/auth"
#define YOUTUBE_TOKEN_URL "https://www.googleapis.com/oauth2/v4/token"
#define YOUTUBE_SCOPE_VERSION 1
#define YOUTUBE_API_STATE_LENGTH 32
#define SECTION_NAME "YouTube"

#define YOUTUBE_CHAT_PLACEHOLDER_URL "https://obsproject.com/placeholders/youtube-chat"
#define YOUTUBE_CHAT_POPOUT_URL "https://www.youtube.com/live_chat?is_popout=1&dark_theme=1&v=%1"

#define YOUTUBE_CHAT_DOCK_NAME "ytChat"

static const char allowedChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
static const int allowedCount = static_cast<int>(sizeof(allowedChars) - 1);
/* ------------------------------------------------------------------------- */

static inline void OpenBrowser(const QString auth_uri)
{
	QUrl url(auth_uri, QUrl::StrictMode);
	QDesktopServices::openUrl(url);
}

static void DeleteCookies()
{
	if (panel_cookies) {
		panel_cookies->DeleteCookies("youtube.com", "");
		panel_cookies->DeleteCookies("google.com", "");
	}
}

void RegisterYoutubeAuth()
{
	for (auto &service : youtubeServices) {
		OAuth::RegisterOAuth(
			service, [service]() { return std::make_shared<YoutubeApiWrappers>(service); },
			YoutubeAuth::Login, DeleteCookies);
	}
}

YoutubeAuth::YoutubeAuth(const Def &d) : OAuthStreamKey(d), section(SECTION_NAME) {}

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
	config_set_string(main->Config(), service(), "DockState", main->saveState().toBase64().constData());

	const char *section_name = section.c_str();
	config_set_string(main->Config(), section_name, "RefreshToken", refresh_token.c_str());
	config_set_string(main->Config(), section_name, "Token", token.c_str());
	config_set_uint(main->Config(), section_name, "ExpireTime", expire_time);
	config_set_int(main->Config(), section_name, "ScopeVer", currentScopeVer);
}

static inline std::string get_config_str(OBSBasic *main, const char *section, const char *name)
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
	expire_time = config_get_uint(main->Config(), section_name, "ExpireTime");
	currentScopeVer = (int)config_get_int(main->Config(), section_name, "ScopeVer");
	firstLoad = false;
	return implicit ? !token.empty() : !refresh_token.empty();
}

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

	chat = new YoutubeChatDock(QTStr("Auth.Chat"));
	chat->setObjectName(YOUTUBE_CHAT_DOCK_NAME);
	chat->resize(300, 600);
	chat->setMinimumSize(200, 300);
	chat->setAllowedAreas(Qt::AllDockWidgetAreas);

	browser = cef->create_widget(chat, YOUTUBE_CHAT_PLACEHOLDER_URL, panel_cookies);

	chat->SetWidget(browser);
	main->AddDockWidget(chat, Qt::RightDockWidgetArea);

	chat->setFloating(true);
	chat->move(pos.x() + size.width() - chat->width() - 50, pos.y() + 50);

	if (firstLoad) {
		chat->setVisible(true);
	}
#endif

	main->NewYouTubeAppDock();

	if (!firstLoad) {
		const char *dockStateStr = config_get_string(main->Config(), service(), "DockState");
		QByteArray dockState = QByteArray::fromBase64(QByteArray(dockStateStr));

		if (main->isVisible() || !main->isMaximized())
			main->restoreState(dockState);
	}

	uiLoaded = true;
}

void YoutubeAuth::SetChatId(const QString &chat_id)
{
#ifdef BROWSER_AVAILABLE
	QString chat_url = QString(YOUTUBE_CHAT_POPOUT_URL).arg(chat_id);

	if (chat && chat->cefWidget) {
		chat->cefWidget->setURL(chat_url.toStdString());
	}
#else
	UNUSED_PARAMETER(chat_id);
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

void YoutubeAuth::ReloadChat()
{
#ifdef BROWSER_AVAILABLE
	if (chat && chat->cefWidget) {
		chat->cefWidget->reloadPage();
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
std::shared_ptr<Auth> YoutubeAuth::Login(QWidget *owner, const std::string &service)
{
	QString auth_code;
	AuthListener server;

	auto it = std::find_if(youtubeServices.begin(), youtubeServices.end(),
			       [service](auto &item) { return service == item.service; });
	if (it == youtubeServices.end()) {
		return nullptr;
	}
	const auto auth = std::make_shared<YoutubeApiWrappers>(*it);

	QString redirect_uri = QString("http://127.0.0.1:%1").arg(server.GetPort());

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
	QString url = url_template.arg(YOUTUBE_AUTH_URL, clientid.c_str(), redirect_uri, state);

	QString text = QTStr("YouTube.Auth.WaitingAuth.Text");
	text = text.arg(QString("<a href='%1'>Google OAuth Service</a>").arg(url));

	dlg.setText(text);
	dlg.setTextFormat(Qt::RichText);
	dlg.setStandardButtons(QMessageBox::StandardButton::Cancel);
#if defined(__APPLE__) && QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
	/* We can't show clickable links with the native NSAlert, so let's
	 * force the old non-native dialog instead. */
	dlg.setOption(QMessageBox::Option::DontUseNativeDialog);
#endif

	connect(&dlg, &QMessageBox::buttonClicked, &dlg, [&](QAbstractButton *) {
#ifdef _DEBUG
		blog(LOG_DEBUG, "Action Cancelled.");
#endif
		// TODO: Stop server.
		dlg.reject();
	});

	// Async Login.
	connect(&server, &AuthListener::ok, &dlg, [&dlg, &auth_code](QString code) {
#ifdef _DEBUG
		blog(LOG_DEBUG, "Got youtube redirected answer: %s", QT_TO_UTF8(code));
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

	auto open_external_browser = [url]() {
		OpenBrowser(url);
	};
	QScopedPointer<QThread> thread(CreateQThread(open_external_browser));
	thread->start();

#if defined(__APPLE__) && QT_VERSION >= QT_VERSION_CHECK(6, 5, 0) && QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
	const bool nativeDialogs = qApp->testAttribute(Qt::AA_DontUseNativeDialogs);
	App()->setAttribute(Qt::AA_DontUseNativeDialogs, true);
	dlg.exec();
	App()->setAttribute(Qt::AA_DontUseNativeDialogs, nativeDialogs);
#else
	dlg.exec();
#endif

	if (dlg.result() == QMessageBox::Cancel || dlg.result() == QDialog::Rejected)
		return nullptr;

	if (!auth->GetToken(YOUTUBE_TOKEN_URL, clientid, secret, QT_TO_UTF8(redirect_uri), YOUTUBE_SCOPE_VERSION,
			    QT_TO_UTF8(auth_code), true)) {
		return nullptr;
	}

	config_t *config = OBSBasic::Get()->Config();
	config_remove_value(config, "YouTube", "ChannelName");

	ChannelDescription cd;
	if (auth->GetChannelDescription(cd))
		config_set_string(config, "YouTube", "ChannelName", QT_TO_UTF8(cd.title));

	config_save_safe(config, "tmp", nullptr);
	return auth;
}
