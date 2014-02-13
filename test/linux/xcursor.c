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
#include <stdlib.h>
#include <stdio.h>
#include <X11/extensions/Xfixes.h>
#include "xcursor.h"

uint32_t *xcursor_pixels(XFixesCursorImage *xc) {
    int size = xc->width * xc->height;
    uint32_t *pixels = bmalloc(size * 4);
    
    // pixel data from XFixes is defined as unsigned long ...
    // TODO: check why everybody is making a fuss about this
    for (int i = 0; i < size; ++i)
        pixels[i] = (uint32_t) xc->pixels[i];
    
    return pixels;
}

void xcursor_create(xcursor_t *data, XFixesCursorImage *xc) {
    // get cursor pixel data
    uint32_t *pixels = xcursor_pixels(xc);
    
    // if the cursor has the same size as the last one we can simply update
    if (data->tex
     && data->last_height == xc->width
     && data->last_width == xc->height) {
        texture_setimage(data->tex, (void **) pixels, xc->width * 4, False);
    }
    else {
        if (data->tex)
            texture_destroy(data->tex);
        
        data->tex = gs_create_texture(
            xc->width, xc->height,
            GS_RGBA, 1,
            (const void **) &pixels,
            GS_DYNAMIC
        );
    }
    bfree(pixels);
    
    // set some data
    data->last_serial = xc->cursor_serial;
    data->last_width = xc->width;
    data->last_height = xc->height;
}

xcursor_t *xcursor_init(Display *dpy) {
    xcursor_t *data = bmalloc(sizeof(xcursor_t));
    memset(data, 0, sizeof(xcursor_t));
    
    data->dpy = dpy;
    
    // initialize texture so we don't crash
    xcursor_tick(data);
    
    return data;
}

void xcursor_destroy(xcursor_t *data) {
    if (data->tex)
        texture_destroy(data->tex);
    bfree(data);
}

void xcursor_tick(xcursor_t *data) {
    // get cursor data
    XFixesCursorImage *xc = XFixesGetCursorImage(data->dpy);
    
    // update cursor if necessary
    if (!data->tex || data->last_serial != xc->cursor_serial)
        xcursor_create(data, xc);
    
    // update cursor position
    data->pos_x = -1.0 * (xc->x - xc->xhot);
    data->pos_y = -1.0 * (xc->y - xc->yhot);
    
    XFree(xc);
}

void xcursor_render(xcursor_t *data) {
    // TODO: why do i need effects ?
    effect_t effect  = gs_geteffect();
    eparam_t diffuse = effect_getparambyname(effect, "diffuse");

    effect_settexture(effect, diffuse, data->tex);
    
    gs_matrix_push();
    
    // move cursor to the right position
    gs_matrix_translate3f(
        data->pos_x,
        data->pos_y,
        0
    );
    
    // blend cursor
    gs_enable_blending(True);
    gs_blendfunction(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);
    gs_draw_sprite(data->tex, 0, 0, 0);
    gs_enable_blending(False);
    
    gs_matrix_pop();
}