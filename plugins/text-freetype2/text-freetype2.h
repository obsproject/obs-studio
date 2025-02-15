/******************************************************************************
Copyright (C) 2014 by Nibbles

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

#include <obs-module.h>
#include <ft2build.h>

#define num_cache_slots 65535
#define src_glyph srcdata->cacheglyphs[glyph_index]

struct glyph_info {
	float u, v, u2, v2;
	int32_t w, h, xoff, yoff;
	FT_Pos xadv;
};

struct ft2_source {
	char *font_name;
	char *font_style;
	uint16_t font_size;
	uint32_t font_flags;

	bool file_load_failed;
	bool from_file;
	bool antialiasing;
	char *text_file;
	wchar_t *text;
	time_t m_timestamp;
	bool update_file;
	uint64_t last_checked;

	uint32_t cx, cy, max_h, custom_width;
	uint32_t outline_width;
	uint32_t texbuf_x, texbuf_y;
	uint32_t color[2];

	int32_t cur_scroll, scroll_speed;

	gs_texture_t *tex;

	struct glyph_info *cacheglyphs[num_cache_slots];

	FT_Face font_face;

	uint8_t *texbuf;
	gs_vertbuffer_t *vbuf;

	gs_effect_t *draw_effect;
	bool outline_text, drop_shadow;
	bool log_mode, word_wrap;
	uint32_t log_lines;

	obs_source_t *src;
};

extern FT_Library ft2_lib;

void draw_outlines(struct ft2_source *srcdata);
void draw_drop_shadow(struct ft2_source *srcdata);

uint32_t get_ft2_text_width(wchar_t *text, struct ft2_source *srcdata);

time_t get_modified_timestamp(char *filename);
void load_text_from_file(struct ft2_source *srcdata, const char *filename);
void read_from_end(struct ft2_source *srcdata, const char *filename);

void cache_standard_glyphs(struct ft2_source *srcdata);
void cache_glyphs(struct ft2_source *srcdata, wchar_t *cache_glyphs);

void set_up_vertex_buffer(struct ft2_source *srcdata);
void fill_vertex_buffer(struct ft2_source *srcdata);
