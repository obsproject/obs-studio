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

#include <libavutil/avutil.h>

#include <util/darray.h>
#include <util/dstr.h>
#include <util/base.h>
#include <util/platform.h>
#include <media-io/video-io.h>
#include <obs-module.h>
#include <obs-avc.h>
#include <obs-av1.h>
#ifdef ENABLE_HEVC
#include <obs-hevc.h>
#endif
#include <opts-parser.h>

#include <unistd.h>

#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_vaapi.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>

#include <pci/pci.h>

#include "vaapi-utils.h"
#include "obs-ffmpeg-formats.h"

#include <va/va_drmcommon.h>
#include <libdrm/drm_fourcc.h>

#define do_log(level, format, ...) \
	blog(level, "[FFmpeg VAAPI encoder: '%s'] " format, obs_encoder_get_name(enc->encoder), ##__VA_ARGS__)

#define warn(format, ...) do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...) do_log(LOG_INFO, format, ##__VA_ARGS__)
#define debug(format, ...) do_log(LOG_DEBUG, format, ##__VA_ARGS__)

enum codec_type {
	CODEC_H264,
	CODEC_HEVC,
	CODEC_AV1,
};

struct vaapi_surface {
	AVFrame *frame;
	gs_texture_t *textures[4];
	uint32_t num_textures;
};

struct vaapi_encoder {
	obs_encoder_t *encoder;
	enum codec_type codec;

	AVBufferRef *vadevice_ref;
	AVBufferRef *vaframes_ref;
	VADisplay va_dpy;

	const AVCodec *vaapi;
	AVCodecContext *context;

	AVPacket *packet;

	AVFrame *vframe;

	DARRAY(uint8_t) buffer;

	uint8_t *header;
	size_t header_size;

	uint8_t *sei;
	size_t sei_size;

	int height;
	bool first_packet;
	bool initialized;
};

static const char *h264_vaapi_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "FFmpeg VAAPI H.264";
}

static const char *av1_vaapi_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "FFmpeg VAAPI AV1";
}

#ifdef ENABLE_HEVC
static const char *hevc_vaapi_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "FFmpeg VAAPI HEVC";
}
#endif

static inline bool vaapi_valid_format(struct vaapi_encoder *enc, enum video_format format)
{
	if (enc->codec == CODEC_H264) {
		return format == VIDEO_FORMAT_NV12;
	} else if (enc->codec == CODEC_HEVC || enc->codec == CODEC_AV1) {
		return (format == VIDEO_FORMAT_NV12) || (format == VIDEO_FORMAT_P010);
	} else {
		return false;
	}
}

static void vaapi_video_info(void *data, struct video_scale_info *info)
{
	struct vaapi_encoder *enc = data;
	enum video_format pref_format;

	pref_format = obs_encoder_get_preferred_video_format(enc->encoder);

	if (!vaapi_valid_format(enc, pref_format)) {
		pref_format = vaapi_valid_format(enc, info->format) ? info->format : VIDEO_FORMAT_NV12;
	}

	info->format = pref_format;
}

