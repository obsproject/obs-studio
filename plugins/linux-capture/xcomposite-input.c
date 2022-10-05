#include <obs-module.h>
#include <obs-nix-platform.h>
#include <glad/glad.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xlib-xcb.h>
#include <xcb/xcb.h>
#include <xcb/composite.h>
#include <pthread.h>

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>

#include "xhelpers.h"
#include "xcursor-xcb.h"
#include "xcomposite-input.h"
#include <util/platform.h>
#include <util/dstr.h>
#include <util/darray.h>

#define WIN_STRING_DIV "\r\n"
#define FIND_WINDOW_INTERVAL 0.5

static Display *disp = NULL;
static xcb_connection_t *conn = NULL;
// Atoms used throughout our plugin
xcb_atom_t ATOM_UTF8_STRING;
xcb_atom_t ATOM_STRING;
xcb_atom_t ATOM_TEXT;
xcb_atom_t ATOM_COMPOUND_TEXT;
xcb_atom_t ATOM_WM_NAME;
xcb_atom_t ATOM_WM_CLASS;
xcb_atom_t ATOM__NET_WM_NAME;
xcb_atom_t ATOM__NET_SUPPORTING_WM_CHECK;
xcb_atom_t ATOM__NET_CLIENT_LIST;

struct xcompcap {
	obs_source_t *source;

	const char *windowName;
	xcb_window_t win;
	int crop_top;
	int crop_left;
	int crop_right;
	int crop_bot;
	bool swapRedBlue;
	bool include_border;
	bool exclude_alpha;

	float window_check_time;
	bool window_changed;

	uint32_t width;
	uint32_t height;
	uint32_t border;

	Pixmap pixmap;
	gs_texture_t *gltex;

	pthread_mutex_t lock;

	bool show_cursor;
	bool cursor_outside;
	xcb_xcursor_t *cursor;
};

static void xcompcap_update(void *data, obs_data_t *settings);

xcb_atom_t get_atom(xcb_connection_t *conn, const char *name)
{
	xcb_intern_atom_cookie_t atom_c =
		xcb_intern_atom(conn, 1, strlen(name), name);
	xcb_intern_atom_reply_t *atom_r =
		xcb_intern_atom_reply(conn, atom_c, NULL);
	xcb_atom_t a = atom_r->atom;
	free(atom_r);
	return a;
}

void xcomp_gather_atoms(xcb_connection_t *conn)
{
	ATOM_UTF8_STRING = get_atom(conn, "UTF8_STRING");
	ATOM_STRING = get_atom(conn, "STRING");
	ATOM_TEXT = get_atom(conn, "TEXT");
	ATOM_COMPOUND_TEXT = get_atom(conn, "COMPOUND_TEXT");
	ATOM_WM_NAME = get_atom(conn, "WM_NAME");
	ATOM_WM_CLASS = get_atom(conn, "WM_CLASS");
	ATOM__NET_WM_NAME = get_atom(conn, "_NET_WM_NAME");
	ATOM__NET_SUPPORTING_WM_CHECK =
		get_atom(conn, "_NET_SUPPORTING_WM_CHECK");
	ATOM__NET_CLIENT_LIST = get_atom(conn, "_NET_CLIENT_LIST");
}

bool xcomp_window_exists(xcb_connection_t *conn, xcb_window_t win)
{
	xcb_generic_error_t *err = NULL;
	xcb_get_window_attributes_cookie_t attr_cookie =
		xcb_get_window_attributes(conn, win);
	xcb_get_window_attributes_reply_t *attr =
		xcb_get_window_attributes_reply(conn, attr_cookie, &err);

	bool exists = err == NULL && attr->map_state == XCB_MAP_STATE_VIEWABLE;
	free(attr);
	return exists;
}

