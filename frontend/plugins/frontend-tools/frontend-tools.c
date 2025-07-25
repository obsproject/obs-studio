#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("frontend-tools", "en-US")

void InitSceneSwitcher();
void FreeSceneSwitcher();

#if defined(_WIN32)
void InitCaptions();
void FreeCaptions();
#endif

void initOutputTimer();
void freeOutputTimer();

#if defined(ENABLE_SCRIPTING)
void InitScripts();
void FreeScripts();
#endif

bool obs_module_load(void)
{
#if defined(_WIN32)
	InitCaptions();
#endif
	InitSceneSwitcher();
	initOutputTimer();
#if defined(ENABLE_SCRIPTING)
	InitScripts();
#endif
	return true;
}

void obs_module_unload(void)
{
#if defined(_WIN32)
	FreeCaptions();
#endif
	FreeSceneSwitcher();
	freeOutputTimer();
#if defined(ENABLE_SCRIPTING)
	FreeScripts();
#endif
}
