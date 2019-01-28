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
	"https://obsproject.com/app-auth/twitch-auth.php?action=redirect"
#define TWITCH_TOKEN_URL \
	"https://obsproject.com/app-auth/twitch-token.php"
#define ACCEPT_HEADER \
	"Accept: application/vnd.twitchtv.v5+json"

#define TWITCH_SCOPE_VERSION 1

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
	uiLoadTimer.setSingleShot(true);
	uiLoadTimer.setInterval(500);
	connect(&uiLoadTimer, &QTimer::timeout,
			this, &TwitchAuth::TryLoadSecondaryUIPanes);
}

bool TwitchAuth::TokenExpired()
{
	if (token.empty())
		return true;
	if ((uint64_t)time(nullptr) > expire_time - 5)
		return true;
	return false;
}

bool TwitchAuth::RetryLogin()
{
	OAuthLogin login(OBSBasic::Get(), TWITCH_AUTH_URL, false);
	if (login.exec() == QDialog::Rejected) {
		return false;
	}

	return GetToken(QT_TO_UTF8(login.GetCode()), true);
}

bool TwitchAuth::GetToken(const std::string &auth_code, bool retry)
try {
	std::string output;
	std::string error;
	std::string desc;

	if (currentScopeVer > 0 && currentScopeVer < TWITCH_SCOPE_VERSION) {
		if (RetryLogin()) {
			return true;
		} else {
			QString title = QTStr("Auth.InvalidScope.Title");
			QString text = QTStr("Auth.InvalidScope.Text")
				.arg("Twitch");

			QMessageBox::warning(OBSBasic::Get(), title, text);
		}
	}

	if (auth_code.empty() && !TokenExpired()) {
		return true;
	}

	std::string client_id = TWITCH_CLIENTID;
	deobfuscate_str(&client_id[0], TWITCH_HASH);

	std::string post_data;
	post_data += "action=redirect&client_id=";
	post_data += client_id;

	if (!auth_code.empty()) {
		post_data += "&grant_type=authorization_code&code=";
		post_data += auth_code;
	} else {
		post_data += "&grant_type=refresh_token&refresh_token=";
		post_data += refresh_token;
	}

	bool success = GetRemoteFileSafeBlock(
			TWITCH_TOKEN_URL,
			output,
			error,
			nullptr,
			"application/x-www-form-urlencoded",
			post_data.c_str(),
			std::vector<std::string>(),
			nullptr,
			5);
	if (!success || output.empty())
		throw ErrorInfo("Failed to get token from remote", error);

	Json json = Json::parse(output, error);
	if (!error.empty())
		throw ErrorInfo("Failed to parse json", error);

	/* -------------------------- */
	/* error handling             */

	error = json["error"].string_value();
	if (!retry && error == "invalid_grant") {
		if (RetryLogin()) {
			return true;
		}
	}
	if (!error.empty())
		throw ErrorInfo(error, json["error_description"].string_value());

	/* -------------------------- */
	/* success!                   */

	expire_time = (uint64_t)time(nullptr) + json["expires_in"].int_value();
	token       = json["access_token"].string_value();
	if (token.empty())
		throw ErrorInfo("Failed to get token from remote", error);

	if (!auth_code.empty()) {
		refresh_token = json["refresh_token"].string_value();
		if (refresh_token.empty())
			throw ErrorInfo("Failed to get refresh token from "
					"remote", error);

		currentScopeVer = TWITCH_SCOPE_VERSION;
	}

	return true;

} catch (ErrorInfo info) {
	if (!retry) {
		QString title = QTStr("Auth.AuthFailure.Title");
		QString text = QTStr("Auth.AuthFailure.Text")
			.arg("Twitch", info.message.c_str(), info.error.c_str());

		QMessageBox::warning(OBSBasic::Get(), title, text);
	}

	blog(LOG_WARNING, "%s: %s: %s",
			__FUNCTION__,
			info.message.c_str(),
			info.error.c_str());
	return false;
}

