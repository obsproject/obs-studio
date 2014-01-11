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

#include "../util/threading.h"
#include "../util/darray.h"
#include "../util/circlebuf.h"
#include "../util/platform.h"

#include "audio-io.h"

/* TODO: Incomplete */

struct audio_line {
	char                       *name;

	struct audio_output        *audio;
	struct circlebuf           buffer;
	pthread_mutex_t            mutex;
	DARRAY(uint8_t)            volume_buffer;
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
	circlebuf_free(&line->buffer);
	da_free(line->volume_buffer);
	pthread_mutex_destroy(&line->mutex);
	bfree(line->name);
	bfree(line);
}

struct audio_output {
	struct audio_info          info;
	size_t                     block_size;
	size_t                     channels;
	media_t                    media;
	media_output_t             output;

	pthread_t                  thread;
	event_t                    stop_event;

	DARRAY(uint8_t)            pending_bytes;

	DARRAY(uint8_t)            mix_buffer;

	bool                       initialized;

	pthread_mutex_t            line_mutex;
	struct audio_line          *first_line;
};

static inline void audio_output_removeline(struct audio_output *audio,
		struct audio_line *line)
{
	pthread_mutex_lock(&audio->line_mutex);
	*line->prev_next = line->next;
	pthread_mutex_unlock(&audio->line_mutex);

	audio_line_destroy_data(line);
}

static inline size_t convert_to_sample_offset(audio_t audio, uint64_t offset)
{
	double audio_offset_d = (double)offset;
	audio_offset_d /= 1000000000.0;
	audio_offset_d *= (double)audio->info.samples_per_sec;

	return (size_t)audio_offset_d * audio->block_size;
}

/* ------------------------------------------------------------------------- */

static inline void clear_excess_audio_data(struct audio_line *line,
		uint64_t size)
{
	if (size > line->buffer.size)
		size = line->buffer.size;

	blog(LOG_WARNING, "Excess audio data for audio line '%s', somehow "
	                  "audio data went back in time by %llu bytes",
	                  line->name, size);

	circlebuf_pop_front(&line->buffer, NULL, (size_t)size);
}

static inline uint64_t min_uint64(uint64_t a, uint64_t b)
{
	return a < b ? a : b;
}

static inline void mix_audio_line(struct audio_output *audio,
		struct audio_line *line, size_t size, uint64_t timestamp)
{
	/* TODO: this just overwrites, handle actual mixing */
	if (!line->buffer.size) {
		if (!line->alive)
			audio_output_removeline(audio, line);
		return;
	}

	size_t time_offset = convert_to_sample_offset(audio,
			line->base_timestamp - timestamp);
	if (time_offset > size)
		return;

	size -= time_offset;

	size_t pop_size = min_uint64(size, line->buffer.size);
	circlebuf_pop_front(&line->buffer,
			audio->mix_buffer.array + time_offset,
			pop_size);
}

static void mix_audio_lines(struct audio_output *audio, uint64_t audio_time,
		uint64_t prev_time)
{
	struct audio_line *line = audio->first_line;
	uint64_t time_offset = audio_time - prev_time;
	size_t byte_offset = convert_to_sample_offset(audio, time_offset);

	da_resize(audio->mix_buffer, byte_offset);
	memset(audio->mix_buffer.array, 0, byte_offset);

	while (line) {
		struct audio_line *next = line->next;

		if (line->buffer.size && line->base_timestamp < prev_time) {
			clear_excess_audio_data(line,
					prev_time - line->base_timestamp);
			line->base_timestamp = prev_time;
		}

		mix_audio_line(audio, line, byte_offset, prev_time);
		line->base_timestamp = audio_time;
		line = next;
	}

	/* TODO - not good enough */
	/*if (audio->output)
		media_output_data(audio->output, audio->mix_buffer.array);*/
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
		mix_audio_lines(audio, audio_time, prev_time);
		prev_time = audio_time;

		pthread_mutex_unlock(&audio->line_mutex);
	}

	return NULL;
}

/* ------------------------------------------------------------------------- */

static inline bool valid_audio_params(struct audio_info *info)
{
	return info->format && info->name && info->samples_per_sec > 0 &&
	       info->speakers > 0;
}

static bool ao_add_to_media(audio_t audio)
{
	struct media_output_info oi;
	oi.obj     = audio;
	oi.connect = NULL;
	oi.format  = NULL; /* TODO */

	audio->output = media_output_create(&oi);
	if (!audio->output)
		return false;

	media_add_output(audio->media, audio->output);
	return true;
}

