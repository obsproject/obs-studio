/******************************************************************************
 Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

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
	INTERACT_NONE = 0,
	INTERACT_CAPS_KEY = 1,
	INTERACT_SHIFT_KEY = 1 << 1,
	INTERACT_CONTROL_KEY = 1 << 2,
	INTERACT_ALT_KEY = 1 << 3,
	INTERACT_MOUSE_LEFT = 1 << 4,
	INTERACT_MOUSE_MIDDLE = 1 << 5,
	INTERACT_MOUSE_RIGHT = 1 << 6,
	INTERACT_COMMAND_KEY = 1 << 7,
	INTERACT_NUMLOCK_KEY = 1 << 8,
	INTERACT_IS_KEY_PAD = 1 << 9,
	INTERACT_IS_LEFT = 1 << 10,
	INTERACT_IS_RIGHT = 1 << 11,
};

enum obs_mouse_button_type {
	MOUSE_LEFT,
	MOUSE_MIDDLE,
	MOUSE_RIGHT,
};

struct obs_mouse_event {
	uint32_t modifiers;
	int32_t x;
	int32_t y;
};

struct obs_key_event {
	uint32_t modifiers;
	char *text;
	uint32_t native_modifiers;
	uint32_t native_scancode;
	uint32_t native_vkey;
};

/* IME ranges use UTF-16 code units, independently of the UTF-8 text encoding. */
enum obs_ime_event_type {
	OBS_IME_COMPOSITION = 1,
	OBS_IME_COMMIT,
	OBS_IME_CANCEL,
};

struct obs_ime_underline {
	uint32_t start;
	uint32_t end;
	uint32_t color;            /* ARGB */
	uint32_t background_color; /* ARGB */
	bool thick;
};

struct obs_ime_event {
	enum obs_ime_event_type type;
	uint64_t generation; /* Snapshot from obs_source_get_ime_generation. */
	const char *text;    /* UTF-8, borrowed for the duration of the callback. */
	const struct obs_ime_underline *underlines;
	size_t underline_count;
	uint32_t selection_start;
	uint32_t selection_end;
};

struct obs_ime_rect {
	int32_t x;
	int32_t y;
	int32_t width;
	int32_t height;
};
