/* mf-plugin.cpp */
#include <obs-module.h>
#include <strsafe.h>
#include <strmif.h>
#include "plugin-macros.generated.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

extern void RegisterMediaFoundationSource();

bool obs_module_load(void)
{
	RegisterMediaFoundationSource();
	obs_data_t *obs_settings = obs_data_create();
	obs_apply_private_data(obs_settings);
	obs_data_release(obs_settings);
	return true;
}

void obs_module_unload()
{
	blog(LOG_INFO, "plugin unloaded");
}
