/******************************************************************************
    Copyright (C) 2022 by Georges Basile Stavracas Neto <georges.stavracas@gmail.com>

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
#include "obs-nix-platform.h"
#include "obs-nix-drm.h"

#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

struct obs_hotkeys_platform {
};

void obs_nix_drm_log_info(void) {}

static bool obs_nix_drm_hotkeys_platform_init(struct obs_core_hotkeys *hotkeys)
{
	return true;
}

static void obs_nix_drm_hotkeys_platform_free(struct obs_core_hotkeys *hotkeys)
{
	obs_hotkeys_platform_t *plat = hotkeys->platform_context;
	bfree(plat);
}

static bool
obs_nix_drm_hotkeys_platform_is_pressed(obs_hotkeys_platform_t *context,
					obs_key_t key)
{
	UNUSED_PARAMETER(context);
	UNUSED_PARAMETER(key);
	// This function is only used by the hotkey thread for capturing out of
	// focus hotkey triggers. Since wayland never delivers key events when out
	// of focus we leave this blank intentionally.
	return false;
}

static void obs_nix_drm_key_to_str(obs_key_t key, struct dstr *dstr) {}

static obs_key_t obs_nix_drm_key_from_virtual_key(int sym)
{
	return OBS_KEY_NONE;
}

static int obs_nix_drm_key_to_virtual_key(obs_key_t key)
{
	return 0;
}

static const struct obs_nix_hotkeys_vtable drm_hotkeys_vtable = {
	.init = obs_nix_drm_hotkeys_platform_init,
	.free = obs_nix_drm_hotkeys_platform_free,
	.is_pressed = obs_nix_drm_hotkeys_platform_is_pressed,
	.key_to_str = obs_nix_drm_key_to_str,
	.key_from_virtual_key = obs_nix_drm_key_from_virtual_key,
	.key_to_virtual_key = obs_nix_drm_key_to_virtual_key,

};

const struct obs_nix_hotkeys_vtable *obs_nix_drm_get_hotkeys_vtable(void)
{
	return &drm_hotkeys_vtable;
}
