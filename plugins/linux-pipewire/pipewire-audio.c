/* pipewire-audio.c
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

#include <util/platform.h>

#include <spa/utils/json.h>

/* Utilities */
bool json_object_find(const char *obj, const char *key, char *value, size_t len)
{
	/* From PipeWire's source */

	struct spa_json it[2];
	const char *v;
	char k[128];

	spa_json_init(&it[0], obj, strlen(obj));
	if (spa_json_enter_object(&it[0], &it[1]) <= 0) {
		return false;
	}

	while (spa_json_get_string(&it[1], k, sizeof(k)) > 0) {
		if (spa_streq(k, key)) {
			if (spa_json_get_string(&it[1], value, len) > 0) {
				return true;
			}
		} else if (spa_json_next(&it[1], &v) <= 0) {
			break;
		}
	}
	return false;
}
/* ------------------------------------------------- */

/* PipeWire stream wrapper */
void obs_channels_to_spa_audio_position(enum spa_audio_channel *position,
					uint32_t channels)
{
	switch (channels) {
	case 1:
		position[0] = SPA_AUDIO_CHANNEL_MONO;
		break;
	case 2:
		position[0] = SPA_AUDIO_CHANNEL_FL;
		position[1] = SPA_AUDIO_CHANNEL_FR;
		break;
	case 3:
		position[0] = SPA_AUDIO_CHANNEL_FL;
		position[1] = SPA_AUDIO_CHANNEL_FR;
		position[2] = SPA_AUDIO_CHANNEL_LFE;
		break;
	case 4:
		position[0] = SPA_AUDIO_CHANNEL_FL;
		position[1] = SPA_AUDIO_CHANNEL_FR;
		position[2] = SPA_AUDIO_CHANNEL_FC;
		position[3] = SPA_AUDIO_CHANNEL_RC;
		break;
	case 5:
		position[0] = SPA_AUDIO_CHANNEL_FL;
		position[1] = SPA_AUDIO_CHANNEL_FR;
		position[2] = SPA_AUDIO_CHANNEL_FC;
		position[3] = SPA_AUDIO_CHANNEL_LFE;
		position[4] = SPA_AUDIO_CHANNEL_RC;
		break;
	case 6:
		position[0] = SPA_AUDIO_CHANNEL_FL;
		position[1] = SPA_AUDIO_CHANNEL_FR;
		position[2] = SPA_AUDIO_CHANNEL_FC;
		position[3] = SPA_AUDIO_CHANNEL_LFE;
		position[4] = SPA_AUDIO_CHANNEL_RL;
		position[5] = SPA_AUDIO_CHANNEL_RR;
		break;
	case 8:
		position[0] = SPA_AUDIO_CHANNEL_FL;
		position[1] = SPA_AUDIO_CHANNEL_FR;
		position[2] = SPA_AUDIO_CHANNEL_FC;
		position[3] = SPA_AUDIO_CHANNEL_LFE;
		position[4] = SPA_AUDIO_CHANNEL_RL;
		position[5] = SPA_AUDIO_CHANNEL_RR;
		position[6] = SPA_AUDIO_CHANNEL_SL;
		position[7] = SPA_AUDIO_CHANNEL_SR;
		break;
	default:
		for (size_t i = 0; i < channels; i++) {
			position[i] = SPA_AUDIO_CHANNEL_UNKNOWN;
		}
		break;
	}
}

enum audio_format spa_to_obs_audio_format(enum spa_audio_format format)
{
	switch (format) {
	case SPA_AUDIO_FORMAT_U8:
		return AUDIO_FORMAT_U8BIT;
	case SPA_AUDIO_FORMAT_S16_LE:
		return AUDIO_FORMAT_16BIT;
	case SPA_AUDIO_FORMAT_S32_LE:
		return AUDIO_FORMAT_32BIT;
	case SPA_AUDIO_FORMAT_F32_LE:
		return AUDIO_FORMAT_FLOAT;
	case SPA_AUDIO_FORMAT_U8P:
		return AUDIO_FORMAT_U8BIT_PLANAR;
	case SPA_AUDIO_FORMAT_S16P:
		return AUDIO_FORMAT_16BIT_PLANAR;
	case SPA_AUDIO_FORMAT_S32P:
		return AUDIO_FORMAT_32BIT_PLANAR;
	case SPA_AUDIO_FORMAT_F32P:
		return AUDIO_FORMAT_FLOAT_PLANAR;
	default:
		return AUDIO_FORMAT_UNKNOWN;
	}
}

