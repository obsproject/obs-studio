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

#include <obs-module.h>
#include <util/platform.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H
#include FT_BITMAP_H
#include <sys/stat.h>
#include "text-freetype2.h"
#include "obs-convenience.h"

void set_up_vertex_buffer(struct ft2_source *srcdata)
{
	FT_UInt glyph_index = 0;
	uint32_t x = 0, space_pos = 0, word_width = 0;
	size_t len;
	size_t vbuf_size;

	if (!srcdata->text)
		return;

	if (srcdata->custom_width >= 100)
		srcdata->cx = srcdata->custom_width;
	else
		srcdata->cx = get_ft2_text_width(srcdata->text, srcdata);
	srcdata->cy = srcdata->max_h;

	obs_enter_graphics();
	if (srcdata->vbuf != NULL) {
		gs_vertbuffer_t *tmpvbuf = srcdata->vbuf;
		srcdata->vbuf = NULL;
		gs_vertexbuffer_destroy(tmpvbuf);
	}

	if (*srcdata->text == 0) {
		obs_leave_graphics();
		return;
	}

	vbuf_size = 6;
	if (srcdata->outline_text)
		vbuf_size += 6;
	if (srcdata->drop_shadow)
		vbuf_size += 6;
	srcdata->n_vbuf = (uint32_t)wcslen(srcdata->text) * vbuf_size;
	srcdata->vbuf = create_uv_vbuffer(srcdata->n_vbuf, true);

	if (srcdata->custom_width > 100 && srcdata->word_wrap) {
		len = wcslen(srcdata->text);

		for (uint32_t i = 0; i <= len; i++) {
			if (i == len || srcdata->text[i] == L' ' ||
			    srcdata->text[i] == L'\n') {
				if (x + word_width > srcdata->custom_width) {
					if (space_pos != 0)
						srcdata->text[space_pos] =
							L'\n';
					x = 0;
				}
				if (i == len)
					continue;

				x += word_width;
				word_width = 0;
				if (srcdata->text[i] == L'\n')
					x = 0;
				if (srcdata->text[i] == L' ')
					space_pos = i;
			}

			glyph_index = FT_Get_Char_Index(srcdata->font_face,
							srcdata->text[i]);
			if (srcdata->cacheglyphs[glyph_index])
				word_width +=
					srcdata->cacheglyphs[glyph_index]->xadv;
		}
	}

	fill_vertex_buffer(srcdata);
	obs_leave_graphics();
}

