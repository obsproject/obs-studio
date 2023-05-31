// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include <obs-module.h>

struct hls_service {
	char *server;
	char *stream_key;
};

static const char *hls_service_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return obs_module_text("CustomServices.CustomHLS");
}

static void hls_service_update(void *data, obs_data_t *settings)
{
	struct hls_service *service = data;

	bfree(service->server);
	bfree(service->stream_key);

#define GET_STRING_SETTINGS(name)                                             \
	service->name =                                                       \
		obs_data_has_user_value(settings, #name) &&                   \
				(strcmp(obs_data_get_string(settings, #name), \
					"") != 0)                             \
			? bstrdup(obs_data_get_string(settings, #name))       \
			: NULL

	GET_STRING_SETTINGS(server);

	GET_STRING_SETTINGS(stream_key);

#undef GET_STRING_SETTINGS
}

static void hls_service_destroy(void *data)
{
	struct hls_service *service = data;

	bfree(service->server);
	bfree(service->stream_key);
	bfree(service);
}

static void *hls_service_create(obs_data_t *settings, obs_service_t *service)
{
	UNUSED_PARAMETER(service);

	struct hls_service *data = bzalloc(sizeof(struct hls_service));
	hls_service_update(data, settings);

	return data;
}

static const char *hls_service_protocol(void *data)
{
	UNUSED_PARAMETER(data);
	return "HLS";
}

static const char *hls_service_connect_info(void *data, uint32_t type)
{
	struct hls_service *service = data;

	switch ((enum obs_service_connect_info)type) {
	case OBS_SERVICE_CONNECT_INFO_SERVER_URL:
		return service->server;
	case OBS_SERVICE_CONNECT_INFO_STREAM_KEY:
		return !service->stream_key ? "" : service->stream_key;
	default:
		return NULL;
	}
}

enum obs_service_audio_track_cap hls_service_audio_track_cap(void *data)
{
	UNUSED_PARAMETER(data);

	return OBS_SERVICE_AUDIO_MULTI_TRACK;
}

bool hls_service_can_try_to_connect(void *data)
{
	struct hls_service *service = data;

	if (!service->server)
		return false;

	/* Require stream key */
	if (!service->stream_key)
		return false;

	return true;
}

static obs_properties_t *hls_service_properties(void *data)
{
	UNUSED_PARAMETER(data);

	obs_properties_t *ppts = obs_properties_create();

	/* Add warning about how HLS stream key works */
	obs_property_t *p = obs_properties_add_text(
		ppts, "hls_warning",
		obs_module_text("CustomServices.HLS.Warning"), OBS_TEXT_INFO);
	obs_property_text_set_info_type(p, OBS_TEXT_INFO_WARNING);

	/* Add server field */
	obs_properties_add_text(ppts, "server",
				obs_module_text("CustomServices.Server"),
				OBS_TEXT_DEFAULT);

	/* Add connect info fields */
	obs_properties_add_text(ppts, "stream_key",
				obs_module_text("CustomServices.StreamID.Key"),
				OBS_TEXT_PASSWORD);

	return ppts;
}

const struct obs_service_info custom_hls = {
	.id = "custom_hls",
	.flags = OBS_SERVICE_INTERNAL,
	.supported_protocols = "HLS",
	.get_name = hls_service_name,
	.create = hls_service_create,
	.destroy = hls_service_destroy,
	.update = hls_service_update,
	.get_protocol = hls_service_protocol,
	.get_properties = hls_service_properties,
	.get_connect_info = hls_service_connect_info,
	.can_try_to_connect = hls_service_can_try_to_connect,
	.get_audio_track_cap = hls_service_audio_track_cap,
};
