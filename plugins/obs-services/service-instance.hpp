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
	BPtr<obs_service_resolution> supportedResolutions;
	size_t supportedResolutionsCount;
	bool supportedResolutionsWithFps;

	static const char *InfoGetName(void *typeData);
	static void *InfoCreate(obs_data_t *settings, obs_service_t *service);
	static void InfoDestroy(void *data);
	static void InfoUpdate(void *data, obs_data_t *settings);

	static const char *InfoGetConnectInfo(void *data, uint32_t type);

	static const char *InfoGetProtocol(void *data);

	static const char **InfoGetSupportedVideoCodecs(void *data);
	static const char **InfoGetSupportedAudioCodecs(void *data);

	static bool InfoCanTryToConnect(void *data);

	static void InfoGetMaxFps(void *data, int *fps);

	static int InfoGetMaxCodecBitrate(void *data, const char *codec);

	static void InfoGetSupportedResolutions2(
		void *data, struct obs_service_resolution **resolutions,
		size_t *count, bool *withFps);

	static int
	InfoGetMaxVideoBitrate(void *data, const char *codec,
			       struct obs_service_resolution resolution);

	static void InfoGetDefault2(void *type_data, obs_data_t *settings);
	static obs_properties_t *InfoGetProperties2(void *data, void *typeData);

	static void InfoApplySettings2(void *data, const char *encoderId,
				       obs_data_t *encoderSettings);

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

	void
	GetSupportedResolutions(struct obs_service_resolution **resolutions,
				size_t *count, bool *withFps) const;

	int GetMaxCodecBitrate(const char *codec) const;

	int GetMaxVideoBitrate(const char *codec,
			       struct obs_service_resolution resolution) const;

	obs_properties_t *GetProperties();
};
