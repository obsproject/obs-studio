/******************************************************************************
    Copyright (C) 2016 by Hugh Bailey <obs.jim@gmail.com>

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

#include <util/platform.h>
#include <util/darray.h>
#include <util/dstr.h>
#include <util/base.h>
#include <media-io/video-io.h>
#include <opts-parser.h>
#include <obs-module.h>

#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libavformat/avformat.h>

#include "obs-ffmpeg-formats.h"

#define do_log(level, format, ...)                 \
	blog(level, "[AV1 encoder: '%s'] " format, \
	     obs_encoder_get_name(enc->encoder), ##__VA_ARGS__)

#define error(format, ...) do_log(LOG_ERROR, format, ##__VA_ARGS__)
#define warn(format, ...) do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...) do_log(LOG_INFO, format, ##__VA_ARGS__)
#define debug(format, ...) do_log(LOG_DEBUG, format, ##__VA_ARGS__)

struct av1_encoder {
	obs_encoder_t *encoder;
	const char *enc_name;
	bool svtav1;

	AVCodec *avcodec;
	AVCodecContext *context;
	int64_t start_ts;
	bool first_packet;

	AVFrame *vframe;

	DARRAY(uint8_t) buffer;
	DARRAY(uint8_t) header;

	int height;
	bool initialized;
};

static const char *aom_av1_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "AOM AV1";
}

static const char *svt_av1_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "SVT-AV1";
}

static void av1_video_info(void *data, struct video_scale_info *info)
{
	UNUSED_PARAMETER(data);

	switch (info->format) {
	case VIDEO_FORMAT_I010:
	case VIDEO_FORMAT_P010:
		info->format = VIDEO_FORMAT_I010;
		break;
	default:
		info->format = VIDEO_FORMAT_I420;
	}
}

static bool av1_init_codec(struct av1_encoder *enc)
{
	int ret;

	enc->context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	ret = avcodec_open2(enc->context, enc->avcodec, NULL);
	if (ret < 0) {
		if (!obs_encoder_get_last_error(enc->encoder)) {
			struct dstr error_message = {0};

			dstr_copy(&error_message,
				  obs_module_text("Encoder.Error"));
			dstr_replace(&error_message, "%1", enc->enc_name);
			dstr_replace(&error_message, "%2", av_err2str(ret));
			dstr_cat(&error_message, "\r\n\r\n");

			obs_encoder_set_last_error(enc->encoder,
						   error_message.array);
			dstr_free(&error_message);
		}
		warn("Failed to open %s: %s", enc->enc_name, av_err2str(ret));
		return false;
	}

	enc->vframe = av_frame_alloc();
	if (!enc->vframe) {
		warn("Failed to allocate video frame");
		return false;
	}

	enc->vframe->format = enc->context->pix_fmt;
	enc->vframe->width = enc->context->width;
	enc->vframe->height = enc->context->height;
	enc->vframe->colorspace = enc->context->colorspace;
	enc->vframe->color_range = enc->context->color_range;

	ret = av_frame_get_buffer(enc->vframe, base_get_alignment());
	if (ret < 0) {
		warn("Failed to allocate vframe: %s", av_err2str(ret));
		return false;
	}

	enc->initialized = true;
	return true;
}

enum RC_MODE { RC_MODE_CBR, RC_MODE_VBR, RC_MODE_CQP, RC_MODE_LOSSLESS };

static bool av1_update(struct av1_encoder *enc, obs_data_t *settings)
{
	const char *rc = obs_data_get_string(settings, "rate_control");
	int bitrate = (int)obs_data_get_int(settings, "bitrate");
	int cqp = (int)obs_data_get_int(settings, "cqp");
	int keyint_sec = (int)obs_data_get_int(settings, "keyint_sec");
	int preset = (int)obs_data_get_int(settings, "preset");

	video_t *video = obs_encoder_video(enc->encoder);
	const struct video_output_info *voi = video_output_get_info(video);
	struct video_scale_info info;

	info.format = voi->format;
	info.colorspace = voi->colorspace;
	info.range = voi->range;

	enc->context->thread_count = 0;

	av1_video_info(enc, &info);
	av_opt_set_int(enc->context->priv_data,
		       enc->svtav1 ? "preset" : "cpu-used", preset, 0);
	if (!enc->svtav1) {
		av_opt_set(enc->context->priv_data, "usage", "realtime", 0);
#if 0
		av_opt_set_int(enc->context->priv_data, "tile-columns", 4, 0);
		//av_opt_set_int(enc->context->priv_data, "tile-rows", 4, 0);
#else
		av_opt_set_int(enc->context->priv_data, "tile-columns", 2, 0);
		av_opt_set_int(enc->context->priv_data, "tile-rows", 2, 0);
#endif
		av_opt_set_int(enc->context->priv_data, "row-mt", 1, 0);
	}

	if (enc->svtav1)
		av_opt_set(enc->context->priv_data, "rc", "vbr", 0);

	if (astrcmpi(rc, "cqp") == 0) {
		bitrate = 0;
		enc->context->global_quality = cqp;

		if (enc->svtav1)
			av_opt_set(enc->context->priv_data, "rc", "cqp", 0);

	} else if (astrcmpi(rc, "vbr") != 0) { /* CBR by default */
		const int64_t rate = bitrate * INT64_C(1000);
		enc->context->rc_max_rate = rate;
		enc->context->rc_min_rate = rate;
		cqp = 0;

		if (enc->svtav1)
			av_opt_set(enc->context->priv_data, "rc", "cvbr", 0);
	}

	const int rate = bitrate * 1000;
	enc->context->bit_rate = rate;
	enc->context->rc_buffer_size = rate;
	enc->context->width = obs_encoder_get_width(enc->encoder);
	enc->context->height = obs_encoder_get_height(enc->encoder);
	enc->context->time_base = (AVRational){voi->fps_den, voi->fps_num};
	enc->context->pix_fmt = obs_to_ffmpeg_video_format(info.format);
	enc->context->color_range = info.range == VIDEO_RANGE_FULL
					    ? AVCOL_RANGE_JPEG
					    : AVCOL_RANGE_MPEG;

	switch (info.colorspace) {
	case VIDEO_CS_601:
		enc->context->color_primaries = AVCOL_PRI_SMPTE170M;
		enc->context->color_trc = AVCOL_TRC_SMPTE170M;
		enc->context->colorspace = AVCOL_SPC_SMPTE170M;
		break;
	case VIDEO_CS_DEFAULT:
	case VIDEO_CS_709:
		enc->context->color_primaries = AVCOL_PRI_BT709;
		enc->context->color_trc = AVCOL_TRC_BT709;
		enc->context->colorspace = AVCOL_SPC_BT709;
		break;
	case VIDEO_CS_SRGB:
		enc->context->color_primaries = AVCOL_PRI_BT709;
		enc->context->color_trc = AVCOL_TRC_IEC61966_2_1;
		enc->context->colorspace = AVCOL_SPC_BT709;
		break;
	case VIDEO_CS_2100_PQ:
		enc->context->color_primaries = AVCOL_PRI_BT2020;
		enc->context->color_trc = AVCOL_TRC_SMPTE2084;
		enc->context->colorspace = AVCOL_SPC_BT2020_NCL;
		break;
	case VIDEO_CS_2100_HLG:
		enc->context->color_primaries = AVCOL_PRI_BT2020;
		enc->context->color_trc = AVCOL_TRC_ARIB_STD_B67;
		enc->context->colorspace = AVCOL_SPC_BT2020_NCL;
	}

	if (keyint_sec)
		enc->context->gop_size =
			keyint_sec * voi->fps_num / voi->fps_den;

	enc->height = enc->context->height;

	const char *ffmpeg_opts = obs_data_get_string(settings, "ffmpeg_opts");
	struct obs_options opts = obs_parse_options(ffmpeg_opts);

	for (size_t i = 0; i < opts.count; i++) {
		struct obs_option *opt = &opts.options[i];
		av_opt_set(enc->context->priv_data, opt->name, opt->value, 0);
	}

	obs_free_options(opts);

	info("settings:\n"
	     "\tencoder:      %s\n"
	     "\trate_control: %s\n"
	     "\tbitrate:      %d\n"
	     "\tcqp:          %d\n"
	     "\tkeyint:       %d\n"
	     "\tpreset:       %d\n"
	     "\twidth:        %d\n"
	     "\theight:       %d\n"
	     "\tffmpeg opts:  %s\n",
	     enc->enc_name, rc, bitrate, cqp, enc->context->gop_size, preset,
	     enc->context->width, enc->context->height, ffmpeg_opts);

	return av1_init_codec(enc);
}

