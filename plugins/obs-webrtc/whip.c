#include "obs-module.h"

struct whip {
	char *server, *key;
};

static const char *whip_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("WhipWebRTCStreaming");
}

static void whip_update(void *data, obs_data_t *settings)
{
	struct whip *service = data;

	bfree(service->server);
	bfree(service->key);

	service->server = bstrdup(obs_data_get_string(settings, "server"));
	service->key = bstrdup(obs_data_get_string(settings, "key"));
}

static void whip_destroy(void *data)
{
	struct whip *service = data;

	bfree(service->server);
	bfree(service->key);
	bfree(service);
}

static void *whip_create(obs_data_t *settings, obs_service_t *service)
{
	struct whip *data = bzalloc(sizeof(struct whip));
	whip_update(data, settings);

	UNUSED_PARAMETER(service);
	return data;
}

static const char *whip_url(void *data)
{
	struct whip *service = data;
	return service->server;
}

static const char *whip_key(void *data)
{
	struct whip *service = data;
	return service->key;
}

static obs_properties_t *whip_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *ppts = obs_properties_create();

	obs_properties_add_text(ppts, "server", "URL", OBS_TEXT_DEFAULT);
	obs_properties_add_text(ppts, "key", obs_module_text("StreamKey"),
				OBS_TEXT_PASSWORD);
	return ppts;
}

static const char *whip_get_output_type(void *data)
{
	struct whip *service = data;
	return "webrtc_output";
}

struct obs_service_info whip_service = {
	.id = "whip",
	.get_name = whip_name,
	.create = whip_create,
	.destroy = whip_destroy,
	.update = whip_update,
	.get_properties = whip_properties,
	.get_url = whip_url,
	.get_key = whip_key,
	.get_output_type = whip_get_output_type,
	//.apply_encoder_settings = rtmp_custom_apply_settings,
};