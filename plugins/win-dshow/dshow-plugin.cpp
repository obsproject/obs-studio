#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("win-dshow", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "Windows DirectShow source/encoder";
}

extern void RegisterDShowSource();
extern void RegisterDShowEncoders();

bool obs_module_load(void)
{
	RegisterDShowSource();
	RegisterDShowEncoders();
	return true;
}
