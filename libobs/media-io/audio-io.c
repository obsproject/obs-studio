/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

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

#include <math.h>
#include <inttypes.h>

#include "../util/threading.h"
#include "../util/darray.h"
#include "../util/circlebuf.h"
#include "../util/platform.h"

#include "audio-io.h"
#include "audio-resampler.h"

/* #define DEBUG_AUDIO */

#define nop() do {int invalid = 0;} while(0)

struct audio_input {
	struct audio_convert_info conversion;
	audio_resampler_t         *resampler;

	audio_output_callback_t callback;
	void *param;
};

static inline void audio_input_free(struct audio_input *input)
{
	audio_resampler_destroy(input->resampler);
}

struct audio_line {
	char                       *name;

	struct audio_output        *audio;
	struct circlebuf           buffers[MAX_AV_PLANES];
	pthread_mutex_t            mutex;
	DARRAY(uint8_t)            volume_buffers[MAX_AV_PLANES];
	uint64_t                   base_timestamp;
	uint64_t                   last_timestamp;

	uint64_t                   next_ts_min;

	/* specifies which mixes this line applies to via bits */
	uint32_t                   mixers;

	/* states whether this line is still being used.  if not, then when the
	 * buffer is depleted, it's destroyed */
	bool                       alive;

	struct audio_line          **prev_next;
	struct audio_line          *next;
};

static inline void audio_line_destroy_data(struct audio_line *line)
{
	for (size_t i = 0; i < MAX_AV_PLANES; i++) {
		circlebuf_free(&line->buffers[i]);
		da_free(line->volume_buffers[i]);
	}

	pthread_mutex_destroy(&line->mutex);
	bfree(line->name);
	bfree(line);
}

struct audio_mix {
	DARRAY(struct audio_input) inputs;
	DARRAY(uint8_t)            mix_buffers[MAX_AV_PLANES];
};

struct audio_output {
	struct audio_output_info   info;
	size_t                     block_size;
	size_t                     channels;
	size_t                     planes;

	pthread_t                  thread;
	os_event_t                 *stop_event;

	bool                       initialized;

	pthread_mutex_t            line_mutex;
	struct audio_line          *first_line;

	pthread_mutex_t            input_mutex;

	struct audio_mix           mixes[MAX_AUDIO_MIXES];
};

static inline void audio_output_removeline(struct audio_output *audio,
		struct audio_line *line)
{
	pthread_mutex_lock(&audio->line_mutex);
	if (line->prev_next)
		*line->prev_next = line->next;
	if (line->next)
		line->next->prev_next = line->prev_next;
	pthread_mutex_unlock(&audio->line_mutex);

	audio_line_destroy_data(line);
}

/* ------------------------------------------------------------------------- */
/* the following functions are used to calculate frame offsets based upon
 * timestamps.  this will actually work accurately as long as you handle the
 * values correctly */

static inline double ts_to_frames(const audio_t *audio, uint64_t ts)
{
	double audio_offset_d = (double)ts;
	audio_offset_d /= 1000000000.0;
	audio_offset_d *= (double)audio->info.samples_per_sec;

	return audio_offset_d;
}

static inline double positive_round(double val)
{
	return floor(val+0.5);
}

static size_t ts_diff_frames(const audio_t *audio, uint64_t ts1, uint64_t ts2)
{
	double diff = ts_to_frames(audio, ts1) - ts_to_frames(audio, ts2);
	return (size_t)positive_round(diff);
}

static size_t ts_diff_bytes(const audio_t *audio, uint64_t ts1, uint64_t ts2)
{
	return ts_diff_frames(audio, ts1, ts2) * audio->block_size;
}

/* unless the value is 3+ hours worth of frames, this won't overflow */
static inline uint64_t conv_frames_to_time(const audio_t *audio,
		uint32_t frames)
{
	return (uint64_t)frames * 1000000000ULL /
		(uint64_t)audio->info.samples_per_sec;
}

/* ------------------------------------------------------------------------- */

/* this only really happens with the very initial data insertion.  can be
 * ignored safely. */
