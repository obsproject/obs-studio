/* pipewire-audio-capture-device.c
 *
 * Copyright 2022-2024 Dimitris Papaioannou <dimtpap@protonmail.com>
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

#include "pipewire-audio.h"

#include <util/dstr.h>

/* Source for capturing device audio using PipeWire */

struct target_node {
	const char *friendly_name;
	const char *name;
	uint32_t serial;
	uint32_t id;
	uint32_t channels;

	struct spa_hook node_listener;

	struct obs_pw_audio_capture_device *pwac;
};

enum capture_type {
	INPUT,
	OUTPUT,
};

struct obs_pw_audio_capture_device {
	obs_source_t *source;

	enum capture_type capture_type;

	struct obs_pw_audio_instance pw;

	struct {
		struct obs_pw_audio_default_node_metadata metadata;
		bool autoconnect;
		uint32_t node_serial;
		struct dstr name;
	} default_info;

	struct obs_pw_audio_proxy_list targets;

	struct dstr target_name;
	uint32_t connected_serial;
};

static void start_streaming(struct obs_pw_audio_capture_device *pwac,
			    struct target_node *node)
{
	dstr_copy(&pwac->target_name, node->name);

	if (pw_stream_get_state(pwac->pw.audio.stream, NULL) !=
	    PW_STREAM_STATE_UNCONNECTED) {
		if (node->serial == pwac->connected_serial) {
			/* Already connected to this node */
			return;
		}

		pw_stream_disconnect(pwac->pw.audio.stream);
		pwac->connected_serial = SPA_ID_INVALID;
	}

	if (!node->channels) {
		return;
	}

	if (obs_pw_audio_stream_connect(&pwac->pw.audio, node->id, node->serial,
					node->channels) == 0) {
		pwac->connected_serial = node->serial;
		blog(LOG_INFO, "[pipewire] %p streaming from %u",
		     pwac->pw.audio.stream, node->serial);
	} else {
		pwac->connected_serial = SPA_ID_INVALID;
		blog(LOG_WARNING, "[pipewire] Error connecting stream %p",
		     pwac->pw.audio.stream);
	}

	pw_stream_set_active(pwac->pw.audio.stream,
			     obs_source_active(pwac->source));
}

struct target_node *get_node_by_name(struct obs_pw_audio_capture_device *pwac,
				     const char *name)
{
	struct obs_pw_audio_proxy_list_iter iter;
	obs_pw_audio_proxy_list_iter_init(&iter, &pwac->targets);

	struct target_node *node;
	while (obs_pw_audio_proxy_list_iter_next(&iter, (void **)&node)) {
		if (strcmp(node->name, name) == 0) {
			return node;
		}
	}

	return NULL;
}

struct target_node *get_node_by_serial(struct obs_pw_audio_capture_device *pwac,
				       uint32_t serial)
{
	struct obs_pw_audio_proxy_list_iter iter;
	obs_pw_audio_proxy_list_iter_init(&iter, &pwac->targets);

	struct target_node *node;
	while (obs_pw_audio_proxy_list_iter_next(&iter, (void **)&node)) {
		if (node->serial == serial) {
			return node;
		}
	}

	return NULL;
}

/* Target node */
static void on_node_info_cb(void *data, const struct pw_node_info *info)
{
	if ((info->change_mask & PW_NODE_CHANGE_MASK_PROPS) == 0 ||
	    !info->props || !info->props->n_items) {
		return;
	}

	const char *prop_channels =
		spa_dict_lookup(info->props, PW_KEY_AUDIO_CHANNELS);
	if (!prop_channels) {
		return;
	}

	uint32_t channels = strtoul(prop_channels, NULL, 10);

	struct target_node *n = data;
	if (n->channels == channels) {
		return;
	}
	n->channels = channels;

	struct obs_pw_audio_capture_device *pwac = n->pwac;

	/** If this is the default device and the stream is not already connected to it
	  * or the stream is unconnected and this node has the desired target name */
	if ((pwac->default_info.autoconnect &&
	     pwac->connected_serial != n->serial &&
	     !dstr_is_empty(&pwac->default_info.name) &&
	     dstr_cmp(&pwac->default_info.name, n->name) == 0) ||
	    (pw_stream_get_state(pwac->pw.audio.stream, NULL) ==
		     PW_STREAM_STATE_UNCONNECTED &&
	     !dstr_is_empty(&pwac->target_name) &&
	     dstr_cmp(&pwac->target_name, n->name) == 0)) {
		start_streaming(pwac, n);
	}
}