xcb_get_property_reply_t *xcomp_property_sync(xcb_connection_t *conn,
					      xcb_window_t win, xcb_atom_t atom)
{
	if (atom == XCB_ATOM_NONE)
		return NULL;

	xcb_generic_error_t *err = NULL;
	// Read properties up to 4096*4 bytes
	xcb_get_property_cookie_t prop_cookie =
		xcb_get_property(conn, 0, win, atom, 0, 0, 4096);
	xcb_get_property_reply_t *prop =
		xcb_get_property_reply(conn, prop_cookie, &err);
	if (err != NULL || xcb_get_property_value_length(prop) == 0) {
		free(prop);
		return NULL;
	}

	return prop;
}

// See ICCCM https://www.x.org/releases/X11R7.6/doc/xorg-docs/specs/ICCCM/icccm.html#text_properties
// for more info on the galactic brained string types used in Xorg.
struct dstr xcomp_window_name(xcb_connection_t *conn, Display *disp,
			      xcb_window_t win)
{
	struct dstr ret = {0};

	xcb_get_property_reply_t *name =
		xcomp_property_sync(conn, win, ATOM__NET_WM_NAME);
	if (name) {
		// Guaranteed to be UTF8_STRING.
		const char *data = (const char *)xcb_get_property_value(name);
		dstr_ncopy(&ret, data, xcb_get_property_value_length(name));
		free(name);
		return ret;
	}

	name = xcomp_property_sync(conn, win, ATOM_WM_NAME);
	if (!name) {
		goto fail;
	}

	char *data = (char *)xcb_get_property_value(name);
	if (name->type == ATOM_UTF8_STRING) {
		dstr_ncopy(&ret, data, xcb_get_property_value_length(name));
		free(name);
		return ret;
	}
	if (name->type == ATOM_STRING) { // Latin-1, safe enough
		dstr_ncopy(&ret, data, xcb_get_property_value_length(name));
		free(name);
		return ret;
	}
	if (name->type == ATOM_TEXT) { // Default charset
		char *utf8;
		if (!os_mbs_to_utf8_ptr(
			    data, xcb_get_property_value_length(name), &utf8)) {
			free(name);
			goto fail;
		}
		free(name);
		dstr_init_move_array(&ret, utf8);
		return ret;
	}
	if (name->type ==
	    ATOM_COMPOUND_TEXT) { // LibX11 is the only decoder for these.
		XTextProperty xname = {
			(unsigned char *)data, name->type,
			8, // 8 by definition.
			1, // Only decode the first element of string arrays.
		};
		char **list;
		int len = 0;
		if (XmbTextPropertyToTextList(disp, &xname, &list, &len) <
			    Success ||
		    !list || len < 1) {
			free(name);
			goto fail;
		}
		char *utf8;
		if (!os_mbs_to_utf8_ptr(list[0], 0, &utf8)) {
			XFreeStringList(list);
			free(name);
			goto fail;
		}
		dstr_init_move_array(&ret, utf8);
		XFreeStringList(list);
		free(name);
		return ret;
	}

fail:
	dstr_copy(&ret, "unknown");
	return ret;
}

struct dstr xcomp_window_class(xcb_connection_t *conn, xcb_window_t win)
{
	struct dstr ret = {0};
	dstr_copy(&ret, "unknown");
	xcb_get_property_reply_t *cls =
		xcomp_property_sync(conn, win, ATOM_WM_CLASS);
	if (!cls)
		return ret;

	// WM_CLASS is formatted differently from other strings, it's two null terminated strings.
	// Since we want the first one, let's just let copy run strlen.
	dstr_copy(&ret, (const char *)xcb_get_property_value(cls));
	free(cls);
	return ret;
}

