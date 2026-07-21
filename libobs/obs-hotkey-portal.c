/******************************************************************************
    Copyright (C) 2026 by Adam Fallon

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "obs-hotkey-portal.h"
#include "obs-hotkey.h"
#include "obs.h"
#include "util/base.h"
#include "util/bmem.h"
#include "util/c99defs.h"
#include <gio/gio.h>
#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <uthash.h>

#define PORTAL_NAME "org.freedesktop.portal.Desktop"
#define PORTAL_PATH "/org/freedesktop/portal/desktop"
#define SHORTCUTS_IFACE "org.freedesktop.portal.GlobalShortcuts"
#define REQUEST_IFACE "org.freedesktop.portal.Request"
#define SESSION_IFACE "org.freedesktop.portal.Session"

#undef HASH_FUNCTION
#define HASH_FUNCTION(s, len, hashv) (hashv) = *s % UINT_MAX
#define HASH_FIND_HKEY(head, id, out) HASH_FIND(hh, head, &(id), sizeof(size_t), out)

static struct obs_hotkey_portal_state {
	GDBusConnection *conn;
	char *session_path;
	bool bind_shortcuts_called_for_session;
	// tracks the timestamp when pending_hotkeys was last added to
	long add_to_pending_hotkeys_timestamp_us;
	// stores list of hotkeys that need to be registered with the portal
	GQueue *pending_hotkeys;
	// maps portal id -> hotkey id
	GHashTable *registered_hotkeys;
} *state = NULL;

typedef struct obs_hotkey_portal_state obs_hotkey_portal_state_t;

enum obs_hotkey_portal_registered_type {
	SINGLE,
	PAIR,
};
typedef enum obs_hotkey_portal_registered_type obs_hotkey_portal_registered_type_t;

struct obs_hotkey_portal_registered_hotkey {
	obs_hotkey_portal_registered_type_t type;
	size_t id;
};
typedef struct obs_hotkey_portal_registered_hotkey obs_hotkey_portal_registered_hotkey_t;

static void close_session();
static void create_session();

gboolean destroy_registered_hotkey(gpointer key, gpointer value, gpointer user_data)
{
	UNUSED_PARAMETER(user_data);
	bfree(key);
	bfree(value);
	return TRUE;
}

void obs_hotkey_portal_free()
{
	if (state == NULL)
		return;
	g_dbus_connection_close_sync(state->conn, NULL, NULL);
	g_object_unref(state->conn);
	bfree(state->session_path);
	g_queue_free_full(state->pending_hotkeys, bfree);
	g_hash_table_foreach_remove(state->registered_hotkeys, destroy_registered_hotkey, NULL);
	g_hash_table_destroy(state->registered_hotkeys);
	bfree(state);
	state = NULL;
}

GString *get_formatted_sender(GDBusConnection *conn)
{
	g_autoptr(GString) temp = NULL;
	GString *unique_name = NULL;
	temp = g_string_new(g_dbus_connection_get_unique_name(conn));

	g_string_replace(temp, ".", "_", 0);

	if (temp->len < 1) {
		return NULL;
	}

	unique_name = g_string_new(NULL);
	g_string_printf(unique_name, "%s", &temp->str[1]);
	return unique_name;
}

char *get_hotkey_portal_id(obs_hotkey_t *hotkey)
{
	if (hotkey == NULL) {
		return NULL;
	}

	const char *append_to_id = NULL;
	void *registerer = obs_weak_object_get_object(hotkey->registerer);
	if (registerer) {
		switch (obs_hotkey_get_registerer_type(hotkey)) {
		case OBS_HOTKEY_REGISTERER_SOURCE:
			append_to_id = obs_source_get_uuid(registerer);
			obs_object_release(registerer);
			break;
		default:
			append_to_id = obs_obj_get_id(obs_weak_object_get_object(hotkey->registerer));
			break;
		}
	}

	// append to id
	char *id_dst;
	if (append_to_id != NULL) {
		id_dst = bzalloc(strlen(hotkey->name) + strlen(append_to_id) + 8);
		sprintf(id_dst, "%s.%s", hotkey->name, append_to_id);
		return id_dst;
	} else {
		id_dst = bstrdup(hotkey->name);
	}

	obs_object_release(registerer);
	return id_dst;
}

char *get_hotkey_portal_description(obs_hotkey_t *hotkey)
{
	if (hotkey == NULL) {
		return NULL;
	}

	const char *append_to_description = NULL;
	void *registerer = obs_weak_object_get_object(hotkey->registerer);
	if (registerer) {
		switch (obs_hotkey_get_registerer_type(hotkey)) {
		case OBS_HOTKEY_REGISTERER_SOURCE:
			append_to_description = obs_source_get_name(registerer);
			break;
		default:
			break;
		}
	}

	char *description_dst;
	if (append_to_description != NULL) {
		description_dst = bzalloc(strlen(hotkey->description) + strlen(append_to_description) + 8);
		sprintf(description_dst, "%s (%s)", hotkey->description, append_to_description);
	} else {
		description_dst = bstrdup(hotkey->description);
	}

	obs_object_release(registerer);
	return description_dst;
}

void add_hotkey_to_builder(GVariantBuilder *shortcuts_builder, char *id, char *description)
{
	GVariantBuilder vardict_builder;
	g_variant_builder_init(&vardict_builder, G_VARIANT_TYPE_VARDICT);
	g_variant_builder_add(&vardict_builder, "{sv}", "description", g_variant_new_string(description));

	// add shortcut to builder
	GVariant *data[] = {g_variant_new_string(id), g_variant_builder_end(&vardict_builder)};
	g_variant_builder_add_value(shortcuts_builder, g_variant_new_tuple(data, 2));
}

// register all pending hotkeys, if any
static gboolean obs_hotkey_portal_finish_registration()
{
	if (!state || !state->session_path) {
		return G_SOURCE_CONTINUE;
	}

	// check if at least 0.5 seconds have passed since the last time we added to pending_hotkeys
	long current_timestamp = g_get_monotonic_time();
	if (current_timestamp - state->add_to_pending_hotkeys_timestamp_us < 500000) {
		return G_SOURCE_CONTINUE;
	}

	// check if there are any pending hotkeys
	if (g_queue_is_empty(state->pending_hotkeys)) {
		return G_SOURCE_CONTINUE;
	}

	// if BindShortcuts has already been called
	if (state->bind_shortcuts_called_for_session) {
		// destroy session
		close_session();

		// re-queue all hotkeys
		GHashTableIter iter;
		gpointer portal_id, dst;
		g_hash_table_iter_init(&iter, state->registered_hotkeys);
		while (g_hash_table_iter_next(&iter, &portal_id, &dst)) {
			obs_hotkey_portal_registered_hotkey_t *hk = dst;
			g_queue_push_head(state->pending_hotkeys, hk);
			bfree(portal_id);
		}

		g_hash_table_remove_all(state->registered_hotkeys);

		// open session back up
		create_session();
		return G_SOURCE_CONTINUE;
	}

	state->bind_shortcuts_called_for_session = true;

	// bind some shortcuts
	GVariantBuilder shortcuts_builder;
	g_variant_builder_init(&shortcuts_builder, G_VARIANT_TYPE("a(sa{sv})"));

	for (obs_hotkey_portal_registered_hotkey_t *queued_hk = g_queue_pop_head(state->pending_hotkeys);
	     queued_hk != NULL; queued_hk = g_queue_pop_head(state->pending_hotkeys)) {

		if (queued_hk->type == SINGLE) {
			obs_hotkey_t *hotkey;
			HASH_FIND_HKEY(obs->hotkeys.hotkeys, queued_hk->id, hotkey);
			blog(LOG_INFO, "Registering hotkey id with portal: %lu", queued_hk->id);

			if (!hotkey)
				continue;

			char *id = get_hotkey_portal_id(hotkey);
			char *description = get_hotkey_portal_description(hotkey);

			blog(LOG_INFO, "Registering hotkey with portal: %s: %s", hotkey->name, hotkey->description);

			add_hotkey_to_builder(&shortcuts_builder, id, description);
			g_hash_table_insert(state->registered_hotkeys, id, queued_hk);

			// id is now owned by state->registered_hotkeys
			bfree(description);
		} else if (queued_hk->type == PAIR) {
			blog(LOG_INFO, "Registering hotkey pair id with portal: %lu", queued_hk->id);
			// get pair
			obs_hotkey_pair_t *pair;
			HASH_FIND_HKEY(obs->hotkeys.hotkey_pairs, queued_hk->id, pair);

			if (!pair)
				continue;

			// get both hotkeys in that pair
			obs_hotkey_t *hotkey1, *hotkey2;
			HASH_FIND_HKEY(obs->hotkeys.hotkeys, pair->id[0], hotkey1);
			HASH_FIND_HKEY(obs->hotkeys.hotkeys, pair->id[1], hotkey2);

			if (!hotkey1 || !hotkey2)
				continue;

			// get id
			char *hotkey1_id = get_hotkey_portal_id(hotkey1);
			char *hotkey2_id = get_hotkey_portal_id(hotkey2);
			size_t joint_id_len = strlen(hotkey1_id) + strlen(hotkey2_id) + 2;
			char *joint_id = bzalloc(joint_id_len);
			snprintf(joint_id, joint_id_len, "%s.%s", hotkey1_id, hotkey2_id);

			// get description
			char *hotkey1_desc = get_hotkey_portal_description(hotkey1);
			char *hotkey2_desc = get_hotkey_portal_description(hotkey2);
			size_t joint_desc_len = strlen(hotkey1_desc) + strlen(hotkey2_desc) + 2;
			char *joint_desc = bzalloc(joint_desc_len);
			snprintf(joint_desc, joint_desc_len, "%s/%s", hotkey1_desc, hotkey2_desc);

			// add hotkey
			add_hotkey_to_builder(&shortcuts_builder, joint_id, joint_desc);
			g_hash_table_insert(state->registered_hotkeys, joint_id, queued_hk);

			bfree(hotkey1_id);
			bfree(hotkey1_desc);
			bfree(hotkey2_id);
			bfree(hotkey2_desc);
			// joint_id is now owned by state->registered_hotkeys
			bfree(joint_desc);
		} else {
			blog(LOG_ERROR, "Invalid hotkey type: %d", queued_hk->type);
		}
	}

	// options
	GVariantBuilder options_builder;
	char request_handle_token[64] = {};
	snprintf(request_handle_token, sizeof(request_handle_token), "obs_globalshortcuts_request%d", rand());

	g_variant_builder_init(&options_builder, G_VARIANT_TYPE_VARDICT);
	g_variant_builder_add(&options_builder, "{sv}", "handle_token", g_variant_new_string(request_handle_token));

	GVariant *options[] = {g_variant_new_object_path(state->session_path),
			       g_variant_builder_end(&shortcuts_builder), g_variant_new_string(""),
			       g_variant_builder_end(&options_builder)};
	g_dbus_connection_call(state->conn, PORTAL_NAME, PORTAL_PATH, SHORTCUTS_IFACE, "BindShortcuts",
			       g_variant_new_tuple(options, 4), NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);

	return G_SOURCE_CONTINUE;
}

static void call_hotkey_cb(const char *received_id, bool pressed)
{
	char *iter_id = NULL;

	obs_hotkey_portal_registered_hotkey_t *received_hk;
	received_hk = g_hash_table_lookup(state->registered_hotkeys, received_id);

	if (!received_hk) {
		blog(LOG_ERROR, "Failed to find activated hotkey for portal");
		return;
	}

	if (received_hk->type == SINGLE) {
		obs_hotkey_t *hotkey;
		HASH_FIND_HKEY(obs->hotkeys.hotkeys, received_hk->id, hotkey);

		if (!hotkey)
			return;
		if (pressed == hotkey->pressed)
			return;

		hotkey->pressed = pressed;

		if (!obs->hotkeys.reroute_hotkeys)
			hotkey->func(hotkey->data, hotkey->id, hotkey, pressed);
		else if (obs->hotkeys.router_func)
			obs->hotkeys.router_func(obs->hotkeys.router_func_data, hotkey->id, pressed);

		bfree(iter_id);
	} else if (received_hk->type == PAIR) {
		obs_hotkey_pair_t *pair;
		HASH_FIND_HKEY(obs->hotkeys.hotkey_pairs, received_hk->id, pair);

		if (!pair)
			return;

		for (size_t i = 0; i < 2; ++i) {
			obs_hotkey_t *hk;
			HASH_FIND_HKEY(obs->hotkeys.hotkeys, pair->id[i], hk);
			if (!obs->hotkeys.reroute_hotkeys)
				pair->func[i](hk->data, hk->id, hk, pressed);
			else if (obs->hotkeys.router_func)
				obs->hotkeys.router_func(obs->hotkeys.router_func_data, hk->id, pressed);
		}

	} else {
		blog(LOG_ERROR, "Invalid hotkey type: %d", received_hk->type);
	}
}

static void activated_cb(GDBusConnection *conn, const char *sender_name, const char *object_path,
			 const char *interface_name, const char *signal_name, GVariant *parameters, gpointer user_data)
{
	UNUSED_PARAMETER(conn);
	UNUSED_PARAMETER(sender_name);
	UNUSED_PARAMETER(object_path);
	UNUSED_PARAMETER(interface_name);
	UNUSED_PARAMETER(signal_name);
	UNUSED_PARAMETER(parameters);
	UNUSED_PARAMETER(user_data);
	g_autofree char *received_id = NULL;
	g_variant_get(parameters, "(ost@a{sv})", NULL, &received_id, NULL, NULL);
	call_hotkey_cb(received_id, true);
}

static void deactivated_cb(GDBusConnection *conn, const char *sender_name, const char *object_path,
			   const char *interface_name, const char *signal_name, GVariant *parameters,
			   gpointer user_data)
{
	UNUSED_PARAMETER(conn);
	UNUSED_PARAMETER(sender_name);
	UNUSED_PARAMETER(object_path);
	UNUSED_PARAMETER(interface_name);
	UNUSED_PARAMETER(signal_name);
	UNUSED_PARAMETER(parameters);
	UNUSED_PARAMETER(user_data);
	g_autofree char *received_id = NULL;
	g_variant_get(parameters, "(ost@a{sv})", NULL, &received_id, NULL, NULL);
	call_hotkey_cb(received_id, false);
}

static void shortcuts_changed_cb(GDBusConnection *conn, const char *sender_name, const char *object_path,
				 const char *interface_name, const char *signal_name, GVariant *parameters,
				 gpointer user_data)
{
	UNUSED_PARAMETER(conn);
	UNUSED_PARAMETER(sender_name);
	UNUSED_PARAMETER(object_path);
	UNUSED_PARAMETER(interface_name);
	UNUSED_PARAMETER(signal_name);
	UNUSED_PARAMETER(parameters);
	UNUSED_PARAMETER(user_data);

	// TODO: update UI?
	g_print("Shortcuts changed!\n");
}

static void create_session_cb(GDBusConnection *conn, const char *sender_name, const char *object_path,
			      const char *interface_name, const char *signal_name, GVariant *parameters,
			      gpointer user_data)
{
	if (!state || state->session_path) {
		return;
	}

	UNUSED_PARAMETER(sender_name);
	UNUSED_PARAMETER(object_path);
	UNUSED_PARAMETER(interface_name);
	UNUSED_PARAMETER(signal_name);
	UNUSED_PARAMETER(user_data);

	int response;
	g_autoptr(GVariant) options = NULL;
	const char *session_handle = NULL;

	g_variant_get(parameters, "(u@a{sv})", &response, &options);
	g_variant_lookup(options, "session_handle", "&s", &session_handle);

	// start listening to shortcuts
	g_dbus_connection_signal_subscribe(conn, PORTAL_NAME, SHORTCUTS_IFACE, "Activated", PORTAL_PATH, NULL,
					   G_DBUS_SIGNAL_FLAGS_NONE, activated_cb, NULL, NULL);
	g_dbus_connection_signal_subscribe(conn, PORTAL_NAME, SHORTCUTS_IFACE, "Deactivated", PORTAL_PATH, NULL,
					   G_DBUS_SIGNAL_FLAGS_NONE, deactivated_cb, NULL, NULL);
	g_dbus_connection_signal_subscribe(conn, PORTAL_NAME, SHORTCUTS_IFACE, "ShortcutsChanged", PORTAL_PATH, NULL,
					   G_DBUS_SIGNAL_FLAGS_NONE, shortcuts_changed_cb, NULL, NULL);

	state->session_path = bstrdup(session_handle);
}

static void close_session()
{
	if (!state || !state->session_path || !state->conn) {
		return;
	}

	GError *error = NULL;
	g_dbus_connection_call_sync(state->conn, PORTAL_NAME, state->session_path, SESSION_IFACE, "Close", NULL, NULL,
				    G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

	if (error != NULL) {
		blog(LOG_ERROR, "Failed to destroy global shortcuts portal session: %s", error->message);
		return;
	}
	bfree(state->session_path);
	state->bind_shortcuts_called_for_session = false;
	state->session_path = NULL;
}

static void create_session()
{
	if (!state || state->session_path || !state->conn) {
		return;
	}

	state->bind_shortcuts_called_for_session = false;

	g_autoptr(GString) session_token = NULL;
	g_autoptr(GString) request_handle_token = NULL;
	GVariant *options = NULL;
	g_autoptr(GString) request_path = NULL;
	g_autoptr(GString) sender = NULL;
	GVariantBuilder options_builder;
	g_autoptr(GVariant) result = NULL;
	GError *error = NULL;

	sender = get_formatted_sender(state->conn);

	// get tokens
	session_token = g_string_new(NULL);
	g_string_printf(session_token, "obs_globalshortcuts_session%d", rand());

	request_handle_token = g_string_new(NULL);
	g_string_printf(request_handle_token, "obs_globalshortcuts_request%d", rand());

	// connect to session parameters
	g_variant_builder_init(&options_builder, G_VARIANT_TYPE("a{sv}"));
	g_variant_builder_add(&options_builder, "{sv}", "handle_token",
			      g_variant_new_string(request_handle_token->str));
	g_variant_builder_add(&options_builder, "{sv}", "session_handle_token",
			      g_variant_new_string(session_token->str));
	options = g_variant_builder_end(&options_builder);

	// start to listen to request object in advance
	request_path = g_string_new(NULL);
	g_string_printf(request_path, "/org/freedesktop/portal/desktop/request/%s/%s", sender->str,
			request_handle_token->str);
	g_print("Listening to request at path: %s\n", request_path->str);
	g_dbus_connection_signal_subscribe(state->conn, PORTAL_NAME, REQUEST_IFACE, "Response", request_path->str, NULL,
					   G_DBUS_SIGNAL_FLAGS_NONE, create_session_cb, NULL, NULL);

	// finally, connect to session
	result = g_dbus_connection_call_sync(state->conn, PORTAL_NAME, PORTAL_PATH, SHORTCUTS_IFACE, "CreateSession",
					     g_variant_new_tuple(&options, 1), NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
					     &error);

	if (result == NULL) {
		blog(LOG_WARNING, "Failed to create global shortcuts portal session: %s", error->message);
		return;
	}
}

static void obs_hotkey_portal_init()
{
	// only allow this function to run once
	static bool init_run = false;
	if (init_run) {
		return;
	}
	init_run = true;

	g_autoptr(GDBusConnection) conn = NULL;
	GError *error = NULL;

	// get connection
	conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
	if (!conn) {
		g_printerr("Failed to connect to dbus: %s\n", error->message);
		return;
	}

	// initialise state
	state = bzalloc(sizeof(obs_hotkey_portal_state_t));
	state->conn = conn;
	state->session_path = NULL;
	state->pending_hotkeys = g_queue_new();
	state->registered_hotkeys = g_hash_table_new(g_str_hash, g_str_equal);

	create_session();

	// schedule a timer to register all pending hotkeys every 0.1 seconds
	g_timeout_add(500, (GSourceFunc)obs_hotkey_portal_finish_registration, NULL);
}

void obs_hotkey_portal_register(obs_hotkey_t *hotkey)
{
	// init if haven't already
	obs_hotkey_portal_init();
	if (state == NULL) {
		blog(LOG_WARNING,
		     "Attempting to register a hotkey with the xdg-desktop-portal, but this feature has not been initialised");
		return;
	}

	// add to pending list
	obs_hotkey_portal_registered_hotkey_t *hk;
	hk = bzalloc(sizeof(obs_hotkey_portal_registered_hotkey_t));
	hk->type = SINGLE;
	hk->id = hotkey->id;

	blog(LOG_WARNING, "Registering hotkey with id %lu to xdg-desktop-portal", hk->id);

	g_queue_push_tail(state->pending_hotkeys, hk);
	state->add_to_pending_hotkeys_timestamp_us = g_get_monotonic_time();
}

void obs_hotkey_portal_register_pair(obs_hotkey_pair_t *pair)
{
	// init if haven't already
	obs_hotkey_portal_init();
	if (state == NULL) {
		blog(LOG_WARNING,
		     "Attempting to register a hotkey with the xdg-desktop-portal, but this feature has not been initialised");
		return;
	}

	blog(LOG_WARNING, "Registering hotkey pair with id %lu to xdg-desktop-portal", pair->pair_id);

	// add to pending list
	obs_hotkey_portal_registered_hotkey_t *hk;
	hk = bzalloc(sizeof(obs_hotkey_portal_registered_hotkey_t));
	hk->type = PAIR;
	hk->id = pair->pair_id;

	g_queue_push_tail(state->pending_hotkeys, hk);
	state->add_to_pending_hotkeys_timestamp_us = g_get_monotonic_time();
}

void obs_hotkey_portal_unregister(obs_hotkey_id id)
{
	if (state == NULL) {
		blog(LOG_WARNING,
		     "Attempting to unregister a hotkey with the xdg-desktop-portal, but this feature has not been initialised");
		return;
	}

	blog(LOG_WARNING, "Unregistering hotkey with id %lu from xdg-desktop-portal", id);

	// remove from pending list (if it's there)
	for (GList *l = state->pending_hotkeys->head; l != NULL; l = l->next) {
		if (*(obs_hotkey_id *)l->data == id) {
			g_queue_delete_link(state->pending_hotkeys, l);
			break;
		}
	}
}
