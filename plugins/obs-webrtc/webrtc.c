#include <obs-module.h>
#include "./webrtc/bindings.h"
#include "webrtc-call.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-webrtc", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "OBS webrtc module";
}

extern struct obs_output_info webrtc_output_info;
extern struct obs_service_info whip_service;
extern struct obs_source_info webrtc_source_info;

struct gs_texture *webrtcTexture;

bool obs_module_load(void)
{
	obs_register_output(&webrtc_output_info);
	obs_register_service(&whip_service);
	obs_register_source(&webrtc_source_info);
	return true;
}

void obs_module_post_load(void)
{
	setup_call();
	start_call();
}

void obs_module_unload(void) {}
