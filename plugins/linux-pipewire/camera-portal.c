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

#include "formats.h"
#include "portal.h"

#include <util/dstr.h>

#include <fcntl.h>
#include <unistd.h>
#include <gio/gio.h>
#include <gio/gunixfdlist.h>
#include <spa/node/keys.h>
#include <spa/pod/iter.h>
#include <spa/pod/parser.h>
#include <spa/param/props.h>
#include <spa/utils/defs.h>
#include <spa/utils/keys.h>
#include <spa/utils/result.h>

struct camera_portal_source {
	obs_source_t *source;
	obs_data_t *settings;

	obs_pipewire_stream *obs_pw_stream;
	char *device_id;

	struct {
		struct spa_rectangle rect;
		bool set;
	} resolution;

	struct {
		struct spa_fraction fraction;
		bool set;
	} framerate;
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
		camera_proxy = g_dbus_proxy_new_sync(portal_get_dbus_connection(), G_DBUS_PROXY_FLAGS_NONE, NULL,
						     "org.freedesktop.portal.Desktop",
						     "/org/freedesktop/portal/desktop", "org.freedesktop.portal.Camera",
						     NULL, &error);

		if (error) {
			blog(LOG_WARNING, "[portals] Error retrieving D-Bus proxy: %s", error->message);
			return;
		}
	}
}

static GDBusProxy *get_camera_portal_proxy(void)
{
	ensure_camera_portal_proxy();
	return camera_proxy;
}

/* ------------------------------------------------- */

struct camera_device {
	uint32_t id;
	struct pw_properties *properties;
	struct pw_proxy *proxy;
	struct spa_hook proxy_listener;

	struct pw_node *node;
	struct spa_hook node_listener;

	struct pw_node_info *info;

	uint32_t changed;
	struct spa_list pending_list;
	struct spa_list param_list;
	int pending_sync;
};

struct param {
	uint32_t id;
	int32_t seq;
	struct spa_list link;
	struct spa_pod *param;
};

static uint32_t clear_params(struct spa_list *param_list, uint32_t id)
{
	struct param *p, *t;
	uint32_t count = 0;

	spa_list_for_each_safe(p, t, param_list, link)
	{
		if (id == SPA_ID_INVALID || p->id == id) {
			spa_list_remove(&p->link);
			free(p);
			count++;
		}
	}
	return count;
}

static struct param *add_param(struct spa_list *params, int seq, uint32_t id, const struct spa_pod *param)
{
	struct param *p;

	if (id == SPA_ID_INVALID) {
		if (param == NULL || !spa_pod_is_object(param)) {
			errno = EINVAL;
			return NULL;
		}
		id = SPA_POD_OBJECT_ID(param);
	}

	p = malloc(sizeof(*p) + (param != NULL ? SPA_POD_SIZE(param) : 0));
	if (p == NULL)
		return NULL;

	p->id = id;
	p->seq = seq;
	if (param != NULL) {
		p->param = SPA_PTROFF(p, sizeof(*p), struct spa_pod);
		memcpy(p->param, param, SPA_POD_SIZE(param));
	} else {
		clear_params(params, id);
		p->param = NULL;
	}
	spa_list_append(params, &p->link);

	return p;
}

static void object_update_params(struct spa_list *param_list, struct spa_list *pending_list, uint32_t n_params,
				 struct spa_param_info *params)
{
	struct param *p, *t;
	uint32_t i;

	for (i = 0; i < n_params; i++) {
		spa_list_for_each_safe(p, t, pending_list, link)
		{
			if (p->id == params[i].id && p->seq != params[i].seq && p->param != NULL) {
				spa_list_remove(&p->link);
				free(p);
			}
		}
	}
	spa_list_consume(p, pending_list, link)
	{
		spa_list_remove(&p->link);
		if (p->param == NULL) {
			clear_params(param_list, p->id);
			free(p);
		} else {
			spa_list_append(param_list, &p->link);
		}
	}
}

static struct camera_device *camera_device_new(uint32_t id, const struct spa_dict *properties)
{
	struct camera_device *device = bzalloc(sizeof(struct camera_device));
	device->id = id;
	device->properties = properties ? pw_properties_new_dict(properties) : NULL;
	spa_list_init(&device->pending_list);
	spa_list_init(&device->param_list);
	return device;
}

