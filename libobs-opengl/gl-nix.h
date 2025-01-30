/******************************************************************************
    Copyright (C) 2019 by Jason Francis <cycl0ps@tuta.io>

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

#include <obs.h>
#include <obs-nix-platform.h>

#include "gl-subsystem.h"

struct gl_winsys_vtable {
	struct gl_windowinfo *(*windowinfo_create)(const struct gs_init_data *info);
	void (*windowinfo_destroy)(struct gl_windowinfo *info);

	struct gl_platform *(*platform_create)(gs_device_t *device, uint32_t adapter);

	void (*platform_destroy)(struct gl_platform *plat);

	bool (*platform_init_swapchain)(struct gs_swap_chain *swap);

	void (*platform_cleanup_swapchain)(struct gs_swap_chain *swap);

	void (*device_enter_context)(gs_device_t *device);

	void (*device_leave_context)(gs_device_t *device);

	void *(*device_get_device_obj)(gs_device_t *device);

	void (*getclientsize)(const struct gs_swap_chain *swap, uint32_t *width, uint32_t *height);

	void (*clear_context)(gs_device_t *device);

	void (*update)(gs_device_t *device);

	void (*device_load_swapchain)(gs_device_t *device, gs_swapchain_t *swap);

	void (*device_present)(gs_device_t *device);

	struct gs_texture *(*device_texture_create_from_dmabuf)(gs_device_t *device, unsigned int width,
								unsigned int height, uint32_t drm_format,
								enum gs_color_format color_format, uint32_t n_planes,
								const int *fds, const uint32_t *strides,
								const uint32_t *offsets, const uint64_t *modifiers);

	bool (*device_query_dmabuf_capabilities)(gs_device_t *device, enum gs_dmabuf_flags *dmabuf_flags,
						 uint32_t **drm_formats, size_t *n_formats);

	bool (*device_query_dmabuf_modifiers_for_format)(gs_device_t *device, uint32_t drm_format, uint64_t **modifiers,
							 size_t *n_modifiers);

	struct gs_texture *(*device_texture_create_from_pixmap)(gs_device_t *device, uint32_t width, uint32_t height,
								enum gs_color_format color_format, uint32_t target,
								void *pixmap);
	bool (*device_enum_adapters)(gs_device_t *device, bool (*callback)(void *param, const char *name, uint32_t id),
				     void *param);

	bool (*device_query_sync_capabilities)(gs_device_t *device);

	gs_sync_t *(*device_sync_create)(gs_device_t *device);

	gs_sync_t *(*device_sync_create_from_syncobj_timeline_point)(gs_device_t *device, int syncobj_fd,
								     uint64_t timeline_point);

	void (*device_sync_destroy)(gs_device_t *device, gs_sync_t *sync);

	bool (*device_sync_export_syncobj_timeline_point)(gs_device_t *device, gs_sync_t *sync, int syncobj_fd,
							  uint64_t timeline_point);

	bool (*device_sync_signal_syncobj_timeline_point)(gs_device_t *device, int syncobj_fd, uint64_t timeline_point);

	bool (*device_sync_wait)(gs_device_t *device, gs_sync_t *sync);
};
