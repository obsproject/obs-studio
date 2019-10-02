#include "auth-mixer.hpp"

#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <qt-wrappers.hpp>
#include <obs-app.hpp>

#include "window-dock-browser.hpp"
#include "window-basic-main.hpp"
#include "remote-text.hpp"

#include <json11.hpp>

#include <ctime>

#include "ui-config.h"
#include "obf.h"

using namespace json11;

/* ------------------------------------------------------------------------- */

#define MIXER_AUTH_URL "https://obsproject.com/app-auth/mixer?action=redirect"
#define MIXER_TOKEN_URL "https://obsproject.com/app-auth/mixer-token"

#define MIXER_SCOPE_VERSION 1

static Auth::Def mixerDef = {"Mixer", Auth::Type::OAuth_StreamKey};

/* ------------------------------------------------------------------------- */

MixerAuth::MixerAuth(const Def &d) : OAuthStreamKey(d) {}

bool MixerAuth::GetChannelInfo(bool allow_retry)
try {
	std::string client_id = MIXER_CLIENTID;
	deobfuscate_str(&client_id[0], MIXER_HASH);

	if (!GetToken(MIXER_TOKEN_URL, client_id, MIXER_SCOPE_VERSION))
		return false;
	if (token.empty())
		return false;
	if (!key_.empty())
		return true;

	std::string auth;
	auth += "Authorization: Bearer ";
	auth += token;

	std::vector<std::string> headers;
	headers.push_back(std::string("Client-ID: ") + client_id);
	headers.push_back(std::move(auth));

	std::string output;
	std::string error;
	Json json;
	bool success;

	if (id.empty()) {
		auto func = [&]() {
			success = GetRemoteFile(
				"https://mixer.com/api/v1/users/current",
				output, error, nullptr, "application/json",
				nullptr, headers, nullptr, 5);
		};

		ExecThreadedWithoutBlocking(
			func, QTStr("Auth.LoadingChannel.Title"),
			QTStr("Auth.LoadingChannel.Text").arg(service()));
		if (!success || output.empty())
			throw ErrorInfo("Failed to get user info from remote",
					error);

		Json json = Json::parse(output, error);
		if (!error.empty())
			throw ErrorInfo("Failed to parse json", error);

		error = json["error"].string_value();
		if (!error.empty())
			throw ErrorInfo(
				error,
				json["error_description"].string_value());

		id = std::to_string(json["channel"]["id"].int_value());
		name = json["channel"]["token"].string_value();
	}

	/* ------------------ */

	std::string url;
	url += "https://mixer.com/api/v1/channels/";
	url += id;
	url += "/details";

	output.clear();

	auto func = [&]() {
		success = GetRemoteFile(url.c_str(), output, error, nullptr,
					"application/json", nullptr, headers,
					nullptr, 5);
	};

	ExecThreadedWithoutBlocking(
		func, QTStr("Auth.LoadingChannel.Title"),
		QTStr("Auth.LoadingChannel.Text").arg(service()));
	if (!success || output.empty())
		throw ErrorInfo("Failed to get stream key from remote", error);

	json = Json::parse(output, error);
	if (!error.empty())
		throw ErrorInfo("Failed to parse json", error);

	error = json["error"].string_value();
	if (!error.empty())
		throw ErrorInfo(error,
				json["error_description"].string_value());

	std::string key_suffix = json["streamKey"].string_value();

	/* Mixer does not throw an error; instead it gives you the channel data
	 * json without the data you normally have privileges for, which means
	 * it'll be an empty stream key usually.  So treat empty stream key as
	 * an error. */
	if (key_suffix.empty()) {
		if (allow_retry && RetryLogin()) {
			return GetChannelInfo(false);
		}
		throw ErrorInfo("Auth Failure", "Could not get channel data");
	}

	key_ = id + "-" + key_suffix;

	return true;
} catch (ErrorInfo info) {
	QString title = QTStr("Auth.ChannelFailure.Title");
	QString text = QTStr("Auth.ChannelFailure.Text")
			       .arg(service(), info.message.c_str(),
				    info.error.c_str());

	QMessageBox::warning(OBSBasic::Get(), title, text);

	blog(LOG_WARNING, "%s: %s: %s", __FUNCTION__, info.message.c_str(),
	     info.error.c_str());
	return false;
}