static void camera_device_free(struct camera_device *device)
{
	if (!device)
		return;

	clear_params(&device->pending_list, SPA_ID_INVALID);
	clear_params(&device->param_list, SPA_ID_INVALID);
	g_clear_pointer(&device->info, pw_node_info_free);
	g_clear_pointer(&device->proxy, pw_proxy_destroy);
	g_clear_pointer(&device->properties, pw_properties_free);
	bfree(device);
}

/* ------------------------------------------------- */

static bool update_device_id(struct camera_portal_source *camera_source, const char *new_device_id)
{
	if (strcmp(camera_source->device_id, new_device_id) == 0)
		return false;

	g_clear_pointer(&camera_source->device_id, bfree);
	camera_source->device_id = bstrdup(new_device_id);

	return true;
}

static void stream_camera(struct camera_portal_source *camera_source)
{
	struct obs_pipwire_connect_stream_info connect_info;
	const struct spa_rectangle *resolution = NULL;
	const struct spa_fraction *framerate = NULL;
	struct camera_device *device;

	g_clear_pointer(&camera_source->obs_pw_stream, obs_pipewire_stream_destroy);

	device = g_hash_table_lookup(connection->devices, camera_source->device_id);

	if (!device)
		return;

	blog(LOG_INFO, "[camera-portal] streaming camera '%s'", camera_source->device_id);

	if (camera_source->resolution.set)
		resolution = &camera_source->resolution.rect;
	if (camera_source->framerate.set)
		framerate = &camera_source->framerate.fraction;

	connect_info = (struct obs_pipwire_connect_stream_info){
		.stream_name = "OBS PipeWire Camera",
		.stream_properties = pw_properties_new(PW_KEY_MEDIA_TYPE, "Video", PW_KEY_MEDIA_CATEGORY, "Capture",
						       PW_KEY_MEDIA_ROLE, "Camera", NULL),
		.video = {
			.resolution = resolution,
			.framerate = framerate,
		}};

	camera_source->obs_pw_stream =
		obs_pipewire_connect_stream(connection->obs_pw, camera_source->source, device->id, &connect_info);
}

static void camera_format_list(struct camera_device *dev, obs_property_t *prop)
{
	struct param *p;
	enum video_format last_format = UINT32_MAX;

	obs_property_list_clear(prop);

	spa_list_for_each(p, &dev->param_list, link)
	{
		struct obs_pw_video_format obs_pw_video_format;
		uint32_t media_type, media_subtype, format;

		if (p->id != SPA_PARAM_EnumFormat || p->param == NULL)
			continue;

		if (spa_format_parse(p->param, &media_type, &media_subtype) < 0)
			continue;
		if (media_type != SPA_MEDIA_TYPE_video)
			continue;
		if (media_subtype == SPA_MEDIA_SUBTYPE_raw) {
			if (spa_pod_parse_object(p->param, SPA_TYPE_OBJECT_Format, NULL, SPA_FORMAT_VIDEO_format,
						 SPA_POD_Id(&format)) < 0)
				continue;
		} else {
			format = SPA_VIDEO_FORMAT_ENCODED;
		}

		if (!obs_pw_video_format_from_spa_format(format, &obs_pw_video_format))
			continue;

		if (obs_pw_video_format.video_format == last_format)
			continue;

		last_format = obs_pw_video_format.video_format;

		obs_property_list_add_int(prop, obs_pw_video_format.pretty_name, format);
	}
}

static bool control_changed(void *data, obs_properties_t *props, obs_property_t *prop, obs_data_t *settings)
{
	UNUSED_PARAMETER(props);

	struct camera_device *dev;
	const char *device_id;
	uint32_t id;
	char buf[1024];
	struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buf, sizeof(buf));
	struct spa_pod_frame f[1];
	struct spa_pod *param;

	device_id = obs_data_get_string(settings, "device_id");

	dev = g_hash_table_lookup(connection->devices, device_id);
	if (dev == NULL) {
		blog(LOG_ERROR, "unknown device %s", device_id);
		return false;
	}

	id = SPA_PTR_TO_UINT32(data);

	spa_pod_builder_push_object(&b, &f[0], SPA_TYPE_OBJECT_Props, SPA_PARAM_Props);

	switch (obs_property_get_type(prop)) {
	case OBS_PROPERTY_BOOL: {
		bool val = obs_data_get_bool(settings, obs_property_name(prop));
		spa_pod_builder_add(&b, id, SPA_POD_Bool(val), 0);
		break;
	}
	case OBS_PROPERTY_FLOAT: {
		float val = obs_data_get_double(settings, obs_property_name(prop));
		spa_pod_builder_add(&b, id, SPA_POD_Float(val), 0);
		break;
	}
	case OBS_PROPERTY_INT:
	case OBS_PROPERTY_LIST: {
		int val = obs_data_get_int(settings, obs_property_name(prop));
		spa_pod_builder_add(&b, id, SPA_POD_Int(val), 0);
		break;
	}
	default:
		blog(LOG_ERROR, "unknown property type for %s", obs_property_name(prop));
		return false;
	}
	param = spa_pod_builder_pop(&b, &f[0]);

	pw_node_set_param((struct pw_node *)dev->proxy, SPA_PARAM_Props, 0, param);

	return true;
}

