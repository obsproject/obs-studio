#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("win-wasapi", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "Windows WASAPI audio input/output sources";
}

void RegisterWASAPIInput();
void RegisterWASAPIOutput();
void patchMediaCrash();

bool obs_module_load(void)
{
	RegisterWASAPIInput();
	RegisterWASAPIOutput();
	patchMediaCrash();
	return true;
}
