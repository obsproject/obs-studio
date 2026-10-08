/******************************************************************************
    Copyright (C) 2026 pkv <pkv@obsproject.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "monitoring-mix.h"

#include "../obs-internal.h"
#include "../util/darray.h"
#include "../util/deque.h"
#include "../util/platform.h"
#include "../util/spsc-fifo.h"
#include "../util/threading.h"

#include <inttypes.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

enum {
	kMonitoringMixPeriodFrames = 256,
	kMonitoringFifoCapacity = 16385,
};

struct monitoring_mix_delay_packet {
	uint64_t timestamp;
	uint32_t frames;
};

struct monitoring_mix_source {
	obs_source_t *source;
	struct spsc_fifo fifo[MAX_AUDIO_CHANNELS];

	bool source_has_video;
	uint64_t last_recv_time;
	uint64_t prev_video_ts;
	uint64_t time_since_prev;

	struct deque delay_buffer;
	DARRAY(float) delay_audio;
};

struct monitoring_mix_output {
	monitoring_mix_output_callback_t callback;
	void *param;
};

struct monitoring_mix_data {
	DARRAY(struct monitoring_mix_source *) sources;
	DARRAY(struct monitoring_mix_output) outputs;

	uint32_t sample_rate;
	uint32_t channels;
	uint32_t block_size;

	pthread_mutex_t sources_mutex;
	pthread_mutex_t outputs_mutex;

	pthread_t mix_thread;
	volatile bool shutting_down;

	float mix_buffer[MAX_AUDIO_CHANNELS][kMonitoringMixPeriodFrames];
};

static bool monitoring_mix_process_audio_delay(struct monitoring_mix_data *mix_data,
					       struct monitoring_mix_source *mix_source,
					       const struct audio_data *audio_data, struct audio_data *output);

static uint32_t monitoring_mix_mixing_source(struct monitoring_mix_data *mix_data,
					     struct monitoring_mix_source *mix_source)
{
	float source_buffer[kMonitoringMixPeriodFrames];
	long frames = spsc_fifo_readable_frames(&mix_source->fifo[0]);
	if (frames <= 0) {
		return 0;
	}

	if (frames > (long)mix_data->block_size) {
		frames = mix_data->block_size;
	}

	const float volume = mix_source->source->user_volume;

	for (int32_t channel = mix_data->channels - 1; channel >= 0; channel--) {
		if (!spsc_fifo_read(&mix_source->fifo[channel], source_buffer, frames)) {
			return 0;
		}

		float *dst = mix_data->mix_buffer[channel];

		for (long frame = 0; frame < frames; frame++) {
			dst[frame] += source_buffer[frame] * volume;
		}
	}

	return (uint32_t)frames;
}

static void monitoring_mix_clamp(struct monitoring_mix_data *mix_data, uint32_t frames)
{
	for (uint32_t channel = 0; channel < mix_data->channels; channel++) {
		float *samples = mix_data->mix_buffer[channel];
		for (uint32_t frame = 0; frame < frames; frame++) {
			float sample = samples[frame];
			if (isnan(sample)) {
				sample = 0.0f;
			}
			sample = (sample > 1.0f) ? 1.0f : sample;
			sample = (sample < -1.0f) ? -1.0f : sample;
			samples[frame] = sample;
		}
	}
}

static uint32_t monitoring_mix_do_mixing(struct monitoring_mix_data *mix_data)
{
	uint32_t mixed_frames = 0;

	memset(mix_data->mix_buffer, 0, sizeof(mix_data->mix_buffer));

	pthread_mutex_lock(&mix_data->sources_mutex);

	for (size_t i = 0; i < mix_data->sources.num; i++) {
		struct monitoring_mix_source *mix_source = mix_data->sources.array[i];

		const uint32_t source_frames = monitoring_mix_mixing_source(mix_data, mix_source);

		if (source_frames > mixed_frames) {
			mixed_frames = source_frames;
		}
	}

	pthread_mutex_unlock(&mix_data->sources_mutex);
	if (mixed_frames > 0) {
		monitoring_mix_clamp(mix_data, mixed_frames);
	}

	return mixed_frames;
}

static void monitoring_mix_dispatch_outputs(struct monitoring_mix_data *mix_data, uint32_t frames)
{
	if (!mix_data || frames == 0 || frames > kMonitoringMixPeriodFrames) {
		return;
	}

	struct audio_data audio = {
		.frames = frames,
		.timestamp = os_gettime_ns(),
	};

	for (uint32_t channel = 0; channel < mix_data->channels; channel++) {
		audio.data[channel] = (uint8_t *)mix_data->mix_buffer[channel];
	}

	pthread_mutex_lock(&mix_data->outputs_mutex);

	for (size_t i = 0; i < mix_data->outputs.num; i++) {
		const struct monitoring_mix_output *output = &mix_data->outputs.array[i];

		output->callback(output->param, &audio);
	}

	pthread_mutex_unlock(&mix_data->outputs_mutex);
}

static void *mix_sources_thread(void *param)
{
	struct monitoring_mix_data *mix_data = param;
	uint64_t elapsed_frames = 0;
	const uint64_t start_time = os_gettime_ns();

	os_set_thread_name("audio-monitoring: mix thread");

	while (!os_atomic_load_bool(&mix_data->shutting_down)) {
		elapsed_frames += mix_data->block_size;
		const uint64_t deadline = start_time + audio_frames_to_ns(mix_data->sample_rate, elapsed_frames);
		os_sleepto_ns_fast(deadline);
		if (os_atomic_load_bool(&mix_data->shutting_down)) {
			break;
		}
		const uint32_t mixed_frames = monitoring_mix_do_mixing(mix_data);
		if (mixed_frames > 0) {
			monitoring_mix_dispatch_outputs(mix_data, mixed_frames);
		}
	}

	return NULL;
}

struct monitoring_mix_data *monitoring_mix_create(uint32_t sample_rate, uint32_t channels)
{
	if (sample_rate == 0 || channels == 0 || channels > MAX_AUDIO_CHANNELS) {
		return NULL;
	}

	struct monitoring_mix_data *mix_data = bzalloc(sizeof(struct monitoring_mix_data));
	if (!mix_data) {
		return NULL;
	}

	mix_data->sample_rate = sample_rate;
	mix_data->channels = channels;
	mix_data->block_size = kMonitoringMixPeriodFrames;
	da_init(mix_data->sources);
	da_init(mix_data->outputs);

	pthread_mutex_init_value(&mix_data->sources_mutex);
	pthread_mutex_init_value(&mix_data->outputs_mutex);

	if (pthread_mutex_init(&mix_data->sources_mutex, NULL) != 0) {
		goto fail_sources_mutex;
	}

	if (pthread_mutex_init(&mix_data->outputs_mutex, NULL) != 0) {
		goto fail_outputs_mutex;
	}

	if (pthread_create(&mix_data->mix_thread, NULL, mix_sources_thread, mix_data) != 0) {
		goto fail;
	}

	return mix_data;

fail:
	pthread_mutex_destroy(&mix_data->outputs_mutex);

fail_outputs_mutex:
	pthread_mutex_destroy(&mix_data->sources_mutex);

fail_sources_mutex:
	da_free(mix_data->outputs);
	da_free(mix_data->sources);
	bfree(mix_data);
	return NULL;
}

/* Taken from audio-monitoring/wasapi-output.c*/
static bool monitoring_mix_process_audio_delay(struct monitoring_mix_data *mix_data,
					       struct monitoring_mix_source *mix_source,
					       const struct audio_data *audio_data, struct audio_data *output)
{
	obs_source_t *source = mix_source->source;
	const uint64_t last_frame_ts = source->last_frame_ts;
	const uint64_t current_time = os_gettime_ns();
	uint64_t front_ts;
	int64_t diff;
	const uint32_t channels = mix_data->channels;

	/* cut off audio if long-since leftover audio in delay buffer */
	if (current_time - mix_source->last_recv_time > 1000000000) {
		deque_free(&mix_source->delay_buffer);
	}
	mix_source->last_recv_time = current_time;

	struct monitoring_mix_delay_packet input_packet = {
		.timestamp = audio_data->timestamp,
		.frames = audio_data->frames,
	};

	input_packet.timestamp += source->sync_offset;

	deque_push_back(&mix_source->delay_buffer, &input_packet, sizeof(input_packet));

	const size_t input_channel_size = (size_t)input_packet.frames * sizeof(float);

	for (uint32_t channel = 0; channel < channels; channel++) {
		deque_push_back(&mix_source->delay_buffer, audio_data->data[channel], input_channel_size);
	}

	if (!mix_source->prev_video_ts) {
		mix_source->prev_video_ts = last_frame_ts;
	} else if (mix_source->prev_video_ts == last_frame_ts) {
		mix_source->time_since_prev += audio_frames_to_ns(mix_data->sample_rate, input_packet.frames);
	} else {
		mix_source->time_since_prev = 0;
	}

	const long readable_frames = spsc_fifo_readable_frames(&mix_source->fifo[0]);
	const uint32_t padding_frames = readable_frames > 0 ? (uint32_t)readable_frames : 0;
	const uint64_t padding_ns = audio_frames_to_ns(mix_data->sample_rate, padding_frames);

	while (mix_source->delay_buffer.size != 0) {
		size_t size;
		bool bad_diff;
		struct monitoring_mix_delay_packet packet;

		deque_peek_front(&mix_source->delay_buffer, &packet, sizeof(packet));
		front_ts = packet.timestamp - padding_ns;
		diff = (int64_t)front_ts - (int64_t)last_frame_ts;
		bad_diff = !last_frame_ts || llabs(diff) > 5000000000 || mix_source->time_since_prev > 100000000ULL;

		/* delay audio if rushing */
		if (!bad_diff && diff > 75000000) {
			return false;
		}

		deque_pop_front(&mix_source->delay_buffer, NULL, sizeof(packet));

		size = (size_t)packet.frames * channels;
		size_t size_bytes = size * sizeof(float);
		da_resize(mix_source->delay_audio, size);
		deque_pop_front(&mix_source->delay_buffer, mix_source->delay_audio.array, size_bytes);

		/* cut audio if dragging */
		if (!bad_diff && diff < -75000000 && mix_source->delay_buffer.size != 0) {
			continue;
		}

		*output = (struct audio_data){
			.frames = packet.frames,
			.timestamp = packet.timestamp,
		};

		for (uint32_t channel = 0; channel < channels; channel++) {
			output->data[channel] =
				(uint8_t *)(mix_source->delay_audio.array + (size_t)channel * packet.frames);
		}
		return true;
	}

	return false;
}

