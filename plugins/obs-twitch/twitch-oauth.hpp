// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <util/util.hpp>
#include <obs-frontend-api.h>
#include <obs-module.h>

#include <oauth-service-base.hpp>

#include "twitch-api.hpp"
#include "twitch-browser-widget.hpp"

namespace TwitchApi {

class ServiceOAuth : public OAuth::ServiceBase {
	RequestError lastError;

	User userInfo;

	std::string streamKey;

	TwitchBrowserWidget::Addon addon = TwitchBrowserWidget::Addon::NONE;

	TwitchBrowserWidget *chat = nullptr;
	TwitchBrowserWidget *info = nullptr;
	TwitchBrowserWidget *stats = nullptr;
	TwitchBrowserWidget *feed = nullptr;
	QString feedUuid;

	std::string UserAgent() override;

	const char *TokenUrl() override;

	std::string ClientId() override;

	int64_t ScopeVersion() override;

	bool LoginInternal(const OAuth::LoginReason &reason, std::string &code,
			   std::string &redirectUri) override;
	bool SignOutInternal() override;

	void SetSettings(obs_data_t *data) override;
	obs_data_t *GetSettingsData() override;

	void LoadFrontendInternal() override;
	void UnloadFrontendInternal() override;

	void DuplicationResetInternal() override;

	const char *PluginLogName() override { return "obs-twitch"; };

	void LoginError(RequestError &error) override;

	bool WrappedGetRemoteFile(const char *url, std::string &str,
				  long *responseCode,
				  const char *contentType = nullptr,
				  std::string request_type = "",
				  const char *postData = nullptr,
				  int postDataSize = 0);

	bool TryGetRemoteFile(const char *funcName, const char *url,
			      std::string &str,
			      const char *contentType = nullptr,
			      std::string request_type = "",
			      const char *postData = nullptr,
			      int postDataSize = 0);

public:
	ServiceOAuth() : OAuth::ServiceBase() {}

	void SetAddon(const TwitchBrowserWidget::Addon &newAddon);

	RequestError GetLastError() { return lastError; }

	bool GetUser(User &user);
	bool GetStreamKey(std::string &streamKey);
};

}
