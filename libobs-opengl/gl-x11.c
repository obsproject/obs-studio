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

/* Version 2 of the GLX backend...
 * Difference from version 1 is that we use XCB to help alleviate
 * pains in a threaded environment that is prone to error.
 * These errors must be readable and handled for the sake of,
 * not only the users' sanity, but my own.
 *
 * With that said, we have more error checking capabilities...
 * and not all of them are used to help simplify current code.
 *
 * TODO: Implement more complete error checking.
 * NOTE: GLX loading functions are placed illogically
 * 	for the sake of convenience.
 */

#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>

#include <xcb/xcb.h>

#include <stdio.h>

#include "gl-subsystem.h"

#include <glad/glad_glx.h>

static const int ctx_attribs[] = {
#ifdef _DEBUG
	GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_DEBUG_BIT_ARB,
#endif
	GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
	GLX_CONTEXT_MAJOR_VERSION_ARB, 3, GLX_CONTEXT_MINOR_VERSION_ARB, 2,
	None,
};

struct gl_windowinfo {
	/* We store this value since we can fetch a lot
	 * of information not only concerning the config
	 * but the visual, and various other settings
	 * for the context.
	 */
	GLXFBConfig config;

	/* Windows in X11 are defined with integers (XID).
	 * xcb_window_t is a define for this... they are
	 * compatible with Xlib as well.
	 */
	xcb_window_t window;

	/* We can't fetch screen without a request so we cache it. */
	int screen;
};

struct gl_platform {
	Display *display;
	GLXContext context;
	struct gs_swap_chain swap;
};

extern struct gs_swap_chain *gl_platform_getswap(struct gl_platform *platform)
{
	return &platform->swap;
}

static void print_info_stuff(const struct gs_init_data *info)
{
	blog(	LOG_INFO,
		"X and Y: %i %i\n"
		"Backbuffers: %i\n"
		"Color Format: %i\n"
		"ZStencil Format: %i\n"
		"Adapter: %i\n",
		info->cx, info->cy,
		info->num_backbuffers,
		info->format, info->zsformat,
		info->adapter
	);
}
/* The following utility functions are copied verbatim from WGL code.
 * GLX and WGL are more similar than most people realize. */

/* For now, only support basic 32bit formats for graphics output. */
static inline int get_color_format_bits(enum gs_color_format format)
{
	switch ((uint32_t)format) {
	case GS_RGBA:
		return 32;
	default:
		return 0;
	}
}

static inline int get_depth_format_bits(enum gs_zstencil_format zsformat)
{
	switch ((uint32_t)zsformat) {
	case GS_Z16:
		return 16;
	case GS_Z24_S8:
		return 24;
	default:
		return 0;
	}
}

static inline int get_stencil_format_bits(enum gs_zstencil_format zsformat)
{
	switch ((uint32_t)zsformat) {
	case GS_Z24_S8:
		return 8;
	default:
		return 0;
	}
}

/*
 * Since we cannot take advantage of the asynchronous nature of xcb,
 * all of the helper functions are synchronous but thread-safe.
 *
 * They check for errors and will return 0 on problems
 * with the exception of when 0 is a valid return value... in which case
 * read the specific function comments.
 */

/* Returns -1 on invalid screen. */
static int get_screen_num_from_xcb_screen(xcb_connection_t *xcb_conn,
		xcb_screen_t *screen)
{
	xcb_screen_iterator_t iter =
		xcb_setup_roots_iterator(xcb_get_setup(xcb_conn));
	int screen_num = 0;

	for (; iter.rem; xcb_screen_next(&iter), ++screen_num)
		if (iter.data == screen)
			return screen_num;

	return -1;
}

static xcb_screen_t *get_screen_from_root(xcb_connection_t *xcb_conn,
		xcb_window_t root)
{
	xcb_screen_iterator_t iter =
		xcb_setup_roots_iterator(xcb_get_setup(xcb_conn));

	while (iter.rem) {
		if (iter.data->root == root)
			return iter.data;

		xcb_screen_next(&iter);
	}

	return 0;
}

static inline int get_screen_num_from_root(xcb_connection_t *xcb_conn,
		xcb_window_t root)
{
	xcb_screen_t *screen = get_screen_from_root(xcb_conn, root);

	if (!screen) return -1;

	return get_screen_num_from_xcb_screen(xcb_conn, screen);
}

static xcb_get_geometry_reply_t* get_window_geometry(
		xcb_connection_t *xcb_conn, xcb_drawable_t drawable)
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

	free(error);
	return reply;
}

