#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("syphon", "en-US")

extern struct obs_source_info syphon_info;

bool obs_module_load(void)
{
	obs_register_source(&syphon_info);
	return true;
}
