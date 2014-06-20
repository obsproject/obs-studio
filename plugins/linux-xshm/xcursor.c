/*
Copyright (C) 2014 by Leonhard Oelke <leonhard@in-verted.de>

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
*/

#include <stdint.h>
#include <X11/extensions/Xfixes.h>

#include <util/bmem.h>
#include "xcursor.h"

/*
 * Get pixel data for the cursor
 *
 * XFixes has the data defined as unsigned long, so we can not use memcpy.
 * Theres a lot of talk about this in other implementation and they tend to
 * be really complicated, but this naive approach seems to work fine ...
 */
static uint32_t *xcursor_pixels(XFixesCursorImage *xc) {
	uint_fast32_t size = xc->width * xc->height;
	uint32_t *pixels = bmalloc(size * sizeof(uint32_t));

	for (uint_fast32_t i = 0; i < size; ++i)
		pixels[i] = (uint32_t) xc->pixels[i];

	return pixels;
}

/*
 * Create the cursor texture, either by updating if the new cursor has the same
 * size or by creating a new texture if the size is different
 */
static void xcursor_create(xcursor_t *data, XFixesCursorImage *xc) {
	uint32_t *pixels = xcursor_pixels(xc);

	if (data->tex
	&& data->last_height == xc->width
	&& data->last_width == xc->height) {
		texture_setimage(data->tex, (void **) pixels,
			xc->width * sizeof(uint32_t), False);
	} else {
		if (data->tex)
			texture_destroy(data->tex);

		data->tex = gs_create_texture(xc->width, xc->height,
			GS_BGRA, 1, (const void **) &pixels, GS_DYNAMIC);
	}

	bfree(pixels);

	data->last_serial = xc->cursor_serial;
	data->last_width = xc->width;
	data->last_height = xc->height;
}

xcursor_t *xcursor_init(Display *dpy) {
	xcursor_t *data = bzalloc(sizeof(xcursor_t));

	data->dpy = dpy;
	xcursor_tick(data);

	return data;
}

void xcursor_destroy(xcursor_t *data) {
	if (data->tex)
		texture_destroy(data->tex);
	bfree(data);
}

void xcursor_tick(xcursor_t *data) {
	XFixesCursorImage *xc = XFixesGetCursorImage(data->dpy);

	if (!data->tex || data->last_serial != xc->cursor_serial)
		xcursor_create(data, xc);
	data->pos_x = -1.0 * (xc->x - xc->xhot - data->x_org);
	data->pos_y = -1.0 * (xc->y - xc->yhot - data->y_org);

	XFree(xc);
}

void xcursor_render(xcursor_t *data) {
	/* TODO: why do i need effects ? */
	effect_t effect  = gs_geteffect();
	eparam_t image = effect_getparambyname(effect, "image");

	effect_settexture(effect, image, data->tex);

	gs_matrix_push();

	gs_matrix_translate3f(-data->pos_x, -data->pos_y, 0);

	gs_enable_blending(True);
	gs_blendfunction(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);
	gs_draw_sprite(data->tex, 0, 0, 0);
	gs_enable_blending(False);

	gs_matrix_pop();
}

void xcursor_offset(xcursor_t* data, int_fast32_t x_org, int_fast32_t y_org)
{
	data->x_org = x_org;
	data->y_org = y_org;
}

