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

#include "gl-nix.h"
#include "gl-x11-egl.h"

#ifdef ENABLE_WAYLAND
#include "gl-wayland-egl.h"
#endif

static const struct gl_winsys_vtable *gl_vtable = NULL;

static void init_winsys(void)
{
	assert(gl_vtable == NULL);

	enum obs_nix_platform_type platform = obs_get_nix_platform();

	if (platform == OBS_NIX_PLATFORM_X11_EGL)
		gl_vtable = gl_x11_egl_get_winsys_vtable();

#ifdef ENABLE_WAYLAND
	if (platform == OBS_NIX_PLATFORM_WAYLAND) {
		gl_vtable = gl_wayland_egl_get_winsys_vtable();
		blog(LOG_INFO, "Using EGL/Wayland");
	}
#endif

	assert(gl_vtable != NULL);
}

extern struct gl_windowinfo *gl_windowinfo_create(const struct gs_init_data *info)
{
	return gl_vtable->windowinfo_create(info);
}

extern void gl_windowinfo_destroy(struct gl_windowinfo *info)
{
	gl_vtable->windowinfo_destroy(info);
}

extern struct gl_platform *gl_platform_create(gs_device_t *device, uint32_t adapter)
{
	init_winsys();

	return gl_vtable->platform_create(device, adapter);
}

extern void gl_platform_destroy(struct gl_platform *plat)
{
	gl_vtable->platform_destroy(plat);

	gl_vtable = NULL;
}

extern bool gl_platform_init_swapchain(struct gs_swap_chain *swap)
{
	return gl_vtable->platform_init_swapchain(swap);
}

extern void gl_platform_cleanup_swapchain(struct gs_swap_chain *swap)
{
	gl_vtable->platform_cleanup_swapchain(swap);
}

extern void device_enter_context(gs_device_t *device)
{
	gl_vtable->device_enter_context(device);
}

extern void device_leave_context(gs_device_t *device)
{
	device->cur_render_target = NULL;
	device->cur_zstencil_buffer = NULL;
	device->cur_vertex_buffer = NULL;
	device->cur_index_buffer = NULL;
	device->cur_swap = NULL;
	device->cur_fbo = NULL;
	gl_vtable->device_leave_context(device);
}

extern bool device_enum_adapters(gs_device_t *device, bool (*callback)(void *param, const char *name, uint32_t id),
				 void *param)
{
	return gl_vtable->device_enum_adapters(device, callback, param);
}

extern void *device_get_device_obj(gs_device_t *device)
{
	return gl_vtable->device_get_device_obj(device);
}

extern void gl_getclientsize(const struct gs_swap_chain *swap, uint32_t *width, uint32_t *height)
{
	gl_vtable->getclientsize(swap, width, height);
}

extern void gl_clear_context(gs_device_t *device)
{
	gl_vtable->clear_context(device);
}

extern void gl_update(gs_device_t *device)
{
	gl_vtable->update(device);
}

extern void device_load_swapchain(gs_device_t *device, gs_swapchain_t *swap)
{
	gl_vtable->device_load_swapchain(device, swap);
}

extern bool device_is_present_ready(gs_device_t *device)
{
	UNUSED_PARAMETER(device);
	return true;
}

extern void device_present(gs_device_t *device)
{
	gl_vtable->device_present(device);
}

extern bool device_is_monitor_hdr(gs_device_t *device, void *monitor)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(monitor);

	return false;
}

extern struct gs_texture *device_texture_create_from_dmabuf(gs_device_t *device, unsigned int width,
							    unsigned int height, uint32_t drm_format,
							    enum gs_color_format color_format, uint32_t n_planes,
							    const int *fds, const uint32_t *strides,
							    const uint32_t *offsets, const uint64_t *modifiers)
{
	return gl_vtable->device_texture_create_from_dmabuf(device, width, height, drm_format, color_format, n_planes,
							    fds, strides, offsets, modifiers);
}

extern bool device_query_dmabuf_capabilities(gs_device_t *device, enum gs_dmabuf_flags *dmabuf_flags,
					     uint32_t **drm_formats, size_t *n_formats)
{
	return gl_vtable->device_query_dmabuf_capabilities(device, dmabuf_flags, drm_formats, n_formats);
}

extern bool device_query_dmabuf_modifiers_for_format(gs_device_t *device, uint32_t drm_format, uint64_t **modifiers,
						     size_t *n_modifiers)
{
	return gl_vtable->device_query_dmabuf_modifiers_for_format(device, drm_format, modifiers, n_modifiers);
}

struct gs_texture *device_texture_create_from_pixmap(gs_device_t *device, uint32_t width, uint32_t height,
						     enum gs_color_format color_format, uint32_t target, void *pixmap)
{
	return gl_vtable->device_texture_create_from_pixmap(device, width, height, color_format, target, pixmap);
}

bool device_query_sync_capabilities(gs_device_t *device)
{
	return gl_vtable->device_query_sync_capabilities(device);
}

gs_sync_t *device_sync_create(gs_device_t *device)
{
	return gl_vtable->device_sync_create(device);
}

gs_sync_t *device_sync_create_from_syncobj_timeline_point(gs_device_t *device, int syncobj_fd, uint64_t timeline_point)
{
	return gl_vtable->device_sync_create_from_syncobj_timeline_point(device, syncobj_fd, timeline_point);
}

void device_sync_destroy(gs_device_t *device, gs_sync_t *sync)
{
	return gl_vtable->device_sync_destroy(device, sync);
}

bool device_sync_export_syncobj_timeline_point(gs_device_t *device, gs_sync_t *sync, int syncobj_fd,
					       uint64_t timeline_point)
{
	return gl_vtable->device_sync_export_syncobj_timeline_point(device, sync, syncobj_fd, timeline_point);
}

bool device_sync_signal_syncobj_timeline_point(gs_device_t *device, int syncobj_fd, uint64_t timeline_point)
{
	return gl_vtable->device_sync_signal_syncobj_timeline_point(device, syncobj_fd, timeline_point);
}

bool device_sync_wait(gs_device_t *device, gs_sync_t *sync)
{
	return gl_vtable->device_sync_wait(device, sync);
}
