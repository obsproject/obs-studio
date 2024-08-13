/* pipewire-audio.h
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

/* Stuff used by the PipeWire audio capture sources */

#pragma once

#include <obs-module.h>

#include <pipewire/pipewire.h>
#include <pipewire/extensions/metadata.h>
#include <spa/param/audio/format-utils.h>

/* PipeWire Stream wrapper */

/**
 * Audio metadata
 */
struct obs_pw_audio_info {
	uint32_t sample_rate;
	enum audio_format format;
	enum speaker_layout speakers;
};

/**
 * PipeWire stream wrapper that outputs to an OBS source
 */
struct obs_pw_audio_stream {
	struct pw_stream *stream;
	struct spa_hook stream_listener;
	struct obs_pw_audio_info info;
	struct spa_io_position *pos;

	obs_source_t *output;
};

/**
 * Connect a stream with the default params
 * @return 0 on success, < 0 on error
 */
int obs_pw_audio_stream_connect(struct obs_pw_audio_stream *s,
				uint32_t target_id, uint32_t target_serial,
				uint32_t channels);
/* ------------------------------------------------- */

/**
 * Common PipeWire components
 */
struct obs_pw_audio_instance {
	struct pw_thread_loop *thread_loop;
	struct pw_context *context;

	struct pw_core *core;
	struct spa_hook core_listener;
	int seq;

	struct pw_registry *registry;
	struct spa_hook registry_listener;

	struct obs_pw_audio_stream audio;
};

/**
 * Initialize a PipeWire instance
 * @warning The thread loop is left locked
 * @return true on success, false on error
 */
bool obs_pw_audio_instance_init(
	struct obs_pw_audio_instance *pw,
	const struct pw_registry_events *registry_events,
	void *registry_cb_data, bool stream_capture_sink,
	bool stream_want_driver, obs_source_t *stream_output);

/**
 * Destroy a PipeWire instance
 * @warning Call with the thread loop locked
 */
void obs_pw_audio_instance_destroy(struct obs_pw_audio_instance *pw);

/**
 * Trigger a PipeWire core sync
 */
void obs_pw_audio_instance_sync(struct obs_pw_audio_instance *pw);
/* ------------------------------------------------- */

/**
 * PipeWire metadata
 */
struct obs_pw_audio_default_node_metadata {
	struct pw_proxy *proxy;
	struct spa_hook proxy_listener;
	struct spa_hook metadata_listener;

	bool wants_sink;

	void (*default_node_callback)(void *data, const char *name);
	void *data;
};

/**
 * Add listeners to the metadata
 * @return true on success, false on error
 */
bool obs_pw_audio_default_node_metadata_listen(
	struct obs_pw_audio_default_node_metadata *metadata,
	struct obs_pw_audio_instance *pw, uint32_t global_id, bool wants_sink,
	void (*default_node_callback)(void *data, const char *name),
	void *data);
/* ------------------------------------------------- */

/* Helpers for storing remote PipeWire objects */

/**
 * Wrapper over a PipeWire proxy that's a member of a spa_list.
 * Automatically handles adding and removing itself from the list.
 */
struct obs_pw_audio_proxied_object;

/**
 * Get the user data of a proxied object
 */
void *obs_pw_audio_proxied_object_get_user_data(
	struct obs_pw_audio_proxied_object *obj);

/**
 * Convenience wrapper over spa_lists that holds proxied objects
 */
struct obs_pw_audio_proxy_list {
	struct spa_list list;
	void (*bound_callback)(void *data, uint32_t global_id);
	void (*destroy_callback)(void *data);
};

void obs_pw_audio_proxy_list_init(struct obs_pw_audio_proxy_list *list,
				  void (*bound_callback)(void *data,
							 uint32_t global_id),
				  void (*destroy_callback)(void *data));

void obs_pw_audio_proxy_list_append(struct obs_pw_audio_proxy_list *list,
				    struct pw_proxy *proxy);

/**
 * Destroy all stored proxies.
 */
void obs_pw_audio_proxy_list_clear(struct obs_pw_audio_proxy_list *list);

/**
 * Iterator over all user data of the proxies in the list
 */
struct obs_pw_audio_proxy_list_iter {
	struct obs_pw_audio_proxy_list *proxy_list;
	struct obs_pw_audio_proxied_object *current;
};

void obs_pw_audio_proxy_list_iter_init(struct obs_pw_audio_proxy_list_iter *iter,
				       struct obs_pw_audio_proxy_list *list);

/**
 * @return true when there are more items to process, false otherwise
 */
bool obs_pw_audio_proxy_list_iter_next(
	struct obs_pw_audio_proxy_list_iter *iter, void **proxy_user_data);
/* ------------------------------------------------- */

/* Sources */
void pipewire_audio_capture_load(void);
void pipewire_audio_capture_app_load(void);
/* ------------------------------------------------- */
