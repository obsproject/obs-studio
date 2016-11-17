#include <obs-module.h>
#include "frontend-tools-config.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("frontend-tools", "en-US")

#if defined(_WIN32) || defined(__APPLE__)
void InitSceneSwitcher();
void FreeSceneSwitcher();
#endif

#if defined(_WIN32) && BUILD_CAPTIONS
void InitCaptions();
void FreeCaptions();
#endif

void InitOutputTimer();
void FreeOutputTimer();

bool obs_module_load(void)
{
#if defined(_WIN32) || defined(__APPLE__)
	InitSceneSwitcher();
#endif
#if defined(_WIN32) && BUILD_CAPTIONS
	InitCaptions();
#endif
	InitOutputTimer();
	return true;
}

void obs_module_unload(void)
{
#if defined(_WIN32) || defined(__APPLE__)
	FreeSceneSwitcher();
#endif
#if defined(_WIN32) && BUILD_CAPTIONS
	FreeCaptions();
#endif
	FreeOutputTimer();
}