static inline void clear_excess_audio_data(struct audio_line *line,
		uint64_t prev_time)
{
	size_t size = ts_diff_bytes(line->audio, prev_time,
			line->base_timestamp);

	/*blog(LOG_DEBUG, "Excess audio data for audio line '%s', somehow "
	                "audio data went back in time by %"PRIu32" bytes.  "
	                "prev_time: %"PRIu64", line->base_timestamp: %"PRIu64,
	                line->name, (uint32_t)size,
	                prev_time, line->base_timestamp);*/

	for (size_t i = 0; i < line->audio->planes; i++) {
		size_t clear_size = (size < line->buffers[i].size) ?
			size : line->buffers[i].size;

		circlebuf_pop_front(&line->buffers[i], NULL, clear_size);
	}
}

static inline uint64_t min_uint64(uint64_t a, uint64_t b)
{
	return a < b ? a : b;
}

static inline size_t min_size(size_t a, size_t b)
{
	return a < b ? a : b;
}

#ifndef CLAMP
#define CLAMP(val, minval, maxval) \
	((val > maxval) ? maxval : ((val < minval) ? minval : val))
#endif

#define MIX_BUFFER_SIZE 256

/* TODO: optimize mixing */
static void mix_float(struct audio_output *audio, struct audio_line *line,
		size_t size, size_t time_offset, size_t plane)
{
	float *mixes[MAX_AUDIO_MIXES];
	float vals[MIX_BUFFER_SIZE];

	for (size_t mix_idx = 0; mix_idx < MAX_AUDIO_MIXES; mix_idx++) {
		uint8_t *bytes = audio->mixes[mix_idx].mix_buffers[plane].array;
		mixes[mix_idx] = (float*)&bytes[time_offset];
	}

	while (size) {
		size_t pop_count = min_size(size, sizeof(vals));
		size -= pop_count;

		circlebuf_pop_front(&line->buffers[plane], vals, pop_count);
		pop_count /= sizeof(float);

		for (size_t mix_idx = 0; mix_idx < MAX_AUDIO_MIXES; mix_idx++) {
			/* only include this audio line in this mix if it's set
			 * via the line's 'mixes' variable */
			if ((line->mixers & (1 << mix_idx)) == 0)
				continue;

			for (size_t i = 0; i < pop_count; i++) {
				*(mixes[mix_idx]++) += vals[i];
			}
		}
	}
}

static inline bool mix_audio_line(struct audio_output *audio,
		struct audio_line *line, size_t size, uint64_t timestamp)
{
	size_t time_offset = ts_diff_bytes(audio,
			line->base_timestamp, timestamp);
	if (time_offset > size)
		return false;

	size -= time_offset;

#ifdef DEBUG_AUDIO
	blog(LOG_DEBUG, "shaved off %lu bytes", size);
#endif

	for (size_t i = 0; i < audio->planes; i++) {
		size_t pop_size = min_size(size, line->buffers[i].size);

		mix_float(audio, line, pop_size, time_offset, i);
	}

	return true;
}

static bool resample_audio_output(struct audio_input *input,
		struct audio_data *data)
{
	bool success = true;

	if (input->resampler) {
		uint8_t  *output[MAX_AV_PLANES];
		uint32_t frames;
		uint64_t offset;

		memset(output, 0, sizeof(output));

		success = audio_resampler_resample(input->resampler,
				output, &frames, &offset,
				(const uint8_t *const *)data->data,
				data->frames);

		for (size_t i = 0; i < MAX_AV_PLANES; i++)
			data->data[i] = output[i];
		data->frames     = frames;
		data->timestamp -= offset;
	}

	return success;
}

static inline void do_audio_output(struct audio_output *audio,
		size_t mix_idx, uint64_t timestamp, uint32_t frames)
{
	struct audio_mix *mix = &audio->mixes[mix_idx];
	struct audio_data data;

	for (size_t i = 0; i < MAX_AV_PLANES; i++)
		data.data[i] = mix->mix_buffers[i].array;

	data.frames = frames;
	data.timestamp = timestamp;
	data.volume = 1.0f;

	pthread_mutex_lock(&audio->input_mutex);

	for (size_t i = mix->inputs.num; i > 0; i--) {
		struct audio_input *input = mix->inputs.array+(i-1);

		if (resample_audio_output(input, &data))
			input->callback(input->param, mix_idx, &data);
	}

	pthread_mutex_unlock(&audio->input_mutex);
}

