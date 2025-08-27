/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

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

#include <inttypes.h>
#include "util/platform.h"
#include "util/util_uint64.h"
#include "util/array-serializer.h"
#include "graphics/math-extra.h"
#include "obs.h"
#include "obs-internal.h"
#include "obs-av1.h"

#include <caption/caption.h>
#include <caption/mpeg.h>

#define get_weak(output) ((obs_weak_output_t *)output->context.control)

#define RECONNECT_RETRY_MAX_MSEC (15 * 60 * 1000)
#define RECONNECT_RETRY_BASE_EXP 1.5f

static inline bool active(const struct obs_output *output)
{
	return os_atomic_load_bool(&output->active);
}

static inline bool reconnecting(const struct obs_output *output)
{
	return os_atomic_load_bool(&output->reconnecting);
}

static inline bool stopping(const struct obs_output *output)
{
	return os_event_try(output->stopping_event) == EAGAIN;
}

static inline bool delay_active(const struct obs_output *output)
{
	return os_atomic_load_bool(&output->delay_active);
}

static inline bool delay_capturing(const struct obs_output *output)
{
	return os_atomic_load_bool(&output->delay_capturing);
}

static inline bool data_capture_ending(const struct obs_output *output)
{
	return os_atomic_load_bool(&output->end_data_capture_thread_active);
}

static inline bool flag_encoded(const struct obs_output *output)
{
	return (output->info.flags & OBS_OUTPUT_ENCODED) != 0;
}

static inline bool log_flag_encoded(const struct obs_output *output, const char *func_name, bool inverse_log)
{
	const char *prefix = inverse_log ? "n encoded" : " raw";
	bool ret = flag_encoded(output);
	if ((!inverse_log && !ret) || (inverse_log && ret))
		blog(LOG_WARNING, "Output '%s': Tried to use %s on a%s output", output->context.name, func_name,
		     prefix);
	return ret;
}

static inline bool flag_video(const struct obs_output *output)
{
	return (output->info.flags & OBS_OUTPUT_VIDEO) != 0;
}

static inline bool log_flag_video(const struct obs_output *output, const char *func_name)
{
	bool ret = flag_video(output);
	if (!ret)
		blog(LOG_WARNING, "Output '%s': Tried to use %s on a non-video output", output->context.name,
		     func_name);
	return ret;
}

static inline bool flag_audio(const struct obs_output *output)
{
	return (output->info.flags & OBS_OUTPUT_AUDIO) != 0;
}

static inline bool log_flag_audio(const struct obs_output *output, const char *func_name)
{
	bool ret = flag_audio(output);
	if (!ret)
		blog(LOG_WARNING, "Output '%s': Tried to use %s on a non-audio output", output->context.name,
		     func_name);
	return ret;
}

static inline bool flag_service(const struct obs_output *output)
{
	return (output->info.flags & OBS_OUTPUT_SERVICE) != 0;
}

static inline bool log_flag_service(const struct obs_output *output, const char *func_name)
{
	bool ret = flag_service(output);
	if (!ret)
		blog(LOG_WARNING, "Output '%s': Tried to use %s on a non-service output", output->context.name,
		     func_name);
	return ret;
}

const struct obs_output_info *find_output(const char *id)
{
	size_t i;
	for (i = 0; i < obs->output_types.num; i++)
		if (strcmp(obs->output_types.array[i].id, id) == 0)
			return obs->output_types.array + i;

	return NULL;
}

const char *obs_output_get_display_name(const char *id)
{
	const struct obs_output_info *info = find_output(id);
	return (info != NULL) ? info->get_name(info->type_data) : NULL;
}

obs_module_t *obs_output_get_module(const char *id)
{
	obs_module_t *module = obs->first_module;
	while (module) {
		for (size_t i = 0; i < module->outputs.num; i++) {
			if (strcmp(module->outputs.array[i], id) == 0) {
				return module;
			}
		}
		module = module->next;
	}

	module = obs->first_disabled_module;
	while (module) {
		for (size_t i = 0; i < module->outputs.num; i++) {
			if (strcmp(module->outputs.array[i], id) == 0) {
				return module;
			}
		}
		module = module->next;
	}

	return NULL;
}

enum obs_module_load_state obs_output_load_state(const char *id)
{
	obs_module_t *module = obs_output_get_module(id);
	if (!module) {
		return OBS_MODULE_MISSING;
	}
	return module->load_state;
}

static const char *output_signals[] = {
	"void start(ptr output)",
	"void stop(ptr output, int code)",
	"void pause(ptr output)",
	"void unpause(ptr output)",
	"void starting(ptr output)",
	"void stopping(ptr output)",
	"void activate(ptr output)",
	"void deactivate(ptr output)",
	"void reconnect(ptr output)",
	"void reconnect_success(ptr output)",
	NULL,
};

static bool init_output_handlers(struct obs_output *output, const char *name, obs_data_t *settings,
				 obs_data_t *hotkey_data)
{
	if (!obs_context_data_init(&output->context, OBS_OBJ_TYPE_OUTPUT, settings, name, NULL, hotkey_data, false))
		return false;

	signal_handler_add_array(output->context.signals, output_signals);
	return true;
}

obs_output_t *obs_output_create(const char *id, const char *name, obs_data_t *settings, obs_data_t *hotkey_data)
{
	const struct obs_output_info *info = find_output(id);
	struct obs_output *output;
	int ret;

	output = bzalloc(sizeof(struct obs_output));
	pthread_mutex_init_value(&output->interleaved_mutex);
	pthread_mutex_init_value(&output->delay_mutex);
	pthread_mutex_init_value(&output->pause.mutex);
	pthread_mutex_init_value(&output->pkt_callbacks_mutex);

	if (pthread_mutex_init(&output->interleaved_mutex, NULL) != 0)
		goto fail;
	if (pthread_mutex_init(&output->delay_mutex, NULL) != 0)
		goto fail;
	if (pthread_mutex_init(&output->pause.mutex, NULL) != 0)
		goto fail;
	if (pthread_mutex_init(&output->pkt_callbacks_mutex, NULL) != 0)
		goto fail;
	if (os_event_init(&output->stopping_event, OS_EVENT_TYPE_MANUAL) != 0)
		goto fail;
	if (!init_output_handlers(output, name, settings, hotkey_data))
		goto fail;

	os_event_signal(output->stopping_event);

	if (!info) {
		blog(LOG_ERROR, "Output ID '%s' not found", id);

		output->info.id = bstrdup(id);
		output->owns_info_id = true;
	} else {
		output->info = *info;
	}
	if (!flag_encoded(output)) {
		output->video = obs_get_video();
		output->audio = obs_get_audio();
	}
	if (output->info.get_defaults)
		output->info.get_defaults(output->context.settings);

	ret = os_event_init(&output->reconnect_stop_event, OS_EVENT_TYPE_MANUAL);
	if (ret < 0)
		goto fail;

	output->reconnect_retry_sec = 2;
	output->reconnect_retry_max = 20;
	output->reconnect_retry_exp = RECONNECT_RETRY_BASE_EXP + (rand_float(0) * 0.05f);
	output->valid = true;

	obs_context_init_control(&output->context, output, (obs_destroy_cb)obs_output_destroy);
	obs_context_data_insert(&output->context, &obs->data.outputs_mutex, &obs->data.first_output);

	if (info)
		output->context.data = info->create(output->context.settings, output);
	if (!output->context.data)
		blog(LOG_ERROR, "Failed to create output '%s'!", name);

	blog(LOG_DEBUG, "output '%s' (%s) created", name, id);
	return output;

fail:
	obs_output_destroy(output);
	return NULL;
}

static inline void free_packets(struct obs_output *output)
{
	for (size_t i = 0; i < output->interleaved_packets.num; i++)
		obs_encoder_packet_release(output->interleaved_packets.array + i);
	da_free(output->interleaved_packets);
}

static inline void clear_raw_audio_buffers(obs_output_t *output)
{
	for (size_t i = 0; i < MAX_AUDIO_MIXES; i++) {
		for (size_t j = 0; j < MAX_AV_PLANES; j++) {
			deque_free(&output->audio_buffer[i][j]);
		}
	}
}

static void destroy_caption_track(struct caption_track_data **ctrack_ptr)
{
	if (!ctrack_ptr || !*ctrack_ptr) {
		return;
	}
	struct caption_track_data *ctrack = *ctrack_ptr;
	pthread_mutex_destroy(&ctrack->caption_mutex);
	deque_free(&ctrack->caption_data);
	bfree(ctrack);
	*ctrack_ptr = NULL;
}

void obs_output_destroy(obs_output_t *output)
{
	if (output) {
		obs_context_data_remove(&output->context);

		blog(LOG_DEBUG, "output '%s' destroyed", output->context.name);

		if (output->valid && active(output))
			obs_output_actual_stop(output, true, 0);

		os_event_wait(output->stopping_event);
		if (data_capture_ending(output))
			pthread_join(output->end_data_capture_thread, NULL);

		if (output->service)
			output->service->output = NULL;
		if (output->context.data)
			output->info.destroy(output->context.data);

		free_packets(output);

		for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
			if (output->video_encoders[i]) {
				obs_encoder_remove_output(output->video_encoders[i], output);
				obs_encoder_release(output->video_encoders[i]);
			}
			if (output->caption_tracks[i]) {
				destroy_caption_track(&output->caption_tracks[i]);
			}
		}

		for (size_t i = 0; i < MAX_OUTPUT_AUDIO_ENCODERS; i++) {
			if (output->audio_encoders[i]) {
				obs_encoder_remove_output(output->audio_encoders[i], output);
				obs_encoder_release(output->audio_encoders[i]);
			}
		}

		da_free(output->keyframe_group_tracking);

		for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++)
			da_free(output->encoder_packet_times[i]);

		da_free(output->pkt_callbacks);

		clear_raw_audio_buffers(output);

		os_event_destroy(output->stopping_event);
		pthread_mutex_destroy(&output->pause.mutex);
		pthread_mutex_destroy(&output->interleaved_mutex);
		pthread_mutex_destroy(&output->delay_mutex);
		pthread_mutex_destroy(&output->pkt_callbacks_mutex);
		os_event_destroy(output->reconnect_stop_event);
		obs_context_data_free(&output->context);
		deque_free(&output->delay_data);
		if (output->owns_info_id)
			bfree((void *)output->info.id);
		if (output->last_error_message)
			bfree(output->last_error_message);
		bfree(output);
	}
}

const char *obs_output_get_name(const obs_output_t *output)
{
	return obs_output_valid(output, "obs_output_get_name") ? output->context.name : NULL;
}

bool obs_output_actual_start(obs_output_t *output)
{
	bool success = false;

	os_event_wait(output->stopping_event);
	output->stop_code = 0;
	if (output->last_error_message) {
		bfree(output->last_error_message);
		output->last_error_message = NULL;
	}

	if (output->context.data)
		success = output->info.start(output->context.data);

	if (success) {
		output->starting_drawn_count = obs->video.total_frames;
		output->starting_lagged_count = obs->video.lagged_frames;
	}

	if (os_atomic_load_long(&output->delay_restart_refs))
		os_atomic_dec_long(&output->delay_restart_refs);

	for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
		struct caption_track_data *ctrack = output->caption_tracks[i];
		if (!ctrack) {
			continue;
		}
		pthread_mutex_lock(&ctrack->caption_mutex);
		ctrack->caption_timestamp = 0;
		deque_free(&ctrack->caption_data);
		deque_init(&ctrack->caption_data);
		pthread_mutex_unlock(&ctrack->caption_mutex);
	}

	return success;
}

bool obs_output_start(obs_output_t *output)
{
	if (!obs_output_valid(output, "obs_output_start"))
		return false;
	if (!output->context.data)
		return false;

	if (flag_service(output) &&
	    !(obs_service_can_try_to_connect(output->service) && obs_service_initialize(output->service, output)))
		return false;

	if (output->delay_sec) {
		return obs_output_delay_start(output);
	} else {
		if (obs_output_actual_start(output)) {
			do_output_signal(output, "starting");
			return true;
		}

		return false;
	}
}

static inline bool data_active(struct obs_output *output)
{
	return os_atomic_load_bool(&output->data_active);
}

