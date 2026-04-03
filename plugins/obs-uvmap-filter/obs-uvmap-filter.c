#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-uvmap-filter", "en-US")

MODULE_EXPORT const char *obs_module_description(void)
{
	return "UV Map remapping filter for virtual set / 3D set workflows";
}

extern struct obs_source_info uvmap_filter;

bool obs_module_load(void)
{
	obs_register_source(&uvmap_filter);
	return true;
}