static inline void clamp_audio_output(struct audio_output *audio, size_t bytes)
{
	size_t float_size = bytes / sizeof(float);

	for (size_t mix_idx = 0; mix_idx < MAX_AUDIO_MIXES; mix_idx++) {
		struct audio_mix *mix = &audio->mixes[mix_idx];

		/* do not process mixing if a specific mix is inactive */
		if (!mix->inputs.num)
			continue;

		for (size_t plane = 0; plane < audio->planes; plane++) {
			float *mix_data = (float*)mix->mix_buffers[plane].array;
			float *mix_end = &mix_data[float_size];

			while (mix_data < mix_end) {
				float val = *mix_data;
				val = (val >  1.0f) ?  1.0f : val;
				val = (val < -1.0f) ? -1.0f : val;
				*(mix_data++) = val;
			}
		}
	}
}

static uint64_t mix_and_output(struct audio_output *audio, uint64_t audio_time,
		uint64_t prev_time)
{
	struct audio_line *line = audio->first_line;
	uint32_t frames = (uint32_t)ts_diff_frames(audio, audio_time,
	                                           prev_time);
	size_t bytes = frames * audio->block_size;

#ifdef DEBUG_AUDIO
	blog(LOG_DEBUG, "audio_time: %llu, prev_time: %llu, bytes: %lu",
			audio_time, prev_time, bytes);
#endif

	/* return an adjusted audio_time according to the amount
	 * of data that was sampled to ensure seamless transmission */
	audio_time = prev_time + conv_frames_to_time(audio, frames);

	/* resize and clear mix buffers */
	for (size_t mix_idx = 0; mix_idx < MAX_AUDIO_MIXES; mix_idx++) {
		struct audio_mix *mix = &audio->mixes[mix_idx];

		for (size_t i = 0; i < audio->planes; i++) {
			da_resize(mix->mix_buffers[i], bytes);
			memset(mix->mix_buffers[i].array, 0, bytes);
		}
	}

	/* mix audio lines */
	while (line) {
		struct audio_line *next = line->next;

		/* if line marked for removal, destroy and move to the next */
		if (!line->buffers[0].size) {
			if (!line->alive) {
				audio_output_removeline(audio, line);
				line = next;
				continue;
			}
		}

		pthread_mutex_lock(&line->mutex);

		if (line->buffers[0].size && line->base_timestamp < prev_time) {
			clear_excess_audio_data(line, prev_time);
			line->base_timestamp = prev_time;
		}

		if (mix_audio_line(audio, line, bytes, prev_time))
			line->base_timestamp = audio_time;

		pthread_mutex_unlock(&line->mutex);

		line = next;
	}

	/* clamps audio data to -1.0..1.0 */
	clamp_audio_output(audio, bytes);

	/* output */
	for (size_t i = 0; i < MAX_AUDIO_MIXES; i++)
		do_audio_output(audio, i, prev_time, frames);

	return audio_time;
}

/* sample audio 40 times a second */
#define AUDIO_WAIT_TIME (1000/40)

static void *audio_thread(void *param)
{
	struct audio_output *audio = param;
	uint64_t buffer_time = audio->info.buffer_ms * 1000000;
	uint64_t prev_time = os_gettime_ns() - buffer_time;
	uint64_t audio_time;

	os_set_thread_name("audio-io: audio thread");

	while (os_event_try(audio->stop_event) == EAGAIN) {
		os_sleep_ms(AUDIO_WAIT_TIME);

		pthread_mutex_lock(&audio->line_mutex);

		audio_time = os_gettime_ns() - buffer_time;
		audio_time = mix_and_output(audio, audio_time, prev_time);
		prev_time  = audio_time;

		pthread_mutex_unlock(&audio->line_mutex);
	}

	return NULL;
}