static bool vaapi_init_codec(struct vaapi_encoder *enc, const char *path)
{
	int ret;

	ret = av_hwdevice_ctx_create(&enc->vadevice_ref, AV_HWDEVICE_TYPE_VAAPI, path, NULL, 0);
	if (ret < 0) {
		warn("Failed to create VAAPI device context: %s", av_err2str(ret));
		return false;
	}

	AVHWDeviceContext *vahwctx = (AVHWDeviceContext *)enc->vadevice_ref->data;
	AVVAAPIDeviceContext *vadevctx = vahwctx->hwctx;
	enc->va_dpy = vadevctx->display;

	enc->vaframes_ref = av_hwframe_ctx_alloc(enc->vadevice_ref);
	if (!enc->vaframes_ref) {
		warn("Failed to alloc HW frames context");
		return false;
	}

	AVHWFramesContext *frames_ctx = (AVHWFramesContext *)enc->vaframes_ref->data;
	frames_ctx->format = AV_PIX_FMT_VAAPI;
	frames_ctx->sw_format = enc->context->pix_fmt;
	frames_ctx->width = enc->context->width;
	frames_ctx->height = enc->context->height;

	ret = av_hwframe_ctx_init(enc->vaframes_ref);
	if (ret < 0) {
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
	enc->vframe->chroma_location = enc->context->chroma_sample_location;

	ret = av_frame_get_buffer(enc->vframe, base_get_alignment());
	if (ret < 0) {
		warn("Failed to allocate vframe: %s", av_err2str(ret));
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

	enc->packet = av_packet_alloc();

	enc->initialized = true;
	return true;
}

/* "Allowed" options per Rate Control
 * See FFMPEG libavcodec/vaapi_encode.c (vaapi_encode_rc_modes)
 */
typedef struct {
	const char *name;
	bool qp;
	bool bitrate;
	bool maxrate;
} rc_mode_t;

static const rc_mode_t *get_rc_mode(const char *name)
{
	/* Set "allowed" options per Rate Control */
	static const rc_mode_t RC_MODES[] = {{.name = "CBR", .qp = false, .bitrate = true, .maxrate = false},
					     {.name = "CQP", .qp = true, .bitrate = false, .maxrate = false},
					     {.name = "VBR", .qp = false, .bitrate = true, .maxrate = true},
					     {.name = "QVBR", .qp = true, .bitrate = true, .maxrate = true},
					     {0}};

	const rc_mode_t *rc_mode = RC_MODES;

	while (!!rc_mode->name && astrcmpi(rc_mode->name, name) != 0)
		rc_mode++;

	return rc_mode ? rc_mode : RC_MODES;
}

static bool vaapi_update(void *data, obs_data_t *settings)
{
	struct vaapi_encoder *enc = data;

	const char *device = obs_data_get_string(settings, "vaapi_device");

	const char *rate_control = obs_data_get_string(settings, "rate_control");
	const rc_mode_t *rc_mode = get_rc_mode(rate_control);
	bool cbr = strcmp(rc_mode->name, "CBR") == 0;

	int profile = (int)obs_data_get_int(settings, "profile");
	int bf = (int)obs_data_get_int(settings, "bf");
	int qp = rc_mode->qp ? (int)obs_data_get_int(settings, "qp") : 0;

	enc->context->global_quality = enc->codec == CODEC_AV1 ? qp * 5 : qp;

	int level = (int)obs_data_get_int(settings, "level");
	int bitrate = rc_mode->bitrate ? (int)obs_data_get_int(settings, "bitrate") : 0;
	int maxrate = rc_mode->maxrate ? (int)obs_data_get_int(settings, "maxrate") : 0;
	int keyint_sec = (int)obs_data_get_int(settings, "keyint_sec");

	/* For Rate Control which allows maxrate, FFMPEG will give
	 * an error if maxrate > bitrate. To prevent that set maxrate
	 * to 0.
	 * For CBR, maxrate = bitrate
	 */
	if (cbr)
		maxrate = bitrate;
	else if (rc_mode->maxrate && maxrate && maxrate < bitrate)
		maxrate = 0;

	video_t *video = obs_encoder_video(enc->encoder);
	const struct video_output_info *voi = video_output_get_info(video);
	struct video_scale_info info;

	info.format = voi->format;
	info.colorspace = voi->colorspace;
	info.range = voi->range;

#ifdef ENABLE_HEVC
	if (enc->codec == CODEC_HEVC) {
		if ((profile == AV_PROFILE_HEVC_MAIN) && (info.format == VIDEO_FORMAT_P010)) {
			warn("Forcing Main10 for P010");
			profile = AV_PROFILE_HEVC_MAIN_10;
		}
	}
#endif
	vaapi_video_info(enc, &info);

	enc->context->profile = profile;
	enc->context->max_b_frames = bf;
	enc->context->level = level;
	enc->context->bit_rate = bitrate * 1000;
	enc->context->rc_max_rate = maxrate * 1000;
	enc->context->rc_initial_buffer_occupancy = (maxrate ? maxrate : bitrate) * 1000;

	enc->context->width = obs_encoder_get_width(enc->encoder);
	enc->context->height = obs_encoder_get_height(enc->encoder);

	enc->context->time_base = (AVRational){voi->fps_den, voi->fps_num};
	const enum AVPixelFormat pix_fmt = obs_to_ffmpeg_video_format(info.format);
	enc->context->pix_fmt = pix_fmt;
	enc->context->color_range = info.range == VIDEO_RANGE_FULL ? AVCOL_RANGE_JPEG : AVCOL_RANGE_MPEG;

	enum AVColorSpace colorspace = AVCOL_SPC_UNSPECIFIED;
	switch (info.colorspace) {
	case VIDEO_CS_601:
		enc->context->color_primaries = AVCOL_PRI_SMPTE170M;
		enc->context->color_trc = AVCOL_TRC_SMPTE170M;
		colorspace = AVCOL_SPC_SMPTE170M;
		break;
	case VIDEO_CS_DEFAULT:
	case VIDEO_CS_709:
		enc->context->color_primaries = AVCOL_PRI_BT709;
		enc->context->color_trc = AVCOL_TRC_BT709;
		colorspace = AVCOL_SPC_BT709;
		break;
	case VIDEO_CS_SRGB:
		enc->context->color_primaries = AVCOL_PRI_BT709;
		enc->context->color_trc = AVCOL_TRC_IEC61966_2_1;
		colorspace = AVCOL_SPC_BT709;
		break;
	case VIDEO_CS_2100_PQ:
		enc->context->color_primaries = AVCOL_PRI_BT2020;
		enc->context->color_trc = AVCOL_TRC_SMPTE2084;
		colorspace = AVCOL_SPC_BT2020_NCL;
		break;
	case VIDEO_CS_2100_HLG:
		enc->context->color_primaries = AVCOL_PRI_BT2020;
		enc->context->color_trc = AVCOL_TRC_ARIB_STD_B67;
		colorspace = AVCOL_SPC_BT2020_NCL;
		break;
	default:
		break;
	}

	enc->context->colorspace = colorspace;
	enc->context->chroma_sample_location = determine_chroma_location(pix_fmt, colorspace);

	if (keyint_sec > 0) {
		enc->context->gop_size = keyint_sec * voi->fps_num / voi->fps_den;
	} else {
		enc->context->gop_size = 120;
	}

	enc->height = enc->context->height;

	const char *ffmpeg_opts = obs_data_get_string(settings, "ffmpeg_opts");
	struct obs_options opts = obs_parse_options(ffmpeg_opts);
	for (size_t i = 0; i < opts.count; i++) {
		struct obs_option *opt = &opts.options[i];
		av_opt_set(enc->context->priv_data, opt->name, opt->value, 0);
	}
	obs_free_options(opts);

	info("settings:\n"
	     "\tdevice:       %s\n"
	     "\trate_control: %s\n"
	     "\tprofile:      %d\n"
	     "\tlevel:        %d\n"
	     "\tqp:           %d\n"
	     "\tbitrate:      %d\n"
	     "\tmaxrate:      %d\n"
	     "\tkeyint:       %d\n"
	     "\twidth:        %d\n"
	     "\theight:       %d\n"
	     "\tb-frames:     %d\n"
	     "\tffmpeg opts:  %s\n",
	     device, rate_control, profile, level, qp, bitrate, maxrate, enc->context->gop_size, enc->context->width,
	     enc->context->height, enc->context->max_b_frames, ffmpeg_opts);

	return vaapi_init_codec(enc, device);
}

static inline enum gs_color_format drm_to_gs_color_format(int format)
{
	switch (format) {
	case DRM_FORMAT_R8:
		return GS_R8;
	case DRM_FORMAT_R16:
		return GS_R16;
	case DRM_FORMAT_GR88:
		return GS_R8G8;
	case DRM_FORMAT_GR1616:
		return GS_RG16;
	default:
		blog(LOG_ERROR, "Unsupported DRM format %d", format);
		return GS_UNKNOWN;
	}
}

static void vaapi_destroy_surface(struct vaapi_surface *out)
{
	for (uint32_t i = 0; i < out->num_textures; ++i) {
		if (out->textures[i]) {
			gs_texture_destroy(out->textures[i]);
			out->textures[i] = NULL;
		}
	}

	av_frame_free(&out->frame);
}

static bool vaapi_create_surface(struct vaapi_encoder *enc, struct vaapi_surface *out)
{
	int ret;
	VAStatus vas;
	VADRMPRIMESurfaceDescriptor desc;
	const AVPixFmtDescriptor *fmt_desc;
	bool ok = true;

	memset(out, 0, sizeof(*out));

	fmt_desc = av_pix_fmt_desc_get(enc->context->pix_fmt);
	if (!fmt_desc) {
		warn("Failed to get pix fmt descriptor");
		return false;
	}

	out->frame = av_frame_alloc();
	if (!out->frame) {
		warn("Failed to allocate hw frame");
		return false;
	}

	ret = av_hwframe_get_buffer(enc->vaframes_ref, out->frame, 0);
	if (ret < 0) {
		warn("Failed to get hw frame buffer: %s", av_err2str(ret));
		goto fail;
	}

	vas = vaExportSurfaceHandle(enc->va_dpy, (uintptr_t)out->frame->data[3], VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME_2,
				    VA_EXPORT_SURFACE_WRITE_ONLY | VA_EXPORT_SURFACE_SEPARATE_LAYERS, &desc);
	if (vas != VA_STATUS_SUCCESS) {
		warn("Failed to export VA surface handle: %s", vaErrorStr(vas));
		goto fail;
	}

	for (uint32_t i = 0; i < desc.num_layers; ++i) {
		unsigned int width = desc.width;
		unsigned int height = desc.height;
		if (i) {
			width /= 1 << fmt_desc->log2_chroma_w;
			height /= 1 << fmt_desc->log2_chroma_h;
		}

		out->textures[i] = gs_texture_create_from_dmabuf(
			width, height, desc.layers[i].drm_format, drm_to_gs_color_format(desc.layers[i].drm_format), 1,
			&desc.objects[desc.layers[i].object_index[0]].fd, &desc.layers[i].pitch[0],
			&desc.layers[i].offset[0], &desc.objects[desc.layers[i].object_index[0]].drm_format_modifier);

		if (!out->textures[i]) {
			warn("Failed to import VA surface texture");
			ok = false;
		}

		out->num_textures++;
	}

	for (uint32_t i = 0; i < desc.num_objects; ++i)
		close(desc.objects[i].fd);

	if (ok)
		return true;

fail:
	vaapi_destroy_surface(out);
	return false;
}

static inline void flush_remaining_packets(struct vaapi_encoder *enc)
{
	int r_pkt = 1;

	while (r_pkt) {
		if (avcodec_receive_packet(enc->context, enc->packet) < 0)
			break;

		if (r_pkt)
			av_packet_unref(enc->packet);
	}
}

static void vaapi_destroy(void *data)
{
	struct vaapi_encoder *enc = data;

	if (enc->initialized)
		flush_remaining_packets(enc);

	av_packet_free(&enc->packet);
	avcodec_free_context(&enc->context);
	av_frame_unref(enc->vframe);
	av_frame_free(&enc->vframe);
	av_buffer_unref(&enc->vaframes_ref);
	av_buffer_unref(&enc->vadevice_ref);
	da_free(enc->buffer);
	bfree(enc->header);
	bfree(enc->sei);

	bfree(enc);
}

static inline const char *vaapi_encoder_name(enum codec_type codec)
{
	if (codec == CODEC_H264) {
		return "h264_vaapi";
	} else if (codec == CODEC_HEVC) {
		return "hevc_vaapi";
	} else if (codec == CODEC_AV1) {
		return "av1_vaapi";
	}
	return NULL;
}

static void *vaapi_create_internal(obs_data_t *settings, obs_encoder_t *encoder, enum codec_type codec)
{
	struct vaapi_encoder *enc;

	enc = bzalloc(sizeof(*enc));
	enc->encoder = encoder;

	enc->codec = codec;
	enc->vaapi = avcodec_find_encoder_by_name(vaapi_encoder_name(codec));

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

static inline bool vaapi_test_texencode(struct vaapi_encoder *enc)
{
	struct vaapi_surface surface;

	if (obs_encoder_scaling_enabled(enc->encoder) && !obs_encoder_gpu_scaling_enabled(enc->encoder))
		return false;

	obs_enter_graphics();
	bool success = vaapi_create_surface(enc, &surface);
	vaapi_destroy_surface(&surface);
	obs_leave_graphics();
	return success;
}

static void *vaapi_create_tex_internal(obs_data_t *settings, obs_encoder_t *encoder, enum codec_type codec,
				       const char *fallback)
{
	void *enc = vaapi_create_internal(settings, encoder, codec);
	if (!enc) {
		return NULL;
	}
	if (!vaapi_test_texencode(enc)) {
		vaapi_destroy(enc);
		blog(LOG_WARNING, "VAAPI: Falling back to %s encoder", fallback);
		return obs_encoder_create_rerouted(encoder, fallback);
	}
	return enc;
}

static void *h264_vaapi_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	return vaapi_create_internal(settings, encoder, CODEC_H264);
}

static void *h264_vaapi_create_tex(obs_data_t *settings, obs_encoder_t *encoder)
{
	return vaapi_create_tex_internal(settings, encoder, CODEC_H264, "ffmpeg_vaapi");
}

static void *av1_vaapi_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	return vaapi_create_internal(settings, encoder, CODEC_AV1);
}

static void *av1_vaapi_create_tex(obs_data_t *settings, obs_encoder_t *encoder)
{
	return vaapi_create_tex_internal(settings, encoder, CODEC_AV1, "av1_ffmpeg_vaapi");
}

#ifdef ENABLE_HEVC
static void *hevc_vaapi_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	return vaapi_create_internal(settings, encoder, CODEC_HEVC);
}

