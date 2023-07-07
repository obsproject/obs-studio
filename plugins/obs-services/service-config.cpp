// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "service-config.hpp"

#include <util/dstr.h>

#include "services-json-util.hpp"

ServiceConfig::ServiceConfig(obs_data_t *settings, obs_service_t *self)
	: service(reinterpret_cast<ServiceInstance *>(
		  obs_service_get_type_data(self)))
{
	Update(settings);
}

void ServiceConfig::Update(obs_data_t *settings)
{
	std::string newProtocol = obs_data_get_string(settings, "protocol");
	if (protocol.empty() || newProtocol != protocol) {
		protocol = newProtocol;

		if (service->HasSupportedVideoCodecs())
			supportedVideoCodecs =
				service->GetSupportedVideoCodecs(protocol);

		if (service->HasSupportedAudioCodecs())
			supportedAudioCodecs =
				service->GetSupportedAudioCodecs(protocol);
	}

	serverUrl = obs_data_get_string(settings, "server");

	streamId = obs_data_get_string(settings, "stream_id");

	username = obs_data_get_string(settings, "username");
	password = obs_data_get_string(settings, "password");

	encryptPassphrase = obs_data_get_string(settings, "encrypt_passphrase");

	bearerToken = obs_data_get_string(settings, "bearer_token");
}

const char *ServiceConfig::ConnectInfo(uint32_t type)
{
	switch ((enum obs_service_connect_info)type) {
	case OBS_SERVICE_CONNECT_INFO_SERVER_URL:
		if (!serverUrl.empty())
			return serverUrl.c_str();
		break;
	case OBS_SERVICE_CONNECT_INFO_STREAM_ID:
		if (!streamId.empty())
			return streamId.c_str();
		break;
	case OBS_SERVICE_CONNECT_INFO_USERNAME:
		if (!username.empty())
			return username.c_str();
		break;
	case OBS_SERVICE_CONNECT_INFO_PASSWORD:
		if (!password.empty())
			return password.c_str();
		break;
	case OBS_SERVICE_CONNECT_INFO_ENCRYPT_PASSPHRASE:
		if (!encryptPassphrase.empty())
			return encryptPassphrase.c_str();
		break;
	case OBS_SERVICE_CONNECT_INFO_BEARER_TOKEN:
		if (!bearerToken.empty())
			return bearerToken.c_str();
		break;
	}

	return nullptr;
}

bool ServiceConfig::CanTryToConnect()
{
	if (serverUrl.empty())
		return false;

	switch (StdStringToServerProtocol(protocol)) {
	case OBSServices::ServerProtocol::RTMP:
	case OBSServices::ServerProtocol::RTMPS:
	case OBSServices::ServerProtocol::HLS:
		if (streamId.empty())
			return false;
		break;
	case OBSServices::ServerProtocol::RIST: {
		OBSServices::RistProperties properties =
			service->GetRISTProperties();

		if (properties.encryptPassphrase && encryptPassphrase.empty())
			return false;

		if (properties.srpUsernamePassword &&
		    (username.empty() || password.empty()))
			return false;

		break;
	}

	case OBSServices::ServerProtocol::SRT: {
		OBSServices::SrtProperties properties =
			service->GetSRTProperties();

		if (properties.encryptPassphrase && encryptPassphrase.empty())
			return false;

		if (properties.streamId && streamId.empty())
			return false;

		break;
	}
	case OBSServices::ServerProtocol::WHIP:
		if (bearerToken.empty())
			return false;
		break;
	}

	return true;
}

void ServiceConfig::GetSupportedResolutions(
	struct obs_service_resolution **resolutions, size_t *count,
	bool *withFps)
{
	service->GetSupportedResolutions(resolutions, count, withFps);
}

int ServiceConfig::GetMaxCodecBitrate(const char *codec)
{
	return service->GetMaxCodecBitrate(codec);
}

int ServiceConfig::GetMaxVideoBitrate(const char *codec,
				      struct obs_service_resolution resolution)
{
	return service->GetMaxVideoBitrate(codec, resolution);
}

void ServiceConfig::ApplySettings2(const char *encoderId,
				   obs_data_t *encoderSettings)
{
	enum obs_encoder_type type = obs_get_encoder_type(encoderId);

	switch (StdStringToServerProtocol(protocol)) {
	case OBSServices::ServerProtocol::RIST:
	case OBSServices::ServerProtocol::SRT: {
		switch (type) {
		case OBS_ENCODER_VIDEO:
			obs_data_set_bool(encoderSettings, "repeat_headers",
					  true);
			break;
		case OBS_ENCODER_AUDIO:
			if (strcmp(encoderId, "libfdk_aac") == 0)
				obs_data_set_bool(encoderSettings,
						  "set_to_ADTS", true);
			break;
		}
		break;
	}
	case OBSServices::ServerProtocol::WHIP:
		if (type != OBS_ENCODER_VIDEO)
			break;

		obs_data_set_bool(encoderSettings, "repeat_headers", true);
		obs_data_set_int(encoderSettings, "bf", 0);
		obs_data_set_string(encoderSettings, "rate_control", "CBR");
		break;
	case OBSServices::ServerProtocol::RTMP:
	case OBSServices::ServerProtocol::RTMPS:
	case OBSServices::ServerProtocol::HLS:
		break;
	}

	service->ApplySettings2(type, encoderId, encoderSettings);
}
