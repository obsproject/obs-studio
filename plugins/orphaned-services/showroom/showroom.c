// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include <obs-module.h>
#include <obs-service.h>

#include "showroom.h"
#include "showroom-ingest.h"

static const char *API_URL =
	"https://www.showroom-live.com/api/obs/streaming_info?obs_key=";
static const char *VIDEO_CODEC[] = {"h264", NULL};

struct showroom {
	char *key;
	struct showroom_ingest *ingest;
};

static const char *showroom_getname(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return "SHOWROOM";
}

static void showroom_update(void *data, obs_data_t *settings)
{
	UNUSED_PARAMETER(settings);

	struct showroom *service = data;
	bfree(service->key);
	service->key = bstrdup(obs_data_get_string(settings, "key"));
}

static void *showroom_create(obs_data_t *settings, obs_service_t *service)
{
	UNUSED_PARAMETER(service);

	struct showroom *data = bzalloc(sizeof(struct showroom));
	showroom_update(data, settings);

	return data;
}

static void showroom_destroy(void *data)
{
	struct showroom *service = data;
	bfree(service->key);
	bfree(service);
}

static const char *showroom_get_protocol(void *data)
{
	UNUSED_PARAMETER(data);
	return "RTMP";
}

static const char *showroom_get_connect_info(void *data, uint32_t type)
{
	struct showroom *service = data;
	if (!service->ingest) {
		service->ingest = showroom_get_ingest(API_URL, service->key);
	}

	switch ((enum obs_service_connect_info)type) {
	case OBS_SERVICE_CONNECT_INFO_SERVER_URL:
		return service->ingest->url;
	case OBS_SERVICE_CONNECT_INFO_STREAM_KEY:
		return service->ingest->key;
	default:
		return NULL;
	}
}

static bool showroom_can_try_to_connect(void *data)
{
	struct showroom *service = data;
	if (!service->ingest) {
		service->ingest = showroom_get_ingest(API_URL, service->key);
	}

	return (service->ingest->key != NULL &&
		service->ingest->key[0] != '\0');
}

static const char **showroom_get_supported_video_codecs(void *data)
{
	UNUSED_PARAMETER(data);
	return VIDEO_CODEC;
}

static obs_properties_t *showroom_properties(void *data)
{
	UNUSED_PARAMETER(data);

	obs_properties_t *ppts = obs_properties_create();

	obs_properties_add_text(ppts, "key", obs_module_text("Access.Key"),
				OBS_TEXT_PASSWORD);

	return ppts;
}

void load_showroom(void)
{
	struct obs_service_info showroom_service = {
		.id = "showroom",
		.flags = OBS_SERVICE_UNCOMMON | OBS_SERVICE_DEPRECATED,
		.get_name = showroom_getname,
		.create = showroom_create,
		.destroy = showroom_destroy,
		.update = showroom_update,
		.get_properties = showroom_properties,
		.get_protocol = showroom_get_protocol,
		.get_connect_info = showroom_get_connect_info,
		.get_supported_video_codecs =
			showroom_get_supported_video_codecs,
		.can_try_to_connect = showroom_can_try_to_connect,
		.supported_protocols = "RTMP",
	};

	obs_register_service(&showroom_service);
}

void unload_showroom(void)
{
	free_showroom_data();
}
