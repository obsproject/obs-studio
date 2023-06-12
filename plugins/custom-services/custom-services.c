// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("custom-services", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "OBS Custom Services";
}

extern struct obs_service_info custom_service;

extern struct obs_service_info custom_rtmp;
extern struct obs_service_info custom_rtmps;
extern struct obs_service_info custom_hls;
extern struct obs_service_info custom_srt;
extern struct obs_service_info custom_rist;
extern struct obs_service_info custom_whip;

bool obs_module_load(void)
{
	obs_register_service(&custom_service);

	obs_register_service(&custom_rtmp);
	obs_register_service(&custom_rtmps);
	obs_register_service(&custom_hls);
	obs_register_service(&custom_srt);
	obs_register_service(&custom_rist);
	obs_register_service(&custom_whip);

	return true;
}
