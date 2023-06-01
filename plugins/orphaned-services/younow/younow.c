// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include <obs-module.h>
#include <obs-service.h>

#include "younow.h"
#include "younow-ingest.h"

static const char *API_URL =
	"https://api.younow.com/php/api/broadcast/ingest?id=";

struct younow {
	char *key;
};

static const char *younow_getname(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return "YouNow";
}

static void younow_update(void *data, obs_data_t *settings)
{
	UNUSED_PARAMETER(settings);

	struct younow *service = data;
	bfree(service->key);
	service->key = bstrdup(obs_data_get_string(settings, "key"));
}

static void *younow_create(obs_data_t *settings, obs_service_t *service)
{
	UNUSED_PARAMETER(service);

	struct younow *data = bzalloc(sizeof(struct younow));
	younow_update(data, settings);

	return data;
}

static void younow_destroy(void *data)
{
	struct younow *service = data;
	bfree(service->key);
	bfree(service);
}

static const char *younow_get_protocol(void *data)
{
	UNUSED_PARAMETER(data);
	return "FTL";
}

static const char *younow_get_connect_info(void *data, uint32_t type)
{
	struct younow *service = data;

	switch ((enum obs_service_connect_info)type) {
	case OBS_SERVICE_CONNECT_INFO_SERVER_URL:
		return younow_get_ingest(API_URL, service->key);
	case OBS_SERVICE_CONNECT_INFO_STREAM_KEY:
		return service->key;
	default:
		return NULL;
	}
}

static bool younow_can_try_to_connect(void *data)
{
	struct younow *service = data;

	return (service->key != NULL && service->key[0] != '\0');
}

static obs_properties_t *younow_properties(void *data)
{
	UNUSED_PARAMETER(data);

	obs_properties_t *ppts = obs_properties_create();

	obs_properties_add_text(ppts, "key", obs_module_text("Api.Key"),
				OBS_TEXT_PASSWORD);

	return ppts;
}

void load_younow(void)
{
	struct obs_service_info younow_service = {
		.id = "younow",
		.flags = OBS_SERVICE_UNCOMMON | OBS_SERVICE_DEPRECATED,
		.get_name = younow_getname,
		.create = younow_create,
		.destroy = younow_destroy,
		.update = younow_update,
		.get_properties = younow_properties,
		.get_protocol = younow_get_protocol,
		.get_connect_info = younow_get_connect_info,
		.can_try_to_connect = younow_can_try_to_connect,
		.supported_protocols = "FTL",
	};

	obs_register_service(&younow_service);
}