// Specification for checking for ewmh support at
// http://standards.freedesktop.org/wm-spec/wm-spec-latest.html#idm140200472693600
bool xcomp_check_ewmh(xcb_connection_t *conn, xcb_window_t root)
{
	xcb_get_property_reply_t *check =
		xcomp_property_sync(conn, root, ATOM__NET_SUPPORTING_WM_CHECK);
	if (!check)
		return false;

	xcb_window_t ewmh_window =
		((xcb_window_t *)xcb_get_property_value(check))[0];
	free(check);

	xcb_get_property_reply_t *check2 = xcomp_property_sync(
		conn, ewmh_window, ATOM__NET_SUPPORTING_WM_CHECK);
	if (!check2)
		return false;
	free(check2);

	return true;
}

struct darray xcomp_top_level_windows(xcb_connection_t *conn)
{
	DARRAY(xcb_window_t) res = {0};

	// EWMH top level window listing is not supported.
	if (ATOM__NET_CLIENT_LIST == XCB_ATOM_NONE)
		return res.da;

	xcb_screen_iterator_t screen_iter =
		xcb_setup_roots_iterator(xcb_get_setup(conn));
	for (; screen_iter.rem > 0; xcb_screen_next(&screen_iter)) {
		xcb_generic_error_t *err = NULL;
		// Read properties up to 4096*4 bytes
		xcb_get_property_cookie_t cl_list_cookie =
			xcb_get_property(conn, 0, screen_iter.data->root,
					 ATOM__NET_CLIENT_LIST, 0, 0, 4096);
		xcb_get_property_reply_t *cl_list =
			xcb_get_property_reply(conn, cl_list_cookie, &err);
		if (err != NULL) {
			goto done;
		}

		uint32_t len = xcb_get_property_value_length(cl_list) /
			       sizeof(xcb_window_t);
		for (uint32_t i = 0; i < len; i++)
			da_push_back(res,
				     &(((xcb_window_t *)xcb_get_property_value(
					     cl_list))[i]));

	done:
		free(cl_list);
	}

	return res.da;
}

xcb_window_t xcomp_find_window(xcb_connection_t *conn, Display *disp,
			       const char *str)
{
	xcb_window_t ret = 0;
	DARRAY(xcb_window_t) tlw = {0};
	tlw.da = xcomp_top_level_windows(conn);
	if (!str || strlen(str) == 0) {
		if (tlw.num > 0)
			ret = *(xcb_window_t *)darray_item(sizeof(xcb_window_t),
							   &tlw.da, 0);
		goto cleanup1;
	}

	size_t markSize = strlen(WIN_STRING_DIV);

	const char *firstMark = strstr(str, WIN_STRING_DIV);
	const char *secondMark =
		firstMark ? strstr(firstMark + markSize, WIN_STRING_DIV) : NULL;
	const char *strEnd = str + strlen(str);

	const char *secondStr = firstMark + markSize;
	const char *thirdStr = secondMark + markSize;

	// wstr only consists of the window-id
	if (!firstMark)
		return (xcb_window_t)atol(str);

	// wstr also contains window-name and window-class
	char *wname = bzalloc(secondMark - secondStr + 1);
	char *wcls = bzalloc(strEnd - thirdStr + 1);
	memcpy(wname, secondStr, secondMark - secondStr);
	memcpy(wcls, thirdStr, strEnd - thirdStr);
	ret = (xcb_window_t)strtol(str, NULL, 10);

	// first try to find a match by the window-id
	if (da_find(tlw, &ret, 0) != DARRAY_INVALID) {
		goto cleanup2;
	}

	// then try to find a match by name & class
	for (size_t i = 0; i < tlw.num; i++) {
		xcb_window_t cwin = *(xcb_window_t *)darray_item(
			sizeof(xcb_window_t), &tlw.da, i);

		struct dstr cwname = xcomp_window_name(conn, disp, cwin);
		struct dstr cwcls = xcomp_window_class(conn, cwin);
		bool found = strcmp(wname, cwname.array) == 0 &&
			     strcmp(wcls, cwcls.array) == 0;

		dstr_free(&cwname);
		dstr_free(&cwcls);
		if (found) {
			ret = cwin;
			goto cleanup2;
		}
	}

	blog(LOG_DEBUG,
	     "Did not find new window id for Name '%s' or Class '%s'", wname,
	     wcls);

cleanup2:
	bfree(wname);
	bfree(wcls);
cleanup1:
	da_free(tlw);
	return ret;
}

