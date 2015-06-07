#include <obs-module.h>

extern void RegisterMFAACEncoder();

bool obs_module_load(void)
{
	RegisterMFAACEncoder();
	return true;
}

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("win-mf", "en-US")
