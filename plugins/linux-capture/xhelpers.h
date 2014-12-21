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

#ifdef __cplusplus
extern "C" {
#endif

#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>
#include <xcb/shm.h>
#include <xcb/xproto.h>
#include <obs.h>

typedef struct {
	XShmSegmentInfo info;
	XImage *image;
	Display *dpy;
	bool attached;
} xshm_t;

typedef struct {
	xcb_connection_t *xcb;
	xcb_shm_seg_t    seg;
	int              shmid;
	uint8_t          *data;
} xcb_shm_t;

/**
 * Check for Xinerama extension
 *
 * @return > 0 if Xinerama is available and active
 */
int_fast32_t xinerama_is_active(Display *dpy);

/**
 * Get the number of Xinerama screens
 *
 * @return number of screens
 */
int_fast32_t xinerama_screen_count(Display *dpy);

/**
 * Get screen geometry for a Xinerama screen
 *
 * @note On error the passed coordinates/sizes will be set to 0.
 *
 * @param dpy X11 display
 * @param screen screen number to get geometry for
 * @param x x-coordinate of the screen
 * @param y y-coordinate of the screen
 * @param w width of the screen
 * @param h height of the screen
 *
 * @return < 0 on error
 */
int_fast32_t xinerama_screen_geo(Display *dpy, const int_fast32_t screen,
	int_fast32_t *x, int_fast32_t *y, int_fast32_t *w, int_fast32_t *h);

/**
 * Get screen geometry for a X11 screen
 *
 * @note On error the passed sizes will be set to 0.
 *
 * @param dpy X11 display
 * @param screen screen number to get geometry for
 * @param w width of the screen
 * @param h height of the screen
 *
 * @return < 0 on error
 */
int_fast32_t x11_screen_geo(Display *dpy, const int_fast32_t screen,
	int_fast32_t *w, int_fast32_t *h);

/**
 * Attach a shared memory segment to the X-Server
 *
 * @param dpy X11 Display
 * @param screen X11 Screen
 * @param w width for the shared memory segment
 * @param h height for the shared memory segment
 *
 * @return NULL on error
 */
xshm_t *xshm_attach(Display *dpy, Screen *screen,
	int_fast32_t w, int_fast32_t h);

/**
 * Detach a shared memory segment
 */
void xshm_detach(xshm_t *xshm);

/**
 * Attach a shared memory segment to the X-Server
 *
 * @param xcb xcb connection
 * @param w width of the captured screen
 * @param h height of the captured screen
 *
 * @return NULL on error
 */
xcb_shm_t *xshm_xcb_attach(xcb_connection_t *xcb, const int w, const int h);

/**
 * Detach a shared memory segment
 */
void xshm_xcb_detach(xcb_shm_t *shm);

/**
 * Get screen by id for a xcb connection
 *
 * @param xcb xcb connection
 * @param screen id of the screen
 * @return screen info structure
 */
xcb_screen_t *xcb_get_screen(xcb_connection_t *xcb, int screen);

#ifdef __cplusplus
}
#endif
