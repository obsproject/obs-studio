/* camera-portal.c
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

#include "pipewire.h"
#include "portal.h"

#include <util/dstr.h>

#include <fcntl.h>
#include <unistd.h>
#include <gio/gio.h>
#include <gio/gunixfdlist.h>
#include <spa/debug/dict.h>
#include <spa/node/keys.h>
#include <spa/pod/iter.h>
#include <spa/utils/defs.h>
#include <spa/utils/keys.h>

struct camera_portal_source {
	obs_source_t *source;
	obs_data_t *settings;

	obs_pipewire_stream *obs_pw_stream;
	char *device_id;
};

/* ------------------------------------------------- */

struct pw_portal_connection {
	obs_pipewire *obs_pw;
	GHashTable *devices;
	GCancellable *cancellable;
	GPtrArray *sources;
	bool initializing;
};

struct pw_portal_connection *connection = NULL;

static void pw_portal_connection_free(struct pw_portal_connection *connection)
{
	if (!connection)
		return;

	g_cancellable_cancel(connection->cancellable);

	g_clear_pointer(&connection->devices, g_hash_table_destroy);
	g_clear_pointer(&connection->obs_pw, obs_pipewire_destroy);
	g_clear_pointer(&connection->sources, g_ptr_array_unref);
	g_clear_object(&connection->cancellable);
	bfree(connection);
}

static GDBusProxy *camera_proxy = NULL;

static void ensure_camera_portal_proxy(void)
{
	g_autoptr(GError) error = NULL;
	if (!camera_proxy) {
		camera_proxy = g_dbus_proxy_new_sync(
			portal_get_dbus_connection(), G_DBUS_PROXY_FLAGS_NONE,
			NULL, "org.freedesktop.portal.Desktop",
			"/org/freedesktop/portal/desktop",
			"org.freedesktop.portal.Camera", NULL, &error);

		if (error) {
			blog(LOG_WARNING,
			     "[portals] Error retrieving D-Bus proxy: %s",
			     error->message);
			return;
		}
	}
}

static GDBusProxy *get_camera_portal_proxy(void)
{
	ensure_camera_portal_proxy();
	return camera_proxy;
}

static uint32_t get_camera_version(void)
{
	g_autoptr(GVariant) cached_version = NULL;
	uint32_t version;

	ensure_camera_portal_proxy();

	if (!camera_proxy)
		return 0;

	cached_version =
		g_dbus_proxy_get_cached_property(camera_proxy, "version");
	version = cached_version ? g_variant_get_uint32(cached_version) : 0;

	return version;
}

/* ------------------------------------------------- */

struct camera_device {
	uint32_t id;
	struct pw_properties *properties;
	struct pw_proxy *proxy;
	struct spa_hook proxy_listener;
};

static struct camera_device *
camera_device_new(uint32_t id, const struct spa_dict *properties)
{
	struct camera_device *device = bzalloc(sizeof(struct camera_device));
	device->id = id;
	device->properties = properties ? pw_properties_new_dict(properties)
					: NULL;
	return device;
}

static void camera_device_free(struct camera_device *device)
{
	if (!device)
		return;

	g_clear_pointer(&device->proxy, pw_proxy_destroy);
	g_clear_pointer(&device->properties, pw_properties_free);
	bfree(device);
}

/* ------------------------------------------------- */

static bool update_device_id(struct camera_portal_source *camera_source,
			     const char *new_device_id)
{
	if (strcmp(camera_source->device_id, new_device_id) == 0)
		return false;

	g_clear_pointer(&camera_source->device_id, bfree);
	camera_source->device_id = bstrdup(new_device_id);

	return true;
}

static void stream_camera(struct camera_portal_source *camera_source)
{
	struct camera_device *device;

	g_clear_pointer(&camera_source->obs_pw_stream,
			obs_pipewire_stream_destroy);

	device = g_hash_table_lookup(connection->devices,
				     camera_source->device_id);

	if (!device)
		return;

	blog(LOG_INFO, "[camera-portal] selected camera '%s'",
	     camera_source->device_id);

	camera_source->obs_pw_stream = obs_pipewire_connect_stream(
		connection->obs_pw, camera_source->source, device->id,
		"OBS PipeWire Camera",
		pw_properties_new(PW_KEY_MEDIA_TYPE, "Video",
				  PW_KEY_MEDIA_CATEGORY, "Capture",
				  PW_KEY_MEDIA_ROLE, "Camera", NULL));
}