static void log_frame_info(struct obs_output *output)
{
	struct obs_core_video *video = &obs->video;

	uint32_t drawn = video->total_frames - output->starting_drawn_count;
	uint32_t lagged = video->lagged_frames - output->starting_lagged_count;

	int dropped = obs_output_get_frames_dropped(output);
	int total = output->total_frames;

	double percentage_lagged = 0.0f;
	double percentage_dropped = 0.0f;

	if (drawn)
		percentage_lagged = (double)lagged / (double)drawn * 100.0;
	if (dropped)
		percentage_dropped = (double)dropped / (double)total * 100.0;

	blog(LOG_INFO, "Output '%s': stopping", output->context.name);
	if (!dropped || !total)
		blog(LOG_INFO, "Output '%s': Total frames output: %d", output->context.name, total);
	else
		blog(LOG_INFO,
		     "Output '%s': Total frames output: %d"
		     " (%d attempted)",
		     output->context.name, total - dropped, total);

	if (!lagged || !drawn)
		blog(LOG_INFO, "Output '%s': Total drawn frames: %" PRIu32, output->context.name, drawn);
	else
		blog(LOG_INFO, "Output '%s': Total drawn frames: %" PRIu32 " (%" PRIu32 " attempted)",
		     output->context.name, drawn - lagged, drawn);

	if (drawn && lagged)
		blog(LOG_INFO,
		     "Output '%s': Number of lagged frames due "
		     "to rendering lag/stalls: %" PRIu32 " (%0.1f%%)",
		     output->context.name, lagged, percentage_lagged);
	if (total && dropped)
		blog(LOG_INFO,
		     "Output '%s': Number of dropped frames due "
		     "to insufficient bandwidth/connection stalls: "
		     "%d (%0.1f%%)",
		     output->context.name, dropped, percentage_dropped);
}

static inline void signal_stop(struct obs_output *output);

void obs_output_actual_stop(obs_output_t *output, bool force, uint64_t ts)
{
	bool call_stop = true;
	bool was_reconnecting = false;

	if (stopping(output) && !force)
		return;

	obs_output_pause(output, false);

	os_event_reset(output->stopping_event);

	was_reconnecting = reconnecting(output) && !delay_active(output);
	if (reconnecting(output)) {
		os_event_signal(output->reconnect_stop_event);
		if (output->reconnect_thread_active)
			pthread_join(output->reconnect_thread, NULL);
	}

	if (force) {
		if (delay_active(output)) {
			call_stop = delay_capturing(output);
			os_atomic_set_bool(&output->delay_active, false);
			os_atomic_set_bool(&output->delay_capturing, false);
			output->stop_code = OBS_OUTPUT_SUCCESS;
			obs_output_end_data_capture(output);
			os_event_signal(output->stopping_event);
		} else {
			call_stop = true;
		}
	} else {
		call_stop = true;
	}

	if (output->context.data && call_stop) {
		output->info.stop(output->context.data, ts);

	} else if (was_reconnecting) {
		output->stop_code = OBS_OUTPUT_SUCCESS;
		signal_stop(output);
		os_event_signal(output->stopping_event);
	}

	for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
		struct caption_track_data *ctrack = output->caption_tracks[i];
		if (!ctrack) {
			continue;
		}
		while (ctrack->caption_head) {
			ctrack->caption_tail = ctrack->caption_head->next;
			bfree(ctrack->caption_head);
			ctrack->caption_head = ctrack->caption_tail;
		}
	}

	da_clear(output->keyframe_group_tracking);
}

void obs_output_stop(obs_output_t *output)
{
	if (!obs_output_valid(output, "obs_output_stop"))
		return;
	if (!output->context.data)
		return;
	if (!active(output) && !reconnecting(output))
		return;
	if (reconnecting(output)) {
		obs_output_force_stop(output);
		return;
	}

	if (flag_encoded(output) && output->active_delay_ns) {
		obs_output_delay_stop(output);
	} else if (!stopping(output)) {
		do_output_signal(output, "stopping");
		obs_output_actual_stop(output, false, os_gettime_ns());
	}
}

void obs_output_force_stop(obs_output_t *output)
{
	if (!obs_output_valid(output, "obs_output_force_stop"))
		return;

	if (!stopping(output)) {
		output->stop_code = 0;
		do_output_signal(output, "stopping");
	}
	obs_output_actual_stop(output, true, 0);
}

bool obs_output_active(const obs_output_t *output)
{
	return (output != NULL) ? (active(output) || reconnecting(output)) : false;
}

uint32_t obs_output_get_flags(const obs_output_t *output)
{
	return obs_output_valid(output, "obs_output_get_flags") ? output->info.flags : 0;
}

uint32_t obs_get_output_flags(const char *id)
{
	const struct obs_output_info *info = find_output(id);
	return info ? info->flags : 0;
}

static inline obs_data_t *get_defaults(const struct obs_output_info *info)
{
	obs_data_t *settings = obs_data_create();
	if (info->get_defaults)
		info->get_defaults(settings);
	return settings;
}

obs_data_t *obs_output_defaults(const char *id)
{
	const struct obs_output_info *info = find_output(id);
	return (info) ? get_defaults(info) : NULL;
}

obs_properties_t *obs_get_output_properties(const char *id)
{
	const struct obs_output_info *info = find_output(id);
	if (info && info->get_properties) {
		obs_data_t *defaults = get_defaults(info);
		obs_properties_t *properties;

		properties = info->get_properties(NULL);
		obs_properties_apply_settings(properties, defaults);
		obs_data_release(defaults);
		return properties;
	}
	return NULL;
}

obs_properties_t *obs_output_properties(const obs_output_t *output)
{
	if (!obs_output_valid(output, "obs_output_properties"))
		return NULL;

	if (output && output->info.get_properties) {
		obs_properties_t *props;
		props = output->info.get_properties(output->context.data);
		obs_properties_apply_settings(props, output->context.settings);
		return props;
	}

	return NULL;
}

void obs_output_update(obs_output_t *output, obs_data_t *settings)
{
	if (!obs_output_valid(output, "obs_output_update"))
		return;

	obs_data_apply(output->context.settings, settings);

	if (output->info.update)
		output->info.update(output->context.data, output->context.settings);
}

obs_data_t *obs_output_get_settings(const obs_output_t *output)
{
	if (!obs_output_valid(output, "obs_output_get_settings"))
		return NULL;

	obs_data_addref(output->context.settings);
	return output->context.settings;
}

bool obs_output_can_pause(const obs_output_t *output)
{
	return obs_output_valid(output, "obs_output_can_pause") ? !!(output->info.flags & OBS_OUTPUT_CAN_PAUSE) : false;
}

static inline void end_pause(struct pause_data *pause, uint64_t ts)
{
	if (!pause->ts_end) {
		pause->ts_end = ts;
		pause->ts_offset += pause->ts_end - pause->ts_start;
	}
}

static inline uint64_t get_closest_v_ts(struct pause_data *pause)
{
	uint64_t interval = obs->video.video_frame_interval_ns;
	uint64_t i2 = interval * 2;
	uint64_t ts = os_gettime_ns();

	return pause->last_video_ts + ((ts - pause->last_video_ts + i2) / interval) * interval;
}

static inline bool pause_can_start(struct pause_data *pause)
{
	return !pause->ts_start && !pause->ts_end;
}

static inline bool pause_can_stop(struct pause_data *pause)
{
	return !!pause->ts_start && !pause->ts_end;
}

static bool get_first_audio_encoder_index(const struct obs_output *output, size_t *index)
{
	if (!index)
		return false;

	for (size_t i = 0; i < MAX_OUTPUT_AUDIO_ENCODERS; i++) {
		if (output->audio_encoders[i]) {
			*index = i;
			return true;
		}
	}

	return false;
}

static bool get_first_video_encoder_index(const struct obs_output *output, size_t *index)
{
	if (!index)
		return false;

	for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
		if (output->video_encoders[i]) {
			*index = i;
			return true;
		}
	}

	return false;
}

static bool obs_encoded_output_pause(obs_output_t *output, bool pause)
{
	obs_encoder_t *venc[MAX_OUTPUT_VIDEO_ENCODERS];
	obs_encoder_t *aenc[MAX_OUTPUT_AUDIO_ENCODERS];
	uint64_t closest_v_ts;
	bool success = false;

	for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++)
		venc[i] = output->video_encoders[i];
	for (size_t i = 0; i < MAX_OUTPUT_AUDIO_ENCODERS; i++)
		aenc[i] = output->audio_encoders[i];

	for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
		if (venc[i]) {
			pthread_mutex_lock(&venc[i]->pause.mutex);
		}
	}
	for (size_t i = 0; i < MAX_OUTPUT_AUDIO_ENCODERS; i++) {
		if (aenc[i]) {
			pthread_mutex_lock(&aenc[i]->pause.mutex);
		}
	}

	/* ---------------------------- */

	size_t first_venc_index;
	if (!get_first_video_encoder_index(output, &first_venc_index))
		goto fail;

	closest_v_ts = get_closest_v_ts(&venc[first_venc_index]->pause);

	if (pause) {
		for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
			if (venc[i] && !pause_can_start(&venc[i]->pause)) {
				goto fail;
			}
		}
		for (size_t i = 0; i < MAX_OUTPUT_AUDIO_ENCODERS; i++) {
			if (aenc[i] && !pause_can_start(&aenc[i]->pause)) {
				goto fail;
			}
		}

		for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
			if (venc[i]) {
				os_atomic_set_bool(&venc[i]->paused, true);
				venc[i]->pause.ts_start = closest_v_ts;
			}
		}
		for (size_t i = 0; i < MAX_OUTPUT_AUDIO_ENCODERS; i++) {
			if (aenc[i]) {
				os_atomic_set_bool(&aenc[i]->paused, true);
				aenc[i]->pause.ts_start = closest_v_ts;
			}
		}
	} else {
		for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
			if (venc[i] && !pause_can_stop(&venc[i]->pause)) {
				goto fail;
			}
		}
		for (size_t i = 0; i < MAX_OUTPUT_AUDIO_ENCODERS; i++) {
			if (aenc[i] && !pause_can_stop(&aenc[i]->pause)) {
				goto fail;
			}
		}

		for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
			if (venc[i]) {
				os_atomic_set_bool(&venc[i]->paused, false);
				end_pause(&venc[i]->pause, closest_v_ts);
			}
		}
		for (size_t i = 0; i < MAX_OUTPUT_AUDIO_ENCODERS; i++) {
			if (aenc[i]) {
				os_atomic_set_bool(&aenc[i]->paused, false);
				end_pause(&aenc[i]->pause, closest_v_ts);
			}
		}
	}

	/* ---------------------------- */

	success = true;

fail:
	for (size_t i = MAX_OUTPUT_AUDIO_ENCODERS; i > 0; i--) {
		if (aenc[i - 1]) {
			pthread_mutex_unlock(&aenc[i - 1]->pause.mutex);
		}
	}
	for (size_t i = MAX_OUTPUT_VIDEO_ENCODERS; i > 0; i--) {
		if (venc[i - 1]) {
			pthread_mutex_unlock(&venc[i - 1]->pause.mutex);
		}
	}

	return success;
}

static bool obs_raw_output_pause(obs_output_t *output, bool pause)
{
	bool success;
	uint64_t closest_v_ts;

	pthread_mutex_lock(&output->pause.mutex);
	closest_v_ts = get_closest_v_ts(&output->pause);
	if (pause) {
		success = pause_can_start(&output->pause);
		if (success)
			output->pause.ts_start = closest_v_ts;
	} else {
		success = pause_can_stop(&output->pause);
		if (success)
			end_pause(&output->pause, closest_v_ts);
	}
	pthread_mutex_unlock(&output->pause.mutex);

	return success;
}

bool obs_output_pause(obs_output_t *output, bool pause)
{
	bool success;

	if (!obs_output_valid(output, "obs_output_pause"))
		return false;
	if ((output->info.flags & OBS_OUTPUT_CAN_PAUSE) == 0)
		return false;
	if (!os_atomic_load_bool(&output->active))
		return false;
	if (os_atomic_load_bool(&output->paused) == pause)
		return true;

	success = flag_encoded(output) ? obs_encoded_output_pause(output, pause) : obs_raw_output_pause(output, pause);
	if (success) {
		os_atomic_set_bool(&output->paused, pause);
		do_output_signal(output, pause ? "pause" : "unpause");

		blog(LOG_INFO, "output %s %spaused", output->context.name, pause ? "" : "un");
	}
	return success;
}

bool obs_output_paused(const obs_output_t *output)
{
	return obs_output_valid(output, "obs_output_paused") ? os_atomic_load_bool(&output->paused) : false;
}

uint64_t obs_output_get_pause_offset(obs_output_t *output)
{
	uint64_t offset;

	if (!obs_output_valid(output, "obs_output_get_pause_offset"))
		return 0;

	pthread_mutex_lock(&output->pause.mutex);
	offset = output->pause.ts_offset;
	pthread_mutex_unlock(&output->pause.mutex);

	return offset;
}

signal_handler_t *obs_output_get_signal_handler(const obs_output_t *output)
{
	return obs_output_valid(output, "obs_output_get_signal_handler") ? output->context.signals : NULL;
}

proc_handler_t *obs_output_get_proc_handler(const obs_output_t *output)
{
	return obs_output_valid(output, "obs_output_get_proc_handler") ? output->context.procs : NULL;
}

void obs_output_set_media(obs_output_t *output, video_t *video, audio_t *audio)
{
	if (!obs_output_valid(output, "obs_output_set_media"))
		return;
	if (log_flag_encoded(output, __FUNCTION__, true))
		return;

	if (flag_video(output))
		output->video = video;
	if (flag_audio(output))
		output->audio = audio;
}

