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

#include "obs-internal.h"
#include "gl-subsystem.h"

#ifdef ENABLE_X11
#include "gl-x11.h"
#endif
#ifdef ENABLE_WAYLAND
#include "gl-wayland.h"
#endif

extern struct gl_windowinfo *
gl_windowinfo_create(const struct gs_init_data *info)
{
#ifdef ENABLE_X11
	if (obs_get_platform() == OBS_PLATFORM_DEFAULT) {
		return gl_x11_windowinfo_create(info);
	}
#endif
#ifdef ENABLE_WAYLAND
	if (obs_get_platform() == OBS_PLATFORM_WAYLAND) {
		return gl_wayland_windowinfo_create(info);
	}
#endif
	return NULL;
}

extern void gl_windowinfo_destroy(struct gl_windowinfo *info)
{
#ifdef ENABLE_X11
	if (obs_get_platform() == OBS_PLATFORM_DEFAULT) {
		gl_x11_windowinfo_destroy(info);
	}
#endif
#ifdef ENABLE_WAYLAND
	if (obs_get_platform() == OBS_PLATFORM_WAYLAND) {
		gl_wayland_windowinfo_destroy(info);
	}
#endif
}

static struct {
	struct gl_platform *(*platform_create)(gs_device_t *device,
					       uint32_t adapter);

	void (*platform_destroy)(struct gl_platform *plat);

	bool (*platform_init_swapchain)(struct gs_swap_chain *swap);

	void (*platform_cleanup_swapchain)(struct gs_swap_chain *swap);

	void (*device_enter_context)(gs_device_t *device);

	void (*device_leave_context)(gs_device_t *device);

	void *(*device_get_device_obj)(gs_device_t *device);

	void (*getclientsize)(const struct gs_swap_chain *swap, uint32_t *width,
			      uint32_t *height);

	void (*update)(gs_device_t *device);

	void (*device_load_swapchain)(gs_device_t *device,
				      gs_swapchain_t *swap);

	void (*device_present)(gs_device_t *device);
} gl_callbacks;

extern struct gl_platform *gl_platform_create(gs_device_t *device,
					      uint32_t adapter)
{
#ifdef ENABLE_X11
	if (obs_get_platform() == OBS_PLATFORM_DEFAULT) {
		gl_callbacks.platform_create = gl_x11_platform_create;
		gl_callbacks.platform_destroy = gl_x11_platform_destroy;
		gl_callbacks.platform_init_swapchain =
			gl_x11_platform_init_swapchain;
		gl_callbacks.platform_cleanup_swapchain =
			gl_x11_platform_cleanup_swapchain;
		gl_callbacks.device_enter_context = x11_device_enter_context;
		gl_callbacks.device_leave_context = x11_device_leave_context;
		gl_callbacks.device_get_device_obj = x11_device_get_device_obj;
		gl_callbacks.getclientsize = gl_x11_getclientsize;
		gl_callbacks.update = gl_x11_update;
		gl_callbacks.device_load_swapchain = x11_device_load_swapchain;
		gl_callbacks.device_present = x11_device_present;
	}
#endif
#ifdef ENABLE_WAYLAND
	if (obs_get_platform() == OBS_PLATFORM_WAYLAND) {
		gl_callbacks.platform_create = gl_wayland_platform_create;
		gl_callbacks.platform_destroy = gl_wayland_platform_destroy;
		gl_callbacks.platform_init_swapchain =
			gl_wayland_platform_init_swapchain;
		gl_callbacks.platform_cleanup_swapchain =
			gl_wayland_platform_cleanup_swapchain;
		gl_callbacks.device_enter_context =
			wayland_device_enter_context;
		gl_callbacks.device_leave_context =
			wayland_device_leave_context;
		gl_callbacks.device_get_device_obj =
			wayland_device_get_device_obj;
		gl_callbacks.getclientsize = gl_wayland_getclientsize;
		gl_callbacks.update = gl_wayland_update;
		gl_callbacks.device_load_swapchain =
			wayland_device_load_swapchain;
		gl_callbacks.device_present = wayland_device_present;
	}
#endif
	return gl_callbacks.platform_create(device, adapter);
}

extern void gl_platform_destroy(struct gl_platform *plat)
{
	gl_callbacks.platform_destroy(plat);

	gl_callbacks.platform_create = NULL;
	gl_callbacks.platform_destroy = NULL;
	gl_callbacks.platform_init_swapchain = NULL;
	gl_callbacks.platform_cleanup_swapchain = NULL;
	gl_callbacks.device_enter_context = NULL;
	gl_callbacks.device_leave_context = NULL;
	gl_callbacks.getclientsize = NULL;
	gl_callbacks.update = NULL;
	gl_callbacks.device_load_swapchain = NULL;
	gl_callbacks.device_present = NULL;
}

extern bool gl_platform_init_swapchain(struct gs_swap_chain *swap)
{
	return gl_callbacks.platform_init_swapchain(swap);
}

extern void gl_platform_cleanup_swapchain(struct gs_swap_chain *swap)
{
	gl_callbacks.platform_cleanup_swapchain(swap);
}

extern void device_enter_context(gs_device_t *device)
{
	gl_callbacks.device_enter_context(device);
}

extern void device_leave_context(gs_device_t *device)
{
	gl_callbacks.device_leave_context(device);
}

extern void *device_get_device_obj(gs_device_t *device)
{
	return gl_callbacks.device_get_device_obj(device);
}

extern void gl_getclientsize(const struct gs_swap_chain *swap, uint32_t *width,
			     uint32_t *height)
{
	gl_callbacks.getclientsize(swap, width, height);
}

extern void gl_update(gs_device_t *device)
{
	gl_callbacks.update(device);
}

extern void device_load_swapchain(gs_device_t *device, gs_swapchain_t *swap)
{
	gl_callbacks.device_load_swapchain(device, swap);
}

extern void device_present(gs_device_t *device)
{
	gl_callbacks.device_present(device);
}
