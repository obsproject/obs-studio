// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "restream-config.hpp"

constexpr const char *STREAM_KEY_LINK =
	"https://restream.io/settings/streaming-setup?from=OBS";
constexpr const char *AUTODETECT_INGEST = "rtmp://live.restream.io/live";

constexpr const char *BANDWIDTH_TEST = "?test=true";

RestreamConfig::RestreamConfig(obs_data_t *settings, obs_service_t *self)
	: typeData(reinterpret_cast<RestreamService *>(
		  obs_service_get_type_data(self)))
{
	Update(settings);
}

void RestreamConfig::Update(obs_data_t *settings)
{
	server = obs_data_get_string(settings, "server");

	streamKey = obs_data_get_string(settings, "stream_key");
	bandwidthTestStreamKey = streamKey + BANDWIDTH_TEST;
}

RestreamConfig::~RestreamConfig() {}

void RestreamConfig::InfoGetDefault(obs_data_t *settings)
{
	obs_data_set_string(settings, "server", AUTODETECT_INGEST);
}

const char *RestreamConfig::ConnectInfo(uint32_t type)
{
	switch ((enum obs_service_connect_info)type) {
	case OBS_SERVICE_CONNECT_INFO_SERVER_URL:
		if (!server.empty())
			return server.c_str();

		break;
	case OBS_SERVICE_CONNECT_INFO_STREAM_KEY:
		if (bandwidthTest ? !bandwidthTestStreamKey.empty()
				  : !streamKey.empty()) {
			return bandwidthTest ? bandwidthTestStreamKey.c_str()
					     : streamKey.c_str();
		}
		break;
	default:
		break;
	}

	return nullptr;
}

bool RestreamConfig::CanTryToConnect()
{
	if (server.empty() || streamKey.empty())
		return false;

	return true;
}

obs_properties_t *RestreamConfig::GetProperties()
{
	obs_properties_t *ppts = obs_properties_create();
	obs_property_t *p;

	p = obs_properties_add_list(ppts, "server",
				    obs_module_text("Restream.Server"),
				    OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);

	std::vector<RestreamApi::Ingest> ingests = typeData->GetIngests();

	if (ingests.empty()) {
		obs_property_list_add_string(p, "Autodetect",
					     AUTODETECT_INGEST);
	} else {

		for (const RestreamApi::Ingest &ingest : ingests) {
			obs_property_list_add_string(p, ingest.name.c_str(),
						     ingest.rtmpUrl.c_str());
		}
	}

	p = obs_properties_add_text(ppts, "stream_key",
				    obs_module_text("Restream.StreamKey"),
				    OBS_TEXT_PASSWORD);

	p = obs_properties_add_button(ppts, "get_stream_key",
				      obs_module_text("Restream.GetStreamKey"),
				      nullptr);
	obs_property_button_set_type(p, OBS_BUTTON_URL);
	obs_property_button_set_url(p, (char *)STREAM_KEY_LINK);

	return ppts;
}