video_t *obs_output_video(const obs_output_t *output)
{
	if (!obs_output_valid(output, "obs_output_video"))
		return NULL;

	if (!flag_encoded(output))
		return output->video;

	obs_encoder_t *vencoder = obs_output_get_video_encoder(output);
	return obs_encoder_video(vencoder);
}

audio_t *obs_output_audio(const obs_output_t *output)
{
	if (!obs_output_valid(output, "obs_output_audio"))
		return NULL;

	if (!flag_encoded(output))
		return output->audio;

	for (size_t i = 0; i < MAX_OUTPUT_AUDIO_ENCODERS; i++) {
		if (output->audio_encoders[i])
			return obs_encoder_audio(output->audio_encoders[i]);
	}

	return NULL;
}

static inline size_t get_first_mixer(const obs_output_t *output)
{
	for (size_t i = 0; i < MAX_AUDIO_MIXES; i++) {
		if ((((size_t)1 << i) & output->mixer_mask) != 0) {
			return i;
		}
	}

	return 0;
}

void obs_output_set_mixer(obs_output_t *output, size_t mixer_idx)
{
	if (!obs_output_valid(output, "obs_output_set_mixer"))
		return;
	if (log_flag_encoded(output, __FUNCTION__, true))
		return;
	if (active(output))
		return;

	output->mixer_mask = (size_t)1 << mixer_idx;
}

size_t obs_output_get_mixer(const obs_output_t *output)
{
	if (!obs_output_valid(output, "obs_output_get_mixer"))
		return 0;

	return get_first_mixer(output);
}

void obs_output_set_mixers(obs_output_t *output, size_t mixers)
{
	if (!obs_output_valid(output, "obs_output_set_mixers"))
		return;
	if (log_flag_encoded(output, __FUNCTION__, true))
		return;
	if (active(output))
		return;

	output->mixer_mask = mixers;
}

size_t obs_output_get_mixers(const obs_output_t *output)
{
	return obs_output_valid(output, "obs_output_get_mixers") ? output->mixer_mask : 0;
}

void obs_output_remove_encoder_internal(struct obs_output *output, struct obs_encoder *encoder)
{
	if (!obs_output_valid(output, "obs_output_remove_encoder_internal"))
		return;

	if (encoder->info.type == OBS_ENCODER_VIDEO) {
		for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
			obs_encoder_t *video = output->video_encoders[i];
			if (video == encoder) {
				output->video_encoders[i] = NULL;
				obs_encoder_release(video);
			}
		}
	} else if (encoder->info.type == OBS_ENCODER_AUDIO) {
		for (size_t i = 0; i < MAX_OUTPUT_AUDIO_ENCODERS; i++) {
			obs_encoder_t *audio = output->audio_encoders[i];
			if (audio == encoder) {
				output->audio_encoders[i] = NULL;
				obs_encoder_release(audio);
			}
		}
	}
}

void obs_output_remove_encoder(struct obs_output *output, struct obs_encoder *encoder)
{
	if (!obs_output_valid(output, "obs_output_remove_encoder"))
		return;
	if (active(output))
		return;

	obs_output_remove_encoder_internal(output, encoder);
}

static struct caption_track_data *create_caption_track()
{
	struct caption_track_data *rval = bzalloc(sizeof(struct caption_track_data));
	pthread_mutex_init_value(&rval->caption_mutex);

	if (pthread_mutex_init(&rval->caption_mutex, NULL) != 0) {
		bfree(rval);
		rval = NULL;
	}
	return rval;
}

void obs_output_set_video_encoder2(obs_output_t *output, obs_encoder_t *encoder, size_t idx)
{
	if (!obs_output_valid(output, "obs_output_set_video_encoder2"))
		return;
	if (!log_flag_encoded(output, __FUNCTION__, false) || !log_flag_video(output, __FUNCTION__))
		return;
	if (encoder && encoder->info.type != OBS_ENCODER_VIDEO) {
		blog(LOG_WARNING, "obs_output_set_video_encoder: "
				  "encoder passed is not a video encoder");
		return;
	}
	if (active(output)) {
		blog(LOG_WARNING,
		     "%s: tried to set video encoder on output \"%s\" "
		     "while the output is still active!",
		     __FUNCTION__, output->context.name);
		return;
	}

	if ((output->info.flags & OBS_OUTPUT_MULTI_TRACK_VIDEO) != 0) {
		if (idx >= MAX_OUTPUT_VIDEO_ENCODERS) {
			return;
		}
	} else {
		if (idx > 0) {
			return;
		}
	}

	if (output->video_encoders[idx] == encoder)
		return;

	obs_encoder_remove_output(output->video_encoders[idx], output);
	obs_encoder_release(output->video_encoders[idx]);

	output->video_encoders[idx] = obs_encoder_get_ref(encoder);
	obs_encoder_add_output(output->video_encoders[idx], output);

	destroy_caption_track(&output->caption_tracks[idx]);
	if (encoder != NULL) {
		output->caption_tracks[idx] = create_caption_track();
	} else {
		output->caption_tracks[idx] = NULL;
	}

	// Set preferred resolution on the default index to preserve old behavior
	if (idx == 0) {
		/* set the preferred resolution on the encoder */
		if (output->scaled_width && output->scaled_height)
			obs_encoder_set_scaled_size(output->video_encoders[idx], output->scaled_width,
						    output->scaled_height);
	}
}

void obs_output_set_video_encoder(obs_output_t *output, obs_encoder_t *encoder)
{
	if (!obs_output_valid(output, "obs_output_set_video_encoder"))
		return;

	obs_output_set_video_encoder2(output, encoder, 0);
}

void obs_output_set_audio_encoder(obs_output_t *output, obs_encoder_t *encoder, size_t idx)
{
	if (!obs_output_valid(output, "obs_output_set_audio_encoder"))
		return;
	if (!log_flag_encoded(output, __FUNCTION__, false) || !log_flag_audio(output, __FUNCTION__))
		return;
	if (encoder && encoder->info.type != OBS_ENCODER_AUDIO) {
		blog(LOG_WARNING, "obs_output_set_audio_encoder: "
				  "encoder passed is not an audio encoder");
		return;
	}
	if (active(output)) {
		blog(LOG_WARNING,
		     "%s: tried to set audio encoder %d on output \"%s\" "
		     "while the output is still active!",
		     __FUNCTION__, (int)idx, output->context.name);
		return;
	}

	if ((output->info.flags & OBS_OUTPUT_MULTI_TRACK_AUDIO) != 0) {
		if (idx >= MAX_OUTPUT_AUDIO_ENCODERS) {
			return;
		}
	} else {
		if (idx > 0) {
			return;
		}
	}

	if (output->audio_encoders[idx] == encoder)
		return;

	obs_encoder_remove_output(output->audio_encoders[idx], output);
	obs_encoder_release(output->audio_encoders[idx]);

	output->audio_encoders[idx] = obs_encoder_get_ref(encoder);
	obs_encoder_add_output(output->audio_encoders[idx], output);
}

obs_encoder_t *obs_output_get_video_encoder2(const obs_output_t *output, size_t idx)
{
	if (!obs_output_valid(output, "obs_output_get_video_encoder2"))
		return NULL;

	if (idx >= MAX_OUTPUT_VIDEO_ENCODERS)
		return NULL;

	return output->video_encoders[idx];
}

obs_encoder_t *obs_output_get_video_encoder(const obs_output_t *output)
{
	if (!obs_output_valid(output, "obs_output_get_video_encoder"))
		return NULL;

	size_t first_venc_idx;
	if (get_first_video_encoder_index(output, &first_venc_idx))
		return obs_output_get_video_encoder2(output, first_venc_idx);
	else
		return NULL;
}

obs_encoder_t *obs_output_get_audio_encoder(const obs_output_t *output, size_t idx)
{
	if (!obs_output_valid(output, "obs_output_get_audio_encoder"))
		return NULL;

	if (idx >= MAX_OUTPUT_AUDIO_ENCODERS)
		return NULL;

	return output->audio_encoders[idx];
}

void obs_output_set_service(obs_output_t *output, obs_service_t *service)
{
	if (!obs_output_valid(output, "obs_output_set_service"))
		return;
	if (!log_flag_service(output, __FUNCTION__) || active(output) || !service || service->active)
		return;

	if (service->output)
		service->output->service = NULL;

	output->service = service;
	service->output = output;
}

obs_service_t *obs_output_get_service(const obs_output_t *output)
{
	return obs_output_valid(output, "obs_output_get_service") ? output->service : NULL;
}

void obs_output_set_reconnect_settings(obs_output_t *output, int retry_count, int retry_sec)
{
	if (!obs_output_valid(output, "obs_output_set_reconnect_settings"))
		return;

	output->reconnect_retry_max = retry_count;
	output->reconnect_retry_sec = retry_sec;
}

uint64_t obs_output_get_total_bytes(const obs_output_t *output)
{
	if (!obs_output_valid(output, "obs_output_get_total_bytes"))
		return 0;
	if (!output->info.get_total_bytes)
		return 0;

	if (delay_active(output) && !delay_capturing(output))
		return 0;

	return output->info.get_total_bytes(output->context.data);
}

int obs_output_get_frames_dropped(const obs_output_t *output)
{
	if (!obs_output_valid(output, "obs_output_get_frames_dropped"))
		return 0;
	if (!output->info.get_dropped_frames)
		return 0;

	return output->info.get_dropped_frames(output->context.data);
}

int obs_output_get_total_frames(const obs_output_t *output)
{
	return obs_output_valid(output, "obs_output_get_total_frames") ? output->total_frames : 0;
}

void obs_output_set_preferred_size2(obs_output_t *output, uint32_t width, uint32_t height, size_t idx)
{
	if (!obs_output_valid(output, "obs_output_set_preferred_size2"))
		return;
	if (!log_flag_video(output, __FUNCTION__))
		return;
	if (idx >= MAX_OUTPUT_VIDEO_ENCODERS)
		return;

	if (active(output)) {
		blog(LOG_WARNING,
		     "output '%s': Cannot set the preferred "
		     "resolution while the output is active",
		     obs_output_get_name(output));
		return;
	}

	// Used for raw video output
	if (idx == 0) {
		output->scaled_width = width;
		output->scaled_height = height;
	}

	if (flag_encoded(output)) {
		if (output->video_encoders[idx])
			obs_encoder_set_scaled_size(output->video_encoders[idx], width, height);
	}
}

void obs_output_set_preferred_size(obs_output_t *output, uint32_t width, uint32_t height)
{
	if (!obs_output_valid(output, "obs_output_set_preferred_size"))
		return;
	if (!log_flag_video(output, __FUNCTION__))
		return;

	obs_output_set_preferred_size2(output, width, height, 0);
}

uint32_t obs_output_get_width2(const obs_output_t *output, size_t idx)
{
	if (!obs_output_valid(output, "obs_output_get_width2"))
		return 0;
	if (!log_flag_video(output, __FUNCTION__))
		return 0;
	if (idx >= MAX_OUTPUT_VIDEO_ENCODERS)
		return 0;

	if (flag_encoded(output)) {
		if (output->video_encoders[idx])
			return obs_encoder_get_width(output->video_encoders[idx]);
		else
			return 0;
	} else
		return output->scaled_width != 0 ? output->scaled_width : video_output_get_width(output->video);
}

uint32_t obs_output_get_width(const obs_output_t *output)
{
	if (!obs_output_valid(output, "obs_output_get_width"))
		return 0;
	if (!log_flag_video(output, __FUNCTION__))
		return 0;

	return obs_output_get_width2(output, 0);
}

uint32_t obs_output_get_height2(const obs_output_t *output, size_t idx)
{
	if (!obs_output_valid(output, "obs_output_get_height2"))
		return 0;
	if (!log_flag_video(output, __FUNCTION__))
		return 0;
	if (idx >= MAX_OUTPUT_VIDEO_ENCODERS)
		return 0;

	if (flag_encoded(output)) {
		if (output->video_encoders[idx])
			return obs_encoder_get_height(output->video_encoders[idx]);
		else
			return 0;
	} else
		return output->scaled_height != 0 ? output->scaled_height : video_output_get_height(output->video);
}

uint32_t obs_output_get_height(const obs_output_t *output)
{
	if (!obs_output_valid(output, "obs_output_get_height"))
		return 0;
	if (!log_flag_video(output, __FUNCTION__))
		return 0;

	return obs_output_get_height2(output, 0);
}

void obs_output_set_video_conversion(obs_output_t *output, const struct video_scale_info *conversion)
{
	if (!obs_output_valid(output, "obs_output_set_video_conversion"))
		return;
	if (!obs_ptr_valid(conversion, "obs_output_set_video_conversion"))
		return;
	if (log_flag_encoded(output, __FUNCTION__, true) || !log_flag_video(output, __FUNCTION__))
		return;

	output->video_conversion = *conversion;
	output->video_conversion_set = true;
}

void obs_output_set_audio_conversion(obs_output_t *output, const struct audio_convert_info *conversion)
{
	if (!obs_output_valid(output, "obs_output_set_audio_conversion"))
		return;
	if (!obs_ptr_valid(conversion, "obs_output_set_audio_conversion"))
		return;
	if (log_flag_encoded(output, __FUNCTION__, true) || !log_flag_audio(output, __FUNCTION__))
		return;

	output->audio_conversion = *conversion;
	output->audio_conversion_set = true;
}

