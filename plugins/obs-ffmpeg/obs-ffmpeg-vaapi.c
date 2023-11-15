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

#include <libavutil/avutil.h>

#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(55, 27, 100)

#include <util/darray.h>
#include <util/dstr.h>
#include <util/base.h>
#include <util/platform.h>
#include <media-io/video-io.h>
#include <obs-module.h>
#include <obs-avc.h>

#include <unistd.h>

#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libavutil/hwcontext.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>

#include <pci/pci.h>

#include "vaapi-utils.h"
#include "obs-ffmpeg-formats.h"

#define do_log(level, format, ...)                          \
	blog(level, "[FFmpeg VAAPI encoder: '%s'] " format, \
	     obs_encoder_get_name(enc->encoder), ##__VA_ARGS__)

#define warn(format, ...) do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...) do_log(LOG_INFO, format, ##__VA_ARGS__)
#define debug(format, ...) do_log(LOG_DEBUG, format, ##__VA_ARGS__)

struct vaapi_encoder {
	obs_encoder_t *encoder;

	AVBufferRef *vadevice_ref;
	AVBufferRef *vaframes_ref;

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

static const char *vaapi_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "FFmpeg VAAPI H.264";
}

static inline bool valid_format(enum video_format format)
{
	return format == VIDEO_FORMAT_NV12;
}

static void vaapi_video_info(void *data, struct video_scale_info *info)
{
	struct vaapi_encoder *enc = data;
	enum video_format pref_format;

	pref_format = obs_encoder_get_preferred_video_format(enc->encoder);

	if (!valid_format(pref_format)) {
		pref_format = valid_format(info->format) ? info->format
							 : VIDEO_FORMAT_NV12;
	}

	info->format = pref_format;
}

static bool vaapi_init_codec(struct vaapi_encoder *enc, const char *path)
{
	int ret;

	ret = av_hwdevice_ctx_create(&enc->vadevice_ref, AV_HWDEVICE_TYPE_VAAPI,
				     path, NULL, 0);
	if (ret < 0) {
		warn("Failed to create VAAPI device context: %s",
		     av_err2str(ret));
		return false;
	}

	enc->vaframes_ref = av_hwframe_ctx_alloc(enc->vadevice_ref);
	if (!enc->vaframes_ref) {
		warn("Failed to alloc HW frames context");
		return false;
	}

	AVHWFramesContext *frames_ctx =
		(AVHWFramesContext *)enc->vaframes_ref->data;
	frames_ctx->format = AV_PIX_FMT_VAAPI;
	frames_ctx->sw_format = AV_PIX_FMT_NV12;
	frames_ctx->width = enc->context->width;
	frames_ctx->height = enc->context->height;
	frames_ctx->initial_pool_size = 20;

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
	static const rc_mode_t RC_MODES[] = {
		{.name = "CBR", .qp = false, .bitrate = true, .maxrate = false},
		{.name = "CQP", .qp = true, .bitrate = false, .maxrate = false},
		{.name = "VBR", .qp = false, .bitrate = true, .maxrate = true},
		NULL};

	const rc_mode_t *rc_mode = RC_MODES;

	while (!!rc_mode && strcmp(rc_mode->name, name) != 0)
		rc_mode++;

	return rc_mode ? rc_mode : RC_MODES;
}

static bool vaapi_update(void *data, obs_data_t *settings)
{
	struct vaapi_encoder *enc = data;

	const char *device = obs_data_get_string(settings, "vaapi_device");

	const char *rate_control =
		obs_data_get_string(settings, "rate_control");
	const rc_mode_t *rc_mode = get_rc_mode(rate_control);
	bool cbr = strcmp(rc_mode->name, "CBR") == 0;

	int profile = (int)obs_data_get_int(settings, "profile");
	int bf = (int)obs_data_get_int(settings, "bf");
	int qp = rc_mode->qp ? (int)obs_data_get_int(settings, "qp") : 0;

	av_opt_set_int(enc->context->priv_data, "qp", qp, 0);

	int level = (int)obs_data_get_int(settings, "level");
	int bitrate = rc_mode->bitrate
			      ? (int)obs_data_get_int(settings, "bitrate")
			      : 0;
	int maxrate = rc_mode->maxrate
			      ? (int)obs_data_get_int(settings, "maxrate")
			      : 0;
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

	vaapi_video_info(enc, &info);

	enc->context->profile = profile;
	enc->context->max_b_frames = bf;
	enc->context->level = level;
	enc->context->bit_rate = bitrate * 1000;
	enc->context->rc_max_rate = maxrate * 1000;

	enc->context->width = obs_encoder_get_width(enc->encoder);
	enc->context->height = obs_encoder_get_height(enc->encoder);

	enc->context->time_base = (AVRational){voi->fps_den, voi->fps_num};
	const enum AVPixelFormat pix_fmt =
		obs_to_ffmpeg_video_format(info.format);
	enc->context->pix_fmt = pix_fmt;
	enc->context->color_range = info.range == VIDEO_RANGE_FULL
					    ? AVCOL_RANGE_JPEG
					    : AVCOL_RANGE_MPEG;

	enum AVColorSpace colorspace = AVCOL_SPC_UNSPECIFIED;
	switch (info.colorspace) {
	case VIDEO_CS_601:
		enc->context->color_trc = AVCOL_TRC_SMPTE170M;
		enc->context->color_primaries = AVCOL_PRI_SMPTE170M;
		colorspace = AVCOL_SPC_SMPTE170M;
		break;
	case VIDEO_CS_DEFAULT:
	case VIDEO_CS_709:
		enc->context->color_trc = AVCOL_TRC_BT709;
		enc->context->color_primaries = AVCOL_PRI_BT709;
		colorspace = AVCOL_SPC_BT709;
		break;
	case VIDEO_CS_SRGB:
		enc->context->color_trc = AVCOL_TRC_IEC61966_2_1;
		enc->context->color_primaries = AVCOL_PRI_BT709;
		colorspace = AVCOL_SPC_BT709;
		break;
	}

	enc->context->colorspace = colorspace;
	enc->context->chroma_sample_location =
		determine_chroma_location(pix_fmt, colorspace);

	if (keyint_sec > 0) {
		enc->context->gop_size =
			keyint_sec * voi->fps_num / voi->fps_den;
	} else {
		enc->context->gop_size = 120;
	}

	enc->height = enc->context->height;

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
	     "\tb-frames:     %d\n",
	     device, rate_control, profile, level, qp, bitrate, maxrate,
	     enc->context->gop_size, enc->context->width, enc->context->height,
	     enc->context->max_b_frames);

	return vaapi_init_codec(enc, device);
}

