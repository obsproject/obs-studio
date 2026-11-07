/******************************************************************************
    Copyright (C) 2025-2026 pkv <pkv@obsproject.com>
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

#include <obs-frontend-api.h>
#include <obs-module.h>
#include <util/platform.h>

extern bool loadHost();
extern void unloadHost();
extern bool retrieveVst3List(obs_source_t *src);
extern void freeVst3List();
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
	if (obs_frontend_is_safe_mode_enabled()) {
		blog(LOG_INFO, "VST3 plugin disabled in safe mode");
		return false;
	}

	if (!loadHost()) {
		return false;
	}

	if (!retrieveVst3List(nullptr)) {
		blog(LOG_WARNING, "OBS-VST3: no VST3 found or scanner could not be started.");
	}

	register_vst3_source();
	blog(LOG_INFO, "OBS-VST3 filter loaded successfully (version %s)", PLUGIN_VERSION);
	return true;
}

void obs_module_unload()
{
	freeVst3List();
	unloadHost();
}
