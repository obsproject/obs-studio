#include "auth-twitch.hpp"

#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <qt-wrappers.hpp>
#include <obs-app.hpp>

#include "window-basic-main.hpp"
#include "remote-text.hpp"

#include <json11.hpp>

#include "ui-config.h"
#include "obf.h"

using namespace json11;

#include <browser-panel.hpp>
extern QCef *cef;
extern QCefCookieManager *panel_cookies;

/* ------------------------------------------------------------------------- */

#define TWITCH_AUTH_URL \
	"https://obsproject.com/app-auth/twitch?action=redirect"
#define ACCEPT_HEADER \
	"Accept: application/vnd.twitchtv.v5+json"

static Auth::Def twitchDef = {
	"Twitch",
	Auth::Type::OAuth_StreamKey
};

/* ------------------------------------------------------------------------- */

TwitchAuth::TwitchAuth(const Def &d)
	: OAuthStreamKey(d)
{
	cef->add_popup_whitelist_url(
			"https://twitch.tv/popout/frankerfacez/chat?ffz-settings",
			this);
	implicit = true;
}

bool TwitchAuth::GetChannelInfo()
try {
	if (token.empty())
		return false;

	std::string auth;
	auth += "Authorization: OAuth ";
	auth += token;

	std::string client_id = TWITCH_CLIENTID;
	deobfuscate_str(&client_id[0], TWITCH_HASH);

	std::vector<std::string> headers;
	headers.push_back(std::string("Client-ID: ") + client_id);
	headers.push_back(ACCEPT_HEADER);
	headers.push_back(std::move(auth));

	std::string output;
	std::string error;

	bool success = GetRemoteFileSafeBlock(
			"https://api.twitch.tv/kraken/channel",
			output,
			error,
			nullptr,
			"application/json",
			nullptr,
			headers,
			nullptr,
			5);
	if (!success || output.empty())
		throw ErrorInfo("Failed to get text from remote", error);

	Json json = Json::parse(output, error);
	if (!error.empty())
		throw ErrorInfo("Failed to parse json", error);

	name = json["name"].string_value();
	key_ = json["stream_key"].string_value();

	return true;
} catch (ErrorInfo info) {
	blog(LOG_DEBUG, "%s: %s: %s",
			__FUNCTION__,
			info.message.c_str(),
			info.error.c_str());
	return false;
}

void TwitchAuth::SaveInternal()
{
	OBSBasic *main = OBSBasic::Get();
	config_set_string(main->Config(), service(), "Name", name.c_str());
	config_set_string(main->Config(), service(), "DockState",
			main->saveState().toBase64().constData());
	OAuthStreamKey::SaveInternal();
}

static inline std::string get_config_str(
		OBSBasic *main,
		const char *section,
		const char *name)
{
	const char *val = config_get_string(main->Config(), section, name);
	return val ? val : "";
}

bool TwitchAuth::LoadInternal()
{
	OBSBasic *main = OBSBasic::Get();
	name = get_config_str(main, service(), "Name");
	firstLoad = false;
	return OAuthStreamKey::LoadInternal();
}

class TwitchChat : public QDockWidget {
public:
	inline TwitchChat() : QDockWidget() {}

	QScopedPointer<QCefWidget> widget;
};

static const char *ffz_script = "\
var ffz = document.createElement('script');\
ffz.setAttribute('src','https://cdn.frankerfacez.com/script/script.min.js');\
document.head.appendChild(ffz);";

static const char *bttv_script = "\
var bttv = document.createElement('script');\
bttv.setAttribute('src','https://cdn.betterttv.net/betterttv.js');\
document.head.appendChild(bttv);";

void TwitchAuth::LoadUI()
{
	if (uiLoaded)
		return;
	if (!GetChannelInfo())
		return;

	OBSBasic::InitBrowserPanelSafeBlock(false);
	OBSBasic *main = OBSBasic::Get();

	std::string url;
	url += "https://www.twitch.tv/popout/";
	url += name;
	url += "/chat?darkpopout";

	chat.reset(new TwitchChat());
	chat->setObjectName("twitchChat");
	chat->resize(300, 600);
	chat->setMinimumSize(200, 300);
	chat->setWindowTitle(QTStr("TwitchAuth.Chat"));
	chat->setAllowedAreas(Qt::AllDockWidgetAreas);

	QCefWidget *browser = cef->create_widget(nullptr, url, panel_cookies);
	chat->setWidget(browser);

	std::string script;
	script += ffz_script;
	script += bttv_script;

	browser->setStartupScript(script);

	main->addDockWidget(Qt::RightDockWidgetArea, chat.data());
	chatMenu.reset(main->AddDockWidgetMenu(chat.data()));

	chat->setFloating(true);
	if (firstLoad) {
		chat->setVisible(true);
	} else {
		const char *dockStateStr = config_get_string(main->Config(),
				service(), "DockState");
		QByteArray dockState =
			QByteArray::fromBase64(QByteArray(dockStateStr));
		main->restoreState(dockState);
	}

	uiLoaded = true;
}

std::shared_ptr<Auth> TwitchAuth::Login(QWidget *parent)
{
	OAuthLogin login(parent, TWITCH_AUTH_URL, true);
	if (login.exec() == QDialog::Rejected) {
		return nullptr;
	}

	std::shared_ptr<TwitchAuth> auth =
		std::make_shared<TwitchAuth>(twitchDef);
	auth->token = QT_TO_UTF8(login.GetCode());

	std::string error;
	if (auth->GetChannelInfo()) {
		return auth;
	}

	return nullptr;
}

void TwitchAuth::OnStreamConfig()
{
	OBSBasic *main = OBSBasic::Get();
	obs_service_t *service = main->GetService();

	obs_data_t *settings = obs_service_get_settings(service);

	obs_data_set_string(settings, "key", key_.c_str());
	obs_service_update(service, settings);

	obs_data_release(settings);
}

static std::shared_ptr<Auth> CreateTwitchAuth()
{
	return std::make_shared<TwitchAuth>(twitchDef);
}

static void DeleteCookies()
{
	if (panel_cookies)
		panel_cookies->DeleteCookies("twitch.tv", std::string());
}

void RegisterTwitchAuth()
{
	OAuth::RegisterOAuth(
			twitchDef,
			CreateTwitchAuth,
			TwitchAuth::Login,
			DeleteCookies);
}
