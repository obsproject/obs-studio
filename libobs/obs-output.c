/******************************************************************************
    Copyright (C) 2013-2014 by Hugh Bailey <obs.jim@gmail.com>

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
#include "obs.h"
#include "obs-internal.h"

#if BUILD_CAPTIONS
#include <caption/caption.h>
#include <caption/avc.h>
#endif

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

const struct obs_output_info *find_output(const char *id)
{
	size_t i;
	for (i = 0; i < obs->output_types.num; i++)
		if (strcmp(obs->output_types.array[i].id, id) == 0)
			return obs->output_types.array+i;

	return NULL;
}

const char *obs_output_get_display_name(const char *id)
{
	const struct obs_output_info *info = find_output(id);
	return (info != NULL) ? info->get_name(info->type_data) : NULL;
}

static const char *output_signals[] = {
	"void start(ptr output)",
	"void stop(ptr output, int code)",
	"void starting(ptr output)",
	"void stopping(ptr output)",
	"void activate(ptr output)",
	"void deactivate(ptr output)",
	"void reconnect(ptr output)",
	"void reconnect_success(ptr output)",
	NULL
};

static bool init_output_handlers(struct obs_output *output, const char *name,
		obs_data_t *settings, obs_data_t *hotkey_data)
{
	if (!obs_context_data_init(&output->context, OBS_OBJ_TYPE_OUTPUT,
				settings, name, hotkey_data, false))
		return false;

	signal_handler_add_array(output->context.signals, output_signals);
	return true;
}

obs_output_t *obs_output_create(const char *id, const char *name,
		obs_data_t *settings, obs_data_t *hotkey_data)
{
	const struct obs_output_info *info = find_output(id);
	struct obs_output *output;
	int ret;

	output = bzalloc(sizeof(struct obs_output));
	pthread_mutex_init_value(&output->interleaved_mutex);
	pthread_mutex_init_value(&output->delay_mutex);
	pthread_mutex_init_value(&output->caption_mutex);

	if (pthread_mutex_init(&output->interleaved_mutex, NULL) != 0)
		goto fail;
	if (pthread_mutex_init(&output->delay_mutex, NULL) != 0)
		goto fail;
	if (pthread_mutex_init(&output->caption_mutex, NULL) != 0)
		goto fail;
	if (os_event_init(&output->stopping_event, OS_EVENT_TYPE_MANUAL) != 0)
		goto fail;
	if (!init_output_handlers(output, name, settings, hotkey_data))
		goto fail;

	os_event_signal(output->stopping_event);

	if (!info) {
		blog(LOG_ERROR, "Output ID '%s' not found", id);

		output->info.id      = bstrdup(id);
		output->owns_info_id = true;
	} else {
		output->info = *info;
	}
	output->video    = obs_get_video();
	output->audio    = obs_get_audio();
	if (output->info.get_defaults)
		output->info.get_defaults(output->context.settings);

	ret = os_event_init(&output->reconnect_stop_event,
			OS_EVENT_TYPE_MANUAL);
	if (ret < 0)
		goto fail;

	output->reconnect_retry_sec = 2;
	output->reconnect_retry_max = 20;
	output->valid               = true;

	output->control = bzalloc(sizeof(obs_weak_output_t));
	output->control->output = output;

	obs_context_data_insert(&output->context,
			&obs->data.outputs_mutex,
			&obs->data.first_output);

	if (info)
		output->context.data = info->create(output->context.settings,
				output);
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
		obs_encoder_packet_release(output->interleaved_packets.array+i);
	da_free(output->interleaved_packets);
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

		if (output->video_encoder) {
			obs_encoder_remove_output(output->video_encoder,
					output);
		}

		for (size_t i = 0; i < MAX_AUDIO_MIXES; i++) {
			if (output->audio_encoders[i]) {
				obs_encoder_remove_output(
						output->audio_encoders[i],
						output);
			}
		}

		os_event_destroy(output->stopping_event);
		pthread_mutex_destroy(&output->caption_mutex);
		pthread_mutex_destroy(&output->interleaved_mutex);
		pthread_mutex_destroy(&output->delay_mutex);
		os_event_destroy(output->reconnect_stop_event);
		obs_context_data_free(&output->context);
		circlebuf_free(&output->delay_data);
		if (output->owns_info_id)
			bfree((void*)output->info.id);
		if (output->last_error_message)
			bfree(output->last_error_message);
		bfree(output);
	}
}

const char *obs_output_get_name(const obs_output_t *output)
{
	return obs_output_valid(output, "obs_output_get_name") ?
		output->context.name : NULL;
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

	if (success && output->video) {
		output->starting_frame_count =
			video_output_get_total_frames(output->video);
		output->starting_drawn_count = obs->video.total_frames;
		output->starting_lagged_count = obs->video.lagged_frames;
	}

	if (os_atomic_load_long(&output->delay_restart_refs))
		os_atomic_dec_long(&output->delay_restart_refs);

	output->caption_timestamp = 0;
	return success;
}

bool obs_output_start(obs_output_t *output)
{
	bool encoded;
	if (!obs_output_valid(output, "obs_output_start"))
		return false;
	if (!output->context.data)
		return false;

	encoded = (output->info.flags & OBS_OUTPUT_ENCODED) != 0;

	if (encoded && output->delay_sec) {
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

	uint32_t drawn  = video->total_frames - output->starting_drawn_count;
	uint32_t lagged = video->lagged_frames - output->starting_lagged_count;

	int dropped = obs_output_get_frames_dropped(output);
	int total = output->total_frames;

	double percentage_lagged = 0.0f;
	double percentage_dropped = 0.0f;

	if (drawn)
		percentage_lagged = (double)lagged  / (double)drawn * 100.0;
	if (dropped)
		percentage_dropped = (double)dropped / (double)total * 100.0;

	blog(LOG_INFO, "Output '%s': stopping", output->context.name);
	if (!dropped || !total)
		blog(LOG_INFO, "Output '%s': Total frames output: %d",
				output->context.name, total);
	else
		blog(LOG_INFO, "Output '%s': Total frames output: %d"
				" (%d attempted)",
				output->context.name, total - dropped, total);

	if (!lagged || !drawn)
		blog(LOG_INFO, "Output '%s': Total drawn frames: %"PRIu32,
				output->context.name, drawn);
	else
		blog(LOG_INFO, "Output '%s': Total drawn frames: %"PRIu32
				" (%"PRIu32" attempted)",
				output->context.name, drawn - lagged, drawn);

	if (drawn && lagged)
		blog(LOG_INFO, "Output '%s': Number of lagged frames due "
				"to rendering lag/stalls: %"PRIu32" (%0.1f%%)",
				output->context.name,
				lagged, percentage_lagged);
	if (total && dropped)
		blog(LOG_INFO, "Output '%s': Number of dropped frames due "
				"to insufficient bandwidth/connection stalls: "
				"%d (%0.1f%%)",
				output->context.name,
				dropped, percentage_dropped);
}

static inline void signal_stop(struct obs_output *output);

void obs_output_actual_stop(obs_output_t *output, bool force, uint64_t ts)
{
	bool call_stop = true;
	bool was_reconnecting = false;

	if (stopping(output) && !force)
		return;
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

	while (output->caption_head) {
		output->caption_tail = output->caption_head->next;
		bfree(output->caption_head);
		output->caption_head = output->caption_tail;
	}
}

void obs_output_stop(obs_output_t *output)
{
	bool encoded;
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

	encoded = (output->info.flags & OBS_OUTPUT_ENCODED) != 0;

	if (encoded && output->active_delay_ns) {
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
	return (output != NULL) ?
		(active(output) || reconnecting(output)) : false;
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
		obs_data_t       *defaults = get_defaults(info);
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
		output->info.update(output->context.data,
				output->context.settings);
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
	return obs_output_valid(output, "obs_output_can_pause") ?
		(output->info.pause != NULL) : false;
}

void obs_output_pause(obs_output_t *output)
{
	if (!obs_output_valid(output, "obs_output_pause"))
		return;

	if (output->info.pause)
		output->info.pause(output->context.data);
}

signal_handler_t *obs_output_get_signal_handler(const obs_output_t *output)
{
	return obs_output_valid(output, "obs_output_get_signal_handler") ?
		output->context.signals : NULL;
}

proc_handler_t *obs_output_get_proc_handler(const obs_output_t *output)
{
	return obs_output_valid(output, "obs_output_get_proc_handler") ?
		output->context.procs : NULL;
}

void obs_output_set_media(obs_output_t *output, video_t *video, audio_t *audio)
{
	if (!obs_output_valid(output, "obs_output_set_media"))
		return;

	output->video = video;
	output->audio = audio;
}

video_t *obs_output_video(const obs_output_t *output)
{
	return obs_output_valid(output, "obs_output_video") ?
		output->video : NULL;
}

audio_t *obs_output_audio(const obs_output_t *output)
{
	return obs_output_valid(output, "obs_output_audio") ?
		output->audio : NULL;
}

void obs_output_set_mixer(obs_output_t *output, size_t mixer_idx)
{
	if (!obs_output_valid(output, "obs_output_set_mixer"))
		return;

	if (!active(output))
		output->mixer_idx = mixer_idx;
}

size_t obs_output_get_mixer(const obs_output_t *output)
{
	return obs_output_valid(output, "obs_output_get_mixer") ?
		output->mixer_idx : 0;
}

void obs_output_remove_encoder(struct obs_output *output,
		struct obs_encoder *encoder)
{
	if (!obs_output_valid(output, "obs_output_remove_encoder"))
		return;

	if (output->video_encoder == encoder) {
		output->video_encoder = NULL;
	} else {
		for (size_t i = 0; i < MAX_AUDIO_MIXES; i++) {
			if (output->audio_encoders[i] == encoder)
				output->audio_encoders[i] = NULL;
		}
	}
}

void obs_output_set_video_encoder(obs_output_t *output, obs_encoder_t *encoder)
{
	if (!obs_output_valid(output, "obs_output_set_video_encoder"))
		return;
	if (encoder && encoder->info.type != OBS_ENCODER_VIDEO) {
		blog(LOG_WARNING, "obs_output_set_video_encoder: "
				"encoder passed is not a video encoder");
		return;
	}

	if (output->video_encoder == encoder) return;

	obs_encoder_remove_output(output->video_encoder, output);
	obs_encoder_add_output(encoder, output);
	output->video_encoder = encoder;

	/* set the preferred resolution on the encoder */
	if (output->scaled_width && output->scaled_height)
		obs_encoder_set_scaled_size(output->video_encoder,
				output->scaled_width, output->scaled_height);
}

