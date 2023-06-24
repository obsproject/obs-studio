// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "twitch-config.hpp"

constexpr const char *STREAM_KEY_LINK =
	"https://dashboard.twitch.tv/settings/stream";
constexpr const char *DEFAULT_RTMPS_INGEST = "rtmps://live.twitch.tv/app";
constexpr const char *DEFAULT_RTMP_INGEST = "rtmp://live.twitch.tv/app";

constexpr const char *BANDWIDTH_TEST = "?bandwidthtest=true";

TwitchConfig::TwitchConfig(obs_data_t *settings, obs_service_t *self)
	: typeData(reinterpret_cast<TwitchService *>(
		  obs_service_get_type_data(self)))
{

	Update(settings);
}

void TwitchConfig::Update(obs_data_t *settings)
{
	protocol = obs_data_get_string(settings, "protocol");

	std::string server = obs_data_get_string(settings, "server");
	serverAuto = server == "auto";
	if (!serverAuto)
		serverUrl = obs_data_get_string(settings, "server");

	streamKey = obs_data_get_string(settings, "stream_key");
	bandwidthTestStreamKey = streamKey + BANDWIDTH_TEST;

	enforceBandwidthTest = obs_data_get_bool(settings, "enforce_bwtest");
}

TwitchConfig::~TwitchConfig() {}

void TwitchConfig::InfoGetDefault(obs_data_t *settings)
{
	obs_data_set_string(settings, "protocol", "RTMPS");
	obs_data_set_string(settings, "server", "auto");
}

const char *TwitchConfig::ConnectInfo(uint32_t type)
{
	switch ((enum obs_service_connect_info)type) {
	case OBS_SERVICE_CONNECT_INFO_SERVER_URL:
		if (serverAuto) {
			std::vector<TwitchApi::Ingest> ingests =
				typeData->GetIngests(true);
			if (ingests.empty()) {
				return protocol == "RTMPS"
					       ? DEFAULT_RTMPS_INGEST
					       : DEFAULT_RTMP_INGEST;
			}

			serverUrl = protocol == "RTMPS"
					    ? ingests[0].url_template_secure
					    : ingests[0].url_template;
		}

		if (!serverUrl.empty())
			return serverUrl.c_str();

		break;
	case OBS_SERVICE_CONNECT_INFO_STREAM_KEY:
		if (BandwidthTestEnabled() ? !bandwidthTestStreamKey.empty()
					   : !streamKey.empty()) {
			return BandwidthTestEnabled()
				       ? bandwidthTestStreamKey.c_str()
				       : streamKey.c_str();
		}
		break;
	default:
		break;
	}

	return nullptr;
}

bool TwitchConfig::CanTryToConnect()
{
	if ((!serverAuto && serverUrl.empty()) || streamKey.empty())
		return false;

	return true;
}

static bool ModifiedProtocolCb(void *priv, obs_properties_t *props,
			       obs_property_t *, obs_data_t *settings)
{
	TwitchService *typeData = reinterpret_cast<TwitchService *>(priv);

	std::string protocol = obs_data_get_string(settings, "protocol");
	obs_property_t *p = obs_properties_get(props, "server");

	if (protocol.empty())
		return false;

	std::vector<TwitchApi::Ingest> ingests = typeData->GetIngests();
	obs_property_list_clear(p);

	obs_property_list_add_string(p, obs_module_text("Twitch.Ingest.Auto"),
				     "auto");

	if (!ingests.empty()) {
		for (const TwitchApi::Ingest &ingest : ingests) {
			obs_property_list_add_string(
				p, ingest.name.c_str(),
				protocol == "RTMPS"
					? ingest.url_template_secure.c_str()
					: ingest.url_template.c_str());
		}
	}

	obs_property_list_add_string(p,
				     obs_module_text("Twitch.Ingest.Default"),
				     protocol == "RTMPS" ? DEFAULT_RTMPS_INGEST
							 : DEFAULT_RTMP_INGEST);

	return true;
}

obs_properties_t *TwitchConfig::GetProperties()
{
	obs_properties_t *ppts = obs_properties_create();
	obs_property_t *p;

	p = obs_properties_add_list(ppts, "protocol",
				    obs_module_text("Twitch.Protocol"),
				    OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);

	if (obs_is_output_protocol_registered("RTMPS"))
		obs_property_list_add_string(p, "RTMPS", "RTMPS");

	if (obs_is_output_protocol_registered("RTMP"))
		obs_property_list_add_string(p, "RTMP", "RTMP");

	obs_property_set_modified_callback2(p, ModifiedProtocolCb, typeData);

	obs_properties_add_list(ppts, "server",
				obs_module_text("Twitch.Server"),
				OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	p = obs_properties_add_text(ppts, "stream_key",
				    obs_module_text("Twitch.StreamKey"),
				    OBS_TEXT_PASSWORD);

	p = obs_properties_add_button(ppts, "get_stream_key",
				      obs_module_text("Twitch.GetStreamKey"),
				      nullptr);
	obs_property_button_set_type(p, OBS_BUTTON_URL);
	obs_property_button_set_url(p, (char *)STREAM_KEY_LINK);

	obs_properties_add_bool(
		ppts, "enforce_bwtest",
		obs_module_text("Twitch.EnforceBandwidthTestMode"));

	return ppts;
}
