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

#include "../util/bmem.h"
#include "audio-resampler.h"
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>

struct audio_resampler {
	struct SwrContext   *context;
	bool                opened;

	uint32_t            input_freq;
	uint64_t            input_layout;
	enum AVSampleFormat input_format;

	uint8_t             *output_buffer;
	uint64_t            output_layout;
	enum AVSampleFormat output_format;
	int                 output_size;
	uint32_t            output_ch;
	uint32_t            output_freq;
};

static inline enum AVSampleFormat convert_audio_format(enum audio_format format)
{
	switch (format) {
	case AUDIO_FORMAT_UNKNOWN: return AV_SAMPLE_FMT_S16;
	case AUDIO_FORMAT_U8BIT:   return AV_SAMPLE_FMT_U8;
	case AUDIO_FORMAT_16BIT:   return AV_SAMPLE_FMT_S16;
	case AUDIO_FORMAT_32BIT:   return AV_SAMPLE_FMT_S32;
	case AUDIO_FORMAT_FLOAT:   return AV_SAMPLE_FMT_FLT;
	}

	/* shouldn't get here */
	return AV_SAMPLE_FMT_S16;
}

static inline uint64_t convert_speaker_layout(enum speaker_layout layout)
{
	switch (layout) {
	case SPEAKERS_UNKNOWN:          return 0;
	case SPEAKERS_MONO:             return AV_CH_LAYOUT_MONO;
	case SPEAKERS_STEREO:           return AV_CH_LAYOUT_STEREO;
	case SPEAKERS_2POINT1:          return AV_CH_LAYOUT_2_1;
	case SPEAKERS_QUAD:             return AV_CH_LAYOUT_QUAD;
	case SPEAKERS_4POINT1:          return AV_CH_LAYOUT_4POINT1;
	case SPEAKERS_5POINT1:          return AV_CH_LAYOUT_5POINT1;
	case SPEAKERS_5POINT1_SURROUND: return AV_CH_LAYOUT_5POINT1_BACK;
	case SPEAKERS_7POINT1:          return AV_CH_LAYOUT_7POINT1;
	case SPEAKERS_7POINT1_SURROUND: return AV_CH_LAYOUT_7POINT1_WIDE_BACK;
	case SPEAKERS_SURROUND:         return AV_CH_LAYOUT_SURROUND;
	}

	/* shouldn't get here */
	return 0;
}

audio_resampler_t audio_resampler_create(struct resample_info *dst,
		struct resample_info *src)
{
	struct audio_resampler *rs = bmalloc(sizeof(struct audio_resampler));
	int errcode;

	rs->opened        = false;
	rs->input_freq    = src->samples_per_sec;
	rs->input_layout  = convert_speaker_layout(src->speakers);
	rs->input_format  = convert_audio_format(src->format);
	rs->output_buffer = NULL;
	rs->output_size   = 0;
	rs->output_ch     = get_audio_channels(dst->speakers);
	rs->output_freq   = dst->samples_per_sec;
	rs->output_layout = convert_speaker_layout(dst->speakers);
	rs->output_format = convert_audio_format(dst->format);

	rs->context = swr_alloc_set_opts(NULL,
		rs->output_layout, rs->output_format, dst->samples_per_sec,
		rs->input_layout,  rs->input_format,  src->samples_per_sec,
		0, NULL);

	if (!rs->context) {
		blog(LOG_ERROR, "swr_alloc_set_opts failed");
		audio_resampler_destroy(rs);
		return NULL;
	}

	errcode = swr_init(rs->context);
	if (errcode != 0) {
		blog(LOG_ERROR, "avresample_open failed: error code %d",
				errcode);
		audio_resampler_destroy(rs);
		return NULL;
	}

	return rs;
}

void audio_resampler_destroy(audio_resampler_t rs)
{
	if (rs) {
		if (rs->context)
			swr_free(&rs->context);
		if (rs->output_buffer)
			av_freep(&rs->output_buffer);

		bfree(rs);
	}
}

bool audio_resampler_resample(audio_resampler_t rs,
		 void **output, uint32_t *out_frames,
		 const void *input, uint32_t in_frames,
		 uint64_t *timestamp_offset)
{
	struct SwrContext *context = rs->context;
	int ret;
	int64_t delay = swr_get_delay(context, rs->input_freq);
	int estimated = (int)av_rescale_rnd(
			delay + (int64_t)in_frames,
			(int64_t)rs->output_freq, (int64_t)rs->input_freq,
			AV_ROUND_UP);

	*timestamp_offset = (uint64_t)swr_get_delay(context, 1000000000);

	/* resize the buffer if bigger */
	if (estimated > rs->output_size) {
		if (rs->output_buffer)
			av_freep(&rs->output_buffer);
		av_samples_alloc(&rs->output_buffer, NULL, rs->output_ch,
				estimated, rs->output_format, 0);

		rs->output_size = estimated;
	}

	ret = swr_convert(context,
			&rs->output_buffer, rs->output_size,
			(const uint8_t**)&input, in_frames);

	if (ret < 0) {
		blog(LOG_ERROR, "swr_convert failed: %d", ret);
		return false;
	}

	*output     = rs->output_buffer;
	*out_frames = (uint32_t)ret;
	return true;
}
