// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include <obs-module.h>
#include <obs-service.h>

#include "dacast.h"
#include "dacast-ingest.h"

static const char *API_ENDPOINT =
	"https://developer.dacast.com/v3/encoder-setup/";
static const char *VIDEO_CODEC[] = {"h264", NULL};

struct dacast {
	char *key;
	struct dacast_ingest *ingest;
};

static const char *dacast_getname(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return "Dacast";
}

static void dacast_update(void *data, obs_data_t *settings)
{
	UNUSED_PARAMETER(settings);

	struct dacast *service = data;
	bfree(service->key);
	service->key = bstrdup(obs_data_get_string(settings, "key"));
}

static void *dacast_create(obs_data_t *settings, obs_service_t *service)
{
	UNUSED_PARAMETER(service);

	struct dacast *data = bzalloc(sizeof(struct dacast));
	dacast_update(data, settings);

	return data;
}

static void dacast_destroy(void *data)
{
	struct dacast *service = data;
	bfree(service->key);
	bfree(service);
}

static const char *dacast_get_protocol(void *data)
{
	UNUSED_PARAMETER(data);
	return "RTMP";
}

static const char *dacast_get_connect_info(void *data, uint32_t type)
{
	struct dacast *service = data;
	if (!service->ingest) {
		dacast_ingests_load_data(API_ENDPOINT, service->key);
		service->ingest = dacast_ingest(service->key);
	}

	switch ((enum obs_service_connect_info)type) {
	case OBS_SERVICE_CONNECT_INFO_SERVER_URL:
		return service->ingest->url;
	case OBS_SERVICE_CONNECT_INFO_STREAM_KEY:
		return service->ingest->streamkey;
	case OBS_SERVICE_CONNECT_INFO_USERNAME:
		return service->ingest->username;
	case OBS_SERVICE_CONNECT_INFO_PASSWORD:
		return service->ingest->password;
	default:
		return NULL;
	}
}

static bool dacast_can_try_to_connect(void *data)
{
	struct dacast *service = data;
	if (!service->ingest) {
		dacast_ingests_load_data(API_ENDPOINT, service->key);
		service->ingest = dacast_ingest(service->key);
	}

	return (service->ingest->streamkey != NULL &&
		service->ingest->streamkey[0] != '\0');
}

static const char **dacast_get_supported_video_codecs(void *data)
{
	UNUSED_PARAMETER(data);
	return VIDEO_CODEC;
}

static obs_properties_t *dacast_properties(void *data)
{
	UNUSED_PARAMETER(data);

	obs_properties_t *ppts = obs_properties_create();

	obs_properties_add_text(ppts, "key", obs_module_text("Api.Key"),
				OBS_TEXT_PASSWORD);

	return ppts;
}

void load_dacast(void)
{
	init_dacast_data();

	struct obs_service_info dacast_service = {
		.id = "dacast",
		.flags = OBS_SERVICE_UNCOMMON | OBS_SERVICE_DEPRECATED,
		.get_name = dacast_getname,
		.create = dacast_create,
		.destroy = dacast_destroy,
		.update = dacast_update,
		.get_properties = dacast_properties,
		.get_protocol = dacast_get_protocol,
		.get_connect_info = dacast_get_connect_info,
		.get_supported_video_codecs = dacast_get_supported_video_codecs,
		.can_try_to_connect = dacast_can_try_to_connect,
		.supported_protocols = "RTMP",
	};

	obs_register_service(&dacast_service);
}

void unload_dacast(void)
{
	unload_dacast_data();
}