void MixerAuth::SaveInternal()
{
	OBSBasic *main = OBSBasic::Get();
	config_set_string(main->Config(), service(), "Name", name.c_str());
	config_set_string(main->Config(), service(), "Id", id.c_str());
	if (uiLoaded) {
		config_set_string(main->Config(), service(), "DockState",
				  main->saveState().toBase64().constData());
	}
	OAuthStreamKey::SaveInternal();
}

static inline std::string get_config_str(OBSBasic *main, const char *section,
					 const char *name)
{
	const char *val = config_get_string(main->Config(), section, name);
	return val ? val : "";
}

bool MixerAuth::LoadInternal()
{
	if (!cef)
		return false;

	OBSBasic *main = OBSBasic::Get();
	name = get_config_str(main, service(), "Name");
	id = get_config_str(main, service(), "Id");
	firstLoad = false;
	return OAuthStreamKey::LoadInternal();
}

void MixerAuth::LoadUI()
{
	if (!cef)
		return;
	if (uiLoaded)
		return;
	if (!GetChannelInfo())
		return;

	OBSBasic::InitBrowserPanelSafeBlock();
	OBSBasic *main = OBSBasic::Get();

	std::string url;
	url += "https://mixer.com/embed/chat/";
	url += id;

	QSize size = main->frameSize();
	QPoint pos = main->pos();

	chat.reset(new BrowserDock());
	chat->setObjectName("mixerChat");
	chat->resize(300, 600);
	chat->setMinimumSize(200, 300);
	chat->setWindowTitle(QTStr("Auth.Chat"));
	chat->setAllowedAreas(Qt::AllDockWidgetAreas);

	QCefWidget *browser = cef->create_widget(nullptr, url, panel_cookies);
	chat->SetWidget(browser);

	main->addDockWidget(Qt::RightDockWidgetArea, chat.data());
	chatMenu.reset(main->AddDockWidget(chat.data()));

	/* ----------------------------------- */

	chat->setFloating(true);
	chat->move(pos.x() + size.width() - chat->width() - 50, pos.y() + 50);

	if (firstLoad) {
		chat->setVisible(true);
	} else {
		const char *dockStateStr = config_get_string(
			main->Config(), service(), "DockState");
		QByteArray dockState =
			QByteArray::fromBase64(QByteArray(dockStateStr));
		main->restoreState(dockState);
	}

	uiLoaded = true;
}

bool MixerAuth::RetryLogin()
{
	if (!cef)
		return false;

	OAuthLogin login(OBSBasic::Get(), MIXER_AUTH_URL, false);
	cef->add_popup_whitelist_url("about:blank", &login);

	if (login.exec() == QDialog::Rejected) {
		return false;
	}

	std::shared_ptr<MixerAuth> auth = std::make_shared<MixerAuth>(mixerDef);
	std::string client_id = MIXER_CLIENTID;
	deobfuscate_str(&client_id[0], MIXER_HASH);

	return GetToken(MIXER_TOKEN_URL, client_id, MIXER_SCOPE_VERSION,
			QT_TO_UTF8(login.GetCode()), true);
}

std::shared_ptr<Auth> MixerAuth::Login(QWidget *parent)
{
	if (!cef) {
		return nullptr;
	}

	OAuthLogin login(parent, MIXER_AUTH_URL, false);
	cef->add_popup_whitelist_url("about:blank", &login);

	if (login.exec() == QDialog::Rejected) {
		return nullptr;
	}

	std::shared_ptr<MixerAuth> auth = std::make_shared<MixerAuth>(mixerDef);

	std::string client_id = MIXER_CLIENTID;
	deobfuscate_str(&client_id[0], MIXER_HASH);

	if (!auth->GetToken(MIXER_TOKEN_URL, client_id, MIXER_SCOPE_VERSION,
			    QT_TO_UTF8(login.GetCode()))) {
		return nullptr;
	}

	std::string error;
	if (auth->GetChannelInfo(false)) {
		return auth;
	}

	return nullptr;
}

static std::shared_ptr<Auth> CreateMixerAuth()
{
	return std::make_shared<MixerAuth>(mixerDef);
}

static void DeleteCookies()
{
	if (panel_cookies) {
		panel_cookies->DeleteCookies("mixer.com", std::string());
		panel_cookies->DeleteCookies("microsoft.com", std::string());
	}
}

void RegisterMixerAuth()
{
	OAuth::RegisterOAuth(mixerDef, CreateMixerAuth, MixerAuth::Login,
			     DeleteCookies);
}