static void on_mix_source_play(void *param, obs_source_t *source, const struct audio_data *audio_data, bool muted)
{
	UNUSED_PARAMETER(muted);
	struct monitoring_mix_source *mix_source = (struct monitoring_mix_source *)param;

	if (!source || !param || source != mix_source->source || !audio_data || !audio_data->data[0]) {
		return;
	}

	if (os_atomic_load_long(&source->activate_refs) == 0) {
		return;
	}

	struct monitoring_mix_data *mix_data = obs->audio.monitoring_mix;
	struct audio_data delayed_audio = {0};
	const struct audio_data *input_audio = audio_data;
	const bool decouple_audio = source->async_unbuffered && source->async_decoupled;

	if (mix_source->source_has_video && !decouple_audio) {
		if (!monitoring_mix_process_audio_delay(mix_data, mix_source, audio_data, &delayed_audio)) {
			return;
		}

		input_audio = &delayed_audio;
	}

	const long requested_frames = (long)input_audio->frames;
	long frames = requested_frames;
	const long writable_frames = spsc_fifo_writable_frames(&mix_source->fifo[0]);

	if (frames > writable_frames) {
		frames = writable_frames;
	}

	for (int32_t channel = mix_data->channels - 1; channel >= 0; channel--) {
		spsc_fifo_write(&mix_source->fifo[channel], (const float *)input_audio->data[channel], frames);
	}
}