static void av1_destroy(void *data)
{
	struct av1_encoder *enc = data;

	if (enc->initialized) {
		AVPacket pkt = {0};
		int r_pkt = 1;

		/* flush remaining data */
		avcodec_send_frame(enc->context, NULL);

		while (r_pkt) {
			if (avcodec_receive_packet(enc->context, &pkt) < 0)
				break;

			if (r_pkt)
				av_packet_unref(&pkt);
		}
	}

	avcodec_free_context(&enc->context);
	av_frame_unref(enc->vframe);
	av_frame_free(&enc->vframe);
	da_free(enc->buffer);
	da_free(enc->header);

	bfree(enc);
}

static void *av1_create_internal(obs_data_t *settings, obs_encoder_t *encoder,
				 const char *enc_lib, const char *enc_name)
{
	struct av1_encoder *enc;

	enc = bzalloc(sizeof(*enc));
	enc->encoder = encoder;
	enc->avcodec = avcodec_find_encoder_by_name(enc_lib);
	enc->enc_name = enc_name;
	if (strcmp(enc_lib, "libsvtav1") == 0)
		enc->svtav1 = true;
	enc->first_packet = true;

	blog(LOG_INFO, "---------------------------------");

	if (!enc->avcodec) {
		obs_encoder_set_last_error(encoder,
					   "Couldn't find AV1 encoder");
		warn("Couldn't find AV1 encoder");
		goto fail;
	}

	enc->context = avcodec_alloc_context3(enc->avcodec);
	if (!enc->context) {
		warn("Failed to create codec context");
		goto fail;
	}

	if (!av1_update(enc, settings))
		goto fail;

	return enc;

fail:
	av1_destroy(enc);
	return NULL;
}