static bool device_selected(void *data, obs_properties_t *props,
			    obs_property_t *property, obs_data_t *settings)
{
	struct camera_portal_source *camera_source = data;
	const char *device_id;

	device_id = obs_data_get_string(settings, "device_id");

	if (update_device_id(camera_source, device_id))
		stream_camera(camera_source);

	return true;
}

static void populate_cameras_list(struct camera_portal_source *camera_source,
				  obs_property_t *device_list)
{
	struct camera_device *device;
	GHashTableIter iter;
	const char *device_id;
	bool device_found;

	if (!connection)
		return;

	obs_property_list_clear(device_list);

	device_found = false;
	g_hash_table_iter_init(&iter, connection->devices);
	while (g_hash_table_iter_next(&iter, (gpointer *)&device_id,
				      (gpointer *)&device)) {
		const char *device_name;

		device_name = pw_properties_get(device->properties,
						PW_KEY_NODE_DESCRIPTION);

		obs_property_list_add_string(device_list, device_name,
					     device_id);

		device_found |= strcmp(device_id, camera_source->device_id) ==
				0;
	}

	if (!device_found && camera_source->device_id) {
		size_t device_index;
		device_index = obs_property_list_add_string(
			device_list, camera_source->device_id,
			camera_source->device_id);
		obs_property_list_item_disable(device_list, device_index, true);
	}
}

/* ------------------------------------------------- */

static void on_proxy_removed_cb(void *data)
{
	struct camera_device *device = data;
	pw_proxy_destroy(device->proxy);
}

static void on_destroy_proxy_cb(void *data)
{
	struct camera_device *device = data;

	spa_hook_remove(&device->proxy_listener);

	device->proxy = NULL;
}

static const struct pw_proxy_events proxy_events = {
	PW_VERSION_PROXY_EVENTS,
	.removed = on_proxy_removed_cb,
	.destroy = on_destroy_proxy_cb,
};

static void on_registry_global_cb(void *user_data, uint32_t id,
				  uint32_t permissions, const char *type,
				  uint32_t version,
				  const struct spa_dict *props)
{
	UNUSED_PARAMETER(user_data);
	UNUSED_PARAMETER(permissions);
	UNUSED_PARAMETER(version);

	struct camera_device *device;
	struct pw_registry *registry;
	const char *device_id;

	if (strcmp(type, PW_TYPE_INTERFACE_Node) != 0)
		return;

	registry = obs_pipewire_get_registry(connection->obs_pw);
	device_id = spa_dict_lookup(props, SPA_KEY_NODE_NAME);

	device = camera_device_new(id, props);
	device->proxy = pw_registry_bind(registry, id, type, version, 0);
	if (!device->proxy) {
		blog(LOG_WARNING, "[pipewire-camera] Failed to bind device %s",
		     device_id);
		bfree(device);
		return;
	}
	pw_proxy_add_listener(device->proxy, &device->proxy_listener,
			      &proxy_events, device);

	g_hash_table_insert(connection->devices, bstrdup(device_id), device);
}

static void on_registry_global_remove_cb(void *user_data, uint32_t id)
{
	UNUSED_PARAMETER(user_data);

	struct camera_device *device;
	const char *device_id;
	GHashTableIter iter;

	g_hash_table_iter_init(&iter, connection->devices);
	while (g_hash_table_iter_next(&iter, (gpointer *)&device_id,
				      (gpointer *)&device)) {
		if (device->id != id)
			continue;
		g_hash_table_iter_remove(&iter);
	}
}

static const struct pw_registry_events registry_events = {
	PW_VERSION_REGISTRY_EVENTS,
	.global = on_registry_global_cb,
	.global_remove = on_registry_global_remove_cb,
};

/* ------------------------------------------------- */