static inline void flush_remaining_packets(struct vaapi_encoder *enc)
{
	int r_pkt = 1;

	while (r_pkt) {
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57, 40, 101)
		if (avcodec_receive_packet(enc->context, enc->packet) < 0)
			break;
#else
		if (avcodec_encode_video2(enc->context, enc->packet, NULL,
					  &r_pkt) < 0)
			break;
#endif

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

static void *vaapi_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	struct vaapi_encoder *enc;
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 9, 100)
	avcodec_register_all();
#endif

	enc = bzalloc(sizeof(*enc));
	enc->encoder = encoder;

	enc->vaapi = avcodec_find_encoder_by_name("h264_vaapi");

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

static bool vaapi_encode(void *data, struct encoder_frame *frame,
			 struct encoder_packet *packet, bool *received_packet)
{
	struct vaapi_encoder *enc = data;
	AVFrame *hwframe = NULL;
	int got_packet;
	int ret;

	hwframe = av_frame_alloc();
	if (!hwframe) {
		warn("vaapi_encode: failed to allocate hw frame");
		return false;
	}

	ret = av_hwframe_get_buffer(enc->vaframes_ref, hwframe, 0);
	if (ret < 0) {
		warn("vaapi_encode: failed to get buffer for hw frame: %s",
		     av_err2str(ret));
		goto fail;
	}

	copy_data(enc->vframe, frame, enc->height, enc->context->pix_fmt);

	enc->vframe->pts = frame->pts;
	hwframe->pts = frame->pts;
	hwframe->width = enc->vframe->width;
	hwframe->height = enc->vframe->height;

	ret = av_hwframe_transfer_data(hwframe, enc->vframe, 0);
	if (ret < 0) {
		warn("vaapi_encode: failed to upload hw frame: %s",
		     av_err2str(ret));
		goto fail;
	}

	ret = av_frame_copy_props(hwframe, enc->vframe);
	if (ret < 0) {
		warn("vaapi_encode: failed to copy props to hw frame: %s",
		     av_err2str(ret));
		goto fail;
	}

#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57, 40, 101)
	ret = avcodec_send_frame(enc->context, hwframe);
	if (ret == 0)
		ret = avcodec_receive_packet(enc->context, enc->packet);

	got_packet = (ret == 0);

	if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
		ret = 0;
#else
	ret = avcodec_encode_video2(enc->context, enc->packet, hwframe,
				    &got_packet);
#endif
	if (ret < 0) {
		warn("vaapi_encode: Error encoding: %s", av_err2str(ret));
		goto fail;
	}

