// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <obs-module.h>
#include <string>

#include "twitch-service.hpp"

class TwitchConfig {
	TwitchService *typeData;
#ifdef OAUTH_ENABLED
	obs_service_t *serviceObj;

	std::string uuid;
	TwitchApi::ServiceOAuth *oauth = nullptr;
#endif

	std::string protocol;
	bool serverAuto;
	std::string serverUrl;

	std::string streamKey;
	std::string bandwidthTestStreamKey;

	bool bandwidthTest = false;
	bool enforceBandwidthTest = false;

public:
	TwitchConfig(obs_data_t *settings, obs_service_t *self);
	~TwitchConfig();

	void Update(obs_data_t *settings);

	static void InfoGetDefault(obs_data_t *settings);

	const char *Protocol() { return protocol.c_str(); };

	const char *ConnectInfo(uint32_t type);

	bool CanTryToConnect();

	bool CanBandwidthTest() { return true; }
	void EnableBandwidthTest(bool enabled) { bandwidthTest = enabled; }
	bool BandwidthTestEnabled()
	{
		return bandwidthTest || enforceBandwidthTest;
	}

	obs_properties_t *GetProperties();
};
