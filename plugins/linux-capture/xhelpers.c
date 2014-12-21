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
#include <sys/shm.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xinerama.h>

#include "xhelpers.h"

int_fast32_t xinerama_is_active(Display *dpy)
{
	int minor, major;
	if (!dpy)
		return 0;
	if (!XineramaQueryVersion(dpy, &minor, &major))
		return 0;
	if (!XineramaIsActive(dpy))
		return 0;
	return 1;
}

int_fast32_t xinerama_screen_count(Display *dpy)
{
	int screens;
	if (!dpy)
		return 0;
	XFree(XineramaQueryScreens(dpy, &screens));
	return screens;
}

int_fast32_t xinerama_screen_geo(Display *dpy, const int_fast32_t screen,
	int_fast32_t *x, int_fast32_t *y, int_fast32_t *w, int_fast32_t *h)
{
	int screens;
	XineramaScreenInfo *info = NULL;

	if (!dpy)
		goto fail;
	info = XineramaQueryScreens(dpy, &screens);
	if (screen < 0 || screen >= screens)
		goto fail;

	*x = info[screen].x_org;
	*y = info[screen].y_org;
	*w = info[screen].width;
	*h = info[screen].height;

	XFree(info);
	return 0;
fail:
	if (info)
		XFree(info);
	
	*x = *y = *w = *h = 0;
	return -1;
}

int_fast32_t x11_screen_geo(Display *dpy, const int_fast32_t screen,
	int_fast32_t *w, int_fast32_t *h)
{
	Screen *scr;

	if (!dpy || screen < 0 || screen >= XScreenCount(dpy))
		goto fail;

	scr = XScreenOfDisplay(dpy, screen);
	if (!scr)
		goto fail;

	*w = XWidthOfScreen(scr);
	*h = XHeightOfScreen(scr);

	return 0;
fail:
	*w = *h = 0;
	return -1;
}

xcb_shm_t* xshm_xcb_attach(xcb_connection_t *xcb, const int w, const int h)
{
	if (!xcb)
		return NULL;

	xcb_shm_t *shm = bzalloc(sizeof(xcb_shm_t));
	shm->xcb       = xcb;
	shm->seg       = xcb_generate_id(shm->xcb);

	shm->shmid = shmget(IPC_PRIVATE, w * h * 4, IPC_CREAT | 0777);
	if (shm->shmid == -1)
		goto fail;

	xcb_shm_attach(shm->xcb, shm->seg, shm->shmid, false);

	shm->data = shmat(shm->shmid, NULL, 0);

	return shm;
fail:
	xshm_xcb_detach(shm);
	return NULL;
}

void xshm_xcb_detach(xcb_shm_t *shm)
{
	if (!shm)
		return;

	xcb_shm_detach(shm->xcb, shm->seg);

	if ((char *) shm->data != (char *) -1)
		shmdt(shm->data);

	if (shm->shmid != -1)
		shmctl(shm->shmid, IPC_RMID, NULL);

	bfree(shm);
}

xcb_screen_t *xcb_get_screen(xcb_connection_t *xcb, int screen)
{
	xcb_screen_iterator_t iter;

	iter = xcb_setup_roots_iterator(xcb_get_setup(xcb));
	for (; iter.rem; --screen, xcb_screen_next(&iter)) {
		if (screen == 0)
			return iter.data;
	}

	return NULL;
}
