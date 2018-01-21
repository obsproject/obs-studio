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

#include <util/darray.h>
#include <util/dstr.h>
#include <util/base.h>
#include <media-io/video-io.h>
#include <obs-module.h>
#include <obs-avc.h>

#include <unistd.h>

#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_vaapi.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>

#include "obs-ffmpeg-formats.h"

#define do_log(level, format, ...) \
	blog(level, "[FFMPEG VAAPI encoder: '%s'] " format, \
			obs_encoder_get_name(enc->encoder), ##__VA_ARGS__)

#define warn(format, ...)  do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...)  do_log(LOG_INFO,    format, ##__VA_ARGS__)
#define debug(format, ...) do_log(LOG_DEBUG,   format, ##__VA_ARGS__)

struct vaapi_encoder {
	obs_encoder_t                  *encoder;

	AVBufferRef                    *vadevice_ref;
	AVBufferRef                    *vaframes_ref;

	AVCodec                        *vaapi;
	AVCodecContext                 *context;

	AVPicture                      dst_picture;
	AVFrame                        *vframe;

	DARRAY(uint8_t)                buffer;

	uint8_t                        *header;
	size_t                         header_size;

	uint8_t                        *sei;
	size_t                         sei_size;

	int                            height;
	bool                           first_packet;
	bool                           initialized;
};

static const char *vaapi_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "FFMPEG VAAPI";
}

static inline bool valid_format(enum video_format format)
{
	/* NV12 is the only supported format by current VAAPI backends */
	return format == VIDEO_FORMAT_NV12;
}

static void vaapi_video_info(void *data, struct video_scale_info *info)
{
	struct vaapi_encoder *enc = data;
	enum video_format pref_format;

	pref_format = obs_encoder_get_preferred_video_format(enc->encoder);

	if (!valid_format(pref_format)) {
		pref_format = valid_format(info->format) ?
			info->format : VIDEO_FORMAT_NV12;
	}

	info->format = pref_format;
}

static bool vaapi_init_codec(struct vaapi_encoder *enc)
{
	int ret;



	ret = av_hwdevice_ctx_create(&enc->vadevice_ref, AV_HWDEVICE_TYPE_VAAPI, "/dev/dri/renderD128", NULL, 0);
	if (ret < 0) {
		warn("Failed to create VAAPI device context: %s", av_err2str(ret));
		return false;
	}

	enc->vaframes_ref = av_hwframe_ctx_alloc(enc->vadevice_ref);
	if (!enc->vaframes_ref) {
		warn("Failed to alloc HW frames context");
		return false;
	}

	AVHWFramesContext *frames_ctx = (AVHWFramesContext*)enc->vaframes_ref->data;
	frames_ctx->format = AV_PIX_FMT_VAAPI;
	frames_ctx->sw_format = enc->context->pix_fmt; /* FIXME constraint ? */
	frames_ctx->width = enc->context->width;
	frames_ctx->height = enc->context->height;

	ret = av_hwframe_ctx_init(enc->vaframes_ref);
	if (ret < 0 ) {
		warn("Failed to init HW frames context: %s", av_err2str(ret));
		return false;
	}

	/* 2. Create software frame and picture */
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

	ret = avpicture_alloc(&enc->dst_picture, enc->context->pix_fmt,
			enc->context->width, enc->context->height);
	if (ret < 0) {
		warn("Failed to allocate dst_picture: %s", av_err2str(ret));
		return false;
	}

	/* 3. set up codec */
	enc->context->pix_fmt = AV_PIX_FMT_VAAPI;
	enc->context->hw_frames_ctx = av_buffer_ref(enc->vaframes_ref);

	ret = avcodec_open2(enc->context, enc->vaapi, NULL);
	if (ret < 0) {
		warn("Failed to open VAAPI codec: %s", av_err2str(ret));
		return false;
	}

	enc->initialized = true;

	*((AVPicture*)enc->vframe) = enc->dst_picture;
	return true;
}

