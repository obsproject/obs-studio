// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <obs-module.h>
#include <string>

class YouTubeConfig {
	std::string protocol;
	std::string serverUrl;

	std::string streamKey;

public:
	YouTubeConfig(obs_data_t *settings, obs_service_t *self);
	inline ~YouTubeConfig(){};

	void Update(obs_data_t *settings);

	static void InfoGetDefault(obs_data_t *settings);

	const char *Protocol() { return protocol.c_str(); };

	const char *ConnectInfo(uint32_t type);

	bool CanTryToConnect();

	obs_properties_t *GetProperties();
};
