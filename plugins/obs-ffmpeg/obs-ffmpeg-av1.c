/******************************************************************************
    Copyright (C) 2022 by Hugh Bailey <obs.jim@gmail.com>

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

#include "obs-ffmpeg-video-encoders.h"

#define do_log(level, format, ...)                 \
	blog(level, "[AV1 encoder: '%s'] " format, \
	     obs_encoder_get_name(enc->ffve.encoder), ##__VA_ARGS__)

#define error(format, ...) do_log(LOG_ERROR, format, ##__VA_ARGS__)
#define warn(format, ...) do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...) do_log(LOG_INFO, format, ##__VA_ARGS__)
#define debug(format, ...) do_log(LOG_DEBUG, format, ##__VA_ARGS__)

enum av1_encoder_type {
	AV1_ENCODER_TYPE_AOM,
	AV1_ENCODER_TYPE_SVT,
};

struct av1_encoder {
	struct ffmpeg_video_encoder ffve;
	enum av1_encoder_type type;

	DARRAY(uint8_t) header;
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

static bool av1_update(struct av1_encoder *enc, obs_data_t *settings)
{
	const char *rc = obs_data_get_string(settings, "rate_control");
	int bitrate = (int)obs_data_get_int(settings, "bitrate");
	int cqp = (int)obs_data_get_int(settings, "cqp");
	int keyint_sec = (int)obs_data_get_int(settings, "keyint_sec");
	int preset = (int)obs_data_get_int(settings, "preset");
	AVDictionary *svtav1_opts = NULL;

	video_t *video = obs_encoder_video(enc->ffve.encoder);
	const struct video_output_info *voi = video_output_get_info(video);
	struct video_scale_info info;

	info.format = voi->format;
	info.colorspace = voi->colorspace;
	info.range = voi->range;

	enc->ffve.context->thread_count = 0;

	av1_video_info(enc, &info);

	if (enc->type == AV1_ENCODER_TYPE_SVT) {
		av_opt_set_int(enc->ffve.context->priv_data, "preset", preset,
			       0);
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59, 37, 100)
		av_dict_set_int(&svtav1_opts, "rc", 1, 0);
#else
		av_opt_set(enc->ffve.context->priv_data, "rc", "vbr", 0);
#endif
	} else if (enc->type == AV1_ENCODER_TYPE_AOM) {
		av_opt_set_int(enc->ffve.context->priv_data, "cpu-used", preset,
			       0);
		av_opt_set(enc->ffve.context->priv_data, "usage", "realtime",
			   0);
#if 0
		av_opt_set_int(enc->ffve.context->priv_data, "tile-columns", 4, 0);
		//av_opt_set_int(enc->ffve.context->priv_data, "tile-rows", 4, 0);
#else
		av_opt_set_int(enc->ffve.context->priv_data, "tile-columns", 2,
			       0);
		av_opt_set_int(enc->ffve.context->priv_data, "tile-rows", 2, 0);
#endif
		av_opt_set_int(enc->ffve.context->priv_data, "row-mt", 1, 0);
	}

	if (astrcmpi(rc, "cqp") == 0) {
		bitrate = 0;
		av_opt_set_int(enc->ffve.context->priv_data, "crf", cqp, 0);

		if (enc->type == AV1_ENCODER_TYPE_SVT) {
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59, 37, 100)
			av_dict_set_int(&svtav1_opts, "rc", 0, 0);
			av_opt_set_int(enc->ffve.context->priv_data, "qp", cqp,
				       0);
#else
			av_opt_set(enc->ffve.context->priv_data, "rc", "cqp",
				   0);
			av_opt_set_int(enc->ffve.context->priv_data, "qp", cqp,
				       0);
#endif
		}

	} else if (astrcmpi(rc, "vbr") != 0) { /* CBR by default */
		const int64_t rate = bitrate * INT64_C(1000);
		enc->ffve.context->rc_min_rate = rate;
		cqp = 0;

		if (enc->type == AV1_ENCODER_TYPE_SVT) {
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59, 37, 100)
			av_dict_set_int(&svtav1_opts, "rc", 2, 0);
			av_dict_set_int(&svtav1_opts, "pred-struct", 1, 0);
			av_dict_set_int(&svtav1_opts, "bias-pct", 0, 0);
			av_dict_set_int(&svtav1_opts, "tbr", rate, 0);
#else
			enc->ffve.context->rc_max_rate = rate;
			av_opt_set(enc->ffve.context->priv_data, "rc", "cvbr",
				   0);