enum speaker_layout spa_to_obs_speakers(uint32_t channels)
{
	switch (channels) {
	case 1:
		return SPEAKERS_MONO;
	case 2:
		return SPEAKERS_STEREO;
	case 3:
		return SPEAKERS_2POINT1;
	case 4:
		return SPEAKERS_4POINT0;
	case 5:
		return SPEAKERS_4POINT1;
	case 6:
		return SPEAKERS_5POINT1;
	case 8:
		return SPEAKERS_7POINT1;
	default:
		return SPEAKERS_UNKNOWN;
	}
}

bool spa_to_obs_pw_audio_info(struct obs_pw_audio_info *info,
			      const struct spa_pod *param)
{
	struct spa_audio_info_raw audio_info;

	if (spa_format_audio_raw_parse(param, &audio_info) < 0) {
		info->sample_rate = 0;
		info->format = AUDIO_FORMAT_UNKNOWN;
		info->speakers = SPEAKERS_UNKNOWN;

		return false;
	}

	info->sample_rate = audio_info.rate;
	info->speakers = spa_to_obs_speakers(audio_info.channels);
	info->format = spa_to_obs_audio_format(audio_info.format);

	return true;
}

static void on_process_cb(void *data)
{
	uint64_t now = os_gettime_ns();

	struct obs_pw_audio_stream *s = data;

	struct pw_buffer *b = pw_stream_dequeue_buffer(s->stream);

	if (!b) {
		return;
	}

	struct spa_buffer *buf = b->buffer;

	if (!s->info.sample_rate || buf->n_datas == 0 ||
	    buf->datas[0].chunk->stride == 0 ||
	    buf->datas[0].type != SPA_DATA_MemPtr) {
		goto queue;
	}

	struct obs_source_audio out = {
		.frames =
			buf->datas[0].chunk->size / buf->datas[0].chunk->stride,
		.speakers = s->info.speakers,
		.format = s->info.format,
		.samples_per_sec = s->info.sample_rate,
	};

	for (size_t i = 0; i < buf->n_datas && i < MAX_AV_PLANES; i++) {
		out.data[i] = buf->datas[i].data;
	}

	if (s->info.sample_rate * s->pos->clock.rate_diff) {
		/** Taken from PipeWire's implementation of JACK's jack_get_cycle_times
		  * (https://gitlab.freedesktop.org/pipewire/pipewire/-/blob/0.3.52/pipewire-jack/src/pipewire-jack.c#L5639)
		  * which is used in the linux-jack plugin to correctly set the timestamp
		  * (https://github.com/obsproject/obs-studio/blob/27.2.4/plugins/linux-jack/jack-wrapper.c#L87) */
		double period_nsecs =
			s->pos->clock.duration * (double)SPA_NSEC_PER_SEC /
			(s->info.sample_rate * s->pos->clock.rate_diff);

		out.timestamp = now - (uint64_t)period_nsecs;
	} else {
		out.timestamp = now - audio_frames_to_ns(s->info.sample_rate,
							 out.frames);
	}

	obs_source_output_audio(s->output, &out);

queue:
	pw_stream_queue_buffer(s->stream, b);
}

static void on_state_changed_cb(void *data, enum pw_stream_state old,
				enum pw_stream_state state, const char *error)
{
	UNUSED_PARAMETER(old);

	struct obs_pw_audio_stream *s = data;

	blog(LOG_DEBUG, "[pipewire] Stream %p state: \"%s\" (error: %s)",
	     s->stream, pw_stream_state_as_string(state),
	     error ? error : "none");
}

static void on_param_changed_cb(void *data, uint32_t id,
				const struct spa_pod *param)
{
	if (!param || id != SPA_PARAM_Format) {
		return;
	}

	struct obs_pw_audio_stream *s = data;

	if (!spa_to_obs_pw_audio_info(&s->info, param)) {
		blog(LOG_WARNING,
		     "[pipewire] Stream %p failed to parse audio format info",
		     s->stream);
	} else {
		blog(LOG_INFO,
		     "[pipewire] %p Got format: rate %u - channels %u - format %u",
		     s->stream, s->info.sample_rate, s->info.speakers,
		     s->info.format);
	}
}

static void on_io_changed_cb(void *data, uint32_t id, void *area, uint32_t size)
{
	UNUSED_PARAMETER(size);

	struct obs_pw_audio_stream *s = data;

	if (id == SPA_IO_Position) {
		s->pos = area;
	}
}

