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
	struct audio_output        *audio;
	struct circlebuf           buffer;
	DARRAY(uint8_t)            volume_buffer;
	uint64_t                   base_timestamp;
	uint64_t                   last_timestamp;

	/* states whether this line is still being used.  if not, then when the
	 * buffer is depleted, it's destroyed */
	bool                       alive;
};

static inline void audio_line_destroy_data(struct audio_line *line)
{
	circlebuf_free(&line->buffer);
	da_free(line->volume_buffer);
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

	bool                       initialized;

	pthread_mutex_t            line_mutex;
	DARRAY(struct audio_line*) lines;
};

static inline void audio_output_removeline(struct audio_output *audio,
		struct audio_line *line)
{
	pthread_mutex_lock(&audio->line_mutex);
	da_erase_item(audio->lines, &line);
	pthread_mutex_unlock(&audio->line_mutex);
	audio_line_destroy_data(line);
}

/* ------------------------------------------------------------------------- */

static void *audio_thread(void *param)
{
	struct audio_output *audio = param;

	while (event_try(&audio->stop_event) == EAGAIN) {
		/* TODO */
	}

	return NULL;
}

/* ------------------------------------------------------------------------- */

static inline bool valid_audio_params(struct audio_info *info)
{
	return info->format && info->name && info->samples_per_sec > 0 &&
	       info->speakers > 0;
}

static inline bool ao_add_to_media(audio_t audio)
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

	if (pthread_mutex_init(&out->line_mutex, NULL) != 0)
		goto fail;
	if (event_init(&out->stop_event, true) != 0)
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

audio_line_t audio_output_createline(audio_t audio)
{
	struct audio_line *line = bmalloc(sizeof(struct audio_line));
	memset(line, 0, sizeof(struct audio_line));
	line->alive = true;

	pthread_mutex_lock(&audio->line_mutex);
	da_push_back(audio->lines, &line);
	pthread_mutex_unlock(&audio->line_mutex);
	return line;
}

const struct audio_info *audio_output_getinfo(audio_t audio)
{
	return &audio->info;
}

void audio_output_close(audio_t audio)
{
	void *thread_ret;
	size_t i;

	if (!audio)
		return;

	if (audio->initialized) {
		event_signal(&audio->stop_event);
		pthread_join(audio->thread, &thread_ret);
	}

	for (i = 0; i < audio->lines.num; i++)
		audio_line_destroy_data(audio->lines.array[i]);

	da_free(audio->lines);
	media_remove_output(audio->media, audio->output);
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

static inline uint64_t convert_to_sample_offset(audio_t audio, uint64_t offset)
{
	return (uint64_t)((double)offset *
	                  (1000000000.0 / (double)audio->info.samples_per_sec));
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

static void audio_line_place_data(struct audio_line *line,
		const struct audio_data *data, size_t position)
{
	size_t total_size = data->frames * line->audio->block_size;
	size_t total_num  = data->frames * line->audio->channels;

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

void audio_line_output(audio_line_t line, const struct audio_data *data)
{
	if (!line->buffer.size) {
		line->base_timestamp = data->timestamp;

		audio_line_place_data(line, data, 0);
	} else {
		uint64_t position = data->timestamp - line->base_timestamp;
		position = convert_to_sample_offset(line->audio, position);
		position *= line->audio->block_size;

		audio_line_place_data(line, data, (size_t)position);
	}
}
