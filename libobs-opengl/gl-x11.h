/******************************************************************************
    Copyright (C) 2014 by Zachary Lund <admin@computerquip.com>

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

#include "gl-subsystem.h"

struct gl_windowinfo *gl_x11_windowinfo_create(const struct gs_init_data *info);

void gl_x11_windowinfo_destroy(struct gl_windowinfo *info);

struct gl_platform *gl_x11_platform_create(gs_device_t *device,
					   uint32_t adapter);

void gl_x11_platform_destroy(struct gl_platform *plat);

bool gl_x11_platform_init_swapchain(struct gs_swap_chain *swap);

void gl_x11_platform_cleanup_swapchain(struct gs_swap_chain *swap);

void x11_device_enter_context(gs_device_t *device);

void x11_device_leave_context(gs_device_t *device);

void *x11_device_get_device_obj(gs_device_t *device);

void gl_x11_getclientsize(const struct gs_swap_chain *swap, uint32_t *width,
			  uint32_t *height);

void gl_x11_clear_context(gs_device_t *device);

void gl_x11_update(gs_device_t *device);

void x11_device_load_swapchain(gs_device_t *device, gs_swapchain_t *swap);

void x11_device_present(gs_device_t *device);