static void *hevc_vaapi_create_tex(obs_data_t *settings, obs_encoder_t *encoder)
{
	return vaapi_create_tex_internal(settings, encoder, CODEC_HEVC, "hevc_ffmpeg_vaapi");
}
#endif

static inline void copy_data(AVFrame *pic, const struct encoder_frame *frame, int height, enum AVPixelFormat format)
{
	int h_chroma_shift, v_chroma_shift;
	av_pix_fmt_get_chroma_sub_sample(format, &h_chroma_shift, &v_chroma_shift);
	for (int plane = 0; plane < MAX_AV_PLANES; plane++) {
		if (!frame->data[plane])
			continue;

		int frame_rowsize = (int)frame->linesize[plane];
		int pic_rowsize = pic->linesize[plane];
		int bytes = frame_rowsize < pic_rowsize ? frame_rowsize : pic_rowsize;
		int plane_height = height >> (plane ? v_chroma_shift : 0);

		for (int y = 0; y < plane_height; y++) {
			int pos_frame = y * frame_rowsize;
			int pos_pic = y * pic_rowsize;

			memcpy(pic->data[plane] + pos_pic, frame->data[plane] + pos_frame, bytes);
		}
	}
}

static bool vaapi_encode_internal(struct vaapi_encoder *enc, AVFrame *frame, struct encoder_packet *packet,
				  bool *received_packet)
{
	int got_packet;
	int ret;

	ret = avcodec_send_frame(enc->context, frame);
	if (ret == 0 || ret == AVERROR(EAGAIN))
		ret = avcodec_receive_packet(enc->context, enc->packet);

	got_packet = (ret == 0);

	if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
		ret = 0;

	if (ret < 0) {
		warn("vaapi_encode: Error encoding: %s", av_err2str(ret));
		goto fail;
	}

	if (got_packet && enc->packet->size) {
		if (enc->first_packet) {
			uint8_t *new_packet;
			size_t size;

			enc->first_packet = false;

			switch (enc->codec) {
			case CODEC_HEVC:
#ifdef ENABLE_HEVC
				obs_extract_hevc_headers(enc->packet->data, enc->packet->size, &new_packet, &size,
							 &enc->header, &enc->header_size, &enc->sei, &enc->sei_size);
				break;
#else
				warn("vaapi_encode: HEVC codec is not supported");
				goto fail;
#endif
			case CODEC_H264:
				obs_extract_avc_headers(enc->packet->data, enc->packet->size, &new_packet, &size,
							&enc->header, &enc->header_size, &enc->sei, &enc->sei_size);
				break;

			case CODEC_AV1:
				obs_extract_av1_headers(enc->packet->data, enc->packet->size, &new_packet, &size,
							&enc->header, &enc->header_size);
				break;
			}

			da_copy_array(enc->buffer, new_packet, size);
			bfree(new_packet);
		} else {
			da_copy_array(enc->buffer, enc->packet->data, enc->packet->size);
		}

		packet->pts = enc->packet->pts;
		packet->dts = enc->packet->dts;
		packet->data = enc->buffer.array;
		packet->size = enc->buffer.num;
		packet->type = OBS_ENCODER_VIDEO;
#ifdef ENABLE_HEVC
		if (enc->codec == CODEC_HEVC) {
			packet->keyframe = obs_hevc_keyframe(packet->data, packet->size);
		} else
#endif
			if (enc->codec == CODEC_H264) {
			packet->keyframe = obs_avc_keyframe(packet->data, packet->size);
		} else if (enc->codec == CODEC_AV1) {
			packet->keyframe = obs_av1_keyframe(packet->data, packet->size);
		}
		*received_packet = true;
	}

	av_packet_unref(enc->packet);
	return true;

fail:
	av_packet_unref(enc->packet);
	return false;
}