static const struct pw_stream_events stream_events = {
	PW_VERSION_STREAM_EVENTS,
	.process = on_process_cb,
	.state_changed = on_state_changed_cb,
	.param_changed = on_param_changed_cb,
	.io_changed = on_io_changed_cb,
};

int obs_pw_audio_stream_connect(struct obs_pw_audio_stream *s,
				uint32_t target_id, uint32_t target_serial,
				uint32_t audio_channels)
{
	enum spa_audio_channel pos[8];
	obs_channels_to_spa_audio_position(pos, audio_channels);

	uint8_t buffer[2048];
	struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
	const struct spa_pod *params[1];

	params[0] = spa_pod_builder_add_object(
		&b, SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat,
		SPA_FORMAT_mediaType, SPA_POD_Id(SPA_MEDIA_TYPE_audio),
		SPA_FORMAT_mediaSubtype, SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
		SPA_FORMAT_AUDIO_channels, SPA_POD_Int(audio_channels),
		SPA_FORMAT_AUDIO_position,
		SPA_POD_Array(sizeof(enum spa_audio_channel), SPA_TYPE_Id,
			      audio_channels, pos),
		SPA_FORMAT_AUDIO_format,
		SPA_POD_CHOICE_ENUM_Id(
			8, SPA_AUDIO_FORMAT_U8, SPA_AUDIO_FORMAT_S16_LE,
			SPA_AUDIO_FORMAT_S32_LE, SPA_AUDIO_FORMAT_F32_LE,
			SPA_AUDIO_FORMAT_U8P, SPA_AUDIO_FORMAT_S16P,
			SPA_AUDIO_FORMAT_S32P, SPA_AUDIO_FORMAT_F32P));

	struct pw_properties *stream_props = pw_properties_new(NULL, NULL);
	pw_properties_setf(stream_props, PW_KEY_TARGET_OBJECT, "%u",
			   target_serial);
	pw_stream_update_properties(s->stream, &stream_props->dict);
	pw_properties_free(stream_props);

	return pw_stream_connect(s->stream, PW_DIRECTION_INPUT, target_id,
				 PW_STREAM_FLAG_AUTOCONNECT |
					 PW_STREAM_FLAG_MAP_BUFFERS |
					 PW_STREAM_FLAG_DONT_RECONNECT,
				 params, 1);
}
/* ------------------------------------------------- */

/* Common PipeWire components */
static void on_core_done_cb(void *data, uint32_t id, int seq)
{
	struct obs_pw_audio_instance *pw = data;

	if (id == PW_ID_CORE && pw->seq == seq) {
		pw_thread_loop_signal(pw->thread_loop, false);
	}
}

static void on_core_error_cb(void *data, uint32_t id, int seq, int res,
			     const char *message)
{
	struct obs_pw_audio_instance *pw = data;

	blog(LOG_ERROR, "[pipewire] Error id:%u seq:%d res:%d :%s", id, seq,
	     res, message);

	pw_thread_loop_signal(pw->thread_loop, false);
}

static const struct pw_core_events core_events = {
	PW_VERSION_CORE_EVENTS,
	.done = on_core_done_cb,
	.error = on_core_error_cb,
};

bool obs_pw_audio_instance_init(
	struct obs_pw_audio_instance *pw,
	const struct pw_registry_events *registry_events,
	void *registry_cb_data, bool stream_capture_sink,
	bool stream_want_driver, obs_source_t *stream_output)
{
	pw->thread_loop = pw_thread_loop_new("PipeWire thread loop", NULL);
	pw->context = pw_context_new(pw_thread_loop_get_loop(pw->thread_loop),
				     NULL, 0);

	pw_thread_loop_lock(pw->thread_loop);

	if (pw_thread_loop_start(pw->thread_loop) < 0) {
		blog(LOG_WARNING,
		     "[pipewire] Error starting threaded mainloop");
		return false;
	}

	pw->core = pw_context_connect(pw->context, NULL, 0);
	if (!pw->core) {
		blog(LOG_WARNING, "[pipewire] Error creating PipeWire core");
		return false;
	}

	pw_core_add_listener(pw->core, &pw->core_listener, &core_events, pw);

	pw->registry = pw_core_get_registry(pw->core, PW_VERSION_REGISTRY, 0);
	if (!pw->registry) {
		return false;
	}
	pw_registry_add_listener(pw->registry, &pw->registry_listener,
				 registry_events, registry_cb_data);

	pw->audio.output = stream_output;
	pw->audio.stream = pw_stream_new(
		pw->core, "OBS",
		pw_properties_new(
			PW_KEY_NODE_NAME, "OBS", PW_KEY_NODE_DESCRIPTION,
			"OBS Audio Capture", PW_KEY_MEDIA_TYPE, "Audio",
			PW_KEY_MEDIA_CATEGORY, "Capture", PW_KEY_MEDIA_ROLE,
			"Production", PW_KEY_NODE_WANT_DRIVER,
			stream_want_driver ? "true" : "false",
			PW_KEY_STREAM_CAPTURE_SINK,
			stream_capture_sink ? "true" : "false", NULL));

	if (!pw->audio.stream) {
		blog(LOG_WARNING, "[pipewire] Failed to create stream");
		return false;
	}
	blog(LOG_INFO, "[pipewire] Created stream %p", pw->audio.stream);

	pw_stream_add_listener(pw->audio.stream, &pw->audio.stream_listener,
			       &stream_events, &pw->audio);

	return true;
}

