#include <obs-module.h>

struct rtmp_custom {
	char *server, *key;
};

static const char *rtmp_custom_name(void)
{
	return obs_module_text("CustomStreamingServer");
}

static void rtmp_custom_update(void *data, obs_data_t settings)
{
	struct rtmp_custom *service = data;

	bfree(service->server);
	bfree(service->key);

	service->server = bstrdup(obs_data_get_string(settings, "server"));
	service->key    = bstrdup(obs_data_get_string(settings, "key"));
}

static void rtmp_custom_destroy(void *data)
{
	struct rtmp_custom *service = data;

	bfree(service->server);
	bfree(service->key);
	bfree(service);
}

static void *rtmp_custom_create(obs_data_t settings, obs_service_t service)
{
	struct rtmp_custom *data = bzalloc(sizeof(struct rtmp_custom));
	rtmp_custom_update(data, settings);

	UNUSED_PARAMETER(service);
	return data;
}

static obs_properties_t rtmp_custom_properties(void)
{
	obs_properties_t ppts = obs_properties_create();

	obs_properties_add_text(ppts, "server", "URL", OBS_TEXT_DEFAULT);

	obs_properties_add_text(ppts, "key", obs_module_text("StreamKey"),
			OBS_TEXT_PASSWORD);
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

struct obs_service_info rtmp_custom_service = {
	.id             = "rtmp_custom",
	.get_name       = rtmp_custom_name,
	.create         = rtmp_custom_create,
	.destroy        = rtmp_custom_destroy,
	.update         = rtmp_custom_update,
	.get_properties = rtmp_custom_properties,
	.get_url        = rtmp_custom_url,
	.get_key        = rtmp_custom_key
};
