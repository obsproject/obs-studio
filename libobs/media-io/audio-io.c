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
	audio_resampler_t         resampler;

	void (*callback)(void *param, const struct audio_data *data);
	void *param;
};

static inline audio_input_free(struct audio_input *input)
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

struct audio_output {
	struct audio_output_info   info;
	size_t                     block_size;
	size_t                     channels;
	size_t                     planes;

	pthread_t                  thread;
	event_t                    stop_event;

	DARRAY(uint8_t)            mix_buffers[MAX_AV_PLANES];

	bool                       initialized;

	pthread_mutex_t            line_mutex;
	struct audio_line          *first_line;

	pthread_mutex_t            input_mutex;
	DARRAY(struct audio_input) inputs;
};

static inline void audio_output_removeline(struct audio_output *audio,
		struct audio_line *line)
{
	pthread_mutex_lock(&audio->line_mutex);
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

static inline double ts_to_frames(audio_t audio, uint64_t ts)
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

static size_t ts_diff_frames(audio_t audio, uint64_t ts1, uint64_t ts2)
{
	double diff = ts_to_frames(audio, ts1) - ts_to_frames(audio, ts2);
	return (size_t)positive_round(diff);
}

static size_t ts_diff_bytes(audio_t audio, uint64_t ts1, uint64_t ts2)
{
	return ts_diff_frames(audio, ts1, ts2) * audio->block_size;
}

/* unless the value is 3+ hours worth of frames, this won't overflow */
static inline uint64_t conv_frames_to_time(audio_t audio, uint32_t frames)
{
	return (uint64_t)frames * 1000000000ULL /
		(uint64_t)audio->info.samples_per_sec;
}

/* ------------------------------------------------------------------------- */

static inline void clear_excess_audio_data(struct audio_line *line,
		uint64_t prev_time)
{
	size_t size = ts_diff_bytes(line->audio, prev_time,
			line->base_timestamp);

	blog(LOG_WARNING, "Excess audio data for audio line '%s', somehow "
	                  "audio data went back in time by %"PRIu32" bytes.  "
	                  "prev_time: %"PRIu64", line->base_timestamp: %"PRIu64,
	                  line->name, (uint32_t)size,
	                  prev_time, line->base_timestamp);

	for (size_t i = 0; i < line->audio->planes; i++) {
		size_t clear_size = (size > line->buffers[i].size) ?
			(size_t)size : line->buffers[i].size;

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

#define MIN_S8  -128
#define MAX_S8   127
#define MIN_S16 -32767
#define MAX_S16  32767
#define MIN_S32 -2147483647
#define MAX_S32  2147483647

#define MIX_BUFFER_SIZE 256

/* TODO: optimize mixing */
static void mix_u8(uint8_t *mix, struct circlebuf *buf, size_t size)
{
	uint8_t  vals[MIX_BUFFER_SIZE];
	register int16_t mix_val;

	while (size) {
		size_t pop_count = min_size(size, sizeof(vals));
		size -= pop_count;

		circlebuf_pop_front(buf, vals, pop_count);

		for (size_t i = 0; i < pop_count; i++) {
			mix_val =  (int16_t)*mix - 128;
			mix_val += (int16_t)vals[i] - 128;
			mix_val = CLAMP(mix_val, MIN_S8, MAX_S8) + 128;
			*(mix++) = (uint8_t)mix_val;
		}
	}
}

static void mix_s16(uint8_t *mix_in, struct circlebuf *buf, size_t size)
{
	int16_t *mix = (int16_t*)mix_in;
	int16_t vals[MIX_BUFFER_SIZE];
	register int32_t mix_val;

	while (size) {
		size_t pop_count = min_size(size, sizeof(vals));
		size -= pop_count;

		circlebuf_pop_front(buf, vals, pop_count);
		pop_count /= sizeof(int16_t);

		for (size_t i = 0; i < pop_count; i++) {
			mix_val =  (int32_t)*mix;
			mix_val += (int32_t)vals[i];
			*(mix++) = (int16_t)CLAMP(mix_val, MIN_S16, MAX_S16);
		}
	}
}

static void mix_s32(uint8_t *mix_in, struct circlebuf *buf, size_t size)
{
	int32_t *mix = (int32_t*)mix_in;
	int32_t vals[MIX_BUFFER_SIZE];
	register int64_t mix_val;

	while (size) {
		size_t pop_count = min_size(size, sizeof(vals));
		size -= pop_count;

		circlebuf_pop_front(buf, vals, pop_count);
		pop_count /= sizeof(int32_t);

		for (size_t i = 0; i < pop_count; i++) {
			mix_val =  (int64_t)*mix;
			mix_val += (int64_t)vals[i];
			*(mix++) = (int32_t)CLAMP(mix_val, MIN_S32, MAX_S32);
		}
	}
}

static void mix_float(uint8_t *mix_in, struct circlebuf *buf, size_t size)
{
	float *mix = (float*)mix_in;
	float vals[MIX_BUFFER_SIZE];
	register float mix_val;

	while (size) {
		size_t pop_count = min_size(size, sizeof(vals));
		size -= pop_count;

		circlebuf_pop_front(buf, vals, pop_count);
		pop_count /= sizeof(float);

		for (size_t i = 0; i < pop_count; i++) {
			mix_val =  *mix + vals[i];
			*(mix++) = CLAMP(mix_val, -1.0f, 1.0f);
		}
	}
}

static inline void mix_audio(enum audio_format format,
		uint8_t *mix, struct circlebuf *buf, size_t size)
{
	switch (format) {
	case AUDIO_FORMAT_UNKNOWN:
		break;

	case AUDIO_FORMAT_U8BIT:
	case AUDIO_FORMAT_U8BIT_PLANAR:
		mix_u8(mix, buf, size); break;

	case AUDIO_FORMAT_16BIT:
	case AUDIO_FORMAT_16BIT_PLANAR:
		mix_s16(mix, buf, size); break;

	case AUDIO_FORMAT_32BIT:
	case AUDIO_FORMAT_32BIT_PLANAR:
		mix_s32(mix, buf, size); break;

	case AUDIO_FORMAT_FLOAT:
	case AUDIO_FORMAT_FLOAT_PLANAR:
		mix_float(mix, buf, size); break;
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

		mix_audio(audio->info.format,
				audio->mix_buffers[i].array + time_offset,
				&line->buffers[i], pop_size);
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
				data->data, data->frames);

		for (size_t i = 0; i < MAX_AV_PLANES; i++)
			data->data[i] = output[i];
		data->frames     = frames;
		data->timestamp -= offset;
	}

	return success;
}

static inline void do_audio_output(struct audio_output *audio,
		uint64_t timestamp, uint32_t frames)
{
	struct audio_data data;
	for (size_t i = 0; i < MAX_AV_PLANES; i++)
		data.data[i] = audio->mix_buffers[i].array;
	data.frames = frames;
	data.timestamp = timestamp;
	data.volume = 1.0f;

	pthread_mutex_lock(&audio->input_mutex);

	for (size_t i = 0; i < audio->inputs.num; i++) {
		struct audio_input *input = audio->inputs.array+i;

		if (resample_audio_output(input, &data))
			input->callback(input->param, &data);
	}

	pthread_mutex_unlock(&audio->input_mutex);
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
	for (size_t i = 0; i < audio->planes; i++) {
		da_resize(audio->mix_buffers[i], bytes);
		memset(audio->mix_buffers[i].array, 0, bytes);
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

	/* output */
	do_audio_output(audio, prev_time, frames);

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

	while (event_try(&audio->stop_event) == EAGAIN) {
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

static size_t audio_get_input_idx(audio_t video,
		void (*callback)(void *param, const struct audio_data *data),
		void *param)
{
	for (size_t i = 0; i < video->inputs.num; i++) {
		struct audio_input *input = video->inputs.array+i;
		if (input->callback == callback && input->param == param)
			return i;
	}

	return DARRAY_INVALID;
}

static inline void audio_input_init(struct audio_input *input,
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
	} else {
		input->resampler = NULL;
	}
}

void audio_output_connect(audio_t audio,
		struct audio_convert_info *conversion,
		void (*callback)(void *param, const struct audio_data *data),
		void *param)
{
	pthread_mutex_lock(&audio->input_mutex);

	if (audio_get_input_idx(audio, callback, param) == DARRAY_INVALID) {
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

		audio_input_init(&input, audio);
		da_push_back(audio->inputs, &input);
	}

	pthread_mutex_unlock(&audio->input_mutex);
}

void audio_output_disconnect(audio_t audio,
		void (*callback)(void *param, const struct audio_data *data),
		void *param)
{
	pthread_mutex_lock(&audio->input_mutex);

	size_t idx = audio_get_input_idx(audio, callback, param);
	if (idx != DARRAY_INVALID) {
		audio_input_free(audio->inputs.array+idx);
		da_erase(audio->inputs, idx);
	}

	pthread_mutex_unlock(&audio->input_mutex);
}

static inline bool valid_audio_params(struct audio_output_info *info)
{
	return info->format && info->name && info->samples_per_sec > 0 &&
	       info->speakers > 0;
}

int audio_output_open(audio_t *audio, struct audio_output_info *info)
{
	struct audio_output *out;
	pthread_mutexattr_t attr;
	bool planar = is_audio_planar(info->format);

	if (!valid_audio_params(info))
		return AUDIO_OUTPUT_INVALIDPARAM;

	out = bzalloc(sizeof(struct audio_output));

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
	if (pthread_mutex_init(&out->input_mutex, NULL) != 0)
		goto fail;
	if (event_init(&out->stop_event, EVENT_TYPE_MANUAL) != 0)
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

void audio_output_close(audio_t audio)
{
	void *thread_ret;
	struct audio_line *line;

	if (!audio)
		return;

	if (audio->initialized) {
		event_signal(&audio->stop_event);
		pthread_join(audio->thread, &thread_ret);
	}

	line = audio->first_line;
	while (line) {
		struct audio_line *next = line->next;
		audio_line_destroy_data(line);
		line = next;
	}

	for (size_t i = 0; i < audio->inputs.num; i++)
		audio_input_free(audio->inputs.array+i);

	for (size_t i = 0; i < MAX_AV_PLANES; i++)
		da_free(audio->mix_buffers[i]);

	da_free(audio->inputs);
	event_destroy(&audio->stop_event);
	pthread_mutex_destroy(&audio->line_mutex);
	bfree(audio);
}

audio_line_t audio_output_createline(audio_t audio, const char *name)
{
	struct audio_line *line = bzalloc(sizeof(struct audio_line));
	line->alive = true;
	line->audio = audio;

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

const struct audio_output_info *audio_output_getinfo(audio_t audio)
{
	return &audio->info;
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

size_t audio_output_blocksize(audio_t audio)
{
	return audio->block_size;
}

size_t audio_output_planes(audio_t audio)
{
	return audio->planes;
}

size_t audio_output_channels(audio_t audio)
{
	return audio->channels;
}

/* TODO: Optimization of volume multiplication functions */

static inline void mul_vol_u8bit(void *array, float volume, size_t total_num)
{
	uint8_t *vals = array;
	int16_t vol = (int16_t)(volume * 127.0f);

	for (size_t i = 0; i < total_num; i++) {
		int16_t val = (int16_t)(vals[i] ^ 0x80) << 8;
		vals[i] = (uint8_t)((val * vol / 127) + 128);
	}
}

static inline void mul_vol_16bit(void *array, float volume, size_t total_num)
{
	uint16_t *vals = array;
	int32_t vol = (int32_t)(volume * 32767.0f);

	for (size_t i = 0; i < total_num; i++)
		vals[i] = (int32_t)((int32_t)vals[i] * vol / 32767);
}

static inline float conv_24bit_to_float(uint8_t *vals)
{
	int32_t val = ((int32_t)vals[0]) |
	              ((int32_t)vals[1] << 8) |
	              ((int32_t)vals[2] << 16);

	if ((val & 0x800000) != 0)
		val |= 0xFF000000;

	return (float)val / 8388607.0f;
}

static inline void conv_float_to_24bit(float fval, uint8_t *vals)
{
	int32_t val = (int32_t)(fval * 8388607.0f);
	vals[0] = (val)       & 0xFF;
	vals[1] = (val >> 8)  & 0xFF;
	vals[2] = (val >> 16) & 0xFF;
}

static inline void mul_vol_24bit(void *array, float volume, size_t total_num)
{
	uint8_t *vals = array;

	for (size_t i = 0; i < total_num; i++) {
		float val = conv_24bit_to_float(vals) * volume;
		conv_float_to_24bit(val, vals);
		vals += 3;
	}
}

static inline void mul_vol_32bit(void *array, float volume, size_t total_num)
{
	int32_t *vals = array;

	for (size_t i = 0; i < total_num; i++) {
		float val = (float)vals[i] / 2147483647.0f;
		vals[i] = (int32_t)(val * volume / 2147483647.0f);
	}
}

static inline void mul_vol_float(void *array, float volume, size_t total_num)
{
	float *vals = array;

	for (size_t i = 0; i < total_num; i++)
		vals[i] *= volume;
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
		case AUDIO_FORMAT_U8BIT:
		case AUDIO_FORMAT_U8BIT_PLANAR:
			mul_vol_u8bit(array, data->volume, total_num);
			break;
		case AUDIO_FORMAT_16BIT:
		case AUDIO_FORMAT_16BIT_PLANAR:
			mul_vol_16bit(array, data->volume, total_num);
			break;
		case AUDIO_FORMAT_32BIT:
		case AUDIO_FORMAT_32BIT_PLANAR:
			mul_vol_32bit(array, data->volume, total_num);
			break;
		case AUDIO_FORMAT_FLOAT:
		case AUDIO_FORMAT_FLOAT_PLANAR:
			mul_vol_float(array, data->volume, total_num);
			break;
		case AUDIO_FORMAT_UNKNOWN:
			blog(LOG_ERROR, "audio_line_place_data_pos: "
					"Unknown format");
			break;
		}

		circlebuf_place(&line->buffers[i], position,
				line->volume_buffers[i].array, total_size);
	}
}

void audio_line_place_data(struct audio_line *line,
		const struct audio_data *data)
{
	size_t pos = ts_diff_bytes(line->audio, data->timestamp,
			line->base_timestamp);

#ifdef DEBUG_AUDIO
	blog(LOG_DEBUG, "data->timestamp: %llu, line->base_timestamp: %llu, "
			"pos: %lu, bytes: %lu, buf size: %lu",
			data->timestamp, line->base_timestamp, pos,
			data->frames * line->audio->block_size,
			line->buffers[0].size);
#endif

	audio_line_place_data_pos(line, data, pos);
}

void audio_line_output(audio_line_t line, const struct audio_data *data)
{
	/* TODO: prevent insertation of data too far away from expected
	 * audio timing */

	pthread_mutex_lock(&line->mutex);

	if (!line->buffers[0].size) {
		/* XXX: not entirely sure if this is the wisest course of
		 * action in all circumstances */
		line->base_timestamp = data->timestamp -
		                       line->audio->info.buffer_ms * 1000000;
		audio_line_place_data(line, data);

	} else if (line->base_timestamp <= data->timestamp) {
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
