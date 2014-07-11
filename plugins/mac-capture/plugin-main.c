#include <obs-module.h>

OBS_DECLARE_MODULE()

extern struct obs_source_info coreaudio_input_capture_info;
extern struct obs_source_info coreaudio_output_capture_info;
extern struct obs_source_info display_capture_info;

bool obs_module_load(uint32_t libobs_version)
{
	obs_register_source(&coreaudio_input_capture_info);
	obs_register_source(&coreaudio_output_capture_info);
	obs_register_source(&display_capture_info);

	UNUSED_PARAMETER(libobs_version);
	return true;
}
