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

#ifdef __cplusplus
extern "C" {
#endif

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

struct gs_image_file2 {
	struct gs_image_file image;
	uint64_t mem_usage;
};

struct gs_image_file3 {
	struct gs_image_file2 image2;
	enum gs_image_alpha_mode alpha_mode;
};

typedef struct gs_image_file gs_image_file_t;
typedef struct gs_image_file2 gs_image_file2_t;
typedef struct gs_image_file3 gs_image_file3_t;

EXPORT void gs_image_file_init(gs_image_file_t *image, const char *file);
EXPORT void gs_image_file_free(gs_image_file_t *image);

EXPORT void gs_image_file_init_texture(gs_image_file_t *image);
EXPORT bool gs_image_file_tick(gs_image_file_t *image,
			       uint64_t elapsed_time_ns);
EXPORT void gs_image_file_update_texture(gs_image_file_t *image);

EXPORT void gs_image_file2_init(gs_image_file2_t *if2, const char *file);

EXPORT bool gs_image_file2_tick(gs_image_file2_t *if2,
				uint64_t elapsed_time_ns);
EXPORT void gs_image_file2_update_texture(gs_image_file2_t *if2);

EXPORT void gs_image_file3_init(gs_image_file3_t *if3, const char *file,
				enum gs_image_alpha_mode alpha_mode);

EXPORT bool gs_image_file3_tick(gs_image_file3_t *if3,
				uint64_t elapsed_time_ns);
EXPORT void gs_image_file3_update_texture(gs_image_file3_t *if3);

static void gs_image_file2_free(gs_image_file2_t *if2)
{
	gs_image_file_free(&if2->image);
	if2->mem_usage = 0;
}

static void gs_image_file2_init_texture(gs_image_file2_t *if2)
{
	gs_image_file_init_texture(&if2->image);
}

static void gs_image_file3_free(gs_image_file3_t *if3)
{
	gs_image_file2_free(&if3->image2);
}

static void gs_image_file3_init_texture(gs_image_file3_t *if3)
{
	gs_image_file2_init_texture(&if3->image2);
}

#ifdef __cplusplus
}
#endif
