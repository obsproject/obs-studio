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

#include "ffmpeg-decode.h"
#include "obs-ffmpeg-compat.h"
#include <obs-avc.h>
#ifdef ENABLE_HEVC
#include <obs-hevc.h>
#endif

enum AVHWDeviceType hw_priority[] = {
	AV_HWDEVICE_TYPE_D3D11VA,
	AV_HWDEVICE_TYPE_DXVA2,
	AV_HWDEVICE_TYPE_QSV,
	AV_HWDEVICE_TYPE_NONE,
};

static bool has_hw_type(const AVCodec *c, enum AVHWDeviceType type)
{
	for (int i = 0;; i++) {
		const AVCodecHWConfig *config = avcodec_get_hw_config(c, i);
		if (!config) {
			break;
		}

		if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX && config->device_type == type)
			return true;
	}

	return false;
}

static void init_hw_decoder(struct ffmpeg_decode *d)
{
	enum AVHWDeviceType *priority = hw_priority;
	AVBufferRef *hw_ctx = NULL;

	while (*priority != AV_HWDEVICE_TYPE_NONE) {
		if (has_hw_type(d->codec, *priority)) {
			int ret = av_hwdevice_ctx_create(&hw_ctx, *priority, NULL, NULL, 0);
			if (ret == 0)
				break;
		}

		priority++;
	}

	if (hw_ctx) {
		d->hw_device_ctx = hw_ctx;
		d->decoder->hw_device_ctx = av_buffer_ref(hw_ctx);
		d->hw = true;
	}
}

int ffmpeg_decode_init(struct ffmpeg_decode *decode, enum AVCodecID id, bool use_hw)
{
	int ret;

	memset(decode, 0, sizeof(*decode));

	decode->codec = avcodec_find_decoder(id);
	if (!decode->codec)
		return -1;

	decode->decoder = avcodec_alloc_context3(decode->codec);

	decode->decoder->thread_count = 0;

	if (use_hw)
		init_hw_decoder(decode);

	ret = avcodec_open2(decode->decoder, decode->codec, NULL);
	if (ret < 0) {
		ffmpeg_decode_free(decode);
		return ret;
	}

	return 0;
}

void ffmpeg_decode_free(struct ffmpeg_decode *decode)
{
	if (decode->hw_frame)
		av_frame_free(&decode->hw_frame);

	if (decode->decoder)
		avcodec_free_context(&decode->decoder);

	if (decode->frame)
		av_frame_free(&decode->frame);

	if (decode->hw_device_ctx)
		av_buffer_unref(&decode->hw_device_ctx);

	if (decode->packet_buffer)
		bfree(decode->packet_buffer);

	memset(decode, 0, sizeof(*decode));
}

static inline enum video_format convert_pixel_format(int f)
{
	switch (f) {
	case AV_PIX_FMT_NONE:
		return VIDEO_FORMAT_NONE;
	case AV_PIX_FMT_GRAY8:
		return VIDEO_FORMAT_Y800;
	case AV_PIX_FMT_YUV420P:
	case AV_PIX_FMT_YUVJ420P:
		return VIDEO_FORMAT_I420;
	case AV_PIX_FMT_NV12:
		return VIDEO_FORMAT_NV12;
	case AV_PIX_FMT_YUYV422:
		return VIDEO_FORMAT_YUY2;
	case AV_PIX_FMT_YVYU422:
		return VIDEO_FORMAT_YVYU;
	case AV_PIX_FMT_UYVY422:
		return VIDEO_FORMAT_UYVY;
	case AV_PIX_FMT_YUV422P:
	case AV_PIX_FMT_YUVJ422P:
		return VIDEO_FORMAT_I422;
	case AV_PIX_FMT_RGBA:
		return VIDEO_FORMAT_RGBA;
	case AV_PIX_FMT_BGRA:
		return VIDEO_FORMAT_BGRA;
	case AV_PIX_FMT_YUV420P10LE:
		return VIDEO_FORMAT_I010;
	case AV_PIX_FMT_BGR0:
		return VIDEO_FORMAT_BGRX;
	case AV_PIX_FMT_P010LE:
		return VIDEO_FORMAT_P010;
	default:;
	}

	return VIDEO_FORMAT_NONE;
}