/* ------------------------------------------------------------------------- */

static size_t audio_get_input_idx(const audio_t *audio, size_t mix_idx,
		audio_output_callback_t callback, void *param)
{
	const struct audio_mix *mix = &audio->mixes[mix_idx];

	for (size_t i = 0; i < mix->inputs.num; i++) {
		struct audio_input *input = mix->inputs.array+i;

		if (input->callback == callback && input->param == param)
			return i;
	}

	return DARRAY_INVALID;
}

static inline bool audio_input_init(struct audio_input *input,
		struct audio_output *audio)
{
	if (input->conversion.format          != audio->info.format          ||
	    input->conversion.samples_per_sec != audio->info.samples_per_sec ||
	    input->conversion.speakers        != audio->info.speakers) {
		struct resample_info from = {
			.format          = audio->info.format,
			.samples_per_sec = audio->info.samples_per_sec,
			.speakers        = audio->info.speakers
		};

		struct resample_info to = {
			.format          = input->conversion.format,
			.samples_per_sec = input->conversion.samples_per_sec,
			.speakers        = input->conversion.speakers
		};

		input->resampler = audio_resampler_create(&to, &from);
		if (!input->resampler) {
			blog(LOG_ERROR, "audio_input_init: Failed to "
			                "create resampler");
			return false;
		}
	} else {
		input->resampler = NULL;
	}

	return true;
}

bool audio_output_connect(audio_t *audio, size_t mi,
		const struct audio_convert_info *conversion,
		audio_output_callback_t callback, void *param)
{
	bool success = false;

	if (!audio || mi >= MAX_AUDIO_MIXES) return false;

	pthread_mutex_lock(&audio->input_mutex);

	if (audio_get_input_idx(audio, mi, callback, param) == DARRAY_INVALID) {
		struct audio_mix *mix = &audio->mixes[mi];
		struct audio_input input;
		input.callback = callback;
		input.param    = param;

		if (conversion) {
			input.conversion = *conversion;
		} else {
			input.conversion.format = audio->info.format;
			input.conversion.speakers = audio->info.speakers;
			input.conversion.samples_per_sec =
				audio->info.samples_per_sec;
		}

		if (input.conversion.format == AUDIO_FORMAT_UNKNOWN)
			input.conversion.format = audio->info.format;
		if (input.conversion.speakers == SPEAKERS_UNKNOWN)
			input.conversion.speakers = audio->info.speakers;
		if (input.conversion.samples_per_sec == 0)
			input.conversion.samples_per_sec =
				audio->info.samples_per_sec;

		success = audio_input_init(&input, audio);
		if (success)
			da_push_back(mix->inputs, &input);
	}

	pthread_mutex_unlock(&audio->input_mutex);

	return success;
}

void audio_output_disconnect(audio_t *audio, size_t mix_idx,
		audio_output_callback_t callback, void *param)
{
	if (!audio || mix_idx >= MAX_AUDIO_MIXES) return;

	pthread_mutex_lock(&audio->input_mutex);

	size_t idx = audio_get_input_idx(audio, mix_idx, callback, param);
	if (idx != DARRAY_INVALID) {
		struct audio_mix *mix = &audio->mixes[mix_idx];
		audio_input_free(mix->inputs.array+idx);
		da_erase(mix->inputs, idx);
	}

	pthread_mutex_unlock(&audio->input_mutex);
}

static inline bool valid_audio_params(const struct audio_output_info *info)
{
	return info->format && info->name && info->samples_per_sec > 0 &&
	       info->speakers > 0;
}

