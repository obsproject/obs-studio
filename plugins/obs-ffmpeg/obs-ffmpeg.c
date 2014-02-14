#include <obs-module.h>

OBS_DECLARE_MODULE()

extern struct obs_output_info ffmpeg_output;

bool obs_module_load(uint32_t obs_version)
{
	obs_register_output(&ffmpeg_output);

	UNUSED_PARAMETER(obs_version);
	return true;
}
