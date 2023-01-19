#include <obs-module.h>
#include <util/dstr.h>

struct rtmp_custom {
	char *server, *key;
	bool use_auth;
	char *username, *password;
};

static const char *rtmp_custom_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("CustomStreamingServer");
}

static void rtmp_custom_update(void *data, obs_data_t *settings)
{
	struct rtmp_custom *service = data;

	bfree(service->server);
	bfree(service->key);
	bfree(service->username);
	bfree(service->password);

	service->server = bstrdup(obs_data_get_string(settings, "server"));
	service->key = bstrdup(obs_data_get_string(settings, "key"));
	service->use_auth = obs_data_get_bool(settings, "use_auth");
	service->username = bstrdup(obs_data_get_string(settings, "username"));
	service->password = bstrdup(obs_data_get_string(settings, "password"));
}

static void rtmp_custom_destroy(void *data)
{
	struct rtmp_custom *service = data;

	bfree(service->server);
	bfree(service->key);
	bfree(service->username);
	bfree(service->password);
	bfree(service);
}

static void *rtmp_custom_create(obs_data_t *settings, obs_service_t *service)
{
	struct rtmp_custom *data = bzalloc(sizeof(struct rtmp_custom));
	rtmp_custom_update(data, settings);

	UNUSED_PARAMETER(service);
	return data;
}

static bool use_auth_modified(obs_properties_t *ppts, obs_property_t *p,
			      obs_data_t *settings)
{
	bool use_auth = obs_data_get_bool(settings, "use_auth");
	p = obs_properties_get(ppts, "username");
	obs_property_set_visible(p, use_auth);
	p = obs_properties_get(ppts, "password");
	obs_property_set_visible(p, use_auth);
	return true;
}

static obs_properties_t *rtmp_custom_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *ppts = obs_properties_create();
	obs_property_t *p;

	obs_properties_add_text(ppts, "server", "URL", OBS_TEXT_DEFAULT);

	obs_properties_add_text(ppts, "key", obs_module_text("StreamKey"),
				OBS_TEXT_PASSWORD);

	p = obs_properties_add_bool(ppts, "use_auth",
				    obs_module_text("UseAuth"));
	obs_properties_add_text(ppts, "username", obs_module_text("Username"),
				OBS_TEXT_DEFAULT);
	obs_properties_add_text(ppts, "password", obs_module_text("Password"),
				OBS_TEXT_PASSWORD);
	obs_property_set_modified_callback(p, use_auth_modified);
	return ppts;
}

static const char *rtmp_custom_url(void *data)
{
	struct rtmp_custom *service = data;
	return service->server;
}

static const char *rtmp_custom_key(void *data)
{
	struct rtmp_custom *service = data;
	return service->key;
}

static const char *rtmp_custom_username(void *data)
{
	struct rtmp_custom *service = data;
	if (!service->use_auth)
		return NULL;
	return service->username;
}

static const char *rtmp_custom_password(void *data)
{
	struct rtmp_custom *service = data;
	if (!service->use_auth)
		return NULL;
	return service->password;
}

#define RTMPS_PREFIX "rtmps://"
#define FTL_PREFIX "ftl://"
#define SRT_PREFIX "srt://"
#define RIST_PREFIX "rist://"

static const char *rtmp_custom_get_protocol(void *data)
{
	struct rtmp_custom *service = data;

	if (strncmp(service->server, RTMPS_PREFIX, strlen(RTMPS_PREFIX)) == 0)
		return "RTMPS";

	if (strncmp(service->server, FTL_PREFIX, strlen(FTL_PREFIX)) == 0)
		return "FTL";

	if (strncmp(service->server, SRT_PREFIX, strlen(SRT_PREFIX)) == 0)
		return "SRT";

	if (strncmp(service->server, RIST_PREFIX, strlen(RIST_PREFIX)) == 0)
		return "RIST";

	return "RTMP";
}

#define RTMP_PROTOCOL "rtmp"

static void rtmp_custom_apply_settings(void *data, obs_data_t *video_settings,
				       obs_data_t *audio_settings)
{
	struct rtmp_custom *service = data;
	if (service->server != NULL && video_settings != NULL &&
	    strncmp(service->server, RTMP_PROTOCOL, strlen(RTMP_PROTOCOL)) !=
		    0) {
		obs_data_set_bool(video_settings, "repeat_headers", true);
		obs_data_set_bool(audio_settings, "set_to_ADTS", true);
	}
}

static const char *rtmp_custom_get_connect_info(void *data, uint32_t type)
{
	switch ((enum obs_service_connect_info)type) {
	case OBS_SERVICE_CONNECT_INFO_SERVER_URL:
		return rtmp_custom_url(data);
	case OBS_SERVICE_CONNECT_INFO_STREAM_ID:
		return rtmp_custom_key(data);
	case OBS_SERVICE_CONNECT_INFO_USERNAME:
		return rtmp_custom_username(data);
	case OBS_SERVICE_CONNECT_INFO_PASSWORD:
		return rtmp_custom_password(data);
	case OBS_SERVICE_CONNECT_INFO_ENCRYPT_PASSPHRASE: {
		const char *protocol = rtmp_custom_get_protocol(data);

		if ((strcmp(protocol, "SRT") == 0))
			return rtmp_custom_password(data);
		else if ((strcmp(protocol, "RIST") == 0))
			return rtmp_custom_key(data);

		break;
	}
	}

	return NULL;
}

struct obs_service_info rtmp_custom_service = {
	.id = "rtmp_custom",
	.get_name = rtmp_custom_name,
	.create = rtmp_custom_create,
	.destroy = rtmp_custom_destroy,
	.update = rtmp_custom_update,
	.get_properties = rtmp_custom_properties,
	.get_protocol = rtmp_custom_get_protocol,
	.get_url = rtmp_custom_url,
	.get_key = rtmp_custom_key,
	.get_connect_info = rtmp_custom_get_connect_info,
	.get_username = rtmp_custom_username,
	.get_password = rtmp_custom_password,
	.apply_encoder_settings = rtmp_custom_apply_settings,
};
