#include <obs-module.h>

OBS_DECLARE_MODULE()

extern struct obs_encoder_info obs_x264_encoder;

bool obs_module_load(uint32_t libobs_ver)
{
	obs_register_encoder(&obs_x264_encoder);

	UNUSED_PARAMETER(libobs_ver);
	return true;
}