static inline void add_control_property(obs_properties_t *props, obs_data_t *settings, struct camera_device *dev,
					struct param *p)
{
	UNUSED_PARAMETER(dev);

	const struct spa_pod *type, *pod, *labels = NULL;
	uint32_t id, n_vals, choice, container = SPA_ID_INVALID;
	obs_property_t *prop = NULL;
	const char *name;

	if (spa_pod_parse_object(p->param, SPA_TYPE_OBJECT_PropInfo, NULL, SPA_PROP_INFO_id, SPA_POD_Id(&id),
				 SPA_PROP_INFO_description, SPA_POD_OPT_String(&name), SPA_PROP_INFO_type,
				 SPA_POD_PodChoice(&type), SPA_PROP_INFO_container, SPA_POD_OPT_Id(&container),
				 SPA_PROP_INFO_labels, SPA_POD_OPT_PodStruct(&labels)) < 0)
		return;

	pod = spa_pod_get_values(type, &n_vals, &choice);

	container = container != SPA_ID_INVALID ? container : SPA_POD_TYPE(pod);

	switch (SPA_POD_TYPE(pod)) {
	case SPA_TYPE_Int: {
		int32_t *vals = SPA_POD_BODY(pod);
		int32_t min, max, def, step;
		if (n_vals < 1)
			return;
		def = vals[0];
		if (choice == SPA_CHOICE_Enum) {
			struct spa_pod_parser prs;
			struct spa_pod_frame f;

			if (labels == NULL)
				return;

			prop = obs_properties_add_list(props, (char *)name, (char *)name, OBS_COMBO_TYPE_LIST,
						       OBS_COMBO_FORMAT_INT);

			spa_pod_parser_pod(&prs, (struct spa_pod *)labels);
			if (spa_pod_parser_push_struct(&prs, &f) < 0)
				return;

			while (1) {
				int32_t id;
				const char *desc;
				if (spa_pod_parser_get_int(&prs, &id) < 0 || spa_pod_parser_get_string(&prs, &desc) < 0)
					break;
				obs_property_list_add_int(prop, (char *)desc, id);
			}
		} else {
			min = n_vals > 1 ? vals[1] : def;
			max = n_vals > 2 ? vals[2] : def;
			step = n_vals > 3 ? vals[3] : (max - min) / 256.0f;
			prop = obs_properties_add_int_slider(props, (char *)name, (char *)name, min, max, step);
		}
		obs_data_set_default_int(settings, (char *)name, def);
		obs_property_set_modified_callback2(prop, control_changed, SPA_UINT32_TO_PTR(id));
		break;
	}
	case SPA_TYPE_Bool: {
		int32_t *vals = SPA_POD_BODY(pod);
		if (n_vals < 1)
			return;
		prop = obs_properties_add_bool(props, (char *)name, (char *)name);
		obs_data_set_default_bool(settings, (char *)name, vals[0]);
		obs_property_set_modified_callback2(prop, control_changed, SPA_UINT32_TO_PTR(id));
		break;
	}
	case SPA_TYPE_Float: {
		float *vals = SPA_POD_BODY(pod);
		float min, max, def, step;
		if (n_vals < 1)
			return;
		def = vals[0];
		min = n_vals > 1 ? vals[1] : def;
		max = n_vals > 2 ? vals[2] : def;
		step = n_vals > 3 ? vals[3] : (max - min) / 256.0f;
		prop = obs_properties_add_float_slider(props, (char *)name, (char *)name, min, max, step);
		obs_data_set_default_double(settings, (char *)name, def);
		obs_property_set_modified_callback2(prop, control_changed, SPA_UINT32_TO_PTR(id));
		break;
	}
	default:
		break;
	}
}