static const struct pw_node_events node_events = {
	PW_VERSION_NODE_EVENTS,
	.info = on_node_info_cb,
};

static void node_destroy_cb(void *data)
{
	struct target_node *node = data;

	struct obs_pw_audio_capture_device *pwac = node->pwac;
	if (node->serial == pwac->connected_serial) {
		if (pw_stream_get_state(pwac->pw.audio.stream, NULL) !=
		    PW_STREAM_STATE_UNCONNECTED) {
			pw_stream_disconnect(pwac->pw.audio.stream);
		}
		pwac->connected_serial = SPA_ID_INVALID;
	}

	spa_hook_remove(&node->node_listener);

	bfree((void *)node->friendly_name);
	bfree((void *)node->name);
}

static void register_target_node(struct obs_pw_audio_capture_device *pwac,
				 const char *friendly_name, const char *name,
				 uint32_t object_serial, uint32_t global_id)
{
	struct pw_proxy *node_proxy = pw_registry_bind(
		pwac->pw.registry, global_id, PW_TYPE_INTERFACE_Node,
		PW_VERSION_NODE, sizeof(struct target_node));
	if (!node_proxy) {
		return;
	}

	struct target_node *node = pw_proxy_get_user_data(node_proxy);
	node->friendly_name = bstrdup(friendly_name);
	node->name = bstrdup(name);
	node->serial = object_serial;
	node->channels = 0;
	node->pwac = pwac;

	obs_pw_audio_proxy_list_append(&pwac->targets, node_proxy);

	spa_zero(node->node_listener);
	pw_proxy_add_object_listener(node_proxy, &node->node_listener,
				     &node_events, node);
}
/* ------------------------------------------------- */

/* Default device metadata */
static void default_node_cb(void *data, const char *name)
{
	struct obs_pw_audio_capture_device *pwac = data;

	blog(LOG_DEBUG, "[pipewire] New default device %s", name);

	dstr_copy(&pwac->default_info.name, name);

	struct target_node *node = get_node_by_name(pwac, name);
	if (node) {
		pwac->default_info.node_serial = node->serial;
		if (pwac->default_info.autoconnect) {
			start_streaming(pwac, node);
		}
	} else {
		blog(LOG_DEBUG,
		     "[pipewire] Failed to find node corresponding to default device");
	}
}
/* ------------------------------------------------- */

/* Registry */
static void on_global_cb(void *data, uint32_t id, uint32_t permissions,
			 const char *type, uint32_t version,
			 const struct spa_dict *props)
{
	UNUSED_PARAMETER(permissions);
	UNUSED_PARAMETER(version);

	struct obs_pw_audio_capture_device *pwac = data;

	if (!props || !type) {
		return;
	}

	if (strcmp(type, PW_TYPE_INTERFACE_Node) == 0) {
		const char *node_name, *media_class;
		if (!(node_name = spa_dict_lookup(props, PW_KEY_NODE_NAME)) ||
		    !(media_class =
			      spa_dict_lookup(props, PW_KEY_MEDIA_CLASS))) {
			return;
		}

		/* Target device */
		if ((pwac->capture_type == INPUT &&
		     (strcmp(media_class, "Audio/Source") == 0 ||
		      strcmp(media_class, "Audio/Source/Virtual") == 0)) ||
		    (pwac->capture_type == OUTPUT &&
		     (strcmp(media_class, "Audio/Sink") == 0 ||
		      strcmp(media_class, "Audio/Duplex") == 0))) {

			const char *prop_serial =
				spa_dict_lookup(props, PW_KEY_OBJECT_SERIAL);
			if (!prop_serial) {
				blog(LOG_WARNING,
				     "[pipewire] No object serial found on node %u",
				     id);
				return;
			}
			uint32_t object_serial = strtoul(prop_serial, NULL, 10);

			const char *node_friendly_name =
				spa_dict_lookup(props, PW_KEY_NODE_NICK);
			if (!node_friendly_name) {
				node_friendly_name = spa_dict_lookup(
					props, PW_KEY_NODE_DESCRIPTION);
				if (!node_friendly_name) {
					node_friendly_name = node_name;
				}
			}

			register_target_node(pwac, node_friendly_name,
					     node_name, object_serial, id);
		}
	} else if (strcmp(type, PW_TYPE_INTERFACE_Metadata) == 0) {
		const char *name = spa_dict_lookup(props, PW_KEY_METADATA_NAME);
		if (!name || strcmp(name, "default") != 0) {
			return;
		}

		if (!obs_pw_audio_default_node_metadata_listen(
			    &pwac->default_info.metadata, &pwac->pw, id,
			    pwac->capture_type == OUTPUT, default_node_cb,
			    pwac)) {
			blog(LOG_WARNING,
			     "[pipewire] Failed to get default metadata, cannot detect default audio devices");
		}
	}
}