static bool gl_context_create(struct gl_platform *plat)
{
	Display *display = plat->display;
	GLXFBConfig config = plat->swap.wi->config;
	int major, minor;
	GLXContext context;

	{
		int error_base;
		int event_base;

		if (!glXQueryExtension(display, &error_base, &event_base)) {
			blog(LOG_ERROR, "GLX not supported.");
			return 0;
		}
	}

	/* We require glX version 1.3 */
	glXQueryVersion(display, &major, &minor);
	if (major < 1 || (major == 1 && minor < 3)) {
		blog(LOG_ERROR, "GLX version found: %i.%i\nRequired: "
				"1.3", major, minor);
		return false;
	}

	if (!GLAD_GLX_ARB_create_context) {
		blog(LOG_ERROR, "ARB_GLX_create_context not supported!");
		return false;
	}

	context = glXCreateContextAttribsARB(display, config, NULL,
			true, ctx_attribs);
	if (!context) {
		blog(LOG_ERROR, "Failed to create OpenGL context.");
		return false;
	}

	plat->context = context;
	plat->display = display;

	return true;
}

static void gl_context_destroy(struct gl_platform *plat)
{
	Display *display = plat->display;

	glXMakeCurrent(display, None, 0);
	glXDestroyContext(display, plat->context);
	bfree(plat);
}

extern struct gl_windowinfo *gl_windowinfo_create(const struct gs_init_data *info)
{
	UNUSED_PARAMETER(info);
	return bmalloc(sizeof(struct gl_windowinfo));
}

extern void gl_windowinfo_destroy(struct gl_windowinfo *info)
{
	UNUSED_PARAMETER(info);
	bfree(info);
}

extern struct gl_platform *gl_platform_create(gs_device_t *device,
		const struct gs_init_data *info)
{
	/* There's some trickery here... we're mixing libX11, xcb, and GLX
	   For an explanation see here: http://xcb.freedesktop.org/MixingCalls/
	   Essentially, GLX requires Xlib. Everything else we use xcb. */
	struct gl_windowinfo *wi = gl_windowinfo_create(info);
	struct gl_platform * plat = bmalloc(sizeof(struct gl_platform));
	Display * display;

	print_info_stuff(info);

	if (!wi) {
		blog(LOG_ERROR, "Failed to create window info!");
		goto fail_wi_create;
	}

	display = XOpenDisplay(XDisplayString(info->window.display));
	if (!display) {
		blog(LOG_ERROR, "Unable to open new X connection!");
		goto fail_display_open;
	}

	XSetEventQueueOwner(display, XCBOwnsEventQueue);

	/* We assume later that cur_swap is already set. */
	device->cur_swap = &plat->swap;
	device->plat = plat;

	plat->display = display;
	plat->swap.device = device;
	plat->swap.info = *info;
	plat->swap.wi = wi;

	if (!gl_platform_init_swapchain(&plat->swap)) {
		blog(LOG_ERROR, "Failed to initialize swap chain!");
		goto fail_init_swapchain;
	}

	if (!gl_context_create(plat)) {
		blog(LOG_ERROR, "Failed to create context!");
		goto fail_context_create;
	}

	if (!glXMakeCurrent(plat->display, wi->window, plat->context)) {
		blog(LOG_ERROR, "Failed to make context current.");
		goto fail_make_current;
	}

	if (!gladLoadGL()) {
		blog(LOG_ERROR, "Failed to load OpenGL entry functions.");
		goto fail_load_gl;
	}

	blog(LOG_INFO, "OpenGL version: %s\n", glGetString(GL_VERSION));

	goto success;

fail_make_current:
	gl_context_destroy(plat);
fail_context_create:
fail_load_gl:
fail_init_swapchain:
	XCloseDisplay(display);
fail_display_open:
fail_wi_create:
	gl_windowinfo_destroy(wi);
	free(plat);
	plat = NULL;
success:
	return plat;
}

extern void gl_platform_destroy(struct gl_platform *plat)
{
	if (!plat) /* In what case would platform be invalid here? */
		return;

	struct gl_windowinfo *wi = plat->swap.wi;

	gl_context_destroy(plat);
	gl_windowinfo_destroy(wi);
}

