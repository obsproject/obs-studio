/* portal.c
 *
 * Copyright 2021 Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
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

#include <stdint.h>
#include <gio/gio.h>

enum portal_capture_type {
	PORTAL_CAPTURE_TYPE_MONITOR = 1 << 0,
	PORTAL_CAPTURE_TYPE_WINDOW = 1 << 1,
	PORTAL_CAPTURE_TYPE_VIRTUAL = 1 << 2,
};

enum portal_cursor_mode {
	PORTAL_CURSOR_MODE_HIDDEN = 1 << 0,
	PORTAL_CURSOR_MODE_EMBEDDED = 1 << 1,
	PORTAL_CURSOR_MODE_METADATA = 1 << 2,
};

uint32_t portal_get_available_capture_types(void);
uint32_t portal_get_available_cursor_modes(void);
uint32_t portal_get_screencast_version(void);

GDBusConnection *portal_get_dbus_connection(void);
GDBusProxy *portal_get_dbus_proxy(void);

void portal_create_request_path(char **out_path, char **out_token);
void portal_create_session_path(char **out_path, char **out_token);
