#include <obs-module.h>

//#include <iostream>
//using namespace std;


OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("pluginstore", "en-US")


#if defined(_WIN32) || defined(__APPLE__)
void InitPluginStore();
void FreePluginStore();
void InitOpenGLContexts();
#endif

#if defined(_WIN32)
#include <windows.h>

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpvReserved)
{

    if (dwReason == DLL_PROCESS_ATTACH)
        InitOpenGLContexts();
    if (dwReason == DLL_PROCESS_DETACH)
        return TRUE;
}
#endif // defined(_WIN32)


bool obs_module_load(void)
{
#if defined(_WIN32) || defined(__APPLE__)
	InitPluginStore();
#endif

 return true;
}

void obs_module_unload(void)
{
#if defined(_WIN32) || defined(__APPLE__)
	FreePluginStore();
#endif

}
