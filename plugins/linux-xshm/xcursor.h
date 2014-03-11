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

#pragma once

#include <obs.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	Display *dpy;
	float pos_x;
	float pos_y;
	unsigned long last_serial;
	unsigned short int last_width;
	unsigned short int last_height;
	texture_t tex;
} xcursor_t;

/**
 * Initializes the xcursor object
 * 
 * This needs to be executed within a valid render context
 */
xcursor_t *xcursor_init(Display *dpy);

/**
 * Destroys the xcursor object
 */
void xcursor_destroy(xcursor_t *data);

/**
 * Update the cursor texture
 * 
 * This needs to be executed within a valid render context
 */
void xcursor_tick(xcursor_t *data);

/**
 * Draw the cursor
 * 
 * This needs to be executed within a valid render context
 */
void xcursor_render(xcursor_t *data);

#ifdef __cplusplus
}
#endif
