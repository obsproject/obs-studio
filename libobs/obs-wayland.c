/******************************************************************************
    Copyright (C) 2019 by Jason Francis <cycl0ps@tuta.io>

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
******************************************************************************/

#include "obs-internal.h"
#include "obs-wayland.h"

#include <wayland-client.h>

void obs_wayland_log_info(void)
{
	struct wl_display *display = obs_get_platform_display();
	if (display == NULL) {
		blog(LOG_INFO, "Unable to connect to Wayland server");
		return;
	}
	blog(LOG_INFO, "Connected to Wayland server");
}

/*
 * Placeholders here until wayland gets a global hotkey protocol.
 */

bool obs_wayland_hotkeys_platform_init(struct obs_core_hotkeys *hotkeys)
{
	UNUSED_PARAMETER(hotkeys);
	return true;
}

void obs_wayland_hotkeys_platform_free(struct obs_core_hotkeys *hotkeys)
{
	UNUSED_PARAMETER(hotkeys);
}

bool obs_wayland_hotkeys_platform_is_pressed(obs_hotkeys_platform_t *context,
					     obs_key_t key)
{
	UNUSED_PARAMETER(context);
	UNUSED_PARAMETER(key);
	return false;
}

void obs_wayland_key_to_str(obs_key_t key, struct dstr *dstr)
{
	UNUSED_PARAMETER(key);
	UNUSED_PARAMETER(dstr);
}

obs_key_t obs_wayland_key_from_virtual_key(int sym)
{
	UNUSED_PARAMETER(sym);
	return OBS_KEY_NONE;
}

int obs_wayland_key_to_virtual_key(obs_key_t key)
{
	UNUSED_PARAMETER(key);
	return 0;
}
