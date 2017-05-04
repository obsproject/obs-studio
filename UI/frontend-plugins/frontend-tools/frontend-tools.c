#include <obs-module.h>
#include "frontend-tools-config.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("frontend-tools", "en-US")

void InitSceneSwitcher();
void FreeSceneSwitcher();

#if defined(_WIN32) && BUILD_CAPTIONS
void InitCaptions();
void FreeCaptions();
#endif

void InitOutputTimer();
void FreeOutputTimer();

bool obs_module_load(void)
{
#if defined(_WIN32) && BUILD_CAPTIONS
	InitCaptions();
#endif
	InitSceneSwitcher();
	InitOutputTimer();
	return true;
}

void obs_module_unload(void)
{
#if defined(_WIN32) && BUILD_CAPTIONS
	FreeCaptions();
#endif
	FreeSceneSwitcher();
	FreeOutputTimer();
}
