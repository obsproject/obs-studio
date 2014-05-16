#include <obs-module.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#endif

OBS_DECLARE_MODULE()

extern struct obs_output_info rtmp_output_info;
extern struct obs_output_info flv_output_info;

bool obs_module_load(uint32_t libobs_ver)
{
#ifdef _WIN32
	WSADATA wsad;
	WSAStartup(MAKEWORD(2, 2), &wsad);
#endif

	obs_register_output(&rtmp_output_info);
	obs_register_output(&flv_output_info);

	UNUSED_PARAMETER(libobs_ver);
	return true;
}

#ifdef _WIN32
void obs_module_unload(void)
{
	WSACleanup();
}
#endif
