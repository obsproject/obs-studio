/*
 * Copyright (c) 2023 Lain Bailey <lain@obsproject.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "decode.h"

#include "media-playback.h"
#include "media.h"
#include <libavutil/mastering_display_metadata.h>

enum AVHWDeviceType hw_priority[] = {
	AV_HWDEVICE_TYPE_CUDA,  AV_HWDEVICE_TYPE_D3D11VA, AV_HWDEVICE_TYPE_DXVA2,        AV_HWDEVICE_TYPE_VAAPI,
	AV_HWDEVICE_TYPE_VDPAU, AV_HWDEVICE_TYPE_QSV,     AV_HWDEVICE_TYPE_VIDEOTOOLBOX, AV_HWDEVICE_TYPE_NONE,
};

static bool has_hw_type(const AVCodec *c, enum AVHWDeviceType type, enum AVPixelFormat *hw_format)
{
	for (int i = 0;; i++) {
		const AVCodecHWConfig *config = avcodec_get_hw_config(c, i);
		if (!config) {
			break;
		}

		if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX && config->device_type == type) {
			*hw_format = config->pix_fmt;
			return true;
		}
	}

	return false;
}

static void init_hw_decoder(struct mp_decode *d, AVCodecContext *c)
{
	enum AVHWDeviceType *priority = hw_priority;
	AVBufferRef *hw_ctx = NULL;

	while (*priority != AV_HWDEVICE_TYPE_NONE) {
		if (has_hw_type(d->codec, *priority, &d->hw_format)) {
			int ret = av_hwdevice_ctx_create(&hw_ctx, *priority, NULL, NULL, 0);
			if (ret == 0)
				break;
		}

		priority++;
	}

	if (hw_ctx) {
		c->hw_device_ctx = av_buffer_ref(hw_ctx);
		c->opaque = d;
		d->hw_ctx = hw_ctx;
		d->hw = true;
	}
}

static int mp_open_codec(struct mp_decode *d, bool hw)
{
	AVCodecContext *c;
	int ret;

	c = avcodec_alloc_context3(d->codec);
	if (!c) {
		blog(LOG_WARNING, "MP: Failed to allocate context");
		return -1;
	}

	ret = avcodec_parameters_to_context(c, d->stream->codecpar);
	if (ret < 0)
		goto fail;

	d->hw = false;

	if (hw)
		init_hw_decoder(d, c);

	if (c->thread_count == 1 && c->codec_id != AV_CODEC_ID_PNG && c->codec_id != AV_CODEC_ID_TIFF &&
	    c->codec_id != AV_CODEC_ID_JPEG2000 && c->codec_id != AV_CODEC_ID_MPEG4 && c->codec_id != AV_CODEC_ID_WEBP)
		c->thread_count = 0;

	ret = avcodec_open2(c, d->codec, NULL);
	if (ret < 0)
		goto fail;

	d->decoder = c;
	return ret;

fail:
	avcodec_free_context(&c);
	avcodec_free_context(&d->decoder);

	return ret;
}

static uint16_t get_max_luminance(const AVStream *stream)
{
	uint32_t max_luminance = 0;

	for (int i = 0; i < stream->codecpar->nb_coded_side_data; i++) {
		const AVPacketSideData *const sd = &stream->codecpar->coded_side_data[i];
		switch (sd->type) {
		case AV_PKT_DATA_MASTERING_DISPLAY_METADATA: {
			const AVMasteringDisplayMetadata *mastering = (AVMasteringDisplayMetadata *)sd->data;
			if (mastering->has_luminance) {
				max_luminance = (uint32_t)(av_q2d(mastering->max_luminance) + 0.5);
			}

			break;
		}
		case AV_PKT_DATA_CONTENT_LIGHT_LEVEL: {
			const AVContentLightMetadata *const md = (AVContentLightMetadata *)&sd->data;
			max_luminance = md->MaxCLL;
			break;
		}
		default:
			break;
		}
	}

	return max_luminance;
}

bool mp_decode_init(mp_media_t *m, enum AVMediaType type, bool hw)
{
	struct mp_decode *d = type == AVMEDIA_TYPE_VIDEO ? &m->v : &m->a;
	enum AVCodecID id;
	AVStream *stream;
	int ret;

	memset(d, 0, sizeof(*d));
	d->m = m;
	d->audio = type == AVMEDIA_TYPE_AUDIO;

	ret = av_find_best_stream(m->fmt, type, -1, -1, NULL, 0);
	if (ret < 0)
		return false;
	stream = d->stream = m->fmt->streams[ret];
	id = stream->codecpar->codec_id;

	if (type == AVMEDIA_TYPE_VIDEO)
		d->max_luminance = get_max_luminance(stream);

	if (id == AV_CODEC_ID_VP8 || id == AV_CODEC_ID_VP9) {
		AVDictionaryEntry *tag = NULL;
		tag = av_dict_get(stream->metadata, "alpha_mode", tag, AV_DICT_IGNORE_SUFFIX);

		if (tag && strcmp(tag->value, "1") == 0) {
			char *codec = (id == AV_CODEC_ID_VP8) ? "libvpx" : "libvpx-vp9";
			d->codec = avcodec_find_decoder_by_name(codec);
		}
	}

	if (!d->codec)
		d->codec = avcodec_find_decoder(id);

	if (!d->codec) {
		blog(LOG_WARNING, "MP: Failed to find %s codec", av_get_media_type_string(type));
		return false;
	}

	ret = mp_open_codec(d, hw);
	if (ret < 0) {
		blog(LOG_WARNING, "MP: Failed to open %s decoder: %s", av_get_media_type_string(type), av_err2str(ret));
		return false;
	}

	d->sw_frame = av_frame_alloc();
	if (!d->sw_frame) {
		blog(LOG_WARNING, "MP: Failed to allocate %s frame", av_get_media_type_string(type));
		return false;
	}

	if (d->hw) {
		d->hw_frame = av_frame_alloc();
		if (!d->hw_frame) {
			blog(LOG_WARNING, "MP: Failed to allocate %s hw frame", av_get_media_type_string(type));
			return false;
		}

		d->in_frame = d->hw_frame;
	} else {
		d->in_frame = d->sw_frame;
	}

	d->orig_pkt = av_packet_alloc();
	d->pkt = av_packet_alloc();

	return true;
}

extern void mp_media_free_packet(mp_media_t *m, AVPacket *pkt);

void mp_decode_clear_packets(struct mp_decode *d)
{
	if (d->packet_pending) {
		av_packet_unref(d->orig_pkt);
		d->packet_pending = false;
	}

	while (d->packets.size) {
		AVPacket *pkt;
		deque_pop_front(&d->packets, &pkt, sizeof(pkt));
		mp_media_free_packet(d->m, pkt);
	}
}

void mp_decode_free(struct mp_decode *d)
{
	mp_decode_clear_packets(d);
	deque_free(&d->packets);

	av_packet_free(&d->pkt);
	av_packet_free(&d->orig_pkt);

	if (d->hw_frame) {
		av_frame_unref(d->hw_frame);
		av_free(d->hw_frame);
	}

	if (d->decoder)
		avcodec_free_context(&d->decoder);

	if (d->sw_frame) {
		av_frame_unref(d->sw_frame);
		av_free(d->sw_frame);
	}

	if (d->hw_ctx) {
		av_buffer_unref(&d->hw_ctx);
	}

	memset(d, 0, sizeof(*d));
}

void mp_decode_push_packet(struct mp_decode *decode, AVPacket *packet)
{
	deque_push_back(&decode->packets, &packet, sizeof(packet));
}

static inline int64_t get_estimated_duration(struct mp_decode *d, int64_t last_pts)
{
	if (d->audio) {
		return av_rescale_q(d->in_frame->nb_samples, (AVRational){1, d->in_frame->sample_rate},
				    (AVRational){1, 1000000000});
	} else {
		if (last_pts)
			return d->frame_pts - last_pts;

		if (d->last_duration)
			return d->last_duration;

		return av_rescale_q(d->decoder->time_base.num, d->decoder->time_base, (AVRational){1, 1000000000});
	}
}

static int decode_packet(struct mp_decode *d, int *got_frame)
{
	int ret;
	*got_frame = 0;

	ret = avcodec_receive_frame(d->decoder, d->in_frame);
	if (ret != 0 && ret != AVERROR(EAGAIN)) {
		if (ret == AVERROR_EOF)
			ret = 0;
		return ret;
	}

	if (ret != 0) {
		ret = avcodec_send_packet(d->decoder, d->pkt);
		if (ret != 0 && ret != AVERROR(EAGAIN)) {
			if (ret == AVERROR_EOF)
				ret = 0;
			return ret;
		}

		ret = avcodec_receive_frame(d->decoder, d->in_frame);
		if (ret != 0 && ret != AVERROR(EAGAIN)) {
			if (ret == AVERROR_EOF)
				ret = 0;
			return ret;
		}

		*got_frame = (ret == 0);
		ret = d->pkt->size;
	} else {
		ret = 0;
		*got_frame = 1;
	}

	if (*got_frame && d->hw) {
		if (d->hw_frame->format != d->hw_format) {
			d->frame = d->hw_frame;
			return ret;
		}

		av_frame_unref(d->sw_frame);

		int err = av_hwframe_transfer_data(d->sw_frame, d->hw_frame, 0);
		if (err == 0) {
			err = av_frame_copy_props(d->sw_frame, d->hw_frame);
		}
		if (err) {
			ret = 0;
			*got_frame = false;
		}
	}

	d->frame = d->sw_frame;
	return ret;
}

bool mp_decode_next(struct mp_decode *d)
{
	bool eof = d->m->eof;
	int got_frame;
	int ret;

	d->frame_ready = false;

	if (!eof && !d->packets.size)
		return true;

	while (!d->frame_ready) {
		if (!d->packet_pending) {
			if (!d->packets.size) {
				if (eof) {
					d->pkt->data = NULL;
					d->pkt->size = 0;
				} else {
					return true;
				}
			} else {
				mp_media_free_packet(d->m, d->orig_pkt);
				deque_pop_front(&d->packets, &d->orig_pkt, sizeof(d->orig_pkt));
				av_packet_ref(d->pkt, d->orig_pkt);
				d->packet_pending = true;
			}
		}

		ret = decode_packet(d, &got_frame);

		if (!got_frame && ret == 0) {
			d->eof = true;
			return true;
		}
		if (ret < 0) {
#ifdef DETAILED_DEBUG_INFO
			blog(LOG_DEBUG, "MP: decode failed: %s", av_err2str(ret));
#endif

			if (d->packet_pending) {
				av_packet_unref(d->orig_pkt);
				av_packet_unref(d->pkt);
				d->packet_pending = false;
			}
			return true;
		}

		d->frame_ready = !!got_frame;

		if (d->packet_pending) {
			if (d->pkt->size) {
				d->pkt->data += ret;
				d->pkt->size -= ret;
			}

			if (d->pkt->size <= 0) {
				av_packet_unref(d->orig_pkt);
				av_packet_unref(d->pkt);
				d->packet_pending = false;
			}
		}
	}

	if (d->frame_ready) {
		int64_t last_pts = d->frame_pts;

		if (d->in_frame->best_effort_timestamp == AV_NOPTS_VALUE)
			d->frame_pts = d->next_pts;
		else
			d->frame_pts = av_rescale_q(d->in_frame->best_effort_timestamp, d->stream->time_base,
						    (AVRational){1, 1000000000});

		int64_t duration = d->in_frame->duration;
		if (!duration)
			duration = get_estimated_duration(d, last_pts);
		else
			duration = av_rescale_q(duration, d->stream->time_base, (AVRational){1, 1000000000});

		if (d->m->speed != 100) {
			d->frame_pts = av_rescale_q(d->frame_pts, (AVRational){1, d->m->speed}, (AVRational){1, 100});
			duration = av_rescale_q(duration, (AVRational){1, d->m->speed}, (AVRational){1, 100});
		}

		d->last_duration = duration;
		d->next_pts = d->frame_pts + duration;
	}

	return true;
}

void mp_decode_flush(struct mp_decode *d)
{
	avcodec_flush_buffers(d->decoder);
	mp_decode_clear_packets(d);
	d->eof = false;
	d->frame_pts = 0;
	d->frame_ready = false;
	d->next_pts = 0;
}
