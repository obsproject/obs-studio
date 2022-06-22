/* screencast-portal.c
 *
 * Copyright 2022 Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "pipewire.h"
#include "portal.h"

/* obs_source_info methods */

static const char *screencast_portal_desktop_capture_get_name(void *data)
{
	UNUSED_PARAMETER(data);
	return obs_module_text("PipeWireDesktopCapture");
}

static const char *screencast_portal_window_capture_get_name(void *data)
{
	UNUSED_PARAMETER(data);
	return obs_module_text("PipeWireWindowCapture");
}

static void *screencast_portal_desktop_capture_create(obs_data_t *settings,
						      obs_source_t *source)
{
	return obs_pipewire_create(PORTAL_CAPTURE_TYPE_MONITOR, settings,
				   source);
}
static void *screencast_portal_window_capture_create(obs_data_t *settings,
						     obs_source_t *source)
{
	return obs_pipewire_create(PORTAL_CAPTURE_TYPE_WINDOW, settings,
				   source);
}

static void screencast_portal_capture_destroy(void *data)
{
	obs_pipewire_destroy(data);
}

static void screencast_portal_capture_save(void *data, obs_data_t *settings)
{
	obs_pipewire_save(data, settings);
}

static void screencast_portal_capture_get_defaults(obs_data_t *settings)
{
	obs_pipewire_get_defaults(settings);
}

static obs_properties_t *screencast_portal_capture_get_properties(void *data)
{
	enum portal_capture_type capture_type;
	obs_pipewire_data *obs_pw = data;

	capture_type = obs_pipewire_get_capture_type(obs_pw);

	switch (capture_type) {
	case PORTAL_CAPTURE_TYPE_MONITOR:
		return obs_pipewire_get_properties(data,
						   "PipeWireSelectMonitor");
	case PORTAL_CAPTURE_TYPE_WINDOW:
		return obs_pipewire_get_properties(data,
						   "PipeWireSelectWindow");
	case PORTAL_CAPTURE_TYPE_VIRTUAL:
	default:
		return NULL;
	}
}

static void screencast_portal_capture_update(void *data, obs_data_t *settings)
{
	obs_pipewire_update(data, settings);
}

static void screencast_portal_capture_show(void *data)
{
	obs_pipewire_show(data);
}

static void screencast_portal_capture_hide(void *data)
{
	obs_pipewire_hide(data);
}

static uint32_t screencast_portal_capture_get_width(void *data)
{
	return obs_pipewire_get_width(data);
}

static uint32_t screencast_portal_capture_get_height(void *data)
{
	return obs_pipewire_get_height(data);
}

static void screencast_portal_capture_video_render(void *data,
						   gs_effect_t *effect)
{
	obs_pipewire_video_render(data, effect);
}

void screencast_portal_load(void)
{
	uint32_t available_capture_types = portal_get_available_capture_types();
	bool desktop_capture_available =
		(available_capture_types & PORTAL_CAPTURE_TYPE_MONITOR) != 0;
	bool window_capture_available =
		(available_capture_types & PORTAL_CAPTURE_TYPE_WINDOW) != 0;

	if (available_capture_types == 0) {
		blog(LOG_INFO, "[pipewire] No captures available");
		return;
	}

	blog(LOG_INFO, "[pipewire] Available captures:");
	if (desktop_capture_available)
		blog(LOG_INFO, "[pipewire]     - Desktop capture");
	if (window_capture_available)
		blog(LOG_INFO, "[pipewire]     - Window capture");

	// Desktop capture
	const struct obs_source_info screencast_portal_desktop_capture_info = {
		.id = "pipewire-desktop-capture-source",
		.type = OBS_SOURCE_TYPE_INPUT,
		.output_flags = OBS_SOURCE_VIDEO,
		.get_name = screencast_portal_desktop_capture_get_name,
		.create = screencast_portal_desktop_capture_create,
		.destroy = screencast_portal_capture_destroy,
		.save = screencast_portal_capture_save,
		.get_defaults = screencast_portal_capture_get_defaults,
		.get_properties = screencast_portal_capture_get_properties,
		.update = screencast_portal_capture_update,
		.show = screencast_portal_capture_show,
		.hide = screencast_portal_capture_hide,
		.get_width = screencast_portal_capture_get_width,
		.get_height = screencast_portal_capture_get_height,
		.video_render = screencast_portal_capture_video_render,
		.icon_type = OBS_ICON_TYPE_DESKTOP_CAPTURE,
	};
	if (desktop_capture_available)
		obs_register_source(&screencast_portal_desktop_capture_info);

	// Window capture
	const struct obs_source_info screencast_portal_window_capture_info = {
		.id = "pipewire-window-capture-source",
		.type = OBS_SOURCE_TYPE_INPUT,
		.output_flags = OBS_SOURCE_VIDEO,
		.get_name = screencast_portal_window_capture_get_name,
		.create = screencast_portal_window_capture_create,
		.destroy = screencast_portal_capture_destroy,
		.save = screencast_portal_capture_save,
		.get_defaults = screencast_portal_capture_get_defaults,
		.get_properties = screencast_portal_capture_get_properties,
		.update = screencast_portal_capture_update,
		.show = screencast_portal_capture_show,
		.hide = screencast_portal_capture_hide,
		.get_width = screencast_portal_capture_get_width,
		.get_height = screencast_portal_capture_get_height,
		.video_render = screencast_portal_capture_video_render,
		.icon_type = OBS_ICON_TYPE_WINDOW_CAPTURE,
	};
	if (window_capture_available)
		obs_register_source(&screencast_portal_window_capture_info);
}