static bool vaapi_encode_copy(void *data, struct encoder_frame *frame, struct encoder_packet *packet,
			      bool *received_packet)
{
	struct vaapi_encoder *enc = data;
	AVFrame *hwframe = NULL;
	int ret;

	*received_packet = false;

	hwframe = av_frame_alloc();
	if (!hwframe) {
		warn("vaapi_encode_copy: failed to allocate hw frame");
		return false;
	}

	ret = av_hwframe_get_buffer(enc->vaframes_ref, hwframe, 0);
	if (ret < 0) {
		warn("vaapi_encode_copy: failed to get buffer for hw frame: %s", av_err2str(ret));
		goto fail;
	}

	copy_data(enc->vframe, frame, enc->height, enc->context->pix_fmt);

	enc->vframe->pts = frame->pts;
	hwframe->pts = frame->pts;
	hwframe->width = enc->vframe->width;
	hwframe->height = enc->vframe->height;

	ret = av_hwframe_transfer_data(hwframe, enc->vframe, 0);
	if (ret < 0) {
		warn("vaapi_encode_copy: failed to upload hw frame: %s", av_err2str(ret));
		goto fail;
	}

	ret = av_frame_copy_props(hwframe, enc->vframe);
	if (ret < 0) {
		warn("vaapi_encode_copy: failed to copy props to hw frame: %s", av_err2str(ret));
		goto fail;
	}

	if (!vaapi_encode_internal(enc, hwframe, packet, received_packet))
		goto fail;

	av_frame_free(&hwframe);
	return true;

fail:
	av_frame_free(&hwframe);
	return false;
}

