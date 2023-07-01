// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <util/util.hpp>
#include <obs-frontend-api.h>

#include <oauth.hpp>

namespace OAuth {

class ServiceBase {
	/* NOTE: To support service copy (e.g. settings) */
	std::vector<obs_service_t *> bondedServices;

	bool duplicationMarker = false;

	bool connected = false;
	bool frontendLoaded = false;

	std::string accessToken;
	int64_t expiresTime = 0;
	std::string refreshToken;

	bool defferedLogin = false;
	bool defferedLoadFrontend = false;

	inline void ReLogin(bool deferUiFunction);

	inline void ApplyNewTokens(const OAuth::AccessTokenResponse &response);
	obs_data_t *GetOAuthData();

	void LoadFrontend();
	void UnloadFrontend();

	void DuplicationReset();

protected:
	inline std::string AccessToken() { return accessToken; }
	bool IsAccessTokenExpired();
	bool RefreshAccessToken(RequestError &error);

	virtual inline bool AccessTokenHasExpireTime() { return true; }
	virtual inline bool AccessTokenHasRefreshToken() { return true; }

	virtual std::string UserAgent() = 0;

	virtual const char *TokenUrl() = 0;

	virtual std::string ClientId() = 0;
	virtual inline std::string ClientSecret() { return {}; }

	virtual int64_t ScopeVersion() = 0;

	virtual bool LoginInternal(std::string &code,
				   std::string &redirectUri) = 0;
	virtual bool SignOutInternal() = 0;

	virtual inline void SetSettings(obs_data_t * /* settingsData */){};
	virtual inline obs_data_t *GetSettingsData() { return nullptr; }

	virtual inline void LoadFrontendInternal() {}
	virtual inline void UnloadFrontendInternal() {}

	virtual void DuplicationResetInternal() {}

	virtual inline void
	AddBondedServiceFrontend(obs_service_t * /* service */){};
	virtual inline void
	RemoveBondedServiceFrontend(obs_service_t * /* service */){};

	virtual const char *PluginLogName() = 0;

	virtual inline void LoginError(RequestError & /* error */) {}

public:
	ServiceBase() {}
	virtual ~ServiceBase(){};

	void Setup(obs_data_t *data, bool deferUiFunction = false);
	obs_data_t *GetData();

	void OBSEvent(obs_frontend_event event);

	void AddBondedService(obs_service_t *service);
	void RemoveBondedService(obs_service_t *service);

	bool Connected() { return connected; }

	bool Login();
	bool SignOut();
};

}
