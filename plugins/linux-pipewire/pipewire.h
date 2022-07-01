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

#include <pipewire/keys.h>
#include <pipewire/properties.h>

typedef struct _obs_pipewire obs_pipewire;

obs_pipewire *obs_pipewire_create(int pipewire_fd);
void obs_pipewire_connect_stream(obs_pipewire *obs_pw, int pipewire_node,
				 const char *stream_name,
				 struct pw_properties *stream_properties);
void obs_pipewire_destroy(obs_pipewire *obs_pw);

void obs_pipewire_show(obs_pipewire *obs_pw);
void obs_pipewire_hide(obs_pipewire *obs_pw);
uint32_t obs_pipewire_get_width(obs_pipewire *obs_pw);
uint32_t obs_pipewire_get_height(obs_pipewire *obs_pw);
void obs_pipewire_video_render(obs_pipewire *obs_pw, gs_effect_t *effect);

void obs_pipewire_set_cursor_visible(obs_pipewire *obs_pw, bool cursor_visible);