void xcomp_cleanup_pixmap(Display *disp, struct xcompcap *s)
{
	if (s->gltex) {
		gs_texture_destroy(s->gltex);
		s->gltex = 0;
	}

	if (s->pixmap) {
		XFreePixmap(disp, s->pixmap);
		s->pixmap = 0;
	}
}

static enum gs_color_format gs_format_from_tex()
{
	GLint iformat = 0;
	// consider GL_ARB_internalformat_query
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT,
				 &iformat);

	// These formats are known to be wrong on Intel platforms. We intentionally
	// use swapped internal formats here to preserve historic behavior which
	// swapped colors accidentally and because D3D11 would not support a
	// GS_RGBX format.
	switch (iformat) {
	case GL_RGB:
		return GS_BGRX_UNORM;
	case GL_RGBA:
		return GS_RGBA_UNORM;
	default:
		return GS_RGBA_UNORM;
	}
}

static int silence_x11_errors(Display *display, XErrorEvent *err)
{
	UNUSED_PARAMETER(display);
	UNUSED_PARAMETER(err);

	return 0;
}

void xcomp_create_pixmap(xcb_connection_t *conn, struct xcompcap *s,
			 int log_level)
{
	if (!s->win)
		return;

	xcb_generic_error_t *err = NULL;
	xcb_get_geometry_cookie_t geom_cookie = xcb_get_geometry(conn, s->win);
	xcb_get_geometry_reply_t *geom =
		xcb_get_geometry_reply(conn, geom_cookie, &err);
	if (err != NULL) {
		return;
	}

	s->border = s->include_border ? geom->border_width : 0;
	s->width = geom->width;
	s->height = geom->height;
	// We don't have an alpha channel, but may have garbage in the texture.
	int32_t depth = geom->depth;
	if (depth != 32) {
		s->exclude_alpha = true;
	}
	xcb_window_t root = geom->root;
	free(geom);

	uint32_t vert_borders = s->crop_top + s->crop_bot + 2 * s->border;
	uint32_t hori_borders = s->crop_left + s->crop_right + 2 * s->border;
	// Skip 0 sized textures.
	if (vert_borders > s->height || hori_borders > s->width)
		return;

	s->pixmap = xcb_generate_id(conn);
	xcb_void_cookie_t name_cookie =
		xcb_composite_name_window_pixmap_checked(conn, s->win,
							 s->pixmap);
	err = NULL;
	if ((err = xcb_request_check(conn, name_cookie)) != NULL) {
		blog(log_level, "xcb_composite_name_window_pixmap failed");
		s->pixmap = 0;
		return;
	}

	XErrorHandler prev = XSetErrorHandler(silence_x11_errors);
	s->gltex = gs_texture_create_from_pixmap(s->width, s->height,
						 GS_BGRA_UNORM, GL_TEXTURE_2D,
						 (void *)s->pixmap);
	XSetErrorHandler(prev);
}

struct reg_item {
	struct xcompcap *src;
	xcb_window_t win;
};
static DARRAY(struct reg_item) watcher_registry = {0};
static pthread_mutex_t watcher_lock = PTHREAD_MUTEX_INITIALIZER;

void watcher_register(xcb_connection_t *conn, struct xcompcap *s)
{
	pthread_mutex_lock(&watcher_lock);

	if (xcomp_window_exists(conn, s->win)) {
		// Subscribe to Events
		uint32_t vals[1] = {StructureNotifyMask | ExposureMask |
				    VisibilityChangeMask};
		xcb_change_window_attributes(conn, s->win, XCB_CW_EVENT_MASK,
					     vals);
		xcb_composite_redirect_window(conn, s->win,
					      XCB_COMPOSITE_REDIRECT_AUTOMATIC);

		da_push_back(watcher_registry, (&(struct reg_item){s, s->win}));
	}

	pthread_mutex_unlock(&watcher_lock);
}