static bool vaapi_update(void *data, obs_data_t *settings)
{
	struct vaapi_encoder *enc = data;

	int codec = (int)obs_data_get_string(settings, "vaapi_codec");

	int profile = (int)obs_data_get_int(settings, "profile");
	int bf = (int)obs_data_get_int(settings, "bf");

	int level = (int)obs_data_get_int(settings, "level");
	int bitrate = (int)obs_data_get_int(settings, "bitrate");
	int keyint_sec = (int)obs_data_get_int(settings, "keyint_sec");

	int qp = (int)obs_data_get_int(settings, "qp");
	int quality = (int)obs_data_get_int(settings, "quality");
	int lfl = (int)obs_data_get_int(settings, "lfl");
	int lfs = (int)obs_data_get_int(settings, "lfs");

	if(codec==AV_CODEC_ID_H264 || codec==AV_CODEC_ID_HEVC) {
		av_opt_set_int(enc->context->priv_data, "qp", qp, 0);
	}

	if(codec==AV_CODEC_ID_H264) {
		av_opt_set_int(enc->context->priv_data, "quality", quality, 0);
	}

	if(codec==AV_CODEC_ID_VP8 || codec==AV_CODEC_ID_VP9 || codec==AV_CODEC_ID_MPEG2VIDEO) {
		enc->context->global_quality = quality;
	}

	if(codec==AV_CODEC_ID_VP8 || codec==AV_CODEC_ID_VP9) {
		av_opt_set_int(enc->context->priv_data, "loop_filter_level", lfl, 0);
		av_opt_set_int(enc->context->priv_data, "loop_filter_sharpness", lfs, 0);
	}

	video_t *video = obs_encoder_video(enc->encoder);
	const struct video_output_info *voi = video_output_get_info(video);
	struct video_scale_info info;

	info.format = voi->format;
	info.colorspace = voi->colorspace;
	info.range = voi->range;

	vaapi_video_info(enc, &info);

	if(codec!=AV_CODEC_ID_VP8) {
		enc->context->profile = profile;
		enc->context->max_b_frames = bf;
	}

	enc->context->level = level;
	enc->context->bit_rate = bitrate * 1000;

	enc->context->width = obs_encoder_get_width(enc->encoder);
	enc->context->height = obs_encoder_get_height(enc->encoder);

	/* for h.264, 1080 isn't devisable by 16, so we must lower */
	/* height to prevent green or black bars */

	if(enc->context->height == 1080 && codec==AV_CODEC_ID_H264) {
		enc->context->height = 1072;
	}

	enc->context->time_base = (AVRational){voi->fps_den, voi->fps_num};
	enc->context->pix_fmt = obs_to_ffmpeg_video_format(info.format);
	enc->context->colorspace = info.colorspace == VIDEO_CS_709 ?
		AVCOL_SPC_BT709 : AVCOL_SPC_BT470BG;
	enc->context->color_range = info.range == VIDEO_RANGE_FULL ?
		AVCOL_RANGE_JPEG : AVCOL_RANGE_MPEG;

	if (keyint_sec > 0) {
		enc->context->gop_size = keyint_sec * voi->fps_num / voi->fps_den;
	} else {
		enc->context->gop_size = 120;
	}

	enc->height = enc->context->height;

	info("settings:\n"
	     "\tqp:           %d\n"
	     "\tquality:      %d\n"
	     "\tprofile:      %d\n"
	     "\tlevel:        %d\n"
	     "\tbitrate:      %d\n"
	     "\tkeyint:       %d\n"
	     "\twidth:        %d\n"
	     "\theight:       %d\n"
	     "\tb-frames:     %d\n"
	     "\tloop-filter-level:     %d\n"
	     "\tloop-filter-sharpness:     %d\n",
             qp, quality, profile, level,
	     bitrate, enc->context->gop_size,
	     enc->context->width, enc->context->height,
	     enc->context->max_b_frames,lfl,lfs);

	return vaapi_init_codec(enc);
}

static void vaapi_destroy(void *data)
{
	struct vaapi_encoder *enc = data;

	if (enc->initialized) {
		AVPacket pkt = {0};
		int r_pkt = 1;

		while (r_pkt) {
			if (avcodec_encode_video2(enc->context, &pkt, NULL,
						&r_pkt) < 0)
				break;

			if (r_pkt)
				av_free_packet(&pkt);
		}
	}

	avcodec_close(enc->context);
	av_frame_free(&enc->vframe);
	avpicture_free(&enc->dst_picture);
	av_buffer_unref(&enc->vaframes_ref);
	av_buffer_unref(&enc->vadevice_ref);
	da_free(enc->buffer);
	bfree(enc->header);
	bfree(enc->sei);

	bfree(enc);
}

