#include "auth-onlyfans.hpp"
//-------------------------------------------------------------------------//
#include <json11.hpp>
//-------------------------------------------------------------------------//
#include <QRegularExpression>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QUuid>
//-------------------------------------------------------------------------//
#include <qt-wrappers.hpp>
//-------------------------------------------------------------------------//
#include <obs-app.hpp>
//-------------------------------------------------------------------------//
#include "window-dock-browser.hpp"
#include "window-basic-main.hpp"
#include "window-basic-settings.hpp"
#include "remote-text.hpp"

#include "ui-config.h"
#include "obf.h"
//-------------------------------------------------------------------------//
#include "auth-oauth-onlyfans.hpp"
//-------------------------------------------------------------------------//
using namespace json11;
//-------------------------------------------------------------------------//
#define SERVICE_NAME "OnlyFans.com"
//-------------------------------------------------------------------------//
#define OAUTH_OF_BASE_URL "https://obs-app.onlyfans.com/api/v1/"
#define OAUTH_OF_LOGIN_PATH "/login?redirectTo=%2Flogin%2Ffinalise&credentials=true"
#define ONLYFANS_SETTING_DOCK_NAME "onlyfansSettings"
//-------------------------------------------------------------------------//
class OBSBasicSettings;
//-------------------------------------------------------------------------//
static Auth::Def onlyfansDef = {SERVICE_NAME, Auth::Type::OAuth_StreamKey};
// Keeps stream started flag.
static bool stream_started = false;
// Keeps Onlyfans frontend url.
static std::string of_base_url = OF_BASE_URL;
// Keeps Onlyfans backend url.
static std::string oauth_of_base_url = OAUTH_OF_BASE_URL;
//-------------------------------------------------------------------------//
static inline std::string get_config_str(OBSBasic *main, const char *section, const char *name)
{
	const char *val = config_get_string(main->Config(), section, name);
	return val ? val : "";
}

static void onstream_event(enum obs_frontend_event event, void * /*private_data*/)
{
	if (event == OBS_FRONTEND_EVENT_STREAMING_STARTED) {
		stream_started = true;
	} else if (event == OBS_FRONTEND_EVENT_STREAMING_STOPPED) {
		stream_started = false;
	}
}

static void try_override_of_urls()
{
	// Trying to read Onlyfans base url from environment.
	auto url = getenv("OF_BASE_URL");

	if (url != nullptr) {
		// Overriding the base url.
		of_base_url = url;
	}
	// Removing the last slash.
	if (of_base_url.back() == '/') {
		of_base_url.pop_back();
	}

	// Trying to read Onlyfans backend url from environment.
	url = getenv("OAUTH_OF_BASE_URL");

	if (url != nullptr) {
		// Overriding the base url.
		oauth_of_base_url = url;
	}

	if (oauth_of_base_url.back() != '/') {
		// Removing the last symbol.
		oauth_of_base_url.push_back('/');
	}
}
//-------------------------------------------------------------------------//
std::shared_ptr<Auth> OnlyfansAuth::Login(QWidget *parent, const std::string &)
{
	OAuthOnlyfansLogin login(parent, of_base_url + OAUTH_OF_LOGIN_PATH, of_base_url);

	if (login.exec() == QDialog::Rejected) {
		return nullptr;
	}

	std::shared_ptr<OnlyfansAuth> auth = std::make_shared<OnlyfansAuth>(onlyfansDef, QT_TO_UTF8(login.GetToken()),
									    QT_TO_UTF8(login.GetRefreshToken()));

	if (auth->GetChannelInfo()) {
		return auth;
	}
	return nullptr;
}
//-------------------------------------------------------------------------//
OnlyfansAuth::OnlyfansAuth(const Def &d) : base_class(d)
{
	if (cef == nullptr) {
		return;
	}

	// Loading internal data.
	base_class::LoadInternal();

	if (not this->refresh_token.empty() &&
	    not this->GetToken(oauth_of_base_url + "refreshToken", this->refresh_token)) {
		// Clearing tokens.
		this->refresh_token.clear(), this->token.clear();
		this->key_.clear(), this->server_.clear();

		throw(std::invalid_argument("Client's token is invalid or not connected"));
	}

	cef->add_popup_whitelist_url(of_base_url, this);
	/* enables javascript-based popups.  basically bttv popups */
	cef->add_popup_whitelist_url("about:blank#blocked", this);

	this->updaterTimer.setSingleShot(false);
	this->updaterTimer.setInterval(1000);

	// Creating a timer which watches for panel.
	QObject::connect(&this->updaterTimer, &QTimer::timeout, this, &OnlyfansAuth::updateSettingsUIPanes);
}