static void on_pipewire_remote_opened_cb(GObject *source, GAsyncResult *res,
					 void *user_data)
{
	UNUSED_PARAMETER(user_data);

	g_autoptr(GUnixFDList) fd_list = NULL;
	g_autoptr(GVariant) result = NULL;
	g_autoptr(GError) error = NULL;
	int pipewire_fd;
	int fd_index;

	result = g_dbus_proxy_call_with_unix_fd_list_finish(
		G_DBUS_PROXY(source), &fd_list, res, &error);
	if (error) {
		if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
			blog(LOG_ERROR,
			     "[camera-portal] Error retrieving PipeWire fd: %s",
			     error->message);
		return;
	}

	g_variant_get(result, "(h)", &fd_index, &error);

	pipewire_fd = g_unix_fd_list_get(fd_list, fd_index, &error);
	if (error) {
		if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
			blog(LOG_ERROR,
			     "[camera-portal] Error retrieving PipeWire fd: %s",
			     error->message);
		return;
	}

	connection->obs_pw =
		obs_pipewire_create(pipewire_fd, &registry_events, connection);

	obs_pipewire_roundtrip(connection->obs_pw);

	for (size_t i = 0; i < connection->sources->len; i++) {
		struct camera_portal_source *camera_source =
			g_ptr_array_index(connection->sources, i);
		stream_camera(camera_source);
	}
}

static void open_pipewire_remote(void)
{
	GVariantBuilder builder;

	g_variant_builder_init(&builder, G_VARIANT_TYPE_VARDICT);

	g_dbus_proxy_call_with_unix_fd_list(get_camera_portal_proxy(),
					    "OpenPipeWireRemote",
					    g_variant_new("(a{sv})", &builder),
					    G_DBUS_CALL_FLAGS_NONE, -1, NULL,
					    connection->cancellable,
					    on_pipewire_remote_opened_cb, NULL);
}

/* ------------------------------------------------- */

static void on_access_camera_response_received_cb(GVariant *parameters,
						  void *user_data)
{
	UNUSED_PARAMETER(user_data);

	g_autoptr(GVariant) result = NULL;
	uint32_t response;

	g_variant_get(parameters, "(u@a{sv})", &response, &result);

	if (response != 0) {
		blog(LOG_WARNING,
		     "[camera-portal] Failed to create session, denied or cancelled by user");
		return;
	}

	blog(LOG_INFO, "[camera-portal] Successfully accessed cameras");

	open_pipewire_remote();
}

static void on_access_camera_finished_cb(GObject *source, GAsyncResult *res,
					 void *user_data)
{
	UNUSED_PARAMETER(user_data);

	g_autoptr(GVariant) result = NULL;
	g_autoptr(GError) error = NULL;

	result = g_dbus_proxy_call_finish(G_DBUS_PROXY(source), res, &error);
	if (error) {
		if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
			blog(LOG_ERROR,
			     "[camera-portal] Error accessing camera: %s",
			     error->message);
		return;
	}
}

static void access_camera(struct camera_portal_source *camera_source)
{
	GVariantBuilder builder;
	char *request_token;
	char *request_path;

	if (connection && connection->obs_pw) {
		stream_camera(camera_source);
		return;
	}

	if (!connection) {
		connection = bzalloc(sizeof(struct pw_portal_connection));
		connection->devices = g_hash_table_new_full(
			g_str_hash, g_str_equal, bfree,
			(GDestroyNotify)camera_device_free);
		connection->cancellable = g_cancellable_new();
		connection->sources = g_ptr_array_new();
		connection->initializing = false;
	}

	g_ptr_array_add(connection->sources, camera_source);

	if (connection->initializing)
		return;

	portal_create_request_path(&request_path, &request_token);

	portal_signal_subscribe(request_path, NULL,
				on_access_camera_response_received_cb, NULL);

	g_variant_builder_init(&builder, G_VARIANT_TYPE_VARDICT);
	g_variant_builder_add(&builder, "{sv}", "handle_token",
			      g_variant_new_string(request_token));

	g_dbus_proxy_call(get_camera_portal_proxy(), "AccessCamera",
			  g_variant_new("(a{sv})", &builder),
			  G_DBUS_CALL_FLAGS_NONE, -1, connection->cancellable,
			  on_access_camera_finished_cb, NULL);

	connection->initializing = true;

	bfree(request_token);
	bfree(request_path);
}

