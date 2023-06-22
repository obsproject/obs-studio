// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "youtube-config.hpp"

#ifdef OAUTH_ENABLED
#include <util/platform.h>
#endif

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

YouTubeConfig::YouTubeConfig(obs_data_t *settings, obs_service_t *self)
#ifdef OAUTH_ENABLED
	: serviceObj(self),
	  typeData(reinterpret_cast<YouTubeService *>(
		  obs_service_get_type_data(self)))
#endif
{
#ifdef OAUTH_ENABLED
	if (!obs_data_has_user_value(settings, "uuid")) {
		BPtr<char> newUuid = os_generate_uuid();
		obs_data_set_string(settings, "uuid", newUuid);
	}

	uuid = obs_data_get_string(settings, "uuid");
#else
	UNUSED_PARAMETER(self);
#endif

	Update(settings);
}

void YouTubeConfig::Update(obs_data_t *settings)
{
	protocol = obs_data_get_string(settings, "protocol");
	serverUrl = obs_data_get_string(settings, "server");

#ifdef OAUTH_ENABLED
	std::string newUuid = obs_data_get_string(settings, "uuid");

	if (newUuid != uuid) {
		if (oauth) {
			typeData->ReleaseOAuth(uuid, serviceObj);
			oauth = nullptr;
		}

		uuid = newUuid;
	}

	if (!oauth)
		oauth = typeData->GetOAuth(uuid, serviceObj);

	oauth->SetIngestionType(
		protocol == "HLS"
			? YouTubeApi::LiveStreamCdnIngestionType::HLS
			: YouTubeApi::LiveStreamCdnIngestionType::RTMP);

	if (!oauth->Connected()) {
		streamKey = obs_data_get_string(settings, "stream_key");
	}
#else
	streamKey = obs_data_get_string(settings, "stream_key");
#endif
}

YouTubeConfig::~YouTubeConfig()
{
#ifdef OAUTH_ENABLED
	typeData->ReleaseOAuth(uuid, serviceObj);
#endif
}

void YouTubeConfig::InfoGetDefault(obs_data_t *settings)
{
	obs_data_set_string(settings, "protocol", "RTMPS");
	obs_data_set_string(settings, "server", PRIMARY_INGESTS.rtmps);
}

const char *YouTubeConfig::ConnectInfo(uint32_t type)
{
	switch ((enum obs_service_connect_info)type) {
	case OBS_SERVICE_CONNECT_INFO_SERVER_URL:
		if (!serverUrl.empty())
			return serverUrl.c_str();
		break;
	case OBS_SERVICE_CONNECT_INFO_STREAM_KEY:
#ifdef OAUTH_ENABLED
		if (oauth->Connected()) {
			const char *streamKey = oauth->GetStreamKey();

			if (bandwidthTest) {
				/* Create throwaway stream key for bandwidth test */
				bandwidthTestStream = {};
				bandwidthTestStream.snippet.title =
					"OBS Studio Test Stream";
				bandwidthTestStream.cdn.ingestionType =
					oauth->GetIngestionType();
				bandwidthTestStream.cdn.resolution =
					YouTubeApi::LiveStreamCdnResolution::
						RESOLUTION_VARIABLE;
				bandwidthTestStream.cdn.frameRate =
					YouTubeApi::LiveStreamCdnFrameRate::
						FRAMERATE_VARIABLE;
				YouTubeApi::LiveStreamContentDetails
					contentDetails;
				contentDetails.isReusable = false;
				bandwidthTestStream.contentDetails =
					contentDetails;
				if (!oauth->InsertLiveStream(
					    bandwidthTestStream)) {
					blog(LOG_ERROR,
					     "[obs-youtube][ConnectInfo] "
					     "Bandwidth test stream could no be created");
					return nullptr;
				}

				if (bandwidthTestStream.cdn.ingestionInfo
					    .streamName.empty())
					return nullptr;

				return bandwidthTestStream.cdn.ingestionInfo
					.streamName.c_str();
			}

			if (streamKey)
				return streamKey;
		} else if (!streamKey.empty()) {
			return streamKey.c_str();
		}
#else
		if (!streamKey.empty())
			return streamKey.c_str();
#endif
		break;
	default:
		break;
	}

	return nullptr;
}

bool YouTubeConfig::CanTryToConnect()
{
#ifdef OAUTH_ENABLED
	if (serverUrl.empty())
		return false;

	if (oauth->Connected()) {
		return bandwidthTest ? true : !!oauth->GetStreamKey();
	} else {
		return !streamKey.empty();
	}
#else
	if (serverUrl.empty() || streamKey.empty())
		return false;
#endif

	return true;
}