static void *vaapi_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	struct vaapi_encoder *enc;
	avcodec_register_all();

	enc = bzalloc(sizeof(*enc));
	enc->encoder = encoder;

	int vaapi_codec = (int)obs_data_get_int(settings, "vaapi_codec");

	if(vaapi_codec==AV_CODEC_ID_HEVC) {
		enc->vaapi = avcodec_find_encoder_by_name("hevc_vaapi");
	}
	if(vaapi_codec==AV_CODEC_ID_H264) {
		enc->vaapi = avcodec_find_encoder_by_name("h264_vaapi");
	}
	if(vaapi_codec==AV_CODEC_ID_MPEG2VIDEO) {
		enc->vaapi = avcodec_find_encoder_by_name("mpeg2_vaapi");
	}
	if(vaapi_codec==AV_CODEC_ID_VP8) {
		enc->vaapi = avcodec_find_encoder_by_name("vp8_vaapi");
	}
	if(vaapi_codec==AV_CODEC_ID_VP9) {
		enc->vaapi = avcodec_find_encoder_by_name("vp9_vaapi");
	}

	enc->first_packet = true;

	blog(LOG_INFO, "---------------------------------");

	if (!enc->vaapi) {
		warn("Couldn't find encoder");
		goto fail;
	}

	enc->context = avcodec_alloc_context3(enc->vaapi);
	if (!enc->context) {
		warn("Failed to create codec context");
		goto fail;
	}

	if (!vaapi_update(enc, settings))
		goto fail;

	return enc;

fail:
	vaapi_destroy(enc);
	return NULL;
}

static inline void copy_data(AVPicture *pic, const struct encoder_frame *frame,
		int height, enum AVPixelFormat format)
{
	int h_chroma_shift, v_chroma_shift;
	av_pix_fmt_get_chroma_sub_sample(format, &h_chroma_shift, &v_chroma_shift);
	for (int plane = 0; plane < MAX_AV_PLANES; plane++) {
		if (!frame->data[plane])
			continue;

		int frame_rowsize = (int)frame->linesize[plane];
		int pic_rowsize   = pic->linesize[plane];
		int bytes = frame_rowsize < pic_rowsize ?
			frame_rowsize : pic_rowsize;
		int plane_height = height >> (plane ? v_chroma_shift : 0);

		for (int y = 0; y < plane_height; y++) {
			int pos_frame = y * frame_rowsize;
			int pos_pic   = y * pic_rowsize;

			memcpy(pic->data[plane] + pos_pic,
			       frame->data[plane] + pos_frame,
			       bytes);
		}
	}
}

static bool vaapi_encode(void *data, struct encoder_frame *frame,
		struct encoder_packet *packet, bool *received_packet)
{
	struct vaapi_encoder *enc = data;
	AVFrame *hwframe;
	AVPacket av_pkt = {0};
	int got_packet;
	int ret;

	hwframe = av_frame_alloc();
	if (!hwframe) {
		warn("vaapi_encode: failed to allocate hw frame");
		return false;
	}

	ret = av_hwframe_get_buffer(enc->vaframes_ref, hwframe, 0);
	if (ret < 0) {
		warn("vaapi_encode: failed to get buffer for hw frame: %s", av_err2str(ret));
		return false;
	}

	copy_data(&enc->dst_picture, frame, enc->height, enc->context->pix_fmt);
	enc->vframe->pts = frame->pts;
	hwframe->pts = frame->pts;
	hwframe->width = enc->vframe->width;
	hwframe->height = enc->vframe->height;

	ret = av_hwframe_transfer_data(hwframe, enc->vframe, 0);
	if (ret < 0) {
		warn("vaapi_encode: failed to upload hw frame: %s", av_err2str(ret));
		return false;
	}

	ret = av_frame_copy_props(hwframe, enc->vframe);
	if (ret < 0) {
		warn("vaapi_encode: failed to copy props to hw frame: %s", av_err2str(ret));
		return false;
	}

	av_init_packet(&av_pkt); /* FIXME leaks on errors? */

	ret = avcodec_encode_video2(enc->context, &av_pkt, hwframe,
			&got_packet);
	if (ret < 0) {
		warn("vaapi_encode: Error encoding: %s", av_err2str(ret));
		return false;
	}

	if (got_packet && av_pkt.size) {
		if (enc->first_packet) {
			uint8_t *new_packet;
			size_t size;

			enc->first_packet = false;
			obs_extract_avc_headers(av_pkt.data, av_pkt.size,
					&new_packet, &size,
					&enc->header, &enc->header_size,
					&enc->sei, &enc->sei_size);

			da_copy_array(enc->buffer, new_packet, size);
			bfree(new_packet);
		} else {
			da_copy_array(enc->buffer, av_pkt.data, av_pkt.size);
		}

		packet->pts = av_pkt.pts;
		packet->dts = av_pkt.dts;
		packet->data = enc->buffer.array;
		packet->size = enc->buffer.num;
		packet->type = OBS_ENCODER_VIDEO;
		packet->keyframe = obs_avc_keyframe(packet->data, packet->size);
		*received_packet = true;
	} else {
		*received_packet = false;
	}