static void camera_update_controls(struct camera_device *dev, obs_properties_t *props, obs_data_t *settings)
{
	struct param *p;
	spa_list_for_each(p, &dev->param_list, link)
	{
		if (p->id != SPA_PARAM_PropInfo || p->param == NULL)
			continue;
		add_control_property(props, settings, dev, p);
	}
}

static bool device_selected(void *data, obs_properties_t *props, obs_property_t *property, obs_data_t *settings)
{
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(property);

	struct camera_portal_source *camera_source = data;
	const char *device_id;
	struct camera_device *device;
	obs_properties_t *new_control_properties;

	device_id = obs_data_get_string(settings, "device_id");
	blog(LOG_INFO, "[camera-portal] selected camera '%s'", device_id);

	device = g_hash_table_lookup(connection->devices, device_id);
	if (device == NULL)
		return false;

	if (update_device_id(camera_source, device_id))
		stream_camera(camera_source);

	blog(LOG_INFO, "[camera-portal] Updating pixel formats");

	property = obs_properties_get(props, "pixelformat");
	new_control_properties = obs_properties_create();
	obs_properties_remove_by_name(props, "controls");

	camera_format_list(device, property);
	camera_update_controls(device, new_control_properties, settings);

	obs_properties_add_group(props, "controls", obs_module_text("CameraControls"), OBS_GROUP_NORMAL,
				 new_control_properties);

	obs_property_modified(property, settings);

	return true;
}

static int sort_resolutions(gconstpointer a, gconstpointer b)
{
	const struct spa_rectangle *resolution_a = a;
	const struct spa_rectangle *resolution_b = b;
	int64_t area_a = resolution_a->width * resolution_a->height;
	int64_t area_b = resolution_b->width * resolution_b->height;

	return area_a - area_b;
}

static void resolution_list(struct camera_device *dev, uint32_t pixelformat, obs_property_t *prop)
{
	struct spa_rectangle last_resolution = SPA_RECTANGLE(0, 0);
	g_autoptr(GArray) resolutions = NULL;
	struct param *p;
	obs_data_t *data;

	resolutions = g_array_new(FALSE, FALSE, sizeof(struct spa_rectangle));

	spa_list_for_each(p, &dev->param_list, link)
	{
		struct spa_rectangle resolution;
		uint32_t media_type, media_subtype, format;

		if (p->id != SPA_PARAM_EnumFormat || p->param == NULL)
			continue;

		if (spa_format_parse(p->param, &media_type, &media_subtype) < 0)
			continue;
		if (media_type != SPA_MEDIA_TYPE_video)
			continue;
		if (media_subtype == SPA_MEDIA_SUBTYPE_raw) {
			if (spa_pod_parse_object(p->param, SPA_TYPE_OBJECT_Format, NULL, SPA_FORMAT_VIDEO_format,
						 SPA_POD_Id(&format)) < 0)
				continue;
		} else {
			format = SPA_VIDEO_FORMAT_ENCODED;
		}

		if (format != pixelformat)
			continue;

		if (spa_pod_parse_object(p->param, SPA_TYPE_OBJECT_Format, NULL, SPA_FORMAT_VIDEO_size,
					 SPA_POD_OPT_Rectangle(&resolution)) < 0)
			continue;

		if (resolution.width == last_resolution.width && resolution.height == last_resolution.height)
			continue;

		last_resolution = resolution;
		g_array_append_val(resolutions, resolution);
	}

	g_array_sort(resolutions, sort_resolutions);

	obs_property_list_clear(prop);

	data = obs_data_create();
	for (size_t i = 0; i < resolutions->len; i++) {
		const struct spa_rectangle *resolution = &g_array_index(resolutions, struct spa_rectangle, i);
		struct dstr str = {};

		dstr_printf(&str, "%ux%u", resolution->width, resolution->height);

		obs_data_set_int(data, "width", resolution->width);
		obs_data_set_int(data, "height", resolution->height);

		obs_property_list_add_string(prop, str.array, obs_data_get_json(data));

		dstr_free(&str);
	}
	obs_data_release(data);
}

/*
 * Format selected callback
 */
static bool format_selected(void *data, obs_properties_t *properties, obs_property_t *property, obs_data_t *settings)
{
	UNUSED_PARAMETER(property);
	UNUSED_PARAMETER(settings);

	struct camera_portal_source *camera_source = data;
	struct camera_device *device;
	obs_property_t *resolution;

	blog(LOG_INFO, "[camera-portal] Selected format for '%s'", camera_source->device_id);

	device = g_hash_table_lookup(connection->devices, camera_source->device_id);
	if (device == NULL)
		return false;

	resolution = obs_properties_get(properties, "resolution");
	resolution_list(device, obs_data_get_int(settings, "pixelformat"), resolution);

	return true;
}