int audio_output_open(audio_t **audio, struct audio_output_info *info)
{
	struct audio_output *out;
	pthread_mutexattr_t attr;
	bool planar = is_audio_planar(info->format);

	if (!valid_audio_params(info))
		return AUDIO_OUTPUT_INVALIDPARAM;

	out = bzalloc(sizeof(struct audio_output));
	if (!out)
		goto fail;

	memcpy(&out->info, info, sizeof(struct audio_output_info));
	pthread_mutex_init_value(&out->line_mutex);
	out->channels   = get_audio_channels(info->speakers);
	out->planes     = planar ? out->channels : 1;
	out->block_size = (planar ? 1 : out->channels) *
	                  get_audio_bytes_per_channel(info->format);

	if (pthread_mutexattr_init(&attr) != 0)
		goto fail;
	if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) != 0)
		goto fail;
	if (pthread_mutex_init(&out->line_mutex, &attr) != 0)
		goto fail;
	if (pthread_mutex_init(&out->input_mutex, &attr) != 0)
		goto fail;
	if (os_event_init(&out->stop_event, OS_EVENT_TYPE_MANUAL) != 0)
		goto fail;
	if (pthread_create(&out->thread, NULL, audio_thread, out) != 0)
		goto fail;

	out->initialized = true;
	*audio = out;
	return AUDIO_OUTPUT_SUCCESS;

fail:
	audio_output_close(out);
	return AUDIO_OUTPUT_FAIL;
}

void audio_output_close(audio_t *audio)
{
	void *thread_ret;
	struct audio_line *line;

	if (!audio)
		return;

	if (audio->initialized) {
		os_event_signal(audio->stop_event);
		pthread_join(audio->thread, &thread_ret);
	}

	line = audio->first_line;
	while (line) {
		struct audio_line *next = line->next;
		audio_line_destroy_data(line);
		line = next;
	}

	for (size_t mix_idx = 0; mix_idx < MAX_AUDIO_MIXES; mix_idx++) {
		struct audio_mix *mix = &audio->mixes[mix_idx];

		for (size_t i = 0; i < mix->inputs.num; i++)
			audio_input_free(mix->inputs.array+i);

		for (size_t i = 0; i < MAX_AV_PLANES; i++)
			da_free(mix->mix_buffers[i]);

		da_free(mix->inputs);
	}

	os_event_destroy(audio->stop_event);
	pthread_mutex_destroy(&audio->line_mutex);
	bfree(audio);
}

audio_line_t *audio_output_create_line(audio_t *audio, const char *name,
		uint32_t mixers)
{
	if (!audio) return NULL;

	struct audio_line *line = bzalloc(sizeof(struct audio_line));
	line->alive = true;
	line->audio = audio;
	line->mixers = mixers;

	if (pthread_mutex_init(&line->mutex, NULL) != 0) {
		blog(LOG_ERROR, "audio_output_createline: Failed to create "
		                "mutex");
		bfree(line);
		return NULL;
	}

	pthread_mutex_lock(&audio->line_mutex);

	if (audio->first_line) {
		audio->first_line->prev_next = &line->next;
		line->next = audio->first_line;
	}

	line->prev_next = &audio->first_line;
	audio->first_line = line;

	pthread_mutex_unlock(&audio->line_mutex);

	line->name = bstrdup(name ? name : "(unnamed audio line)");
	return line;
}

const struct audio_output_info *audio_output_get_info(const audio_t *audio)
{
	return audio ? &audio->info : NULL;
}

void audio_line_destroy(struct audio_line *line)
{
	if (line) {
		if (!line->buffers[0].size)
			audio_output_removeline(line->audio, line);
		else
			line->alive = false;
	}
}

bool audio_output_active(const audio_t *audio)
{
	if (!audio) return false;

	for (size_t mix_idx = 0; mix_idx < MAX_AUDIO_MIXES; mix_idx++) {
		const struct audio_mix *mix = &audio->mixes[mix_idx];

		if (mix->inputs.num != 0)
			return true;
	}

	return false;
}

size_t audio_output_get_block_size(const audio_t *audio)
{
	return audio ? audio->block_size : 0;
}

size_t audio_output_get_planes(const audio_t *audio)
{
	return audio ? audio->planes : 0;
}

size_t audio_output_get_channels(const audio_t *audio)
{
	return audio ? audio->channels : 0;
}

uint32_t audio_output_get_sample_rate(const audio_t *audio)
{
	return audio ? audio->info.samples_per_sec : 0;
}