static bool vaapi_encode_tex(void *data, struct encoder_texture *texture, int64_t pts, uint64_t lock_key,
			     uint64_t *next_key, struct encoder_packet *packet, bool *received_packet)
{
	UNUSED_PARAMETER(lock_key);
	UNUSED_PARAMETER(next_key);

	struct vaapi_encoder *enc = data;
	struct vaapi_surface surface;
	int ret;

	*received_packet = false;

	obs_enter_graphics();

	// Destroyed piecemeal to avoid taking the graphics lock again.
	if (!vaapi_create_surface(enc, &surface)) {
		warn("vaapi_encode_tex: failed to create texture hw frame");
		obs_leave_graphics();
		return false;
	}

	for (uint32_t i = 0; i < surface.num_textures; ++i) {
		if (!texture->tex[i]) {
			warn("vaapi_encode_tex: unexpected number of textures");
			obs_leave_graphics();
			goto fail;
		}
		gs_copy_texture(surface.textures[i], texture->tex[i]);
		gs_texture_destroy(surface.textures[i]);
	}

	gs_flush();

	obs_leave_graphics();

	enc->vframe->pts = pts;

	ret = av_frame_copy_props(surface.frame, enc->vframe);
	if (ret < 0) {
		warn("vaapi_encode_tex: failed to copy props to hw frame: %s", av_err2str(ret));
		goto fail;
	}

	if (!vaapi_encode_internal(enc, surface.frame, packet, received_packet))
		goto fail;

	av_frame_free(&surface.frame);
	return true;

fail:
	av_frame_free(&surface.frame);
	return false;
}

static void set_visible(obs_properties_t *ppts, const char *name, bool visible)
{
	obs_property_t *p = obs_properties_get(ppts, name);
	obs_property_set_visible(p, visible);
}

static inline VAProfile vaapi_profile(enum codec_type codec)
{
	if (codec == CODEC_H264) {
		return VAProfileH264ConstrainedBaseline;
	} else if (codec == CODEC_AV1) {
		return VAProfileAV1Profile0;
#if ENABLE_HEVC
	} else if (codec == CODEC_HEVC) {
		return VAProfileHEVCMain;
#endif
	}
	return VAProfileNone;
}

static inline const char *vaapi_default_device(enum codec_type codec)
{
	if (codec == CODEC_H264) {
		return vaapi_get_h264_default_device();
	} else if (codec == CODEC_AV1) {
		return vaapi_get_av1_default_device();
#if ENABLE_HEVC
	} else if (codec == CODEC_HEVC) {
		return vaapi_get_hevc_default_device();
#endif
	}
	return NULL;
}

static void vaapi_defaults_internal(obs_data_t *settings, enum codec_type codec)
{
	const char *const device = vaapi_default_device(codec);
	obs_data_set_default_string(settings, "vaapi_device", device);
#ifdef ENABLE_HEVC
	if (codec == CODEC_HEVC)
		obs_data_set_default_int(settings, "profile", AV_PROFILE_HEVC_MAIN);
	else
#endif
		if (codec == CODEC_H264)
		obs_data_set_default_int(settings, "profile", AV_PROFILE_H264_HIGH);
	else if (codec == CODEC_AV1)
		obs_data_set_default_int(settings, "profile", AV_PROFILE_AV1_MAIN);
	obs_data_set_default_int(settings, "level", AV_LEVEL_UNKNOWN);
	obs_data_set_default_int(settings, "bitrate", 2500);
	obs_data_set_default_int(settings, "keyint_sec", 0);
	obs_data_set_default_int(settings, "bf", 0);
	obs_data_set_default_int(settings, "qp", 20);
	obs_data_set_default_int(settings, "maxrate", 0);

	int drm_fd = -1;
	VADisplay va_dpy = vaapi_open_device(&drm_fd, device, "vaapi_defaults");
	if (!va_dpy)
		return;

	const VAProfile profile = vaapi_profile(codec);
	if (vaapi_device_rc_supported(profile, va_dpy, VA_RC_CBR, device))
		obs_data_set_default_string(settings, "rate_control", "CBR");
	else if (vaapi_device_rc_supported(profile, va_dpy, VA_RC_VBR, device))
		obs_data_set_default_string(settings, "rate_control", "VBR");
	else
		obs_data_set_default_string(settings, "rate_control", "CQP");

	vaapi_close_device(&drm_fd, va_dpy);
}