static int compare_framerates(gconstpointer a, gconstpointer b)
{
	const struct spa_fraction *framerate_a = a;
	const struct spa_fraction *framerate_b = b;
	double da = framerate_a->num / (double)framerate_a->denom;
	double db = framerate_b->num / (double)framerate_b->denom;

	return da - db;
}

static void framerate_list(struct camera_device *dev, uint32_t pixelformat, const struct spa_rectangle *resolution,
			   obs_property_t *prop)
{
	g_autoptr(GArray) framerates = NULL;
	struct param *p;
	obs_data_t *data;

	framerates = g_array_new(FALSE, FALSE, sizeof(struct spa_fraction));

	spa_list_for_each(p, &dev->param_list, link)
	{
		const struct spa_fraction *framerate_values;
		enum spa_choice_type choice;
		const struct spa_pod_prop *prop;
		struct spa_rectangle this_resolution;
		struct spa_pod *framerate_pod;
		uint32_t media_subtype;
		uint32_t media_type;
		uint32_t n_framerates;
		uint32_t format;

		if (p->id != SPA_PARAM_EnumFormat || p->param == NULL)
			continue;

		if (spa_format_parse(p->param, &media_type, &media_subtype) < 0)
			continue;
		if (media_type != SPA_MEDIA_TYPE_video)
			continue;
		if (media_subtype == SPA_MEDIA_SUBTYPE_raw) {
			if (spa_pod_parse_object(p->param, SPA_TYPE_OBJECT_Format, NULL, SPA_FORMAT_VIDEO_format,
						 SPA_POD_Id(&format)) < 0)
				continue;
		} else {
			format = SPA_VIDEO_FORMAT_ENCODED;
		}

		if (format != pixelformat)
			continue;

		if (spa_pod_parse_object(p->param, SPA_TYPE_OBJECT_Format, NULL, SPA_FORMAT_VIDEO_size,
					 SPA_POD_OPT_Rectangle(&this_resolution)) < 0)
			continue;

		if (this_resolution.width != resolution->width || this_resolution.height != resolution->height)
			continue;

		prop = spa_pod_find_prop(p->param, NULL, SPA_FORMAT_VIDEO_framerate);
		if (!prop)
			continue;

		framerate_pod = spa_pod_get_values(&prop->value, &n_framerates, &choice);
		if (framerate_pod->type != SPA_TYPE_Fraction) {
			blog(LOG_WARNING, "Framerate is not a fraction - something is wrong");
			continue;
		}

		framerate_values = SPA_POD_BODY(framerate_pod);

		switch (choice) {
		case SPA_CHOICE_None:
			g_array_append_val(framerates, framerate_values[0]);
			break;
		case SPA_CHOICE_Range:
			blog(LOG_WARNING, "Ranged framerates not supported");
			break;
		case SPA_CHOICE_Step:
			blog(LOG_WARNING, "Stepped framerates not supported");
			break;
		case SPA_CHOICE_Enum:
			/* i=0 is the default framerate, skip it */
			for (uint32_t i = 1; i < n_framerates; i++)
				g_array_append_val(framerates, framerate_values[i]);
			break;
		default:
			break;
		}
	}

	g_array_sort(framerates, compare_framerates);

	obs_property_list_clear(prop);

	data = obs_data_create();
	for (size_t i = 0; i < framerates->len; i++) {
		const struct spa_fraction *framerate = &g_array_index(framerates, struct spa_fraction, i);
		struct media_frames_per_second fps;
		struct dstr str = {};

		fps = (struct media_frames_per_second){
			.numerator = framerate->num,
			.denominator = framerate->denom,
		};
		obs_data_set_frames_per_second(data, "framerate", fps, NULL);

		dstr_printf(&str, "%.2f", framerate->num / (double)framerate->denom);
		obs_property_list_add_string(prop, str.array, obs_data_get_json(data));

		dstr_free(&str);
	}
	obs_data_release(data);
}

static bool parse_framerate(struct spa_fraction *dest, const char *json)
{
	struct media_frames_per_second fps;
	obs_data_t *data = obs_data_create_from_json(json);

	if (!data)
		return false;

	if (!obs_data_get_frames_per_second(data, "framerate", &fps, NULL)) {
		obs_data_release(data);
		return false;
	}

	dest->num = fps.numerator;
	dest->denom = fps.denominator;

	obs_data_release(data);
	return true;
}

