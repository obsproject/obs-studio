/******************************************************************************
    Copyright (C) 2019 by Ivan Avdeev <me@w23.ru>

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

/* GL context initialization using EGL instead of GLX
 * Which is essential for improved and more performant screen grabbing and
 * VA-API feeding techniques.
 *
 * Note: most of x11-related functionality was taken from gl-x11.c
 */

#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>

#include <xcb/xcb.h>

#include <stdio.h>

#include "gl-egl-common.h"
#include "gl-x11-egl.h"

#include <glad/glad_egl.h>

typedef EGLDisplay(EGLAPIENTRYP PFNEGLGETPLATFORMDISPLAYEXTPROC)(EGLenum platform, void *native_display,
								 const EGLint *attrib_list);

static const int ctx_attribs[] = {
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
	EGL_NONE,
};

static int ctx_pbuffer_attribs[] = {EGL_WIDTH, 2, EGL_HEIGHT, 2, EGL_NONE};

static const EGLint ctx_config_attribs[] = {EGL_STENCIL_SIZE,
					    0,
					    EGL_DEPTH_SIZE,
					    0,
					    EGL_BUFFER_SIZE,
					    24,
					    EGL_ALPHA_SIZE,
					    0,
					    EGL_RENDERABLE_TYPE,
					    EGL_OPENGL_BIT,
					    EGL_SURFACE_TYPE,
					    EGL_WINDOW_BIT | EGL_PBUFFER_BIT,
					    EGL_NONE};

struct gl_windowinfo {
	/* Windows in X11 are defined with integers (XID).
	 * xcb_window_t is a define for this... they are
	 * compatible with Xlib as well.
	 */
	xcb_window_t window;
	EGLSurface surface;
};

struct gl_platform {
	Display *xdisplay;
	EGLDisplay edisplay;
	EGLConfig config;
	EGLContext context;
	EGLSurface pbuffer;
	bool close_xdisplay;
	int drm_fd;
};

/* The following utility function is copied verbatim from GLX code. */

static xcb_get_geometry_reply_t *get_window_geometry(xcb_connection_t *xcb_conn, xcb_drawable_t drawable)
{
	xcb_get_geometry_cookie_t cookie;
	xcb_generic_error_t *error;
	xcb_get_geometry_reply_t *reply;

	cookie = xcb_get_geometry(xcb_conn, drawable);
	reply = xcb_get_geometry_reply(xcb_conn, cookie, &error);

	if (error) {
		blog(LOG_ERROR, "Failed to fetch parent window geometry!");
		free(error);
		free(reply);
		return 0;
	}

	return reply;
}

static const char *get_egl_error_string2(const EGLint error)
{
	switch (error) {
#define OBS_EGL_CASE_ERROR(e) \
	case e:               \
		return #e;
		OBS_EGL_CASE_ERROR(EGL_SUCCESS)
		OBS_EGL_CASE_ERROR(EGL_NOT_INITIALIZED)
		OBS_EGL_CASE_ERROR(EGL_BAD_ACCESS)
		OBS_EGL_CASE_ERROR(EGL_BAD_ALLOC)
		OBS_EGL_CASE_ERROR(EGL_BAD_ATTRIBUTE)
		OBS_EGL_CASE_ERROR(EGL_BAD_CONTEXT)
		OBS_EGL_CASE_ERROR(EGL_BAD_CONFIG)
		OBS_EGL_CASE_ERROR(EGL_BAD_CURRENT_SURFACE)
		OBS_EGL_CASE_ERROR(EGL_BAD_DISPLAY)
		OBS_EGL_CASE_ERROR(EGL_BAD_SURFACE)
		OBS_EGL_CASE_ERROR(EGL_BAD_MATCH)
		OBS_EGL_CASE_ERROR(EGL_BAD_PARAMETER)
		OBS_EGL_CASE_ERROR(EGL_BAD_NATIVE_PIXMAP)
		OBS_EGL_CASE_ERROR(EGL_BAD_NATIVE_WINDOW)
		OBS_EGL_CASE_ERROR(EGL_CONTEXT_LOST)
#undef OBS_EGL_CASE_ERROR
	default:
		return "Unknown";
	}
}

