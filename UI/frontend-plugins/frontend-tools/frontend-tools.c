#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("frontend-tools", "en-US")

#ifdef _WIN32
void InitSceneSwitcher();
void FreeSceneSwitcher();
void InitOutputTimer();
void FreeOutputTimer();

bool obs_module_load(void)
{
	InitSceneSwitcher();
	InitOutputTimer();
	return true;
}

void obs_module_unload(void)
{
	FreeSceneSwitcher;
	FreeOutputTimer();
}

#elif _APPLE_
void InitSceneSwitcher();
void FreeSceneSwitcher();
void InitOutputTimer();
void FreeOutputTimer();

bool obs_module_load(void)
{
	InitSceneSwitcher();
	InitOutputTimer();
	return true;
}

void obs_module_unload(void)
{
	FreeSceneSwitcher;
	FreeOutputTimer();
}

#else
void InitOutputTimer();
void FreeOutputTimer();

bool obs_module_load(void)
{
	InitOutputTimer();
	return true;
}

void obs_module_unload(void)
{
	FreeOutputTimer();
}

#endif