void watcher_unregister(xcb_connection_t *conn, struct xcompcap *s)
{
	pthread_mutex_lock(&watcher_lock);
	size_t idx = DARRAY_INVALID;
	xcb_window_t win = 0;
	for (size_t i = 0; i < watcher_registry.num; i++) {
		struct reg_item *item = (struct reg_item *)darray_item(
			sizeof(struct reg_item), &watcher_registry.da, i);

		if (item->src == s) {
			idx = i;
			win = item->win;
			break;
		}
	}
	if (idx == DARRAY_INVALID)
		goto done;

	da_erase(watcher_registry, idx);

	// Check if there are still sources listening for the same window.
	bool windowInUse = false;
	for (size_t i = 0; i < watcher_registry.num; i++) {
		struct reg_item *item = (struct reg_item *)darray_item(
			sizeof(struct reg_item), &watcher_registry.da, i);

		if (item->win == win) {
			windowInUse = true;
			break;
		}
	}

	if (!windowInUse && xcomp_window_exists(conn, s->win)) {
		// Last source released, stop listening for events.
		uint32_t vals[1] = {0};
		xcb_change_window_attributes(conn, win, XCB_CW_EVENT_MASK,
					     vals);
	}

done:
	pthread_mutex_unlock(&watcher_lock);
}

void watcher_process(xcb_generic_event_t *ev)
{
	if (!ev)
		return;

	pthread_mutex_lock(&watcher_lock);
	xcb_window_t win = 0;

	switch (ev->response_type & ~0x80) {
	case XCB_CONFIGURE_NOTIFY:
		win = ((xcb_configure_notify_event_t *)ev)->event;
		break;
	case XCB_MAP_NOTIFY:
		win = ((xcb_map_notify_event_t *)ev)->event;
		break;
	case XCB_EXPOSE:
		win = ((xcb_expose_event_t *)ev)->window;
		break;
	case XCB_VISIBILITY_NOTIFY:
		win = ((xcb_visibility_notify_event_t *)ev)->window;
		break;
	case XCB_DESTROY_NOTIFY:
		win = ((xcb_destroy_notify_event_t *)ev)->event;
		break;
	};

	if (win != 0) {
		for (size_t i = 0; i < watcher_registry.num; i++) {
			struct reg_item *item = (struct reg_item *)darray_item(
				sizeof(struct reg_item), &watcher_registry.da,
				i);
			if (item->win == win) {
				item->src->window_changed = true;
			}
		}
	}

	pthread_mutex_unlock(&watcher_lock);
}

void watcher_unload()
{
	da_free(watcher_registry);
}

static uint32_t xcompcap_get_width(void *data)
{
	struct xcompcap *s = (struct xcompcap *)data;
	if (!s->gltex)
		return 0;

	int32_t border = s->crop_left + s->crop_right + 2 * s->border;
	int32_t width = s->width - border;
	return width < 0 ? 0 : width;
}

static uint32_t xcompcap_get_height(void *data)
{
	struct xcompcap *s = (struct xcompcap *)data;
	if (!s->gltex)
		return 0;

	int32_t border = s->crop_bot + s->crop_top + 2 * s->border;
	int32_t height = s->height - border;
	return height < 0 ? 0 : height;
}

static void *xcompcap_create(obs_data_t *settings, obs_source_t *source)
{
	struct xcompcap *s =
		(struct xcompcap *)bzalloc(sizeof(struct xcompcap));
	pthread_mutex_init(&s->lock, NULL);
	s->show_cursor = true;
	s->source = source;

	obs_enter_graphics();
	s->cursor = xcb_xcursor_init(conn);
	obs_leave_graphics();

	xcompcap_update(s, settings);
	return s;
}