static const char *get_egl_error_string()
{
	return get_egl_error_string2(eglGetError());
}

static EGLDisplay get_egl_display(struct gl_platform *plat)
{
	Display *display = plat->xdisplay;
	EGLDisplay edisplay = EGL_NO_DISPLAY;
	const char *egl_client_extensions = NULL;

	egl_client_extensions = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);

	PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT =
		(PFNEGLGETPLATFORMDISPLAYEXTPROC)(strstr(egl_client_extensions, "EGL_EXT_platform_base")
							  ? eglGetProcAddress("eglGetPlatformDisplayEXT")
							  : NULL);

	if (eglGetPlatformDisplayEXT) {
		const EGLint plat_attribs[] = {EGL_NONE};
		edisplay = eglGetPlatformDisplayEXT(EGL_PLATFORM_X11_EXT, display, plat_attribs);
		if (EGL_NO_DISPLAY == edisplay)
			blog(LOG_ERROR, "Failed to get EGL/X11 display");
	}

	if (EGL_NO_DISPLAY == edisplay)
		edisplay = eglGetDisplay(display);

	return edisplay;
}

static bool gl_context_create(struct gl_platform *plat)
{
	Display *display = plat->xdisplay;
	int frame_buf_config_count = 0;
	EGLDisplay edisplay = EGL_NO_DISPLAY;
	EGLConfig config = NULL;
	EGLContext context = EGL_NO_CONTEXT;
	int egl_min = 0, egl_maj = 0;
	bool success = false;

	eglBindAPI(EGL_OPENGL_API);

	edisplay = get_egl_display(plat);

	if (EGL_NO_DISPLAY == edisplay) {
		blog(LOG_ERROR, "Failed to get EGL display using eglGetDisplay");
		return false;
	}

	if (!eglInitialize(edisplay, &egl_maj, &egl_min)) {
		blog(LOG_ERROR, "Failed to initialize EGL: %s", get_egl_error_string());
		return false;
	}

	if (!eglChooseConfig(edisplay, ctx_config_attribs, &config, 1, &frame_buf_config_count)) {
		blog(LOG_ERROR, "Unable to find suitable EGL config: %s", get_egl_error_string());
		goto error;
	}

	context = eglCreateContext(edisplay, config, EGL_NO_CONTEXT, ctx_attribs);
#ifdef _DEBUG
	if (EGL_NO_CONTEXT == context) {
		const EGLint error = eglGetError();
		if (error == EGL_BAD_ATTRIBUTE) {
			/* Sometimes creation fails because debug gl is not supported */
			blog(LOG_ERROR, "Unable to create EGL context with DEBUG attrib, trying without");
			context = eglCreateContext(edisplay, config, EGL_NO_CONTEXT, ctx_attribs + 2);
		} else {
			blog(LOG_ERROR, "Unable to create EGL context: %s", get_egl_error_string2(error));
			goto error;
		}
	}
#endif
	if (EGL_NO_CONTEXT == context) {
		blog(LOG_ERROR, "Unable to create EGL context: %s", get_egl_error_string());
		goto error;
	}

	plat->pbuffer = eglCreatePbufferSurface(edisplay, config, ctx_pbuffer_attribs);
	if (EGL_NO_SURFACE == plat->pbuffer) {
		blog(LOG_ERROR, "Failed to create OpenGL pbuffer: %s", get_egl_error_string());
		goto error;
	}

	plat->edisplay = edisplay;
	plat->config = config;
	plat->context = context;

	success = true;
	blog(LOG_DEBUG, "Created EGLDisplay %p", plat->edisplay);

error:
	if (!success) {
		if (EGL_NO_CONTEXT != context)
			eglDestroyContext(edisplay, context);
		eglTerminate(edisplay);
	}

	XSync(display, false);
	return success;
}

