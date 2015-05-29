#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-ffmpeg", "en-US")

extern struct obs_source_info  ffmpeg_source;
extern struct obs_output_info  ffmpeg_output;
extern struct obs_output_info  ffmpeg_muxer;
extern struct obs_encoder_info aac_encoder_info;

void initialize_ffmpeg_source();

bool obs_module_load(void)
{
	initialize_ffmpeg_source();

	obs_register_source(&ffmpeg_source);
	obs_register_output(&ffmpeg_output);
	obs_register_output(&ffmpeg_muxer);
	obs_register_encoder(&aac_encoder_info);
	return true;
}
