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

#ifndef GL_SUBSYSTEM_H
#define GL_SUBSYSTEM_H

#include "graphics/graphics.h"
#include "glew/include/GL/glew.h"

struct gl_platform;
struct gl_windowinfo;

struct gs_swap_chain {
	device_t device;
	struct gl_windowinfo *wi;
	struct gs_init_data  info;
};

struct gs_device {
	struct gl_platform *plat;

	struct gs_swap_chain *cur_swap;
};

extern struct gl_platform   *gl_platform_create(device_t device,
                                                struct gs_init_data *info);
extern struct gs_swap_chain *gl_platform_getswap(struct gl_platform *platform);
extern void                  gl_platform_destroy(struct gl_platform *platform);

extern struct gl_windowinfo *gl_windowinfo_create(struct gs_init_data *info);
extern void                  gl_windowinfo_destroy(struct gl_windowinfo *wi);

#endif
