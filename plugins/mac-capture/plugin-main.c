#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("mac-capture", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "macOS audio input/output and window/display capture";
}

extern struct obs_source_info coreaudio_input_capture_info;
extern struct obs_source_info coreaudio_output_capture_info;
extern struct obs_source_info display_capture_info;
extern struct obs_source_info window_capture_info;

bool obs_module_load(void)
{
	obs_register_source(&coreaudio_input_capture_info);
	obs_register_source(&coreaudio_output_capture_info);
	obs_register_source(&display_capture_info);
	obs_register_source(&window_capture_info);
	return true;
}