static void gl_context_destroy(struct gl_platform *plat)
{
	eglMakeCurrent(plat->edisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroyContext(plat->edisplay, plat->context);
}

static struct gl_windowinfo *gl_x11_egl_windowinfo_create(const struct gs_init_data *info)
{
	UNUSED_PARAMETER(info);
	return bmalloc(sizeof(struct gl_windowinfo));
}

static void gl_x11_egl_windowinfo_destroy(struct gl_windowinfo *info)
{
	bfree(info);
}

static Display *open_windowless_display(bool *should_close)
{
	Display *platform_display = obs_get_nix_platform_display();
	Display *display;
	xcb_connection_t *xcb_conn;

	if (platform_display) {
		display = platform_display;
		*should_close = false;
	} else {
		display = XOpenDisplay(NULL);
		*should_close = true;
	}

	if (!display) {
		blog(LOG_ERROR, "Unable to open new X connection!");
		return NULL;
	}

	xcb_conn = XGetXCBConnection(display);
	if (!xcb_conn) {
		blog(LOG_ERROR, "Unable to get XCB connection to main display");
		goto error;
	}

	if (!gladLoadEGL()) {
		blog(LOG_ERROR, "Unable to load EGL entry functions.");
		goto error;
	}

	return display;

error:
	if (*should_close)
		XCloseDisplay(display);
	return NULL;
}

static int x_error_handler(Display *display, XErrorEvent *error)
{
	char str1[512];
	char str2[512];
	char str3[512];
	XGetErrorText(display, error->error_code, str1, sizeof(str1));
	XGetErrorText(display, error->request_code, str2, sizeof(str2));
	XGetErrorText(display, error->minor_code, str3, sizeof(str3));

	blog(LOG_ERROR,
	     "X Error: %s, Major opcode: %s, "
	     "Minor opcode: %s, Serial: %lu",
	     str1, str2, str3, error->serial);
	return 0;
}

static struct gl_platform *gl_x11_egl_platform_create(gs_device_t *device, uint32_t adapter)
{
	/* There's some trickery here... we're mixing libX11, xcb, and EGL
	   For an explanation see here: http://xcb.freedesktop.org/MixingCalls/
	   Essentially, EGL requires Xlib. Everything else we use xcb. */
	struct gl_platform *plat = bmalloc(sizeof(struct gl_platform));
	Display *display = open_windowless_display(&plat->close_xdisplay);

	if (!display) {
		goto fail_display_open;
	}

	XSetEventQueueOwner(display, XCBOwnsEventQueue);
	XSetErrorHandler(x_error_handler);

	/* We assume later that cur_swap is already set. */
	device->plat = plat;

	plat->xdisplay = display;

	if (!gl_context_create(plat)) {
		blog(LOG_ERROR, "Failed to create context!");
		goto fail_context_create;
	}

	if (!eglMakeCurrent(plat->edisplay, plat->pbuffer, plat->pbuffer, plat->context)) {
		blog(LOG_ERROR, "Failed to make context current: %s", get_egl_error_string());
		goto fail_make_current;
	}

	if (!gladLoadGL()) {
		blog(LOG_ERROR, "Failed to load OpenGL entry functions.");
		goto fail_load_gl;
	}

	plat->drm_fd = get_drm_render_node_fd(plat->edisplay);
	if (plat->drm_fd < 0) {
		blog(LOG_WARNING, "Unable to open DRM render node.");
	}

	goto success;

fail_make_current:
	gl_context_destroy(plat);
fail_context_create:
fail_load_gl:
	if (plat->close_xdisplay)
		XCloseDisplay(plat->xdisplay);
fail_display_open:
	bfree(plat);
	plat = NULL;
success:
	UNUSED_PARAMETER(adapter);
	return plat;
}

static void gl_x11_egl_platform_destroy(struct gl_platform *plat)
{
	if (!plat)
		return;

	gl_context_destroy(plat);
	eglTerminate(plat->edisplay);
	if (plat->close_xdisplay)
		XCloseDisplay(plat->xdisplay);
	close_drm_render_node_fd(plat->drm_fd);
	bfree(plat);
}

static bool gl_x11_egl_platform_init_swapchain(struct gs_swap_chain *swap)
{
	const struct gl_platform *plat = swap->device->plat;
	Display *display = plat->xdisplay;
	xcb_connection_t *xcb_conn = XGetXCBConnection(display);
	xcb_window_t wid = xcb_generate_id(xcb_conn);
	xcb_window_t parent = swap->info.window.id;
	xcb_get_geometry_reply_t *geometry = get_window_geometry(xcb_conn, parent);
	bool status = false;

	int visual;

	if (!geometry)
		goto fail_geometry_request;

	{
		if (!eglGetConfigAttrib(plat->edisplay, plat->config, EGL_NATIVE_VISUAL_ID, (EGLint *)&visual)) {
			blog(LOG_ERROR, "Cannot get visual id for EGL context: %s", get_egl_error_string());
			goto fail_visual_id;
		}
	}

	xcb_colormap_t colormap = xcb_generate_id(xcb_conn);
	uint32_t mask = XCB_CW_BORDER_PIXEL | XCB_CW_COLORMAP;
	uint32_t mask_values[] = {0, colormap, 0};

	xcb_create_colormap(xcb_conn, XCB_COLORMAP_ALLOC_NONE, colormap, parent, visual);

	xcb_void_cookie_t window_cookie = xcb_create_window_checked(xcb_conn, 24 /* Hardcoded? */, wid, parent, 0, 0,
								    geometry->width, geometry->height, 0, 0, visual,
								    mask, mask_values);
	xcb_generic_error_t *err = xcb_request_check(xcb_conn, window_cookie);
	if (err != NULL) {
		char text[512];
		XGetErrorText(display, err->error_code, text, sizeof(text));
		blog(LOG_ERROR,
		     "Swapchain window creation failed: %s"
		     ", Major opcode: %d, Minor opcode: %d",
		     text, err->major_code, err->minor_code);
		goto fail_window_surface;
	}

	const EGLSurface surface = eglCreateWindowSurface(plat->edisplay, plat->config, wid, 0);
	if (EGL_NO_SURFACE == surface) {
		blog(LOG_ERROR, "Cannot get window EGL surface: %s", get_egl_error_string());
		goto fail_window_surface;
	}

	swap->wi->window = wid;
	swap->wi->surface = surface;

	xcb_map_window(xcb_conn, wid);

	status = true;
	goto success;

fail_window_surface:
fail_visual_id:
fail_geometry_request:
success:
	free(geometry);
	return status;
}

static void gl_x11_egl_platform_cleanup_swapchain(struct gs_swap_chain *swap)
{
	UNUSED_PARAMETER(swap);
	/* Really nothing to clean up? */
}

static void gl_x11_egl_device_enter_context(gs_device_t *device)
{
	const EGLContext context = device->plat->context;
	const EGLDisplay display = device->plat->edisplay;
	const EGLSurface surface = (device->cur_swap) ? device->cur_swap->wi->surface : device->plat->pbuffer;

	if (!eglMakeCurrent(display, surface, surface, context))
		blog(LOG_ERROR, "Failed to make context current: %s", get_egl_error_string());
}

static void gl_x11_egl_device_leave_context(gs_device_t *device)
{
	const EGLDisplay display = device->plat->edisplay;

	if (!eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT)) {
		blog(LOG_ERROR, "Failed to reset current context: %s", get_egl_error_string());
	}
}

