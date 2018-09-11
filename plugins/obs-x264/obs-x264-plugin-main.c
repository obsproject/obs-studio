#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-x264", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "x264 based encoder";
}

extern struct obs_encoder_info obs_x264_encoder;

bool obs_module_load(void)
{
	obs_register_encoder(&obs_x264_encoder);
	return true;
}
