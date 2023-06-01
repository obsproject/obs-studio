// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include <obs-module.h>
#include <obs-service.h>

#include "nimotv.h"
#include "nimotv-ingest.h"

static const char *AUTO = "auto";
static const char *VIDEO_CODEC[] = {"h264", NULL};

struct nimotv {
	char *server;
	char *key;
};

static const char *nimotv_getname(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return "Nimo TV";
}

static void nimotv_update(void *data, obs_data_t *settings)
{
	UNUSED_PARAMETER(settings);

	struct nimotv *service = data;
	bfree(service->server);
	bfree(service->key);
	service->server = bstrdup(obs_data_get_string(settings, "server"));
	service->key = bstrdup(obs_data_get_string(settings, "key"));
}

static void *nimotv_create(obs_data_t *settings, obs_service_t *service)
{
	UNUSED_PARAMETER(service);

	struct nimotv *data = bzalloc(sizeof(struct nimotv));
	nimotv_update(data, settings);

	return data;
}

static void nimotv_destroy(void *data)
{
	struct nimotv *service = data;
	bfree(service->server);
	bfree(service->key);
	bfree(service);
}

static const char *nimotv_get_protocol(void *data)
{
	UNUSED_PARAMETER(data);
	return "RTMP";
}

static const char *nimotv_get_connect_info(void *data, uint32_t type)
{
	struct nimotv *service = data;

	switch ((enum obs_service_connect_info)type) {
	case OBS_SERVICE_CONNECT_INFO_SERVER_URL:
		if (service->server && strcmp(service->server, AUTO) == 0)
			return nimotv_get_ingest(service->key);
		return service->server;
	case OBS_SERVICE_CONNECT_INFO_STREAM_KEY:
		return service->key;
	default:
		return NULL;
	}
}

static bool nimotv_can_try_to_connect(void *data)
{
	const char *server = nimotv_get_connect_info(
		data, OBS_SERVICE_CONNECT_INFO_SERVER_URL);
	const char *key = nimotv_get_connect_info(
		data, OBS_SERVICE_CONNECT_INFO_STREAM_KEY);

	return (server != NULL && server[0] != '\0') &&
	       (key != NULL && key[0] != '\0');
}

static const char **nimotv_get_supported_video_codecs(void *data)
{
	UNUSED_PARAMETER(data);
	return VIDEO_CODEC;
}

static obs_properties_t *nimotv_properties(void *data)
{
	UNUSED_PARAMETER(data);

	obs_properties_t *ppts = obs_properties_create();
	obs_property_t *p;

	p = obs_properties_add_list(ppts, "server", obs_module_text("Server"),
				    OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);

	obs_property_list_add_string(p, obs_module_text("Server.Auto"), AUTO);

	obs_property_list_add_string(p, "Global:1",
				     "rtmp://wspush.rtmp.nimo.tv/live/");
	obs_property_list_add_string(p, "Global:2",
				     "rtmp://txpush.rtmp.nimo.tv/live/");
	obs_property_list_add_string(p, "Global:3",
				     "rtmp://alpush.rtmp.nimo.tv/live/");

	obs_properties_add_text(ppts, "key", obs_module_text("Stream.Key"),
				OBS_TEXT_PASSWORD);

	return ppts;
}

void load_nimotv(void)
{
	struct obs_service_info nimotv_service = {
		.id = "nimotv",
		.flags = OBS_SERVICE_UNCOMMON | OBS_SERVICE_DEPRECATED,
		.get_name = nimotv_getname,
		.create = nimotv_create,
		.destroy = nimotv_destroy,
		.update = nimotv_update,
		.get_properties = nimotv_properties,
		.get_protocol = nimotv_get_protocol,
		.get_connect_info = nimotv_get_connect_info,
		.get_supported_video_codecs = nimotv_get_supported_video_codecs,
		.can_try_to_connect = nimotv_can_try_to_connect,
		.supported_protocols = "RTMP",
	};

	obs_register_service(&nimotv_service);
}
