// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <obs-module.h>
#include <util/util.hpp>

#include "generated/services-json.hpp"

class ServiceInstance {
	obs_service_info info = {0};

	const OBSServices::Service service;

	BPtr<char> supportedProtocols;

	static const char *InfoGetName(void *typeData);
	static void *InfoCreate(obs_data_t *settings, obs_service_t *service);
	static void InfoDestroy(void *data);
	static void InfoUpdate(void *data, obs_data_t *settings);

	static const char *InfoGetConnectInfo(void *data, uint32_t type);

	static const char *InfoGetProtocol(void *data);

	static const char **InfoGetSupportedVideoCodecs(void *data);
	static const char **InfoGetSupportedAudioCodecs(void *data);

	static bool InfoCanTryToConnect(void *data);

	static void InfoGetDefault2(void *type_data, obs_data_t *settings);
	static obs_properties_t *InfoGetProperties2(void *data, void *typeData);

public:
	ServiceInstance(const OBSServices::Service &service);
	inline ~ServiceInstance(){};

	OBSServices::RistProperties GetRISTProperties() const;
	OBSServices::SrtProperties GetSRTProperties() const;

	bool HasSupportedVideoCodecs() const
	{
		return service.supportedCodecs.has_value() &&
		       service.supportedCodecs->video.has_value();
	}
	bool HasSupportedAudioCodecs() const
	{
		return service.supportedCodecs.has_value() &&
		       service.supportedCodecs->audio.has_value();
	}

	char **GetSupportedVideoCodecs(const std::string &protocol) const;
	char **GetSupportedAudioCodecs(const std::string &protocol) const;

	const char *GetName();

	void GetDefaults(obs_data_t *settings);

	obs_properties_t *GetProperties();
};
