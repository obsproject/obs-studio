#include "auth-restream.hpp"

#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <qt-wrappers.hpp>
#include <json11.hpp>
#include <ctime>
#include <sstream>

#include <obs-app.hpp>
#include "window-basic-main.hpp"
#include "remote-text.hpp"
#include "ui-config.h"
#include "obf.h"
#include <browser-panel.hpp>

using namespace json11;

extern QCef *cef;
extern QCefCookieManager *panel_cookies;

/* ------------------------------------------------------------------------- */

#define RESTREAM_AUTH_URL "https://api.restream.io/login?response_type=code"
#define RESTREAM_TOKEN_URL "https://api.restream.io/oauth/token"
#define RESTREAM_STREAMKEY_URL "https://api.restream.io/v2/user/streamKey"
#define RESTREAM_REDIRECT_URL "https%3A%2F%2Fobsproject.com%2Fapp-auth%2Frestream_post.php"
#define RESTREAM_SCOPE_VERSION 1


static Auth::Def restreamDef = {	
	"Restream",
	Auth::Type::OAuth_StreamKey
};

/* ------------------------------------------------------------------------- */

RestreamAuth::RestreamAuth(const Def &d)
	: OAuthStreamKey(d)
{
}

bool RestreamAuth::GetStreamKey()
try {
	std::string client_id = RESTREAM_CLIENTID;
	//deobfuscate_str(&client_id[0], RESTREAM_HASH);

	if (!GetToken(RESTREAM_TOKEN_URL, client_id, RESTREAM_SCOPE_VERSION))
		return false;
	if (token.empty())
		return false;
	if (!key_.empty())
		return true;

	std::string auth = "Authorization: Bearer " + token;

	std::vector<std::string> headers;
	//headers.push_back(std::string("Client-ID: ") + client_id);
	headers.push_back(std::move(auth));

	std::string output;
	std::string error;
	Json json;
	bool success;

	success = GetRemoteFile(
			RESTREAM_STREAMKEY_URL,
			output,
			error,
			nullptr,
			"application/json",
			nullptr,
			headers,
			nullptr,
			5);

	if (!success || output.empty())
		throw ErrorInfo("Failed to get stream key from remote", error);

	json = Json::parse(output, error);
	if (!error.empty())
		throw ErrorInfo("Failed to parse json", error);

	error = json["error"].string_value();
	if (!error.empty())
		throw ErrorInfo(error, json["error_description"].string_value());

	key_ = json["streamKey"].string_value();

	return true;
} catch (ErrorInfo info) {
	blog(LOG_WARNING, "%s: %s: %s",
			__FUNCTION__,
			info.message.c_str(),
			info.error.c_str());
	return false;
}

