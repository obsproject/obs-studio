/* pipewire-capture.c
 *
 * Copyright 2020 Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
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

/* obs_source_info methods */

static const char *pipewire_desktop_capture_get_name(void *data)
{
	UNUSED_PARAMETER(data);
	return obs_module_text("PipeWireDesktopCapture");
}

static const char *pipewire_window_capture_get_name(void *data)
{
	UNUSED_PARAMETER(data);
	return obs_module_text("PipeWireWindowCapture");
}

static void *pipewire_desktop_capture_create(obs_data_t *settings,
					     obs_source_t *source)
{
	return obs_pipewire_create(DESKTOP_CAPTURE, settings, source);
}
static void *pipewire_window_capture_create(obs_data_t *settings,
					    obs_source_t *source)
{
	return obs_pipewire_create(WINDOW_CAPTURE, settings, source);
}

static void pipewire_capture_destroy(void *data)
{
	obs_pipewire_destroy(data);
}

static void pipewire_capture_get_defaults(obs_data_t *settings)
{
	obs_pipewire_get_defaults(settings);
}

static obs_properties_t *pipewire_capture_get_properties(void *data)
{
	enum obs_pw_capture_type capture_type;
	obs_pipewire_data *obs_pw = data;

	capture_type = obs_pipewire_get_capture_type(obs_pw);

	switch (capture_type) {
	case DESKTOP_CAPTURE:
		return obs_pipewire_get_properties(data,
						   "PipeWireSelectMonitor");
	case WINDOW_CAPTURE:
		return obs_pipewire_get_properties(data,
						   "PipeWireSelectWindow");
	default:
		return NULL;
	}
}

static void pipewire_capture_update(void *data, obs_data_t *settings)
{
	obs_pipewire_update(data, settings);
}

static void pipewire_capture_show(void *data)
{
	obs_pipewire_show(data);
}

static void pipewire_capture_hide(void *data)
{
	obs_pipewire_hide(data);
}

static uint32_t pipewire_capture_get_width(void *data)
{
	return obs_pipewire_get_width(data);
}

static uint32_t pipewire_capture_get_height(void *data)
{
	return obs_pipewire_get_height(data);
}

static void pipewire_capture_video_render(void *data, gs_effect_t *effect)
{
	obs_pipewire_video_render(data, effect);
}

void pipewire_capture_load(void)
{
	// Desktop capture
	const struct obs_source_info pipewire_desktop_capture_info = {
		.id = "pipewire-desktop-capture-source",
		.type = OBS_SOURCE_TYPE_INPUT,
		.output_flags = OBS_SOURCE_VIDEO,
		.get_name = pipewire_desktop_capture_get_name,
		.create = pipewire_desktop_capture_create,
		.destroy = pipewire_capture_destroy,
		.get_defaults = pipewire_capture_get_defaults,
		.get_properties = pipewire_capture_get_properties,
		.update = pipewire_capture_update,
		.show = pipewire_capture_show,
		.hide = pipewire_capture_hide,
		.get_width = pipewire_capture_get_width,
		.get_height = pipewire_capture_get_height,
		.video_render = pipewire_capture_video_render,
		.icon_type = OBS_ICON_TYPE_DESKTOP_CAPTURE,
	};
	obs_register_source(&pipewire_desktop_capture_info);

	// Window capture
	const struct obs_source_info pipewire_window_capture_info = {
		.id = "pipewire-window-capture-source",
		.type = OBS_SOURCE_TYPE_INPUT,
		.output_flags = OBS_SOURCE_VIDEO,
		.get_name = pipewire_window_capture_get_name,
		.create = pipewire_window_capture_create,
		.destroy = pipewire_capture_destroy,
		.get_defaults = pipewire_capture_get_defaults,
		.get_properties = pipewire_capture_get_properties,
		.update = pipewire_capture_update,
		.show = pipewire_capture_show,
		.hide = pipewire_capture_hide,
		.get_width = pipewire_capture_get_width,
		.get_height = pipewire_capture_get_height,
		.video_render = pipewire_capture_video_render,
		.icon_type = OBS_ICON_TYPE_WINDOW_CAPTURE,
	};
	obs_register_source(&pipewire_window_capture_info);

	pw_init(NULL, NULL);
}

void pipewire_capture_unload(void)
{
	pw_deinit();
}
