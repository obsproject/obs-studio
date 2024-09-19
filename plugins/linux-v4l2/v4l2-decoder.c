/*
Copyright (C) 2020 by Morten Bøgeskov <source@kosmisk.dk>

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
*/

#include <obs-module.h>
#include <linux/videodev2.h>

#include "v4l2-decoder.h"

#define blog(level, msg, ...) \
	blog(level, "v4l2-input: decoder: " msg, ##__VA_ARGS__)

static enum AVPixelFormat get_hw_format(AVCodecContext *ctx,
					const enum AVPixelFormat *pix_fmts)
{
	const struct v4l2_decoder *decoder = (struct v4l2_decoder *)ctx->opaque;

	const enum AVPixelFormat *p;

	for (p = pix_fmts; *p != -1; p++) {
		if (*p == decoder->hw_pix_fmt) {
			return *p;
		}
	}

	blog(LOG_ERROR, "Failed to get HW surface format.");
	return AV_PIX_FMT_NONE;
}

int v4l2_init_decoder(struct v4l2_decoder *decoder, int pixfmt, bool hwaccel)
{
	if (pixfmt == V4L2_PIX_FMT_MJPEG) {
		decoder->codec = avcodec_find_decoder(AV_CODEC_ID_MJPEG);
	} else if (pixfmt == V4L2_PIX_FMT_H264) {
		decoder->codec = avcodec_find_decoder(AV_CODEC_ID_H264);
	}
	if (!decoder->codec) {
		if (pixfmt == V4L2_PIX_FMT_MJPEG) {
			blog(LOG_ERROR, "failed to find MJPEG decoder");
		} else if (pixfmt == V4L2_PIX_FMT_H264) {
			blog(LOG_ERROR, "failed to find H264 decoder");
		}
		return -1;
	}

	decoder->context = avcodec_alloc_context3(decoder->codec);
	if (!decoder->context) {
		return -1;
	}

	decoder->packet = av_packet_alloc();
	if (!decoder->packet) {
		return -1;
	}

	decoder->frame = av_frame_alloc();
	if (!decoder->frame) {
		return -1;
	}

	decoder->context->flags2 |= AV_CODEC_FLAG2_FAST;
	decoder->context->opaque = decoder;

	if (hwaccel &&
	    (pixfmt == V4L2_PIX_FMT_MJPEG || pixfmt == V4L2_PIX_FMT_H264)) {
		for (int i = 0;; i++) {
			const AVCodecHWConfig *config =
				avcodec_get_hw_config(decoder->codec, i);
			if (!config) {
				blog(LOG_ERROR, "No hw decoding device");
				break;
			}

			if (config->methods &
				    AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
			    config->device_type == AV_HWDEVICE_TYPE_VAAPI) {

				AVBufferRef *hw_device_ctx = NULL;
				if (av_hwdevice_ctx_create(
					    &hw_device_ctx,
					    AV_HWDEVICE_TYPE_VAAPI, NULL, NULL,
					    0) >= 0) {
					decoder->hw_pix_fmt = config->pix_fmt;
					decoder->context->get_format =
						get_hw_format;
					decoder->context->hw_device_ctx =
						av_buffer_ref(hw_device_ctx);
					av_buffer_unref(&hw_device_ctx);
					blog(LOG_INFO,
					     "Using VAAPI decoder hw_pix_fmt: %s",
					     av_get_pix_fmt_name(
						     decoder->hw_pix_fmt));
					break;
				}
			}
		}
	}

	if (avcodec_open2(decoder->context, decoder->codec, NULL) < 0) {
		blog(LOG_ERROR, "failed to open codec");
		return -1;
	}

	blog(LOG_DEBUG, "initialized avcodec");

	return 0;
}

void v4l2_destroy_decoder(struct v4l2_decoder *decoder)
{
	blog(LOG_DEBUG, "destroying avcodec");
	if (decoder->frame) {
		av_frame_free(&decoder->frame);
	}

	if (decoder->packet) {
		av_packet_free(&decoder->packet);
	}

	if (decoder->context) {
#if LIBAVCODEC_VERSION_MAJOR < 61
		avcodec_close(decoder->context);
#endif
		avcodec_free_context(&decoder->context);
	}
}

int v4l2_decode_frame(struct obs_source_frame *out, uint8_t *data,
		      size_t length, struct v4l2_decoder *decoder)
{
	decoder->packet->data = data;
	decoder->packet->size = length;
	if (avcodec_send_packet(decoder->context, decoder->packet) < 0) {
		blog(LOG_ERROR, "failed to send frame to codec");
		return -1;
	}

	if (avcodec_receive_frame(decoder->context, decoder->frame) < 0) {
		blog(LOG_ERROR, "failed to receive frame from codec");
		return -1;
	}

	enum AVPixelFormat pix_fmt = decoder->context->pix_fmt;
	if (decoder->context->hw_device_ctx &&
	    decoder->frame->format == decoder->hw_pix_fmt) {
		AVFrame *tmpframe;
		if (!(tmpframe = av_frame_alloc())) {
			blog(LOG_ERROR, "failed to allocate frame");
			return -1;
		}
		/* NV12 is the most compatible format for downloading */
		tmpframe->format = AV_PIX_FMT_NV12;

		/* retrieve data from GPU to CPU */
		blog(LOG_DEBUG, "retrieve frame from GPU");
		if (av_hwframe_transfer_data(tmpframe, decoder->frame, 0) < 0) {
			blog(LOG_ERROR,
			     "Error transferring the data to system memory\n");
			av_frame_free(&tmpframe);
			return -1;
		}
		av_frame_free(&decoder->frame);
		decoder->frame = tmpframe;
		pix_fmt = tmpframe->format;
	}

	for (uint_fast32_t i = 0; i < MAX_AV_PLANES; ++i) {
		out->data[i] = decoder->frame->data[i];
		out->linesize[i] = decoder->frame->linesize[i];
	}

	switch (pix_fmt) {
	case AV_PIX_FMT_GRAY8:
		out->format = VIDEO_FORMAT_Y800;
		break;
	case AV_PIX_FMT_YUVJ422P:
	case AV_PIX_FMT_YUV422P:
		out->format = VIDEO_FORMAT_I422;
		break;
	case AV_PIX_FMT_YUVJ420P:
	case AV_PIX_FMT_YUV420P:
		out->format = VIDEO_FORMAT_I420;
		break;
	case AV_PIX_FMT_YUVJ444P:
	case AV_PIX_FMT_YUV444P:
		out->format = VIDEO_FORMAT_I444;
		break;
	case AV_PIX_FMT_NV12:
		out->format = VIDEO_FORMAT_NV12;
		break;
	default:
		break;
	}

	return 0;
}
