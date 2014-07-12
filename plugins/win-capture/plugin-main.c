#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("win-capture", "en-US")

extern struct obs_source_info monitor_capture_info;
extern struct obs_source_info window_capture_info;

bool obs_module_load(uint32_t libobs_ver)
{
	obs_register_source(&monitor_capture_info);
	obs_register_source(&window_capture_info);

	UNUSED_PARAMETER(libobs_ver);
	return true;
}

void obs_module_unload(void)
{
	OBS_MODULE_FREE_DEFAULT_LOCALE();
}