static inline bool video_valid(const struct obs_output *output)
{
	if (flag_encoded(output)) {
		for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
			if (output->video_encoders[i]) {
				return true;
			}
		}
		return false;
	} else {
		return output->video != NULL;
	}
}

static inline bool audio_valid(const struct obs_output *output)
{
	if (flag_encoded(output)) {
		for (size_t i = 0; i < MAX_OUTPUT_AUDIO_ENCODERS; i++) {
			if (output->audio_encoders[i]) {
				return true;
			}
		}
		return false;
	}

	return output->audio != NULL;
}

static bool can_begin_data_capture(const struct obs_output *output)
{
	if (flag_video(output) && !video_valid(output))
		return false;

	if (flag_audio(output) && !audio_valid(output))
		return false;

	if (flag_service(output) && !output->service)
		return false;

	return true;
}

static inline bool has_scaling(const struct obs_output *output)
{
	uint32_t video_width = video_output_get_width(output->video);
	uint32_t video_height = video_output_get_height(output->video);

	return output->scaled_width && output->scaled_height &&
	       (video_width != output->scaled_width || video_height != output->scaled_height);
}

const struct video_scale_info *obs_output_get_video_conversion(struct obs_output *output)
{
	if (log_flag_encoded(output, __FUNCTION__, true) || !log_flag_video(output, __FUNCTION__))
		return NULL;

	if (output->video_conversion_set) {
		if (!output->video_conversion.width)
			output->video_conversion.width = obs_output_get_width(output);

		if (!output->video_conversion.height)
			output->video_conversion.height = obs_output_get_height(output);

		return &output->video_conversion;

	} else if (has_scaling(output)) {
		const struct video_output_info *info = video_output_get_info(output->video);

		output->video_conversion.format = info->format;
		output->video_conversion.colorspace = VIDEO_CS_DEFAULT;
		output->video_conversion.range = VIDEO_RANGE_DEFAULT;
		output->video_conversion.width = output->scaled_width;
		output->video_conversion.height = output->scaled_height;
		return &output->video_conversion;
	}

	return NULL;
}

static inline struct audio_convert_info *get_audio_conversion(struct obs_output *output)
{
	return output->audio_conversion_set ? &output->audio_conversion : NULL;
}

static size_t get_encoder_index(const struct obs_output *output, struct encoder_packet *pkt)
{
	if (pkt->type == OBS_ENCODER_VIDEO) {
		for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
			struct obs_encoder *encoder = output->video_encoders[i];

			if (encoder && pkt->encoder == encoder)
				return i;
		}
	} else if (pkt->type == OBS_ENCODER_AUDIO) {
		for (size_t i = 0; i < MAX_OUTPUT_AUDIO_ENCODERS; i++) {
			struct obs_encoder *encoder = output->audio_encoders[i];

			if (encoder && pkt->encoder == encoder)
				return i;
		}
	}

	assert(false);
	return 0;
}

static inline void check_received(struct obs_output *output, struct encoder_packet *out)
{
	if (out->type == OBS_ENCODER_VIDEO) {
		if (!output->received_video[out->track_idx])
			output->received_video[out->track_idx] = true;
	} else {
		if (!output->received_audio)
			output->received_audio = true;
	}
}

static inline void apply_interleaved_packet_offset(struct obs_output *output, struct encoder_packet *out,
						   struct encoder_packet_time *packet_time)
{
	int64_t offset;

	/* audio and video need to start at timestamp 0, and the encoders
	 * may not currently be at 0 when we get data.  so, we store the
	 * current dts as offset and subtract that value from the dts/pts
	 * of the output packet. */
	offset = (out->type == OBS_ENCODER_VIDEO) ? output->video_offsets[out->track_idx]
						  : output->audio_offsets[out->track_idx];

	out->dts -= offset;
	out->pts -= offset;
	if (packet_time)
		packet_time->pts -= offset;

	/* convert the newly adjusted dts to relative dts time to ensure proper
	 * interleaving.  if we're using an audio encoder that's already been
	 * started on another output, then the first audio packet may not be
	 * quite perfectly synced up in terms of system time (and there's
	 * nothing we can really do about that), but it will always at least be
	 * within a 23ish millisecond threshold (at least for AAC) */
	out->dts_usec = packet_dts_usec(out);
}

static inline bool has_higher_opposing_ts(struct obs_output *output, struct encoder_packet *packet)
{
	bool has_higher = true;

	for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
		if (!output->video_encoders[i] || (packet->type == OBS_ENCODER_VIDEO && i == packet->track_idx))
			continue;
		has_higher = has_higher && output->highest_video_ts[i] > packet->dts_usec;
	}

	return packet->type == OBS_ENCODER_AUDIO ? has_higher
						 : (has_higher && output->highest_audio_ts > packet->dts_usec);
}

static size_t extract_buffer_from_sei(sei_t *sei, uint8_t **data_out)
{
	if (!sei || !sei->head) {
		return 0;
	}
	/* We should only need to get one payload, because the SEI that was
	 * generated should only have one message, so no need to iterate. If
	 * we did iterate, we would need to generate multiple OBUs. */
	sei_message_t *msg = sei_message_head(sei);
	int payload_size = (int)sei_message_size(msg);
	uint8_t *payload_data = sei_message_data(msg);
	*data_out = bmalloc(payload_size);
	memcpy(*data_out, payload_data, payload_size);
	return payload_size;
}

static const uint8_t nal_start[4] = {0, 0, 0, 1};
static bool add_caption(struct obs_output *output, struct encoder_packet *out)
{
	struct encoder_packet backup = *out;
	sei_t sei;
	uint8_t *data = NULL;
	size_t size;
	long ref = 1;
	bool avc = false;
	bool hevc = false;
	bool av1 = false;

	/* Instead of exiting early for unsupported codecs, we will continue
	 * processing to allow the freeing of caption data even if the captions
	 * will not be included in the bitstream due to being unimplemented in
	 * the given codec. */
	if (strcmp(out->encoder->info.codec, "h264") == 0) {
		avc = true;
	} else if (strcmp(out->encoder->info.codec, "av1") == 0) {
		av1 = true;
#ifdef ENABLE_HEVC
	} else if (strcmp(out->encoder->info.codec, "hevc") == 0) {
		hevc = true;
#endif
	}

	DARRAY(uint8_t) out_data;

	if (out->priority > 1)
		return false;

	struct caption_track_data *ctrack = output->caption_tracks[out->track_idx];
	if (!ctrack) {
		blog(LOG_DEBUG, "Caption track for index: %lu has not been initialized", out->track_idx);
		return false;
	}

#ifdef ENABLE_HEVC
	uint8_t hevc_nal_header[2];
	if (hevc) {
		size_t nal_header_index_start = 4;
		// Skip past the annex-b start code
		if (memcmp(out->data, nal_start + 1, 3) == 0) {
			nal_header_index_start = 3;
		} else if (memcmp(out->data, nal_start, 4) == 0) {
			nal_header_index_start = 4;

		} else {
			/* We shouldn't ever see this unless we start getting
			 * packets without annex-b start codes. */
			blog(LOG_DEBUG, "Annex-B start code not found. We may not "
					"generate a valid HEVC NAL unit header "
					"for our caption");
			return false;
		}
		/* We will use the same 2 byte NAL unit header for the CC SEI,
		 * but swap the NAL types out. */
		hevc_nal_header[0] = out->data[nal_header_index_start];
		hevc_nal_header[1] = out->data[nal_header_index_start + 1];
	}
#endif
	sei_init(&sei, 0.0);

	da_init(out_data);
	da_push_back_array(out_data, (uint8_t *)&ref, sizeof(ref));
	da_push_back_array(out_data, out->data, out->size);

	if (ctrack->caption_data.size > 0) {

		cea708_t cea708;
		cea708_init(&cea708, 0); // set up a new popon frame
		void *caption_buf = bzalloc(3 * sizeof(uint8_t));

		while (ctrack->caption_data.size > 0) {
			deque_pop_front(&ctrack->caption_data, caption_buf, 3 * sizeof(uint8_t));

			if ((((uint8_t *)caption_buf)[0] & 0x3) != 0) {
				// only send cea 608
				continue;
			}

			uint16_t captionData = ((uint8_t *)caption_buf)[1];
			captionData = captionData << 8;
			captionData += ((uint8_t *)caption_buf)[2];

			// padding
			if (captionData == 0x8080) {
				continue;
			}

			if (captionData == 0) {
				continue;
			}

			if (!eia608_parity_varify(captionData)) {
				continue;
			}

			cea708_add_cc_data(&cea708, 1, ((uint8_t *)caption_buf)[0] & 0x3, captionData);
		}

		bfree(caption_buf);

		sei_message_t *msg = sei_message_new(sei_type_user_data_registered_itu_t_t35, 0, CEA608_MAX_SIZE);
		msg->size = cea708_render(&cea708, sei_message_data(msg), sei_message_size(msg));
		sei_message_append(&sei, msg);
	} else if (ctrack->caption_head) {
		caption_frame_t cf;
		caption_frame_init(&cf);
		caption_frame_from_text(&cf, &ctrack->caption_head->text[0]);

		sei_from_caption_frame(&sei, &cf);

		struct caption_text *next = ctrack->caption_head->next;
		bfree(ctrack->caption_head);
		ctrack->caption_head = next;
	}

	if (avc || hevc || av1) {
		if (avc || hevc) {
			data = bmalloc(sei_render_size(&sei));
			size = sei_render(&sei, data);
		}
		/* In each of these specs there is an identical structure that
		 * carries caption information. It is named slightly differently
		 * in each one. The metadata_itut_t35 in AV1 or the
		 * user_data_registered_itu_t_t35 in HEVC/AVC. We have an AVC
		 * SEI wrapped version of that here. We will strip it out and
		 * repackage it slightly to fit the different codec carrying
		 * mechanisms. A slightly modified SEI for HEVC and a metadata
		 * OBU for AV1. */
		if (avc) {
			/* TODO: SEI should come after AUD/SPS/PPS,
			 * but before any VCL */
			da_push_back_array(out_data, nal_start, 4);
			da_push_back_array(out_data, data, size);
#ifdef ENABLE_HEVC
		} else if (hevc) {
			/* Only first NAL (VPS/PPS/SPS) should use the 4 byte
			 * start code. SEIs use 3 byte version */
			da_push_back_array(out_data, nal_start + 1, 3);
			/* nal_unit_header( ) {
			 * forbidden_zero_bit       f(1)
			 * nal_unit_type            u(6)
			 * nuh_layer_id             u(6)
			 * nuh_temporal_id_plus1    u(3)
			 * }
			 */
			const uint8_t prefix_sei_nal_type = 39;
			/* The first bit is always 0, so we just need to
			 * save the last bit off the original header and
			 * add the SEI NAL type. */
			uint8_t first_byte = (prefix_sei_nal_type << 1) | (0x01 & hevc_nal_header[0]);
			hevc_nal_header[0] = first_byte;
			/* The HEVC NAL unit header is 2 byte instead of
			 * one, otherwise everything else is the
			 * same. */
			da_push_back_array(out_data, hevc_nal_header, 2);
			da_push_back_array(out_data, &data[1], size - 1);
#endif
		} else if (av1) {
			uint8_t *obu_buffer = NULL;
			size_t obu_buffer_size = 0;
			size = extract_buffer_from_sei(&sei, &data);
			metadata_obu(data, size, &obu_buffer, &obu_buffer_size, METADATA_TYPE_ITUT_T35);
			if (obu_buffer) {
				da_push_back_array(out_data, obu_buffer, obu_buffer_size);
				bfree(obu_buffer);
			}
		}
		if (data) {
			bfree(data);
		}
		obs_encoder_packet_release(out);

		*out = backup;
		out->data = (uint8_t *)out_data.array + sizeof(ref);
		out->size = out_data.num - sizeof(ref);
	}
	sei_free(&sei);
	return avc || hevc || av1;
}

