/*
 * Copyright (c) 2015 Hugh Bailey <obs.jim@gmail.com>
 *               2021 Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
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
#include <gio/gio.h>
#include "bmem.h"
#include "dstr.h"

#define PORTAL_NAME "org.freedesktop.portal.Desktop"

struct portal_inhibit_info {
	GDBusConnection *c;
	GCancellable *cancellable;
	unsigned int signal_id;
	char *sender_name;
	char *request_path;
	bool active;
};

static void new_request(struct portal_inhibit_info *info, char **out_token,
			char **out_path)
{
	struct dstr token;
	struct dstr path;
	uint32_t id;

	id = rand();

	dstr_init(&token);
	dstr_printf(&token, "obs_inhibit_portal%u", id);
	*out_token = token.array;

	dstr_init(&path);
	dstr_printf(&path, "/org/freedesktop/portal/desktop/request/%s/%s",
		    info->sender_name, token.array);
	*out_path = path.array;
}

static inline void unsubscribe_from_request(struct portal_inhibit_info *info)
{
	if (info->signal_id > 0) {
		g_dbus_connection_signal_unsubscribe(info->c, info->signal_id);
		info->signal_id = 0;
	}
}

static inline void remove_inhibit_data(struct portal_inhibit_info *info)
{
	g_clear_pointer(&info->request_path, bfree);
	info->active = false;
}

static void response_received(GDBusConnection *bus, const char *sender_name,
			      const char *object_path,
			      const char *interface_name,
			      const char *signal_name, GVariant *parameters,
			      gpointer data)
{
	UNUSED_PARAMETER(bus);
	UNUSED_PARAMETER(sender_name);
	UNUSED_PARAMETER(object_path);
	UNUSED_PARAMETER(interface_name);
	UNUSED_PARAMETER(signal_name);

	struct portal_inhibit_info *info = data;
	g_autoptr(GVariant) ret = NULL;
	uint32_t response;

	g_variant_get(parameters, "(u@a{sv})", &response, &ret);

	if (response != 0) {
		if (response == 1)
			blog(LOG_WARNING, "Inhibit denied by user");

		remove_inhibit_data(info);
	}

	unsubscribe_from_request(info);
}

static void inhibited_cb(GObject *source_object, GAsyncResult *result,
			 gpointer user_data)
{
	UNUSED_PARAMETER(source_object);

	struct portal_inhibit_info *info = user_data;
	g_autoptr(GVariant) reply = NULL;
	g_autoptr(GError) error = NULL;

	reply = g_dbus_connection_call_finish(info->c, result, &error);

	if (error != NULL) {
		if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
			blog(LOG_ERROR, "Failed to inhibit: %s",
			     error->message);
		unsubscribe_from_request(info);
		remove_inhibit_data(info);
	}

	g_clear_object(&info->cancellable);
}

static void do_inhibit(struct portal_inhibit_info *info, const char *reason)
{
	GVariantBuilder options;
	uint32_t flags = 0xC;
	char *token;

	info->active = true;

	new_request(info, &token, &info->request_path);

	info->signal_id = g_dbus_connection_signal_subscribe(
		info->c, PORTAL_NAME, "org.freedesktop.portal.Request",
		"Response", info->request_path, NULL,
		G_DBUS_SIGNAL_FLAGS_NO_MATCH_RULE, response_received, info,
		NULL);

	g_variant_builder_init(&options, G_VARIANT_TYPE_VARDICT);
	g_variant_builder_add(&options, "{sv}", "handle_token",
			      g_variant_new_string(token));
	g_variant_builder_add(&options, "{sv}", "reason",
			      g_variant_new_string(reason));

	bfree(token);

	info->cancellable = g_cancellable_new();
	g_dbus_connection_call(info->c, PORTAL_NAME,
			       "/org/freedesktop/portal/desktop",
			       "org.freedesktop.portal.Inhibit", "Inhibit",
			       g_variant_new("(sua{sv})", "", flags, &options),
			       NULL, G_DBUS_CALL_FLAGS_NONE, -1,
			       info->cancellable, inhibited_cb, info);
}

static void uninhibited_cb(GObject *source_object, GAsyncResult *result,
			   gpointer user_data)
{
	UNUSED_PARAMETER(source_object);

	struct portal_inhibit_info *info = user_data;
	g_autoptr(GVariant) reply = NULL;
	g_autoptr(GError) error = NULL;

	reply = g_dbus_connection_call_finish(info->c, result, &error);

	if (error)
		blog(LOG_WARNING, "Error uninhibiting: %s", error->message);
}

static void do_uninhibit(struct portal_inhibit_info *info)
{
	if (info->cancellable) {
		/* If uninhibit is called before the inhibit call is finished,
		 * cancel it instead.
		 */
		g_cancellable_cancel(info->cancellable);
		g_clear_object(&info->cancellable);
	} else {
		g_dbus_connection_call(info->c, PORTAL_NAME, info->request_path,
				       "org.freedesktop.portal.Request",
				       "Close", g_variant_new("()"),
				       G_VARIANT_TYPE_UNIT,
				       G_DBUS_CALL_FLAGS_NONE, -1, NULL,
				       uninhibited_cb, info);
	}

	remove_inhibit_data(info);
}

void portal_inhibit_info_destroy(struct portal_inhibit_info *info)
{
	if (info) {
		g_cancellable_cancel(info->cancellable);
		unsubscribe_from_request(info);
		remove_inhibit_data(info);
		g_clear_pointer(&info->sender_name, bfree);
		g_clear_object(&info->cancellable);
		g_clear_object(&info->c);
		bfree(info);
	}
}

struct portal_inhibit_info *portal_inhibit_info_create(void)
{
	struct portal_inhibit_info *info = bzalloc(sizeof(*info));
	g_autoptr(GVariant) reply = NULL;
	g_autoptr(GError) error = NULL;
	char *aux;

	info->c = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
	if (!info->c) {
		blog(LOG_ERROR, "Could not create dbus connection: %s",
		     error->message);
		bfree(info);
		return NULL;
	}

	info->sender_name =
		bstrdup(g_dbus_connection_get_unique_name(info->c) + 1);
	while ((aux = strstr(info->sender_name, ".")) != NULL)
		*aux = '_';

	reply = g_dbus_connection_call_sync(
		info->c, "org.freedesktop.DBus", "/org/freedesktop/DBus",
		"org.freedesktop.DBus", "GetNameOwner",
		g_variant_new("(s)", PORTAL_NAME), NULL,
		G_DBUS_CALL_FLAGS_NO_AUTO_START, -1, NULL, NULL);

	if (reply != NULL) {
		blog(LOG_DEBUG, "Found portal inhibitor");
		return info;
	}

	portal_inhibit_info_destroy(info);
	return NULL;
}

void portal_inhibit(struct portal_inhibit_info *info, const char *reason,
		    bool active)
{
	if (active == info->active)
		return;

	if (active)
		do_inhibit(info, reason);
	else
		do_uninhibit(info);
}
