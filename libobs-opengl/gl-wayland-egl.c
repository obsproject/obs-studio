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

#include "gl-wayland-egl.h"

#include <wayland-client.h>
#include <wayland-egl.h>

#include "gl-egl-common.h"

#include <glad/glad_egl.h>

static const EGLint config_attribs_native[] = {EGL_SURFACE_TYPE,
					       EGL_WINDOW_BIT,
					       EGL_RENDERABLE_TYPE,
					       EGL_OPENGL_BIT,
					       EGL_STENCIL_SIZE,
					       0,
					       EGL_DEPTH_SIZE,
					       0,
					       EGL_BUFFER_SIZE,
					       32,
					       EGL_ALPHA_SIZE,
					       8,
					       EGL_NATIVE_RENDERABLE,
					       EGL_TRUE,
					       EGL_NONE};

static const EGLint config_attribs[] = {EGL_SURFACE_TYPE,
					EGL_WINDOW_BIT,
					EGL_RENDERABLE_TYPE,
					EGL_OPENGL_BIT,
					EGL_STENCIL_SIZE,
					0,
					EGL_DEPTH_SIZE,
					0,
					EGL_BUFFER_SIZE,
					32,
					EGL_ALPHA_SIZE,
					8,
					EGL_NONE};

static const EGLint ctx_attribs[] = {
#ifdef _DEBUG
	EGL_CONTEXT_OPENGL_DEBUG,
	EGL_TRUE,
#endif
	EGL_CONTEXT_OPENGL_PROFILE_MASK,
	EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
	EGL_CONTEXT_MAJOR_VERSION,
	3,
	EGL_CONTEXT_MINOR_VERSION,
	3,
	EGL_NONE};

static const EGLint khr_ctx_attribs[] = {
#ifdef _DEBUG
	EGL_CONTEXT_FLAGS_KHR,
	EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR,
#endif
	EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR,
	EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR,
	EGL_CONTEXT_MAJOR_VERSION_KHR,
	3,
	EGL_CONTEXT_MINOR_VERSION_KHR,
	3,
	EGL_NONE};

struct gl_windowinfo {
	struct wl_egl_window *window;
	EGLSurface egl_surface;
};

struct gl_platform {
	struct wl_display *wl_display;
	EGLDisplay display;
	EGLConfig config;
	EGLContext context;

	int drm_fd;
};

struct gl_windowinfo *gl_wayland_egl_windowinfo_create(const struct gs_init_data *info)
{
	struct wl_egl_window *window = wl_egl_window_create(info->window.display, info->cx, info->cy);
	if (window == NULL) {
		blog(LOG_ERROR, "wl_egl_window_create failed");
		return NULL;
	}

	struct gl_windowinfo *wi = bmalloc(sizeof(struct gl_windowinfo));
	wi->window = window;
	return wi;
}

static void gl_wayland_egl_windowinfo_destroy(struct gl_windowinfo *info)
{
	wl_egl_window_destroy(info->window);
	bfree(info);
}

static bool egl_make_current(EGLDisplay display, EGLSurface surface, EGLContext context)
{
	if (eglBindAPI(EGL_OPENGL_API) == EGL_FALSE) {
		blog(LOG_ERROR, "eglBindAPI failed");
	}

	if (!eglMakeCurrent(display, surface, surface, context)) {
		blog(LOG_ERROR, "eglMakeCurrent failed");
		return false;
	}

	if (surface != EGL_NO_SURFACE)
		glDrawBuffer(GL_BACK);

	return true;
}

static bool egl_context_create(struct gl_platform *plat, const EGLint *attribs)
{
	bool success = false;
	EGLint num_config;

	if (eglBindAPI(EGL_OPENGL_API) == EGL_FALSE) {
		blog(LOG_ERROR, "eglBindAPI failed");
	}

	EGLBoolean result = eglChooseConfig(plat->display, config_attribs_native, &plat->config, 1, &num_config);
	if (result != EGL_TRUE || num_config == 0) {
		result = eglChooseConfig(plat->display, config_attribs, &plat->config, 1, &num_config);
		if (result != EGL_TRUE || num_config == 0) {
			blog(LOG_ERROR, "eglChooseConfig failed");
			goto error;
		}
	}

	plat->context = eglCreateContext(plat->display, plat->config, EGL_NO_CONTEXT, attribs);
	if (plat->context == EGL_NO_CONTEXT) {
		blog(LOG_ERROR, "eglCreateContext failed");
		goto error;
	}

	success = egl_make_current(plat->display, EGL_NO_SURFACE, plat->context);

error:
	return success;
}

