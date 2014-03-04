#include <obs-module.h>

OBS_DECLARE_MODULE()

extern struct obs_source_info wasapiInput;
extern struct obs_source_info wasapiOutput;

bool obs_module_load(uint32_t libobs_ver)
{
	obs_register_source(&wasapiInput);
	obs_register_source(&wasapiOutput);
	return true;
}
