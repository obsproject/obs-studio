#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("win-capture", "en-US")

extern struct obs_source_info monitor_capture_info;
extern struct obs_source_info window_capture_info;

bool obs_module_load(void)
{
	obs_register_source(&monitor_capture_info);
	obs_register_source(&window_capture_info);
	return true;
}
