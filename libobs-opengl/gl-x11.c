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

#include "GL/glx_obs.h"

static const int fb_attribs[] = {
	/* Hardcoded for now... */
	GLX_STENCIL_SIZE, 8,
	GLX_DEPTH_SIZE, 24, 
	GLX_BUFFER_SIZE, 32, /* Color buffer depth */
	GLX_DOUBLEBUFFER, True,
	None
};

static const int ctx_attribs[] = {
#ifdef _DEBUG
	GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_DEBUG_BIT_ARB,
#endif
	GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
	None, 
};

struct gl_windowinfo {
	uint32_t id;
	Display *display;
};

struct gl_platform {
	GLXContext context;
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

struct gl_platform *gl_platform_create(device_t device,
		struct gs_init_data *info)
{
	int num_configs = 0;
	int error_base = 0, event_base = 0;
	Display *display = info->window.display;
	struct gl_platform *plat = bzalloc(sizeof(struct gl_platform));
	GLXFBConfig* configs;

	print_info_stuff(info);

	if (!display) {
		blog(LOG_ERROR, "Unable to find display. DISPLAY variable "
		                "may not be set correctly.");
		goto fail0;
	}

	if (!glx_LoadFunctions(display, DefaultScreen(display))) {
		blog(LOG_ERROR, "Unable to load GLX entry functions.");
		goto fail0;
	}

	if (!glXQueryExtension(display, &error_base, &event_base)) {
		blog(LOG_ERROR, "GLX not supported.");
		goto fail0;
	}

	/* We require glX version 1.3 */
	{
		int major = 0, minor = 0;
		
		glXQueryVersion(display, &major, &minor);
		if (major < 1 || (major == 1 && minor < 3)) {
			blog(LOG_ERROR, "GLX version found: %i.%i\nRequired: "
			                "1.3", major, minor);
			goto fail0;
		}
	}

	if (!glx_ext_ARB_create_context) {
		blog(LOG_ERROR, "ARB_GLX_create_context not supported!");
		goto fail0;
	}

	configs = glXChooseFBConfig(display, DefaultScreen(display),
			fb_attribs, &num_configs);

	if(!configs) {
		blog(LOG_ERROR, "Attribute list or screen is invalid.");
		goto fail0;
	}

	if(num_configs == 0) {
		blog(LOG_ERROR, "No framebuffer configurations found.");
		goto fail1;
	}

	/* We just use the first configuration found... as it does everything
	 * we want at the very least. */
	plat->context = glXCreateContextAttribsARB(display, configs[0], NULL,
			True, ctx_attribs);
	if(!plat->context) {
		blog(LOG_ERROR, "Failed to create OpenGL context.");
		goto fail1;
	}

	if(!glXMakeCurrent(display, info->window.id, plat->context)) {
		blog(LOG_ERROR, "Failed to make context current.");
		goto fail2;
	}

	if (!ogl_LoadFunctions()) {
		blog(LOG_ERROR, "Failed to load OpenGL entry functions.");
		goto fail2;
	}

	blog(LOG_INFO, "OpenGL version: %s\n", glGetString(GL_VERSION));

	plat->swap.device = device;
	plat->swap.info	  = *info;
	plat->swap.wi     = gl_windowinfo_create(info);

	device->cur_swap = &plat->swap; /* We assume later that cur_swap is already set. */

	XFree(configs);
	XSync(display, False);

	return plat;

fail2:
	glXMakeCurrent(display, None, NULL);
	glXDestroyContext(display, plat->context);
fail1:
	XFree(configs);
fail0:
	bfree(plat);
	return NULL;
}

void gl_platform_destroy(struct gl_platform *platform)
{
	if (!platform)
		return;

	Display *dpy = platform->swap.wi->display;

	glXMakeCurrent(dpy, None, NULL);
	glXDestroyContext(dpy, platform->context);
	gl_windowinfo_destroy(platform->swap.wi);
	bfree(platform);
}

void device_entercontext(device_t device)
{
	GLXContext context = device->plat->context;
	XID window = device->cur_swap->wi->id;
	Display *display = device->cur_swap->wi->display;

	if (!glXMakeCurrent(display, window, context)) {
		blog(LOG_ERROR, "Failed to make context current.");
	}
}

void device_leavecontext(device_t device)
{
	Display *display = device->cur_swap->wi->display;

	if(!glXMakeCurrent(display, None, NULL)) {
		blog(LOG_ERROR, "Failed to reset current context.");
	}
}

void gl_update(device_t device)
{
	/* I don't know of anything we can do here. */
	UNUSED_PARAMETER(device);
}

void device_load_swapchain(device_t device, swapchain_t swap)
{
	if(!swap)
		swap = &device->plat->swap;

	if (device->cur_swap == swap)
		return;

	Display *dpy = swap->wi->display;
	XID window = swap->wi->id;
	GLXContext ctx = device->plat->context;

	device->cur_swap = swap;

	if (!glXMakeCurrent(dpy, window, ctx)) {
		blog(LOG_ERROR, "Failed to make context current.");
	}
}

void device_present(device_t device)
{
	Display *display = device->cur_swap->wi->display;
	XID window = device->cur_swap->wi->id;

	glXSwapBuffers(display, window);
}