static void *gl_x11_egl_device_get_device_obj(gs_device_t *device)
{
	return device->plat->context;
}

static void gl_x11_egl_getclientsize(const struct gs_swap_chain *swap, uint32_t *width, uint32_t *height)
{
	xcb_connection_t *xcb_conn = XGetXCBConnection(swap->device->plat->xdisplay);
	xcb_window_t window = swap->wi->window;

	xcb_get_geometry_reply_t *geometry = get_window_geometry(xcb_conn, window);
	if (geometry) {
		*width = geometry->width;
		*height = geometry->height;
	}

	free(geometry);
}

static void gl_x11_egl_update(gs_device_t *device)
{
	Display *display = device->plat->xdisplay;
	xcb_window_t window = device->cur_swap->wi->window;

	uint32_t values[] = {device->cur_swap->info.cx, device->cur_swap->info.cy};

	xcb_configure_window(XGetXCBConnection(display), window, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
			     values);
}

static void gl_x11_egl_clear_context(gs_device_t *device)
{
	Display *display = device->plat->edisplay;

	if (!eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT)) {
		blog(LOG_ERROR, "Failed to reset current context.");
	}
}

static void gl_x11_egl_device_load_swapchain(gs_device_t *device, gs_swapchain_t *swap)
{
	if (device->cur_swap == swap)
		return;

	device->cur_swap = swap;

	device_enter_context(device);
}