static bool framerate_selected(void *data, obs_properties_t *properties, obs_property_t *property, obs_data_t *settings)
{
	UNUSED_PARAMETER(properties);
	UNUSED_PARAMETER(property);

	struct camera_portal_source *camera_source = data;
	struct camera_device *device;
	struct spa_fraction framerate;

	device = g_hash_table_lookup(connection->devices, camera_source->device_id);
	if (device == NULL)
		return false;

	if (!parse_framerate(&framerate, obs_data_get_string(settings, "framerate")))
		return false;

	if (camera_source->obs_pw_stream)
		obs_pipewire_stream_set_framerate(camera_source->obs_pw_stream, &framerate);

	return true;
}

/*
 * Resolution selected callback
 */

static bool parse_resolution(struct spa_rectangle *dest, const char *json)
{
	obs_data_t *data = obs_data_create_from_json(json);

	if (!data)
		return false;

	dest->width = obs_data_get_int(data, "width");
	dest->height = obs_data_get_int(data, "height");
	obs_data_release(data);
	return true;
}

static bool resolution_selected(void *data, obs_properties_t *properties, obs_property_t *property,
				obs_data_t *settings)
{
	UNUSED_PARAMETER(properties);
	UNUSED_PARAMETER(property);
	UNUSED_PARAMETER(settings);

	struct camera_portal_source *camera_source = data;
	struct spa_rectangle resolution;
	struct camera_device *device;

	blog(LOG_INFO, "[camera-portal] Selected resolution for '%s'", camera_source->device_id);

	device = g_hash_table_lookup(connection->devices, camera_source->device_id);
	if (device == NULL)
		return false;

	if (!parse_resolution(&resolution, obs_data_get_string(settings, "resolution")))
		return false;

	if (camera_source->obs_pw_stream)
		obs_pipewire_stream_set_resolution(camera_source->obs_pw_stream, &resolution);

	property = obs_properties_get(properties, "framerate");
	framerate_list(device, obs_data_get_int(settings, "pixelformat"), &resolution, property);

	return true;
}

static void populate_cameras_list(struct camera_portal_source *camera_source, obs_property_t *device_list)
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
	while (g_hash_table_iter_next(&iter, (gpointer *)&device_id, (gpointer *)&device)) {
		const char *device_name;

		device_name = pw_properties_get(device->properties, PW_KEY_NODE_DESCRIPTION);

		obs_property_list_add_string(device_list, device_name, device_id);

		device_found |= strcmp(device_id, camera_source->device_id) == 0;
	}

	if (!device_found && camera_source->device_id) {
		size_t device_index;
		device_index =
			obs_property_list_add_string(device_list, camera_source->device_id, camera_source->device_id);
		obs_property_list_item_disable(device_list, device_index, true);
	}
}

/* ------------------------------------------------- */

static void node_info(void *data, const struct pw_node_info *info)
{
	struct camera_device *device = data;
	uint32_t i, changed = 0;
	int res;

	info = device->info = pw_node_info_update(device->info, info);
	if (info == NULL)
		return;

	if (info->change_mask & PW_NODE_CHANGE_MASK_PARAMS) {
		for (i = 0; i < info->n_params; i++) {
			uint32_t id = info->params[i].id;

			if (info->params[i].user == 0)
				continue;
			info->params[i].user = 0;

			changed++;
			add_param(&device->pending_list, 0, id, NULL);
			if (!(info->params[i].flags & SPA_PARAM_INFO_READ))
				continue;

			res = pw_node_enum_params((struct pw_node *)device->proxy, ++info->params[i].seq, id, 0, -1,
						  NULL);
			if (SPA_RESULT_IS_ASYNC(res))
				info->params[i].seq = res;
		}
	}

	if (changed) {
		device->changed += changed;
		device->pending_sync = pw_proxy_sync(device->proxy, device->pending_sync);
	}
}

static void node_param(void *data, int seq, uint32_t id, uint32_t index, uint32_t next, const struct spa_pod *param)
{
	UNUSED_PARAMETER(index);
	UNUSED_PARAMETER(next);

	struct camera_device *device = data;
	add_param(&device->pending_list, seq, id, param);
}

static const struct pw_node_events node_events = {
	PW_VERSION_NODE_EVENTS,
	.info = node_info,
	.param = node_param,
};

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
static void on_done_proxy_cb(void *data, int seq)
{
	struct camera_device *device = data;
	if (device->info != NULL && device->pending_sync == seq) {
		object_update_params(&device->param_list, &device->pending_list, device->info->n_params,
				     device->info->params);
	}
}

