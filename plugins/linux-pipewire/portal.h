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

typedef void (*portal_signal_callback)(GVariant *parameters, void *user_data);

GDBusConnection *portal_get_dbus_connection(void);

void portal_create_request_path(char **out_path, char **out_token);
void portal_create_session_path(char **out_path, char **out_token);

void portal_signal_subscribe(const char *path, GCancellable *cancellable,
			     portal_signal_callback callback, void *user_data);
