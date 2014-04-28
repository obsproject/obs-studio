#include <obs-module.h>

OBS_DECLARE_MODULE()

extern struct obs_source_info av_capture_info;

bool obs_module_load(uint32_t libobs_version)
{
	UNUSED_PARAMETER(libobs_version);

	obs_register_source(&av_capture_info);

	return true;
}
