#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("win-wasapi", "en-US")

void RegisterWASAPIInput();
void RegisterWASAPIOutput();

bool obs_module_load(uint32_t libobs_ver)
{
	UNUSED_PARAMETER(libobs_ver);

	RegisterWASAPIInput();
	RegisterWASAPIOutput();
	return true;
}

void obs_module_unload(void)
{
	OBS_MODULE_FREE_DEFAULT_LOCALE();
}
