// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <obs-module.h>
#include <string>

#ifdef OAUTH_ENABLED
#include "youtube-service.hpp"
#endif

class YouTubeConfig {
#ifdef OAUTH_ENABLED
	obs_service_t *serviceObj;
	YouTubeService *typeData;

	std::string uuid;
	YouTubeApi::ServiceOAuth *oauth = nullptr;

	bool bandwidthTest = false;
	YouTubeApi::LiveStream bandwidthTestStream;
#endif

	std::string protocol;
	std::string serverUrl;

	std::string streamKey;

public:
	YouTubeConfig(obs_data_t *settings, obs_service_t *self);
	~YouTubeConfig();

	void Update(obs_data_t *settings);

	static void InfoGetDefault(obs_data_t *settings);

	const char *Protocol() { return protocol.c_str(); };

	const char *ConnectInfo(uint32_t type);

	bool CanTryToConnect();

#ifdef OAUTH_ENABLED
	bool CanBandwidthTest() { return oauth->Connected(); }
	void EnableBandwidthTest(bool enabled) { bandwidthTest = enabled; }
	bool BandwidthTestEnabled() { return bandwidthTest; }
#endif

	obs_properties_t *GetProperties();
};
