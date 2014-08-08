/*
Copyright (C) 2014 by Leonhard Oelke <leonhard@in-verted.de>

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
*/

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <obs-module.h>
#include "xcursor.h"
#include "xhelpers.h"

#define XSHM_DATA(voidptr) struct xshm_data *data = voidptr;

struct xshm_data {
	Display *dpy;
	Screen *screen;

	int_fast32_t x_org, y_org;
	int_fast32_t width, height;

	xshm_t *xshm;
	gs_texture_t texture;

	bool show_cursor;
	xcursor_t *cursor;

	bool use_xinerama;
};

/**
 * Resize the texture
 *
 * This will automatically create the texture if it does not exist
 */
static void xshm_resize_texture(struct xshm_data *data)
{
	obs_enter_graphics();

	if (data->texture)
		gs_texture_destroy(data->texture);
	data->texture = gs_texture_create(data->width, data->height,
		GS_BGRA, 1, NULL, GS_DYNAMIC);

	obs_leave_graphics();
}

/**
 * Update the capture
 *
 * @return < 0 on error, 0 when size is unchanged, > 1 on size change
 */
static int_fast32_t xshm_update_geometry(struct xshm_data *data,
	obs_data_t settings)
{
	int_fast32_t old_width = data->width;
	int_fast32_t old_height = data->height;
	int_fast32_t screen = obs_data_get_int(settings, "screen");

	if (data->use_xinerama) {
		if (xinerama_screen_geo(data->dpy, screen,
			&data->x_org, &data->y_org,
			&data->width, &data->height) < 0) {
			return -1;
		}
		data->screen = XDefaultScreenOfDisplay(data->dpy);
	}
	else {
		data->x_org = 0;
		data->y_org = 0;
		if (x11_screen_geo(data->dpy, screen,
			&data->width, &data->height) < 0) {
			return -1;
		}
		data->screen = XScreenOfDisplay(data->dpy, screen);
	}

	if (!data->width || !data->height) {
		blog(LOG_ERROR, "xshm-input: Failed to get geometry");
		return -1;
	}

	blog(LOG_INFO, "xshm-input: Geometry %"PRIdFAST32"x%"PRIdFAST32
		" @ %"PRIdFAST32",%"PRIdFAST32,
		data->width, data->height, data->x_org, data->y_org);

	if (old_width == data->width && old_height == data->height)
		return 0;

	return 1;
}

/**
 * Returns the name of the plugin
 */
static const char* xshm_getname(void)
{
	return obs_module_text("X11SharedMemoryScreenInput");
}

/**
 * Update the capture with changed settings
 */
static void xshm_update(void *vptr, obs_data_t settings)
{
	XSHM_DATA(vptr);

	data->show_cursor = obs_data_get_bool(settings, "show_cursor");

	if (data->xshm)
		xshm_detach(data->xshm);

	if (xshm_update_geometry(data, settings) < 0) {
		blog(LOG_ERROR, "xshm-input: failed to update geometry !");
		return;
	}

	xshm_resize_texture(data);
	xcursor_offset(data->cursor, data->x_org, data->y_org);

	data->xshm = xshm_attach(data->dpy, data->screen,
		data->width, data->height);
	if (!data->xshm) {
		blog(LOG_ERROR, "xshm-input: failed to attach shm !");
		return;
	}
}

/**
 * Get the default settings for the capture
 */
static void xshm_defaults(obs_data_t defaults)
{
	obs_data_set_default_int(defaults, "screen", 0);
	obs_data_set_default_bool(defaults, "show_cursor", true);
}

/**
 * Get the properties for the capture
 */
static obs_properties_t xshm_properties(void)
{
	obs_properties_t props = obs_properties_create();
	int_fast32_t screen_max;

	Display *dpy = XOpenDisplay(NULL);
	screen_max = xinerama_is_active(dpy)
		? xinerama_screen_count(dpy)
		: XScreenCount(dpy);
	screen_max = (screen_max) ? screen_max - 1 : 0;
	XCloseDisplay(dpy);

	obs_properties_add_int(props, "screen",
			obs_module_text("Screen"), 0, screen_max, 1);
	obs_properties_add_bool(props, "show_cursor",
			obs_module_text("CaptureCursor"));

	return props;
}

/**
 * Destroy the capture
 */
static void xshm_destroy(void *vptr)
{
	XSHM_DATA(vptr);

	if (!data)
		return;

	obs_enter_graphics();

	if (data->texture)
		gs_texture_destroy(data->texture);
	if (data->cursor)
		xcursor_destroy(data->cursor);

	obs_leave_graphics();

	if (data->xshm)
		xshm_detach(data->xshm);
	if (data->dpy)
		XCloseDisplay(data->dpy);

	bfree(data);
}

/**
 * Create the capture
 */
static void *xshm_create(obs_data_t settings, obs_source_t source)
{
	UNUSED_PARAMETER(source);

	struct xshm_data *data = bzalloc(sizeof(struct xshm_data));

	data->dpy = XOpenDisplay(NULL);
	if (!data->dpy) {
		blog(LOG_ERROR, "xshm-input: Unable to open X display !");
		goto fail;
	}

	if (!XShmQueryExtension(data->dpy)) {
		blog(LOG_ERROR, "xshm-input: XShm extension not found !");
		goto fail;
	}

	data->use_xinerama = xinerama_is_active(data->dpy) ? true : false;

	obs_enter_graphics();
	data->cursor = xcursor_init(data->dpy);
	obs_leave_graphics();

	xshm_update(data, settings);

	return data;
fail:
	xshm_destroy(data);
	return NULL;
}

/**
 * Prepare the capture data
 */
static void xshm_video_tick(void *vptr, float seconds)
{
	UNUSED_PARAMETER(seconds);
	XSHM_DATA(vptr);

	if (!data->xshm)
		return;

	obs_enter_graphics();

	XShmGetImage(data->dpy, XRootWindowOfScreen(data->screen),
		data->xshm->image, data->x_org, data->y_org, AllPlanes);
	gs_texture_set_image(data->texture, (void *) data->xshm->image->data,
		data->width * 4, false);

	xcursor_tick(data->cursor);

	obs_leave_graphics();
}

/**
 * Render the capture data
 */
static void xshm_video_render(void *vptr, gs_effect_t effect)
{
	XSHM_DATA(vptr);

	if (!data->xshm)
		return;

	gs_eparam_t image = gs_effect_get_param_by_name(effect, "image");
	gs_effect_set_texture(image, data->texture);

	gs_enable_blending(false);
	gs_draw_sprite(data->texture, 0, 0, 0);

	if (data->show_cursor)
		xcursor_render(data->cursor);

	gs_reset_blend_state();
}

/**
 * Width of the captured data
 */
static uint32_t xshm_getwidth(void *vptr)
{
	XSHM_DATA(vptr);
	return data->width;
}

/**
 * Height of the captured data
 */
static uint32_t xshm_getheight(void *vptr)
{
	XSHM_DATA(vptr);
	return data->height;
}

struct obs_source_info xshm_input = {
	.id             = "xshm_input",
	.type           = OBS_SOURCE_TYPE_INPUT,
	.output_flags   = OBS_SOURCE_VIDEO,
	.get_name       = xshm_getname,
	.create         = xshm_create,
	.destroy        = xshm_destroy,
	.update         = xshm_update,
	.get_defaults   = xshm_defaults,
	.get_properties = xshm_properties,
	.video_tick     = xshm_video_tick,
	.video_render   = xshm_video_render,
	.get_width      = xshm_getwidth,
	.get_height     = xshm_getheight
};
