#include <util/text-lookup.h>
#include <util/threading.h>
#include <util/platform.h>
#include <util/dstr.h>
#include <obs-module.h>


OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("zixi-services", "en-US")

#define ZIXI_SERVICES_LOG_STR "[zixi-services plugin] "
#define ZIXI_SERVICES_VER_STR "zixi-services plugin (libobs " OBS_VERSION ")"

extern struct obs_service_info zixi_service;
extern struct obs_output_info zixi_output;

bool obs_module_load(void)
{
	obs_register_service(&zixi_service);
	obs_register_output(&zixi_output);
	return true;
}

void obs_module_unload(void)
{
}