/* gamescope.c
 *
 * Copyright 2023 Joshua Ashton for Valve Software
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

#include <pipewire/pipewire.h>
#include <gio/gio.h>

struct gamescope_capture {
	uint32_t pipewire_node;

	obs_source_t *source;

	obs_pipewire *obs_pw;
	obs_pipewire_stream *obs_pw_stream;
};

static void registry_event_global(void *user_data, uint32_t id,
				  uint32_t permissions, const char *type,
				  uint32_t version,
				  const struct spa_dict *props)
{
	obs_pipewire *obs_pw = user_data;
	struct gamescope_capture *capture = obs_pipewire_get_userdata(obs_pw);
	UNUSED_PARAMETER(permissions);
	UNUSED_PARAMETER(version);

	if (spa_streq(type, PW_TYPE_INTERFACE_Node)) {
		const char *node_name =
			spa_dict_lookup(props, PW_KEY_NODE_NAME);
		if (spa_streq(node_name, "gamescope")) {
			blog(LOG_INFO,
			     "[gamescope-obs] Found gamescope node: %u", id);
			capture->pipewire_node = id;

			capture->obs_pw_stream = obs_pipewire_connect_stream(
				obs_pw, capture->source, capture->pipewire_node,
				"OBS Studio",
				pw_properties_new(PW_KEY_MEDIA_TYPE, "Video",
						  PW_KEY_MEDIA_CATEGORY,
						  "Capture", PW_KEY_MEDIA_ROLE,
						  "Screen", NULL));
		}
	}
}

static void registry_event_global_remove(void *user_data, uint32_t id)
{
	obs_pipewire *obs_pw = user_data;
	struct gamescope_capture *capture = obs_pipewire_get_userdata(obs_pw);

	if (!capture)
		return;

	if (capture->pipewire_node == id) {
		blog(LOG_INFO, "[gamescope-obs] Removing current node: %u", id);
		capture->pipewire_node = SPA_ID_INVALID;
		g_clear_pointer(&capture->obs_pw_stream,
				obs_pipewire_stream_destroy);
	}
}

static const struct pw_registry_events registry_events = {
	PW_VERSION_REGISTRY_EVENTS,
	.global = registry_event_global,
	.global_remove = registry_event_global_remove,
};

static const char *gamescope_capture_get_name(void *data)
{
	UNUSED_PARAMETER(data);
	return obs_module_text("PipeWireGamescopeCapture");
}

static void gamescope_capture_destroy(void *data)
{
	struct gamescope_capture *capture = data;

	if (!capture)
		return;

	capture->pipewire_node = SPA_ID_INVALID;
	g_clear_pointer(&capture->obs_pw_stream, obs_pipewire_stream_destroy);
	obs_pipewire_destroy(capture->obs_pw);
	bfree(capture);
}

static void *gamescope_capture_create(obs_data_t *settings,
				      obs_source_t *source)
{
	struct gamescope_capture *capture;
	UNUSED_PARAMETER(settings);

	capture = bzalloc(sizeof(struct gamescope_capture));
	capture->pipewire_node = SPA_ID_INVALID;
	capture->source = source;
	capture->obs_pw = obs_pipewire_create(-1, &registry_events, capture);

	if (!capture->obs_pw) {
		gamescope_capture_destroy(capture);
		return NULL;
	}

	return capture;
}

static void gamescope_capture_save(void *data, obs_data_t *settings)
{
	struct gamescope_capture *capture = data;
	UNUSED_PARAMETER(capture);
	UNUSED_PARAMETER(settings);
}

static void gamescope_capture_get_defaults(obs_data_t *settings)
{
	UNUSED_PARAMETER(settings);
}

static obs_properties_t *gamescope_capture_get_properties(void *data)
{
	struct gamescope_capture *capture = data;
	obs_properties_t *properties;

	properties = obs_properties_create();

	return properties;
}

static void gamescope_capture_update(void *data, obs_data_t *settings)
{
	struct gamescope_capture *capture = data;
	UNUSED_PARAMETER(capture);
	UNUSED_PARAMETER(settings);
}

static void gamescope_capture_show(void *data)
{
	struct gamescope_capture *capture = data;

	if (capture->obs_pw_stream)
		obs_pipewire_stream_show(capture->obs_pw_stream);
}

static void gamescope_capture_hide(void *data)
{
	struct gamescope_capture *capture = data;

	if (capture->obs_pw_stream)
		obs_pipewire_stream_hide(capture->obs_pw_stream);
}

static uint32_t gamescope_capture_get_width(void *data)
{
	struct gamescope_capture *capture = data;

	if (capture->obs_pw_stream)
		return obs_pipewire_stream_get_width(capture->obs_pw_stream);
	else
		return 0;
}

static uint32_t gamescope_capture_get_height(void *data)
{
	struct gamescope_capture *capture = data;

	if (capture->obs_pw_stream)
		return obs_pipewire_stream_get_height(capture->obs_pw_stream);
	else
		return 0;
}

static void gamescope_capture_video_render(void *data, gs_effect_t *effect)
{
	struct gamescope_capture *capture = data;

	if (capture->obs_pw_stream)
		obs_pipewire_stream_video_render(capture->obs_pw_stream,
						 effect);
}

void gamescope_load(void)
{
	const struct obs_source_info gamescope_capture_info = {
		.id = "pipewire-gamescope-capture-source",
		.type = OBS_SOURCE_TYPE_INPUT,
		.output_flags = OBS_SOURCE_VIDEO,
		.get_name = gamescope_capture_get_name,
		.create = gamescope_capture_create,
		.destroy = gamescope_capture_destroy,
		.save = gamescope_capture_save,
		.get_defaults = gamescope_capture_get_defaults,
		.get_properties = gamescope_capture_get_properties,
		.update = gamescope_capture_update,
		.show = gamescope_capture_show,
		.hide = gamescope_capture_hide,
		.get_width = gamescope_capture_get_width,
		.get_height = gamescope_capture_get_height,
		.video_render = gamescope_capture_video_render,
		.icon_type = OBS_ICON_TYPE_GAME_CAPTURE,
	};

	obs_register_source(&gamescope_capture_info);
}

void gamescope_unload(void) {}
