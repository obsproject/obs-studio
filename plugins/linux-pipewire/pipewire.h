/* pipewire.h
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

#pragma once

#include <obs-module.h>

#include <pipewire/pipewire.h>

typedef struct _obs_pipewire obs_pipewire;
typedef struct _obs_pipewire_stream obs_pipewire_stream;

struct obs_pipewire_connect_stream_info {
	const char *stream_name;
	struct pw_properties *stream_properties;
	struct {
		bool cursor_visible;
	} screencast;
	struct {
		const enum spa_media_subtype *subtype;
		const struct obs_pw_video_format *format;
		const struct spa_rectangle *resolution;
		const struct spa_fraction *framerate;
	} video;
};

obs_pipewire *obs_pipewire_connect_fd(int pipewire_fd, const struct pw_registry_events *registry_events,
				      void *user_data);
struct pw_registry *obs_pipewire_get_registry(obs_pipewire *obs_pw);
void obs_pipewire_roundtrip(obs_pipewire *obs_pw);
void obs_pipewire_destroy(obs_pipewire *obs_pw);

obs_pipewire_stream *obs_pipewire_connect_stream(obs_pipewire *obs_pw, obs_source_t *source, int pipewire_node,
						 const struct obs_pipewire_connect_stream_info *connect_info);

void obs_pipewire_stream_show(obs_pipewire_stream *obs_pw_stream);
void obs_pipewire_stream_hide(obs_pipewire_stream *obs_pw_stream);
uint32_t obs_pipewire_stream_get_width(obs_pipewire_stream *obs_pw_stream);
uint32_t obs_pipewire_stream_get_height(obs_pipewire_stream *obs_pw_stream);
void obs_pipewire_stream_video_render(obs_pipewire_stream *obs_pw_stream, gs_effect_t *effect);

void obs_pipewire_stream_set_cursor_visible(obs_pipewire_stream *obs_pw_stream, bool cursor_visible);
void obs_pipewire_stream_destroy(obs_pipewire_stream *obs_pw_stream);

void obs_pipewire_stream_set_framerate(obs_pipewire_stream *obs_pw_stream, const struct spa_fraction *framerate);
void obs_pipewire_stream_set_resolution(obs_pipewire_stream *obs_pw, const struct spa_rectangle *resolution);