static void xcompcap_destroy(void *data)
{
	struct xcompcap *s = (struct xcompcap *)data;

	obs_enter_graphics();
	pthread_mutex_lock(&s->lock);

	watcher_unregister(conn, s);
	xcomp_cleanup_pixmap(disp, s);

	if (s->cursor)
		xcb_xcursor_destroy(s->cursor);

	pthread_mutex_unlock(&s->lock);
	obs_leave_graphics();

	pthread_mutex_destroy(&s->lock);
	bfree(s);
}

static void xcompcap_video_tick(void *data, float seconds)
{
	struct xcompcap *s = (struct xcompcap *)data;

	if (!obs_source_showing(s->source))
		return;

	obs_enter_graphics();
	pthread_mutex_lock(&s->lock);

	xcb_generic_event_t *event;
	while ((event = xcb_poll_for_queued_event(conn)))
		watcher_process(event);

	// Reacquire window after interval or immediately if reconfigured.
	s->window_check_time += seconds;
	bool window_lost = !xcomp_window_exists(conn, s->win) || !s->gltex;
	if ((window_lost && s->window_check_time > FIND_WINDOW_INTERVAL) ||
	    s->window_changed) {
		watcher_unregister(conn, s);

		s->window_changed = false;
		s->window_check_time = 0.0;
		s->win = xcomp_find_window(conn, disp, s->windowName);

		watcher_register(conn, s);
		xcomp_cleanup_pixmap(disp, s);
		// Avoid excessive logging. We expect this to fail while windows are
		// minimized or on offscreen workspaces or already captured on NVIDIA.
		xcomp_create_pixmap(conn, s, LOG_DEBUG);
		xcb_xcursor_offset_win(conn, s->cursor, s->win);
		xcb_xcursor_offset(s->cursor, s->cursor->x_org + s->crop_left,
				   s->cursor->y_org + s->crop_top);
	}

	if (!s->gltex)
		goto done;

	if (xcompcap_get_height(s) == 0 || xcompcap_get_width(s) == 0)
		goto done;

	if (s->show_cursor) {
		xcb_xcursor_update(conn, s->cursor);

		s->cursor_outside = s->cursor->x < 0 || s->cursor->y < 0 ||
				    s->cursor->x > (int)xcompcap_get_width(s) ||
				    s->cursor->y > (int)xcompcap_get_height(s);
	}

done:
	pthread_mutex_unlock(&s->lock);
	obs_leave_graphics();
}

static void xcompcap_video_render(void *data, gs_effect_t *effect)
{
	gs_eparam_t *image; // Placate C++ goto rules.
	struct xcompcap *s = (struct xcompcap *)data;

	pthread_mutex_lock(&s->lock);

	if (!s->gltex)
		goto done;

	effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
	if (s->exclude_alpha)
		effect = obs_get_base_effect(OBS_EFFECT_OPAQUE);

	image = gs_effect_get_param_by_name(effect, "image");
	gs_effect_set_texture(image, s->gltex);

	while (gs_effect_loop(effect, "Draw")) {
		gs_draw_sprite_subregion(s->gltex, 0, s->crop_left, s->crop_top,
					 xcompcap_get_width(s),
					 xcompcap_get_height(s));
	}

	if (s->gltex && s->show_cursor && !s->cursor_outside) {
		effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);

		while (gs_effect_loop(effect, "Draw")) {
			xcb_xcursor_render(s->cursor);
		}
	}

done:
	pthread_mutex_unlock(&s->lock);
}

struct WindowInfo {
	struct dstr name_lower;
	struct dstr name;
	struct dstr desc;
};

static int cmp_wi(const void *a, const void *b)
{
	struct WindowInfo *awi = (struct WindowInfo *)a;
	struct WindowInfo *bwi = (struct WindowInfo *)b;
	return strcmp(awi->name_lower.array, bwi->name_lower.array);
}

