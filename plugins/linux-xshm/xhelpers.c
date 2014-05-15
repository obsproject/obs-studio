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
	XineramaScreenInfo *info;

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

xshm_t *xshm_attach(Display *dpy, Screen *screen,
	int_fast32_t w, int_fast32_t h)
{
	if (!dpy || !screen)
		return NULL;

	xshm_t *xshm = bzalloc(sizeof(xshm_t));

	xshm->dpy = dpy;
	xshm->image = XShmCreateImage(xshm->dpy, DefaultVisualOfScreen(screen),
		DefaultDepthOfScreen(screen), ZPixmap, NULL, &xshm->info,
		w, h);
	if (!xshm->image)
		goto fail;

	xshm->info.shmid = shmget(IPC_PRIVATE,
		xshm->image->bytes_per_line * xshm->image->height,
		IPC_CREAT | 0700);
	if (xshm->info.shmid < 0)
		goto fail;

	xshm->info.shmaddr
		= xshm->image->data
		= (char *) shmat(xshm->info.shmid, 0, 0);
	if (xshm->info.shmaddr == (char *) -1)
		goto fail;
	xshm->info.readOnly = false;

	if (!XShmAttach(xshm->dpy, &xshm->info))
		goto fail;

	xshm->attached = true;
	return xshm;
fail:
	xshm_detach(xshm);
	return NULL;
}

void xshm_detach(xshm_t *xshm)
{
	if (!xshm)
		return;

	if (xshm->attached)
		XShmDetach(xshm->dpy, &xshm->info);

	if (xshm->info.shmaddr != (char *) -1)
		shmdt(xshm->info.shmaddr);

	if (xshm->info.shmid != -1)
		shmctl(xshm->info.shmid, IPC_RMID, NULL);

	if (xshm->image)
		XDestroyImage(xshm->image);

	bfree(xshm);
}