static void *svt_av1_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	return av1_create_internal(settings, encoder, "libsvtav1", "SVT-AV1");
}

static void *aom_av1_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	return av1_create_internal(settings, encoder, "libaom-av1", "AOM AV1");
}

static inline void copy_data(AVFrame *pic, const struct encoder_frame *frame,
			     int height, enum AVPixelFormat format)
{
	int h_chroma_shift, v_chroma_shift;
	av_pix_fmt_get_chroma_sub_sample(format, &h_chroma_shift,
					 &v_chroma_shift);
	for (int plane = 0; plane < MAX_AV_PLANES; plane++) {
		if (!frame->data[plane])
			continue;

		int frame_rowsize = (int)frame->linesize[plane];
		int pic_rowsize = pic->linesize[plane];
		int bytes = frame_rowsize < pic_rowsize ? frame_rowsize
							: pic_rowsize;
		int plane_height = height >> (plane ? v_chroma_shift : 0);

		for (int y = 0; y < plane_height; y++) {
			int pos_frame = y * frame_rowsize;
			int pos_pic = y * pic_rowsize;

			memcpy(pic->data[plane] + pos_pic,
			       frame->data[plane] + pos_frame, bytes);
		}
	}
}

#define SEC_TO_NSEC 1000000000LL
#define TIMEOUT_MAX_SEC 5
#define TIMEOUT_MAX_NSEC (TIMEOUT_MAX_SEC * SEC_TO_NSEC)

