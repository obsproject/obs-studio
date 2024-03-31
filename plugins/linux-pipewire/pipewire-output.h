/* pipewire-output.h
 *
 * Copyright 2024 columbarius <co1umbarius@protonmail.com>
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

#include "pipewire.h"

typedef struct _obs_pipewire_output_stream obs_pipewire_output_stream;

obs_pipewire_output_stream *obs_pipewire_connect_output_stream(
	obs_pipewire *obs_pw, obs_output_t *output,
	const char *stream_name, struct pw_properties *stream_properties);

void obs_pipewire_output_stream_show(obs_pipewire_output_stream *obs_pw);
void obs_pipewire_output_stream_hide(obs_pipewire_output_stream *obs_pw);
uint32_t obs_pipewire_output_stream_get_width(obs_pipewire_output_stream *obs_pw);
uint32_t obs_pipewire_output_stream_get_height(obs_pipewire_output_stream *obs_pw);
void obs_pipewire_output_stream_export_frame(obs_pipewire_output_stream *obs_pw,
				      struct video_data *frame);

void obs_pipewire_output_stream_set_cursor_visible(obs_pipewire_output_stream *obs_pw,
					    bool cursor_visible);
void obs_pipewire_output_stream_destroy(obs_pipewire_output_stream *obs_pw_output_stream);