static void gl_x11_egl_device_present(gs_device_t *device)
{
	Display *display = device->plat->xdisplay;

	xcb_connection_t *xcb_conn = XGetXCBConnection(display);
	xcb_generic_event_t *xcb_event;
	while ((xcb_event = xcb_poll_for_event(xcb_conn))) {
		free(xcb_event);
	}

	if (eglSwapInterval(device->plat->edisplay, 0) == EGL_FALSE) {
		blog(LOG_ERROR, "eglSwapInterval failed");
	}
	if (!eglSwapBuffers(device->plat->edisplay, device->cur_swap->wi->surface))
		blog(LOG_ERROR, "Cannot swap EGL buffers: %s", get_egl_error_string());
}

static struct gs_texture *gl_x11_egl_device_texture_create_from_dmabuf(gs_device_t *device, unsigned int width,
								       unsigned int height, uint32_t drm_format,
								       enum gs_color_format color_format,
								       uint32_t n_planes, const int *fds,
								       const uint32_t *strides, const uint32_t *offsets,
								       const uint64_t *modifiers)
{
	struct gl_platform *plat = device->plat;

	return gl_egl_create_dmabuf_image(plat->edisplay, width, height, drm_format, color_format, n_planes, fds,
					  strides, offsets, modifiers);
}

static struct gs_texture *gl_x11_egl_device_texture_create_from_pixmap(gs_device_t *device, uint32_t width,
								       uint32_t height,
								       enum gs_color_format color_format,
								       uint32_t target, void *pixmap)
{
	struct gl_platform *plat = device->plat;

	return gl_egl_create_texture_from_pixmap(plat->edisplay, width, height, color_format, target,
						 (EGLClientBuffer)pixmap);
}

static bool gl_x11_egl_device_query_dmabuf_capabilities(gs_device_t *device, enum gs_dmabuf_flags *dmabuf_flags,
							uint32_t **drm_formats, size_t *n_formats)
{
	struct gl_platform *plat = device->plat;

	return gl_egl_query_dmabuf_capabilities(plat->edisplay, dmabuf_flags, drm_formats, n_formats);
}

static bool gl_x11_egl_device_query_dmabuf_modifiers_for_format(gs_device_t *device, uint32_t drm_format,
								uint64_t **modifiers, size_t *n_modifiers)
{
	struct gl_platform *plat = device->plat;

	return gl_egl_query_dmabuf_modifiers_for_format(plat->edisplay, drm_format, modifiers, n_modifiers);
}

static bool gl_x11_egl_enum_adapters(gs_device_t *device, bool (*callback)(void *param, const char *name, uint32_t id),
				     void *param)
{
	return gl_egl_enum_adapters(device->plat->edisplay, callback, param);
}

static bool gl_x11_egl_device_query_sync_capabilities(gs_device_t *device)
{
	struct gl_platform *plat = device->plat;

	return gl_egl_query_sync_capabilities(plat->drm_fd);
}

static gs_sync_t *gl_x11_egl_device_sync_create(gs_device_t *device)
{
	struct gl_platform *plat = device->plat;

	return gl_egl_create_sync(plat->edisplay);
}

