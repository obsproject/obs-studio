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

#pragma once

#include "obs-internal.h"
#include "obs-hotkey.h"

void obs_wayland_log_info(void);

bool obs_wayland_hotkeys_platform_init(struct obs_core_hotkeys *hotkeys);

void obs_wayland_hotkeys_platform_free(struct obs_core_hotkeys *hotkeys);

bool obs_wayland_hotkeys_platform_is_pressed(obs_hotkeys_platform_t *context,
					     obs_key_t key);

void obs_wayland_key_to_str(obs_key_t key, struct dstr *dstr);

obs_key_t obs_wayland_key_from_virtual_key(int sym);

int obs_wayland_key_to_virtual_key(obs_key_t key);
