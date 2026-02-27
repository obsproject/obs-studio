/******************************************************************************
    Copyright (C) 2025 pkv <pkv@obsproject.com>
    This file is part of obs-vst3.
    It uses the Steinberg VST3 SDK, which is licensed under MIT license.
    See https://github.com/steinbergmedia/vst3sdk for details.
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/
#include <obs-module.h>
#include <util/platform.h>
#ifdef __linux__
#include <obs-nix-platform.h>
#endif
#include <atomic>

extern void load_host();
extern void unload_host();
extern std::atomic<bool> vst3_scan_done;
extern bool retrieve_vst3_list();
extern void free_vst3_list();
extern void register_vst3_source();

const char *PLUGIN_VERSION = "1.0.0";
OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-vst3", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "VST3 audio plugin";
}

bool obs_module_load(void)
{
#if defined(__linux__)
	if (obs_get_nix_platform() != OBS_NIX_PLATFORM_X11_EGL) {
		blog(LOG_INFO, "OBS-VST3 filter disabled due to X11 requirement.");
		return false;
	}
#endif
	load_host();
	if (!retrieve_vst3_list())
		blog(LOG_INFO, "OBS-VST3: you'll have to install VST3s in orer to use this filter.");

	register_vst3_source();
	blog(LOG_INFO, "OBS-VST3 filter loaded successfully (version %s)", PLUGIN_VERSION);
	return true;
}

void obs_module_post_load(void)
{
	for (int i = 0; i < 50 && !vst3_scan_done; ++i)
		os_sleep_ms(100);
}

void obs_module_unload()
{
	free_vst3_list();
	unload_host();
}