bool TwitchAuth::GetChannelInfo()
try {
	if (!GetToken())
		return false;
	if (token.empty())
		return false;
	if (!key_.empty())
		return true;

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

	error = json["error"].string_value();
	if (!error.empty())
		throw ErrorInfo(error, json["error_description"].string_value());

	name = json["name"].string_value();
	key_ = json["stream_key"].string_value();

	return true;
} catch (ErrorInfo info) {
	blog(LOG_WARNING, "%s: %s: %s",
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
	config_set_int(main->Config(), service(), "ScopeVer", currentScopeVer);
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
	currentScopeVer = (int)config_get_int(main->Config(), service(),
			"scope_ver");
	firstLoad = false;
	return OAuthStreamKey::LoadInternal();
}

class TwitchWidget : public QDockWidget {
public:
	inline TwitchWidget() : QDockWidget() {}

	QScopedPointer<QCefWidget> widget;

	inline void SetWidget(QCefWidget *widget_)
	{
		setWidget(widget_);
		widget.reset(widget_);
	}
};

static const char *ffz_script = "\
var ffz = document.createElement('script');\
ffz.setAttribute('src','https://cdn.frankerfacez.com/script/script.min.js');\
document.head.appendChild(ffz);";

static const char *bttv_script = "\
localStorage.setItem('bttv_darkenedMode', true);\
var bttv = document.createElement('script');\
bttv.setAttribute('src','https://cdn.betterttv.net/betterttv.js');\
document.head.appendChild(bttv);";

static const char *referrer_script1 = "\
Object.defineProperty(document, 'referrer', {get : function() { return '";
static const char *referrer_script2 = "'; }});";

void TwitchAuth::LoadUI()
{
	if (uiLoaded)
		return;
	if (!GetToken())
		return;
	if (!GetChannelInfo())
		return;

	OBSBasic::InitBrowserPanelSafeBlock(false);
	OBSBasic *main = OBSBasic::Get();

	QCefWidget *browser;
	std::string url;
	std::string script;

	/* ----------------------------------- */

	url = "https://www.twitch.tv/popout/";
	url += name;
	url += "/chat";

	chat.reset(new TwitchWidget());
	chat->setObjectName("twitchChat");
	chat->resize(300, 600);
	chat->setMinimumSize(200, 300);
	chat->setWindowTitle(QTStr("Auth.Chat"));
	chat->setAllowedAreas(Qt::AllDockWidgetAreas);

	browser = cef->create_widget(nullptr, url, panel_cookies);
	chat->SetWidget(browser);

	script = bttv_script;
	script += ffz_script;
	browser->setStartupScript(script);

	main->addDockWidget(Qt::RightDockWidgetArea, chat.data());
	chatMenu.reset(main->AddDockWidgetMenu(chat.data()));

	/* ----------------------------------- */

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

	TryLoadSecondaryUIPanes();

	uiLoaded = true;
}

void TwitchAuth::LoadSecondaryUIPanes()
{
	OBSBasic *main = OBSBasic::Get();

	QCefWidget *browser;
	std::string url;
	std::string script;

	/* ----------------------------------- */

	url = "https://www.twitch.tv/popout/";
	url += name;
	url += "/dashboard/live/stream-info";

	info.reset(new TwitchWidget());
	info->setObjectName("twitchInfo");
	info->resize(300, 600);
	info->setMinimumSize(200, 300);
	info->setWindowTitle(QTStr("Auth.StreamInfo"));
	info->setAllowedAreas(Qt::AllDockWidgetAreas);

	browser = cef->create_widget(nullptr, url, panel_cookies);
	info->SetWidget(browser);

	script = "localStorage.setItem('twilight.theme', 1);";
	script += referrer_script1;
	script += "https://www.twitch.tv/";
	script += name;
	script += "/dashboard/live";
	script += referrer_script2;
	browser->setStartupScript(script);

	main->addDockWidget(Qt::RightDockWidgetArea, info.data());
	infoMenu.reset(main->AddDockWidgetMenu(info.data()));

	/* ----------------------------------- */

	info->setFloating(true);
	if (firstLoad) {
		info->setVisible(true);
	} else {
		const char *dockStateStr = config_get_string(main->Config(),
				service(), "DockState");
		QByteArray dockState =
			QByteArray::fromBase64(QByteArray(dockStateStr));
		main->restoreState(dockState);
	}
}

/* Twitch.tv has an OAuth for itself.  If we try to load multiple panel pages
 * at once before it's OAuth'ed itself, they will all try to perform the auth
 * process at the same time, get their own request codes, and only the last
 * code will be valid -- so one or more panels are guaranteed to fail.
 *
 * To solve this, we want to load just one panel first (the chat), and then all
 * subsequent panels should only be loaded once we know that Twitch has auth'ed
 * itself (if the cookie "auth-token" exists for twitch.tv).
 *
 * This is annoying to deal with. */
void TwitchAuth::TryLoadSecondaryUIPanes()
{
	QPointer<TwitchAuth> this_ = this;

	auto cb = [this_] (bool found)
	{
		if (!this_) {
			return;
		}

		if (!found) {
			QMetaObject::invokeMethod(&this_->uiLoadTimer,
					"start");
		} else {
			QMetaObject::invokeMethod(this_, "LoadSecondaryUIPanes");
		}
	};

	panel_cookies->CheckForCookie("https://www.twitch.tv", "auth-token", cb);
}

std::shared_ptr<Auth> TwitchAuth::Login(QWidget *parent)
{
	OAuthLogin login(parent, TWITCH_AUTH_URL, false);
	if (login.exec() == QDialog::Rejected) {
		return nullptr;
	}

	std::shared_ptr<TwitchAuth> auth = std::make_shared<TwitchAuth>(twitchDef);
	if (!auth->GetToken(QT_TO_UTF8(login.GetCode()))) {
		return nullptr;
	}

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
