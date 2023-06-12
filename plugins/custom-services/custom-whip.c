// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include <obs-module.h>

struct whip_service {
	char *server;
	char *bearer_token;
};

static const char *whip_service_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return obs_module_text("CustomServices.CustomWHIP");
}

static void whip_service_update(void *data, obs_data_t *settings)
{
	struct whip_service *service = data;

	bfree(service->server);
	bfree(service->bearer_token);

#define GET_STRING_SETTINGS(name)                                             \
	service->name =                                                       \
		obs_data_has_user_value(settings, #name) &&                   \
				(strcmp(obs_data_get_string(settings, #name), \
					"") != 0)                             \
			? bstrdup(obs_data_get_string(settings, #name))       \
			: NULL

	GET_STRING_SETTINGS(server);

	GET_STRING_SETTINGS(bearer_token);

#undef GET_STRING_SETTINGS
}

static void whip_service_destroy(void *data)
{
	struct whip_service *service = data;

	bfree(service->server);
	bfree(service->bearer_token);
	bfree(service);
}

static void *whip_service_create(obs_data_t *settings, obs_service_t *service)
{
	UNUSED_PARAMETER(service);

	struct whip_service *data = bzalloc(sizeof(struct whip_service));
	whip_service_update(data, settings);

	return data;
}

static const char *whip_service_protocol(void *data)
{
	UNUSED_PARAMETER(data);
	return "WHIP";
}

static const char *whip_service_connect_info(void *data, uint32_t type)
{
	struct whip_service *service = data;

	switch ((enum obs_service_connect_info)type) {
	case OBS_SERVICE_CONNECT_INFO_SERVER_URL:
		return service->server;
	case OBS_SERVICE_CONNECT_INFO_BEARER_TOKEN:
		return service->bearer_token;
	default:
		return NULL;
	}
}

enum obs_service_audio_track_cap whip_service_audio_track_cap(void *data)
{
	UNUSED_PARAMETER(data);

	return OBS_SERVICE_AUDIO_SINGLE_TRACK;
}

bool whip_service_can_try_to_connect(void *data)
{
	struct whip_service *service = data;

	if (!service->server || !service->bearer_token)
		return false;

	return true;
}

static void whip_service_apply_settings(void *data, obs_data_t *video_settings,
					obs_data_t *audio_settings)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(audio_settings);

	if (video_settings == NULL)
		return;

	obs_data_set_bool(video_settings, "repeat_headers", true);

	obs_data_set_int(video_settings, "bf", 0);
	obs_data_set_string(video_settings, "rate_control", "CBR");
}

static obs_properties_t *whip_service_properties(void *data)
{
	UNUSED_PARAMETER(data);

	obs_properties_t *ppts = obs_properties_create();

	/* Add server field */
	obs_properties_add_text(ppts, "server",
				obs_module_text("CustomServices.Server"),
				OBS_TEXT_DEFAULT);

	/* Add connect info fields */
	obs_properties_add_text(ppts, "bearer_token",
				obs_module_text("CustomServices.BearerToken"),
				OBS_TEXT_PASSWORD);

	return ppts;
}

const struct obs_service_info custom_whip = {
	.id = "custom_whip",
	.flags = OBS_SERVICE_INTERNAL,
	.supported_protocols = "WHIP",
	.get_name = whip_service_name,
	.create = whip_service_create,
	.destroy = whip_service_destroy,
	.update = whip_service_update,
	.get_protocol = whip_service_protocol,
	.get_properties = whip_service_properties,
	.get_connect_info = whip_service_connect_info,
	.can_try_to_connect = whip_service_can_try_to_connect,
	.get_audio_track_cap = whip_service_audio_track_cap,
	.apply_encoder_settings = whip_service_apply_settings,
};
