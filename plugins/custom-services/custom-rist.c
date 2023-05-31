// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include <obs-module.h>

struct rist_service {
	char *server;
	char *username;
	char *password;
	char *encrypt_passphrase;
};

static const char *rist_service_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return obs_module_text("CustomServices.CustomRIST");
}

static void rist_service_update(void *data, obs_data_t *settings)
{
	struct rist_service *service = data;

	bfree(service->server);
	bfree(service->username);
	bfree(service->password);
	bfree(service->encrypt_passphrase);

#define GET_STRING_SETTINGS(name)                                             \
	service->name =                                                       \
		obs_data_has_user_value(settings, #name) &&                   \
				(strcmp(obs_data_get_string(settings, #name), \
					"") != 0)                             \
			? bstrdup(obs_data_get_string(settings, #name))       \
			: NULL

	GET_STRING_SETTINGS(server);

	GET_STRING_SETTINGS(username);
	GET_STRING_SETTINGS(password);

	GET_STRING_SETTINGS(encrypt_passphrase);

#undef GET_STRING_SETTINGS
}

static void rist_service_destroy(void *data)
{
	struct rist_service *service = data;

	bfree(service->server);
	bfree(service->username);
	bfree(service->password);
	bfree(service->encrypt_passphrase);
	bfree(service);
}

static void *rist_service_create(obs_data_t *settings, obs_service_t *service)
{
	UNUSED_PARAMETER(service);

	struct rist_service *data = bzalloc(sizeof(struct rist_service));
	rist_service_update(data, settings);

	return data;
}

static const char *rist_service_protocol(void *data)
{
	UNUSED_PARAMETER(data);
	return "RIST";
}

static const char *rist_service_connect_info(void *data, uint32_t type)
{
	struct rist_service *service = data;

	switch ((enum obs_service_connect_info)type) {
	case OBS_SERVICE_CONNECT_INFO_SERVER_URL:
		return service->server;
	case OBS_SERVICE_CONNECT_INFO_USERNAME:
		return service->username;
	case OBS_SERVICE_CONNECT_INFO_PASSWORD:
		return service->password;
	case OBS_SERVICE_CONNECT_INFO_ENCRYPT_PASSPHRASE:
		return service->encrypt_passphrase;
	default:
		return NULL;
	}
}

enum obs_service_audio_track_cap rist_service_audio_track_cap(void *data)
{
	UNUSED_PARAMETER(data);

	return OBS_SERVICE_AUDIO_MULTI_TRACK;
}

bool rist_service_can_try_to_connect(void *data)
{
	struct rist_service *service = data;

	if (!service->server)
		return false;

	return true;
}

static obs_properties_t *rist_service_properties(void *data)
{
	UNUSED_PARAMETER(data);

	obs_properties_t *ppts = obs_properties_create();

	/* Add server field */
	obs_properties_add_text(ppts, "server",
				obs_module_text("CustomServices.Server"),
				OBS_TEXT_DEFAULT);

	/* Add connect info fields */
	obs_properties_add_text(
		ppts, "username",
		obs_module_text("CustomServices.Username.Optional"),
		OBS_TEXT_DEFAULT);
	obs_properties_add_text(
		ppts, "password",
		obs_module_text("CustomServices.Password.Optional"),
		OBS_TEXT_PASSWORD);
	obs_properties_add_text(
		ppts, "encrypt_passphrase",
		obs_module_text("CustomServices.EncryptPassphrase.Optional"),
		OBS_TEXT_PASSWORD);

	return ppts;
}

const struct obs_service_info custom_rist = {
	.id = "custom_rist",
	.flags = OBS_SERVICE_INTERNAL,
	.supported_protocols = "RIST",
	.get_name = rist_service_name,
	.create = rist_service_create,
	.destroy = rist_service_destroy,
	.update = rist_service_update,
	.get_protocol = rist_service_protocol,
	.get_properties = rist_service_properties,
	.get_connect_info = rist_service_connect_info,
	.can_try_to_connect = rist_service_can_try_to_connect,
	.get_audio_track_cap = rist_service_audio_track_cap,
};