static inline enum audio_format convert_sample_format(int f)
{
	switch (f) {
	case AV_SAMPLE_FMT_U8:
		return AUDIO_FORMAT_U8BIT;
	case AV_SAMPLE_FMT_S16:
		return AUDIO_FORMAT_16BIT;
	case AV_SAMPLE_FMT_S32:
		return AUDIO_FORMAT_32BIT;
	case AV_SAMPLE_FMT_FLT:
		return AUDIO_FORMAT_FLOAT;
	case AV_SAMPLE_FMT_U8P:
		return AUDIO_FORMAT_U8BIT_PLANAR;
	case AV_SAMPLE_FMT_S16P:
		return AUDIO_FORMAT_16BIT_PLANAR;
	case AV_SAMPLE_FMT_S32P:
		return AUDIO_FORMAT_32BIT_PLANAR;
	case AV_SAMPLE_FMT_FLTP:
		return AUDIO_FORMAT_FLOAT_PLANAR;
	default:;
	}

	return AUDIO_FORMAT_UNKNOWN;
}

static inline enum speaker_layout convert_speaker_layout(uint8_t channels)
{
	switch (channels) {
	case 0:
		return SPEAKERS_UNKNOWN;
	case 1:
		return SPEAKERS_MONO;
	case 2:
		return SPEAKERS_STEREO;
	case 3:
		return SPEAKERS_2POINT1;
	case 4:
		return SPEAKERS_4POINT0;
	case 5:
		return SPEAKERS_4POINT1;
	case 6:
		return SPEAKERS_5POINT1;
	case 8:
		return SPEAKERS_7POINT1;
	default:
		return SPEAKERS_UNKNOWN;
	}
}

static inline void copy_data(struct ffmpeg_decode *decode, uint8_t *data, size_t size)
{
	size_t new_size = size + INPUT_BUFFER_PADDING_SIZE;

	if (decode->packet_size < new_size) {
		decode->packet_buffer = brealloc(decode->packet_buffer, new_size);
		decode->packet_size = new_size;
	}

	memset(decode->packet_buffer + size, 0, INPUT_BUFFER_PADDING_SIZE);
	memcpy(decode->packet_buffer, data, size);
}

bool ffmpeg_decode_audio(struct ffmpeg_decode *decode, uint8_t *data, size_t size, struct obs_source_audio *audio,
			 bool *got_output)
{
	int got_frame = false;
	int ret = 0;

	*got_output = false;

	copy_data(decode, data, size);

	if (!decode->frame) {
		decode->frame = av_frame_alloc();
		if (!decode->frame)
			return false;
	}

	if (data && size) {
		AVPacket *packet = av_packet_alloc();
		packet->data = decode->packet_buffer;
		packet->size = (int)size;

		ret = avcodec_send_packet(decode->decoder, packet);

		av_packet_free(&packet);
	}
	if (ret == 0)
		ret = avcodec_receive_frame(decode->decoder, decode->frame);

	got_frame = (ret == 0);

	if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
		ret = 0;

	if (ret < 0)
		return false;
	else if (!got_frame)
		return true;

	for (size_t i = 0; i < MAX_AV_PLANES; i++)
		audio->data[i] = decode->frame->data[i];

	audio->samples_per_sec = decode->frame->sample_rate;
	audio->format = convert_sample_format(decode->frame->format);
	audio->speakers = convert_speaker_layout((uint8_t)decode->decoder->ch_layout.nb_channels);

	audio->frames = decode->frame->nb_samples;

	if (audio->format == AUDIO_FORMAT_UNKNOWN)
		return false;

	*got_output = true;
	return true;
}

static enum video_colorspace convert_color_space(enum AVColorSpace s, enum AVColorTransferCharacteristic trc,
						 enum AVColorPrimaries color_primaries)
{
	switch (s) {
	case AVCOL_SPC_BT709:
		return (trc == AVCOL_TRC_IEC61966_2_1) ? VIDEO_CS_SRGB : VIDEO_CS_709;
	case AVCOL_SPC_FCC:
	case AVCOL_SPC_BT470BG:
	case AVCOL_SPC_SMPTE170M:
	case AVCOL_SPC_SMPTE240M:
		return VIDEO_CS_601;
	case AVCOL_SPC_BT2020_NCL:
		return (trc == AVCOL_TRC_ARIB_STD_B67) ? VIDEO_CS_2100_HLG : VIDEO_CS_2100_PQ;
	default:
		return (color_primaries == AVCOL_PRI_BT2020)
			       ? ((trc == AVCOL_TRC_ARIB_STD_B67) ? VIDEO_CS_2100_HLG : VIDEO_CS_2100_PQ)
			       : VIDEO_CS_DEFAULT;
	}
}

