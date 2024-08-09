#include "whip-service.h"

const char *audio_codecs[] = {"opus", nullptr};
const char *video_codecs[] = {"h264", "hevc", "av1", nullptr};

WHIPService::WHIPService(obs_data_t *settings, obs_service_t *)
	: server(),
	  bearer_token()
{
	Update(settings);
}

void WHIPService::Update(obs_data_t *settings)
{
	server = obs_data_get_string(settings, "server");
	bearer_token = obs_data_get_string(settings, "bearer_token");
}

obs_properties_t *WHIPService::Properties()
{
	obs_properties_t *ppts = obs_properties_create();

	obs_properties_add_text(ppts, "server", "URL", OBS_TEXT_DEFAULT);
	obs_properties_add_text(ppts, "bearer_token",
				obs_module_text("Service.BearerToken"),
				OBS_TEXT_PASSWORD);

	return ppts;
}

void WHIPService::ApplyEncoderSettings(obs_data_t *video_settings, obs_data_t *)
{
	// For now, ensure maximum compatibility with webrtc peers
	if (video_settings) {
		obs_data_set_int(video_settings, "bf", 0);
		obs_data_set_bool(video_settings, "repeat_headers", true);

		const char *profile =
			obs_data_get_string(video_settings, "profile");
		if (profile == nullptr || profile[0] == '\0') {
			obs_data_set_string(video_settings, "profile", "main");
			blog(LOG_INFO,
			     "[obs-webrtc] no H264 profile set, default to main");
		}
	}
}

const char *WHIPService::GetConnectInfo(enum obs_service_connect_info type)
{
	switch (type) {
	case OBS_SERVICE_CONNECT_INFO_SERVER_URL:
		return server.c_str();
	case OBS_SERVICE_CONNECT_INFO_BEARER_TOKEN:
		return bearer_token.c_str();
	default:
		return nullptr;
	}
}

bool WHIPService::CanTryToConnect()
{
	return !server.empty();
}

void register_whip_service()
{
	struct obs_service_info info = {};

	info.id = "whip_custom";
	info.get_name = [](void *) -> const char * {
		return obs_module_text("Service.Name");
	};
	info.create = [](obs_data_t *settings,
			 obs_service_t *service) -> void * {
		return new WHIPService(settings, service);
	};
	info.destroy = [](void *priv_data) {
		delete static_cast<WHIPService *>(priv_data);
	};
	info.update = [](void *priv_data, obs_data_t *settings) {
		static_cast<WHIPService *>(priv_data)->Update(settings);
	};
	info.get_properties = [](void *) -> obs_properties_t * {
		return WHIPService::Properties();
	};
	info.get_protocol = [](void *) -> const char * {
		return "WHIP";
	};
	info.get_url = [](void *priv_data) -> const char * {
		return static_cast<WHIPService *>(priv_data)->server.c_str();
	};
	info.get_output_type = [](void *) -> const char * {
		return "whip_output";
	};
	info.apply_encoder_settings = [](void *, obs_data_t *video_settings,
					 obs_data_t *audio_settings) {
		WHIPService::ApplyEncoderSettings(video_settings,
						  audio_settings);
	};
	info.get_supported_video_codecs = [](void *) -> const char ** {
		return video_codecs;
	};
	info.get_supported_audio_codecs = [](void *) -> const char ** {
		return audio_codecs;
	};
	info.can_try_to_connect = [](void *priv_data) -> bool {
		return static_cast<WHIPService *>(priv_data)->CanTryToConnect();
	};
	info.get_connect_info = [](void *priv_data,
				   uint32_t type) -> const char * {
		return static_cast<WHIPService *>(priv_data)->GetConnectInfo(
			(enum obs_service_connect_info)type);
	};
	obs_register_service(&info);
}
