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

#include "portal.h"
#include "pipewire.h"

#include <util/dstr.h>

#define REQUEST_PATH "/org/freedesktop/portal/desktop/request/%s/obs%u"
#define SESSION_PATH "/org/freedesktop/portal/desktop/session/%s/obs%u"

static GDBusConnection *connection = NULL;

static void ensure_connection(void)
{
	g_autoptr(GError) error = NULL;
	if (!connection) {
		connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);

		if (error) {
			blog(LOG_WARNING,
			     "[portals] Error retrieving D-Bus connection: %s",
			     error->message);
			return;
		}
	}
}

char *get_sender_name(void)
{
	char *sender_name;
	char *aux;

	ensure_connection();

	sender_name =
		bstrdup(g_dbus_connection_get_unique_name(connection) + 1);

	/* Replace dots by underscores */
	while ((aux = strstr(sender_name, ".")) != NULL)
		*aux = '_';

	return sender_name;
}

GDBusConnection *portal_get_dbus_connection(void)
{
	ensure_connection();
	return connection;
}

void portal_create_request_path(char **out_path, char **out_token)
{
	static uint32_t request_token_count = 0;

	request_token_count++;

	if (out_token) {
		struct dstr str;
		dstr_init(&str);
		dstr_printf(&str, "obs%u", request_token_count);
		*out_token = str.array;
	}

	if (out_path) {
		char *sender_name;
		struct dstr str;

		sender_name = get_sender_name();

		dstr_init(&str);
		dstr_printf(&str, REQUEST_PATH, sender_name,
			    request_token_count);
		*out_path = str.array;

		bfree(sender_name);
	}
}

void portal_create_session_path(char **out_path, char **out_token)
{
	static uint32_t session_token_count = 0;

	session_token_count++;

	if (out_token) {
		struct dstr str;
		dstr_init(&str);
		dstr_printf(&str, "obs%u", session_token_count);
		*out_token = str.array;
	}

	if (out_path) {
		char *sender_name;
		struct dstr str;

		sender_name = get_sender_name();

		dstr_init(&str);
		dstr_printf(&str, SESSION_PATH, sender_name,
			    session_token_count);
		*out_path = str.array;

		bfree(sender_name);
	}
}