static const struct pw_proxy_events proxy_events = {
	PW_VERSION_PROXY_EVENTS,
	.removed = on_proxy_removed_cb,
	.destroy = on_destroy_proxy_cb,
	.done = on_done_proxy_cb,
};

static void on_registry_global_cb(void *user_data, uint32_t id, uint32_t permissions, const char *type,
				  uint32_t version, const struct spa_dict *props)
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

	blog(LOG_INFO, "[camera-portal] Found device %s", device_id);

	device = camera_device_new(id, props);
	device->proxy = pw_registry_bind(registry, id, type, version, 0);
	if (!device->proxy) {
		blog(LOG_WARNING, "[camera-portal] Failed to bind device %s", device_id);
		bfree(device);
		return;
	}
	pw_proxy_add_listener(device->proxy, &device->proxy_listener, &proxy_events, device);
	device->node = (struct pw_node *)device->proxy;
	pw_node_add_listener(device->node, &device->node_listener, &node_events, device);

	g_hash_table_insert(connection->devices, bstrdup(device_id), device);

	for (size_t i = 0; i < connection->sources->len; i++) {
		struct camera_portal_source *camera_source = g_ptr_array_index(connection->sources, i);
		obs_source_update_properties(camera_source->source);
		if (strcmp(camera_source->device_id, device_id) == 0)
			stream_camera(camera_source);
	}
}

static void on_registry_global_remove_cb(void *user_data, uint32_t id)
{
	UNUSED_PARAMETER(user_data);

	struct camera_device *device;
	const char *device_id;
	GHashTableIter iter;

	g_hash_table_iter_init(&iter, connection->devices);
	while (g_hash_table_iter_next(&iter, (gpointer *)&device_id, (gpointer *)&device)) {
		if (device->id != id)
			continue;
		g_hash_table_iter_remove(&iter);
		blog(LOG_INFO, "[pipewire-camera] Removed device %s", device_id);
	}
	for (size_t i = 0; i < connection->sources->len; i++) {
		struct camera_portal_source *camera_source = g_ptr_array_index(connection->sources, i);
		obs_source_update_properties(camera_source->source);
	}
}

static const struct pw_registry_events registry_events = {
	PW_VERSION_REGISTRY_EVENTS,
	.global = on_registry_global_cb,
	.global_remove = on_registry_global_remove_cb,
};

/* ------------------------------------------------- */

static void on_pipewire_remote_opened_cb(GObject *source, GAsyncResult *res, void *user_data)
{
	UNUSED_PARAMETER(user_data);

	g_autoptr(GUnixFDList) fd_list = NULL;
	g_autoptr(GVariant) result = NULL;
	g_autoptr(GError) error = NULL;
	int pipewire_fd;
	int fd_index;

	result = g_dbus_proxy_call_with_unix_fd_list_finish(G_DBUS_PROXY(source), &fd_list, res, &error);
	if (error) {
		if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
			blog(LOG_ERROR, "[camera-portal] Error retrieving PipeWire fd: %s", error->message);
		return;
	}

	g_variant_get(result, "(h)", &fd_index, &error);

	pipewire_fd = g_unix_fd_list_get(fd_list, fd_index, &error);
	if (error) {
		if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
			blog(LOG_ERROR, "[camera-portal] Error retrieving PipeWire fd: %s", error->message);
		return;
	}

	connection->obs_pw = obs_pipewire_connect_fd(pipewire_fd, &registry_events, connection);

	obs_pipewire_roundtrip(connection->obs_pw);
}

static void open_pipewire_remote(void)
{
	GVariantBuilder builder;

	g_variant_builder_init(&builder, G_VARIANT_TYPE_VARDICT);

	g_dbus_proxy_call_with_unix_fd_list(get_camera_portal_proxy(), "OpenPipeWireRemote",
					    g_variant_new("(a{sv})", &builder), G_DBUS_CALL_FLAGS_NONE, -1, NULL,
					    connection->cancellable, on_pipewire_remote_opened_cb, NULL);
}

/* ------------------------------------------------- */

static void on_access_camera_response_received_cb(GVariant *parameters, void *user_data)
{
	UNUSED_PARAMETER(user_data);

	g_autoptr(GVariant) result = NULL;
	uint32_t response;

	g_variant_get(parameters, "(u@a{sv})", &response, &result);

	if (response != 0) {
		blog(LOG_WARNING, "[camera-portal] Failed to create session, denied or cancelled by user");
		return;
	}

	blog(LOG_INFO, "[camera-portal] Successfully accessed cameras");

	open_pipewire_remote();
}

