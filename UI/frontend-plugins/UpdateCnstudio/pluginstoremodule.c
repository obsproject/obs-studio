#include <obs-module.h>
#include <curl/curl.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("UpdateCnOBS", "en-US")

#if defined(_WIN32) || defined(__APPLE__)
void InitPluginUpdateCn();
void FreePluginUpdateCn();
#endif
#if defined(_WIN32)
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpvReserved)
{
	
	if (dwReason == DLL_PROCESS_ATTACH)
		curl_global_init(CURL_GLOBAL_WIN32);
	if (dwReason == DLL_PROCESS_DETACH)
		curl_global_cleanup();
	return TRUE;
}
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
