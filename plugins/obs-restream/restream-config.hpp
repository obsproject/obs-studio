// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <obs-module.h>
#include <string>

#include "restream-service.hpp"

class RestreamConfig {
	RestreamService *typeData;

	std::string server;

	std::string streamKey;
	std::string bandwidthTestStreamKey;

	bool bandwidthTest = false;

public:
	RestreamConfig(obs_data_t *settings, obs_service_t *self);
	~RestreamConfig();

	void Update(obs_data_t *settings);

	static void InfoGetDefault(obs_data_t *settings);

	const char *Protocol() { return "RTMP"; };

	const char *ConnectInfo(uint32_t type);

	bool CanTryToConnect();

	bool CanBandwidthTest() { return true; }
	void EnableBandwidthTest(bool enabled) { bandwidthTest = enabled; }
	bool BandwidthTestEnabled() { return bandwidthTest; }

	obs_properties_t *GetProperties();
};
