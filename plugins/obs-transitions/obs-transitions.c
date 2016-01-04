#include <obs-module.h>

OBS_DECLARE_MODULE()

OBS_MODULE_USE_DEFAULT_LOCALE("obs-transitions", "en-US")

extern struct obs_source_info fade_transition;

bool obs_module_load(void)
{
	obs_register_source(&fade_transition);
	return true;
}