static bool av1_encode(void *data, struct encoder_frame *frame,
		       struct encoder_packet *packet, bool *received_packet)
{
	struct av1_encoder *enc = data;
	AVPacket av_pkt = {0};
	bool timeout = false;
	int64_t cur_ts = (int64_t)os_gettime_ns();
	int got_packet;
	int ret;

	if (!enc->start_ts)
		enc->start_ts = cur_ts;

	copy_data(enc->vframe, frame, enc->height, enc->context->pix_fmt);

	enc->vframe->pts = frame->pts;
	ret = avcodec_send_frame(enc->context, enc->vframe);
	if (ret == 0)
		ret = avcodec_receive_packet(enc->context, &av_pkt);

	got_packet = (ret == 0);

	if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
		ret = 0;
	if (ret < 0) {
		warn("av1_encode: Error encoding: %s", av_err2str(ret));
		return false;
	}

	if (got_packet && av_pkt.size) {
		if (enc->first_packet) {
			if (enc->svtav1) {
				da_copy_array(enc->header,
					      enc->context->extradata,
					      enc->context->extradata_size);
			} else {
				for (int i = 0; i < av_pkt.side_data_elems;
				     i++) {
					AVPacketSideData *side_data =
						av_pkt.side_data + i;
					if (side_data->type ==
					    AV_PKT_DATA_NEW_EXTRADATA) {
						da_copy_array(enc->header,
							      side_data->data,
							      side_data->size);
						break;
					}
				}
			}
			enc->first_packet = false;
		}
		da_copy_array(enc->buffer, av_pkt.data, av_pkt.size);

		packet->pts = av_pkt.pts;
		packet->dts = av_pkt.dts;
		packet->data = enc->buffer.array;
		packet->size = enc->buffer.num;
		packet->type = OBS_ENCODER_VIDEO;
		packet->keyframe = !!(av_pkt.flags & AV_PKT_FLAG_KEY);
		*received_packet = true;

		uint64_t recv_ts_nsec =
			util_mul_div64((uint64_t)av_pkt.dts,
				       (uint64_t)SEC_TO_NSEC,
				       (uint64_t)enc->context->time_base.den) +
			enc->start_ts;

#if 0
		debug("cur: %lld, packet: %lld, diff: %lld", cur_ts,
		      recv_ts_nsec, cur_ts - recv_ts_nsec);
#endif
		if (llabs(cur_ts - recv_ts_nsec) > TIMEOUT_MAX_NSEC) {
			char timeout_str[16];
			snprintf(timeout_str, sizeof(timeout_str), "%d",
				 TIMEOUT_MAX_SEC);

			struct dstr error_text = {0};
			dstr_copy(&error_text,
				  obs_module_text("Encoder.Timeout"));
			dstr_replace(&error_text, "%1", enc->enc_name);
			dstr_replace(&error_text, "%2", timeout_str);
			obs_encoder_set_last_error(enc->encoder,
						   error_text.array);
			dstr_free(&error_text);

			error("Encoding queue duration surpassed %d "
			      "seconds, terminating encoder",
			      TIMEOUT_MAX_SEC);
			timeout = true;
		}
	} else {
		*received_packet = false;
	}

	av_packet_unref(&av_pkt);
	return !timeout;
}

void av1_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "bitrate", 2500);
	obs_data_set_default_int(settings, "keyint_sec", 0);
	obs_data_set_default_int(settings, "cqp", 50);
	obs_data_set_default_string(settings, "rate_control", "CBR");
	obs_data_set_default_int(settings, "preset", 8);
}