OnlyfansAuth::OnlyfansAuth(const Def &d, const std::string &token, const std::string &refresh_token) : base_class(d)
{
	base_class::token = token;
	base_class::refresh_token = refresh_token;

	cef->add_popup_whitelist_url(of_base_url, this);
	/* enables javascript-based popups.  basically bttv popups */
	cef->add_popup_whitelist_url("about:blank#blocked", this);

	this->updaterTimer.setSingleShot(false);
	this->updaterTimer.setInterval(1000);

	// Creating a timer which watches for panel.
	QObject::connect(&this->updaterTimer, &QTimer::timeout, this, &OnlyfansAuth::updateSettingsUIPanes);
}

OnlyfansAuth::~OnlyfansAuth() noexcept
{
	if (not this->uiLoaded) {
		return;
	}

	if (this->updaterTimer.isActive()) {
		this->updaterTimer.stop();
	}

	if (this->cefWidget != nullptr) {
		obs_frontend_remove_event_callback(onstream_event, this->cefWidget);
	}

	OBSBasic *main = OBSBasic::Get();

	main->RemoveDockWidget(ONLYFANS_SETTING_DOCK_NAME);
}
//-------------------------------------------------------------------------//
bool OnlyfansAuth::RetryLogin()
{
	OAuthOnlyfansLogin login(OBSBasic::Get(), of_base_url + OAUTH_OF_LOGIN_PATH, of_base_url);

	if (login.exec() == QDialog::Rejected) {
		return false;
	}

	return !login.GetToken().isEmpty() ||
	       GetToken(oauth_of_base_url + "refreshToken", QT_TO_UTF8(login.GetRefreshToken()));
}

void OnlyfansAuth::SaveInternal()
{
	OBSBasic *main = OBSBasic::Get();

	// Saving parameters.
	config_set_string(main->Config(), service(), "Name", this->name.c_str());
	config_set_string(main->Config(), service(), "UUID", this->uuid.c_str());
	config_set_string(main->Config(), service(), "StreamKey", this->key_.c_str());
	config_set_string(main->Config(), service(), "StreamServer", this->server_.c_str());

	if (this->uiLoaded) {
		config_set_string(main->Config(), service(), "DockState", main->saveState().toBase64().constData());
	}
	base_class::SaveInternal();
}

bool OnlyfansAuth::LoadInternal()
{
	if (cef == nullptr)
		return false;

	OBSBasic *main = OBSBasic::Get();

	if (this->token.empty()) {
		base_class::LoadInternal();
	}

	// Loading parameters.
	this->name = get_config_str(main, service(), "Name");
	this->uuid = get_config_str(main, service(), "UUID");
	this->key_ = get_config_str(main, service(), "StreamKey");
	this->server_ = get_config_str(main, service(), "StreamServer");

	this->firstLoad = false;

	// Clearing loaded token.
	//<???> this->token.clear();

	return not this->token.empty();
}

