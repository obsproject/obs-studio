// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <obs-module.h>
#include <util/util.hpp>

class YouTubeService {
	obs_service_info info = {0};

	static void InfoFreeTypeData(void *typeData);

	static const char *InfoGetName(void *typeData);
	static void *InfoCreate(obs_data_t *settings, obs_service_t *service);
	static void InfoDestroy(void *data);
	static void InfoUpdate(void *data, obs_data_t *settings);

	static const char *InfoGetConnectInfo(void *data, uint32_t type);

	static const char *InfoGetProtocol(void *data);

	static const char **InfoGetSupportedVideoCodecs(void *data);

	static bool InfoCanTryToConnect(void *data);

	static obs_properties_t *InfoGetProperties(void *data);

public:
	YouTubeService();
	~YouTubeService();

	static void Register();

	void GetDefaults(obs_data_t *settings);

	obs_properties_t *GetProperties();
};