static void h264_vaapi_defaults(obs_data_t *settings)
{
	vaapi_defaults_internal(settings, CODEC_H264);
}

static void av1_vaapi_defaults(obs_data_t *settings)
{
	vaapi_defaults_internal(settings, CODEC_AV1);
}

static void hevc_vaapi_defaults(obs_data_t *settings)
{
	vaapi_defaults_internal(settings, CODEC_HEVC);
}

static bool vaapi_device_modified(obs_properties_t *ppts, obs_property_t *p, obs_data_t *settings)
{
	UNUSED_PARAMETER(p);

	const char *device = obs_data_get_string(settings, "vaapi_device");
	int drm_fd = -1;
	VADisplay va_dpy = vaapi_open_device(&drm_fd, device, "vaapi_device_modified");
	int profile = obs_data_get_int(settings, "profile");
	obs_property_t *rc_p = obs_properties_get(ppts, "rate_control");

	obs_property_list_clear(rc_p);

	if (!va_dpy)
		goto fail;

	switch (profile) {
	case AV_PROFILE_H264_CONSTRAINED_BASELINE:
		if (!vaapi_display_h264_supported(va_dpy, device))
			goto fail;
		profile = VAProfileH264ConstrainedBaseline;
		break;
	case AV_PROFILE_H264_MAIN:
		if (!vaapi_display_h264_supported(va_dpy, device))
			goto fail;
		profile = VAProfileH264Main;
		break;
	case AV_PROFILE_H264_HIGH:
		if (!vaapi_display_h264_supported(va_dpy, device))
			goto fail;
		profile = VAProfileH264High;
		break;
	case AV_PROFILE_AV1_MAIN:
		if (!vaapi_display_av1_supported(va_dpy, device))
			goto fail;
		profile = VAProfileAV1Profile0;
		break;
#ifdef ENABLE_HEVC
	case AV_PROFILE_HEVC_MAIN:
		if (!vaapi_display_hevc_supported(va_dpy, device))
			goto fail;
		profile = VAProfileHEVCMain;
		break;
	case AV_PROFILE_HEVC_MAIN_10:
		if (!vaapi_display_hevc_supported(va_dpy, device))
			goto fail;
		profile = VAProfileHEVCMain10;
		break;
#endif
	}

	if (vaapi_device_rc_supported(profile, va_dpy, VA_RC_CBR, device))
		obs_property_list_add_string(rc_p, "CBR", "CBR");

	if (vaapi_device_rc_supported(profile, va_dpy, VA_RC_VBR, device))
		obs_property_list_add_string(rc_p, "VBR", "VBR");

	if (vaapi_device_rc_supported(profile, va_dpy, VA_RC_QVBR, device))
		obs_property_list_add_string(rc_p, "QVBR", "QVBR");

	if (vaapi_device_rc_supported(profile, va_dpy, VA_RC_CQP, device))
		obs_property_list_add_string(rc_p, "CQP", "CQP");

	set_visible(ppts, "bf", vaapi_device_bframe_supported(profile, va_dpy));

fail:
	vaapi_close_device(&drm_fd, va_dpy);
	return true;
}

static bool rate_control_modified(obs_properties_t *ppts, obs_property_t *p, obs_data_t *settings)
{
	UNUSED_PARAMETER(p);

	const char *rate_control = obs_data_get_string(settings, "rate_control");

	const rc_mode_t *rc_mode = get_rc_mode(rate_control);

	/* Set options visibility per Rate Control */
	set_visible(ppts, "qp", rc_mode->qp);
	set_visible(ppts, "bitrate", rc_mode->bitrate);
	set_visible(ppts, "maxrate", rc_mode->maxrate);

	return true;
}

static bool get_device_name_from_pci(struct pci_access *pacc, char *pci_slot, char *buf, int size)
{
	struct pci_filter filter;
	struct pci_dev *dev;
	char *name;

	pci_filter_init(pacc, &filter);
	if (pci_filter_parse_slot(&filter, pci_slot))
		return false;

	pci_scan_bus(pacc);
	for (dev = pacc->devices; dev; dev = dev->next) {
		if (pci_filter_match(&filter, dev)) {
			pci_fill_info(dev, PCI_FILL_IDENT);
			name = pci_lookup_name(pacc, buf, size, PCI_LOOKUP_DEVICE, dev->vendor_id, dev->device_id);
			strcpy(buf, name);
			return true;
		}
	}
	return false;
}

static bool vaapi_device_codec_supported(const char *path, enum codec_type codec)
{
	switch (codec) {
	case CODEC_H264:
		return vaapi_device_h264_supported(path);
	case CODEC_HEVC:
		return vaapi_device_hevc_supported(path);
	case CODEC_AV1:
		return vaapi_device_av1_supported(path);
	default:
		return false;
	}
}

