// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include <obs-module.h>

struct srt_service {
	char *server;
	char *stream_id;
	char *encrypt_passphrase;
};

static const char *srt_service_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return obs_module_text("CustomServices.CustomSRT");
}

static void srt_service_update(void *data, obs_data_t *settings)
{
	struct srt_service *service = data;

	bfree(service->server);
	bfree(service->stream_id);
	bfree(service->encrypt_passphrase);

#define GET_STRING_SETTINGS(name)                                             \
	service->name =                                                       \
		obs_data_has_user_value(settings, #name) &&                   \
				(strcmp(obs_data_get_string(settings, #name), \
					"") != 0)                             \
			? bstrdup(obs_data_get_string(settings, #name))       \
			: NULL

	GET_STRING_SETTINGS(server);

	GET_STRING_SETTINGS(stream_id);

	GET_STRING_SETTINGS(encrypt_passphrase);

#undef GET_STRING_SETTINGS
}

static void srt_service_destroy(void *data)
{
	struct srt_service *service = data;

	bfree(service->server);
	bfree(service->stream_id);
	bfree(service->encrypt_passphrase);
	bfree(service);
}

static void *srt_service_create(obs_data_t *settings, obs_service_t *service)
{
	UNUSED_PARAMETER(service);

	struct srt_service *data = bzalloc(sizeof(struct srt_service));
	srt_service_update(data, settings);

	return data;
}

static const char *srt_service_protocol(void *data)
{
	UNUSED_PARAMETER(data);
	return "SRT";
}

static const char *srt_service_connect_info(void *data, uint32_t type)
{
	struct srt_service *service = data;

	switch ((enum obs_service_connect_info)type) {
	case OBS_SERVICE_CONNECT_INFO_SERVER_URL:
		return service->server;
	case OBS_SERVICE_CONNECT_INFO_STREAM_ID:
		return service->stream_id;
	case OBS_SERVICE_CONNECT_INFO_ENCRYPT_PASSPHRASE:
		return service->encrypt_passphrase;
	default:
		return NULL;
	}
}

enum obs_service_audio_track_cap srt_service_audio_track_cap(void *data)
{
	UNUSED_PARAMETER(data);

	return OBS_SERVICE_AUDIO_MULTI_TRACK;
}

bool srt_service_can_try_to_connect(void *data)
{
	struct srt_service *service = data;

	if (!service->server)
		return false;

	return true;
}

static void srt_service_apply_settings(void *data, obs_data_t *video_settings,
				       obs_data_t *audio_settings)
{
	UNUSED_PARAMETER(data);

	if (video_settings != NULL)
		obs_data_set_bool(video_settings, "repeat_headers", true);

	if (audio_settings != NULL)
		obs_data_set_bool(audio_settings, "set_to_ADTS", true);
}

static obs_properties_t *srt_service_properties(void *data)
{
	UNUSED_PARAMETER(data);

	obs_properties_t *ppts = obs_properties_create();

	/* Add server field */
	obs_properties_add_text(ppts, "server",
				obs_module_text("CustomServices.Server"),
				OBS_TEXT_DEFAULT);

	/* Add connect info fields */
	obs_properties_add_text(ppts, "stream_id",
				obs_module_text("CustomServices.StreamID"),
				OBS_TEXT_PASSWORD);
	obs_properties_add_text(
		ppts, "encrypt_passphrase",
		obs_module_text("CustomServices.EncryptPassphrase.Optional"),
		OBS_TEXT_PASSWORD);

	return ppts;
}

const struct obs_service_info custom_srt = {
	.id = "custom_srt",
	.flags = OBS_SERVICE_INTERNAL,
	.supported_protocols = "SRT",
	.get_name = srt_service_name,
	.create = srt_service_create,
	.destroy = srt_service_destroy,
	.update = srt_service_update,
	.get_protocol = srt_service_protocol,
	.get_properties = srt_service_properties,
	.get_connect_info = srt_service_connect_info,
	.can_try_to_connect = srt_service_can_try_to_connect,
	.get_audio_track_cap = srt_service_audio_track_cap,
	.apply_encoder_settings = srt_service_apply_settings,
};
