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

extern bool is_screen_capture_available() WEAK_IMPORT_ATTRIBUTE;

bool obs_module_load(void)
{
	if (is_screen_capture_available()) {
		extern struct obs_source_info sck_video_capture_info;
		obs_register_source(&sck_video_capture_info);
		if (__builtin_available(macOS 13.0, *)) {
			display_capture_info.output_flags |=
				OBS_SOURCE_DEPRECATED;
			window_capture_info.output_flags |=
				OBS_SOURCE_DEPRECATED;
			coreaudio_output_capture_info.output_flags |=
				OBS_SOURCE_DEPRECATED;
			extern struct obs_source_info sck_audio_capture_info;
			obs_register_source(&sck_audio_capture_info);
		}
	}
	obs_register_source(&display_capture_info);
	obs_register_source(&window_capture_info);
	obs_register_source(&coreaudio_input_capture_info);
	obs_register_source(&coreaudio_output_capture_info);
	return true;
}