#endif
		} else {
			enc->ffve.context->rc_max_rate = rate;
		}
	}

	if (enc->type == AV1_ENCODER_TYPE_SVT) {
		av_opt_set_dict_val(enc->ffve.context->priv_data, "svtav1_opts",
				    svtav1_opts, 0);
	}

	const char *ffmpeg_opts = obs_data_get_string(settings, "ffmpeg_opts");
	ffmpeg_video_encoder_update(&enc->ffve, bitrate, keyint_sec, voi, &info,
				    ffmpeg_opts);
	av_dict_free(&svtav1_opts);

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
	     enc->ffve.enc_name, rc, bitrate, cqp, enc->ffve.context->gop_size,
	     preset, enc->ffve.context->width, enc->ffve.height, ffmpeg_opts);

	enc->ffve.context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	return ffmpeg_video_encoder_init_codec(&enc->ffve);
}

static void av1_destroy(void *data)
{
	struct av1_encoder *enc = data;

	ffmpeg_video_encoder_free(&enc->ffve);
	da_free(enc->header);
	bfree(enc);
}

static void on_first_packet(void *data, AVPacket *pkt, struct darray *da)
{
	struct av1_encoder *enc = data;

	if (enc->type == AV1_ENCODER_TYPE_SVT) {
		da_copy_array(enc->header, enc->ffve.context->extradata,
			      enc->ffve.context->extradata_size);
	} else {
		for (int i = 0; i < pkt->side_data_elems; i++) {
			AVPacketSideData *side_data = pkt->side_data + i;
			if (side_data->type == AV_PKT_DATA_NEW_EXTRADATA) {
				da_copy_array(enc->header, side_data->data,
					      side_data->size);
				break;
			}
		}
	}

	darray_copy_array(1, da, pkt->data, pkt->size);
}

static void *av1_create_internal(obs_data_t *settings, obs_encoder_t *encoder,
				 const char *enc_lib, const char *enc_name)
{
	video_t *video = obs_encoder_video(encoder);
	const struct video_output_info *voi = video_output_get_info(video);
	switch (voi->format) {
	case VIDEO_FORMAT_I010:
	case VIDEO_FORMAT_P010:
		break;
	default:
		switch (voi->colorspace) {
		case VIDEO_CS_2100_PQ:
		case VIDEO_CS_2100_HLG: {
			const char *const text =
				obs_module_text("AV1.8bitUnsupportedHdr");
			obs_encoder_set_last_error(encoder, text);
			blog(LOG_ERROR, "[AV1 encoder] %s", text);
			return NULL;
		}
		}
	}

	struct av1_encoder *enc = bzalloc(sizeof(*enc));

	if (strcmp(enc_lib, "libsvtav1") == 0)
		enc->type = AV1_ENCODER_TYPE_SVT;
	else if (strcmp(enc_lib, "libaom-av1") == 0)
		enc->type = AV1_ENCODER_TYPE_AOM;

	if (!ffmpeg_video_encoder_init(&enc->ffve, enc, encoder, enc_lib, NULL,
				       enc_name, NULL, on_first_packet))
		goto fail;
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

static bool av1_encode(void *data, struct encoder_frame *frame,
		       struct encoder_packet *packet, bool *received_packet)
{
	struct av1_encoder *enc = data;
	return ffmpeg_video_encode(&enc->ffve, frame, packet, received_packet);
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

obs_properties_t *av1_properties(enum av1_encoder_type type)
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

	p = obs_properties_add_int(props, "keyint_sec",
				   obs_module_text("KeyframeIntervalSec"), 0,
				   10, 1);
	obs_property_int_set_suffix(p, " s");

	p = obs_properties_add_list(props, "preset", obs_module_text("Preset"),
				    OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	if (type == AV1_ENCODER_TYPE_SVT) {
		obs_property_list_add_int(p, "Very likely too slow (6)", 6);
		obs_property_list_add_int(p, "Probably too slow (7)", 7);
		obs_property_list_add_int(p, "Seems okay (8)", 8);
		obs_property_list_add_int(p, "Might be better (9)", 9);
		obs_property_list_add_int(p, "A little bit faster? (10)", 10);
		obs_property_list_add_int(p, "Hmm, not bad speed (11)", 11);
		obs_property_list_add_int(
			p, "Whoa, although quality might be not so great (12)",
			12);
	} else if (type == AV1_ENCODER_TYPE_AOM) {
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
	return av1_properties(AV1_ENCODER_TYPE_AOM);
}

obs_properties_t *svt_av1_properties(void *unused)
{
	UNUSED_PARAMETER(unused);
	return av1_properties(AV1_ENCODER_TYPE_SVT);
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
