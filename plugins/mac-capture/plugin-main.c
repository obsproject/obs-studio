#include <obs-module.h>

OBS_DECLARE_MODULE()

extern struct obs_source_info coreaudio_info;

bool obs_module_load(uint32_t libobs_version)
{
	obs_register_source(&coreaudio_info);

	UNUSED_PARAMETER(libobs_version);
	return true;
}