int audio_output_open(audio_t *audio, media_t media, struct audio_info *info)
{
	struct audio_output *out;
	pthread_mutexattr_t attr;

	if (!valid_audio_params(info))
		return AUDIO_OUTPUT_INVALIDPARAM;

	out = bmalloc(sizeof(struct audio_output));
	memset(out, 0, sizeof(struct audio_output));

	memcpy(&out->info, info, sizeof(struct audio_info));
	pthread_mutex_init_value(&out->line_mutex);
	out->media = media;
	out->channels = get_audio_channels(info->speakers);
	out->block_size = out->channels *
	                  get_audio_bytes_per_channel(info->format);

	if (pthread_mutexattr_init(&attr) != 0)
		goto fail;
	if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) != 0)
		goto fail;
	if (pthread_mutex_init(&out->line_mutex, &attr) != 0)
		goto fail;
	if (event_init(&out->stop_event, EVENT_TYPE_MANUAL) != 0)
		goto fail;
	if (!ao_add_to_media(out))
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

audio_line_t audio_output_createline(audio_t audio, const char *name)
{
	struct audio_line *line = bmalloc(sizeof(struct audio_line));
	memset(line, 0, sizeof(struct audio_line));
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

const struct audio_info *audio_output_getinfo(audio_t audio)
{
	return &audio->info;
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

	da_free(audio->mix_buffer);
	da_free(audio->pending_bytes);
	media_remove_output(audio->media, audio->output);
	media_output_destroy(audio->output);
	event_destroy(&audio->stop_event);
	pthread_mutex_destroy(&audio->line_mutex);
	bfree(audio);
}

void audio_line_destroy(struct audio_line *line)
{
	if (line) {
		if (!line->buffer.size)
			audio_output_removeline(line->audio, line);
		else
			line->alive = false;
	}
}

size_t audio_output_blocksize(audio_t audio)
{
	return audio->block_size;
}

static inline void mul_vol_u8bit(struct audio_line *line, float volume,
		size_t total_num)
{
	uint8_t *vals = line->volume_buffer.array;
	int16_t vol = (int16_t)(volume * 127.0f);

	for (size_t i = 0; i < total_num; i++) {
		int16_t val = (int16_t)(vals[i] ^ 0x80) << 8;
		vals[i] = (uint8_t)((val * vol / 127) + 128);
	}
}

static inline void mul_vol_16bit(struct audio_line *line, float volume,
		size_t total_num)
{
	uint16_t *vals = (uint16_t*)line->volume_buffer.array;
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

static inline void mul_vol_24bit(struct audio_line *line, float volume,
		size_t total_num)
{
	uint8_t *vals = line->volume_buffer.array;

	for (size_t i = 0; i < total_num; i++) {
		float val = conv_24bit_to_float(vals) * volume;
		conv_float_to_24bit(val, vals);
		vals += 3;
	}
}

static inline void mul_vol_32bit(struct audio_line *line, float volume,
		size_t total_num)
{
	int32_t *vals = (int32_t*)line->volume_buffer.array;

	for (size_t i = 0; i < total_num; i++) {
		float val = (float)vals[i] / 2147483647.0f;
		vals[i] = (int32_t)(val * volume / 2147483647.0f);
	}
}

static inline void mul_vol_float(struct audio_line *line, float volume,
		size_t total_num)
{
	float *vals = (float*)line->volume_buffer.array;

	for (size_t i = 0; i < total_num; i++)
		vals[i] *= volume;
}

static void audio_line_place_data_pos(struct audio_line *line,
		const struct audio_data *data, size_t position)
{
	size_t total_num  = data->frames * line->audio->channels;
	size_t total_size = data->frames * line->audio->block_size;

	da_copy_array(line->volume_buffer, data->data, total_size);

	switch (line->audio->info.format) {
	case AUDIO_FORMAT_U8BIT:
		mul_vol_u8bit(line, data->volume, total_num);
		break;
	case AUDIO_FORMAT_16BIT:
		mul_vol_16bit(line, data->volume, total_num);
		break;
	case AUDIO_FORMAT_32BIT:
		mul_vol_32bit(line, data->volume, total_num);
		break;
	case AUDIO_FORMAT_FLOAT:
		mul_vol_float(line, data->volume, total_num);
		break;
	case AUDIO_FORMAT_UNKNOWN:
		break;
	}

	circlebuf_place(&line->buffer, position, line->volume_buffer.array,
			total_size);
}

static inline void audio_line_place_data(struct audio_line *line,
		const struct audio_data *data)
{
	uint64_t time_offset = data->timestamp - line->base_timestamp;
	size_t pos = convert_to_sample_offset(line->audio, time_offset);

	audio_line_place_data_pos(line, data, pos);
}

void audio_line_output(audio_line_t line, const struct audio_data *data)
{
	/* TODO: prevent insertation of data too far away from expected
	 * audio timing */

	pthread_mutex_lock(&line->mutex);

	if (!line->buffer.size) {
		line->base_timestamp = data->timestamp;

		audio_line_place_data_pos(line, data, 0);
	} else {

		if (line->base_timestamp <= data->timestamp)
			audio_line_place_data(line, data);
		else
			blog(LOG_DEBUG, "Bad timestamp for audio line '%s', "
			                "data->timestamp: %llu, "
			                "line->base_timestamp: %llu.  "
			                "This can sometimes happen when "
			                "there's a pause in the threads.",
			                line->name, data->timestamp,
			                line->base_timestamp);
	}

	pthread_mutex_unlock(&line->mutex);
}
