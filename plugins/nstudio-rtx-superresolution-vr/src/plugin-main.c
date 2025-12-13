/*
obs-rtx_superresolution
Copyright (C) 2023 Ben Jolley

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-properties.h>
#include <obs-module.h>
#include <plugin-support.h>


OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

extern bool load_nv_superresolution_filter(void);

bool obs_module_load(void)
{
	bool can_load_nv_vfx_sdk = load_nv_superresolution_filter();

	if (can_load_nv_vfx_sdk)
		obs_log(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);
	else
		obs_log(LOG_INFO, "error loading, did you install the nvidia vfx sdk? (version %s)", PLUGIN_VERSION);

	return can_load_nv_vfx_sdk;
}

void obs_module_unload(void)
{
	obs_log(LOG_INFO, "plugin unloaded");
}