	av_free_packet(&av_pkt);
	av_frame_free(&hwframe);
	return true;
}

static void set_visible(obs_properties_t *ppts, const char *name, bool visible)
{
	obs_property_t *p = obs_properties_get(ppts, name);
	obs_property_set_visible(p, visible);
}


static void vaapi_defaults(obs_data_t *settings)
{
	obs_data_set_default_string(settings, "vaapi_device", "/dev/dri/renderD128");
	obs_data_set_default_int(settings, "vaapi_codec", AV_CODEC_ID_H264);
	obs_data_set_default_int(settings, "profile", FF_PROFILE_H264_BASELINE);
	obs_data_set_default_int(settings, "level", 31);
	obs_data_set_default_int(settings, "bitrate", 2500);
	obs_data_set_default_int(settings, "keyint_sec", 0);
	obs_data_set_default_int(settings, "bf", 0);
	obs_data_set_default_int(settings, "qp", 20);
	obs_data_set_default_int(settings, "quality", 0);
	obs_data_set_default_int(settings, "rendermode", 0);
	obs_data_set_default_int(settings, "lfl", 16);
	obs_data_set_default_int(settings, "lfs", 4);
}

static bool profile_modified(obs_properties_t *ppts, obs_property_t *p,
		obs_data_t *settings)
{
	UNUSED_PARAMETER(p);

	int profile_type = (int)obs_data_get_int(settings, "profile");

	set_visible(ppts, "bf", false);

	if(profile_type!=FF_PROFILE_H264_BASELINE || profile_type != FF_PROFILE_H264_MAIN) {
		set_visible(ppts, "bf", true);
		obs_data_set_default_int(settings, "bf", 0);
	}


	return true;
}

static bool codec_modified(obs_properties_t *ppts, obs_property_t *p,
		obs_data_t *settings)
{
	UNUSED_PARAMETER(p);

	int codec_type = (int)obs_data_get_int(settings, "vaapi_codec");

	set_visible(ppts, "quality", false);
	set_visible(ppts, "bf", false);
	set_visible(ppts, "lfl", false);
	set_visible(ppts, "lfs", false);

	if (codec_type!=AV_CODEC_ID_HEVC) {
		set_visible(ppts, "quality", true);
		if (codec_type==AV_CODEC_ID_H264) {
			obs_data_set_default_int(settings, "quality", 0);
		}

		if (codec_type==AV_CODEC_ID_MPEG2VIDEO) {
			obs_data_set_default_int(settings, "quality", 10);
		}

		if (codec_type==AV_CODEC_ID_VP8 || codec_type==AV_CODEC_ID_VP9) {
			obs_data_set_default_int(settings, "quality", 40);
			set_visible(ppts, "lfl", true);
			set_visible(ppts, "lfs", true);
		}
	}

	return true;
}

static bool rendermode_modified(obs_properties_t *ppts, obs_property_t *p,
		obs_data_t *settings)
{
	UNUSED_PARAMETER(p);

	int rendermode = (int)obs_data_get_int(settings, "rendermode");

	set_visible(ppts, "qp", false);
	set_visible(ppts, "bitrate", true);
	obs_data_set_default_int(settings, "bitrate", 2500);

	if (rendermode > 0) {
		set_visible(ppts, "qp", true);
		set_visible(ppts, "bitrate", false);
		obs_data_set_default_int(settings, "bitrate", 0);
	}

	return true;
}