void obs_pw_audio_instance_destroy(struct obs_pw_audio_instance *pw)
{
	if (pw->audio.stream) {
		spa_hook_remove(&pw->audio.stream_listener);
		if (pw_stream_get_state(pw->audio.stream, NULL) !=
		    PW_STREAM_STATE_UNCONNECTED) {
			pw_stream_disconnect(pw->audio.stream);
		}
		pw_stream_destroy(pw->audio.stream);
	}

	if (pw->registry) {
		spa_hook_remove(&pw->registry_listener);
		spa_zero(pw->registry_listener);
		pw_proxy_destroy((struct pw_proxy *)pw->registry);
	}

	pw_thread_loop_unlock(pw->thread_loop);
	pw_thread_loop_stop(pw->thread_loop);

	if (pw->core) {
		spa_hook_remove(&pw->core_listener);
		spa_zero(pw->core_listener);
		pw_core_disconnect(pw->core);
	}

	if (pw->context) {
		pw_context_destroy(pw->context);
	}

	pw_thread_loop_destroy(pw->thread_loop);
}

void obs_pw_audio_instance_sync(struct obs_pw_audio_instance *pw)
{
	pw->seq = pw_core_sync(pw->core, PW_ID_CORE, pw->seq);
}
/* ------------------------------------------------- */

/* PipeWire metadata */
static int on_metadata_property_cb(void *data, uint32_t id, const char *key,
				   const char *type, const char *value)
{
	UNUSED_PARAMETER(type);

	struct obs_pw_audio_default_node_metadata *metadata = data;

	if (id == PW_ID_CORE && key && value &&
	    strcmp(key, metadata->wants_sink ? "default.audio.sink"
					     : "default.audio.source") == 0) {
		char val[128];
		if (json_object_find(value, "name", val, sizeof(val)) && *val) {
			metadata->default_node_callback(metadata->data, val);
		}
	}

	return 0;
}

static const struct pw_metadata_events metadata_events = {
	PW_VERSION_METADATA_EVENTS,
	.property = on_metadata_property_cb,
};

static void on_metadata_proxy_removed_cb(void *data)
{
	struct obs_pw_audio_default_node_metadata *metadata = data;
	pw_proxy_destroy(metadata->proxy);
}

static void on_metadata_proxy_destroy_cb(void *data)
{
	struct obs_pw_audio_default_node_metadata *metadata = data;

	spa_hook_remove(&metadata->metadata_listener);
	spa_hook_remove(&metadata->proxy_listener);
	spa_zero(metadata->metadata_listener);
	spa_zero(metadata->proxy_listener);

	metadata->proxy = NULL;
}

static const struct pw_proxy_events metadata_proxy_events = {
	PW_VERSION_PROXY_EVENTS,
	.removed = on_metadata_proxy_removed_cb,
	.destroy = on_metadata_proxy_destroy_cb,
};

bool obs_pw_audio_default_node_metadata_listen(
	struct obs_pw_audio_default_node_metadata *metadata,
	struct obs_pw_audio_instance *pw, uint32_t global_id, bool wants_sink,
	void (*default_node_callback)(void *data, const char *name), void *data)
{
	if (metadata->proxy) {
		pw_proxy_destroy(metadata->proxy);
	}

	struct pw_proxy *metadata_proxy = pw_registry_bind(
		pw->registry, global_id, PW_TYPE_INTERFACE_Metadata,
		PW_VERSION_METADATA, 0);
	if (!metadata_proxy) {
		return false;
	}

	metadata->proxy = metadata_proxy;

	metadata->wants_sink = wants_sink;

	metadata->default_node_callback = default_node_callback;
	metadata->data = data;

	pw_proxy_add_object_listener(metadata->proxy,
				     &metadata->metadata_listener,
				     &metadata_events, metadata);
	pw_proxy_add_listener(metadata->proxy, &metadata->proxy_listener,
			      &metadata_proxy_events, metadata);

	return true;
}
/* ------------------------------------------------- */

