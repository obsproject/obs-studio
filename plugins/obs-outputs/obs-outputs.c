#include <obs-module.h>

OBS_DECLARE_MODULE()

extern struct obs_output_info rtmp_output_info;

bool obs_module_load(uint32_t libobs_ver)
{
	obs_register_output(&rtmp_output_info);

	UNUSED_PARAMETER(libobs_ver);
	return true;
}
