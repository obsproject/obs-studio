#include <obs-module.h>

extern struct obs_service_info rtmp_common_service;
extern struct obs_service_info rtmp_custom_service;

bool obs_module_load(uint32_t libobs_ver)
{
	obs_register_service(&rtmp_common_service);
	obs_register_service(&rtmp_custom_service);

	UNUSED_PARAMETER(libobs_ver);
	return true;
}