static inline void send_interleaved(struct obs_output *output)
{
	struct encoder_packet out = output->interleaved_packets.array[0];
	struct encoder_packet_time ept_local = {0};
	bool found_ept = false;

	da_erase(output->interleaved_packets, 0);

	if (out.type == OBS_ENCODER_VIDEO) {
		output->total_frames++;

		pthread_mutex_lock(&output->caption_tracks[out.track_idx]->caption_mutex);

		double frame_timestamp = (out.pts * out.timebase_num) / (double)out.timebase_den;

		struct caption_track_data *ctrack = output->caption_tracks[out.track_idx];

		if (ctrack->caption_head && ctrack->caption_timestamp <= frame_timestamp) {
			blog(LOG_DEBUG, "Sending caption: %f \"%s\"", frame_timestamp, &ctrack->caption_head->text[0]);

			double display_duration = ctrack->caption_head->display_duration;

			if (add_caption(output, &out)) {
				ctrack->caption_timestamp = frame_timestamp + display_duration;
			}
		}

		if (ctrack->caption_data.size > 0) {
			if (ctrack->last_caption_timestamp < frame_timestamp) {
				ctrack->last_caption_timestamp = frame_timestamp;
				add_caption(output, &out);
			}
		}
		pthread_mutex_unlock(&ctrack->caption_mutex);

		/* Iterate the array of encoder packet times to
		 * find a matching PTS entry, and drain the array.
		 * Packet timing currently applies to video only.
		 */
		struct encoder_packet_time *ept = NULL;
		size_t num_ept = output->encoder_packet_times[out.track_idx].num;
		if (num_ept) {
			for (size_t i = 0; i < num_ept; i++) {
				ept = &output->encoder_packet_times[out.track_idx].array[i];
				if (ept->pts == out.pts) {
					ept_local = *ept;
					da_erase(output->encoder_packet_times[out.track_idx], i);
					found_ept = true;
					break;
				}
			}
			if (found_ept == false) {
				blog(LOG_DEBUG, "%s: Track %lu encoder packet timing for PTS%" PRId64 " not found.",
				     __FUNCTION__, out.track_idx, out.pts);
			}
		} else {
			// encoder_packet_times should not be empty; log if so.
			blog(LOG_DEBUG, "%s: Track %lu encoder packet timing array empty.", __FUNCTION__,
			     out.track_idx);
		}
	}

	/* Iterate the registered packet callback(s) and invoke
	 * each one. The caption track logic further above should
	 * eventually migrate to the packet callback mechanism.
	 */
	pthread_mutex_lock(&output->pkt_callbacks_mutex);
	for (size_t i = 0; i < output->pkt_callbacks.num; ++i) {
		struct packet_callback *const callback = &output->pkt_callbacks.array[i];
		// Packet interleave request timestamp
		ept_local.pir = os_gettime_ns();
		callback->packet_cb(output, &out, found_ept ? &ept_local : NULL, callback->param);
	}
	pthread_mutex_unlock(&output->pkt_callbacks_mutex);

	output->info.encoded_packet(output->context.data, &out);
	obs_encoder_packet_release(&out);
}

static inline void set_higher_ts(struct obs_output *output, struct encoder_packet *packet)
{
	if (packet->type == OBS_ENCODER_VIDEO) {
		if (output->highest_video_ts[packet->track_idx] < packet->dts_usec)
			output->highest_video_ts[packet->track_idx] = packet->dts_usec;
	} else {
		if (output->highest_audio_ts < packet->dts_usec)
			output->highest_audio_ts = packet->dts_usec;
	}
}

static inline struct encoder_packet *find_first_packet_type(struct obs_output *output, enum obs_encoder_type type,
							    size_t audio_idx);
static int find_first_packet_type_idx(struct obs_output *output, enum obs_encoder_type type, size_t audio_idx);

/* gets the point where audio and video are closest together */
static size_t get_interleaved_start_idx(struct obs_output *output)
{
	int64_t closest_diff = 0x7FFFFFFFFFFFFFFFLL;
	struct encoder_packet *first_video = find_first_packet_type(output, OBS_ENCODER_VIDEO, 0);
	size_t video_idx = DARRAY_INVALID;
	size_t idx = 0;

	for (size_t i = 0; i < output->interleaved_packets.num; i++) {
		struct encoder_packet *packet = &output->interleaved_packets.array[i];
		int64_t diff;

		if (packet->type != OBS_ENCODER_AUDIO) {
			if (packet == first_video)
				video_idx = i;
			continue;
		}

		diff = llabs(packet->dts_usec - first_video->dts_usec);
		if (diff < closest_diff) {
			closest_diff = diff;
			idx = i;
		}
	}

	idx = video_idx < idx ? video_idx : idx;

	/* Early AAC/Opus audio packets will be for "priming" the encoder and contain silence, but they should not be
	 * discarded. Set the idx to the first audio packet if closest PTS was <= 0. */
	size_t first_audio_idx = idx;
	while (output->interleaved_packets.array[first_audio_idx].type != OBS_ENCODER_AUDIO)
		first_audio_idx++;

	if (output->interleaved_packets.array[first_audio_idx].pts <= 0) {
		for (size_t i = 0; i < MAX_OUTPUT_AUDIO_ENCODERS; i++) {
			int audio_idx = find_first_packet_type_idx(output, OBS_ENCODER_AUDIO, i);
			if (audio_idx >= 0 && (size_t)audio_idx < idx)
				idx = audio_idx;
		}
	}

	return idx;
}

static int64_t get_encoder_duration(struct obs_encoder *encoder)
{
	return (encoder->timebase_num * 1000000LL / encoder->timebase_den) * encoder->framesize;
}

static int prune_premature_packets(struct obs_output *output)
{
	struct encoder_packet *video;
	int video_idx;
	int max_idx;
	int64_t duration_usec, max_audio_duration_usec = 0;
	int64_t max_diff = 0;
	int64_t diff = 0;
	int audio_encoders = 0;

	video_idx = find_first_packet_type_idx(output, OBS_ENCODER_VIDEO, 0);
	if (video_idx == -1)
		return -1;

	max_idx = video_idx;
	video = &output->interleaved_packets.array[video_idx];
	duration_usec = video->timebase_num * 1000000LL / video->timebase_den;

	for (size_t i = 0; i < MAX_OUTPUT_AUDIO_ENCODERS; i++) {
		struct encoder_packet *audio;
		int audio_idx;
		int64_t audio_duration_usec = 0;

		if (!output->audio_encoders[i])
			continue;
		audio_encoders++;

		audio_idx = find_first_packet_type_idx(output, OBS_ENCODER_AUDIO, i);
		if (audio_idx == -1) {
			output->received_audio = false;
			return -1;
		}

		audio = &output->interleaved_packets.array[audio_idx];
		if (audio_idx > max_idx)
			max_idx = audio_idx;

		diff = audio->dts_usec - video->dts_usec;
		if (diff > max_diff)
			max_diff = diff;

		audio_duration_usec = get_encoder_duration(output->audio_encoders[i]);
		if (audio_duration_usec > max_audio_duration_usec)
			max_audio_duration_usec = audio_duration_usec;
	}

	/* Once multiple audio encoders are running they are almost always out
	 * of phase by ~Xms. If users change their video to > 100fps then it
	 * becomes probable that this phase difference will be larger than the
	 * video duration preventing us from ever finding a synchronization
	 * point due to their larger frame duration. Instead give up on a tight
	 * video sync. */
	if (audio_encoders > 1 && duration_usec < max_audio_duration_usec) {
		duration_usec = max_audio_duration_usec;
	}

	return diff > duration_usec ? max_idx + 1 : 0;
}

#define DEBUG_STARTING_PACKETS 0

static void discard_to_idx(struct obs_output *output, size_t idx)
{
	for (size_t i = 0; i < idx; i++) {
		struct encoder_packet *packet = &output->interleaved_packets.array[i];
#if DEBUG_STARTING_PACKETS == 1
		blog(LOG_DEBUG, "discarding %s packet, dts: %lld, pts: %lld",
		     packet->type == OBS_ENCODER_VIDEO ? "video" : "audio", packet->dts, packet->pts);
#endif
		if (packet->type == OBS_ENCODER_VIDEO) {
			da_pop_front(output->encoder_packet_times[packet->track_idx]);
		}
		obs_encoder_packet_release(packet);
	}

	da_erase_range(output->interleaved_packets, 0, idx);
}

static bool prune_interleaved_packets(struct obs_output *output)
{
	size_t start_idx = 0;
	int prune_start = prune_premature_packets(output);

#if DEBUG_STARTING_PACKETS == 1
	blog(LOG_DEBUG, "--------- Pruning! %d ---------", prune_start);
	for (size_t i = 0; i < output->interleaved_packets.num; i++) {
		struct encoder_packet *packet = &output->interleaved_packets.array[i];
		blog(LOG_DEBUG, "packet: %s %d, ts: %lld, pruned = %s",
		     packet->type == OBS_ENCODER_AUDIO ? "audio" : "video", (int)packet->track_idx, packet->dts_usec,
		     (int)i < prune_start ? "true" : "false");
	}
#endif

	/* prunes the first video packet if it's too far away from audio */
	if (prune_start == -1)
		return false;
	else if (prune_start != 0)
		start_idx = (size_t)prune_start;
	else
		start_idx = get_interleaved_start_idx(output);

	if (start_idx)
		discard_to_idx(output, start_idx);

	return true;
}

static int find_first_packet_type_idx(struct obs_output *output, enum obs_encoder_type type, size_t idx)
{
	for (size_t i = 0; i < output->interleaved_packets.num; i++) {
		struct encoder_packet *packet = &output->interleaved_packets.array[i];

		if (packet->type == type && packet->track_idx == idx)
			return (int)i;
	}

	return -1;
}

static int find_last_packet_type_idx(struct obs_output *output, enum obs_encoder_type type, size_t idx)
{
	for (size_t i = output->interleaved_packets.num; i > 0; i--) {
		struct encoder_packet *packet = &output->interleaved_packets.array[i - 1];

		if (packet->type == type && packet->track_idx == idx)
			return (int)(i - 1);
	}

	return -1;
}

static inline struct encoder_packet *find_first_packet_type(struct obs_output *output, enum obs_encoder_type type,
							    size_t audio_idx)
{
	int idx = find_first_packet_type_idx(output, type, audio_idx);
	return (idx != -1) ? &output->interleaved_packets.array[idx] : NULL;
}

static inline struct encoder_packet *find_last_packet_type(struct obs_output *output, enum obs_encoder_type type,
							   size_t audio_idx)
{
	int idx = find_last_packet_type_idx(output, type, audio_idx);
	return (idx != -1) ? &output->interleaved_packets.array[idx] : NULL;
}

static bool get_audio_and_video_packets(struct obs_output *output, struct encoder_packet **video,
					struct encoder_packet **audio)
{
	bool found_video = false;
	for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
		if (output->video_encoders[i]) {
			video[i] = find_first_packet_type(output, OBS_ENCODER_VIDEO, i);
			if (!video[i]) {
				output->received_video[i] = false;
				return false;
			} else {
				found_video = true;
			}
		}
	}

	for (size_t i = 0; i < MAX_OUTPUT_AUDIO_ENCODERS; i++) {
		if (output->audio_encoders[i]) {
			audio[i] = find_first_packet_type(output, OBS_ENCODER_AUDIO, i);
			if (!audio[i]) {
				output->received_audio = false;
				return false;
			}
		}
	}

	return found_video;
}

static bool initialize_interleaved_packets(struct obs_output *output)
{
	struct encoder_packet *video[MAX_OUTPUT_VIDEO_ENCODERS] = {0};
	struct encoder_packet *audio[MAX_OUTPUT_AUDIO_ENCODERS] = {0};
	struct encoder_packet *last_audio[MAX_OUTPUT_AUDIO_ENCODERS] = {0};
	size_t start_idx;
	size_t first_audio_idx;
	size_t first_video_idx;

	if (!get_first_audio_encoder_index(output, &first_audio_idx))
		return false;
	if (!get_first_video_encoder_index(output, &first_video_idx))
		return false;

	if (!get_audio_and_video_packets(output, video, audio))
		return false;

	for (size_t i = 0; i < MAX_OUTPUT_AUDIO_ENCODERS; i++) {
		if (output->audio_encoders[i]) {
			last_audio[i] = find_last_packet_type(output, OBS_ENCODER_AUDIO, i);
		}
	}

	/* ensure that there is audio past the first video packet */
	for (size_t i = 0; i < MAX_OUTPUT_AUDIO_ENCODERS; i++) {
		if (output->audio_encoders[i]) {
			if (last_audio[i]->dts_usec < video[first_video_idx]->dts_usec) {
				output->received_audio = false;
				return false;
			}
		}
	}

	/* clear out excess starting audio if it hasn't been already */
	start_idx = get_interleaved_start_idx(output);
	if (start_idx) {
		discard_to_idx(output, start_idx);
		if (!get_audio_and_video_packets(output, video, audio))
			return false;
	}

	/* get new offsets */
	for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
		if (output->video_encoders[i]) {
			output->video_offsets[i] = video[i]->pts;
		}
	}
	for (size_t i = 0; i < MAX_OUTPUT_AUDIO_ENCODERS; i++) {
		if (output->audio_encoders[i] && audio[i]->dts > 0) {
			output->audio_offsets[i] = audio[i]->dts;
		}
	}
#if DEBUG_STARTING_PACKETS == 1
	int64_t v = video[first_video_idx]->dts_usec;
	int64_t a = audio[first_audio_idx]->dts_usec;
	int64_t diff = v - a;

	blog(LOG_DEBUG,
	     "output '%s' offset for video: %lld, audio: %lld, "
	     "diff: %lldms",
	     output->context.name, v, a, diff / 1000LL);