extern bool gl_platform_init_swapchain(struct gs_swap_chain *swap)
{
	Display *display = swap->device->plat->display;
	struct gs_init_data *info = &swap->info;
	xcb_connection_t *xcb_conn = XGetXCBConnection(display);
	xcb_window_t wid = xcb_generate_id(xcb_conn);
	xcb_window_t parent = swap->info.window.id;
	xcb_get_geometry_reply_t *geometry =
		get_window_geometry(xcb_conn, parent);
	bool status = false;

	int screen_num;
	int visual;
	GLXFBConfig *fb_config;

	if (!geometry) goto fail_geometry_request;

	screen_num = get_screen_num_from_root(xcb_conn, geometry->root);
	if (screen_num == -1) {
		goto fail_screen;
	}

	/* NOTE:
	 * So GLX is odd. You can have different extensions per screen,
	 * not just per video card or visual.
	 *
	 * Because of this, it makes sense to call LoadGLX everytime
	 * we open a frackin' window. In Windows, entry points can change
	 * so it makes more sense there. Here, despite it virtually never
	 * having the possibility of changing unless the user is intentionally
	 * being an asshole to cause this behavior, we still have to give it
	 * the correct screen num just out of good practice. *sigh*
	 */
	if (!gladLoadGLX(display, screen_num)) {
		blog(LOG_ERROR, "Unable to load GLX entry functions.");
		goto fail_load_glx;
	}

	/* Define our FBConfig hints for GLX... */
	const int fb_attribs[] = {
		GLX_STENCIL_SIZE, get_stencil_format_bits(info->zsformat),
		GLX_DEPTH_SIZE, get_depth_format_bits(info->zsformat),
		GLX_BUFFER_SIZE, get_color_format_bits(info->format),
		GLX_DOUBLEBUFFER, true,
		GLX_X_RENDERABLE, true,
		None
	};

	/* ...fetch the best match... */
	{
		int num_configs;
		fb_config = glXChooseFBConfig(display, screen_num,
			                      fb_attribs, &num_configs);

		if (!fb_config || !num_configs) {
			blog(LOG_ERROR, "Failed to find FBConfig!");
			goto fail_fb_config;
		}
	}

	/* ...then fetch matching visual info for xcb. */
	{
		int error = glXGetFBConfigAttrib(display, fb_config[0], GLX_VISUAL_ID, &visual);

		if (error) {
			blog(LOG_ERROR, "Bad call to GetFBConfigAttrib!");
			goto fail_visual_id;
		}
	}


	xcb_colormap_t colormap = xcb_generate_id(xcb_conn);
	uint32_t mask = XCB_CW_BORDER_PIXEL | XCB_CW_COLORMAP;
	uint32_t mask_values[] = { 0, colormap, 0 };

	xcb_create_colormap(xcb_conn,
		XCB_COLORMAP_ALLOC_NONE,
		colormap,
		parent,
		visual
	);

	xcb_create_window(
		xcb_conn, 24 /* Hardcoded? */,
		wid, parent,
		0, 0,
		geometry->width,
		geometry->height,
		0, 0,
		visual, mask, mask_values
	);

	swap->wi->config = fb_config[0];
	swap->wi->window = wid;

	xcb_map_window(xcb_conn, wid);

	XFree(fb_config);
	status = true;
	goto success;

fail_visual_id:
	XFree(fb_config);
fail_fb_config:
fail_load_glx:
fail_screen:
fail_geometry_request:
success:
	free(geometry);
	return status;
}

extern void gl_platform_cleanup_swapchain(struct gs_swap_chain *swap)
{
	UNUSED_PARAMETER(swap);
	/* Really nothing to clean up? */
}

extern void device_enter_context(gs_device_t *device)
{
	GLXContext context = device->plat->context;
	XID window = device->cur_swap->wi->window;
	Display *display = device->plat->display;

	if (!glXMakeCurrent(display, window, context)) {
		blog(LOG_ERROR, "Failed to make context current.");
	}
}

extern void device_leave_context(gs_device_t *device)
{
	Display *display = device->plat->display;

	if (!glXMakeCurrent(display, None, NULL)) {
		blog(LOG_ERROR, "Failed to reset current context.");
	}
}

extern void gl_getclientsize(const struct gs_swap_chain *swap,
			     uint32_t *width, uint32_t *height)
{
	xcb_connection_t *xcb_conn = XGetXCBConnection(swap->device->plat->display);
	xcb_window_t window = swap->wi->window;

	xcb_get_geometry_reply_t *geometry = get_window_geometry(xcb_conn, window);
	*width = geometry->width;
	*height = geometry->height;

	free(geometry);
}

extern void gl_update(gs_device_t *device)
{
	Display *display = device->plat->display;
	xcb_window_t window = device->cur_swap->wi->window;

	uint32_t values[] = {
		device->cur_swap->info.cx,
		device->cur_swap->info.cy
	};

	xcb_configure_window(
		XGetXCBConnection(display), window,
		XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
		values
	);
}

extern void device_load_swapchain(gs_device_t *device, gs_swapchain_t *swap)
{
	if (!swap)
		swap = &device->plat->swap;

	if (device->cur_swap == swap)
		return;

	Display *dpy = device->plat->display;
	XID window = swap->wi->window;
	GLXContext ctx = device->plat->context;

	device->cur_swap = swap;

	if (!glXMakeCurrent(dpy, window, ctx)) {
		blog(LOG_ERROR, "Failed to make context current.");
	}
}

extern void device_present(gs_device_t *device)
{
	Display *display = device->plat->display;
	XID window = device->cur_swap->wi->window;

	/* TODO: Handle XCB events. */

	glXSwapBuffers(display, window);
}
