/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

/*
 *   This is a set of helper functions to more easily render to textures
 * without having to duplicate too much code.
 */

#include <assert.h>
#include "graphics.h"

struct gs_texture_render {
	texture_t  target, prev_target;
	zstencil_t zs, prev_zs;

	int cx, cy;

	enum gs_color_format    format;
	enum gs_zstencil_format zsformat;

	bool rendered;
};

texrender_t texrender_create(enum gs_color_format format,
		enum gs_zstencil_format zsformat)
{
	struct gs_texture_render *texrender;
	texrender = bmalloc(sizeof(struct gs_texture_render));
	memset(texrender, 0, sizeof(struct gs_texture_render));

	texrender->format   = format;
	texrender->zsformat = zsformat;

	return texrender;
}

void texrender_destroy(texrender_t texrender)
{
	if (texrender) {
		texture_destroy(texrender->target);
		zstencil_destroy(texrender->zs);
		bfree(texrender);
	}
}

static bool texrender_resetbuffer(texrender_t texrender, int cx, int cy)
{
	texture_destroy(texrender->target);
	zstencil_destroy(texrender->zs);

	texrender->target = NULL;
	texrender->zs     = NULL;
	texrender->cx     = cx;
	texrender->cy     = cy;

	texrender->target = gs_create_texture(cx, cy, texrender->format,
			1, NULL, GS_RENDERTARGET);
	if (!texrender->target)
		return false;

	if (texrender->zsformat != GS_ZS_NONE) {
		texrender->zs = gs_create_zstencil(cx, cy, texrender->zsformat);
		if (!texrender->zs) {
			texture_destroy(texrender->target);
			texrender->target = NULL;

			return false;
		}
	}

	return true;
}

bool texrender_begin(texrender_t texrender, int cx, int cy)
{
	if (texrender->rendered)
		return false;

	if (cx == -1)
		cx = gs_getwidth();
	if (cy == -1)
		cy = gs_getheight();

	assert(cx && cy);
	if (!cx || !cy)
		return false;

	if (texrender->cx != cx || texrender->cy != cy)
		if (!texrender_resetbuffer(texrender, cx, cy))
			return false;

	gs_viewport_push();
	gs_projection_push();
	gs_matrix_push();
	gs_matrix_identity();

	texrender->prev_target = gs_getrendertarget();
	texrender->prev_zs     = gs_getzstenciltarget();
	gs_setrendertarget(texrender->target, texrender->zs);

	gs_setviewport(0, 0, texrender->cx, texrender->cy);

	return true;
}

void texrender_end(texrender_t texrender)
{
	gs_setrendertarget(texrender->prev_target, texrender->prev_zs);

	gs_matrix_pop();
	gs_projection_pop();
	gs_viewport_pop();

	texrender->rendered = true;
}

void texrender_reset(texrender_t texrender)
{
	texrender->rendered = false;
}

texture_t texrender_gettexture(texrender_t texrender)
{
	return texrender->target;
}