static void on_access_camera_finished_cb(GObject *source, GAsyncResult *res, void *user_data)
{
	UNUSED_PARAMETER(user_data);

	g_autoptr(GVariant) result = NULL;
	g_autoptr(GError) error = NULL;

	result = g_dbus_proxy_call_finish(G_DBUS_PROXY(source), res, &error);
	if (error) {
		if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
			blog(LOG_ERROR, "[camera-portal] Error accessing camera: %s", error->message);
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
		connection->devices =
			g_hash_table_new_full(g_str_hash, g_str_equal, bfree, (GDestroyNotify)camera_device_free);
		connection->cancellable = g_cancellable_new();
		connection->sources = g_ptr_array_new();
		connection->initializing = false;
	}

	g_ptr_array_add(connection->sources, camera_source);

	if (connection->initializing)
		return;

	portal_create_request_path(&request_path, &request_token);

	portal_signal_subscribe(request_path, NULL, on_access_camera_response_received_cb, NULL);

	g_variant_builder_init(&builder, G_VARIANT_TYPE_VARDICT);
	g_variant_builder_add(&builder, "{sv}", "handle_token", g_variant_new_string(request_token));

	g_dbus_proxy_call(get_camera_portal_proxy(), "AccessCamera", g_variant_new("(a{sv})", &builder),
			  G_DBUS_CALL_FLAGS_NONE, -1, connection->cancellable, on_access_camera_finished_cb, NULL);

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
	camera_source->device_id = bstrdup(obs_data_get_string(settings, "device_id"));
	camera_source->framerate.set =
		parse_framerate(&camera_source->framerate.fraction, obs_data_get_string(settings, "framerate"));
	camera_source->resolution.set =
		parse_resolution(&camera_source->resolution.rect, obs_data_get_string(settings, "resolution"));

	access_camera(camera_source);

	return camera_source;
}

static void pipewire_camera_destroy(void *data)
{
	struct camera_portal_source *camera_source = data;

	if (connection)
		g_ptr_array_remove(connection->sources, camera_source);

	g_clear_pointer(&camera_source->obs_pw_stream, obs_pipewire_stream_destroy);
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
	obs_properties_t *controls_props;
	obs_properties_t *props;
	obs_property_t *resolution_list;
	obs_property_t *framerate_list;
	obs_property_t *device_list;
	obs_property_t *format_list;

	props = obs_properties_create();

	device_list = obs_properties_add_list(props, "device_id", obs_module_text("PipeWireCameraDevice"),
					      OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	format_list = obs_properties_add_list(props, "pixelformat", obs_module_text("VideoFormat"), OBS_COMBO_TYPE_LIST,
					      OBS_COMBO_FORMAT_INT);

	resolution_list = obs_properties_add_list(props, "resolution", obs_module_text("Resolution"),
						  OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	framerate_list = obs_properties_add_list(props, "framerate", obs_module_text("FrameRate"), OBS_COMBO_TYPE_LIST,
						 OBS_COMBO_FORMAT_STRING);

	// a group to contain the camera control
	controls_props = obs_properties_create();
	obs_properties_add_group(props, "controls", obs_module_text("CameraControls"), OBS_GROUP_NORMAL,
				 controls_props);

	populate_cameras_list(camera_source, device_list);

	obs_property_set_modified_callback2(device_list, device_selected, camera_source);
	obs_property_set_modified_callback2(format_list, format_selected, camera_source);
	obs_property_set_modified_callback2(resolution_list, resolution_selected, camera_source);
	obs_property_set_modified_callback2(framerate_list, framerate_selected, camera_source);

	return props;
}

static void pipewire_camera_update(void *data, obs_data_t *settings)
{
	struct camera_portal_source *camera_source = data;
	const char *device_id;

	device_id = obs_data_get_string(settings, "device_id");

	blog(LOG_INFO, "[camera-portal] Updating device %s", device_id);

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
		return obs_pipewire_stream_get_width(camera_source->obs_pw_stream);
	else
		return 0;
}

static uint32_t pipewire_camera_get_height(void *data)
{
	struct camera_portal_source *camera_source = data;

	if (camera_source->obs_pw_stream)
		return obs_pipewire_stream_get_height(camera_source->obs_pw_stream);
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