void OnlyfansAuth::LoadUI()
{
	if (cef == nullptr)
		return;
	if (this->uiLoaded)
		return;
	if (not this->GetChannelInfo())
		return;

	std::string script;
	OBSBasic::InitBrowserPanelSafeBlock();
	auto *main = OBSBasic::Get();

	/* The panels require a UUID, it does not actually need to be unique,
	 * and is generated client-side.
	 * It is only for preferences stored in the browser's local store.
	 */
	if (this->uuid.empty()) {
		QString qtUuid = QUuid::createUuid().toString();
		qtUuid.replace(QRegularExpression("[{}-]"), "");
		uuid = qtUuid.toStdString();
	}

	std::string url = QString("%1/%2").arg(of_base_url.c_str(), "stream-settings").toStdString();
	/* ----------------------------------- */

	QSize size = main->frameSize();
	QPoint pos = main->pos();

	auto settings = new BrowserDock(QTStr("Settings"));

	settings->setObjectName(ONLYFANS_SETTING_DOCK_NAME);
	settings->resize(300, 600);
	settings->setMinimumSize(420, 420);
	settings->setAllowedAreas(Qt::AllDockWidgetAreas);

	this->cefWidget = cef->create_widget(settings, url, panel_cookies);
	settings->SetWidget(this->cefWidget);
	cef->add_force_popup_url(of_base_url, settings);

	if (App()->IsThemeDark()) {
		script = "localStorage.setItem('twilight.theme', 1);";
	} else {
		script = "localStorage.setItem('twilight.theme', 0);";
	}
	this->cefWidget->setStartupScript(script);

	main->AddDockWidget(settings, Qt::RightDockWidgetArea);
	/* ----------------------------------- */

	settings->setFloating(true);
	settings->move(pos.x() + size.width() - settings->width() - 50, pos.y() + 50);

	if (this->firstLoad) {
		settings->setVisible(true);
	} else {
		const char *dockStateStr = config_get_string(main->Config(), service(), "DockState");
		QByteArray dockState = QByteArray::fromBase64(QByteArray(dockStateStr));

		if (main->isVisible() || !main->isMaximized())
			main->restoreState(dockState);
	}

	obs_frontend_add_event_callback(onstream_event, this->cefWidget);

	this->uiLoaded = true;
	// Starting the timer.
	this->updaterTimer.start();
}
//-------------------------------------------------------------------------//
bool OnlyfansAuth::MakeApiRequest(const char *path, Json &json_out)
{
	std::string url = oauth_of_base_url + std::string(path);

	std::vector<std::string> headers;

	headers.emplace_back("Authorization: Bearer " + this->token);

	std::string output;
	std::string error;
	long error_code = 0;
	bool success = false;
	auto func = [&]() {
		success = GetRemoteFile(url.c_str(), output, error, &error_code, "application/json", "GET", nullptr,
					headers, nullptr, 5);
	};

	ExecThreadedWithoutBlocking(func, QTStr("Auth.LoadingChannel.Title"),
				    QTStr("Auth.LoadingChannel.Text").arg(service()));
	if (error_code == 403) {
		OBSMessageBox::warning(OBSBasic::Get(), Str("OnlyfansAuth.TwoFactorFail.Title"),
				       Str("OnlyfansAuth.TwoFactorFail.Text"), true);
		blog(LOG_WARNING, "%s: %s", __FUNCTION__,
		     "Got 403 from Twitch, user probably does not "
		     "have two-factor authentication enabled on "
		     "their account");
		return false;
	}

	if (not success || output.empty()) {
		throw ErrorInfo("Failed to get text from remote", error);
	}

	json_out = Json::parse(output, error);
	if (not error.empty()) {
		throw ErrorInfo("Failed to parse json", error);
	}

	if (json_out["statusCode"].int_value() != 200) {
		throw ErrorInfo("Request failed",
				"Receive code: " + std::to_string(json_out["statusCode"].int_value()));
	}
	return true;
}

bool OnlyfansAuth::GetChannelInfo()
{
	try {
		if (this->token.empty() && !GetToken(oauth_of_base_url + "refreshToken", this->refresh_token)) {
			return false;
		}
		if (this->token.empty())
			return false;

		if (!this->key_.empty())
			return true;

		Json json;
		// Trying to get a stream key.
		auto success = MakeApiRequest("user/data", json);
		if (not success)
			return false;

		// Getting a name of user.
		this->name = json["data"]["username"].string_value();
		// Getting a stream key.
		this->key_ = json["data"]["streaming"]["streamKey"].string_value();
		// Getting a stream key.
		this->server_ = json["data"]["streaming"]["streamServer"].string_value();

		return true;
	} catch (const ErrorInfo &info) {
		QString title = QTStr("Auth.ChannelFailure.Title");
		QString text =
			QTStr("Auth.ChannelFailure.Text").arg(service(), info.message.c_str(), info.error.c_str());

		QMessageBox::warning(OBSBasic::Get(), title, text);

		blog(LOG_WARNING, "%s: %s: %s", __FUNCTION__, info.message.c_str(), info.error.c_str());
		return false;
	}
}
//-------------------------------------------------------------------------//
void OnlyfansAuth::updateSettingsUIPanes()
{
	if (this->uiLoaded && this->cefWidget != nullptr) {
		if (this->cefWidget->isVisible()) {
			auto script = QString(R"""(
				window.dispatchEvent(new CustomEvent('obsStreamingStatus', {
					detail: {status: '%1'}
				}));
			)""");

			if (stream_started) {
				// Executing custom JS script.
				this->cefWidget->executeJavaScript(script.arg("started").toStdString());
			} else {
				// Executing custom JS script.
				this->cefWidget->executeJavaScript(script.arg("stopped").toStdString());
			}
		}
	}
}
//-------------------------------------------------------------------------//
static std::shared_ptr<Auth> CreateOnlyfansAuth()
{
	try {
		return std::make_shared<OnlyfansAuth>(onlyfansDef);
	} catch (const std::exception &exc) {
		blog(LOG_WARNING, "Make Onlyfans auth failed: %s", exc.what());
	}
	return {};
}

static void DeleteCookies()
{
	if (panel_cookies != nullptr)
		panel_cookies->DeleteCookies(of_base_url, std::string());
}

void RegisterOnlyfansAuth()
{
#if !defined(__APPLE__) && !defined(_WIN32)
	if (QApplication::platformName().contains("wayland"))
		return;
#endif
	// Trying to override Onlyfans urls, if need it.
	try_override_of_urls();

	OAuth::RegisterOAuth(onlyfansDef, CreateOnlyfansAuth, OnlyfansAuth::Login, DeleteCookies);
}