/* Proxied objects */
struct obs_pw_audio_proxied_object {
	void (*bound_callback)(void *data, uint32_t global_id);
	void (*destroy_callback)(void *data);

	struct pw_proxy *proxy;
	struct spa_hook proxy_listener;

	struct spa_list link;
};

static void on_proxy_bound_cb(void *data, uint32_t global_id)
{
	struct obs_pw_audio_proxied_object *obj = data;
	if (obj->bound_callback) {
		obj->bound_callback(pw_proxy_get_user_data(obj->proxy),
				    global_id);
	}
}

static void on_proxy_removed_cb(void *data)
{
	struct obs_pw_audio_proxied_object *obj = data;
	pw_proxy_destroy(obj->proxy);
}

static void on_proxy_destroy_cb(void *data)
{
	struct obs_pw_audio_proxied_object *obj = data;
	spa_hook_remove(&obj->proxy_listener);

	spa_list_remove(&obj->link);

	if (obj->destroy_callback) {
		obj->destroy_callback(pw_proxy_get_user_data(obj->proxy));
	}

	bfree(data);
}

static const struct pw_proxy_events proxy_events = {
	PW_VERSION_PROXY_EVENTS,
	.bound = on_proxy_bound_cb,
	.removed = on_proxy_removed_cb,
	.destroy = on_proxy_destroy_cb,
};

void obs_pw_audio_proxied_object_new(struct pw_proxy *proxy,
				     struct spa_list *list,
				     void (*bound_callback)(void *data,
							    uint32_t global_id),
				     void (*destroy_callback)(void *data))
{
	struct obs_pw_audio_proxied_object *obj =
		bmalloc(sizeof(struct obs_pw_audio_proxied_object));

	obj->proxy = proxy;
	obj->bound_callback = bound_callback;
	obj->destroy_callback = destroy_callback;

	spa_list_append(list, &obj->link);

	spa_zero(obj->proxy_listener);
	pw_proxy_add_listener(obj->proxy, &obj->proxy_listener, &proxy_events,
			      obj);
}

void *obs_pw_audio_proxied_object_get_user_data(
	struct obs_pw_audio_proxied_object *obj)
{
	return pw_proxy_get_user_data(obj->proxy);
}

void obs_pw_audio_proxy_list_init(struct obs_pw_audio_proxy_list *list,
				  void (*bound_callback)(void *data,
							 uint32_t global_id),
				  void (*destroy_callback)(void *data))
{
	spa_list_init(&list->list);

	list->bound_callback = bound_callback;
	list->destroy_callback = destroy_callback;
}

void obs_pw_audio_proxy_list_append(struct obs_pw_audio_proxy_list *list,
				    struct pw_proxy *proxy)
{
	obs_pw_audio_proxied_object_new(proxy, &list->list,
					list->bound_callback,
					list->destroy_callback);
}

void obs_pw_audio_proxy_list_clear(struct obs_pw_audio_proxy_list *list)
{
	struct obs_pw_audio_proxied_object *obj, *temp;
	spa_list_for_each_safe(obj, temp, &list->list, link)
	{
		pw_proxy_destroy(obj->proxy);
	}
}

void obs_pw_audio_proxy_list_iter_init(struct obs_pw_audio_proxy_list_iter *iter,
				       struct obs_pw_audio_proxy_list *list)
{
	iter->proxy_list = list;
	iter->current = spa_list_first(
		&list->list, struct obs_pw_audio_proxied_object, link);
}

bool obs_pw_audio_proxy_list_iter_next(
	struct obs_pw_audio_proxy_list_iter *iter, void **proxy_user_data)
{
	if (spa_list_is_empty(&iter->proxy_list->list)) {
		return false;
	}

	if (spa_list_is_end(iter->current, &iter->proxy_list->list, link)) {
		return false;
	}

	*proxy_user_data =
		obs_pw_audio_proxied_object_get_user_data(iter->current);
	iter->current = spa_list_next(iter->current, link);

	return true;
}
/* ------------------------------------------------- */