static obs_properties_t *vaapi_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *props = obs_properties_create();
	obs_property_t *list;

	list = obs_properties_add_list(props, "vaapi_device", "VAAPI Device",
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	char path[128] = "/dev/dri/renderD1";
	for (int i = 28;; i++) {
		sprintf(path, "/dev/dri/renderD1%d", i);
		if (access(path, F_OK) == 0) {			
			char card[128] = "Card: ";
			sprintf(card, "Card%d: %s", i-28, path);
			obs_property_list_add_string(list, card, path);
		} else {
			break;
		}
	}

	list = obs_properties_add_list(props, "vaapi_codec", "VAAPI Codec",
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(list, "HEVC", AV_CODEC_ID_HEVC);
	obs_property_list_add_int(list, "H.264 (default)", AV_CODEC_ID_H264);
	obs_property_list_add_int(list, "MPEG-2", AV_CODEC_ID_MPEG2VIDEO);
	obs_property_list_add_int(list, "VP8", AV_CODEC_ID_VP8);
	obs_property_list_add_int(list, "VP9", AV_CODEC_ID_VP9);

	obs_property_set_modified_callback(list, codec_modified);

	list = obs_properties_add_list(props, "profile", "Profile",
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(list, "h264-baseline", FF_PROFILE_H264_BASELINE);
	obs_property_list_add_int(list, "h264-main", FF_PROFILE_H264_MAIN);
	obs_property_list_add_int(list, "h264-high", FF_PROFILE_H264_HIGH);
	obs_property_list_add_int(list, "hevc-main", FF_PROFILE_HEVC_MAIN);
	obs_property_list_add_int(list, "hevc-main10", FF_PROFILE_HEVC_MAIN_10);
	obs_property_list_add_int(list, "hevc-main-still-picture", FF_PROFILE_HEVC_MAIN_STILL_PICTURE);
	obs_property_list_add_int(list, "hevc-main-rext", FF_PROFILE_HEVC_REXT);
	obs_property_list_add_int(list, "mpeg2-aac-low", FF_PROFILE_MPEG2_AAC_LOW);
	obs_property_list_add_int(list, "mpeg2-aac-he", FF_PROFILE_MPEG2_AAC_HE);
	obs_property_list_add_int(list, "mpeg2-422", FF_PROFILE_MPEG2_422);
	obs_property_list_add_int(list, "mpeg2-high", FF_PROFILE_MPEG2_HIGH);
	obs_property_list_add_int(list, "mpeg2-ss", FF_PROFILE_MPEG2_SS);
	obs_property_list_add_int(list, "mpeg2-snr-scalable", FF_PROFILE_MPEG2_SNR_SCALABLE);
	obs_property_list_add_int(list, "mpeg2-main", FF_PROFILE_MPEG2_MAIN);
	obs_property_list_add_int(list, "mpeg2-simple", FF_PROFILE_MPEG2_SIMPLE);
	obs_property_list_add_int(list, "vp9-0", FF_PROFILE_VP9_0);
	obs_property_list_add_int(list, "vp9-1", FF_PROFILE_VP9_1);
	obs_property_list_add_int(list, "vp9-2", FF_PROFILE_VP9_2);
	obs_property_list_add_int(list, "vp9-3", FF_PROFILE_VP9_3);

	obs_property_set_modified_callback(list, profile_modified);

	list = obs_properties_add_list(props, "level", "Level",
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(list, "480p30 (3.0)", 30);
	obs_property_list_add_int(list, "720p30/480p60  (3.1 default)", 31);
	obs_property_list_add_int(list, "720p60/1080p30 (4.1)", 41);
	obs_property_list_add_int(list, "1080p60 (4.2)", 42);

	list = obs_properties_add_list(props, "rendermode", "Render Mode",
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(list, "Bitrate", 0);
	obs_property_list_add_int(list, "QP", 1);

	obs_property_set_modified_callback(list, rendermode_modified);

	obs_properties_add_int(props, "bitrate",
			obs_module_text("Bitrate"), 0, 300000, 50);

	obs_properties_add_int(props, "keyint_sec",
			obs_module_text("Keyframe Interval (seconds)"), 0, 20, 1);

	obs_properties_add_int(props, "bf", obs_module_text("B-frames (Not supported on AMD, set 0)"),
			0, 4, 1);

	obs_properties_add_int(props, "qp",
			obs_module_text("QP"), 0, 51, 1);

	obs_properties_add_int(props, "quality",
			obs_module_text("Quality"), 0, 40, 1);

	obs_properties_add_int(props, "lfl",
			obs_module_text("Loop Filter Level"), 0, 16, 1);

	obs_properties_add_int(props, "lfs",
			obs_module_text("Loop Filter Sharpness"), 0, 4, 1);

	return props;
}

static bool vaapi_extra_data(void *data, uint8_t **extra_data, size_t *size)
{
	struct vaapi_encoder *enc = data;

	*extra_data = enc->header;
	*size       = enc->header_size;
	return true;
}

static bool vaapi_sei_data(void *data, uint8_t **extra_data, size_t *size)
{
	struct vaapi_encoder *enc = data;

	*extra_data = enc->sei;
	*size       = enc->sei_size;
	return true;
}


struct obs_encoder_info vaapi_encoder_info = {
	.id             = "ffmpeg_vaapi",
	.type           = OBS_ENCODER_VIDEO,
	.codec          = "h264",
	.get_name       = vaapi_getname,
	.create         = vaapi_create,
	.destroy        = vaapi_destroy,
	.encode         = vaapi_encode,
	.get_defaults   = vaapi_defaults,
	.get_properties = vaapi_properties,
	.get_extra_data = vaapi_extra_data,
	.get_sei_data   = vaapi_sei_data,
	.get_video_info = vaapi_video_info
};