void obs_output_set_audio_encoder(obs_output_t *output, obs_encoder_t *encoder,
		size_t idx)
{
	if (!obs_output_valid(output, "obs_output_set_audio_encoder"))
		return;
	if (encoder && encoder->info.type != OBS_ENCODER_AUDIO) {
		blog(LOG_WARNING, "obs_output_set_audio_encoder: "
				"encoder passed is not an audio encoder");
		return;
	}

	if ((output->info.flags & OBS_OUTPUT_MULTI_TRACK) != 0) {
		if (idx >= MAX_AUDIO_MIXES) {
			return;
		}
	} else {
		if (idx > 0) {
			return;
		}
	}

	if (output->audio_encoders[idx] == encoder) return;

	obs_encoder_remove_output(output->audio_encoders[idx], output);
	obs_encoder_add_output(encoder, output);
	output->audio_encoders[idx] = encoder;
}

obs_encoder_t *obs_output_get_video_encoder(const obs_output_t *output)
{
	return obs_output_valid(output, "obs_output_get_video_encoder") ?
		output->video_encoder : NULL;
}

obs_encoder_t *obs_output_get_audio_encoder(const obs_output_t *output,
		size_t idx)
{
	if (!obs_output_valid(output, "obs_output_get_audio_encoder"))
		return NULL;

	if ((output->info.flags & OBS_OUTPUT_MULTI_TRACK) != 0) {
		if (idx >= MAX_AUDIO_MIXES) {
			return NULL;
		}
	} else {
		if (idx > 0) {
			return NULL;
		}
	}

	return output->audio_encoders[idx];
}

