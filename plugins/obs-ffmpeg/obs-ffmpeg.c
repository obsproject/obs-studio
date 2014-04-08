#include <obs-module.h>

OBS_DECLARE_MODULE()

extern struct obs_output_info  ffmpeg_output;
extern struct obs_encoder_info aac_encoder_info;

bool obs_module_load(uint32_t obs_version)
{
	obs_register_output(&ffmpeg_output);
	obs_register_encoder(&aac_encoder_info);

	UNUSED_PARAMETER(obs_version);
	return true;
}