void fill_vertex_buffer(struct ft2_source *srcdata)
{
	struct gs_vb_data *vdata = gs_vertexbuffer_get_data(srcdata->vbuf);
	if (vdata == NULL || !srcdata->text)
		return;

	struct vec2 *tvarray = (struct vec2 *)vdata->tvarray[0].array;
	uint32_t *col = (uint32_t *)vdata->colors;

	FT_UInt glyph_index = 0;

	uint32_t cur_glyph = 0;
	size_t len = wcslen(srcdata->text);

	uint32_t max_y = srcdata->max_h;

	for (size_t k = 0; k < 3; k++) {
		// k=0: shadow
		// k=1: outline
		// k=2: actual text
		if (k == 0 && !srcdata->drop_shadow)
			continue;
		if (k == 1 && !srcdata->outline_text)
			continue;

		uint32_t dx = 0;
		uint32_t dy = srcdata->max_h;

		for (size_t i = 0; i < len; i++) {
			if (srcdata->text[i] == L'\n') {
				dx = 0;
				dy += srcdata->max_h + 4;
				continue;
			}
			// Skip filthy dual byte Windows line breaks
			if (srcdata->text[i] == L'\r')
				continue;

			glyph_index = FT_Get_Char_Index(srcdata->font_face,
							srcdata->text[i]);
			if (srcdata->cacheglyphs[glyph_index] == NULL)
				continue;
			if (srcdata->outline_text &&
			    srcdata->cacheglyphs_outline[glyph_index] == NULL)
				continue;

			if (srcdata->custom_width >= 100) {
				if (dx + srcdata->cacheglyphs[glyph_index]->xadv >
				    srcdata->custom_width) {
					dx = 0;
					dy += srcdata->max_h + 4;
				}
			}

			struct glyph_info *src_glyph;
			uint32_t c0, c1;
			float x0, y0;
			switch (k) {
			case 0:
				if (srcdata->outline_text)
					src_glyph = srcdata->cacheglyphs_outline
							    [glyph_index];
				else
					src_glyph =
						srcdata->cacheglyphs[glyph_index];
				x0 = (float)dx + (float)src_glyph->xoff + 4.0f;
				y0 = (float)dy - (float)src_glyph->yoff + 4.0f;
				c0 = c1 = srcdata->color[3];
				break;
			case 1:
				src_glyph =
					srcdata->cacheglyphs_outline[glyph_index];
				x0 = (float)dx + (float)src_glyph->xoff;
				y0 = (float)dy - (float)src_glyph->yoff;
				c0 = c1 = srcdata->color[2];
				break;
			case 2:
				src_glyph = srcdata->cacheglyphs[glyph_index];
				x0 = (float)dx + (float)src_glyph->xoff;
				y0 = (float)dy - (float)src_glyph->yoff;
				c0 = srcdata->color[0];
				c1 = srcdata->color[1];
				break;
			}
			set_v3_rect(vdata->points + (cur_glyph * 6), x0, y0,
				    (float)src_glyph->w, (float)src_glyph->h);
			set_v2_uv(tvarray + (cur_glyph * 6), src_glyph->u,
				  src_glyph->v, src_glyph->u2, src_glyph->v2);
			set_rect_colors2(col + (cur_glyph * 6), c0, c1);
			src_glyph = srcdata->cacheglyphs[glyph_index];
			dx += src_glyph->xadv;
			if (dy - (float)src_glyph->yoff + src_glyph->h > max_y)
				max_y = dy - src_glyph->yoff + src_glyph->h;
			cur_glyph++;
		}
	}

	srcdata->cy = max_y;
}

void cache_standard_glyphs(struct ft2_source *srcdata)
{
	for (uint32_t i = 0; i < num_cache_slots; i++) {
		if (srcdata->cacheglyphs[i] != NULL) {
			bfree(srcdata->cacheglyphs[i]);
			srcdata->cacheglyphs[i] = NULL;
		}
	}

	for (uint32_t i = 0; i < num_cache_slots; i++) {
		if (srcdata->cacheglyphs_outline[i] != NULL) {
			bfree(srcdata->cacheglyphs_outline[i]);
			srcdata->cacheglyphs_outline[i] = NULL;
		}
	}

	srcdata->texbuf_x = 0;
	srcdata->texbuf_y = 0;
	srcdata->texbuf_max_h = 0;

	cache_glyphs(srcdata, L"abcdefghijklmnopqrstuvwxyz"
			      L"ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890"
			      L"!@#$%^&*()-_=+,<.>/?\\|[]{}`~ \'\"\0");
}

FT_Render_Mode get_render_mode(struct ft2_source *srcdata)
{
	return srcdata->antialiasing ? FT_RENDER_MODE_NORMAL
				     : FT_RENDER_MODE_MONO;
}

#define flt2pos_eps ((float)(0.5f / 128.f))
static inline FT_Pos flt2pos(float x)
{
	return (FT_Pos)(x * 128.f + 0.5f);
}

void load_glyph(struct ft2_source *srcdata, const FT_UInt glyph_index,
		const FT_Render_Mode render_mode)
{
	const FT_Int32 load_mode = render_mode == FT_RENDER_MODE_MONO
					   ? FT_LOAD_TARGET_MONO
					   : FT_LOAD_DEFAULT;
	FT_Load_Glyph(srcdata->font_face, glyph_index, load_mode);
}