#endif

	/* subtract offsets from highest TS offset variables */
	output->highest_audio_ts -= audio[first_audio_idx]->dts_usec;

	/* apply new offsets to all existing packet DTS/PTS values */
	for (size_t i = 0; i < output->interleaved_packets.num; i++) {
		struct encoder_packet *packet = &output->interleaved_packets.array[i];
		apply_interleaved_packet_offset(output, packet, NULL);
	}

	return true;
}

static inline void insert_interleaved_packet(struct obs_output *output, struct encoder_packet *out)
{
	size_t idx;
	for (idx = 0; idx < output->interleaved_packets.num; idx++) {
		struct encoder_packet *cur_packet;
		cur_packet = output->interleaved_packets.array + idx;

		// sort video packets with same DTS by track index,
		// to prevent the pruning logic from removing additional
		// video tracks
		if (out->dts_usec == cur_packet->dts_usec && out->type == OBS_ENCODER_VIDEO &&
		    cur_packet->type == OBS_ENCODER_VIDEO && out->track_idx > cur_packet->track_idx)
			continue;

		if (out->dts_usec == cur_packet->dts_usec && out->type == OBS_ENCODER_VIDEO) {
			break;
		} else if (out->dts_usec < cur_packet->dts_usec) {
			break;
		}
	}

	da_insert(output->interleaved_packets, idx, out);
}

static void resort_interleaved_packets(struct obs_output *output)
{
	DARRAY(struct encoder_packet) old_array;

	old_array.da = output->interleaved_packets.da;
	memset(&output->interleaved_packets, 0, sizeof(output->interleaved_packets));

	for (size_t i = 0; i < old_array.num; i++) {
		set_higher_ts(output, &old_array.array[i]);

		insert_interleaved_packet(output, &old_array.array[i]);
	}

	da_free(old_array);
}

static void discard_unused_audio_packets(struct obs_output *output, int64_t dts_usec)
{
	size_t idx = 0;

	for (; idx < output->interleaved_packets.num; idx++) {
		struct encoder_packet *p = &output->interleaved_packets.array[idx];

		if (p->dts_usec >= dts_usec)
			break;
	}

	if (idx)
		discard_to_idx(output, idx);
}

static bool purge_encoder_group_keyframe_data(obs_output_t *output, size_t idx)
{
	struct keyframe_group_data *data = &output->keyframe_group_tracking.array[idx];
	uint32_t modified_count = 0;
	for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
		if (data->seen_on_track[i] != KEYFRAME_TRACK_STATUS_NOT_SEEN)
			modified_count += 1;
	}

	if (modified_count == data->required_tracks) {
		da_erase(output->keyframe_group_tracking, idx);
		return true;
	}
	return false;
}

/* Check whether keyframes are emitted from all grouped encoders, and log
 * if keyframes haven't been emitted from all grouped encoders. */
static void check_encoder_group_keyframe_alignment(obs_output_t *output, struct encoder_packet *packet)
{
	size_t idx = 0;
	struct keyframe_group_data insert_data = {0};

	if (!packet->keyframe || packet->type != OBS_ENCODER_VIDEO || !packet->encoder->encoder_group)
		return;

	for (; idx < output->keyframe_group_tracking.num;) {
		struct keyframe_group_data *data = &output->keyframe_group_tracking.array[idx];
		if (data->pts > packet->pts)
			break;
		if (data->group_id != (uintptr_t)packet->encoder->encoder_group) {
			idx += 1;
			continue;
		}

		if (data->pts < packet->pts) {
			if (data->seen_on_track[packet->track_idx] == KEYFRAME_TRACK_STATUS_NOT_SEEN) {
				blog(LOG_WARNING,
				     "obs-output '%s': Missing keyframe with pts %" PRIi64
				     " for encoder '%s' (track: %zu)",
				     obs_output_get_name(output), data->pts, obs_encoder_get_name(packet->encoder),
				     packet->track_idx);
			}

			data->seen_on_track[packet->track_idx] = KEYFRAME_TRACK_STATUS_SKIPPED;

			if (!purge_encoder_group_keyframe_data(output, idx))
				idx += 1;
			continue;
		}

		data->seen_on_track[packet->track_idx] = KEYFRAME_TRACK_STATUS_SEEN;
		purge_encoder_group_keyframe_data(output, idx);
		return;
	}

	insert_data.group_id = (uintptr_t)packet->encoder->encoder_group;
	insert_data.pts = packet->pts;
	insert_data.seen_on_track[packet->track_idx] = KEYFRAME_TRACK_STATUS_SEEN;

	pthread_mutex_lock(&packet->encoder->encoder_group->mutex);
	insert_data.required_tracks = packet->encoder->encoder_group->num_encoders_started;
	pthread_mutex_unlock(&packet->encoder->encoder_group->mutex);

	da_insert(output->keyframe_group_tracking, idx, &insert_data);
}

static void apply_ept_offsets(struct obs_output *output)
{
	for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
		for (size_t j = 0; j < output->encoder_packet_times[i].num; j++) {
			output->encoder_packet_times[i].array[j].pts -= output->video_offsets[i];
		}
	}
}

static inline size_t count_streamable_frames(struct obs_output *output)
{
	size_t eligible = 0;

	for (size_t idx = 0; idx < output->interleaved_packets.num; idx++) {
		struct encoder_packet *pkt = &output->interleaved_packets.array[idx];

		/* Only count an interleaved packet as streamable if there are packets of the opposing type and of a
		 * higher timestamp in the interleave buffer. This ensures that the timestamps are monotonic. */
		if (!has_higher_opposing_ts(output, pkt))
			break;

		eligible++;
	}

	return eligible;
}

static void interleave_packets(void *data, struct encoder_packet *packet, struct encoder_packet_time *packet_time)
{
	struct obs_output *output = data;
	struct encoder_packet out;
	bool was_started;
	bool received_video;
	struct encoder_packet_time *output_packet_time = NULL;

	if (!active(output))
		return;

	packet->track_idx = get_encoder_index(output, packet);

	pthread_mutex_lock(&output->interleaved_mutex);

	/* if first video frame is not a keyframe, discard until received */
	if (packet->type == OBS_ENCODER_VIDEO && !output->received_video[packet->track_idx] && !packet->keyframe) {
		discard_unused_audio_packets(output, packet->dts_usec);
		pthread_mutex_unlock(&output->interleaved_mutex);

		if (output->active_delay_ns)
			obs_encoder_packet_release(packet);
		return;
	}

	received_video = true;
	for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
		if (output->video_encoders[i])
			received_video = received_video && output->received_video[i];
	}

	check_encoder_group_keyframe_alignment(output, packet);

	was_started = output->received_audio && received_video;

	if (output->active_delay_ns)
		out = *packet;
	else
		obs_encoder_packet_create_instance(&out, packet);

	if (packet_time) {
		output_packet_time = da_push_back_new(output->encoder_packet_times[packet->track_idx]);
		*output_packet_time = *packet_time;
	}

	if (was_started)
		apply_interleaved_packet_offset(output, &out, output_packet_time);
	else
		check_received(output, packet);

	insert_interleaved_packet(output, &out);

	received_video = true;
	for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
		if (output->video_encoders[i])
			received_video = received_video && output->received_video[i];
	}

	/* when both video and audio have been received, we're ready
	 * to start sending out packets (one at a time) */
	if (output->received_audio && received_video) {
		if (!was_started) {
			if (prune_interleaved_packets(output)) {
				if (initialize_interleaved_packets(output)) {
					resort_interleaved_packets(output);
					apply_ept_offsets(output);
					send_interleaved(output);
				}
			}
		} else {
			set_higher_ts(output, &out);

			size_t streamable = count_streamable_frames(output);
			if (streamable) {
				send_interleaved(output);

				/* If we have more eligible packets queued than we normally should have,
				 * send one additional packet until we're back below the limit. */
				if (--streamable > output->interleaver_max_batch_size)
					send_interleaved(output);
			}
		}
	}

	pthread_mutex_unlock(&output->interleaved_mutex);
}

static void default_encoded_callback(void *param, struct encoder_packet *packet,
				     struct encoder_packet_time *packet_time)
{
	UNUSED_PARAMETER(packet_time);
	struct obs_output *output = param;

	if (data_active(output)) {
		packet->track_idx = get_encoder_index(output, packet);

		output->info.encoded_packet(output->context.data, packet);

		if (packet->type == OBS_ENCODER_VIDEO)
			output->total_frames++;
	}

	if (output->active_delay_ns)
		obs_encoder_packet_release(packet);
}

static void default_raw_video_callback(void *param, struct video_data *frame)
{
	struct obs_output *output = param;

	if (video_pause_check(&output->pause, frame->timestamp))
		return;

	if (data_active(output))
		output->info.raw_video(output->context.data, frame);
	output->total_frames++;
}

static bool prepare_audio(struct obs_output *output, const struct audio_data *old, struct audio_data *new)
{
	if ((output->info.flags & OBS_OUTPUT_VIDEO) == 0) {
		*new = *old;
		return true;
	}

	if (!output->video_start_ts) {
		pthread_mutex_lock(&output->pause.mutex);
		output->video_start_ts = output->pause.last_video_ts;
		pthread_mutex_unlock(&output->pause.mutex);
	}

	if (!output->video_start_ts)
		return false;

	/* ------------------ */

	*new = *old;

	if (old->timestamp < output->video_start_ts) {
		uint64_t duration = util_mul_div64(old->frames, 1000000000ULL, output->sample_rate);
		uint64_t end_ts = (old->timestamp + duration);
		uint64_t cutoff;

		if (end_ts <= output->video_start_ts)
			return false;

		cutoff = output->video_start_ts - old->timestamp;
		new->timestamp += cutoff;

		cutoff = util_mul_div64(cutoff, output->sample_rate, 1000000000ULL);

		for (size_t i = 0; i < output->planes; i++)
			new->data[i] += output->audio_size *(uint32_t)cutoff;
		new->frames -= (uint32_t)cutoff;
	}

	return true;
}

static void default_raw_audio_callback(void *param, size_t mix_idx, struct audio_data *in)
{
	struct obs_output *output = param;
	struct audio_data out;
	size_t frame_size_bytes;

	if (!data_active(output))
		return;

	/* -------------- */

	if (!prepare_audio(output, in, &out))
		return;
	if (audio_pause_check(&output->pause, &out, output->sample_rate))
		return;
	if (!output->audio_start_ts) {
		output->audio_start_ts = out.timestamp;
	}

	frame_size_bytes = AUDIO_OUTPUT_FRAMES * output->audio_size;

	for (size_t i = 0; i < output->planes; i++)
		deque_push_back(&output->audio_buffer[mix_idx][i], out.data[i], out.frames * output->audio_size);

	/* -------------- */

	while (output->audio_buffer[mix_idx][0].size > frame_size_bytes) {
		for (size_t i = 0; i < output->planes; i++) {
			deque_pop_front(&output->audio_buffer[mix_idx][i], output->audio_data[i], frame_size_bytes);
			out.data[i] = (uint8_t *)output->audio_data[i];
		}

		out.frames = AUDIO_OUTPUT_FRAMES;
		out.timestamp =
			output->audio_start_ts + audio_frames_to_ns(output->sample_rate, output->total_audio_frames);

		pthread_mutex_lock(&output->pause.mutex);
		out.timestamp += output->pause.ts_offset;
		pthread_mutex_unlock(&output->pause.mutex);

		output->total_audio_frames += AUDIO_OUTPUT_FRAMES;

		if (output->info.raw_audio2)
			output->info.raw_audio2(output->context.data, mix_idx, &out);
		else
			output->info.raw_audio(output->context.data, &out);
	}
}

static inline void start_audio_encoders(struct obs_output *output, encoded_callback_t encoded_callback)
{
	for (size_t i = 0; i < MAX_OUTPUT_AUDIO_ENCODERS; i++) {
		if (output->audio_encoders[i]) {
			obs_encoder_start(output->audio_encoders[i], encoded_callback, output);
		}
	}
}

static inline void start_video_encoders(struct obs_output *output, encoded_callback_t encoded_callback)
{
	for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
		if (output->video_encoders[i]) {
			obs_encoder_start(output->video_encoders[i], encoded_callback, output);
		}
	}
}

static inline void start_raw_audio(obs_output_t *output)
{
	if (output->info.raw_audio2) {
		for (int idx = 0; idx < MAX_AUDIO_MIXES; idx++) {
			if ((output->mixer_mask & ((size_t)1 << idx)) != 0) {
				audio_output_connect(output->audio, idx, get_audio_conversion(output),
						     default_raw_audio_callback, output);
			}
		}
	} else {
		audio_output_connect(output->audio, get_first_mixer(output), get_audio_conversion(output),
				     default_raw_audio_callback, output);
	}
}

static void reset_packet_data(obs_output_t *output)
{
	output->received_audio = false;
	output->highest_audio_ts = 0;

	for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
		output->encoder_packet_times[i].num = 0;
	}

	for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
		output->received_video[i] = false;
		output->video_offsets[i] = 0;
		output->highest_video_ts[i] = INT64_MIN;
	}
	for (size_t i = 0; i < MAX_OUTPUT_AUDIO_ENCODERS; i++)
		output->audio_offsets[i] = 0;

	free_packets(output);
}

