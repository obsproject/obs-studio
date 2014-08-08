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

#include <X11/Xlib.h>

#include <stdio.h>

#include "gl-subsystem.h"

#include <glad/glad_glx.h>

static const int fb_attribs[] = {
	/* Hardcoded for now... */
	GLX_STENCIL_SIZE, 8,
	GLX_DEPTH_SIZE, 24,
	GLX_BUFFER_SIZE, 32, /* Color buffer depth */
	GLX_DOUBLEBUFFER, true,
	GLX_X_RENDERABLE, true,
	GLX_RENDER_TYPE, GLX_RGBA_BIT,
	None
};

static const int ctx_attribs[] = {
#ifdef _DEBUG
	GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_DEBUG_BIT_ARB,
#endif
	GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
	GLX_CONTEXT_MAJOR_VERSION_ARB, 3, GLX_CONTEXT_MINOR_VERSION_ARB, 2,
	None,
};

struct gl_windowinfo {
	Display *display;
	uint32_t id;
	uint32_t int_id;
	uint32_t glxid;
};

struct gl_platform {
	GLXContext context;
	GLXFBConfig fbcfg;
	struct gs_swap_chain swap;
};

extern struct gs_swap_chain *gl_platform_getswap(struct gl_platform *platform)
{
	return &platform->swap;
}

extern struct gl_windowinfo *gl_windowinfo_create(struct gs_init_data *info)
{
	struct gl_windowinfo *wi = bzalloc(sizeof(struct gl_windowinfo));
	wi->id = info->window.id;
	wi->display = info->window.display;

	return wi;
}

extern void gl_windowinfo_destroy(struct gl_windowinfo *wi)
{
	bfree(wi);
}

extern void gl_getclientsize(struct gs_swap_chain *swap,
			     uint32_t *width, uint32_t *height)
{
	XWindowAttributes info = { 0 };

	XGetWindowAttributes(swap->wi->display, swap->wi->id, &info);

	*height = info.height;
	*width = info.width;
}

static void print_info_stuff(struct gs_init_data *info)
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

int wait_for_notify(Display *dpy, XEvent *e, char *arg)
{
	UNUSED_PARAMETER(dpy);
	return (e->type == MapNotify) && (e->xmap.window == (Window)arg);
}

static bool got_x_error = false;
static int err_handler(Display *disp, XErrorEvent *e)
{
	char estr[128];
	XGetErrorText(disp, e->error_code, estr, 128);

	blog(LOG_DEBUG, "Got X error: %s", estr);

	got_x_error = true;

	return 0;
}

static bool handle_x_error(Display *disp, const char *error_string)
{
	XSync(disp, 0);

	if (got_x_error) {
		if (error_string)
			blog(LOG_ERROR, "%s", error_string);

		got_x_error = false;
		return true;
	}

	return false;
}

struct gl_platform *gl_platform_create(gs_device_t device,
		struct gs_init_data *info)
{
	int num_configs = 0;
	int error_base = 0, event_base = 0;
	Display *display = info->window.display;
	struct gl_platform *plat = bzalloc(sizeof(struct gl_platform));
	GLXFBConfig* configs;
	XWindowAttributes attrs;
	int screen;
	int major = 0, minor = 0;

	print_info_stuff(info);

	if (!display) {
		blog(LOG_ERROR, "Unable to find display. DISPLAY variable "
		                "may not be set correctly.");
		goto fail0;
	}

	if (!XGetWindowAttributes(display, info->window.id, &attrs)) {
		blog(LOG_ERROR, "Failed getting window attributes");
		goto fail0;
	}

	screen = XScreenNumberOfScreen(attrs.screen);

	if (!gladLoadGLX(display, screen)) {
		blog(LOG_ERROR, "Unable to load GLX entry functions.");
		goto fail0;
	}

	if (!glXQueryExtension(display, &error_base, &event_base)) {
		blog(LOG_ERROR, "GLX not supported.");
		goto fail0;
	}

	/* We require glX version 1.3 */

	glXQueryVersion(display, &major, &minor);
	if (major < 1 || (major == 1 && minor < 3)) {
		blog(LOG_ERROR, "GLX version found: %i.%i\nRequired: "
				"1.3", major, minor);
		goto fail0;
	}

	if (!GLAD_GLX_ARB_create_context) {
		blog(LOG_ERROR, "ARB_GLX_create_context not supported!");
		goto fail0;
	}

	configs = glXChooseFBConfig(display, screen,
			fb_attribs, &num_configs);

	if (!configs) {
		blog(LOG_ERROR, "Attribute list or screen is invalid.");
		goto fail0;
	}

	if (num_configs == 0) {
		XFree(configs);
		blog(LOG_ERROR, "No framebuffer configurations found.");
		goto fail0;
	}

	plat->fbcfg = configs[0];

	XFree(configs);

	handle_x_error(display, NULL);

	/* We just use the first configuration found... as it does everything
	 * we want at the very least. */
	plat->context = glXCreateContextAttribsARB(display, plat->fbcfg, NULL,
			true, ctx_attribs);
	if (!plat->context) {
		blog(LOG_ERROR, "Failed to create OpenGL context.");
		goto fail0;
	}

	if (handle_x_error(display, "Failed to create OpenGL context."))
		goto fail2;

	device->plat = plat;