static bool rate_control_modified(obs_properties_t *ppts, obs_property_t *p,
				  obs_data_t *settings)
{
	const char *rc = obs_data_get_string(settings, "rate_control");
	bool cqp = astrcmpi(rc, "CQP") == 0;
	bool vbr = astrcmpi(rc, "VBR") == 0;

	p = obs_properties_get(ppts, "bitrate");
	obs_property_set_visible(p, !cqp);
	p = obs_properties_get(ppts, "max_bitrate");
	obs_property_set_visible(p, vbr);
	p = obs_properties_get(ppts, "cqp");
	obs_property_set_visible(p, cqp);

	return true;
}

obs_properties_t *av1_properties(bool svtav1)
{
	obs_properties_t *props = obs_properties_create();
	obs_property_t *p;

	p = obs_properties_add_list(props, "rate_control",
				    obs_module_text("RateControl"),
				    OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(p, "CBR", "CBR");
	obs_property_list_add_string(p, "CQP", "CQP");
	obs_property_list_add_string(p, "VBR", "VBR");

	obs_property_set_modified_callback(p, rate_control_modified);

	p = obs_properties_add_int(props, "bitrate", obs_module_text("Bitrate"),
				   50, 300000, 50);
	obs_property_int_set_suffix(p, " Kbps");

	obs_properties_add_int(props, "cqp", obs_module_text("NVENC.CQLevel"),
			       1, 63, 1);

	obs_properties_add_int(props, "keyint_sec",
			       obs_module_text("KeyframeIntervalSec"), 0, 10,
			       1);

	p = obs_properties_add_list(props, "preset", obs_module_text("Preset"),
				    OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	if (svtav1) {
		obs_property_list_add_int(p, "Very likely too slow (6)", 6);
		obs_property_list_add_int(p, "Probably too slow (7)", 7);
		obs_property_list_add_int(p, "Seems okay (8)", 8);
		obs_property_list_add_int(p, "Might be better (9)", 9);
		obs_property_list_add_int(p, "A little bit faster? (10)", 10);
		obs_property_list_add_int(p, "Hmm, not bad speed (11)", 11);
		obs_property_list_add_int(
			p, "Whoa, although quality might be not so great (12)",
			12);
	} else {
		obs_property_list_add_int(p, "Probably too slow (7)", 7);
		obs_property_list_add_int(p, "Okay (8)", 8);
		obs_property_list_add_int(p, "Fast (9)", 9);
		obs_property_list_add_int(p, "Fastest (10)", 10);
	}

	obs_properties_add_text(props, "ffmpeg_opts",
				obs_module_text("FFmpegOpts"),
				OBS_TEXT_DEFAULT);

	return props;
}

obs_properties_t *aom_av1_properties(void *unused)
{
	UNUSED_PARAMETER(unused);
	return av1_properties(false);
}

obs_properties_t *svt_av1_properties(void *unused)
{
	UNUSED_PARAMETER(unused);
	return av1_properties(true);
}

static bool av1_extra_data(void *data, uint8_t **extra_data, size_t *size)
{
	struct av1_encoder *enc = data;

	*extra_data = enc->header.array;
	*size = enc->header.num;
	return true;
}

struct obs_encoder_info svt_av1_encoder_info = {
	.id = "ffmpeg_svt_av1",
	.type = OBS_ENCODER_VIDEO,
	.codec = "av1",
	.get_name = svt_av1_getname,
	.create = svt_av1_create,
	.destroy = av1_destroy,
	.encode = av1_encode,
	.get_defaults = av1_defaults,
	.get_properties = svt_av1_properties,
	.get_extra_data = av1_extra_data,
	.get_video_info = av1_video_info,
};

struct obs_encoder_info aom_av1_encoder_info = {
	.id = "ffmpeg_aom_av1",
	.type = OBS_ENCODER_VIDEO,
	.codec = "av1",
	.get_name = aom_av1_getname,
	.create = aom_av1_create,
	.destroy = av1_destroy,
	.encode = av1_encode,
	.get_defaults = av1_defaults,
	.get_properties = aom_av1_properties,
	.get_extra_data = av1_extra_data,
	.get_video_info = av1_video_info,
};