static obs_properties_t *vaapi_properties_internal(enum codec_type codec)
{
	obs_properties_t *props = obs_properties_create();
	obs_property_t *list;

	list = obs_properties_add_list(props, "vaapi_device", obs_module_text("VAAPI.Device"), OBS_COMBO_TYPE_LIST,
				       OBS_COMBO_FORMAT_STRING);
	if (os_file_exists("/dev/dri/by-path")) {
		os_dir_t *by_path_dir = os_opendir("/dev/dri/by-path");
		struct pci_access *pacc = pci_alloc();
		struct os_dirent *file;
		char namebuf[1024];
		char pci_slot[13];
		char *type;

		pci_init(pacc);
		while ((file = os_readdir(by_path_dir)) != NULL) {
			// file_name pattern: pci-<pci_slot::12>-<type::{"card","render"}>
			char *file_name = file->d_name;
			if (strcmp(file_name, ".") == 0 || strcmp(file_name, "..") == 0)
				continue;

			char path[64] = {0};

			// Use the return value of snprintf to prevent truncation warning.
			int written = snprintf(path, 64, "/dev/dri/by-path/%s", file_name);
			if (written >= 64)
				blog(LOG_DEBUG, "obs-ffmpeg-vaapi: A format truncation may have occurred."
						" This can be ignored since it is quite improbable.");

			type = strrchr(file_name, '-');
			if (type == NULL)
				continue;
			else
				type++;

			if (strcmp(type, "render") == 0) {
				strncpy(pci_slot, file_name + 4, 12);
				pci_slot[12] = 0;
				bool name_found = get_device_name_from_pci(pacc, pci_slot, namebuf, sizeof(namebuf));

				if (!vaapi_device_codec_supported(path, codec))
					continue;

				if (!name_found)
					obs_property_list_add_string(list, path, path);
				else
					obs_property_list_add_string(list, namebuf, path);
			}
		}
		pci_cleanup(pacc);
		os_closedir(by_path_dir);
	}
	if (obs_property_list_item_count(list) == 0) {
		char path[32];
		for (int i = 28;; i++) {
			snprintf(path, sizeof(path), "/dev/dri/renderD1%d", i);
			if (access(path, F_OK) == 0) {
				char card[128];
				int ret = snprintf(card, sizeof(card), "Card%d: %s", i - 28, path);
				if (ret >= (int)sizeof(card))
					blog(LOG_DEBUG, "obs-ffmpeg-vaapi: A format truncation may have occurred."
							" This can be ignored since it is quite improbable.");

				if (!vaapi_device_h264_supported(path))
					continue;

				obs_property_list_add_string(list, card, path);
			} else {
				break;
			}
		}
	}

	obs_property_set_modified_callback(list, vaapi_device_modified);

	list = obs_properties_add_list(props, "profile", obs_module_text("Profile"), OBS_COMBO_TYPE_LIST,
				       OBS_COMBO_FORMAT_INT);
	if (codec == CODEC_HEVC) {
		obs_property_list_add_int(list, "Main", AV_PROFILE_HEVC_MAIN);
		obs_property_list_add_int(list, "Main10", AV_PROFILE_HEVC_MAIN_10);
	} else if (codec == CODEC_H264) {
		obs_property_list_add_int(list, "Constrained Baseline", AV_PROFILE_H264_CONSTRAINED_BASELINE);
		obs_property_list_add_int(list, "Main", AV_PROFILE_H264_MAIN);
		obs_property_list_add_int(list, "High", AV_PROFILE_H264_HIGH);
	} else if (codec == CODEC_AV1) {
		obs_property_list_add_int(list, "Main", AV_PROFILE_AV1_MAIN);
	}

	obs_property_set_modified_callback(list, vaapi_device_modified);

	list = obs_properties_add_list(props, "level", obs_module_text("Level"), OBS_COMBO_TYPE_LIST,
				       OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(list, "Auto", AV_LEVEL_UNKNOWN);
	if (codec == CODEC_H264) {
		obs_property_list_add_int(list, "3.0", 30);
		obs_property_list_add_int(list, "3.1", 31);
		obs_property_list_add_int(list, "4.0", 40);
		obs_property_list_add_int(list, "4.1", 41);
		obs_property_list_add_int(list, "4.2", 42);
		obs_property_list_add_int(list, "5.0", 50);
		obs_property_list_add_int(list, "5.1", 51);
		obs_property_list_add_int(list, "5.2", 52);
	} else if (codec == CODEC_HEVC) {
		obs_property_list_add_int(list, "3.0", 90);
		obs_property_list_add_int(list, "3.1", 93);
		obs_property_list_add_int(list, "4.0", 120);
		obs_property_list_add_int(list, "4.1", 123);
		obs_property_list_add_int(list, "5.0", 150);
		obs_property_list_add_int(list, "5.1", 153);
		obs_property_list_add_int(list, "5.2", 156);
	} else if (codec == CODEC_AV1) {
		obs_property_list_add_int(list, "3.0", 4);
		obs_property_list_add_int(list, "3.1", 5);
		obs_property_list_add_int(list, "4.0", 8);
		obs_property_list_add_int(list, "4.1", 9);
		obs_property_list_add_int(list, "5.0", 12);
		obs_property_list_add_int(list, "5.1", 13);
		obs_property_list_add_int(list, "5.2", 14);
		obs_property_list_add_int(list, "5.3", 15);
	}

	list = obs_properties_add_list(props, "rate_control", obs_module_text("RateControl"), OBS_COMBO_TYPE_LIST,
				       OBS_COMBO_FORMAT_STRING);

	obs_property_set_modified_callback(list, rate_control_modified);

	obs_property_t *p;
	p = obs_properties_add_int(props, "bitrate", obs_module_text("Bitrate"), 0, 300000, 50);
	obs_property_int_set_suffix(p, " Kbps");

	p = obs_properties_add_int(props, "maxrate", obs_module_text("MaxBitrate"), 0, 300000, 50);
	obs_property_int_set_suffix(p, " Kbps");

	obs_properties_add_int(props, "qp", "QP", 0, 51, 1);

	p = obs_properties_add_int(props, "keyint_sec", obs_module_text("KeyframeIntervalSec"), 0, 20, 1);
	obs_property_int_set_suffix(p, " s");

	obs_properties_add_int(props, "bf", obs_module_text("BFrames"), 0, 4, 1);

	obs_properties_add_text(props, "ffmpeg_opts", obs_module_text("FFmpegOpts"), OBS_TEXT_DEFAULT);

	return props;
}

static obs_properties_t *h264_vaapi_properties(void *unused)
{
	UNUSED_PARAMETER(unused);
	return vaapi_properties_internal(CODEC_H264);
}

static obs_properties_t *av1_vaapi_properties(void *unused)
{
	UNUSED_PARAMETER(unused);
	return vaapi_properties_internal(CODEC_AV1);
}

#ifdef ENABLE_HEVC
static obs_properties_t *hevc_vaapi_properties(void *unused)
{
	UNUSED_PARAMETER(unused);
	return vaapi_properties_internal(CODEC_HEVC);
}
#endif

static bool vaapi_extra_data(void *data, uint8_t **extra_data, size_t *size)
{
	struct vaapi_encoder *enc = data;

	*extra_data = enc->header;
	*size = enc->header_size;
	return true;
}

static bool vaapi_sei_data(void *data, uint8_t **extra_data, size_t *size)
{
	struct vaapi_encoder *enc = data;

	*extra_data = enc->sei;
	*size = enc->sei_size;
	return true;
}

struct obs_encoder_info h264_vaapi_encoder_info = {
	.id = "ffmpeg_vaapi",
	.type = OBS_ENCODER_VIDEO,
	.codec = "h264",
	.get_name = h264_vaapi_getname,
	.create = h264_vaapi_create,
	.destroy = vaapi_destroy,
	.encode = vaapi_encode_copy,
	.get_defaults = h264_vaapi_defaults,
	.get_properties = h264_vaapi_properties,
	.get_extra_data = vaapi_extra_data,
	.get_sei_data = vaapi_sei_data,
	.get_video_info = vaapi_video_info,
	.caps = OBS_ENCODER_CAP_INTERNAL,
};

struct obs_encoder_info h264_vaapi_encoder_tex_info = {
	.id = "ffmpeg_vaapi_tex",
	.type = OBS_ENCODER_VIDEO,
	.codec = "h264",
	.get_name = h264_vaapi_getname,
	.create = h264_vaapi_create_tex,
	.destroy = vaapi_destroy,
	.encode_texture2 = vaapi_encode_tex,
	.get_defaults = h264_vaapi_defaults,
	.get_properties = h264_vaapi_properties,
	.get_extra_data = vaapi_extra_data,
	.get_sei_data = vaapi_sei_data,
	.get_video_info = vaapi_video_info,
	.caps = OBS_ENCODER_CAP_PASS_TEXTURE,
};

struct obs_encoder_info av1_vaapi_encoder_info = {
	.id = "av1_ffmpeg_vaapi",
	.type = OBS_ENCODER_VIDEO,
	.codec = "av1",
	.get_name = av1_vaapi_getname,
	.create = av1_vaapi_create,
	.destroy = vaapi_destroy,
	.encode = vaapi_encode_copy,
	.get_defaults = av1_vaapi_defaults,
	.get_properties = av1_vaapi_properties,
	.get_extra_data = vaapi_extra_data,
	.get_sei_data = vaapi_sei_data,
	.get_video_info = vaapi_video_info,
	.caps = OBS_ENCODER_CAP_INTERNAL,
};

struct obs_encoder_info av1_vaapi_encoder_tex_info = {
	.id = "av1_ffmpeg_vaapi_tex",
	.type = OBS_ENCODER_VIDEO,
	.codec = "av1",
	.get_name = av1_vaapi_getname,
	.create = av1_vaapi_create_tex,
	.destroy = vaapi_destroy,
	.encode_texture2 = vaapi_encode_tex,
	.get_defaults = av1_vaapi_defaults,
	.get_properties = av1_vaapi_properties,
	.get_extra_data = vaapi_extra_data,
	.get_sei_data = vaapi_sei_data,
	.get_video_info = vaapi_video_info,
	.caps = OBS_ENCODER_CAP_PASS_TEXTURE,
};

#ifdef ENABLE_HEVC
struct obs_encoder_info hevc_vaapi_encoder_info = {
	.id = "hevc_ffmpeg_vaapi",
	.type = OBS_ENCODER_VIDEO,
	.codec = "hevc",
	.get_name = hevc_vaapi_getname,
	.create = hevc_vaapi_create,
	.destroy = vaapi_destroy,
	.encode = vaapi_encode_copy,
	.get_defaults = hevc_vaapi_defaults,
	.get_properties = hevc_vaapi_properties,
	.get_extra_data = vaapi_extra_data,
	.get_sei_data = vaapi_sei_data,
	.get_video_info = vaapi_video_info,
	.caps = OBS_ENCODER_CAP_INTERNAL,
};

struct obs_encoder_info hevc_vaapi_encoder_tex_info = {
	.id = "hevc_ffmpeg_vaapi_tex",
	.type = OBS_ENCODER_VIDEO,
	.codec = "hevc",
	.get_name = hevc_vaapi_getname,
	.create = hevc_vaapi_create_tex,
	.destroy = vaapi_destroy,
	.encode_texture2 = vaapi_encode_tex,
	.get_defaults = hevc_vaapi_defaults,
	.get_properties = hevc_vaapi_properties,
	.get_extra_data = vaapi_extra_data,
	.get_sei_data = vaapi_sei_data,
	.get_video_info = vaapi_video_info,
	.caps = OBS_ENCODER_CAP_PASS_TEXTURE,
};
#endif