	if (got_packet && enc->packet->size) {
		if (enc->first_packet) {
			uint8_t *new_packet;
			size_t size;

			enc->first_packet = false;
			obs_extract_avc_headers(enc->packet->data,
						enc->packet->size, &new_packet,
						&size, &enc->header,
						&enc->header_size, &enc->sei,
						&enc->sei_size);

			da_copy_array(enc->buffer, new_packet, size);
			bfree(new_packet);
		} else {
			da_copy_array(enc->buffer, enc->packet->data,
				      enc->packet->size);
		}

		packet->pts = enc->packet->pts;
		packet->dts = enc->packet->dts;
		packet->data = enc->buffer.array;
		packet->size = enc->buffer.num;
		packet->type = OBS_ENCODER_VIDEO;
		packet->keyframe = obs_avc_keyframe(packet->data, packet->size);
		*received_packet = true;
	} else {
		*received_packet = false;
	}

	av_packet_unref(enc->packet);
	av_frame_free(&hwframe);
	return true;

fail:
	av_frame_free(&hwframe);
	return false;
}

static void set_visible(obs_properties_t *ppts, const char *name, bool visible)
{
	obs_property_t *p = obs_properties_get(ppts, name);
	obs_property_set_visible(p, visible);
}

static void vaapi_defaults(obs_data_t *settings)
{
	const char *device = vaapi_get_h264_default_device();

	obs_data_set_default_string(settings, "vaapi_device", device);
	obs_data_set_default_int(settings, "profile",
				 FF_PROFILE_H264_CONSTRAINED_BASELINE);
	obs_data_set_default_int(settings, "level", 40);
	obs_data_set_default_int(settings, "bitrate", 2500);
	obs_data_set_default_int(settings, "keyint_sec", 0);
	obs_data_set_default_int(settings, "bf", 0);
	obs_data_set_default_int(settings, "rendermode", 0);
	obs_data_set_default_int(settings, "qp", 20);
	obs_data_set_default_int(settings, "maxrate", 0);

	int drm_fd = -1;
	VADisplay va_dpy = vaapi_open_device(&drm_fd, device, "vaapi_defaults");
	if (!va_dpy)
		return;

	if (vaapi_device_rc_supported(VAProfileH264ConstrainedBaseline, va_dpy,
				      VA_RC_CBR, device))
		obs_data_set_default_string(settings, "rate_control", "CBR");
	else if (vaapi_device_rc_supported(VAProfileH264ConstrainedBaseline,
					   va_dpy, VA_RC_VBR, device))
		obs_data_set_default_string(settings, "rate_control", "VBR");
	else
		obs_data_set_default_string(settings, "rate_control", "CQP");

	vaapi_close_device(&drm_fd, va_dpy);
}

static bool vaapi_device_modified(obs_properties_t *ppts, obs_property_t *p,
				  obs_data_t *settings)
{
	UNUSED_PARAMETER(p);

	const char *device = obs_data_get_string(settings, "vaapi_device");
	int drm_fd = -1;
	VADisplay va_dpy =
		vaapi_open_device(&drm_fd, device, "vaapi_device_modified");
	int profile = obs_data_get_int(settings, "profile");
	obs_property_t *rc_p = obs_properties_get(ppts, "rate_control");

	obs_property_list_clear(rc_p);

	if (!va_dpy || !vaapi_display_h264_supported(va_dpy, device))
		goto fail;

	switch (profile) {
	case FF_PROFILE_H264_CONSTRAINED_BASELINE:
		profile = VAProfileH264ConstrainedBaseline;
		break;
	case FF_PROFILE_H264_MAIN:
		profile = VAProfileH264Main;
		break;
	case FF_PROFILE_H264_HIGH:
		profile = VAProfileH264High;
		break;
	}

	if (vaapi_device_rc_supported(profile, va_dpy, VA_RC_CBR, device))
		obs_property_list_add_string(rc_p, "CBR (default)", "CBR");

	if (vaapi_device_rc_supported(profile, va_dpy, VA_RC_VBR, device))
		obs_property_list_add_string(rc_p, "VBR", "VBR");

	if (vaapi_device_rc_supported(profile, va_dpy, VA_RC_CQP, device))
		obs_property_list_add_string(rc_p, "CQP", "CQP");

fail:
	vaapi_close_device(&drm_fd, va_dpy);
	return true;
}

static bool rate_control_modified(obs_properties_t *ppts, obs_property_t *p,
				  obs_data_t *settings)
{
	UNUSED_PARAMETER(p);

	const char *rate_control =
		obs_data_get_string(settings, "rate_control");

	const rc_mode_t *rc_mode = get_rc_mode(rate_control);

	/* Set options visibility per Rate Control */
	set_visible(ppts, "qp", rc_mode->qp);
	set_visible(ppts, "bitrate", rc_mode->bitrate);
	set_visible(ppts, "maxrate", rc_mode->maxrate);

	return true;
}