struct glyph_info *init_glyph(FT_GlyphSlot slot, const uint32_t dx,
			      const uint32_t dy, const uint32_t g_w,
			      const uint32_t g_h)
{
	struct glyph_info *glyph = bzalloc(sizeof(struct glyph_info));
	glyph->u = (float)dx / (float)texbuf_w;
	glyph->u2 = (float)(dx + g_w) / (float)texbuf_w;
	glyph->v = (float)dy / (float)texbuf_h;
	glyph->v2 = (float)(dy + g_h) / (float)texbuf_h;
	glyph->w = g_w;
	glyph->h = g_h;
	glyph->yoff = slot->bitmap_top;
	glyph->xoff = slot->bitmap_left;
	glyph->xadv = slot->advance.x >> 6;

	return glyph;
}

uint8_t get_pixel_value(const unsigned char *buf_row,
			FT_Render_Mode render_mode, const uint32_t x)
{
	if (render_mode == FT_RENDER_MODE_NORMAL) {
		return buf_row[x];
	}

	const uint32_t byte_index = x / 8;
	const uint8_t bit_index = x % 8;
	const bool pixel_set = (buf_row[byte_index] >> (7 - bit_index)) & 1;
	return pixel_set ? 255 : 0;
}

void rasterize(struct ft2_source *srcdata, FT_GlyphSlot slot,
	       const FT_Render_Mode render_mode, const uint32_t dx,
	       const uint32_t dy)
{
	/**
	 * The pitch's absolute value is the number of bytes taken by one bitmap
	 * row, including padding.
	 *
	 * Source: https://www.freetype.org/freetype2/docs/reference/ft2-basic_types.html
	 */
	const int pitch = abs(slot->bitmap.pitch);

	for (uint32_t y = 0; y < slot->bitmap.rows; y++) {
		const uint32_t row_start = y * pitch;
		const uint32_t row = (dy + y) * texbuf_w;

		for (uint32_t x = 0; x < slot->bitmap.width; x++) {
			const uint32_t row_pixel_position = dx + x;
			const uint8_t pixel_value =
				get_pixel_value(&slot->bitmap.buffer[row_start],
						render_mode, x);
			srcdata->texbuf[row_pixel_position + row] = pixel_value;
		}
	}
}

static int32_t cache_glyphs_one(struct ft2_source *srcdata,
				wchar_t *cache_glyphs,
				struct glyph_info **cacheglyphs,
				bool is_outline)
{
	if (!srcdata->font_face || !cache_glyphs)
		return 0;

	FT_GlyphSlot slot = srcdata->font_face->glyph;

	uint32_t dx = srcdata->texbuf_x;
	uint32_t dy = srcdata->texbuf_y;
	uint32_t dh = srcdata->texbuf_max_h;

	int32_t cached_glyphs = 0;
	const size_t len = wcslen(cache_glyphs);

	const FT_Render_Mode render_mode = get_render_mode(srcdata);

	for (size_t i = 0; i < len; i++) {
		const FT_UInt glyph_index =
			FT_Get_Char_Index(srcdata->font_face, cache_glyphs[i]);

		if (cacheglyphs[glyph_index] != NULL) {
			continue;
		}

		load_glyph(srcdata, glyph_index, render_mode);

		bool is_outline_format = slot->format ==
					 FT_GLYPH_FORMAT_OUTLINE;
		float outline_size = srcdata->outline_size;
		if (is_outline && is_outline_format &&
		    outline_size > flt2pos_eps)
			FT_Outline_Embolden(&slot->outline,
					    flt2pos(outline_size));

		FT_Render_Glyph(slot, render_mode);

		if (is_outline && !is_outline_format &&
		    outline_size > flt2pos_eps)
			FT_Bitmap_Embolden(ft2_lib, &slot->bitmap,
					   flt2pos(outline_size),
					   flt2pos(outline_size));

		const uint32_t g_w = slot->bitmap.width;
		const uint32_t g_h = slot->bitmap.rows;

		if (!is_outline && srcdata->max_h < g_h) {
			srcdata->max_h = g_h;
		}

		if (dx + g_w >= texbuf_w) {
			dx = 0;
			dy += dh + 1;
			dh = g_h;
		} else if (dh < g_h)
			dh = g_h;

		if (dy + g_h >= texbuf_h) {
			blog(LOG_WARNING,
			     "Out of space trying to render glyphs");
			break;
		}

		struct glyph_info *src_glyph =
			init_glyph(slot, dx, dy, g_w, g_h);
		if (is_outline) {
			src_glyph->xoff -= outline_size;
			src_glyph->yoff -= is_outline_format ? outline_size
							     : -outline_size;
		}
		cacheglyphs[glyph_index] = src_glyph;

		rasterize(srcdata, slot, render_mode, dx, dy);

		dx += (g_w + 1);
		cached_glyphs++;
	}

	srcdata->texbuf_x = dx;
	srcdata->texbuf_y = dy;
	srcdata->texbuf_max_h = dh;

	return cached_glyphs;
}

