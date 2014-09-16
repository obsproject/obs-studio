#include <obs-module.h>
#include "obs-ffmpeg-source.h"

static const char *ffmpeg_source_get_name(void)
{
	return obs_module_text("FFmpegSource");
}

static void ffmpeg_source_defaults(obs_data_t settings) {
	obs_data_set_default_string(settings, "filename", "/Users/redstone/Desktop/Test.mov");
    obs_data_set_default_string(settings, "format", "");
}

static obs_properties_t ffmpeg_source_properties(void)
{
	obs_properties_t props = obs_properties_create();
	
	obs_properties_add_text(props, "filename", "Filename", OBS_TEXT_DEFAULT);
    obs_properties_add_text(props, "format", "Format", OBS_TEXT_DEFAULT);
	
	return props;
}

struct obs_source_info ffmpeg_source = {
	.id = "ffmpeg_source",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_ASYNC_VIDEO,
	.get_name = ffmpeg_source_get_name,
	.create = ffmpeg_source_create,
	.destroy = ffmpeg_source_destroy,
	.get_defaults = ffmpeg_source_defaults,
	.get_properties = ffmpeg_source_properties,
	.update = ffmpeg_source_update,
};