static bool ModifiedProtocolCb(obs_properties_t *props, obs_property_t *,
			       obs_data_t *settings)
{
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

#ifdef OAUTH_ENABLED

static inline void AddChannelName(obs_properties_t *props,
				  YouTubeApi::ServiceOAuth *oauth)
{
	YouTubeApi::ChannelInfo info;
	if (oauth->GetChannelInfo(info)) {
		std::string channelTitle = "<b>";
		channelTitle += info.title;
		channelTitle += "</b>";
		obs_property_t *p =
			obs_properties_get(props, "connected_account");
		obs_property_set_long_description(p, channelTitle.c_str());
		obs_property_set_visible(p, true);
	}
}

static bool ConnectCb(obs_properties_t *props, obs_property_t *, void *priv)
{
	YouTubeApi::ServiceOAuth *oauth =
		reinterpret_cast<YouTubeApi::ServiceOAuth *>(priv);

	if (!oauth->Login())
		return false;

	AddChannelName(props, oauth);

	obs_property_set_visible(obs_properties_get(props, "connect"), false);
	obs_property_set_visible(obs_properties_get(props, "disconnect"), true);
	obs_property_set_visible(obs_properties_get(props, "use_stream_key"),
				 false);
	obs_property_set_visible(obs_properties_get(props, "stream_key"),
				 false);
	obs_property_set_visible(obs_properties_get(props, "get_stream_key"),
				 false);
	obs_property_set_enabled(obs_properties_get(props, "protocol"), false);

	return true;
}

static bool DisconnectCb(obs_properties_t *props, obs_property_t *, void *priv)
{
	YouTubeApi::ServiceOAuth *oauth =
		reinterpret_cast<YouTubeApi::ServiceOAuth *>(priv);

	if (!oauth->SignOut())
		return false;

	obs_property_set_visible(obs_properties_get(props, "connect"), true);
	obs_property_set_visible(obs_properties_get(props, "disconnect"),
				 false);
	obs_property_set_visible(obs_properties_get(props, "use_stream_key"),
				 true);
	obs_property_set_visible(obs_properties_get(props, "connected_account"),
				 false);
	obs_property_set_visible(obs_properties_get(props, "stream_key"),
				 false);
	obs_property_set_visible(obs_properties_get(props, "get_stream_key"),
				 false);
	obs_property_set_enabled(obs_properties_get(props, "protocol"), true);

	return true;
}

static bool UseStreamKeyCb(obs_properties_t *props, obs_property_t *, void *)
{
	obs_property_set_visible(obs_properties_get(props, "connect"), true);
	obs_property_set_visible(obs_properties_get(props, "disconnect"),
				 false);
	obs_property_set_visible(obs_properties_get(props, "use_stream_key"),
				 false);
	obs_property_set_visible(obs_properties_get(props, "connected_account"),
				 false);
	obs_property_set_visible(obs_properties_get(props, "stream_key"), true);
	obs_property_set_visible(obs_properties_get(props, "get_stream_key"),
				 true);

	return true;
}
#endif

obs_properties_t *YouTubeConfig::GetProperties()
{
	obs_properties_t *ppts = obs_properties_create();
	obs_property_t *p;

#ifdef OAUTH_ENABLED
	bool connected = oauth->Connected();
	p = obs_properties_add_text(
		ppts, "connected_account",
		obs_module_text("YouTube.Auth.ConnectedAccount"),
		OBS_TEXT_INFO);
	obs_property_set_visible(p, false);
	if (connected)
		AddChannelName(ppts, oauth);
#endif

	p = obs_properties_add_list(ppts, "protocol",
				    obs_module_text("YouTube.Protocol"),
				    OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);
#ifdef OAUTH_ENABLED
	obs_property_set_long_description(
		p, obs_module_text("YouTube.Auth.LockedProtocol"));
	obs_property_set_enabled(p, !connected);
#endif
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

	p = obs_properties_add_text(ppts, "stream_key",
				    obs_module_text("YouTube.StreamKey"),
				    OBS_TEXT_PASSWORD);
#ifdef OAUTH_ENABLED
	obs_property_set_visible(p, !streamKey.empty() && !connected);
#endif

	p = obs_properties_add_button(ppts, "get_stream_key",
				      obs_module_text("YouTube.GetStreamKey"),
				      nullptr);
	obs_property_button_set_type(p, OBS_BUTTON_URL);
	obs_property_button_set_url(p, (char *)STREAM_KEY_LINK);

#ifdef OAUTH_ENABLED
	obs_property_set_visible(p, !streamKey.empty() && !connected);

	p = obs_properties_add_button2(ppts, "connect",
				       obs_module_text("YouTube.Auth.Connect"),
				       ConnectCb, oauth);
	obs_property_set_visible(p, !connected);

	p = obs_properties_add_button2(
		ppts, "disconnect", obs_module_text("YouTube.Auth.Disconnect"),
		DisconnectCb, oauth);
	obs_property_set_visible(p, connected);

	p = obs_properties_add_button(ppts, "use_stream_key",
				      obs_module_text("YouTube.UseStreamKey"),
				      UseStreamKeyCb);
	obs_property_set_visible(p, streamKey.empty() && !connected);

	obs_properties_add_text(
		ppts, "legal_links",
		"<br><a href=\"https://www.youtube.com/t/terms\">"
		"YouTube Terms of Service</a><br>"
		"<a href=\"http://www.google.com/policies/privacy\">"
		"Google Privacy Policy</a><br>"
		"<a href=\"https://security.google.com/settings/security/permissions\">"
		"Google Third-Party Permissions</a>",
		OBS_TEXT_INFO);
#endif

	return ppts;
}
