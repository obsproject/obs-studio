// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <util/util.hpp>
#include <obs-frontend-api.h>
#include <obs-module.h>

#include <oauth-service-base.hpp>

#include "restream-api.hpp"
#include "restream-browser-widget.hpp"

namespace RestreamApi {

class ServiceOAuth : public OAuth::ServiceBase {
	RequestError lastError;

	std::string streamKey;

	RestreamBrowserWidget *chat = nullptr;
	RestreamBrowserWidget *info = nullptr;
	RestreamBrowserWidget *channels = nullptr;

	std::string UserAgent() override;

	const char *TokenUrl() override;

	std::string ClientId() override;

	int64_t ScopeVersion() override;

	bool LoginInternal(const OAuth::LoginReason &reason, std::string &code,
			   std::string &redirectUri) override;
	bool SignOutInternal() override;

	void LoadFrontendInternal() override;
	void UnloadFrontendInternal() override;

	void DuplicationResetInternal() override;

	const char *PluginLogName() override { return "obs-restream"; };

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

	RequestError GetLastError() { return lastError; }

	bool GetStreamKey(std::string &streamKey);
};

}
