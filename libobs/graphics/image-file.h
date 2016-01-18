/******************************************************************************
    Copyright (C) 2016 by Hugh Bailey <obs.jim@gmail.com>

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

#include "graphics.h"
#include "libnsgif/libnsgif.h"

struct gs_image_file {
	gs_texture_t *texture;
	enum gs_color_format format;
	uint32_t cx;
	uint32_t cy;
	bool is_animated_gif;
	bool frame_updated;
	bool loaded;

	gif_animation gif;
	uint8_t *gif_data;
	uint8_t **animation_frame_cache;
	uint8_t *animation_frame_data;
	uint64_t cur_time;
	int cur_frame;
	int cur_loop;
	int last_decoded_frame;

	uint8_t *texture_data;
	gif_bitmap_callback_vt bitmap_callbacks;
};

typedef struct gs_image_file gs_image_file_t;

EXPORT void gs_image_file_init(gs_image_file_t *image, const char *file);
EXPORT void gs_image_file_free(gs_image_file_t *image);

EXPORT void gs_image_file_init_texture(gs_image_file_t *image);
EXPORT bool gs_image_file_tick(gs_image_file_t *image,
		uint64_t elapsed_time_ns);
EXPORT void gs_image_file_update_texture(gs_image_file_t *image);