static const struct pw_registry_events registry_events = {
	PW_VERSION_REGISTRY_EVENTS,
	.global = on_global_cb,
};
/* ------------------------------------------------- */

/* Source */
static void *pipewire_audio_capture_create(obs_data_t *settings,
					   obs_source_t *source,
					   enum capture_type capture_type)
{
	struct obs_pw_audio_capture_device *pwac =
		bzalloc(sizeof(struct obs_pw_audio_capture_device));

	if (!obs_pw_audio_instance_init(&pwac->pw, &registry_events, pwac,
					capture_type == OUTPUT, true, source)) {
		obs_pw_audio_instance_destroy(&pwac->pw);

		bfree(pwac);
		return NULL;
	}

	pwac->source = source;
	pwac->capture_type = capture_type;
	pwac->default_info.node_serial = SPA_ID_INVALID;
	pwac->connected_serial = SPA_ID_INVALID;

	obs_pw_audio_proxy_list_init(&pwac->targets, NULL, node_destroy_cb);

	if (obs_data_get_int(settings, "TargetSerial") != PW_ID_ANY) {
		/** Reset id setting, PipeWire node ids may not persist between sessions.
		  * Connecting to saved target will happen based on the TargetName setting
		  * once target has connected */
		obs_data_set_int(settings, "TargetSerial", 0);
	} else {
		pwac->default_info.autoconnect = true;
	}

	dstr_init_copy(&pwac->target_name,
		       obs_data_get_string(settings, "TargetName"));

	obs_pw_audio_instance_sync(&pwac->pw);
	pw_thread_loop_wait(pwac->pw.thread_loop);
	pw_thread_loop_unlock(pwac->pw.thread_loop);

	return pwac;
}

static void *pipewire_audio_capture_input_create(obs_data_t *settings,
						 obs_source_t *source)
{
	return pipewire_audio_capture_create(settings, source, INPUT);
}

static void *pipewire_audio_capture_output_create(obs_data_t *settings,
						  obs_source_t *source)
{
	return pipewire_audio_capture_create(settings, source, OUTPUT);
}

static void pipewire_audio_capture_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "TargetSerial", PW_ID_ANY);
}