static obs_properties_t *xcompcap_props(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *props = obs_properties_create();
	obs_property_t *prop;

	prop = obs_properties_add_list(props, "capture_window",
				       obs_module_text("Window"),
				       OBS_COMBO_TYPE_LIST,
				       OBS_COMBO_FORMAT_STRING);

	DARRAY(struct WindowInfo) window_strings = {0};
	struct darray windows = xcomp_top_level_windows(conn);
	for (size_t w = 0; w < windows.num; w++) {
		xcb_window_t win = *(xcb_window_t *)darray_item(
			sizeof(xcb_window_t), &windows, w);

		struct dstr name = xcomp_window_name(conn, disp, win);
		struct dstr cls = xcomp_window_class(conn, win);

		struct dstr desc = {0};
		dstr_printf(&desc, "%d" WIN_STRING_DIV "%s" WIN_STRING_DIV "%s",
			    win, name.array, cls.array);
		dstr_free(&cls);

		struct dstr name_lower;
		dstr_init_copy_dstr(&name_lower, &name);
		dstr_to_lower(&name_lower);

		da_push_back(window_strings,
			     (&(struct WindowInfo){name_lower, name, desc}));
	}
	darray_free(&windows);

	qsort(window_strings.array, window_strings.num,
	      sizeof(struct WindowInfo), cmp_wi);

	for (size_t i = 0; i < window_strings.num; i++) {
		struct WindowInfo *s = (struct WindowInfo *)darray_item(
			sizeof(struct WindowInfo), &window_strings.da, i);

		obs_property_list_add_string(prop, s->name.array,
					     s->desc.array);

		dstr_free(&s->name_lower);
		dstr_free(&s->name);
		dstr_free(&s->desc);
	}
	da_free(window_strings);

	prop = obs_properties_add_int(props, "cut_top",
				      obs_module_text("CropTop"), 0, 4096, 1);
	obs_property_int_set_suffix(prop, " px");

	prop = obs_properties_add_int(props, "cut_left",
				      obs_module_text("CropLeft"), 0, 4096, 1);
	obs_property_int_set_suffix(prop, " px");

	prop = obs_properties_add_int(props, "cut_right",
				      obs_module_text("CropRight"), 0, 4096, 1);
	obs_property_int_set_suffix(prop, " px");

	prop = obs_properties_add_int(
		props, "cut_bot", obs_module_text("CropBottom"), 0, 4096, 1);
	obs_property_int_set_suffix(prop, " px");

	obs_properties_add_bool(props, "swap_redblue",
				obs_module_text("SwapRedBlue"));

	obs_properties_add_bool(props, "show_cursor",
				obs_module_text("CaptureCursor"));

	obs_properties_add_bool(props, "include_border",
				obs_module_text("IncludeXBorder"));

	obs_properties_add_bool(props, "exclude_alpha",
				obs_module_text("ExcludeAlpha"));

	return props;
}

static void xcompcap_defaults(obs_data_t *settings)
{
	obs_data_set_default_string(settings, "capture_window", "");
	obs_data_set_default_int(settings, "cut_top", 0);
	obs_data_set_default_int(settings, "cut_left", 0);
	obs_data_set_default_int(settings, "cut_right", 0);
	obs_data_set_default_int(settings, "cut_bot", 0);
	obs_data_set_default_bool(settings, "swap_redblue", false);
	obs_data_set_default_bool(settings, "show_cursor", true);
	obs_data_set_default_bool(settings, "include_border", false);
	obs_data_set_default_bool(settings, "exclude_alpha", false);
}

