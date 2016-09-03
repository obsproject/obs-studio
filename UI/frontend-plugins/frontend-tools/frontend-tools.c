#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("frontend-tools", "en-US")

void InitSceneSwitcher();
void FreeSceneSwitcher();

bool obs_module_load(void)
{
	InitSceneSwitcher();
	return true;
}

void obs_module_unload(void)
{
	FreeSceneSwitcher();
}