bool ffmpeg_decode_video(struct ffmpeg_decode *decode, uint8_t *data, size_t size, long long *ts,
			 enum video_colorspace cs, enum video_range_type range, struct obs_source_frame2 *frame,
			 bool *got_output)
{
	int got_frame = false;
	AVFrame *out_frame;
	int ret;

	*got_output = false;

	copy_data(decode, data, size);

	if (!decode->frame) {
		decode->frame = av_frame_alloc();
		if (!decode->frame)
			return false;

		if (decode->hw && !decode->hw_frame) {
			decode->hw_frame = av_frame_alloc();
			if (!decode->hw_frame)
				return false;
		}
	}

	out_frame = decode->hw ? decode->hw_frame : decode->frame;

	AVPacket *packet = av_packet_alloc();
	packet->data = decode->packet_buffer;
	packet->size = (int)size;
	packet->pts = *ts;

	switch (decode->codec->id) {
	case AV_CODEC_ID_H264:
		if (obs_avc_keyframe(data, size))
			packet->flags |= AV_PKT_FLAG_KEY;
#ifdef ENABLE_HEVC
		break;
	case AV_CODEC_ID_HEVC:
		if (obs_hevc_keyframe(data, size))
			packet->flags |= AV_PKT_FLAG_KEY;
#endif
	}

	ret = avcodec_send_packet(decode->decoder, packet);
	if (ret == 0) {
		ret = avcodec_receive_frame(decode->decoder, out_frame);
	}

	av_packet_free(&packet);

	got_frame = (ret == 0);

	if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
		ret = 0;

	if (ret < 0)
		return false;
	else if (!got_frame)
		return true;

	if (got_frame && decode->hw) {
		ret = av_hwframe_transfer_data(decode->frame, out_frame, 0);
		if (ret < 0) {
			return false;
		}
	}

	for (size_t i = 0; i < MAX_AV_PLANES; i++) {
		frame->data[i] = decode->frame->data[i];
		frame->linesize[i] = decode->frame->linesize[i];
	}

	const enum video_format format = convert_pixel_format(decode->frame->format);
	frame->format = format;

	if (range == VIDEO_RANGE_DEFAULT) {
		range = (decode->frame->color_range == AVCOL_RANGE_JPEG) ? VIDEO_RANGE_FULL : VIDEO_RANGE_PARTIAL;
	}

	if (cs == VIDEO_CS_DEFAULT) {
		cs = convert_color_space(decode->frame->colorspace, decode->frame->color_trc,
					 decode->frame->color_primaries);
	}

	const bool success = video_format_get_parameters_for_format(cs, range, format, frame->color_matrix,
								    frame->color_range_min, frame->color_range_max);
	if (!success) {
		blog(LOG_ERROR,
		     "Failed to get video format "
		     "parameters for video format %u",
		     cs);
		return false;
	}

	frame->range = range;

	*ts = decode->frame->pts;

	frame->width = decode->frame->width;
	frame->height = decode->frame->height;
	frame->flip = false;

	switch (decode->frame->color_trc) {
	case AVCOL_TRC_BT709:
	case AVCOL_TRC_GAMMA22:
	case AVCOL_TRC_GAMMA28:
	case AVCOL_TRC_SMPTE170M:
	case AVCOL_TRC_SMPTE240M:
	case AVCOL_TRC_IEC61966_2_1:
		frame->trc = VIDEO_TRC_SRGB;
		break;
	case AVCOL_TRC_SMPTE2084:
		frame->trc = VIDEO_TRC_PQ;
		break;
	case AVCOL_TRC_ARIB_STD_B67:
		frame->trc = VIDEO_TRC_HLG;
		break;
	default:
		frame->trc = VIDEO_TRC_DEFAULT;
	}

	if (frame->format == VIDEO_FORMAT_NONE)
		return false;

	*got_output = true;
	return true;
}