void obs_output_set_service(obs_output_t *output, obs_service_t *service)
{
	if (!obs_output_valid(output, "obs_output_set_service"))
		return;
	if (active(output) || !service || service->active)
		return;

	if (service->output)
		service->output->service = NULL;

	output->service = service;
	service->output = output;
}

obs_service_t *obs_output_get_service(const obs_output_t *output)
{
	return obs_output_valid(output, "obs_output_get_service") ?
		output->service : NULL;
}

void obs_output_set_reconnect_settings(obs_output_t *output,
		int retry_count, int retry_sec)
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
	return obs_output_valid(output, "obs_output_get_total_frames") ?
		output->total_frames : 0;
}

void obs_output_set_preferred_size(obs_output_t *output, uint32_t width,
		uint32_t height)
{
	if (!obs_output_valid(output, "obs_output_set_preferred_size"))
		return;
	if ((output->info.flags & OBS_OUTPUT_VIDEO) == 0)
		return;

	if (active(output)) {
		blog(LOG_WARNING, "output '%s': Cannot set the preferred "
		                  "resolution while the output is active",
		                  obs_output_get_name(output));
		return;
	}

	output->scaled_width  = width;
	output->scaled_height = height;

	if (output->info.flags & OBS_OUTPUT_ENCODED) {
		if (output->video_encoder)
			obs_encoder_set_scaled_size(output->video_encoder,
					width, height);
	}
}

uint32_t obs_output_get_width(const obs_output_t *output)
{
	if (!obs_output_valid(output, "obs_output_get_width"))
		return 0;
	if ((output->info.flags & OBS_OUTPUT_VIDEO) == 0)
		return 0;

	if (output->info.flags & OBS_OUTPUT_ENCODED)
		return obs_encoder_get_width(output->video_encoder);
	else
		return output->scaled_width != 0 ?
			output->scaled_width :
			video_output_get_width(output->video);
}

uint32_t obs_output_get_height(const obs_output_t *output)
{
	if (!obs_output_valid(output, "obs_output_get_height"))
		return 0;
	if ((output->info.flags & OBS_OUTPUT_VIDEO) == 0)
		return 0;

	if (output->info.flags & OBS_OUTPUT_ENCODED)
		return obs_encoder_get_height(output->video_encoder);
	else
		return output->scaled_height != 0 ?
			output->scaled_height :
			video_output_get_height(output->video);
}

void obs_output_set_video_conversion(obs_output_t *output,
		const struct video_scale_info *conversion)
{
	if (!obs_output_valid(output, "obs_output_set_video_conversion"))
		return;
	if (!obs_ptr_valid(conversion, "obs_output_set_video_conversion"))
		return;

	output->video_conversion = *conversion;
	output->video_conversion_set = true;
}

void obs_output_set_audio_conversion(obs_output_t *output,
		const struct audio_convert_info *conversion)
{
	if (!obs_output_valid(output, "obs_output_set_audio_conversion"))
		return;
	if (!obs_ptr_valid(conversion, "obs_output_set_audio_conversion"))
		return;

	output->audio_conversion = *conversion;
	output->audio_conversion_set = true;
}

static inline bool service_supports_multitrack(const struct obs_output *output)
{
	const struct obs_service *service = output->service;

	if (!service || !service->info.supports_multitrack) {
		return false;
	}

	return service->info.supports_multitrack(service->context.data);
}

static inline size_t num_audio_mixes(const struct obs_output *output)
{
	size_t mix_count = 1;

	if ((output->info.flags & OBS_OUTPUT_SERVICE) != 0) {
		if (!service_supports_multitrack(output)) {
			return 1;
		}
	}

	if ((output->info.flags & OBS_OUTPUT_MULTI_TRACK) != 0) {
		mix_count = 0;

		for (size_t i = 0; i < MAX_AUDIO_MIXES; i++) {
			if (!output->audio_encoders[i])
				break;

			mix_count++;
		}
	}

	return mix_count;
}

static inline bool audio_valid(const struct obs_output *output, bool encoded)
{
	if (encoded) {
		size_t mix_count = num_audio_mixes(output);
		if (!mix_count)
			return false;

		for (size_t i = 0; i < mix_count; i++) {
			if (!output->audio_encoders[i]) {
				return false;
			}
		}
	} else {
		if (!output->audio)
			return false;
	}

	return true;
}

static bool can_begin_data_capture(const struct obs_output *output,
		bool encoded, bool has_video, bool has_audio, bool has_service)
{
	if (has_video) {
		if (encoded) {
			if (!output->video_encoder)
				return false;
		} else {
			if (!output->video)
				return false;
		}
	}

	if (has_audio) {
		if (!audio_valid(output, encoded)) {
			return false;
		}
	}

	if (has_service && !output->service)
		return false;

	return true;
}

static inline bool has_scaling(const struct obs_output *output)
{
	uint32_t video_width  = video_output_get_width(output->video);
	uint32_t video_height = video_output_get_height(output->video);

	return output->scaled_width && output->scaled_height &&
		(video_width  != output->scaled_width ||
		 video_height != output->scaled_height);
}

static inline struct video_scale_info *get_video_conversion(
		struct obs_output *output)
{
	if (output->video_conversion_set) {
		if (!output->video_conversion.width)
			output->video_conversion.width =
				obs_output_get_width(output);

		if (!output->video_conversion.height)
			output->video_conversion.height =
				obs_output_get_height(output);

		return &output->video_conversion;

	} else if (has_scaling(output)) {
		const struct video_output_info *info =
			video_output_get_info(output->video);

		output->video_conversion.format     = info->format;
		output->video_conversion.colorspace = VIDEO_CS_DEFAULT;
		output->video_conversion.range      = VIDEO_RANGE_DEFAULT;
		output->video_conversion.width      = output->scaled_width;
		output->video_conversion.height     = output->scaled_height;
		return &output->video_conversion;
	}