static inline bool preserve_active(struct obs_output *output)
{
	return (output->delay_flags & OBS_OUTPUT_DELAY_PRESERVE) != 0;
}

static void hook_data_capture(struct obs_output *output)
{
	encoded_callback_t encoded_callback;
	bool has_video = flag_video(output);
	bool has_audio = flag_audio(output);

	if (flag_encoded(output)) {
		pthread_mutex_lock(&output->interleaved_mutex);
		reset_packet_data(output);
		pthread_mutex_unlock(&output->interleaved_mutex);

		encoded_callback = (has_video && has_audio) ? interleave_packets : default_encoded_callback;

		if (output->delay_sec) {
			output->active_delay_ns = (uint64_t)output->delay_sec * 1000000000ULL;
			output->delay_cur_flags = output->delay_flags;
			output->delay_callback = encoded_callback;
			encoded_callback = process_delay;
			os_atomic_set_bool(&output->delay_active, true);

			blog(LOG_INFO,
			     "Output '%s': %" PRIu32 " second delay "
			     "active, preserve on disconnect is %s",
			     output->context.name, output->delay_sec, preserve_active(output) ? "on" : "off");
		}

		if (has_audio)
			start_audio_encoders(output, encoded_callback);
		if (has_video)
			start_video_encoders(output, encoded_callback);
	} else {
		if (has_video)
			start_raw_video(output->video, obs_output_get_video_conversion(output), 1,
					default_raw_video_callback, output);
		if (has_audio)
			start_raw_audio(output);
	}
}

static inline void signal_start(struct obs_output *output)
{
	do_output_signal(output, "start");
}

static inline void signal_reconnect(struct obs_output *output)
{
	struct calldata params;
	uint8_t stack[128];

	calldata_init_fixed(&params, stack, sizeof(stack));
	calldata_set_int(&params, "timeout_sec", output->reconnect_retry_cur_msec / 1000);
	calldata_set_ptr(&params, "output", output);
	signal_handler_signal(output->context.signals, "reconnect", &params);
}

static inline void signal_reconnect_success(struct obs_output *output)
{
	do_output_signal(output, "reconnect_success");
}

static inline void signal_stop(struct obs_output *output)
{
	struct calldata params;

	calldata_init(&params);
	calldata_set_string(&params, "last_error", obs_output_get_last_error(output));
	calldata_set_int(&params, "code", output->stop_code);
	calldata_set_ptr(&params, "output", output);

	signal_handler_signal(output->context.signals, "stop", &params);

	calldata_free(&params);
}

bool obs_output_can_begin_data_capture(const obs_output_t *output, uint32_t flags)
{
	UNUSED_PARAMETER(flags);

	if (!obs_output_valid(output, "obs_output_can_begin_data_capture"))
		return false;

	if (delay_active(output))
		return true;
	if (active(output))
		return false;

	if (data_capture_ending(output))
		pthread_join(output->end_data_capture_thread, NULL);

	return can_begin_data_capture(output);
}

static inline bool initialize_audio_encoders(obs_output_t *output)
{
	for (size_t i = 0; i < MAX_OUTPUT_AUDIO_ENCODERS; i++) {
		obs_encoder_t *audio = output->audio_encoders[i];

		if (audio && !obs_encoder_initialize(audio)) {
			obs_output_set_last_error(output, obs_encoder_get_last_error(audio));
			return false;
		}
	}

	return true;
}

static inline bool initialize_video_encoders(obs_output_t *output)
{
	for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
		obs_encoder_t *video = output->video_encoders[i];

		if (video && !obs_encoder_initialize(video)) {
			obs_output_set_last_error(output, obs_encoder_get_last_error(video));
			return false;
		}
	}

	return true;
}

static inline void pair_encoders(obs_output_t *output)
{
	size_t first_venc_idx;
	if (!get_first_video_encoder_index(output, &first_venc_idx))
		return;
	struct obs_encoder *video = output->video_encoders[first_venc_idx];

	pthread_mutex_lock(&video->init_mutex);
	if (video->active) {
		pthread_mutex_unlock(&video->init_mutex);
		return;
	}

	for (size_t i = 0; i < MAX_OUTPUT_AUDIO_ENCODERS; i++) {
		struct obs_encoder *audio = output->audio_encoders[i];
		if (!audio)
			continue;

		pthread_mutex_lock(&audio->init_mutex);
		if (!audio->active && !audio->paired_encoders.num) {
			obs_weak_encoder_t *weak_audio = obs_encoder_get_weak_encoder(audio);
			obs_weak_encoder_t *weak_video = obs_encoder_get_weak_encoder(video);
			da_push_back(video->paired_encoders, &weak_audio);
			da_push_back(audio->paired_encoders, &weak_video);
		}
		pthread_mutex_unlock(&audio->init_mutex);
	}
	pthread_mutex_unlock(&video->init_mutex);
}

bool obs_output_initialize_encoders(obs_output_t *output, uint32_t flags)
{
	UNUSED_PARAMETER(flags);

	if (!obs_output_valid(output, "obs_output_initialize_encoders"))
		return false;
	if (!log_flag_encoded(output, __FUNCTION__, false))
		return false;
	if (active(output))
		return delay_active(output);

	if (flag_video(output) && !initialize_video_encoders(output))
		return false;
	if (flag_audio(output) && !initialize_audio_encoders(output))
		return false;

	return true;
}

static bool begin_delayed_capture(obs_output_t *output)
{
	if (delay_capturing(output))
		return false;

	pthread_mutex_lock(&output->interleaved_mutex);
	reset_packet_data(output);
	os_atomic_set_bool(&output->delay_capturing, true);
	pthread_mutex_unlock(&output->interleaved_mutex);

	if (reconnecting(output)) {
		signal_reconnect_success(output);
		os_atomic_set_bool(&output->reconnecting, false);
	} else {
		signal_start(output);
	}

	return true;
}

static void reset_raw_output(obs_output_t *output)
{
	clear_raw_audio_buffers(output);

	if (output->audio) {
		const struct audio_output_info *aoi = audio_output_get_info(output->audio);
		struct audio_convert_info conv = output->audio_conversion;
		struct audio_convert_info info = {
			aoi->samples_per_sec,
			aoi->format,
			aoi->speakers,
		};

		if (output->audio_conversion_set) {
			if (conv.samples_per_sec)
				info.samples_per_sec = conv.samples_per_sec;
			if (conv.format != AUDIO_FORMAT_UNKNOWN)
				info.format = conv.format;
			if (conv.speakers != SPEAKERS_UNKNOWN)
				info.speakers = conv.speakers;
		}

		output->sample_rate = info.samples_per_sec;
		output->planes = get_audio_planes(info.format, info.speakers);
		output->total_audio_frames = 0;
		output->audio_size = get_audio_size(info.format, info.speakers, 1);
	}

	output->audio_start_ts = 0;
	output->video_start_ts = 0;

	pause_reset(&output->pause);
}

static void calculate_batch_size(struct obs_output *output)
{
	struct obs_video_info ovi;
	obs_get_video_info(&ovi);
	DARRAY(uint64_t) intervals;
	da_init(intervals);

	uint64_t largest_interval = 0;

	/* Step 1: Calculate the largest interval between packets of any encoder. */
	for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
		if (!output->video_encoders[i])
			continue;

		uint32_t den = ovi.fps_den * obs_encoder_get_frame_rate_divisor(output->video_encoders[i]);
		uint64_t encoder_interval = util_mul_div64(1000000000ULL, den, ovi.fps_num);
		da_push_back(intervals, &encoder_interval);

		largest_interval = encoder_interval > largest_interval ? encoder_interval : largest_interval;
	}

	for (size_t i = 0; i < MAX_OUTPUT_AUDIO_ENCODERS; i++) {
		if (!output->audio_encoders[i])
			continue;

		uint32_t sample_rate = obs_encoder_get_sample_rate(output->audio_encoders[i]);
		size_t frame_size = obs_encoder_get_frame_size(output->audio_encoders[i]);
		uint64_t encoder_interval = util_mul_div64(1000000000ULL, frame_size, sample_rate);
		da_push_back(intervals, &encoder_interval);

		largest_interval = encoder_interval > largest_interval ? encoder_interval : largest_interval;
	}

	/* Step 2: Calculate how many packets would fit into double that interval given each encoder's packet rate.
	 * The doubling is done to provide some amount of wiggle room as the largest interval may not be evenly
	 * divisible by all smaller ones. For example, 33.3... ms video (30 FPS) and 21.3... ms audio (48 kHz AAC). */
	for (size_t i = 0; i < intervals.num; i++) {
		uint64_t num = (largest_interval * 2) / intervals.array[i];
		output->interleaver_max_batch_size += num;
	}

	blog(LOG_DEBUG, "Maximum interleaver batch size for '%s' calculated to be %zu packets",
	     obs_output_get_name(output), output->interleaver_max_batch_size);

	da_free(intervals);
}

bool obs_output_begin_data_capture(obs_output_t *output, uint32_t flags)
{
	UNUSED_PARAMETER(flags);

	if (!obs_output_valid(output, "obs_output_begin_data_capture"))
		return false;

	if (delay_active(output))
		return begin_delayed_capture(output);
	if (active(output))
		return false;

	output->total_frames = 0;

	if (!flag_encoded(output))
		reset_raw_output(output);

	if (!can_begin_data_capture(output))
		return false;

	if (flag_video(output) && flag_audio(output))
		pair_encoders(output);

	os_atomic_set_bool(&output->data_active, true);
	hook_data_capture(output);

	calculate_batch_size(output);

	if (flag_service(output))
		obs_service_activate(output->service);

	do_output_signal(output, "activate");
	os_atomic_set_bool(&output->active, true);

	if (reconnecting(output)) {
		signal_reconnect_success(output);
		os_atomic_set_bool(&output->reconnecting, false);

	} else if (delay_active(output)) {
		do_output_signal(output, "starting");

	} else {
		signal_start(output);
	}

	return true;
}

static inline void stop_audio_encoders(obs_output_t *output, encoded_callback_t encoded_callback)
{
	for (size_t i = 0; i < MAX_OUTPUT_AUDIO_ENCODERS; i++) {
		obs_encoder_t *audio = output->audio_encoders[i];
		if (audio)
			obs_encoder_stop(audio, encoded_callback, output);
	}
}

static inline void stop_video_encoders(obs_output_t *output, encoded_callback_t encoded_callback)
{
	for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
		obs_encoder_t *video = output->video_encoders[i];
		if (video)
			obs_encoder_stop(video, encoded_callback, output);
	}
}

static inline void stop_raw_audio(obs_output_t *output)
{
	if (output->info.raw_audio2) {
		for (int idx = 0; idx < MAX_AUDIO_MIXES; idx++) {
			if ((output->mixer_mask & ((size_t)1 << idx)) != 0) {
				audio_output_disconnect(output->audio, idx, default_raw_audio_callback, output);
			}
		}
	} else {
		audio_output_disconnect(output->audio, get_first_mixer(output), default_raw_audio_callback, output);
	}
}

static void *end_data_capture_thread(void *data)
{
	encoded_callback_t encoded_callback;
	obs_output_t *output = data;
	bool has_video = flag_video(output);
	bool has_audio = flag_audio(output);

	if (flag_encoded(output)) {
		if (output->active_delay_ns)
			encoded_callback = process_delay;
		else
			encoded_callback = (has_video && has_audio) ? interleave_packets : default_encoded_callback;

		if (has_video)
			stop_video_encoders(output, encoded_callback);
		if (has_audio)
			stop_audio_encoders(output, encoded_callback);
	} else {
		if (has_video)
			stop_raw_video(output->video, default_raw_video_callback, output);
		if (has_audio)
			stop_raw_audio(output);
	}

	if (flag_service(output))
		obs_service_deactivate(output->service, false);

	if (output->active_delay_ns)
		obs_output_cleanup_delay(output);

	do_output_signal(output, "deactivate");
	os_atomic_set_bool(&output->active, false);
	os_event_signal(output->stopping_event);
	os_atomic_set_bool(&output->end_data_capture_thread_active, false);

	return NULL;
}

static void obs_output_end_data_capture_internal(obs_output_t *output, bool signal)
{
	int ret;

	if (!obs_output_valid(output, "obs_output_end_data_capture"))
		return;

	if (!active(output) || !data_active(output)) {
		if (signal) {
			signal_stop(output);
			output->stop_code = OBS_OUTPUT_SUCCESS;
			os_event_signal(output->stopping_event);
		}
		return;
	}

	if (delay_active(output)) {
		os_atomic_set_bool(&output->delay_capturing, false);

		if (!os_atomic_load_long(&output->delay_restart_refs)) {
			os_atomic_set_bool(&output->delay_active, false);
		} else {
			os_event_signal(output->stopping_event);
			return;
		}
	}

	os_atomic_set_bool(&output->data_active, false);

	if (flag_video(output))
		log_frame_info(output);

	if (data_capture_ending(output))
		pthread_join(output->end_data_capture_thread, NULL);

	os_atomic_set_bool(&output->end_data_capture_thread_active, true);
	ret = pthread_create(&output->end_data_capture_thread, NULL, end_data_capture_thread, output);
	if (ret != 0) {
		blog(LOG_WARNING,
		     "Failed to create end_data_capture_thread "
		     "for output '%s'!",
		     output->context.name);
		end_data_capture_thread(output);
	}

	if (signal) {
		signal_stop(output);
		output->stop_code = OBS_OUTPUT_SUCCESS;
	}
}

