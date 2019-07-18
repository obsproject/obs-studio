#include <obs-module.h>

OBS_DECLARE_MODULE()

OBS_MODULE_USE_DEFAULT_LOCALE("obs-track-out", "en-US")

struct track_out_data {
	obs_source_t *source;
};

const char *track_out_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return obs_module_text("Track");
}

void *track_out_create(obs_data_t *settings, obs_source_t *source)
{
	UNUSED_PARAMETER(settings);
	struct track_out_data *data = bzalloc(sizeof(struct track_out_data));
	data->source = source;
	return data;
}

void track_out_destroy(void *data)
{
	struct track_out_data *src = (struct track_out_data *)data;
	bfree(src);
	src = NULL;
}

struct obs_source_info track_out = {.id = "obs_track_out",
				    .type = OBS_SOURCE_TYPE_INPUT,
				    .output_flags = OBS_SOURCE_AUDIO |
						    OBS_SOURCE_TRACK,
				    .get_name = track_out_name,
				    .create = track_out_create,
				    .destroy = track_out_destroy,
				    .update = NULL,
				    .get_properties = NULL};

bool obs_module_load(void)
{
	obs_register_source(&track_out);
	return true;
}
