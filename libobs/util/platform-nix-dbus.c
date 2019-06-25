/*
 * Copyright (c) 2015 Hugh Bailey <obs.jim@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <assert.h>
#include <dbus/dbus.h>
#include "bmem.h"

/* NOTE: This is basically just the VLC implementation from its d-bus power
 * management inhibition code.  Credit is theirs for this. */

enum service_type {
	FREEDESKTOP_SS, /* freedesktop screensaver (KDE >= 4, GNOME >= 3.10) */
	FREEDESKTOP_PM, /* freedesktop power management (KDE, gnome <= 2.26) */
	MATE_SM,        /* MATE (>= 1.0) session manager */
	GNOME_SM,       /* GNOME 2.26 - 3.4 sessopm mamager */
};

struct service_info {
	const char *name;
	const char *path;
	const char *uninhibit;
};

static const struct service_info services[] = {
	[FREEDESKTOP_SS] =
		{
			.name = "org.freedesktop.ScreenSaver",
			.path = "/ScreenSaver",
			.uninhibit = "UnInhibit",
		},
	[FREEDESKTOP_PM] =
		{
			.name = "org.freedesktop.PowerManagement.Inhibit",
			.path = "/org/freedesktop/PowerManagement",
			.uninhibit = "UnInhibit",
		},
	[MATE_SM] =
		{
			.name = "org.mate.SessionManager",
			.path = "/org/mate/SessionManager",
			.uninhibit = "Uninhibit",
		},
	[GNOME_SM] =
		{
			.name = "org.gnome.SessionManager",
			.path = "/org/gnome/SessionManager",
			.uninhibit = "Uninhibit",
		},
};

static const size_t num_services =
	(sizeof(services) / sizeof(struct service_info));

struct dbus_sleep_info {
	const struct service_info *service;
	DBusPendingCall *pending;
	DBusConnection *c;
	dbus_uint32_t id;
	enum service_type type;
};

void dbus_sleep_info_destroy(struct dbus_sleep_info *info)
{
	if (info) {
		if (info->pending) {
			dbus_pending_call_cancel(info->pending);
			dbus_pending_call_unref(info->pending);
		}

		dbus_connection_close(info->c);
		dbus_connection_unref(info->c);
		bfree(info);
	}
}

struct dbus_sleep_info *dbus_sleep_info_create(void)
{
	struct dbus_sleep_info *info = bzalloc(sizeof(*info));
	DBusError err;

	dbus_error_init(&err);

	info->c = dbus_bus_get_private(DBUS_BUS_SESSION, &err);
	if (!info->c) {
		blog(LOG_ERROR, "Could not create dbus connection: %s",
		     err.message);
		bfree(info);
		return NULL;
	}

	for (size_t i = 0; i < num_services; i++) {
		const struct service_info *service = &services[i];

		if (!service->name)
			continue;

		if (dbus_bus_name_has_owner(info->c, service->name, NULL)) {
			blog(LOG_DEBUG, "Found dbus service: %s",
			     service->name);
			info->service = service;
			info->type = (enum service_type)i;
			return info;
		}
	}

	dbus_sleep_info_destroy(info);
	return NULL;
}

void dbus_inhibit_sleep(struct dbus_sleep_info *info, const char *reason,
			bool active)
{
	DBusMessage *reply;
	const char *method;
	dbus_bool_t success;

	if (info->pending) {

		dbus_pending_call_block(info->pending);
		reply = dbus_pending_call_steal_reply(info->pending);
		dbus_pending_call_unref(info->pending);
		info->pending = NULL;

		if (reply) {
			success = dbus_message_get_args(reply, NULL,
							DBUS_TYPE_UINT32,
							&info->id,
							DBUS_TYPE_INVALID);
			if (!success)
				info->id = 0;
			dbus_message_unref(reply);
		}
	}

	if (active == !!info->id)
		return;

	method = active ? "Inhibit" : info->service->uninhibit;

	reply = dbus_message_new_method_call(info->service->name,
					     info->service->path,
					     info->service->name, method);
	if (reply == NULL) {
		blog(LOG_ERROR, "dbus_message_new_method_call failed");
		return;
	}

	if (active) {
		const char *program = "libobs";
		dbus_uint32_t flags = 0xC;
		dbus_uint32_t xid = 0;

		assert(info->id == 0);

		switch (info->type) {
		case MATE_SM:
		case GNOME_SM:
			success = dbus_message_append_args(
				reply, DBUS_TYPE_STRING, &program,
				DBUS_TYPE_UINT32, &xid, DBUS_TYPE_STRING,
				&reason, DBUS_TYPE_UINT32, &flags,
				DBUS_TYPE_INVALID);
			break;
		default:
			success = dbus_message_append_args(
				reply, DBUS_TYPE_STRING, &program,
				DBUS_TYPE_STRING, &reason, DBUS_TYPE_INVALID);
		}

		if (success) {
			success = dbus_connection_send_with_reply(
				info->c, reply, &info->pending, -1);
			if (!success)
				info->pending = NULL;
		}
	} else {
		assert(info->id != 0);
		success = dbus_message_append_args(
			reply, DBUS_TYPE_UINT32, &info->id, DBUS_TYPE_INVALID);
		if (success)
			success = dbus_connection_send(info->c, reply, NULL);
		if (!success)
			info->id = 0;
	}

	dbus_connection_flush(info->c);
	dbus_message_unref(reply);
}