void RestreamAuth::SaveInternal()
{
	OBSBasic *main = OBSBasic::Get();
	//config_set_string(main->Config(), service(), "Name", name.c_str());
	//config_set_string(main->Config(), service(), "Id", id.c_str());
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

bool RestreamAuth::LoadInternal()
{
	//OBSBasic *main = OBSBasic::Get();
	//name = get_config_str(main, service(), "Name");
	//id = get_config_str(main, service(), "Id");
	firstLoad = false;
	return OAuthStreamKey::LoadInternal();
}

class RestreamWidget : public QDockWidget {
public:
	inline RestreamWidget() : QDockWidget() {}

	QScopedPointer<QCefWidget> widget;

	inline void SetWidget(QCefWidget *widget_)
	{
		setWidget(widget_);
		widget.reset(widget_);
	}
};

void RestreamAuth::LoadUI()
{
	if (uiLoaded)
		return;

	if (!GetStreamKey())
		return;

	OBSBasic::InitBrowserPanelSafeBlock(true);
	OBSBasic *main = OBSBasic::Get();

	QCefWidget *browser;
	std::string url;
	std::string script;

	/* ----------------------------------- */

	url = "https://restream.io/chat-application";
	//url += "/chat";

	QSize size = main->frameSize();
	QPoint pos = main->pos();

	chat.reset(new RestreamWidget());
	chat->setObjectName("restreamChat");
	chat->resize(420, 600);
	chat->setMinimumSize(380, 300);
	chat->setWindowTitle(QTStr("Auth.Chat"));
	chat->setAllowedAreas(Qt::AllDockWidgetAreas);

	browser = cef->create_widget(nullptr, url, panel_cookies);
	chat->SetWidget(browser);

	main->addDockWidget(Qt::RightDockWidgetArea, chat.data());
	chatMenu.reset(main->AddDockWidget(chat.data()));

	chat->setFloating(true);
	chat->move(pos.x() + size.width() - chat->width() - 50, pos.y() + 50);

	if (firstLoad) {
		chat->setVisible(true);
	}
	else {
		const char *dockStateStr = config_get_string(main->Config(),
			service(), "DockState");
		QByteArray dockState =
			QByteArray::fromBase64(QByteArray(dockStateStr));
		main->restoreState(dockState);
	}

	uiLoaded = true;
}

bool RestreamAuth::RetryLogin()
{
	std::string url = RESTREAM_AUTH_URL;
	url += "&client_id=" + std::string(RESTREAM_CLIENTID);
	url += "&redirect_uri=https%3A%2F%2Frestream.io%0D%0A";
	url += "&state=" + std::to_string(time(NULL));

	OAuthLogin login(OBSBasic::Get(), url, false);
	cef->add_popup_whitelist_url("about:blank", &login);

	if (login.exec() == QDialog::Rejected) {
		return false;
	}

	std::shared_ptr<RestreamAuth> auth = std::make_shared<RestreamAuth>(restreamDef);
	std::string client_id = RESTREAM_CLIENTID;
	deobfuscate_str(&client_id[0], MIXER_HASH);

	return GetToken(RESTREAM_TOKEN_URL, client_id, RESTREAM_SCOPE_VERSION, QT_TO_UTF8(login.GetCode()), true);
}


std::shared_ptr<Auth> RestreamAuth::Login(QWidget *parent)
{
	std::string loginUrl = RESTREAM_AUTH_URL;
	loginUrl += "&client_id=" + std::string(RESTREAM_CLIENTID);
	loginUrl += "&redirect_uri=" + std::string(RESTREAM_REDIRECT_URL);
	loginUrl += "&state=" + std::to_string(time(NULL));

	OAuthLogin login(parent, loginUrl, false);
	cef->add_popup_whitelist_url("about:blank", &login);

	if (login.exec() == QDialog::Rejected) {
		return nullptr;
	}

	std::shared_ptr<RestreamAuth> auth = std::make_shared<RestreamAuth>(restreamDef);

	std::string tokenUrl = RESTREAM_TOKEN_URL;
	std::string client_id = RESTREAM_CLIENTID;
	client_id += "&client_secret=" + std::string(RESTREAM_HASH);
	client_id += "&redirect_uri=" + std::string(RESTREAM_REDIRECT_URL);

	//deobfuscate_str(&client_id[0], RESTREAM_HASH);

	if (!auth->GetToken(RESTREAM_TOKEN_URL, client_id, RESTREAM_SCOPE_VERSION, QT_TO_UTF8(login.GetCode()))) {
		return nullptr;
	}

	std::string error;
	if (auth->GetStreamKey()) {
		return auth;
	}

	return nullptr;
}

static std::shared_ptr<Auth> CreateRestreamAuth()
{
	return std::make_shared<RestreamAuth>(restreamDef);
}

static void DeleteCookies()
{
	if (panel_cookies) {
		panel_cookies->DeleteCookies("restream.io", std::string());
	}
}

void RegisterRestreamAuth()
{
	OAuth::RegisterOAuth(
		restreamDef,
		CreateRestreamAuth,
		RestreamAuth::Login,
		DeleteCookies);
}