/* obs_source_info methods */

static const char *pipewire_camera_get_name(void *data)
{
	UNUSED_PARAMETER(data);
	return obs_module_text("PipeWireCamera");
}

static void *pipewire_camera_create(obs_data_t *settings, obs_source_t *source)
{
	struct camera_portal_source *camera_source;

	camera_source = bzalloc(sizeof(struct camera_portal_source));
	camera_source->source = source;
	camera_source->device_id =
		bstrdup(obs_data_get_string(settings, "device_id"));

	access_camera(camera_source);

	return camera_source;
}

static void pipewire_camera_destroy(void *data)
{
	struct camera_portal_source *camera_source = data;

	if (connection)
		g_ptr_array_remove(connection->sources, camera_source);

	g_clear_pointer(&camera_source->obs_pw_stream,
			obs_pipewire_stream_destroy);
	g_clear_pointer(&camera_source->device_id, bfree);

	bfree(camera_source);
}

static void pipewire_camera_get_defaults(obs_data_t *settings)
{
	obs_data_set_default_string(settings, "device_id", NULL);
}

static obs_properties_t *pipewire_camera_get_properties(void *data)
{
	struct camera_portal_source *camera_source = data;
	obs_properties_t *properties;
	obs_property_t *device_list;

	properties = obs_properties_create();

	device_list = obs_properties_add_list(
		properties, "device_id",
		obs_module_text("PipeWireCameraDevice"), OBS_COMBO_TYPE_LIST,
		OBS_COMBO_FORMAT_STRING);

	populate_cameras_list(camera_source, device_list);

	obs_property_set_modified_callback2(device_list, device_selected,
					    camera_source);

	return properties;
}

static void pipewire_camera_update(void *data, obs_data_t *settings)
{
	struct camera_portal_source *camera_source = data;
	const char *device_id;

	device_id = obs_data_get_string(settings, "device_id");

	if (update_device_id(camera_source, device_id))
		stream_camera(camera_source);
}

static void pipewire_camera_show(void *data)
{
	struct camera_portal_source *camera_source = data;

	if (camera_source->obs_pw_stream)
		obs_pipewire_stream_show(camera_source->obs_pw_stream);
}

static void pipewire_camera_hide(void *data)
{
	struct camera_portal_source *camera_source = data;

	if (camera_source->obs_pw_stream)
		obs_pipewire_stream_hide(camera_source->obs_pw_stream);
}

static uint32_t pipewire_camera_get_width(void *data)
{
	struct camera_portal_source *camera_source = data;

	if (camera_source->obs_pw_stream)
		return obs_pipewire_stream_get_width(
			camera_source->obs_pw_stream);
	else
		return 0;
}

static uint32_t pipewire_camera_get_height(void *data)
{
	struct camera_portal_source *camera_source = data;

	if (camera_source->obs_pw_stream)
		return obs_pipewire_stream_get_height(
			camera_source->obs_pw_stream);
	else
		return 0;
}

void camera_portal_load(void)
{
	const struct obs_source_info pipewire_camera_info = {
		.id = "pipewire-camera-source",
		.type = OBS_SOURCE_TYPE_INPUT,
		.output_flags = OBS_SOURCE_ASYNC_VIDEO,
		.get_name = pipewire_camera_get_name,
		.create = pipewire_camera_create,
		.destroy = pipewire_camera_destroy,
		.get_defaults = pipewire_camera_get_defaults,
		.get_properties = pipewire_camera_get_properties,
		.update = pipewire_camera_update,
		.show = pipewire_camera_show,
		.hide = pipewire_camera_hide,
		.get_width = pipewire_camera_get_width,
		.get_height = pipewire_camera_get_height,
		.icon_type = OBS_ICON_TYPE_CAMERA,
	};
	obs_register_source(&pipewire_camera_info);
}

void camera_portal_unload(void)
{
	g_clear_pointer(&connection, pw_portal_connection_free);
}
