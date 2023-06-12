// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include <obs-module.h>

struct custom_service {
	char *protocol;

	char *server;
	char *stream_id;
	char *username;
	char *password;
	char *encrypt_passphrase;
	char *bearer_token;
};

static const char *custom_service_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return obs_module_text("CustomServices.CustomService.Name");
}

static void custom_service_defaults(obs_data_t *settings)
{
	obs_data_set_string(settings, "protocol", "RTMPS");
}

static void custom_service_update(void *data, obs_data_t *settings)
{
	struct custom_service *service = data;

	bfree(service->protocol);
	bfree(service->server);
	bfree(service->stream_id);
	bfree(service->username);
	bfree(service->password);
	bfree(service->encrypt_passphrase);
	bfree(service->bearer_token);

#define GET_STRING_SETTINGS(name)                                             \
	service->name =                                                       \
		obs_data_has_user_value(settings, #name) &&                   \
				(strcmp(obs_data_get_string(settings, #name), \
					"") != 0)                             \
			? bstrdup(obs_data_get_string(settings, #name))       \
			: NULL

	GET_STRING_SETTINGS(protocol);

	GET_STRING_SETTINGS(server);

	GET_STRING_SETTINGS(stream_id);

	GET_STRING_SETTINGS(username);
	GET_STRING_SETTINGS(password);

	GET_STRING_SETTINGS(encrypt_passphrase);

	GET_STRING_SETTINGS(bearer_token);

#undef GET_STRING_SETTINGS
}

static void custom_service_destroy(void *data)
{
	struct custom_service *service = data;

	bfree(service->protocol);
	bfree(service->server);
	bfree(service->stream_id);
	bfree(service->username);
	bfree(service->password);
	bfree(service->encrypt_passphrase);
	bfree(service->bearer_token);
	bfree(service);
}

static void *custom_service_create(obs_data_t *settings, obs_service_t *service)
{
	UNUSED_PARAMETER(service);

	struct custom_service *data = bzalloc(sizeof(struct custom_service));
	custom_service_update(data, settings);

	return data;
}

static const char *custom_service_protocol(void *data)
{
	struct custom_service *service = data;

	return service->protocol;
}

static const char *custom_service_connect_info(void *data, uint32_t type)
{
	struct custom_service *service = data;

	switch ((enum obs_service_connect_info)type) {
	case OBS_SERVICE_CONNECT_INFO_SERVER_URL:
		return service->server;
	case OBS_SERVICE_CONNECT_INFO_STREAM_ID:
		return service->stream_id;
	case OBS_SERVICE_CONNECT_INFO_USERNAME:
		return service->username;
	case OBS_SERVICE_CONNECT_INFO_PASSWORD:
		return service->password;
	case OBS_SERVICE_CONNECT_INFO_ENCRYPT_PASSPHRASE:
		return service->encrypt_passphrase;
	case OBS_SERVICE_CONNECT_INFO_BEARER_TOKEN:
		return service->bearer_token;
	}

	return NULL;
}

enum obs_service_audio_track_cap custom_service_audio_track_cap(void *data)
{
	struct custom_service *service = data;

	if (strcmp(service->protocol, "SRT") == 0 ||
	    strcmp(service->protocol, "RIST") == 0 ||
	    strcmp(service->protocol, "HLS") == 0)
		return OBS_SERVICE_AUDIO_MULTI_TRACK;

	return OBS_SERVICE_AUDIO_SINGLE_TRACK;
}

bool custom_service_can_try_to_connect(void *data)
{
	struct custom_service *service = data;

	if (!service->server)
		return false;

	/* Require username password combo being set if one of those is set. */
	if ((strcmp(service->protocol, "RTMP") == 0) ||
	    (strcmp(service->protocol, "RTMPS") == 0) ||
	    (strcmp(service->protocol, "RIST") == 0)) {
		if ((service->username && !service->password) ||
		    (!service->username && service->password))
			return false;
	}

	/* Require stream id/key */
	if ((strcmp(service->protocol, "RTMP") == 0) ||
	    (strcmp(service->protocol, "RTMPS") == 0) ||
	    (strcmp(service->protocol, "HLS") == 0) ||
	    (strcmp(service->protocol, "FTL") == 0)) {
		if (!service->stream_id)
			return false;
	}

	if (strcmp(service->protocol, "WHIP") == 0 && !service->bearer_token)
		return false;

	return true;
}

static void custom_service_apply_settings(void *data,
					  obs_data_t *video_settings,
					  obs_data_t *audio_settings)
{
	struct custom_service *service = data;
	if ((strcmp(service->protocol, "SRT") != 0) &&
	    (strcmp(service->protocol, "RIST") != 0) &&
	    (strcmp(service->protocol, "WHIP") != 0)) {
		return;
	}

	if (video_settings != NULL) {
		obs_data_set_bool(video_settings, "repeat_headers", true);

		if (strcmp(service->protocol, "WHIP") == 0) {
			obs_data_set_int(video_settings, "bf", 0);
			obs_data_set_string(video_settings, "rate_control",
					    "CBR");
			return;
		}
	}

	if (audio_settings != NULL)
		obs_data_set_bool(audio_settings, "set_to_ADTS", true);
}

bool update_protocol_cb(obs_properties_t *props, obs_property_t *prop,
			obs_data_t *settings)
{
	UNUSED_PARAMETER(prop);

	const char *protocol = obs_data_get_string(settings, "protocol");

#define GET_AND_HIDE(property)                                           \
	obs_property_t *property = obs_properties_get(props, #property); \
	obs_property_set_visible(property, false)

	/* Declare and hide all non-custom fields */
	GET_AND_HIDE(hls_warning);
	GET_AND_HIDE(stream_id);
	GET_AND_HIDE(username);
	GET_AND_HIDE(password);
	GET_AND_HIDE(encrypt_passphrase);
	GET_AND_HIDE(bearer_token);

	GET_AND_HIDE(third_party_field_manager);

#undef GET_AND_HIDE

	/* Restore stream_id original description */
	obs_property_set_description(
		stream_id, obs_module_text("CustomServices.StreamID.Optional"));

#define SHOW(property) obs_property_set_visible(property, true)
	/* Show needed properties for first-party protocol */
	if ((strcmp(protocol, "RTMP") == 0) ||
	    (strcmp(protocol, "RTMPS") == 0)) {
		SHOW(stream_id);
		obs_property_set_description(
			stream_id,
			obs_module_text("CustomServices.StreamID.Key"));
		SHOW(username);
		SHOW(password);
	} else if (strcmp(protocol, "FTL") == 0) {
		SHOW(stream_id);
		obs_property_set_description(
			stream_id,
			obs_module_text("CustomServices.StreamID.Key"));
	} else if (strcmp(protocol, "HLS") == 0) {
		SHOW(hls_warning);
		SHOW(stream_id);
		obs_property_set_description(
			stream_id,
			obs_module_text("CustomServices.StreamID.Key"));
	} else if (strcmp(protocol, "SRT") == 0) {
		SHOW(stream_id);
		SHOW(encrypt_passphrase);
	} else if (strcmp(protocol, "RIST") == 0) {
		SHOW(encrypt_passphrase);
		SHOW(username);
		SHOW(password);
	} else if (strcmp(protocol, "WHIP") == 0) {
		SHOW(bearer_token);
	}
#undef SHOW

	/* Clean superfluous settings */
#define ERASE_DATA_IF_HIDDEN(property)       \
	if (!obs_property_visible(property)) \
	obs_data_set_string(settings, #property, "")

	ERASE_DATA_IF_HIDDEN(stream_id);
	ERASE_DATA_IF_HIDDEN(username);
	ERASE_DATA_IF_HIDDEN(password);
	ERASE_DATA_IF_HIDDEN(encrypt_passphrase);
	ERASE_DATA_IF_HIDDEN(bearer_token);

#undef ERASE_DATA_IF_HIDDEN

	return true;
}

static obs_properties_t *custom_service_properties(void *data)
{
	UNUSED_PARAMETER(data);

	obs_properties_t *ppts = obs_properties_create();

	/* Add protocol list */
	obs_property_t *p = obs_properties_add_list(
		ppts, "protocol",
		obs_module_text("CustomServices.CustomService.Protocol"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	/* Add first-party protocol before third-party */
	if (obs_is_output_protocol_registered("RTMP"))
		obs_property_list_add_string(p, "RTMP", "RTMP");
	if (obs_is_output_protocol_registered("RTMPS"))
		obs_property_list_add_string(p, "RTMPS", "RTMPS");
	if (obs_is_output_protocol_registered("HLS"))
		obs_property_list_add_string(p, "HLS", "HLS");
	if (obs_is_output_protocol_registered("SRT"))
		obs_property_list_add_string(p, "SRT", "SRT");
	if (obs_is_output_protocol_registered("RIST"))
		obs_property_list_add_string(p, "RIST", "RIST");
	if (obs_is_output_protocol_registered("WHIP"))
		obs_property_list_add_string(p, "WHIP", "WHIP");

	char *string = NULL;
	bool add_ftl = false;
	for (size_t i = 0; obs_enum_output_protocols(i, &string); i++) {

		/* Skip FTL, it will be put at the end of the list */
		if (strcmp(string, "FTL") == 0) {
			add_ftl = true;
			continue;
		}

		bool already_listed = false;

		for (size_t i = 0; i < obs_property_list_item_count(p); i++)
			already_listed |=
				(strcmp(obs_property_list_item_string(p, i),
					string) == 0);

		if (!already_listed)
			obs_property_list_add_string(p, string, string);
	}

	/* FTL is deprecated, put it at the end of the list */
	if (add_ftl)
		obs_property_list_add_string(
			p, obs_module_text("CustomServices.CustomService.FTL"),
			"FTL");

	/* Add callback to the list property */
	obs_property_set_modified_callback(p, update_protocol_cb);

	/* Add warning about how HLS stream key works */
	p = obs_properties_add_text(
		ppts, "hls_warning",
		obs_module_text("CustomServices.HLS.Warning"), OBS_TEXT_INFO);
	obs_property_text_set_info_type(p, OBS_TEXT_INFO_WARNING);

	/* Add server field */
	obs_properties_add_text(ppts, "server",
				obs_module_text("CustomServices.Server"),
				OBS_TEXT_DEFAULT);

	/* Add connect info fields */
	obs_properties_add_text(ppts, "stream_id",
				obs_module_text("CustomServices.StreamID"),
				OBS_TEXT_PASSWORD);
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
	obs_properties_add_text(ppts, "bearer_token",
				obs_module_text("CustomServices.BearerToken"),
				OBS_TEXT_PASSWORD);

	return ppts;
}

const struct obs_service_info custom_service = {
	.id = "custom_service",
	.supported_protocols = "RTMP;RTMPS;SRT;RIST;WHIP",
	.get_name = custom_service_name,
	.create = custom_service_create,
	.destroy = custom_service_destroy,
	.update = custom_service_update,
	.get_defaults = custom_service_defaults,
	.get_protocol = custom_service_protocol,
	.get_properties = custom_service_properties,
	.get_connect_info = custom_service_connect_info,
	.can_try_to_connect = custom_service_can_try_to_connect,
	.get_audio_track_cap = custom_service_audio_track_cap,
	.apply_encoder_settings = custom_service_apply_settings,
};