void cache_glyphs(struct ft2_source *srcdata, wchar_t *text)
{
	int32_t cached_glyphs =
		cache_glyphs_one(srcdata, text, srcdata->cacheglyphs, false);
	if (srcdata->outline_text)
		cached_glyphs += cache_glyphs_one(
			srcdata, text, srcdata->cacheglyphs_outline, true);

	// TODO: If it becomes out of memory, need garbage collection. Especially bold outline consumes a lot of image space.

	if (cached_glyphs > 0) {

		obs_enter_graphics();

		if (srcdata->tex != NULL) {
			gs_texture_t *tmp_texture = srcdata->tex;
			srcdata->tex = NULL;
			gs_texture_destroy(tmp_texture);
		}

		srcdata->tex = gs_texture_create(
			texbuf_w, texbuf_h, GS_A8, 1,
			(const uint8_t **)&srcdata->texbuf, 0);

		obs_leave_graphics();
	}
}

time_t get_modified_timestamp(char *filename)
{
	struct stat stats;

	// stat is apparently terrifying and horrible, but we only call it once
	// every second at most.
	if (os_stat(filename, &stats) != 0)
		return -1;

	return stats.st_mtime;
}

static void remove_cr(wchar_t *source)
{
	int j = 0;
	for (int i = 0; source[i] != '\0'; ++i) {
		if (source[i] != L'\r') {
			source[j++] = source[i];
		}
	}
	source[j] = '\0';
}

void load_text_from_file(struct ft2_source *srcdata, const char *filename)
{
	FILE *tmp_file = NULL;
	uint32_t filesize = 0;
	char *tmp_read = NULL;
	uint16_t header = 0;
	size_t bytes_read;

	tmp_file = os_fopen(filename, "rb");
	if (tmp_file == NULL) {
		if (!srcdata->file_load_failed) {
			blog(LOG_WARNING, "Failed to open file %s", filename);
			srcdata->file_load_failed = true;
		}
		return;
	}
	fseek(tmp_file, 0, SEEK_END);
	filesize = (uint32_t)ftell(tmp_file);
	fseek(tmp_file, 0, SEEK_SET);
	bytes_read = fread(&header, 2, 1, tmp_file);

	if (bytes_read == 2 && header == 0xFEFF) {
		// File is already in UTF-16 format
		if (srcdata->text != NULL) {
			bfree(srcdata->text);
			srcdata->text = NULL;
		}
		srcdata->text = bzalloc(filesize);
		bytes_read = fread(srcdata->text, filesize - 2, 1, tmp_file);

		bfree(tmp_read);
		fclose(tmp_file);

		return;
	}

	fseek(tmp_file, 0, SEEK_SET);

	tmp_read = bzalloc(filesize + 1);
	bytes_read = fread(tmp_read, filesize, 1, tmp_file);
	fclose(tmp_file);

	if (srcdata->text != NULL) {
		bfree(srcdata->text);
		srcdata->text = NULL;
	}
	srcdata->text = bzalloc((strlen(tmp_read) + 1) * sizeof(wchar_t));
	os_utf8_to_wcs(tmp_read, strlen(tmp_read), srcdata->text,
		       (strlen(tmp_read) + 1));

	remove_cr(srcdata->text);
	bfree(tmp_read);
}

