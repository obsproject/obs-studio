#include "obs-hotkey-portal.h"
#include "obs.h"
#include "util/base.h"
#include "util/c99defs.h"
#include <gio/gio.h>
#include <glib.h>

#define PORTAL_NAME "org.freedesktop.portal.Desktop"
#define PORTAL_PATH "/org/freedesktop/portal/desktop"
#define SHORTCUTS_IFACE "org.freedesktop.portal.GlobalShortcuts"
#define REQUEST_IFACE "org.freedesktop.portal.Request"
#define SESSION_IFACE "org.freedesktop.portal.Session"

static struct obs_hotkey_portal_state {
	GDBusConnection *conn;
	// GVariant *session_path;
	char *session_path;
	// tracks the timestamp when pending_hotkeys was last added to
	long add_to_pending_hotkeys_timestamp_us;
	GQueue *pending_hotkeys;
	// TODO: use the hash table that obs-hotkey already uses instead of
	// this array. I just want to get this working first
	GArray *registered_hotkeys;
} *state = NULL;

typedef struct obs_hotkey_portal_state obs_hotkey_portal_state_t;

bool obs_hotkey_portal_session_active()
{
	return state != NULL && state->session_path != NULL;
}

GString *get_formatted_sender(GDBusConnection *conn) {
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

GString *get_hotkey_portal_id(obs_hotkey_t *hotkey) {
	if (hotkey == NULL || hotkey->name == NULL) {
		return NULL;
	}

	GString *ret_val = g_string_new(hotkey->name);
	const char *id = NULL;
	switch(hotkey->registerer_type) {
	case OBS_HOTKEY_REGISTERER_FRONTEND:
		break;
	case OBS_HOTKEY_REGISTERER_SOURCE: {
		id = obs_source_get_id(hotkey->registerer);
		break;
	}
	case OBS_HOTKEY_REGISTERER_OUTPUT: {
		id = obs_output_get_id(hotkey->registerer);
		break;
	}
	case OBS_HOTKEY_REGISTERER_ENCODER: {
		id = obs_encoder_get_id(hotkey->registerer);
		break;
	}
	case OBS_HOTKEY_REGISTERER_SERVICE: {
		id = obs_service_get_id(hotkey->registerer);
		break;
	}
	default:
		break;
	}

	// if (id != NULL) {
	// 	g_print("Hotkey portal id: %s\n", ret_val->str);
	// 	g_string_append_printf(ret_val, ".%s", id);
	// 	g_print("Hotkey portal id2: %s\n", ret_val->str);
	// }
	return ret_val;
}

void obs_hotkey_portal_free() {
	if (state == NULL) {
		return;
	}
	g_dbus_connection_close_sync(state->conn, NULL, NULL);
	g_object_unref(state->conn);
	g_free(state->session_path);
	g_queue_free(state->pending_hotkeys);
	g_array_free(state->registered_hotkeys, true);
	g_free(state);
	state = NULL;
}

// register all pending hotkeys
static gboolean obs_hotkey_portal_finish_registration() {
	assert(obs_hotkey_portal_session_active());

	// check if at least 0.5 seconds have passed since the last time we added to pending_hotkeys
	long current_timestamp = g_get_monotonic_time();
	if (current_timestamp - state->add_to_pending_hotkeys_timestamp_us < 500000) {
		return G_SOURCE_CONTINUE;
	}

	// check if there are any pending hotkeys
	if (g_queue_is_empty(state->pending_hotkeys)) {
		return G_SOURCE_CONTINUE;
	}

	// bind some shortcuts
	GVariantBuilder shortcuts_builder;
	g_variant_builder_init(&shortcuts_builder, G_VARIANT_TYPE("a(sa{sv})"));
	for (obs_hotkey_t *hotkey = g_queue_pop_head(state->pending_hotkeys);
		hotkey != NULL;
		hotkey = g_queue_pop_head(state->pending_hotkeys))
	{
		// description
		GVariantBuilder vardict_builder;
		g_variant_builder_init(&vardict_builder, G_VARIANT_TYPE_VARDICT);
		g_variant_builder_add(&vardict_builder, "{sv}", "description", g_variant_new_string(hotkey->description));
		blog(LOG_INFO, "Registering hotkey with portal: %s: %s\n", hotkey->name, hotkey->description);

		// add shortcut to builder
		GVariant *data[] = {g_variant_new_string(get_hotkey_portal_id(hotkey)->str), g_variant_builder_end(&vardict_builder)};
		g_variant_builder_add_value(&shortcuts_builder, g_variant_new_tuple(data, 2));

		// add shortcut to registered list
		g_array_append_val(state->registered_hotkeys, *hotkey);
	}

	// options
	GVariantBuilder options_builder;
	char request_handle_token[64] = {};
	snprintf(request_handle_token, sizeof(request_handle_token), "obs_globalshortcuts_request%d", rand());

	g_variant_builder_init(&options_builder, G_VARIANT_TYPE_VARDICT);
	g_variant_builder_add(&options_builder, "{sv}", "handle_token", g_variant_new_string(request_handle_token));

	GVariant *options[] = {g_variant_new_object_path(state->session_path), g_variant_builder_end(&shortcuts_builder), g_variant_new_string(""), g_variant_builder_end(&options_builder)};
	g_dbus_connection_call(state->conn, PORTAL_NAME, PORTAL_PATH, SHORTCUTS_IFACE, "BindShortcuts", g_variant_new_tuple(options, 4), NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);

	return G_SOURCE_CONTINUE;
}

static void activated_cb(
	GDBusConnection* conn,
	const char *sender_name,
	const char *object_path,
	const char *interface_name,
	const char *signal_name,
	GVariant *parameters,
	gpointer user_data
) {
	UNUSED_PARAMETER(conn);
	UNUSED_PARAMETER(sender_name);
	UNUSED_PARAMETER(object_path);
	UNUSED_PARAMETER(interface_name);
	UNUSED_PARAMETER(signal_name);
	UNUSED_PARAMETER(parameters);
	UNUSED_PARAMETER(user_data);

	g_autofree char *session_path = NULL;
	g_autofree char *shortcut_name = NULL;
	uint64_t timestamp;
	g_autoptr(GVariant) options = NULL;

	g_variant_get(parameters, "(ost@a{sv})", &session_path, &shortcut_name, &timestamp, &options);
	blog(LOG_INFO, "Portal hotkey activated: %s\n", shortcut_name);

	// find shortcut id in registered list
	for (unsigned i = 0; i < state->registered_hotkeys->len; ++i) {
		obs_hotkey_t *hotkey = &g_array_index(state->registered_hotkeys, obs_hotkey_t, i);

		if (strcmp(hotkey->name, shortcut_name) != 0) {
			continue;
		}

		hotkey->func(hotkey->data, hotkey->id, hotkey, true);
		return;
	}
}

static void deactivated_cb(
	GDBusConnection* conn,
	const char *sender_name,
	const char *object_path,
	const char *interface_name,
	const char *signal_name,
	GVariant *parameters,
	gpointer user_data
) {
	UNUSED_PARAMETER(conn);
	UNUSED_PARAMETER(sender_name);
	UNUSED_PARAMETER(object_path);
	UNUSED_PARAMETER(interface_name);
	UNUSED_PARAMETER(signal_name);
	UNUSED_PARAMETER(parameters);
	UNUSED_PARAMETER(user_data);

	g_print("Button released!\n");
}

static void shortcuts_changed_cb(
	GDBusConnection* conn,
	const char *sender_name,
	const char *object_path,
	const char *interface_name,
	const char *signal_name,
	GVariant *parameters,
	gpointer user_data
) {
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

static void create_session_cb(
	GDBusConnection* conn,
	const char *sender_name,
	const char *object_path,
	const char *interface_name,
	const char *signal_name,
	GVariant *parameters,
	gpointer user_data
) {
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
	g_dbus_connection_signal_subscribe(conn, PORTAL_NAME, SHORTCUTS_IFACE, "Activated", PORTAL_PATH, NULL, G_DBUS_SIGNAL_FLAGS_NONE, activated_cb, NULL, NULL);
	g_dbus_connection_signal_subscribe(conn, PORTAL_NAME, SHORTCUTS_IFACE, "Deactivated", PORTAL_PATH, NULL, G_DBUS_SIGNAL_FLAGS_NONE, deactivated_cb, NULL, NULL);
	g_dbus_connection_signal_subscribe(conn, PORTAL_NAME, SHORTCUTS_IFACE, "ShortcutsChanged", PORTAL_PATH, NULL, G_DBUS_SIGNAL_FLAGS_NONE, shortcuts_changed_cb, NULL, NULL);

	state->session_path = bstrdup(session_handle);

	// schedule a timer to register all pending hotkeys every 0.1 seconds
	g_timeout_add(500, (GSourceFunc)obs_hotkey_portal_finish_registration, NULL);
}

void obs_hotkey_portal_init()
{
	// only allow this function to run once
	static bool init_run = false;
	if (init_run) {
		return;
	}
	init_run = true;

	g_autoptr(GDBusConnection) conn = NULL;
	GError *error = NULL;
	GVariant *options = NULL;
	g_autoptr(GString) request_handle_token = NULL;
	g_autoptr(GString) session_token = NULL;
	g_autoptr(GString) request_path = NULL;
	g_autoptr(GVariant) result = NULL;
	g_autoptr(GString) sender = NULL;
	GVariantBuilder options_builder;

	// get connection
	conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
	if (!conn) {
		g_printerr("Failed to connect to dbus: %s\n", error->message);
		return;
	}

	sender = get_formatted_sender(conn);

	// get tokens
	session_token = g_string_new(NULL);
	g_string_printf(session_token, "obs_globalshortcuts_session%d", rand());

	request_handle_token = g_string_new(NULL);
	g_string_printf(request_handle_token, "obs_globalshortcuts_request%d", rand());

	// connect to session parameters
	g_variant_builder_init(&options_builder, G_VARIANT_TYPE("a{sv}"));
	g_variant_builder_add(&options_builder, "{sv}", "handle_token", g_variant_new_string(request_handle_token->str));
	g_variant_builder_add(&options_builder, "{sv}", "session_handle_token", g_variant_new_string(session_token->str));
	options = g_variant_builder_end(&options_builder);

	// start to listen to request object in advance
	request_path = g_string_new(NULL);
	g_string_printf(request_path, "/org/freedesktop/portal/desktop/request/%s/%s", sender->str, request_handle_token->str);
	g_print("Listening to request at path: %s\n", request_path->str);
	g_dbus_connection_signal_subscribe(conn, PORTAL_NAME, REQUEST_IFACE, "Response", request_path->str, NULL, G_DBUS_SIGNAL_FLAGS_NONE, create_session_cb, NULL, NULL);

	// finally, connect to session
	result = g_dbus_connection_call_sync(conn, PORTAL_NAME, PORTAL_PATH, SHORTCUTS_IFACE, "CreateSession", g_variant_new_tuple(&options, 1), NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

	if (result == NULL) {
		blog(LOG_WARNING, "Failed to create global shortcuts portal session: %s", error->message);
		return;
	}

	// initialise state
	state = malloc(sizeof(obs_hotkey_portal_state_t));
	state->conn = conn;
	state->session_path = NULL;
	state->pending_hotkeys = g_queue_new();
	state->registered_hotkeys = g_array_new(false, false, sizeof(obs_hotkey_t));
}

void obs_hotkey_portal_register(obs_hotkey_t *hotkey)
{
	// TODO: check if there is a potential race condition here
	// TODO: check if this is even reachable in the first place
	if (state == NULL) {
		blog(LOG_WARNING, "Attempting to register a hotkey with the xdg-desktop-portal, but this feature has not been initialised");
		return;
	}

	blog(LOG_WARNING, "Registering hotkey with id %lu to xdg-desktop-portal", hotkey->id);

	// add to pending list
	g_queue_push_tail(state->pending_hotkeys, hotkey);
	state->add_to_pending_hotkeys_timestamp_us = g_get_monotonic_time();
}

void obs_hotkey_portal_unregister(obs_hotkey_id id) {
	if (state == NULL) {
		blog(LOG_WARNING, "Attempting to unregister a hotkey with the xdg-desktop-portal, but this feature has not been initialised");
		return;
	}

	blog(LOG_WARNING, "Unregistering hotkey with id %lu from xdg-desktop-portal", id);

	// remove hotkey from registered list
	for (unsigned i = 0; i < state->registered_hotkeys->len; ++i) {
		obs_hotkey_t *hotkey = &g_array_index(state->registered_hotkeys, obs_hotkey_t, i);
		if (hotkey->id == id) {
			g_array_remove_index(state->registered_hotkeys, i);
			break;
		}
	}

	// pending list
	for (GList *l = state->pending_hotkeys->head; l != NULL; l = l->next) {
		obs_hotkey_t *hotkey = (obs_hotkey_t *)l->data;
		if (hotkey->id == id) {
			g_queue_delete_link(state->pending_hotkeys, l);
			break;
		}
	}
}
