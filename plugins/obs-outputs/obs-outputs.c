#include <obs-module.h>

OBS_DECLARE_MODULE()

bool obs_module_load(uint32_t libobs_ver)
{
	UNUSED_PARAMETER(libobs_ver);
	return true;
}
