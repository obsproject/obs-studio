#include <obs-module.h>

OBS_DECLARE_MODULE()

extern struct obs_source_info monitor_capture_info;

bool obs_module_load(uint32_t libobs_ver)
{
	obs_register_source(&monitor_capture_info);

	UNUSED_PARAMETER(libobs_ver);
	return true;
}