static void egl_context_destroy(struct gl_platform *plat)
{
	egl_make_current(plat->display, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroyContext(plat->display, plat->context);
}

static bool extension_supported(const char *extensions, const char *search)
{
	const char *result = strstr(extensions, search);
	unsigned long len = strlen(search);
	return result != NULL && (result == extensions || *(result - 1) == ' ') &&
	       (result[len] == ' ' || result[len] == '\0');
}

static struct gl_platform *gl_wayland_egl_platform_create(gs_device_t *device, uint32_t adapter)
{
	struct gl_platform *plat = bmalloc(sizeof(struct gl_platform));

	plat->wl_display = obs_get_nix_platform_display();

	device->plat = plat;

	const EGLAttrib plat_attribs[] = {EGL_NONE};
	plat->display = eglGetPlatformDisplay(EGL_PLATFORM_WAYLAND_EXT, plat->wl_display, plat_attribs);
	if (plat->display == EGL_NO_DISPLAY) {
		blog(LOG_ERROR, "eglGetDisplay failed");
		goto fail_display_init;
	}

	EGLint major;
	EGLint minor;

	if (eglInitialize(plat->display, &major, &minor) == EGL_FALSE) {
		blog(LOG_ERROR, "eglInitialize failed");
		goto fail_display_init;
	}

	blog(LOG_INFO, "Initialized EGL %d.%d", major, minor);

	const char *extensions = eglQueryString(plat->display, EGL_EXTENSIONS);
	blog(LOG_DEBUG, "Supported EGL Extensions: %s", extensions);

	const EGLint *attribs = ctx_attribs;
	if (major == 1 && minor == 4) {
		if (extension_supported(extensions, "EGL_KHR_create_context")) {
			attribs = khr_ctx_attribs;
		} else {
			blog(LOG_ERROR, "EGL_KHR_create_context extension is required to use EGL 1.4.");
			goto fail_context_create;
		}
	} else if (major < 1 || (major == 1 && minor < 4)) {
		blog(LOG_ERROR, "EGL 1.4 or higher is required.");
		goto fail_context_create;
	}

	if (!egl_context_create(plat, attribs)) {
		goto fail_context_create;
	}

	if (!gladLoadGL()) {
		blog(LOG_ERROR, "Failed to load OpenGL entry functions.");
		goto fail_load_gl;
	}

	if (!gladLoadEGL()) {
		blog(LOG_ERROR, "Unable to load EGL entry functions.");
		goto fail_load_egl;
	}

	plat->drm_fd = get_drm_render_node_fd(plat->display);
	if (plat->drm_fd < 0) {
		blog(LOG_WARNING, "Unable to open DRM render node.");
	}

	goto success;

fail_load_egl:
fail_load_gl:
	egl_context_destroy(plat);
fail_context_create:
	eglTerminate(plat->display);
fail_display_init:
	bfree(plat);
	plat = NULL;
success:
	UNUSED_PARAMETER(adapter);
	return plat;
}

static void gl_wayland_egl_platform_destroy(struct gl_platform *plat)
{
	if (plat) {
		egl_context_destroy(plat);
		eglTerminate(plat->display);
		close_drm_render_node_fd(plat->drm_fd);
		bfree(plat);
	}
}

static bool gl_wayland_egl_platform_init_swapchain(struct gs_swap_chain *swap)
{
	struct gl_platform *plat = swap->device->plat;
	EGLSurface egl_surface = eglCreateWindowSurface(plat->display, plat->config, swap->wi->window, NULL);
	if (egl_surface == EGL_NO_SURFACE) {
		blog(LOG_ERROR, "eglCreateWindowSurface failed");
		return false;
	}
	swap->wi->egl_surface = egl_surface;
	return true;
}

static void gl_wayland_egl_platform_cleanup_swapchain(struct gs_swap_chain *swap)
{
	struct gl_platform *plat = swap->device->plat;
	eglDestroySurface(plat->display, swap->wi->egl_surface);
}

static void gl_wayland_egl_device_enter_context(gs_device_t *device)
{
	struct gl_platform *plat = device->plat;
	EGLSurface surface = EGL_NO_SURFACE;
	if (device->cur_swap != NULL)
		surface = device->cur_swap->wi->egl_surface;
	egl_make_current(plat->display, surface, plat->context);
}

static void gl_wayland_egl_device_leave_context(gs_device_t *device)
{
	struct gl_platform *plat = device->plat;
	egl_make_current(plat->display, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

static void *gl_wayland_egl_device_get_device_obj(gs_device_t *device)
{
	return device->plat->context;
}

static void gl_wayland_egl_getclientsize(const struct gs_swap_chain *swap, uint32_t *width, uint32_t *height)
{
	wl_egl_window_get_attached_size(swap->wi->window, (void *)width, (void *)height);
}

static void gl_wayland_egl_clear_context(gs_device_t *device)
{
	struct gl_platform *plat = device->plat;
	egl_make_current(plat->display, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

static void gl_wayland_egl_update(gs_device_t *device)
{
	wl_egl_window_resize(device->cur_swap->wi->window, device->cur_swap->info.cx, device->cur_swap->info.cy, 0, 0);
}

static void gl_wayland_egl_device_load_swapchain(gs_device_t *device, gs_swapchain_t *swap)
{
	if (device->cur_swap == swap)
		return;

	device->cur_swap = swap;

	struct gl_platform *plat = device->plat;
	if (swap == NULL) {
		egl_make_current(plat->display, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	} else {
		egl_make_current(plat->display, swap->wi->egl_surface, plat->context);
	}
}

static void gl_wayland_egl_device_present(gs_device_t *device)
{
	struct gl_platform *plat = device->plat;
	struct gl_windowinfo *wi = device->cur_swap->wi;
	if (eglSwapInterval(plat->display, 0) == EGL_FALSE) {
		blog(LOG_ERROR, "eglSwapInterval failed");
	}
	if (eglSwapBuffers(plat->display, wi->egl_surface) == EGL_FALSE) {
		blog(LOG_ERROR, "eglSwapBuffers failed");
	}
}

static struct gs_texture *
gl_wayland_egl_device_texture_create_from_dmabuf(gs_device_t *device, unsigned int width, unsigned int height,
						 uint32_t drm_format, enum gs_color_format color_format,
						 uint32_t n_planes, const int *fds, const uint32_t *strides,
						 const uint32_t *offsets, const uint64_t *modifiers)
{
	struct gl_platform *plat = device->plat;

	return gl_egl_create_dmabuf_image(plat->display, width, height, drm_format, color_format, n_planes, fds,
					  strides, offsets, modifiers);
}

static bool gl_wayland_egl_device_query_dmabuf_capabilities(gs_device_t *device, enum gs_dmabuf_flags *dmabuf_flags,
							    uint32_t **drm_formats, size_t *n_formats)
{
	struct gl_platform *plat = device->plat;

	return gl_egl_query_dmabuf_capabilities(plat->display, dmabuf_flags, drm_formats, n_formats);
}

static bool gl_wayland_egl_device_query_dmabuf_modifiers_for_format(gs_device_t *device, uint32_t drm_format,
								    uint64_t **modifiers, size_t *n_modifiers)
{
	struct gl_platform *plat = device->plat;

	return gl_egl_query_dmabuf_modifiers_for_format(plat->display, drm_format, modifiers, n_modifiers);
}

static struct gs_texture *gl_wayland_egl_device_texture_create_from_pixmap(gs_device_t *device, uint32_t width,
									   uint32_t height,
									   enum gs_color_format color_format,
									   uint32_t target, void *pixmap)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(width);
	UNUSED_PARAMETER(height);
	UNUSED_PARAMETER(color_format);
	UNUSED_PARAMETER(target);
	UNUSED_PARAMETER(pixmap);

	return NULL;
}

static bool gl_wayland_egl_enum_adapters(gs_device_t *device,
					 bool (*callback)(void *param, const char *name, uint32_t id), void *param)
{
	return gl_egl_enum_adapters(device->plat->display, callback, param);
}

static bool gl_wayland_egl_device_query_sync_capabilities(gs_device_t *device)
{
	struct gl_platform *plat = device->plat;

	return gl_egl_query_sync_capabilities(plat->drm_fd);
}

static gs_sync_t *gl_wayland_egl_device_sync_create(gs_device_t *device)
{
	struct gl_platform *plat = device->plat;

	return gl_egl_create_sync(plat->display);
}

static gs_sync_t *gl_wayland_egl_device_sync_create_from_syncobj_timeline_point(gs_device_t *device, int syncobj_fd,
										uint64_t timeline_point)
{
	struct gl_platform *plat = device->plat;

	return gl_egl_create_sync_from_syncobj_timeline_point(plat->display, plat->drm_fd, syncobj_fd, timeline_point);
}

static void gl_wayland_egl_device_sync_destroy(gs_device_t *device, gs_sync_t *sync)
{
	struct gl_platform *plat = device->plat;

	gl_egl_device_sync_destroy(plat->display, sync);
}

static bool gl_wayland_egl_device_sync_export_syncobj_timeline_point(gs_device_t *device, gs_sync_t *sync,
								     int syncobj_fd, uint64_t timeline_point)
{
	struct gl_platform *plat = device->plat;

	return gl_egl_sync_export_syncobj_timeline_point(plat->display, sync, plat->drm_fd, syncobj_fd, timeline_point);
}

static bool gl_wayland_egl_device_sync_signal_syncobj_timeline_point(gs_device_t *device, int syncobj_fd,
								     uint64_t timeline_point)
{
	struct gl_platform *plat = device->plat;

	return gl_egl_sync_signal_syncobj_timeline_point(plat->drm_fd, syncobj_fd, timeline_point);
}

static bool gl_wayland_egl_device_sync_wait(gs_device_t *device, gs_sync_t *sync)
{
	struct gl_platform *plat = device->plat;

	return gl_egl_sync_wait(plat->display, sync);
}

static const struct gl_winsys_vtable egl_wayland_winsys_vtable = {
	.windowinfo_create = gl_wayland_egl_windowinfo_create,
	.windowinfo_destroy = gl_wayland_egl_windowinfo_destroy,
	.platform_create = gl_wayland_egl_platform_create,
	.platform_destroy = gl_wayland_egl_platform_destroy,
	.platform_init_swapchain = gl_wayland_egl_platform_init_swapchain,
	.platform_cleanup_swapchain = gl_wayland_egl_platform_cleanup_swapchain,
	.device_enter_context = gl_wayland_egl_device_enter_context,
	.device_leave_context = gl_wayland_egl_device_leave_context,
	.device_get_device_obj = gl_wayland_egl_device_get_device_obj,
	.getclientsize = gl_wayland_egl_getclientsize,
	.clear_context = gl_wayland_egl_clear_context,
	.update = gl_wayland_egl_update,
	.device_load_swapchain = gl_wayland_egl_device_load_swapchain,
	.device_present = gl_wayland_egl_device_present,
	.device_texture_create_from_dmabuf = gl_wayland_egl_device_texture_create_from_dmabuf,
	.device_query_dmabuf_capabilities = gl_wayland_egl_device_query_dmabuf_capabilities,
	.device_query_dmabuf_modifiers_for_format = gl_wayland_egl_device_query_dmabuf_modifiers_for_format,
	.device_texture_create_from_pixmap = gl_wayland_egl_device_texture_create_from_pixmap,
	.device_enum_adapters = gl_wayland_egl_enum_adapters,
	.device_query_sync_capabilities = gl_wayland_egl_device_query_sync_capabilities,
	.device_sync_create = gl_wayland_egl_device_sync_create,
	.device_sync_create_from_syncobj_timeline_point = gl_wayland_egl_device_sync_create_from_syncobj_timeline_point,
	.device_sync_destroy = gl_wayland_egl_device_sync_destroy,
	.device_sync_export_syncobj_timeline_point = gl_wayland_egl_device_sync_export_syncobj_timeline_point,
	.device_sync_signal_syncobj_timeline_point = gl_wayland_egl_device_sync_signal_syncobj_timeline_point,
	.device_sync_wait = gl_wayland_egl_device_sync_wait,
};

const struct gl_winsys_vtable *gl_wayland_egl_get_winsys_vtable(void)
{
	return &egl_wayland_winsys_vtable;
}
