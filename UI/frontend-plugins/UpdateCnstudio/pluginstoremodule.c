#include <obs-module.h>
#include <curl/curl.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("UpdateCnOBS", "en-US")

#if defined(_WIN32) || defined(__APPLE__)
void InitPluginUpdateCn();
void FreePluginUpdateCn();
#endif

bool obs_module_load(void)
{
#if defined(_WIN32) || defined(__APPLE__)
	//curl_global_init(CURL_GLOBAL_ALL);
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
