#include <obs-module.h>

#include <util/windows/win-version.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("win-mf-camera", "en-US")

MODULE_EXPORT const char *obs_module_description(void)
{
	return "Windows Media Foundation video input sources";
}

void RegisterMediaFoundationInput();

bool obs_module_load(void)
{
	RegisterMediaFoundationInput();
	return true;
}
