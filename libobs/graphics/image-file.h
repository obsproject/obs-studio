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

#include "graphics.h"

#ifdef __cplusplus
extern "C" {
#endif

struct gs_image_file_ex {
	gs_texture_t *texture;
	enum gs_color_format format;
	uint32_t cx;
	uint32_t cy;
	bool is_animated_gif;
	bool frame_updated;
	bool loaded;

	struct gs_image_file_internal *internal;

	uint64_t cur_time;
	int cur_frame;
	int cur_loop;

	uint8_t *texture_data;
	uint64_t mem_usage;
	enum gs_image_alpha_mode alpha_mode;
	enum gs_color_space space;
};

typedef struct gs_image_file_ex gs_image_file_ex_t;

EXPORT void gs_image_file_ex_init(gs_image_file_ex_t *image, const char *file, enum gs_image_alpha_mode alpha_mode);
EXPORT void gs_image_file_ex_free(gs_image_file_ex_t *image);

EXPORT void gs_image_file_ex_init_texture(gs_image_file_ex_t *image);
EXPORT bool gs_image_file_ex_tick(gs_image_file_ex_t *image, uint64_t elapsed_time_ns);
EXPORT void gs_image_file_ex_update_texture(gs_image_file_ex_t *image);

#ifdef __cplusplus
}
#endif