static void xcompcap_update(void *data, obs_data_t *settings)
{
	struct xcompcap *s = (struct xcompcap *)data;

	obs_enter_graphics();
	pthread_mutex_lock(&s->lock);

	s->crop_top = obs_data_get_int(settings, "cut_top");
	s->crop_left = obs_data_get_int(settings, "cut_left");
	s->crop_right = obs_data_get_int(settings, "cut_right");
	s->crop_bot = obs_data_get_int(settings, "cut_bot");
	s->swapRedBlue = obs_data_get_bool(settings, "swap_redblue");
	s->show_cursor = obs_data_get_bool(settings, "show_cursor");
	s->include_border = obs_data_get_bool(settings, "include_border");
	s->exclude_alpha = obs_data_get_bool(settings, "exclude_alpha");

	s->windowName = obs_data_get_string(settings, "capture_window");
	s->win = xcomp_find_window(conn, disp, s->windowName);
	if (s->win && s->windowName) {
		struct dstr wname = xcomp_window_name(conn, disp, s->win);
		struct dstr wcls = xcomp_window_class(conn, s->win);
		blog(LOG_INFO,
		     "[window-capture: '%s'] update settings:\n"
		     "\ttitle: %s\n"
		     "\tclass: %s\n",
		     obs_source_get_name(s->source), wname.array, wcls.array);
		dstr_free(&wname);
		dstr_free(&wcls);
	}

	watcher_register(conn, s);
	xcomp_cleanup_pixmap(disp, s);
	xcomp_create_pixmap(conn, s, LOG_ERROR);
	xcb_xcursor_offset_win(conn, s->cursor, s->win);
	xcb_xcursor_offset(s->cursor, s->cursor->x_org + s->crop_left,
			   s->cursor->y_org + s->crop_top);

	pthread_mutex_unlock(&s->lock);
	obs_leave_graphics();
}

static const char *xcompcap_getname(void *data)
{
	UNUSED_PARAMETER(data);
	return obs_module_text("XCCapture");
}

void xcomposite_load(void)
{
	disp = XOpenDisplay(NULL);
	conn = XGetXCBConnection(disp);
	if (xcb_connection_has_error(conn)) {
		blog(LOG_ERROR, "failed opening display");
		return;
	}

	const xcb_query_extension_reply_t *xcomp_ext =
		xcb_get_extension_data(conn, &xcb_composite_id);
	if (!xcomp_ext->present) {
		blog(LOG_ERROR, "Xcomposite extension not supported");
		return;
	}

	xcb_composite_query_version_cookie_t version_cookie =
		xcb_composite_query_version(conn, 0, 2);
	xcb_composite_query_version_reply_t *version =
		xcb_composite_query_version_reply(conn, version_cookie, NULL);
	if (version->major_version == 0 && version->minor_version < 2) {
		blog(LOG_ERROR, "Xcomposite extension is too old: %d.%d < 0.2",
		     version->major_version, version->minor_version);
		free(version);
		return;
	}
	free(version);

	// Must be done before other helpers called.
	xcomp_gather_atoms(conn);

	xcb_screen_t *screen_default =
		xcb_get_screen(conn, DefaultScreen(disp));
	if (!screen_default || !xcomp_check_ewmh(conn, screen_default->root)) {
		blog(LOG_ERROR,
		     "window manager does not support Extended Window Manager Hints (EWMH).\nXComposite capture disabled.");
		return;
	}

	struct obs_source_info sinfo = {
		.id = "xcomposite_input",
		.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW |
				OBS_SOURCE_DO_NOT_DUPLICATE,
		.get_name = xcompcap_getname,
		.create = xcompcap_create,
		.destroy = xcompcap_destroy,
		.get_properties = xcompcap_props,
		.get_defaults = xcompcap_defaults,
		.update = xcompcap_update,
		.video_tick = xcompcap_video_tick,
		.video_render = xcompcap_video_render,
		.get_width = xcompcap_get_width,
		.get_height = xcompcap_get_height,
		.icon_type = OBS_ICON_TYPE_WINDOW_CAPTURE,
	};

	obs_register_source(&sinfo);
}

void xcomposite_unload(void)
{
	XCloseDisplay(disp);
	disp = NULL;
	conn = NULL;
	watcher_unload();
}
