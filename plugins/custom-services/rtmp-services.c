// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include <obs-module.h>

struct rtmp_service {
	char *server;
	char *stream_key;
	char *username;
	char *password;
};

static const char *rtmp_service_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return obs_module_text("CustomServices.CustomRTMP");
}

static const char *rtmp_service_protocol(void *data)
{
	UNUSED_PARAMETER(data);
	return "RTMP";
}

static const char *rtmps_service_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return obs_module_text("CustomServices.CustomRTMPS");
}

static const char *rtmps_service_protocol(void *data)
{
	UNUSED_PARAMETER(data);
	return "RTMPS";
}

static void rtmp_service_update(void *data, obs_data_t *settings)
{
	struct rtmp_service *service = data;

	bfree(service->server);
	bfree(service->stream_key);
	bfree(service->username);
	bfree(service->password);

#define GET_STRING_SETTINGS(name)                                             \
	service->name =                                                       \
		obs_data_has_user_value(settings, #name) &&                   \
				(strcmp(obs_data_get_string(settings, #name), \
					"") != 0)                             \
			? bstrdup(obs_data_get_string(settings, #name))       \
			: NULL

	GET_STRING_SETTINGS(server);

	GET_STRING_SETTINGS(stream_key);

	GET_STRING_SETTINGS(username);
	GET_STRING_SETTINGS(password);

#undef GET_STRING_SETTINGS
}

static void rtmp_service_destroy(void *data)
{
	struct rtmp_service *service = data;

	bfree(service->server);
	bfree(service->stream_key);
	bfree(service->username);
	bfree(service->password);
	bfree(service);
}

static void *rtmp_service_create(obs_data_t *settings, obs_service_t *service)
{
	UNUSED_PARAMETER(service);

	struct rtmp_service *data = bzalloc(sizeof(struct rtmp_service));
	rtmp_service_update(data, settings);

	return data;
}

static const char *rtmp_service_connect_info(void *data, uint32_t type)
{
	struct rtmp_service *service = data;

	switch ((enum obs_service_connect_info)type) {
	case OBS_SERVICE_CONNECT_INFO_SERVER_URL:
		return service->server;
	case OBS_SERVICE_CONNECT_INFO_STREAM_KEY:
		return service->stream_key;
	case OBS_SERVICE_CONNECT_INFO_USERNAME:
		return service->username;
	case OBS_SERVICE_CONNECT_INFO_PASSWORD:
		return service->password;
	default:
		return NULL;
	}
}

bool rtmp_service_can_try_to_connect(void *data)
{
	struct rtmp_service *service = data;

	if (!service->server)
		return false;

	/* Require username password combo being set if one of those is set. */
	if ((service->username && !service->password) ||
	    (!service->username && service->password))
		return false;

	/* Require stream key */
	if (!service->stream_key)
		return false;

	return true;
}

static obs_properties_t *rtmp_service_properties(void *data)
{
	UNUSED_PARAMETER(data);

	obs_properties_t *ppts = obs_properties_create();

	/* Add server field */
	obs_properties_add_text(ppts, "server",
				obs_module_text("CustomServices.Server"),
				OBS_TEXT_DEFAULT);

	/* Add connect info fields */
	obs_properties_add_text(ppts, "stream_key",
				obs_module_text("CustomServices.StreamID.Key"),
				OBS_TEXT_PASSWORD);
	obs_properties_add_text(
		ppts, "username",
		obs_module_text("CustomServices.Username.Optional"),
		OBS_TEXT_DEFAULT);
	obs_properties_add_text(
		ppts, "password",
		obs_module_text("CustomServices.Password.Optional"),
		OBS_TEXT_PASSWORD);

	return ppts;
}

const struct obs_service_info custom_rtmp = {
	.id = "custom_rtmp",
	.flags = OBS_SERVICE_INTERNAL,
	.supported_protocols = "RTMP",
	.get_name = rtmp_service_name,
	.create = rtmp_service_create,
	.destroy = rtmp_service_destroy,
	.update = rtmp_service_update,
	.get_protocol = rtmp_service_protocol,
	.get_properties = rtmp_service_properties,
	.get_connect_info = rtmp_service_connect_info,
	.can_try_to_connect = rtmp_service_can_try_to_connect,
};

const struct obs_service_info custom_rtmps = {
	.id = "custom_rtmps",
	.flags = OBS_SERVICE_INTERNAL,
	.supported_protocols = "RTMPS",
	.get_name = rtmps_service_name,
	.create = rtmp_service_create,
	.destroy = rtmp_service_destroy,
	.update = rtmp_service_update,
	.get_protocol = rtmps_service_protocol,
	.get_properties = rtmp_service_properties,
	.get_connect_info = rtmp_service_connect_info,
	.can_try_to_connect = rtmp_service_can_try_to_connect,
};