void obs_output_end_data_capture(obs_output_t *output)
{
	obs_output_end_data_capture_internal(output, true);
}

static void *reconnect_thread(void *param)
{
	struct obs_output *output = param;

	output->reconnect_thread_active = true;

	if (os_event_timedwait(output->reconnect_stop_event, output->reconnect_retry_cur_msec) == ETIMEDOUT)
		obs_output_actual_start(output);

	if (os_event_try(output->reconnect_stop_event) == EAGAIN)
		pthread_detach(output->reconnect_thread);
	else
		os_atomic_set_bool(&output->reconnecting, false);

	output->reconnect_thread_active = false;
	return NULL;
}

static void output_reconnect(struct obs_output *output)
{
	int ret;

	if (reconnecting(output) && os_event_try(output->reconnect_stop_event) != EAGAIN) {
		os_atomic_set_bool(&output->reconnecting, false);
		return;
	}

	if (!reconnecting(output)) {
		output->reconnect_retry_cur_msec = output->reconnect_retry_sec * 1000;
		output->reconnect_retries = 0;
	}

	if (output->reconnect_retries >= output->reconnect_retry_max) {
		output->stop_code = OBS_OUTPUT_DISCONNECTED;
		os_atomic_set_bool(&output->reconnecting, false);
		if (delay_active(output))
			os_atomic_set_bool(&output->delay_active, false);
		obs_output_end_data_capture(output);
		return;
	}

	if (!reconnecting(output)) {
		os_atomic_set_bool(&output->reconnecting, true);
		os_event_reset(output->reconnect_stop_event);
	}

	if (output->reconnect_retries) {
		output->reconnect_retry_cur_msec =
			(uint32_t)(output->reconnect_retry_cur_msec * output->reconnect_retry_exp);
		if (output->reconnect_retry_cur_msec > RECONNECT_RETRY_MAX_MSEC) {
			output->reconnect_retry_cur_msec = RECONNECT_RETRY_MAX_MSEC;
		}
	}

	output->reconnect_retries++;

	output->stop_code = OBS_OUTPUT_DISCONNECTED;
	ret = pthread_create(&output->reconnect_thread, NULL, &reconnect_thread, output);
	if (ret < 0) {
		blog(LOG_WARNING, "Failed to create reconnect thread");
		os_atomic_set_bool(&output->reconnecting, false);
	} else {
		blog(LOG_INFO, "Output '%s': Reconnecting in %.02f seconds..", output->context.name,
		     (float)(output->reconnect_retry_cur_msec / 1000.0));

		signal_reconnect(output);
	}
}

static inline bool check_reconnect_cb(obs_output_t *output, int code)
{
	if (!output->reconnect_callback.reconnect_cb)
		return true;

	return output->reconnect_callback.reconnect_cb(output->reconnect_callback.param, output, code);
}

static inline bool can_reconnect(obs_output_t *output, int code)
{
	bool reconnect_active = output->reconnect_retry_max != 0;

	if (reconnect_active && !check_reconnect_cb(output, code))
		return false;

	return (reconnecting(output) && code != OBS_OUTPUT_SUCCESS) ||
	       (reconnect_active && code == OBS_OUTPUT_DISCONNECTED);
}

void obs_output_signal_stop(obs_output_t *output, int code)
{
	if (!obs_output_valid(output, "obs_output_signal_stop"))
		return;

	output->stop_code = code;

	if (can_reconnect(output, code)) {
		if (delay_active(output))
			os_atomic_inc_long(&output->delay_restart_refs);
		obs_output_end_data_capture_internal(output, false);
		output_reconnect(output);
	} else {
		if (delay_active(output))
			os_atomic_set_bool(&output->delay_active, false);
		if (reconnecting(output))
			os_atomic_set_bool(&output->reconnecting, false);
		obs_output_end_data_capture(output);
	}
}

void obs_output_release(obs_output_t *output)
{
	if (!output)
		return;

	obs_weak_output_t *control = get_weak(output);
	if (obs_ref_release(&control->ref)) {
		// The order of operations is important here since
		// get_context_by_name in obs.c relies on weak refs
		// being alive while the context is listed
		obs_output_destroy(output);
		obs_weak_output_release(control);
	}
}

void obs_weak_output_addref(obs_weak_output_t *weak)
{
	if (!weak)
		return;

	obs_weak_ref_addref(&weak->ref);
}

void obs_weak_output_release(obs_weak_output_t *weak)
{
	if (!weak)
		return;

	if (obs_weak_ref_release(&weak->ref))
		bfree(weak);
}

obs_output_t *obs_output_get_ref(obs_output_t *output)
{
	if (!output)
		return NULL;

	return obs_weak_output_get_output(get_weak(output));
}

obs_weak_output_t *obs_output_get_weak_output(obs_output_t *output)
{
	if (!output)
		return NULL;

	obs_weak_output_t *weak = get_weak(output);
	obs_weak_output_addref(weak);
	return weak;
}

obs_output_t *obs_weak_output_get_output(obs_weak_output_t *weak)
{
	if (!weak)
		return NULL;

	if (obs_weak_ref_get_ref(&weak->ref))
		return weak->output;

	return NULL;
}

bool obs_weak_output_references_output(obs_weak_output_t *weak, obs_output_t *output)
{
	return weak && output && weak->output == output;
}

void *obs_output_get_type_data(obs_output_t *output)
{
	return obs_output_valid(output, "obs_output_get_type_data") ? output->info.type_data : NULL;
}

const char *obs_output_get_id(const obs_output_t *output)
{
	return obs_output_valid(output, "obs_output_get_id") ? output->info.id : NULL;
}

void obs_output_caption(obs_output_t *output, const struct obs_source_cea_708 *captions)
{
	for (int i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
		struct caption_track_data *ctrack = output->caption_tracks[i];
		if (!ctrack) {
			continue;
		}
		pthread_mutex_lock(&ctrack->caption_mutex);
		for (size_t i = 0; i < captions->packets; i++) {
			deque_push_back(&ctrack->caption_data, captions->data + (i * 3), 3 * sizeof(uint8_t));
		}
		pthread_mutex_unlock(&ctrack->caption_mutex);
	}
}

static struct caption_text *caption_text_new(const char *text, size_t bytes, struct caption_text *tail,
					     struct caption_text **head, double display_duration)
{
	struct caption_text *next = bzalloc(sizeof(struct caption_text));
	snprintf(&next->text[0], CAPTION_LINE_BYTES + 1, "%.*s", (int)bytes, text);
	next->display_duration = display_duration;

	if (!*head) {
		*head = next;
	} else {
		tail->next = next;
	}

	return next;
}

void obs_output_output_caption_text1(obs_output_t *output, const char *text)
{
	if (!obs_output_valid(output, "obs_output_output_caption_text1"))
		return;
	obs_output_output_caption_text2(output, text, 2.0f);
}

void obs_output_output_caption_text2(obs_output_t *output, const char *text, double display_duration)
{
	if (!obs_output_valid(output, "obs_output_output_caption_text2"))
		return;
	if (!active(output))
		return;

	// split text into 32 character strings
	int size = (int)strlen(text);
	blog(LOG_DEBUG, "Caption text: %s", text);

	for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
		struct caption_track_data *ctrack = output->caption_tracks[i];
		if (!ctrack) {
			continue;
		}
		pthread_mutex_lock(&ctrack->caption_mutex);

		ctrack->caption_tail =
			caption_text_new(text, size, ctrack->caption_tail, &ctrack->caption_head, display_duration);

		pthread_mutex_unlock(&ctrack->caption_mutex);
	}
}

float obs_output_get_congestion(obs_output_t *output)
{
	if (!obs_output_valid(output, "obs_output_get_congestion"))
		return 0;

	if (output->info.get_congestion) {
		float val = output->info.get_congestion(output->context.data);
		if (val < 0.0f)
			val = 0.0f;
		else if (val > 1.0f)
			val = 1.0f;
		return val;
	}
	return 0;
}

int obs_output_get_connect_time_ms(obs_output_t *output)
{
	if (!obs_output_valid(output, "obs_output_get_connect_time_ms"))
		return -1;

	if (output->info.get_connect_time_ms)
		return output->info.get_connect_time_ms(output->context.data);
	return -1;
}

const char *obs_output_get_last_error(obs_output_t *output)
{
	if (!obs_output_valid(output, "obs_output_get_last_error"))
		return NULL;

	if (output->last_error_message) {
		return output->last_error_message;
	} else {
		for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
			obs_encoder_t *vencoder = output->video_encoders[i];
			if (vencoder && vencoder->last_error_message) {
				return vencoder->last_error_message;
			}
		}

		for (size_t i = 0; i < MAX_OUTPUT_AUDIO_ENCODERS; i++) {
			obs_encoder_t *aencoder = output->audio_encoders[i];
			if (aencoder && aencoder->last_error_message) {
				return aencoder->last_error_message;
			}
		}
	}

	return NULL;
}

void obs_output_set_last_error(obs_output_t *output, const char *message)
{
	if (!obs_output_valid(output, "obs_output_set_last_error"))
		return;

	if (output->last_error_message)
		bfree(output->last_error_message);

	if (message)
		output->last_error_message = bstrdup(message);
	else
		output->last_error_message = NULL;
}

bool obs_output_reconnecting(const obs_output_t *output)
{
	if (!obs_output_valid(output, "obs_output_reconnecting"))
		return false;

	return reconnecting(output);
}

const char *obs_output_get_supported_video_codecs(const obs_output_t *output)
{
	return obs_output_valid(output, __FUNCTION__) ? output->info.encoded_video_codecs : NULL;
}

const char *obs_output_get_supported_audio_codecs(const obs_output_t *output)
{
	return obs_output_valid(output, __FUNCTION__) ? output->info.encoded_audio_codecs : NULL;
}

const char *obs_output_get_protocols(const obs_output_t *output)
{
	if (!obs_output_valid(output, "obs_output_get_protocols"))
		return NULL;

	return flag_service(output) ? output->info.protocols : NULL;
}

void obs_enum_output_types_with_protocol(const char *protocol, void *data, bool (*enum_cb)(void *data, const char *id))
{
	if (!obs_is_output_protocol_registered(protocol))
		return;

	size_t protocol_len = strlen(protocol);
	for (size_t i = 0; i < obs->output_types.num; i++) {
		if (!(obs->output_types.array[i].flags & OBS_OUTPUT_SERVICE))
			continue;

		const char *substr = obs->output_types.array[i].protocols;
		while (substr && substr[0] != '\0') {
			const char *next = strchr(substr, ';');
			size_t len = next ? (size_t)(next - substr) : strlen(substr);
			if (protocol_len == len && strncmp(substr, protocol, len) == 0) {
				if (!enum_cb(data, obs->output_types.array[i].id))
					return;
			}
			substr = next ? next + 1 : NULL;
		}
	}
}

const char *obs_get_output_supported_video_codecs(const char *id)
{
	const struct obs_output_info *info = find_output(id);
	return info ? info->encoded_video_codecs : NULL;
}

const char *obs_get_output_supported_audio_codecs(const char *id)
{
	const struct obs_output_info *info = find_output(id);
	return info ? info->encoded_audio_codecs : NULL;
}

void obs_output_add_packet_callback(obs_output_t *output,
				    void (*packet_cb)(obs_output_t *output, struct encoder_packet *pkt,
						      struct encoder_packet_time *pkt_time, void *param),
				    void *param)
{
	struct packet_callback data = {packet_cb, param};

	pthread_mutex_lock(&output->pkt_callbacks_mutex);
	da_insert(output->pkt_callbacks, 0, &data);
	pthread_mutex_unlock(&output->pkt_callbacks_mutex);
}

void obs_output_remove_packet_callback(obs_output_t *output,
				       void (*packet_cb)(obs_output_t *output, struct encoder_packet *pkt,
							 struct encoder_packet_time *pkt_time, void *param),
				       void *param)
{
	struct packet_callback data = {packet_cb, param};

	pthread_mutex_lock(&output->pkt_callbacks_mutex);
	da_erase_item(output->pkt_callbacks, &data);
	pthread_mutex_unlock(&output->pkt_callbacks_mutex);
}

void obs_output_set_reconnect_callback(obs_output_t *output,
				       bool (*reconnect_cb)(void *data, obs_output_t *output, int code), void *param)
{
	if (!reconnect_cb) {
		output->reconnect_callback.reconnect_cb = NULL;
		output->reconnect_callback.param = NULL;
	} else {
		output->reconnect_callback.reconnect_cb = reconnect_cb;
		output->reconnect_callback.param = param;
	}
}
