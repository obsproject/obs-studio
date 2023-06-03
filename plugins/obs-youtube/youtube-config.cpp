// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "youtube-config.hpp"

struct YouTubeIngest {
	const char *rtmp;
	const char *rtmps;
	const char *hls;
};

constexpr const char *MORE_INFO_LINK =
	"https://developers.google.com/youtube/v3/live/guides/ingestion-protocol-comparison";
constexpr const char *STREAM_KEY_LINK =
	"https://www.youtube.com/live_dashboard";

/* Note: There is no API to fetch those easily without authentification */
constexpr YouTubeIngest PRIMARY_INGESTS = {
	"rtmp://a.rtmp.youtube.com/live2",
	"rtmps://a.rtmps.youtube.com:443/live2",
	"https://a.upload.youtube.com/http_upload_hls?cid={stream_key}&copy=0&file=out.m3u8"};
constexpr YouTubeIngest BACKUP_INGESTS = {
	"rtmp://b.rtmp.youtube.com/live2?backup=1",
	"rtmps://b.rtmps.youtube.com:443/live2?backup=1",
	"https://b.upload.youtube.com/http_upload_hls?cid={stream_key}&copy=1&file=out.m3u8"};

YouTubeConfig::YouTubeConfig(obs_data_t *settings, obs_service_t * /* self */)
{
	Update(settings);
}

void YouTubeConfig::Update(obs_data_t *settings)
{
	protocol = obs_data_get_string(settings, "protocol");
	serverUrl = obs_data_get_string(settings, "server");

	streamKey = obs_data_get_string(settings, "stream_key");
}

void YouTubeConfig::InfoGetDefault(obs_data_t *settings)
{
	obs_data_set_default_string(settings, "protocol", "RTMPS");
	obs_data_set_default_string(settings, "server", PRIMARY_INGESTS.rtmps);
}

const char *YouTubeConfig::ConnectInfo(uint32_t type)
{
	switch ((enum obs_service_connect_info)type) {
	case OBS_SERVICE_CONNECT_INFO_SERVER_URL:
		if (!serverUrl.empty())
			return serverUrl.c_str();
		break;
	case OBS_SERVICE_CONNECT_INFO_STREAM_ID:
		if (!streamKey.empty())
			return streamKey.c_str();
		break;
	default:
		break;
	}

	return nullptr;
}

bool YouTubeConfig::CanTryToConnect()
{
	if (serverUrl.empty() || streamKey.empty())
		return false;

	return true;
}

static bool ModifiedProtocolCb(obs_properties_t *props, obs_property_t *,
			       obs_data_t *settings)
{
	blog(LOG_DEBUG, "ModifiedProtocolCb");
	std::string protocol = obs_data_get_string(settings, "protocol");
	obs_property_t *p = obs_properties_get(props, "server");

	if (protocol.empty())
		return false;

	const char *primary_ingest = nullptr;
	const char *backup_ingest = nullptr;
	if (protocol == "RTMPS") {
		primary_ingest = PRIMARY_INGESTS.rtmps;
		backup_ingest = BACKUP_INGESTS.rtmps;

	} else if (protocol == "RTMP") {
		primary_ingest = PRIMARY_INGESTS.rtmp;
		backup_ingest = BACKUP_INGESTS.rtmp;
	} else if (protocol == "HLS") {
		primary_ingest = PRIMARY_INGESTS.hls;
		backup_ingest = BACKUP_INGESTS.hls;
	}

	obs_property_list_clear(p);
	obs_property_list_add_string(
		p, obs_module_text("YouTube.Ingest.Primary"), primary_ingest);
	obs_property_list_add_string(
		p, obs_module_text("YouTube.Ingest.Backup"), backup_ingest);

	return true;
}

obs_properties_t *YouTubeConfig::GetProperties()
{
	obs_properties_t *ppts = obs_properties_create();
	obs_property_t *p;

	p = obs_properties_add_list(ppts, "protocol",
				    obs_module_text("YouTube.Protocol"),
				    OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);
	if (obs_is_output_protocol_registered("RTMPS"))
		obs_property_list_add_string(p, "RTMPS", "RTMPS");

	if (obs_is_output_protocol_registered("RTMP"))
		obs_property_list_add_string(p, obs_module_text("YouTube.RTMP"),
					     "RTMP");

	if (obs_is_output_protocol_registered("HLS"))
		obs_property_list_add_string(p, "HLS", "HLS");

	obs_property_set_modified_callback(p, ModifiedProtocolCb);

	p = obs_properties_add_button(ppts, "more_info",
				      obs_module_text("YouTube.MoreInfo"),
				      nullptr);
	obs_property_button_set_type(p, OBS_BUTTON_URL);
	obs_property_button_set_url(p, (char *)MORE_INFO_LINK);

	obs_properties_add_list(ppts, "server",
				obs_module_text("YouTube.Server"),
				OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	obs_properties_add_text(ppts, "stream_key",
				obs_module_text("YouTube.StreamKey"),
				OBS_TEXT_PASSWORD);

	p = obs_properties_add_button(ppts, "get_stream_key",
				      obs_module_text("YouTube.GetStreamKey"),
				      nullptr);
	obs_property_button_set_type(p, OBS_BUTTON_URL);
	obs_property_button_set_url(p, (char *)STREAM_KEY_LINK);

	return ppts;
}