void read_from_end(struct ft2_source *srcdata, const char *filename)
{
	FILE *tmp_file = NULL;
	uint32_t filesize = 0, cur_pos = 0, log_lines = 0;
	char *tmp_read = NULL;
	uint16_t value = 0, line_breaks = 0;
	size_t bytes_read;
	char bvalue;

	bool utf16 = false;

	tmp_file = fopen(filename, "rb");
	if (tmp_file == NULL) {
		if (!srcdata->file_load_failed) {
			blog(LOG_WARNING, "Failed to open file %s", filename);
			srcdata->file_load_failed = true;
		}
		return;
	}
	bytes_read = fread(&value, 2, 1, tmp_file);

	if (bytes_read == 2 && value == 0xFEFF)
		utf16 = true;

	fseek(tmp_file, 0, SEEK_END);
	filesize = (uint32_t)ftell(tmp_file);
	cur_pos = filesize;
	log_lines = srcdata->log_lines;

	while (line_breaks <= log_lines && cur_pos != 0) {
		if (!utf16)
			cur_pos--;
		else
			cur_pos -= 2;
		fseek(tmp_file, cur_pos, SEEK_SET);

		if (!utf16) {
			bytes_read = fread(&bvalue, 1, 1, tmp_file);
			if (bytes_read == 1 && bvalue == '\n')
				line_breaks++;
		} else {
			bytes_read = fread(&value, 2, 1, tmp_file);
			if (bytes_read == 2 && value == L'\n')
				line_breaks++;
		}
	}

	if (cur_pos != 0)
		cur_pos += (utf16) ? 2 : 1;

	fseek(tmp_file, cur_pos, SEEK_SET);

	if (utf16) {
		if (srcdata->text != NULL) {
			bfree(srcdata->text);
			srcdata->text = NULL;
		}
		srcdata->text = bzalloc(filesize - cur_pos);
		bytes_read =
			fread(srcdata->text, (filesize - cur_pos), 1, tmp_file);

		remove_cr(srcdata->text);
		bfree(tmp_read);
		fclose(tmp_file);

		return;
	}

	tmp_read = bzalloc((filesize - cur_pos) + 1);
	bytes_read = fread(tmp_read, filesize - cur_pos, 1, tmp_file);
	fclose(tmp_file);

	if (srcdata->text != NULL) {
		bfree(srcdata->text);
		srcdata->text = NULL;
	}
	srcdata->text = bzalloc((strlen(tmp_read) + 1) * sizeof(wchar_t));
	os_utf8_to_wcs(tmp_read, strlen(tmp_read), srcdata->text,
		       (strlen(tmp_read) + 1));

	remove_cr(srcdata->text);
	bfree(tmp_read);
}

uint32_t get_ft2_text_width(wchar_t *text, struct ft2_source *srcdata)
{
	if (!text) {
		return 0;
	}

	FT_GlyphSlot slot = srcdata->font_face->glyph;
	uint32_t w = 0, max_w = 0;
	const size_t len = wcslen(text);
	for (size_t i = 0; i < len; i++) {
		const FT_UInt glyph_index =
			FT_Get_Char_Index(srcdata->font_face, text[i]);

		load_glyph(srcdata, glyph_index, get_render_mode(srcdata));

		if (text[i] == L'\n')
			w = 0;
		else {
			w += slot->advance.x >> 6;
			if (w > max_w)
				max_w = w;
		}
	}

	return max_w;
}
