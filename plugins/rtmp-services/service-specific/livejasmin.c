#include <obs-module.h>
#include <util/dstr.h>

#include "livejasmin.h"

struct livejasmin_service {
	char *server;
	char *key;
};

static const char *livejasmin_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "LiveJasmin";
}

static void livejasmin_update(void *data, obs_data_t *settings)
{
	struct livejasmin_service *service = data;

	bfree(service->server);
	bfree(service->key);

	service->server = bstrdup(obs_data_get_string(settings, "server"));
	service->key = bstrdup(obs_data_get_string(settings, "key"));
}

static void livejasmin_destroy(void *data)
{
	struct livejasmin_service *service = data;

	bfree(service->server);
	bfree(service->key);
	bfree(service);
}

static void *livejasmin_create(obs_data_t *settings, obs_service_t *service)
{
	struct livejasmin_service *data = bzalloc(sizeof(struct livejasmin_service));
	livejasmin_update(data, settings);

	UNUSED_PARAMETER(service);
	return data;
}

static obs_properties_t *livejasmin_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *ppts = obs_properties_create();

	obs_properties_add_text(ppts, "server", obs_module_text("Server"), OBS_TEXT_DEFAULT);
	obs_properties_add_text(ppts, "key", obs_module_text("StreamKey"), OBS_TEXT_PASSWORD);

	return ppts;
}

static void livejasmin_defaults(obs_data_t *settings)
{
	obs_data_set_default_string(settings, "server", "");
}

static const char *livejasmin_url(void *data)
{
	struct livejasmin_service *service = data;
	return service->server;
}

static const char *livejasmin_key(void *data)
{
	struct livejasmin_service *service = data;
	return service->key;
}

static const char *livejasmin_get_protocol(void *data)
{
	struct livejasmin_service *service = data;

	if (service->server && strncmp(service->server, "rtmps://", 8) == 0)
		return "RTMPS";

	return "RTMP";
}

struct livejasmin_bitrate_entry {
	int cx, cy, fps, max_bitrate;
};

static const struct livejasmin_bitrate_entry bitrate_matrix[] = {
	{1920, 1080, 30, 7500},
	{1280, 720, 30, 3000},
};

static int livejasmin_get_bitrate_matrix_max(void)
{
	struct obs_video_info ovi;
	if (!obs_get_video_info(&ovi))
		return 0;

	double cur_fps = (double)ovi.fps_num / (double)ovi.fps_den;

	for (size_t i = 0; i < sizeof(bitrate_matrix) / sizeof(bitrate_matrix[0]); i++) {
		const struct livejasmin_bitrate_entry *e = &bitrate_matrix[i];
		if ((int)ovi.output_width == e->cx && (int)ovi.output_height == e->cy &&
		    cur_fps <= (double)e->fps + 0.0000001)
			return e->max_bitrate;
	}

	return 0;
}

static void livejasmin_apply_settings(void *data, obs_data_t *video_settings, obs_data_t *audio_settings)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(audio_settings);

	if (!video_settings)
		return;

	/* LiveJasmin recommended settings: low latency */
	obs_data_set_int(video_settings, "bf", 0);
	obs_data_set_int(video_settings, "keyint_sec", 1);

	/* Set encoder profile */
	obs_data_item_t *item = obs_data_item_byname(video_settings, "profile");
	if (obs_data_item_gettype(item) == OBS_DATA_STRING)
		obs_data_set_string(video_settings, "profile", "high");
	obs_data_item_release(&item);

	/* Cap bitrate to matrix max */
	int max_bitrate = livejasmin_get_bitrate_matrix_max();
	if (!max_bitrate)
		max_bitrate = 7500;

	if (obs_data_get_int(video_settings, "bitrate") > max_bitrate) {
		obs_data_set_int(video_settings, "bitrate", max_bitrate);
		obs_data_set_int(video_settings, "buffer_size", max_bitrate);
	}
}

static const char *livejasmin_get_connect_info(void *data, uint32_t type)
{
	switch ((enum obs_service_connect_info)type) {
	case OBS_SERVICE_CONNECT_INFO_SERVER_URL:
		return livejasmin_url(data);
	case OBS_SERVICE_CONNECT_INFO_STREAM_ID:
		return livejasmin_key(data);
	default:
		return NULL;
	}
}

static bool livejasmin_can_try_to_connect(void *data)
{
	struct livejasmin_service *service = data;
	return (service->server != NULL && service->server[0] != '\0');
}

static const char *livejasmin_get_output_type(void *data)
{
	UNUSED_PARAMETER(data);
	return "rtmp_output";
}

static const char **livejasmin_get_supported_video_codecs(void *data)
{
	static const char *codecs[] = {"h264", NULL};
	UNUSED_PARAMETER(data);
	return codecs;
}

static const char **livejasmin_get_supported_audio_codecs(void *data)
{
	static const char *codecs[] = {"aac", NULL};
	UNUSED_PARAMETER(data);
	return codecs;
}

static void livejasmin_get_max_fps(void *data, int *fps)
{
	UNUSED_PARAMETER(data);
	if (fps)
		*fps = 30;
}

static void livejasmin_get_max_bitrate(void *data, int *video_bitrate, int *audio_bitrate)
{
	UNUSED_PARAMETER(data);
	if (video_bitrate) {
		int max = livejasmin_get_bitrate_matrix_max();
		*video_bitrate = max ? max : 7500;
	}
	if (audio_bitrate)
		*audio_bitrate = 160;
}

static void livejasmin_get_supported_resolutions(void *data, struct obs_service_resolution **resolutions, size_t *count)
{
	UNUSED_PARAMETER(data);

	static const struct obs_service_resolution res[] = {
		{1920, 1080},
		{1280, 720},
	};

	*count = sizeof(res) / sizeof(res[0]);
	*resolutions = bmemdup(res, sizeof(res));
}

struct obs_service_info livejasmin_service = {
	.id = "livejasmin",
	.get_name = livejasmin_name,
	.create = livejasmin_create,
	.destroy = livejasmin_destroy,
	.update = livejasmin_update,
	.get_defaults = livejasmin_defaults,
	.get_properties = livejasmin_properties,
	.get_protocol = livejasmin_get_protocol,
	.get_url = livejasmin_url,
	.get_key = livejasmin_key,
	.get_connect_info = livejasmin_get_connect_info,
	.apply_encoder_settings = livejasmin_apply_settings,
	.get_supported_resolutions = livejasmin_get_supported_resolutions,
	.get_max_fps = livejasmin_get_max_fps,
	.get_max_bitrate = livejasmin_get_max_bitrate,
	.get_supported_video_codecs = livejasmin_get_supported_video_codecs,
	.get_supported_audio_codecs = livejasmin_get_supported_audio_codecs,
	.can_try_to_connect = livejasmin_can_try_to_connect,
	.get_output_type = livejasmin_get_output_type,
};

void init_livejasmin_data(void)
{
	/* Nothing to initialize */
}

void unload_livejasmin_data(void)
{
	/* Nothing to cleanup */
}
