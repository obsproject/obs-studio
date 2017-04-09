/******************************************************************************
    Copyright (C) 2014 by Hugh Bailey <obs.jim@gmail.com>

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

#include "ffmpeg-decode.h"
#include <obs-avc.h>

int ffmpeg_decode_init(struct ffmpeg_decode *decode, enum AVCodecID id)
{
	int ret;

	avcodec_register_all();
	memset(decode, 0, sizeof(*decode));

	decode->codec = avcodec_find_decoder(id);
	if (!decode->codec)
		return -1;

	decode->decoder = avcodec_alloc_context3(decode->codec);

	ret = avcodec_open2(decode->decoder, decode->codec, NULL);
	if (ret < 0) {
		ffmpeg_decode_free(decode);
		return ret;
	}

	if (decode->codec->capabilities & CODEC_CAP_TRUNCATED)
		decode->decoder->flags |= CODEC_FLAG_TRUNCATED;

	return 0;
}

void ffmpeg_decode_free(struct ffmpeg_decode *decode)
{
	if (decode->decoder) {
		avcodec_close(decode->decoder);
		av_free(decode->decoder);
	}

	if (decode->frame)
		av_free(decode->frame);

	if (decode->packet_buffer)
		bfree(decode->packet_buffer);

	memset(decode, 0, sizeof(*decode));
}

static inline enum video_format convert_pixel_format(int f)
{
	switch (f) {
	case AV_PIX_FMT_NONE:    return VIDEO_FORMAT_NONE;
	case AV_PIX_FMT_YUV420P: return VIDEO_FORMAT_I420;
	case AV_PIX_FMT_NV12:    return VIDEO_FORMAT_NV12;
	case AV_PIX_FMT_YUYV422: return VIDEO_FORMAT_YUY2;
	case AV_PIX_FMT_UYVY422: return VIDEO_FORMAT_UYVY;
	case AV_PIX_FMT_RGBA:    return VIDEO_FORMAT_RGBA;
	case AV_PIX_FMT_BGRA:    return VIDEO_FORMAT_BGRA;
	case AV_PIX_FMT_BGR0:    return VIDEO_FORMAT_BGRX;
	default:;
	}

	return VIDEO_FORMAT_NONE;
}

static inline enum audio_format convert_sample_format(int f)
{
	switch (f) {
	case AV_SAMPLE_FMT_U8:   return AUDIO_FORMAT_U8BIT;
	case AV_SAMPLE_FMT_S16:  return AUDIO_FORMAT_16BIT;
	case AV_SAMPLE_FMT_S32:  return AUDIO_FORMAT_32BIT;
	case AV_SAMPLE_FMT_FLT:  return AUDIO_FORMAT_FLOAT;
	case AV_SAMPLE_FMT_U8P:  return AUDIO_FORMAT_U8BIT_PLANAR;
	case AV_SAMPLE_FMT_S16P: return AUDIO_FORMAT_16BIT_PLANAR;
	case AV_SAMPLE_FMT_S32P: return AUDIO_FORMAT_32BIT_PLANAR;
	case AV_SAMPLE_FMT_FLTP: return AUDIO_FORMAT_FLOAT_PLANAR;
	default:;
	}

	return AUDIO_FORMAT_UNKNOWN;
}

static inline void copy_data(struct ffmpeg_decode *decode, uint8_t *data,
		size_t size)
{
	size_t new_size = size + FF_INPUT_BUFFER_PADDING_SIZE;

	if (decode->packet_size < new_size) {
		decode->packet_buffer = brealloc(decode->packet_buffer,
				new_size);
		decode->packet_size   = new_size;
	}

	memset(decode->packet_buffer + size, 0, FF_INPUT_BUFFER_PADDING_SIZE);
	memcpy(decode->packet_buffer, data, size);
}

int ffmpeg_decode_audio(struct ffmpeg_decode *decode,
		uint8_t *data, size_t size,
		struct obs_source_audio *audio,
		bool *got_output)
{
	AVPacket packet = {0};
	int got_frame = false;
	int len;

	*got_output = false;

	copy_data(decode, data, size);

	av_init_packet(&packet);
	packet.data = decode->packet_buffer;
	packet.size = (int)size;

	if (!decode->frame) {
		decode->frame = av_frame_alloc();
		if (!decode->frame)
			return -1;
	}

	len = avcodec_decode_audio4(decode->decoder, decode->frame, &got_frame,
			&packet);

	if (len <= 0 || !got_frame)
		return len;

	for (size_t i = 0; i < MAX_AV_PLANES; i++)
		audio->data[i] = decode->frame->data[i];

	audio->samples_per_sec = decode->frame->sample_rate;
	audio->speakers        = (enum speaker_layout)decode->decoder->channels;
	audio->format          = convert_sample_format(decode->frame->format);

	audio->frames = decode->frame->nb_samples;

	if (audio->format == AUDIO_FORMAT_UNKNOWN)
		return 0;

	*got_output = true;
	return len;
}

int ffmpeg_decode_video(struct ffmpeg_decode *decode,
		uint8_t *data, size_t size, long long *ts,
		struct obs_source_frame *frame,
		bool *got_output)
{
	AVPacket packet = {0};
	int got_frame = false;
	enum video_format new_format;
	int len;

	*got_output = false;

	copy_data(decode, data, size);

	av_init_packet(&packet);
	packet.data     = decode->packet_buffer;
	packet.size     = (int)size;
	packet.pts      = *ts;

	if (decode->codec->id == AV_CODEC_ID_H264 &&
			obs_avc_keyframe(data, size))
		packet.flags |= AV_PKT_FLAG_KEY;

	if (!decode->frame) {
		decode->frame = av_frame_alloc();
		if (!decode->frame)
			return -1;
	}

	len = avcodec_decode_video2(decode->decoder, decode->frame, &got_frame,
			&packet);

	if (len <= 0 || !got_frame)
		return len;

	for (size_t i = 0; i < MAX_AV_PLANES; i++) {
		frame->data[i]     = decode->frame->data[i];
		frame->linesize[i] = decode->frame->linesize[i];
	}

	new_format = convert_pixel_format(decode->frame->format);
	if (new_format != frame->format) {
		bool success;
		enum video_range_type range;

		frame->format = new_format;
		frame->full_range =
			decode->frame->color_range == AVCOL_RANGE_JPEG;

		range = frame->full_range ?
			VIDEO_RANGE_FULL : VIDEO_RANGE_PARTIAL;

		success = video_format_get_parameters(VIDEO_CS_601,
				range, frame->color_matrix,
				frame->color_range_min, frame->color_range_max);
		if (!success) {
			blog(LOG_ERROR, "Failed to get video format "
			                "parameters for video format %u",
			                VIDEO_CS_601);
			return 0;
		}
	}

	*ts = decode->frame->pkt_pts;

	frame->width  = decode->frame->width;
	frame->height = decode->frame->height;
	frame->flip   = false;

	if (frame->format == VIDEO_FORMAT_NONE)
		return 0;

	*got_output = true;
	return len;
}