	plat->swap.device               = device;
	plat->swap.info.window.id       = info->window.id;
	plat->swap.info.window.display  = display;
	plat->swap.info.format          = GS_RGBA;
	plat->swap.info.zsformat        = GS_Z24_S8;
	plat->swap.info.num_backbuffers = 1;
	plat->swap.info.adapter         = info->adapter;
	plat->swap.info.cx              = attrs.width;
	plat->swap.info.cy              = attrs.height;
	plat->swap.wi                   = gl_windowinfo_create(info);

	if (!gl_platform_init_swapchain(&plat->swap))
		goto fail2;

	if (!glXMakeCurrent(display, plat->swap.wi->glxid, plat->context)) {
		blog(LOG_ERROR, "Failed to make context current.");
		goto fail2;
	}

	if (!gladLoadGL()) {
		blog(LOG_ERROR, "Failed to load OpenGL entry functions.");
		goto fail2;
	}

	blog(LOG_INFO, "OpenGL version: %s\n", glGetString(GL_VERSION));

	/* We assume later that cur_swap is already set. */
	device->cur_swap = &plat->swap;

	XSync(display, false);

	blog(LOG_INFO, "Created new platform data");

	return plat;

fail2:
	glXMakeCurrent(display, None, NULL);
	glXDestroyContext(display, plat->context);

	gl_platform_cleanup_swapchain(&plat->swap);

fail0:
	bfree(plat);
	device->plat = 0;

	return NULL;
}

void gl_platform_destroy(struct gl_platform *platform)
{
	if (!platform)
		return;

	Display *dpy = platform->swap.wi->display;

	glXMakeCurrent(dpy, None, NULL);
	gl_platform_cleanup_swapchain(&platform->swap);
	glXDestroyContext(dpy, platform->context);
	gl_windowinfo_destroy(platform->swap.wi);
	bfree(platform);
}

bool gl_platform_init_swapchain(struct gs_swap_chain *swap)
{
	Display *display = swap->wi->display;
	struct gl_windowinfo *info = swap->wi;
	struct gl_platform *plat = swap->device->plat;
	XVisualInfo *vi = 0;
	Colormap cmap = 0;
	XSetWindowAttributes swa;
	XWindowAttributes attrs;

	XErrorHandler phandler = XSetErrorHandler(err_handler);

	gl_platform_cleanup_swapchain(swap);

	if (!XGetWindowAttributes(display, info->id, &attrs)) {
		blog(LOG_ERROR, "Failed getting window attributes");
		goto fail;
	}

	vi = glXGetVisualFromFBConfig(display, plat->fbcfg);

	if (handle_x_error(display, "Failed to get visual from fb config."))
		goto fail;

	cmap = XCreateColormap(display, info->id, vi->visual, AllocNone);

	if (handle_x_error(display, "Failed creating colormap"))
		goto fail;

	swa.colormap = cmap;
	swa.border_pixel = 0;

	info->int_id = XCreateWindow(display, info->id, 0, 0,
	                             attrs.width, attrs.height,
	                             0, 24, InputOutput, vi->visual,
	                             CWBorderPixel|CWColormap, &swa);
	XMapWindow(display, info->int_id);

	if (handle_x_error(display, "Failed creating intermediate X window"))
		goto fail;

	info->glxid = glXCreateWindow(display, plat->fbcfg, info->int_id, 0);

	if (handle_x_error(display, "Failed creating intermediate GLX window"))
		goto fail;

	XFreeColormap(display, cmap);
	XFree(vi);

	return true;

fail:

	gl_platform_cleanup_swapchain(swap);

	if (cmap)
		XFreeColormap(display, cmap);

	if (vi)
		XFree(vi);

	XSetErrorHandler(phandler);

	return false;
}

void gl_platform_cleanup_swapchain(struct gs_swap_chain *swap)
{
	Display *display = swap->wi->display;
	struct gl_windowinfo *info = swap->wi;

	if (!info)
		return;

	if (info->glxid)
		glXDestroyWindow(display, info->glxid);

	if (info->int_id)
		XDestroyWindow(display, info->int_id);

	info->glxid = 0;
	info->int_id = 0;
}

void device_enter_context(gs_device_t device)
{
	GLXContext context = device->plat->context;
	XID window = device->cur_swap->wi->glxid;
	Display *display = device->cur_swap->wi->display;

	if (!glXMakeCurrent(display, window, context)) {
		blog(LOG_ERROR, "Failed to make context current.");
	}
}

void device_leave_context(gs_device_t device)
{
	Display *display = device->cur_swap->wi->display;

	if (!glXMakeCurrent(display, None, NULL)) {
		blog(LOG_ERROR, "Failed to reset current context.");
	}
}

void gl_update(gs_device_t device)
{
	Display *display = device->cur_swap->wi->display;
	XID window = device->cur_swap->wi->int_id;

	XResizeWindow(display, window,
			device->cur_swap->info.cx, device->cur_swap->info.cy);
}

void device_load_swapchain(gs_device_t device, gs_swapchain_t swap)
{
	if (!swap)
		swap = &device->plat->swap;

	if (device->cur_swap == swap)
		return;

	Display *dpy = swap->wi->display;
	XID window = swap->wi->glxid;
	GLXContext ctx = device->plat->context;

	device->cur_swap = swap;

	if (!glXMakeCurrent(dpy, window, ctx)) {
		blog(LOG_ERROR, "Failed to make context current.");
	}
}

void device_present(gs_device_t device)
{
	Display *display = device->cur_swap->wi->display;
	XID window = device->cur_swap->wi->glxid;

	glXSwapBuffers(display, window);
}