	return NULL;
}

static inline struct audio_convert_info *get_audio_conversion(
		struct obs_output *output)
{
	return output->audio_conversion_set ? &output->audio_conversion : NULL;
}

static size_t get_track_index(const struct obs_output *output,
		struct encoder_packet *pkt)
{
	for (size_t i = 0; i < MAX_AUDIO_MIXES; i++) {
		struct obs_encoder *encoder = output->audio_encoders[i];

		if (pkt->encoder == encoder)
			return i;
	}

	assert(false);
	return 0;
}

static inline void check_received(struct obs_output *output,
		struct encoder_packet *out)
{
	if (out->type == OBS_ENCODER_VIDEO) {
		if (!output->received_video)
			output->received_video = true;
	} else {
		if (!output->received_audio)
			output->received_audio = true;
	}
}

static inline void apply_interleaved_packet_offset(struct obs_output *output,
		struct encoder_packet *out)
{
	int64_t offset;

	/* audio and video need to start at timestamp 0, and the encoders
	 * may not currently be at 0 when we get data.  so, we store the
	 * current dts as offset and subtract that value from the dts/pts
	 * of the output packet. */
	offset = (out->type == OBS_ENCODER_VIDEO) ?
		output->video_offset : output->audio_offsets[out->track_idx];

	out->dts -= offset;
	out->pts -= offset;

	/* convert the newly adjusted dts to relative dts time to ensure proper
	 * interleaving.  if we're using an audio encoder that's already been
	 * started on another output, then the first audio packet may not be
	 * quite perfectly synced up in terms of system time (and there's
	 * nothing we can really do about that), but it will always at least be
	 * within a 23ish millisecond threshold (at least for AAC) */
	out->dts_usec = packet_dts_usec(out);
}

static inline bool has_higher_opposing_ts(struct obs_output *output,
		struct encoder_packet *packet)
{
	if (packet->type == OBS_ENCODER_VIDEO)
		return output->highest_audio_ts > packet->dts_usec;
	else
		return output->highest_video_ts > packet->dts_usec;
}

#if BUILD_CAPTIONS
static const uint8_t nal_start[4] = {0, 0, 0, 1};

static bool add_caption(struct obs_output *output, struct encoder_packet *out)
{
	struct encoder_packet backup = *out;
	caption_frame_t cf;
	sei_t sei;
	uint8_t *data;
	size_t size;
	long ref = 1;

	DARRAY(uint8_t) out_data;

	if (out->priority > 1)
		return false;

	sei_init(&sei);

	da_init(out_data);
	da_push_back_array(out_data, &ref, sizeof(ref));
	da_push_back_array(out_data, out->data, out->size);

	caption_frame_init(&cf);
	caption_frame_from_text(&cf, &output->caption_head->text[0]);

	sei_from_caption_frame(&sei, &cf);

	data = malloc(sei_render_size(&sei));
	size = sei_render(&sei, data);
	/* TODO SEI should come after AUD/SPS/PPS, but before any VCL */
	da_push_back_array(out_data, nal_start, 4);
	da_push_back_array(out_data, data, size);
	free(data);

	obs_encoder_packet_release(out);

	*out = backup;
	out->data = (uint8_t*)out_data.array + sizeof(ref);
	out->size = out_data.num - sizeof(ref);

	sei_free(&sei);

	struct caption_text *next = output->caption_head->next;
	bfree(output->caption_head);
	output->caption_head = next;
	return true;
}
#endif

static inline void send_interleaved(struct obs_output *output)
{
	struct encoder_packet out = output->interleaved_packets.array[0];

	/* do not send an interleaved packet if there's no packet of the
	 * opposing type of a higher timestamp in the interleave buffer.
	 * this ensures that the timestamps are monotonic */
	if (!has_higher_opposing_ts(output, &out))
		return;

	da_erase(output->interleaved_packets, 0);

	if (out.type == OBS_ENCODER_VIDEO) {
		output->total_frames++;

#if BUILD_CAPTIONS
		pthread_mutex_lock(&output->caption_mutex);

		double frame_timestamp = (out.pts * out.timebase_num) /
			(double)out.timebase_den;

		/* TODO if output->caption_timestamp is more than 5 seconds
		 * old, send empty frame */
		if (output->caption_head &&
		    output->caption_timestamp <= frame_timestamp) {
			blog(LOG_INFO,"Sending caption: %f \"%s\"",
					frame_timestamp,
					&output->caption_head->text[0]);

			if (add_caption(output, &out)) {
				output->caption_timestamp =
					frame_timestamp + 2.0;
			}
		}

		pthread_mutex_unlock(&output->caption_mutex);
#endif
	}

	output->info.encoded_packet(output->context.data, &out);
	obs_encoder_packet_release(&out);
}

static inline void set_higher_ts(struct obs_output *output,
		struct encoder_packet *packet)
{
	if (packet->type == OBS_ENCODER_VIDEO) {
		if (output->highest_video_ts < packet->dts_usec)
			output->highest_video_ts = packet->dts_usec;
	} else {
		if (output->highest_audio_ts < packet->dts_usec)
			output->highest_audio_ts = packet->dts_usec;
	}
}

static inline struct encoder_packet *find_first_packet_type(
		struct obs_output *output, enum obs_encoder_type type,
		size_t audio_idx);
static int find_first_packet_type_idx(struct obs_output *output,
		enum obs_encoder_type type, size_t audio_idx);

