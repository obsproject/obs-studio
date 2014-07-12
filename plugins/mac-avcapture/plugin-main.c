#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("mac-avcapture", "en-US")

extern struct obs_source_info av_capture_info;

bool obs_module_load(uint32_t libobs_version)
{
	UNUSED_PARAMETER(libobs_version);

	obs_register_source(&av_capture_info);

	return true;
}

void obs_module_unload(void)
{
	OBS_MODULE_FREE_DEFAULT_LOCALE();
}