struct monitoring_mix_source *monitoring_mix_add_source(obs_source_t *source)
{
	struct monitoring_mix_data *mix_data = obs->audio.monitoring_mix;
	if (!source || !mix_data || os_atomic_load_bool(&mix_data->shutting_down)) {
		return NULL;
	}

	struct monitoring_mix_source *mix_source = bzalloc(sizeof(*mix_source));
	if (!mix_source) {
		return NULL;
	}

	mix_source->source = source;

	mix_source->source_has_video = (source->info.output_flags & OBS_SOURCE_VIDEO) != 0;
	deque_init(&mix_source->delay_buffer);
	da_init(mix_source->delay_audio);

	uint32_t initialized_channels = 0;
	for (; initialized_channels < mix_data->channels; initialized_channels++) {
		if (!spsc_fifo_init(&mix_source->fifo[initialized_channels], kMonitoringFifoCapacity)) {
			goto fail;
		}
	}

	pthread_mutex_lock(&mix_data->sources_mutex);
	if (os_atomic_load_bool(&mix_data->shutting_down)) {
		pthread_mutex_unlock(&mix_data->sources_mutex);
		goto fail;
	}
	if (source->monitoring_mix_source) {
		struct monitoring_mix_source *existing = source->monitoring_mix_source;

		pthread_mutex_unlock(&mix_data->sources_mutex);

		for (uint32_t channel = 0; channel < mix_data->channels; channel++) {
			spsc_fifo_free(&mix_source->fifo[channel]);
		}
		bfree(mix_source);
		return existing;
	}
	da_push_back(mix_data->sources, &mix_source);
	source->monitoring_mix_source = mix_source;
	obs_source_add_audio_capture_callback(source, on_mix_source_play, mix_source);
	pthread_mutex_unlock(&mix_data->sources_mutex);