static gs_sync_t *gl_x11_egl_device_sync_create_from_syncobj_timeline_point(gs_device_t *device, int syncobj_fd,
									    uint64_t timeline_point)
{
	struct gl_platform *plat = device->plat;

	return gl_egl_create_sync_from_syncobj_timeline_point(plat->edisplay, plat->drm_fd, syncobj_fd, timeline_point);
}

static void gl_x11_egl_device_sync_destroy(gs_device_t *device, gs_sync_t *sync)
{
	struct gl_platform *plat = device->plat;

	gl_egl_device_sync_destroy(plat->edisplay, sync);
}

static bool gl_x11_egl_device_sync_export_syncobj_timeline_point(gs_device_t *device, gs_sync_t *sync, int syncobj_fd,
								 uint64_t timeline_point)
{
	struct gl_platform *plat = device->plat;

	return gl_egl_sync_export_syncobj_timeline_point(plat->edisplay, sync, plat->drm_fd, syncobj_fd,
							 timeline_point);
}

static bool gl_x11_egl_device_sync_signal_syncobj_timeline_point(gs_device_t *device, int syncobj_fd,
								 uint64_t timeline_point)
{
	struct gl_platform *plat = device->plat;

	return gl_egl_sync_signal_syncobj_timeline_point(plat->drm_fd, syncobj_fd, timeline_point);
}

static bool gl_x11_egl_device_sync_wait(gs_device_t *device, gs_sync_t *sync)
{
	struct gl_platform *plat = device->plat;

	return gl_egl_sync_wait(plat->edisplay, sync);
}

static const struct gl_winsys_vtable egl_x11_winsys_vtable = {
	.windowinfo_create = gl_x11_egl_windowinfo_create,
	.windowinfo_destroy = gl_x11_egl_windowinfo_destroy,
	.platform_create = gl_x11_egl_platform_create,
	.platform_destroy = gl_x11_egl_platform_destroy,
	.platform_init_swapchain = gl_x11_egl_platform_init_swapchain,
	.platform_cleanup_swapchain = gl_x11_egl_platform_cleanup_swapchain,
	.device_enter_context = gl_x11_egl_device_enter_context,
	.device_leave_context = gl_x11_egl_device_leave_context,
	.device_get_device_obj = gl_x11_egl_device_get_device_obj,
	.getclientsize = gl_x11_egl_getclientsize,
	.clear_context = gl_x11_egl_clear_context,
	.update = gl_x11_egl_update,
	.device_load_swapchain = gl_x11_egl_device_load_swapchain,
	.device_present = gl_x11_egl_device_present,
	.device_texture_create_from_dmabuf = gl_x11_egl_device_texture_create_from_dmabuf,
	.device_query_dmabuf_capabilities = gl_x11_egl_device_query_dmabuf_capabilities,
	.device_query_dmabuf_modifiers_for_format = gl_x11_egl_device_query_dmabuf_modifiers_for_format,
	.device_texture_create_from_pixmap = gl_x11_egl_device_texture_create_from_pixmap,
	.device_enum_adapters = gl_x11_egl_enum_adapters,
	.device_query_sync_capabilities = gl_x11_egl_device_query_sync_capabilities,
	.device_sync_create = gl_x11_egl_device_sync_create,
	.device_sync_create_from_syncobj_timeline_point = gl_x11_egl_device_sync_create_from_syncobj_timeline_point,
	.device_sync_destroy = gl_x11_egl_device_sync_destroy,
	.device_sync_export_syncobj_timeline_point = gl_x11_egl_device_sync_export_syncobj_timeline_point,
	.device_sync_signal_syncobj_timeline_point = gl_x11_egl_device_sync_signal_syncobj_timeline_point,
	.device_sync_wait = gl_x11_egl_device_sync_wait,
};

const struct gl_winsys_vtable *gl_x11_egl_get_winsys_vtable(void)
{
	return &egl_x11_winsys_vtable;
}