static obs_properties_t *pipewire_audio_capture_properties(void *data)
{
	struct obs_pw_audio_capture_device *pwac = data;

	obs_properties_t *props = obs_properties_create();

	obs_property_t *targets_list = obs_properties_add_list(
		props, "TargetSerial", obs_module_text("Device"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(targets_list, obs_module_text("Default"),
				  PW_ID_ANY);

	if (!pwac->default_info.autoconnect) {
		obs_data_t *settings = obs_source_get_settings(pwac->source);
		/* Saved target serial may be different from connected because a previously connected
		   node may have been replaced by one with the same name */
		obs_data_set_int(settings, "TargetSerial",
				 pwac->connected_serial);
		obs_data_release(settings);
	}

	pw_thread_loop_lock(pwac->pw.thread_loop);

	struct obs_pw_audio_proxy_list_iter iter;
	obs_pw_audio_proxy_list_iter_init(&iter, &pwac->targets);

	struct target_node *node;
	while (obs_pw_audio_proxy_list_iter_next(&iter, (void **)&node)) {
		obs_property_list_add_int(targets_list, node->friendly_name,
					  node->serial);
	}

	pw_thread_loop_unlock(pwac->pw.thread_loop);

	return props;
}

static void pipewire_audio_capture_update(void *data, obs_data_t *settings)
{
	struct obs_pw_audio_capture_device *pwac = data;

	uint32_t new_node_serial = obs_data_get_int(settings, "TargetSerial");

	pw_thread_loop_lock(pwac->pw.thread_loop);

	if ((pwac->default_info.autoconnect = new_node_serial == PW_ID_ANY)) {
		if (pwac->default_info.node_serial != SPA_ID_INVALID) {
			start_streaming(
				pwac,
				get_node_by_serial(
					pwac, pwac->default_info.node_serial));
		}
	} else {
		struct target_node *new_node =
			get_node_by_serial(pwac, new_node_serial);
		if (new_node) {
			start_streaming(pwac, new_node);

			obs_data_set_string(settings, "TargetName",
					    pwac->target_name.array);
		}
	}

	pw_thread_loop_unlock(pwac->pw.thread_loop);
}

static void pipewire_audio_capture_show(void *data)
{
	struct obs_pw_audio_capture_device *pwac = data;

	pw_thread_loop_lock(pwac->pw.thread_loop);
	pw_stream_set_active(pwac->pw.audio.stream, true);
	pw_thread_loop_unlock(pwac->pw.thread_loop);
}

static void pipewire_audio_capture_hide(void *data)
{
	struct obs_pw_audio_capture_device *pwac = data;
	pw_thread_loop_lock(pwac->pw.thread_loop);
	pw_stream_set_active(pwac->pw.audio.stream, false);
	pw_thread_loop_unlock(pwac->pw.thread_loop);
}

static void pipewire_audio_capture_destroy(void *data)
{
	struct obs_pw_audio_capture_device *pwac = data;

	pw_thread_loop_lock(pwac->pw.thread_loop);

	obs_pw_audio_proxy_list_clear(&pwac->targets);

	if (pwac->default_info.metadata.proxy) {
		pw_proxy_destroy(pwac->default_info.metadata.proxy);
	}

	obs_pw_audio_instance_destroy(&pwac->pw);

	dstr_free(&pwac->default_info.name);
	dstr_free(&pwac->target_name);

	bfree(pwac);
}

static const char *pipewire_audio_capture_input_name(void *data)
{
	UNUSED_PARAMETER(data);
	return obs_module_text("PipeWireAudioCaptureInput");
}

static const char *pipewire_audio_capture_output_name(void *data)
{
	UNUSED_PARAMETER(data);
	return obs_module_text("PipeWireAudioCaptureOutput");
}

void pipewire_audio_capture_load(void)
{
	const struct obs_source_info pipewire_audio_capture_input = {
		.id = "pipewire-audio-capture-input",
		.type = OBS_SOURCE_TYPE_INPUT,
		.output_flags = OBS_SOURCE_AUDIO | OBS_SOURCE_DO_NOT_DUPLICATE,
		.get_name = pipewire_audio_capture_input_name,
		.create = pipewire_audio_capture_input_create,
		.get_defaults = pipewire_audio_capture_defaults,
		.get_properties = pipewire_audio_capture_properties,
		.update = pipewire_audio_capture_update,
		.show = pipewire_audio_capture_show,
		.hide = pipewire_audio_capture_hide,
		.destroy = pipewire_audio_capture_destroy,
		.icon_type = OBS_ICON_TYPE_AUDIO_INPUT,
	};
	const struct obs_source_info pipewire_audio_capture_output = {
		.id = "pipewire-audio-capture-output",
		.type = OBS_SOURCE_TYPE_INPUT,
		.output_flags = OBS_SOURCE_AUDIO | OBS_SOURCE_DO_NOT_DUPLICATE |
				OBS_SOURCE_DO_NOT_SELF_MONITOR,
		.get_name = pipewire_audio_capture_output_name,
		.create = pipewire_audio_capture_output_create,
		.get_defaults = pipewire_audio_capture_defaults,
		.get_properties = pipewire_audio_capture_properties,
		.update = pipewire_audio_capture_update,
		.show = pipewire_audio_capture_show,
		.hide = pipewire_audio_capture_hide,
		.destroy = pipewire_audio_capture_destroy,
		.icon_type = OBS_ICON_TYPE_AUDIO_OUTPUT,
	};

	obs_register_source(&pipewire_audio_capture_input);
	obs_register_source(&pipewire_audio_capture_output);
}
