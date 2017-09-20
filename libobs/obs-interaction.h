/******************************************************************************
 Copyright (C) 2014 by Hugh Bailey <obs.jim@gmail.com>

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

#include "util/c99defs.h"

enum obs_interaction_flags {
	INTERACT_NONE          = 0,
	INTERACT_CAPS_KEY      = 1,
	INTERACT_SHIFT_KEY     = 1 << 1,
	INTERACT_CONTROL_KEY   = 1 << 2,
	INTERACT_ALT_KEY       = 1 << 3,
	INTERACT_MOUSE_LEFT    = 1 << 4,
	INTERACT_MOUSE_MIDDLE  = 1 << 5,
	INTERACT_MOUSE_RIGHT   = 1 << 6,
	INTERACT_COMMAND_KEY   = 1 << 7,
	INTERACT_NUMLOCK_KEY   = 1 << 8,
	INTERACT_IS_KEY_PAD    = 1 << 9,
	INTERACT_IS_LEFT       = 1 << 10,
	INTERACT_IS_RIGHT      = 1 << 11
};

enum obs_mouse_button_type {
	MOUSE_LEFT,
	MOUSE_MIDDLE,
	MOUSE_RIGHT
};

enum obs_navigation_key_flags {
	NAVKEY_NONE            = 0,
	NAVKEY_LEFT            = 1,
	NAVKEY_RIGHT           = 1 << 1,
	NAVKEY_UP              = 1 << 2,
	NAVKEY_DOWN            = 1 << 3,
	NAVKEY_START           = 1 << 4,
	NAVKEY_END             = 1 << 5,
	NAVKEY_NEXTPAGE        = 1 << 6,
	NAVKEY_PREVPAGE        = 1 << 7
};

struct obs_mouse_event {
	uint32_t            modifiers;
	int32_t             x;
	int32_t             y;
};

struct obs_key_event {
	uint32_t            modifiers;
	char                *text;
	uint32_t            navigation_keys;
	uint32_t            native_modifiers;
	uint32_t            native_scancode;
	uint32_t            native_vkey;
};
