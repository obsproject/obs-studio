#pragma once
#include <obs-module.h>
#include <string>

#define MAX_CODECS 3

struct WHIPService {
	std::string server;
	std::string bearer_token;

	WHIPService(obs_data_t *settings, obs_service_t *service);

	void Update(obs_data_t *settings);
	static obs_properties_t *Properties();
	static void ApplyEncoderSettings(obs_data_t *video_settings,
					 obs_data_t *audio_settings);
	bool CanTryToConnect();
	const char *GetConnectInfo(enum obs_service_connect_info type);
};

void register_whip_service();
