#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-ffmpeg", "en-US")

extern struct obs_output_info  ffmpeg_output;
extern struct obs_encoder_info aac_encoder_info;

bool obs_module_load(void)
{
	obs_register_output(&ffmpeg_output);
	obs_register_encoder(&aac_encoder_info);
	return true;
}