/* gets the point where audio and video are closest together */
static size_t get_interleaved_start_idx(struct obs_output *output)
{
	int64_t closest_diff = 0x7FFFFFFFFFFFFFFFLL;
	struct encoder_packet *first_video = find_first_packet_type(output,
			OBS_ENCODER_VIDEO, 0);
	size_t video_idx = DARRAY_INVALID;
	size_t idx = 0;

	for (size_t i = 0; i < output->interleaved_packets.num; i++) {
		struct encoder_packet *packet =
			&output->interleaved_packets.array[i];
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

	return video_idx < idx ? video_idx : idx;
}

static int prune_premature_packets(struct obs_output *output)
{
	size_t audio_mixes = num_audio_mixes(output);
	struct encoder_packet *video;
	int video_idx;
	int max_idx;
	int64_t duration_usec;
	int64_t max_diff = 0;
	int64_t diff = 0;

	video_idx = find_first_packet_type_idx(output, OBS_ENCODER_VIDEO, 0);
	if (video_idx == -1) {
		output->received_video = false;
		return -1;
	}

	max_idx = video_idx;
	video = &output->interleaved_packets.array[video_idx];
	duration_usec = video->timebase_num * 1000000LL / video->timebase_den;

	for (size_t i = 0; i < audio_mixes; i++) {
		struct encoder_packet *audio;
		int audio_idx;

		audio_idx = find_first_packet_type_idx(output,
				OBS_ENCODER_AUDIO, i);
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
	}

	return diff > duration_usec ? max_idx + 1 : 0;
}

static void discard_to_idx(struct obs_output *output, size_t idx)
{
	for (size_t i = 0; i < idx; i++) {
		struct encoder_packet *packet =
			&output->interleaved_packets.array[i];
		obs_encoder_packet_release(packet);
	}

	da_erase_range(output->interleaved_packets, 0, idx);
}

#define DEBUG_STARTING_PACKETS 0

static bool prune_interleaved_packets(struct obs_output *output)
{
	size_t start_idx = 0;
	int prune_start = prune_premature_packets(output);

#if DEBUG_STARTING_PACKETS == 1
	blog(LOG_DEBUG, "--------- Pruning! %d ---------", prune_start);
	for (size_t i = 0; i < output->interleaved_packets.num; i++) {
		struct encoder_packet *packet =
			&output->interleaved_packets.array[i];
		blog(LOG_DEBUG, "packet: %s %d, ts: %lld, pruned = %s",
				packet->type == OBS_ENCODER_AUDIO ?
				"audio" : "video", (int)packet->track_idx,
				packet->dts_usec,
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

static int find_first_packet_type_idx(struct obs_output *output,
		enum obs_encoder_type type, size_t audio_idx)
{
	for (size_t i = 0; i < output->interleaved_packets.num; i++) {
		struct encoder_packet *packet =
			&output->interleaved_packets.array[i];

		if (packet->type == type) {
			if (type == OBS_ENCODER_AUDIO &&
			    packet->track_idx != audio_idx) {
				continue;
			}

			return (int)i;
		}
	}

	return -1;
}

static int find_last_packet_type_idx(struct obs_output *output,
		enum obs_encoder_type type, size_t audio_idx)
{
	for (size_t i = output->interleaved_packets.num; i > 0; i--) {
		struct encoder_packet *packet =
			&output->interleaved_packets.array[i - 1];

		if (packet->type == type) {
			if (type == OBS_ENCODER_AUDIO &&
			    packet->track_idx != audio_idx) {
				continue;
			}

			return (int)(i - 1);
		}
	}

	return -1;
}

static inline struct encoder_packet *find_first_packet_type(
		struct obs_output *output, enum obs_encoder_type type,
		size_t audio_idx)
{
	int idx = find_first_packet_type_idx(output, type, audio_idx);
	return (idx != -1) ? &output->interleaved_packets.array[idx] : NULL;
}

static inline struct encoder_packet *find_last_packet_type(
		struct obs_output *output, enum obs_encoder_type type,
		size_t audio_idx)
{
	int idx = find_last_packet_type_idx(output, type, audio_idx);
	return (idx != -1) ? &output->interleaved_packets.array[idx] : NULL;
}

static bool get_audio_and_video_packets(struct obs_output *output,
		struct encoder_packet **video,
		struct encoder_packet **audio, size_t audio_mixes)
{
	*video = find_first_packet_type(output, OBS_ENCODER_VIDEO, 0);
	if (!*video)
		output->received_video = false;

	for (size_t i = 0; i < audio_mixes; i++) {
		audio[i] = find_first_packet_type(output, OBS_ENCODER_AUDIO, i);
		if (!audio[i]) {
			output->received_audio = false;
			return false;
		}
	}

	if (!*video) {
		return false;
	}

	return true;
}

static bool initialize_interleaved_packets(struct obs_output *output)
{
	struct encoder_packet *video;
	struct encoder_packet *audio[MAX_AUDIO_MIXES];
	struct encoder_packet *last_audio[MAX_AUDIO_MIXES];
	size_t audio_mixes = num_audio_mixes(output);
	size_t start_idx;

	if (!get_audio_and_video_packets(output, &video, audio, audio_mixes))
		return false;

	for (size_t i = 0; i < audio_mixes; i++)
		last_audio[i] = find_last_packet_type(output, OBS_ENCODER_AUDIO,
				i);

	/* ensure that there is audio past the first video packet */
	for (size_t i = 0; i < audio_mixes; i++) {
		if (last_audio[i]->dts_usec < video->dts_usec) {
			output->received_audio = false;
			return false;
		}
	}

	/* clear out excess starting audio if it hasn't been already */
	start_idx = get_interleaved_start_idx(output);
	if (start_idx) {
		discard_to_idx(output, start_idx);
		if (!get_audio_and_video_packets(output, &video, audio,
					audio_mixes))
			return false;
	}

	/* get new offsets */
	output->video_offset = video->dts;
	for (size_t i = 0; i < audio_mixes; i++)
		output->audio_offsets[i] = audio[i]->dts;

#if DEBUG_STARTING_PACKETS == 1
	int64_t v = video->dts_usec;
	int64_t a = audio[0]->dts_usec;
	int64_t diff = v - a;

	blog(LOG_DEBUG, "output '%s' offset for video: %lld, audio: %lld, "
			"diff: %lldms", output->context.name, v, a,
			diff / 1000LL);
#endif

	/* subtract offsets from highest TS offset variables */
	output->highest_audio_ts -= audio[0]->dts_usec;
	output->highest_video_ts -= video->dts_usec;

	/* apply new offsets to all existing packet DTS/PTS values */
	for (size_t i = 0; i < output->interleaved_packets.num; i++) {
		struct encoder_packet *packet =
			&output->interleaved_packets.array[i];
		apply_interleaved_packet_offset(output, packet);
	}

	return true;
}

static inline void insert_interleaved_packet(struct obs_output *output,
		struct encoder_packet *out)
{
	size_t idx;
	for (idx = 0; idx < output->interleaved_packets.num; idx++) {
		struct encoder_packet *cur_packet;
		cur_packet = output->interleaved_packets.array + idx;

		if (out->dts_usec < cur_packet->dts_usec)
			break;
	}

	da_insert(output->interleaved_packets, idx, out);
}

static void resort_interleaved_packets(struct obs_output *output)
{
	DARRAY(struct encoder_packet) old_array;

	old_array.da = output->interleaved_packets.da;
	memset(&output->interleaved_packets, 0,
			sizeof(output->interleaved_packets));

	for (size_t i = 0; i < old_array.num; i++)
		insert_interleaved_packet(output, &old_array.array[i]);

	da_free(old_array);
}

static void discard_unused_audio_packets(struct obs_output *output,
		int64_t dts_usec)
{
	size_t idx = 0;

	for (; idx < output->interleaved_packets.num; idx++) {
		struct encoder_packet *p =
			&output->interleaved_packets.array[idx];

		if (p->dts_usec >= dts_usec)
			break;
	}

	if (idx)
		discard_to_idx(output, idx);
}

static void interleave_packets(void *data, struct encoder_packet *packet)
{
	struct obs_output     *output = data;
	struct encoder_packet out;
	bool                  was_started;

	if (!active(output))
		return;

	if (packet->type == OBS_ENCODER_AUDIO)
		packet->track_idx = get_track_index(output, packet);

	pthread_mutex_lock(&output->interleaved_mutex);

	/* if first video frame is not a keyframe, discard until received */
	if (!output->received_video &&
	    packet->type == OBS_ENCODER_VIDEO &&
	    !packet->keyframe) {
		discard_unused_audio_packets(output, packet->dts_usec);
		pthread_mutex_unlock(&output->interleaved_mutex);

		if (output->active_delay_ns)
			obs_encoder_packet_release(packet);
		return;
	}

	was_started = output->received_audio && output->received_video;

	if (output->active_delay_ns)
		out = *packet;
	else
		obs_encoder_packet_create_instance(&out, packet);

	if (was_started)
		apply_interleaved_packet_offset(output, &out);
	else
		check_received(output, packet);

	insert_interleaved_packet(output, &out);
	set_higher_ts(output, &out);

	/* when both video and audio have been received, we're ready
	 * to start sending out packets (one at a time) */
	if (output->received_audio && output->received_video) {
		if (!was_started) {
			if (prune_interleaved_packets(output)) {
				if (initialize_interleaved_packets(output)) {
					resort_interleaved_packets(output);
					send_interleaved(output);
				}
			}
		} else {
			send_interleaved(output);
		}
	}

	pthread_mutex_unlock(&output->interleaved_mutex);
}

static void default_encoded_callback(void *param, struct encoder_packet *packet)
{
	struct obs_output *output = param;

	if (data_active(output)) {
		if (packet->type == OBS_ENCODER_AUDIO)
			packet->track_idx = get_track_index(output, packet);

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
	if (data_active(output))
		output->info.raw_video(output->context.data, frame);
	output->total_frames++;
}

static void default_raw_audio_callback(void *param, size_t mix_idx,
		struct audio_data *frames)
{
	struct obs_output *output = param;
	if (!data_active(output))
		return;

	output->info.raw_audio(output->context.data, frames);

	UNUSED_PARAMETER(mix_idx);
}

static inline void start_audio_encoders(struct obs_output *output,
		encoded_callback_t encoded_callback)
{
	size_t num_mixes = num_audio_mixes(output);

	for (size_t i = 0; i < num_mixes; i++) {
		obs_encoder_start(output->audio_encoders[i],
				encoded_callback, output);
	}
}

static void reset_packet_data(obs_output_t *output)
{
	output->received_audio   = false;
	output->received_video   = false;
	output->highest_audio_ts = 0;
	output->highest_video_ts = 0;
	output->video_offset     = 0;

	for (size_t i = 0; i < MAX_AUDIO_MIXES; i++)
		output->audio_offsets[0] = 0;

	free_packets(output);
}

static inline bool preserve_active(struct obs_output *output)
{
	return (output->delay_flags & OBS_OUTPUT_DELAY_PRESERVE) != 0;
}

static void hook_data_capture(struct obs_output *output, bool encoded,
		bool has_video, bool has_audio)
{
	encoded_callback_t encoded_callback;

	if (encoded) {
		pthread_mutex_lock(&output->interleaved_mutex);
		reset_packet_data(output);
		pthread_mutex_unlock(&output->interleaved_mutex);

		encoded_callback = (has_video && has_audio) ?
			interleave_packets : default_encoded_callback;

		if (output->delay_sec) {
			output->active_delay_ns =
				(uint64_t)output->delay_sec * 1000000000ULL;
			output->delay_cur_flags = output->delay_flags;
			output->delay_callback = encoded_callback;
			encoded_callback = process_delay;
			os_atomic_set_bool(&output->delay_active, true);

			blog(LOG_INFO, "Output '%s': %"PRIu32" second delay "
			               "active, preserve on disconnect is %s",
			               output->context.name,
			               output->delay_sec,
			               preserve_active(output) ? "on" : "off");
		}

		if (has_audio)
			start_audio_encoders(output, encoded_callback);
		if (has_video)
			obs_encoder_start(output->video_encoder,
					encoded_callback, output);
	} else {
		if (has_video)
			video_output_connect(output->video,
					get_video_conversion(output),
					default_raw_video_callback, output);
		if (has_audio)
			audio_output_connect(output->audio, output->mixer_idx,
					get_audio_conversion(output),
					default_raw_audio_callback, output);
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
	calldata_set_int(&params, "timeout_sec",
			output->reconnect_retry_cur_sec);
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
	calldata_set_string(&params, "last_error", output->last_error_message);
	calldata_set_int(&params, "code", output->stop_code);
	calldata_set_ptr(&params, "output", output);

	signal_handler_signal(output->context.signals, "stop", &params);

	calldata_free(&params);
}

static inline void convert_flags(const struct obs_output *output,
		uint32_t flags, bool *encoded, bool *has_video, bool *has_audio,
		bool *has_service)
{
	*encoded = (output->info.flags & OBS_OUTPUT_ENCODED) != 0;
	if (!flags)
		flags = output->info.flags;
	else
		flags &= output->info.flags;

	*has_video   = (flags & OBS_OUTPUT_VIDEO)   != 0;
	*has_audio   = (flags & OBS_OUTPUT_AUDIO)   != 0;
	*has_service = (flags & OBS_OUTPUT_SERVICE) != 0;
}

bool obs_output_can_begin_data_capture(const obs_output_t *output,
		uint32_t flags)
{
	bool encoded, has_video, has_audio, has_service;

	if (!obs_output_valid(output, "obs_output_can_begin_data_capture"))
		return false;

	if (delay_active(output)) return true;
	if (active(output)) return false;

	if (data_capture_ending(output))
		pthread_join(output->end_data_capture_thread, NULL);

	convert_flags(output, flags, &encoded, &has_video, &has_audio,
			&has_service);

	return can_begin_data_capture(output, encoded, has_video, has_audio,
			has_service);
}

static inline bool initialize_audio_encoders(obs_output_t *output,
		size_t num_mixes)
{
	for (size_t i = 0; i < num_mixes; i++) {
		if (!obs_encoder_initialize(output->audio_encoders[i])) {
			return false;
		}
	}

	return true;
}

static inline obs_encoder_t *find_inactive_audio_encoder(obs_output_t *output,
		size_t num_mixes)
{
	for (size_t i = 0; i < num_mixes; i++) {
		struct obs_encoder *audio = output->audio_encoders[i];

		if (!audio->active && !audio->paired_encoder)
			return audio;
	}

	return NULL;
}

static inline void pair_encoders(obs_output_t *output, size_t num_mixes)
{
	struct obs_encoder *video = output->video_encoder;
	struct obs_encoder *audio = find_inactive_audio_encoder(output,
			num_mixes);

	if (video && audio) {
		pthread_mutex_lock(&audio->init_mutex);
		pthread_mutex_lock(&video->init_mutex);

		if (!audio->active &&
		    !video->active &&
		    !video->paired_encoder &&
		    !audio->paired_encoder) {

			audio->wait_for_video = true;
			audio->paired_encoder = video;
			video->paired_encoder = audio;
		}

		pthread_mutex_unlock(&video->init_mutex);
		pthread_mutex_unlock(&audio->init_mutex);
	}
}

bool obs_output_initialize_encoders(obs_output_t *output, uint32_t flags)
{
	bool encoded, has_video, has_audio, has_service;
	size_t num_mixes = num_audio_mixes(output);

	if (!obs_output_valid(output, "obs_output_initialize_encoders"))
		return false;

	if (active(output)) return delay_active(output);

	convert_flags(output, flags, &encoded, &has_video, &has_audio,
			&has_service);

	if (!encoded)
		return false;
	if (has_service && !obs_service_initialize(output->service, output))
		return false;
	if (has_video && !obs_encoder_initialize(output->video_encoder))
		return false;
	if (has_audio && !initialize_audio_encoders(output, num_mixes))
		return false;

	if (has_video && has_audio)
		pair_encoders(output, num_mixes);

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

bool obs_output_begin_data_capture(obs_output_t *output, uint32_t flags)
{
	bool encoded, has_video, has_audio, has_service;

	if (!obs_output_valid(output, "obs_output_begin_data_capture"))
		return false;

	if (delay_active(output)) return begin_delayed_capture(output);
	if (active(output)) return false;

	output->total_frames   = 0;

	convert_flags(output, flags, &encoded, &has_video, &has_audio,
			&has_service);

	if (!can_begin_data_capture(output, encoded, has_video, has_audio,
				has_service))
		return false;

	os_atomic_set_bool(&output->data_active, true);
	hook_data_capture(output, encoded, has_video, has_audio);

	if (has_service)
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

static inline void stop_audio_encoders(obs_output_t *output,
		encoded_callback_t encoded_callback)
{
	size_t num_mixes = num_audio_mixes(output);

	for (size_t i = 0; i < num_mixes; i++) {
		obs_encoder_stop(output->audio_encoders[i],
				encoded_callback, output);
	}
}

static void *end_data_capture_thread(void *data)
{
	bool encoded, has_video, has_audio, has_service;
	encoded_callback_t encoded_callback;
	obs_output_t *output = data;

	convert_flags(output, 0, &encoded, &has_video, &has_audio,
			&has_service);

	if (encoded) {
		if (output->active_delay_ns)
			encoded_callback = process_delay;
		else
			encoded_callback = (has_video && has_audio) ?
				interleave_packets : default_encoded_callback;

		if (has_video)
			obs_encoder_stop(output->video_encoder,
					encoded_callback, output);
		if (has_audio)
			stop_audio_encoders(output, encoded_callback);
	} else {
		if (has_video)
			video_output_disconnect(output->video,
					default_raw_video_callback, output);
		if (has_audio)
			audio_output_disconnect(output->audio,
					output->mixer_idx,
					default_raw_audio_callback, output);
	}

	if (has_service)
		obs_service_deactivate(output->service, false);

	if (output->active_delay_ns)
		obs_output_cleanup_delay(output);

	do_output_signal(output, "deactivate");
	os_atomic_set_bool(&output->active, false);
	os_event_signal(output->stopping_event);
	os_atomic_set_bool(&output->end_data_capture_thread_active, false);

	return NULL;
}

static void obs_output_end_data_capture_internal(obs_output_t *output,
		bool signal)
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

	if (output->video)
		log_frame_info(output);

	if (data_capture_ending(output))
		pthread_join(output->end_data_capture_thread, NULL);

	os_atomic_set_bool(&output->end_data_capture_thread_active, true);
	ret = pthread_create(&output->end_data_capture_thread, NULL,
			end_data_capture_thread, output);
	if (ret != 0) {
		blog(LOG_WARNING, "Failed to create end_data_capture_thread "
				"for output '%s'!", output->context.name);
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
	unsigned long ms = output->reconnect_retry_cur_sec * 1000;

	output->reconnect_thread_active = true;

	if (os_event_timedwait(output->reconnect_stop_event, ms) == ETIMEDOUT)
		obs_output_actual_start(output);

	if (os_event_try(output->reconnect_stop_event) == EAGAIN)
		pthread_detach(output->reconnect_thread);
	else
		os_atomic_set_bool(&output->reconnecting, false);

	output->reconnect_thread_active = false;
	return NULL;
}

#define MAX_RETRY_SEC (15 * 60)

static void output_reconnect(struct obs_output *output)
{
	int ret;

	if (!reconnecting(output)) {
		output->reconnect_retry_cur_sec = output->reconnect_retry_sec;
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
		output->reconnect_retry_cur_sec *= 2;
		if (output->reconnect_retry_cur_sec > MAX_RETRY_SEC)
			output->reconnect_retry_cur_sec = MAX_RETRY_SEC;
	}

	output->reconnect_retries++;

	output->stop_code = OBS_OUTPUT_DISCONNECTED;
	ret = pthread_create(&output->reconnect_thread, NULL,
			&reconnect_thread, output);
	if (ret < 0) {
		blog(LOG_WARNING, "Failed to create reconnect thread");
		os_atomic_set_bool(&output->reconnecting, false);
	} else {
		blog(LOG_INFO, "Output '%s':  Reconnecting in %d seconds..",
				output->context.name,
				output->reconnect_retry_sec);

		signal_reconnect(output);
	}
}

static inline bool can_reconnect(const obs_output_t *output, int code)
{
	bool reconnect_active = output->reconnect_retry_max != 0;

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
		obs_output_end_data_capture(output);
	}
}

void obs_output_addref(obs_output_t *output)
{
	if (!output)
		return;

	obs_ref_addref(&output->control->ref);
}

void obs_output_release(obs_output_t *output)
{
	if (!output)
		return;

	obs_weak_output_t *control = output->control;
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

	return obs_weak_output_get_output(output->control);
}

obs_weak_output_t *obs_output_get_weak_output(obs_output_t *output)
{
	if (!output)
		return NULL;

	obs_weak_output_t *weak = output->control;
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

bool obs_weak_output_references_output(obs_weak_output_t *weak,
		obs_output_t *output)
{
	return weak && output && weak->output == output;
}

void *obs_output_get_type_data(obs_output_t *output)
{
	return obs_output_valid(output, "obs_output_get_type_data")
		? output->info.type_data : NULL;
}

const char *obs_output_get_id(const obs_output_t *output)
{
	return obs_output_valid(output, "obs_output_get_id")
		? output->info.id : NULL;
}

#if BUILD_CAPTIONS
static struct caption_text *caption_text_new(const char *text, size_t bytes,
		struct caption_text *tail, struct caption_text **head)
{
	struct caption_text *next = bzalloc(sizeof(struct caption_text));
	snprintf(&next->text[0], CAPTION_LINE_BYTES + 1, "%.*s", bytes, text);

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
	if (!active(output))
		return;

	// split text into 32 character strings
	int size = (int)strlen(text);
	int r;
	size_t char_count;
	size_t line_length = 0;
	size_t trimmed_length = 0;

	blog(LOG_DEBUG, "Caption text: %s", text);

	pthread_mutex_lock(&output->caption_mutex);

	for (r = 0 ; 0 < size && CAPTION_LINE_CHARS > r; ++r) {
		line_length = utf8_line_length(text);
		trimmed_length = utf8_trimmed_length(text, line_length);
		char_count = utf8_char_count(text, trimmed_length);

		if (SCREEN_COLS < char_count) {
			char_count = utf8_wrap_length(text, CAPTION_LINE_CHARS);
			line_length = utf8_string_length(text, char_count + 1);
		}

		output->caption_tail = caption_text_new(
				text,
				line_length,
				output->caption_tail,
				&output->caption_head);

		text += line_length;
		size -= (int)line_length;
	}

	pthread_mutex_unlock(&output->caption_mutex);
}
#endif

float obs_output_get_congestion(obs_output_t *output)
{
	if (!obs_output_valid(output, "obs_output_get_congestion"))
		return 0;

	if (output->info.get_congestion) {
		float val = output->info.get_congestion(output->context.data);
		if (val < 0.0f) val = 0.0f;
		else if (val > 1.0f) val = 1.0f;
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

	return output->last_error_message;
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
	return obs_output_valid(output, __FUNCTION__) ?
		output->info.encoded_video_codecs : NULL;
}

const char *obs_output_get_supported_audio_codecs(const obs_output_t *output)
{
	return obs_output_valid(output, __FUNCTION__) ?
		output->info.encoded_audio_codecs : NULL;
}
