#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("win-dshow", "en-US")

extern void RegisterDShowSource();

bool obs_module_load(void)
{
	RegisterDShowSource();
	return true;
}