	return mix_source;

fail:
	for (uint32_t channel = 0; channel < initialized_channels; channel++) {
		spsc_fifo_free(&mix_source->fifo[channel]);
	}

	deque_free(&mix_source->delay_buffer);
	da_free(mix_source->delay_audio);

	bfree(mix_source);
	return NULL;
}

static void monitoring_mix_remove_source_internal(struct monitoring_mix_data *mix_data,
						  struct monitoring_mix_source *mix_source)
{
	size_t idx = DARRAY_INVALID;
	for (size_t i = 0; i < mix_data->sources.num; i++) {
		if (mix_data->sources.array[i] == mix_source) {
			idx = i;
			break;
		}
	}

	if (idx == DARRAY_INVALID) {
		return;
	}

	obs_source_remove_audio_capture_callback(mix_source->source, on_mix_source_play, mix_source);
	deque_free(&mix_source->delay_buffer);
	da_free(mix_source->delay_audio);

	da_erase(mix_data->sources, idx);

	if (mix_source->source->monitoring_mix_source == mix_source) {
		mix_source->source->monitoring_mix_source = NULL;
	}

	for (uint32_t channel = 0; channel < mix_data->channels; channel++) {
		spsc_fifo_free(&mix_source->fifo[channel]);
	}

	bfree(mix_source);
}

void monitoring_mix_remove_source(struct monitoring_mix_data *mix_data, struct monitoring_mix_source *mix_source)
{
	if (!mix_data || !mix_source) {
		return;
	}

	pthread_mutex_lock(&mix_data->sources_mutex);
	monitoring_mix_remove_source_internal(mix_data, mix_source);
	pthread_mutex_unlock(&mix_data->sources_mutex);
}

void monitoring_mix_destroy(struct monitoring_mix_data *mix_data)
{
	if (!mix_data) {
		return;
	}

	os_atomic_store_bool(&mix_data->shutting_down, true);
	pthread_join(mix_data->mix_thread, NULL);

	pthread_mutex_lock(&mix_data->outputs_mutex);
	da_free(mix_data->outputs);
	pthread_mutex_unlock(&mix_data->outputs_mutex);

	pthread_mutex_lock(&mix_data->sources_mutex);
	while (mix_data->sources.num > 0) {
		struct monitoring_mix_source *mix_source = mix_data->sources.array[mix_data->sources.num - 1];
		monitoring_mix_remove_source_internal(mix_data, mix_source);
	}
	da_free(mix_data->sources);
	pthread_mutex_unlock(&mix_data->sources_mutex);

	pthread_mutex_destroy(&mix_data->outputs_mutex);
	pthread_mutex_destroy(&mix_data->sources_mutex);

	bfree(mix_data);
}

static size_t monitoring_mix_find_output(const struct monitoring_mix_data *mix_data,
					 monitoring_mix_output_callback_t callback, void *param)
{
	for (size_t i = 0; i < mix_data->outputs.num; i++) {
		const struct monitoring_mix_output *output = &mix_data->outputs.array[i];

		if (output->callback == callback && output->param == param) {
			return i;
		}
	}

	return DARRAY_INVALID;
}

bool monitoring_mix_output_connect(struct monitoring_mix_data *mix_data, monitoring_mix_output_callback_t callback,
				   void *param)
{
	if (!mix_data || !callback) {
		return false;
	}

	bool connected = false;

	pthread_mutex_lock(&mix_data->outputs_mutex);

	if (!os_atomic_load_bool(&mix_data->shutting_down) &&
	    monitoring_mix_find_output(mix_data, callback, param) == DARRAY_INVALID) {
		const struct monitoring_mix_output output = {
			.callback = callback,
			.param = param,
		};

		da_push_back(mix_data->outputs, &output);
		connected = true;
	}

	pthread_mutex_unlock(&mix_data->outputs_mutex);

	return connected;
}

void monitoring_mix_output_disconnect(struct monitoring_mix_data *mix_data, monitoring_mix_output_callback_t callback,
				      void *param)
{
	if (!mix_data || !callback) {
		return;
	}

	pthread_mutex_lock(&mix_data->outputs_mutex);

	const size_t idx = monitoring_mix_find_output(mix_data, callback, param);

	if (idx != DARRAY_INVALID) {
		da_erase(mix_data->outputs, idx);
	}

	pthread_mutex_unlock(&mix_data->outputs_mutex);
}