static bool get_device_name_from_pci(struct pci_access *pacc, char *pci_slot,
				     char *buf, int size)
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
			name = pci_lookup_name(pacc, buf, size,
					       PCI_LOOKUP_DEVICE,
					       dev->vendor_id, dev->device_id);
			strcpy(buf, name);
			return true;
		}
	}
	return false;
}

static obs_properties_t *vaapi_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *props = obs_properties_create();
	obs_property_t *list;

	list = obs_properties_add_list(props, "vaapi_device",
				       obs_module_text("VAAPI.Device"),
				       OBS_COMBO_TYPE_LIST,
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
			if (strcmp(file_name, ".") == 0 ||
			    strcmp(file_name, "..") == 0)
				continue;

			char path[64] = {0};

			// Use the return value of snprintf to prevent truncation warning.
			int written = snprintf(path, 64, "/dev/dri/by-path/%s",
					       file_name);
			if (written >= 64)
				blog(LOG_DEBUG,
				     "obs-ffmpeg-vaapi: A format truncation may have occurred."
				     " This can be ignored since it is quite improbable.");

			type = strrchr(file_name, '-');
			if (type == NULL)
				continue;
			else
				type++;

			if (strcmp(type, "render") == 0) {
				strncpy(pci_slot, file_name + 4, 12);
				pci_slot[12] = 0;
				bool name_found = get_device_name_from_pci(
					pacc, pci_slot, namebuf,
					sizeof(namebuf));

				if (!vaapi_device_h264_supported(path))
					continue;

				if (!name_found)
					obs_property_list_add_string(list, path,
								     path);
				else
					obs_property_list_add_string(
						list, namebuf, path);
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
				int ret = snprintf(card, sizeof(card),
						   "Card%d: %s", i - 28, path);
				if (ret >= (int)sizeof(card))
					blog(LOG_DEBUG,
					     "obs-ffmpeg-vaapi: A format truncation may have occurred."
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

	list = obs_properties_add_list(props, "profile",
				       obs_module_text("Profile"),
				       OBS_COMBO_TYPE_LIST,
				       OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(list, "Constrained Baseline (default)",
				  FF_PROFILE_H264_CONSTRAINED_BASELINE);
	obs_property_list_add_int(list, "Main", FF_PROFILE_H264_MAIN);
	obs_property_list_add_int(list, "High", FF_PROFILE_H264_HIGH);

	obs_property_set_modified_callback(list, vaapi_device_modified);

	list = obs_properties_add_list(props, "level", obs_module_text("Level"),
				       OBS_COMBO_TYPE_LIST,
				       OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(list, "Auto", FF_LEVEL_UNKNOWN);
	obs_property_list_add_int(list, "3.0", 30);
	obs_property_list_add_int(list, "3.1", 31);
	obs_property_list_add_int(list, "4.0 (default) (Compatibility mode)",
				  40);
	obs_property_list_add_int(list, "4.1", 41);
	obs_property_list_add_int(list, "4.2", 42);
	obs_property_list_add_int(list, "5.0", 50);
	obs_property_list_add_int(list, "5.1", 51);
	obs_property_list_add_int(list, "5.2", 52);

	list = obs_properties_add_list(props, "rate_control",
				       obs_module_text("RateControl"),
				       OBS_COMBO_TYPE_LIST,
				       OBS_COMBO_FORMAT_STRING);

	obs_property_set_modified_callback(list, rate_control_modified);

	obs_property_t *p;
	p = obs_properties_add_int(props, "bitrate", obs_module_text("Bitrate"),
				   0, 300000, 50);
	obs_property_int_set_suffix(p, " Kbps");

	p = obs_properties_add_int(
		props, "maxrate", obs_module_text("MaxBitrate"), 0, 300000, 50);
	obs_property_int_set_suffix(p, " Kbps");

	obs_properties_add_int(props, "qp", "QP", 0, 51, 1);

	p = obs_properties_add_int(props, "keyint_sec",
				   obs_module_text("KeyframeIntervalSec"), 0,
				   20, 1);
	obs_property_int_set_suffix(p, " s");

	return props;
}

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

struct obs_encoder_info vaapi_encoder_info = {
	.id = "ffmpeg_vaapi",
	.type = OBS_ENCODER_VIDEO,
	.codec = "h264",
	.get_name = vaapi_getname,
	.create = vaapi_create,
	.destroy = vaapi_destroy,
	.encode = vaapi_encode,
	.get_defaults = vaapi_defaults,
	.get_properties = vaapi_properties,
	.get_extra_data = vaapi_extra_data,
	.get_sei_data = vaapi_sei_data,
	.get_video_info = vaapi_video_info,
};

#endif
