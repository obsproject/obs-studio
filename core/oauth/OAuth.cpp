#include "OAuth.hpp"

#include <utility/RemoteTextThread.hpp>
#include <widgets/OBSBasic.hpp>

#include <qt-wrappers.hpp>

#include <QMessageBox>
#include <json11.hpp>

#include "moc_OAuth.cpp"

using namespace json11;

struct OAuthInfo {
	Auth::Def def;
	OAuth::login_cb login;
	OAuth::delete_cookies_cb delete_cookies;
};

static std::vector<OAuthInfo> loginCBs;
void OAuth::RegisterOAuth(const Def &d, create_cb create, login_cb login, delete_cookies_cb delete_cookies)
{
	OAuthInfo info = {d, login, delete_cookies};
	loginCBs.push_back(info);
	RegisterAuth(d, create);
}

std::shared_ptr<Auth> OAuth::Login(QWidget *parent, const std::string &service)
{
	for (auto &a : loginCBs) {
		if (service.find(a.def.service) != std::string::npos) {
			return a.login(parent, service);
		}
	}

	return nullptr;
}

void OAuth::DeleteCookies(const std::string &service)
{
	for (auto &a : loginCBs) {
		if (service.find(a.def.service) != std::string::npos) {
			a.delete_cookies();
		}
	}
}

void OAuth::SaveInternal()
{
	OBSBasic *main = OBSBasic::Get();
	config_set_string(main->Config(), service(), "RefreshToken", refresh_token.c_str());
	config_set_string(main->Config(), service(), "Token", token.c_str());
	config_set_uint(main->Config(), service(), "ExpireTime", expire_time);
	config_set_int(main->Config(), service(), "ScopeVer", currentScopeVer);
}

static inline std::string get_config_str(OBSBasic *main, const char *section, const char *name)
{
	const char *val = config_get_string(main->Config(), section, name);
	return val ? val : "";
}

bool OAuth::LoadInternal()
{
	OBSBasic *main = OBSBasic::Get();
	refresh_token = get_config_str(main, service(), "RefreshToken");
	token = get_config_str(main, service(), "Token");
	expire_time = config_get_uint(main->Config(), service(), "ExpireTime");
	currentScopeVer = (int)config_get_int(main->Config(), service(), "ScopeVer");
	return implicit ? !token.empty() : !refresh_token.empty();
}

bool OAuth::TokenExpired()
{
	if (token.empty())
		return true;
	if ((uint64_t)time(nullptr) > expire_time - 5)
		return true;
	return false;
}

bool OAuth::GetToken(const char *url, const std::string &client_id, const std::string &secret,
		     const std::string &redirect_uri, int scope_ver, const std::string &auth_code, bool retry)
{
	return GetTokenInternal(url, client_id, secret, redirect_uri, scope_ver, auth_code, retry);
}

bool OAuth::GetToken(const char *url, const std::string &client_id, int scope_ver, const std::string &auth_code,
		     bool retry)
{
	return GetTokenInternal(url, client_id, {}, {}, scope_ver, auth_code, retry);
}

bool OAuth::GetTokenInternal(const char *url, const std::string &client_id, const std::string &secret,
			     const std::string &redirect_uri, int scope_ver, const std::string &auth_code, bool retry)
try {
	std::string output;
	std::string error;
	std::string desc;

	if (currentScopeVer > 0 && currentScopeVer < scope_ver) {
		if (RetryLogin()) {
			return true;
		} else {
			QString title = QTStr("Auth.InvalidScope.Title");
			QString text = QTStr("Auth.InvalidScope.Text").arg(service());

			QMessageBox::warning(OBSBasic::Get(), title, text);
		}
	}

	if (auth_code.empty() && !TokenExpired()) {
		return true;
	}

	std::string post_data;
	post_data += "action=redirect&client_id=";
	post_data += client_id;
	if (!secret.empty()) {
		post_data += "&client_secret=";
		post_data += secret;
	}
	if (!redirect_uri.empty()) {
		post_data += "&redirect_uri=";
		post_data += redirect_uri;
	}

	if (!auth_code.empty()) {
		post_data += "&grant_type=authorization_code&code=";
		post_data += auth_code;
	} else {
		post_data += "&grant_type=refresh_token&refresh_token=";
		post_data += refresh_token;
	}

	bool success = false;

	auto func = [&]() {
		success = GetRemoteFile(url, output, error, nullptr, "application/x-www-form-urlencoded", "",
					post_data.c_str(), std::vector<std::string>(), nullptr, 5);
	};

	ExecThreadedWithoutBlocking(func, QTStr("Auth.Authing.Title"), QTStr("Auth.Authing.Text").arg(service()));
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
	token = json["access_token"].string_value();
	if (token.empty())
		throw ErrorInfo("Failed to get token from remote", error);

	if (!auth_code.empty()) {
		refresh_token = json["refresh_token"].string_value();
		if (refresh_token.empty())
			throw ErrorInfo("Failed to get refresh token from "
					"remote",
					error);

		currentScopeVer = scope_ver;
	}

	return true;

} catch (ErrorInfo &info) {
	if (!retry) {
		QString title = QTStr("Auth.AuthFailure.Title");
		QString text = QTStr("Auth.AuthFailure.Text").arg(service(), info.message.c_str(), info.error.c_str());

		QMessageBox::warning(OBSBasic::Get(), title, text);
	}

	blog(LOG_WARNING, "%s: %s: %s", __FUNCTION__, info.message.c_str(), info.error.c_str());
	return false;
}

void OAuthStreamKey::OnStreamConfig()
{
	if (key_.empty())
		return;

	OBSBasic *main = OBSBasic::Get();
	obs_service_t *service = main->GetService();

	OBSDataAutoRelease settings = obs_service_get_settings(service);

	bool bwtest = obs_data_get_bool(settings, "bwtest");

	if (bwtest && strcmp(this->service(), "Twitch") == 0)
		obs_data_set_string(settings, "key", (key_ + "?bandwidthtest=true").c_str());
	else
		obs_data_set_string(settings, "key", key_.c_str());

	obs_service_update(service, settings);
}
