/*
 * formats.h
 *
 * Copyright 2023 Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
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


/************** duplicated from pipewire.c ***************************/
#include <util/darray.h>
#include <gio/gio.h>
struct obs_pw_version;

struct obs_pw_version {
	int major;
	int minor;
	int micro;
};

static bool check_pw_version(const struct obs_pw_version *pw_version, int major,
			     int minor, int micro)
{
	if (pw_version->major != major)
		return pw_version->major > major;
	if (pw_version->minor != minor)
		return pw_version->minor > minor;
	return pw_version->micro >= micro;
}

struct _obs_pipewire {
	int pipewire_fd;

	struct pw_thread_loop *thread_loop;
	struct pw_context *context;

	struct pw_core *core;
	struct spa_hook core_listener;
	int sync_id;

	struct obs_pw_version server_version;

	struct pw_registry *registry;
	struct spa_hook registry_listener;

	GPtrArray *streams;
};

struct format_info {
	uint32_t spa_format;
	uint32_t drm_format;
	DARRAY(uint64_t) modifiers;
};

struct format_info_base {
	DARRAY(struct format_info) formats;
};

void init_format_info_with_formats(struct format_info_base *base,
		const uint32_t *formats, uint32_t n_formats, bool query_modifiers);
void clear_format_info(struct format_info_base *base);
void remove_modifier_from_format(struct format_info_base *base, struct obs_pw_version *server_version,
					uint32_t spa_format, uint64_t modifier);
/************** duplicated from pipewire.c ***************************/