/* TODO: optimize these two functions */
static inline void mul_vol_float(float *array, float volume, size_t count)
{
	for (size_t i = 0; i < count; i++)
		array[i] *= volume;
}

static void audio_line_place_data_pos(struct audio_line *line,
		const struct audio_data *data, size_t position)
{
	bool   planar     = line->audio->planes > 1;
	size_t total_num  = data->frames * (planar ? 1 : line->audio->channels);
	size_t total_size = data->frames * line->audio->block_size;

	for (size_t i = 0; i < line->audio->planes; i++) {
		da_copy_array(line->volume_buffers[i], data->data[i],
				total_size);

		uint8_t *array = line->volume_buffers[i].array;

		switch (line->audio->info.format) {
		case AUDIO_FORMAT_FLOAT:
		case AUDIO_FORMAT_FLOAT_PLANAR:
			mul_vol_float((float*)array, data->volume, total_num);
			break;
		default:
			blog(LOG_ERROR, "audio_line_place_data_pos: "
			                "Unsupported or unknown format");
			break;
		}

		circlebuf_place(&line->buffers[i], position,
				line->volume_buffers[i].array, total_size);
	}
}

static inline uint64_t smooth_ts(struct audio_line *line, uint64_t timestamp)
{
	if (!line->next_ts_min)
		return timestamp;

	bool ts_under = (timestamp < line->next_ts_min);
	uint64_t diff = ts_under ?
		(line->next_ts_min - timestamp) :
		(timestamp - line->next_ts_min);

#ifdef DEBUG_AUDIO
	if (diff >= TS_SMOOTHING_THRESHOLD)
		blog(LOG_DEBUG, "above TS smoothing threshold by %"PRIu64,
				diff);
#endif

	return (diff < TS_SMOOTHING_THRESHOLD) ? line->next_ts_min : timestamp;
}

static void audio_line_place_data(struct audio_line *line,
		const struct audio_data *data)
{
	size_t pos;
	uint64_t timestamp = smooth_ts(line, data->timestamp);

	pos = ts_diff_bytes(line->audio, timestamp, line->base_timestamp);
	line->next_ts_min =
		timestamp + conv_frames_to_time(line->audio, data->frames);

#ifdef DEBUG_AUDIO
	blog(LOG_DEBUG, "data->timestamp: %llu, line->base_timestamp: %llu, "
			"pos: %lu, bytes: %lu, buf size: %lu",
			timestamp, line->base_timestamp, pos,
			data->frames * line->audio->block_size,
			line->buffers[0].size);
#endif

	audio_line_place_data_pos(line, data, pos);
}

#define MAX_DELAY_NS 6000000000ULL

/* prevent insertation of data too far away from expected audio timing */
static inline bool valid_timestamp_range(struct audio_line *line, uint64_t ts)
{
	uint64_t buffer_ns = 1000000ULL * line->audio->info.buffer_ms;
	uint64_t max_ts    = line->base_timestamp + buffer_ns + MAX_DELAY_NS;

	return ts >= line->base_timestamp && ts < max_ts;
}

void audio_line_output(audio_line_t *line, const struct audio_data *data)
{
	if (!line || !data) return;

	pthread_mutex_lock(&line->mutex);

	if (!line->buffers[0].size) {
		line->base_timestamp = data->timestamp -
		                       line->audio->info.buffer_ms * 1000000;
		audio_line_place_data(line, data);

	} else if (valid_timestamp_range(line, data->timestamp)) {
		audio_line_place_data(line, data);

	} else {
		blog(LOG_DEBUG, "Bad timestamp for audio line '%s', "
		                "data->timestamp: %"PRIu64", "
		                "line->base_timestamp: %"PRIu64".  This can "
		                "sometimes happen when there's a pause in "
		                "the threads.", line->name, data->timestamp,
		                line->base_timestamp);
	}

	pthread_mutex_unlock(&line->mutex);
}

void audio_line_set_mixers(audio_line_t *line, uint32_t mixers)
{
	if (!!line)
		line->mixers = mixers;
}

uint32_t audio_line_get_mixers(audio_line_t *line)
{
	return !!line ? line->mixers : 0;
}
