#include <obs-module.h>


OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("UpdateCnOBS", "en-US")

#if defined(_WIN32) || defined(__APPLE__)
void InitPluginUpdateCn();
void FreePluginUpdateCn();
#endif

bool obs_module_load(void)
{
#if defined(_WIN32) || defined(__APPLE__)
	InitPluginUpdateCn();
	
#endif

 return true;
}

void obs_module_unload(void)
{
#if defined(_WIN32) || defined(__APPLE__)
	FreePluginUpdateCn();
#endif

}
