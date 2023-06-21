// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "oauth-service-base.hpp"

#include <obs.hpp>

constexpr const char *DATA_NAME_OAUTH = "oauth";
constexpr const char *DATA_NAME_SETTINGS = "settings";

constexpr const char *DATA_NAME_ACCESS_TOKEN = "access_token";
constexpr const char *DATA_NAME_EXPIRE_TIME = "expire_time";
constexpr const char *DATA_NAME_REFRESH_TOKEN = "refresh_token";
constexpr const char *DATA_NAME_SCOPE_VERSION = "scope_version";

namespace OAuth {

void ServiceBase::Setup(obs_data_t *data, bool deferUiFunction)
{
	if (!data)
		return;

	if (obs_data_has_user_value(data, DATA_NAME_SETTINGS)) {
		OBSDataAutoRelease settingsData =
			obs_data_get_obj(data, DATA_NAME_SETTINGS);
		SetSettings(settingsData);
	}

	OBSDataAutoRelease oauthData = obs_data_get_obj(data, DATA_NAME_OAUTH);
	if (!oauthData)
		return;

	/* Check if values in config exist if not consider, 'not connected' */
	if (!(obs_data_has_user_value(oauthData, DATA_NAME_ACCESS_TOKEN) &&
	      obs_data_has_user_value(oauthData, DATA_NAME_SCOPE_VERSION))) {
		return;
	}

	if (AccessTokenHasExpireTime() &&
	    !obs_data_has_user_value(oauthData, DATA_NAME_EXPIRE_TIME)) {
		return;
	}

	if (AccessTokenHasRefreshToken() &&
	    !obs_data_has_user_value(oauthData, DATA_NAME_REFRESH_TOKEN)) {
		return;
	}

	/* Check scope version and re-login if mismatch */
	int64_t dataScopeVersion =
		obs_data_get_int(oauthData, DATA_NAME_SCOPE_VERSION);
	if (dataScopeVersion != ScopeVersion()) {
		blog(LOG_WARNING,
		     "[%s][%s]: Old scope version detected, the user will be asked to re-login",
		     PluginLogName(), __FUNCTION__);
		ReLogin(LoginReason::SCOPE_CHANGE, deferUiFunction);
		return;
	}

	accessToken = obs_data_get_string(oauthData, DATA_NAME_ACCESS_TOKEN);

	if (AccessTokenHasExpireTime())
		expiresTime =
			obs_data_get_int(oauthData, DATA_NAME_EXPIRE_TIME);

	if (AccessTokenHasRefreshToken())
		refreshToken =
			obs_data_get_string(oauthData, DATA_NAME_REFRESH_TOKEN);

	/* Check if the access token has expired */
	if (AccessTokenHasExpireTime() && IsAccessTokenExpired()) {
		RequestError error;
		if (!(AccessTokenHasRefreshToken() &&
		      RefreshAccessToken(error))) {
			blog(LOG_WARNING,
			     "[%s][%s]: Access Token has expired, the user will be asked to re-login",
			     PluginLogName(), __FUNCTION__);
			ReLogin(LoginReason::REFRESH_TOKEN_FAILED,
				deferUiFunction);
			return;
		}
	}

	connected = true;

	if (deferUiFunction) {
		defferedLoadFrontend = true;
	} else {
		LoadFrontend();
	}
}

void ServiceBase::ReLogin(const LoginReason &reason, bool deferUiFunction)
{
	if (deferUiFunction) {
		defferedLogin = true;
		defferedLoginReason = reason;
	} else {
		Login(reason);
	}
}

void ServiceBase::ApplyNewTokens(const OAuth::AccessTokenResponse &response)
{
	accessToken = response.accessToken;
	if (AccessTokenHasExpireTime())
		expiresTime = time(nullptr) + response.expiresIn.value();
	if (AccessTokenHasRefreshToken() && response.refreshToken.has_value())
		refreshToken = response.refreshToken.value();
}

obs_data_t *ServiceBase::GetOAuthData()
{
	if (!connected || accessToken.empty())
		return nullptr;

	OBSData data = obs_data_create();
	obs_data_set_string(data, DATA_NAME_ACCESS_TOKEN, accessToken.c_str());
	obs_data_set_int(data, DATA_NAME_SCOPE_VERSION, ScopeVersion());

	if (AccessTokenHasExpireTime() && expiresTime > 0)
		obs_data_set_int(data, DATA_NAME_EXPIRE_TIME, expiresTime);

	if (AccessTokenHasRefreshToken() && !refreshToken.empty())
		obs_data_set_string(data, DATA_NAME_REFRESH_TOKEN,
				    refreshToken.c_str());

	return data;
}

void ServiceBase::LoadFrontend()
{
	if (frontendLoaded)
		return;

	LoadFrontendInternal();

	for (size_t i = 0; i < bondedServices.size(); i++)
		AddBondedServiceFrontend(bondedServices[i]);

	frontendLoaded = true;
}

void ServiceBase::UnloadFrontend()
{
	if (!frontendLoaded)
		return;

	UnloadFrontendInternal();

	for (size_t i = 0; i < bondedServices.size(); i++)
		RemoveBondedServiceFrontend(bondedServices[i]);

	frontendLoaded = false;
}

void ServiceBase::DuplicationReset()
{
	if (!duplicationMarker)
		return;

	connected = false;

	accessToken.clear();
	expiresTime = 0;
	refreshToken.clear();

	DuplicationResetInternal();

	duplicationMarker = false;
}

bool ServiceBase::IsAccessTokenExpired()
{
	if (accessToken.empty())
		return true;

	return time(nullptr) > expiresTime - 5;
}

bool ServiceBase::RefreshAccessToken(RequestError &error)
{
	OAuth::AccessTokenResponse response;
	if (!OAuth::RefreshAccessToken(response, error, UserAgent(), TokenUrl(),
				       refreshToken, ClientId(),
				       ClientSecret())) {
		blog(LOG_WARNING, "[%s][%s]: %s: %s", PluginLogName(),
		     __FUNCTION__, error.message.c_str(), error.error.c_str());
		return false;
	}

	if (AccessTokenHasExpireTime() && !response.expiresIn.has_value()) {
		error = RequestError(
			RequestErrorType::MISSING_REQUIRED_OPT_PARAMETER,
			"Missing \"expiresIn\" in the response");
		blog(LOG_WARNING, "[%s][%s]: %s: %s", PluginLogName(),
		     __FUNCTION__, error.message.c_str(), error.error.c_str());
		return false;
	}

	blog(LOG_DEBUG, "[%s][%s]: Access token successfully refreshed",
	     PluginLogName(), __FUNCTION__);

	ApplyNewTokens(response);

	return true;
}

obs_data_t *ServiceBase::GetData()
{
	OBSDataAutoRelease oauthData = GetOAuthData();
	OBSDataAutoRelease settingsData = GetSettingsData();

	if (oauthData == nullptr && settingsData == nullptr)
		return nullptr;

	OBSData data = obs_data_create();
	if (oauthData != nullptr)
		obs_data_set_obj(data, DATA_NAME_OAUTH, oauthData);

	if (settingsData != nullptr)
		obs_data_set_obj(data, DATA_NAME_SETTINGS, settingsData);

	return data;
}

void ServiceBase::OBSEvent(obs_frontend_event event)
{
	switch (event) {
	case OBS_FRONTEND_EVENT_PROFILE_CHANGED:
		/* If duplication, reset token and ask to login */
		if (duplicationMarker) {
			DuplicationReset();
			Login(LoginReason::PROFILE_DUPLICATION);
		}
		[[fallthrough]];
	case OBS_FRONTEND_EVENT_FINISHED_LOADING:
		if (defferedLogin) {
			Login(defferedLoginReason);
			defferedLogin = false;
		}

		if (defferedLoadFrontend) {
			LoadFrontend();
			defferedLoadFrontend = false;
		}
		break;
	case OBS_FRONTEND_EVENT_PROFILE_CHANGING:
		/* Mark OAuth to detect profile duplication */
		duplicationMarker = true;
		[[fallthrough]];
	case OBS_FRONTEND_EVENT_EXIT:
		UnloadFrontend();
		break;
	default:
		break;
	}
}

void ServiceBase::AddBondedService(obs_service_t *service)
{
	for (size_t i = 0; i < bondedServices.size(); i++) {
		if (bondedServices[i] == service)
			return;
	}

	bondedServices.push_back(service);
	if (frontendLoaded)
		AddBondedServiceFrontend(service);
}

void ServiceBase::RemoveBondedService(obs_service_t *service)
{
	for (size_t i = 0; i < bondedServices.size(); i++) {
		if (bondedServices[i] == service) {
			if (frontendLoaded)
				RemoveBondedServiceFrontend(service);
			bondedServices.erase(bondedServices.begin() + i);
		}
	}
}

bool ServiceBase::Login(const LoginReason &reason)
try {
	if (connected)
		return true;

	std::string code;
	std::string redirectUri;
	if (!LoginInternal(reason, code, redirectUri))
		return false;

	OAuth::AccessTokenResponse response;
	RequestError error;
	if (!OAuth::AccessTokenRequest(response, error, UserAgent(), TokenUrl(),
				       code, redirectUri, ClientId(),
				       ClientSecret())) {
		throw error;
	}

	if (AccessTokenHasExpireTime() && !response.expiresIn.has_value()) {
		throw RequestError(
			RequestErrorType::MISSING_REQUIRED_OPT_PARAMETER,
			"Missing \"expires_in\" in the response");
	} else if (AccessTokenHasRefreshToken() &&
		   !response.refreshToken.has_value()) {
		throw RequestError(
			RequestErrorType::MISSING_REQUIRED_OPT_PARAMETER,
			"Missing \"refresh_token\" in the response");
	}

	ApplyNewTokens(response);

	connected = true;

	LoadFrontend();

	return true;
} catch (RequestError &e) {
	blog(LOG_WARNING, "[%s][%s]: %s: %s", PluginLogName(), __FUNCTION__,
	     e.message.c_str(), e.error.c_str());
	LoginError(e);
	return false;
}

bool ServiceBase::SignOut()
{
	if (!connected)
		return true;

	if (!SignOutInternal())
		return false;

	UnloadFrontend();

	connected = false;

	return true;
}

}
