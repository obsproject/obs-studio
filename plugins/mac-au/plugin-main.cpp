/*****************************************************************************
Copyright (C) 2025 by pkv@obsproject.com

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/
#include <obs-module.h>
const char *PLUGIN_VERSION = "1.0.0";
OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("mac-au", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "macOS AudioUnit host";
}

extern void register_aufilter_source();
#include "mac-au-scan.h"

struct au_list g_au_list;

bool obs_module_load(void)
{
	g_au_list = au_scan_all_effects();

	blog(LOG_INFO, "[AU filter]: Found %d Audio Units :", g_au_list.count);
	if (!g_au_list.count) {
		blog(LOG_INFO, "[AU filter]: No Audio Units found; AU filter disabled");
		return false;
	}
	for (int i = 0; i < g_au_list.count; i++) {
		struct au_descriptor *d = &g_au_list.items[i];
		blog(LOG_INFO, "[AU filter]: %i. %s (%s, version: %s, %s)", i + 1, d->name, d->vendor, d->version,
		     d->is_v3 ? "AUv3" : "AUv2");
	}
	register_aufilter_source();
	blog(LOG_INFO, "MAC-AU filter loaded successfully (version %s)", PLUGIN_VERSION);
	return true;
}

void obs_module_unload(void)
{
	au_free_list(&g_au_list);
}
