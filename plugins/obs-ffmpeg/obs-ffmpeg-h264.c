/******************************************************************************
    Copyright (C) 2023 by Neal Gompa <neal@gompa.dev>
    Partly derived from obs-ffmpeg-av1.c by Hugh Bailey <obs.jim@gmail.com>

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

#define do_log(level, format, ...) \
	blog(level, "[H.264 encoder: '%s'] " format, obs_encoder_get_name(enc->ffve.encoder), ##__VA_ARGS__)

#define error(format, ...) do_log(LOG_ERROR, format, ##__VA_ARGS__)
#define warn(format, ...) do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...) do_log(LOG_INFO, format, ##__VA_ARGS__)
#define debug(format, ...) do_log(LOG_DEBUG, format, ##__VA_ARGS__)

enum h264_encoder_type {
	H264_ENCODER_TYPE_OH264,
};

struct h264_encoder {
	struct ffmpeg_video_encoder ffve;
	enum h264_encoder_type type;

	DARRAY(uint8_t) header;
};

static const char *oh264_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "OpenH264";
}

static void h264_video_info(void *data, struct video_scale_info *info)
{
	UNUSED_PARAMETER(data);

	// OpenH264 only supports I420
	info->format = VIDEO_FORMAT_I420;
}

static bool h264_update(struct h264_encoder *enc, obs_data_t *settings)
{
	const char *profile = obs_data_get_string(settings, "profile");
	int bitrate = (int)obs_data_get_int(settings, "bitrate");
	int keyint_sec = 0;              // This is not supported by OpenH264
	const char *rc_mode = "quality"; // We only want to use quality mode
	int allow_skip_frames = 1;       // This is required for quality mode

	video_t *video = obs_encoder_video(enc->ffve.encoder);
	const struct video_output_info *voi = video_output_get_info(video);
	struct video_scale_info info;

	info.format = voi->format;
	info.colorspace = voi->colorspace;
	info.range = voi->range;

	enc->ffve.context->thread_count = 0;

	h264_video_info(enc, &info);

	av_opt_set(enc->ffve.context->priv_data, "rc_mode", rc_mode, 0);
	av_opt_set(enc->ffve.context->priv_data, "profile", profile, 0);
	av_opt_set_int(enc->ffve.context->priv_data, "allow_skip_frames", allow_skip_frames, 0);

	const char *ffmpeg_opts = obs_data_get_string(settings, "ffmpeg_opts");
	ffmpeg_video_encoder_update(&enc->ffve, bitrate, keyint_sec, voi, &info, ffmpeg_opts);
	info("settings:\n"
	     "\tencoder:      %s\n"
	     "\trc_mode:      %s\n"
	     "\tbitrate:      %d\n"
	     "\tprofile:      %s\n"
	     "\twidth:        %d\n"
	     "\theight:       %d\n"
	     "\tffmpeg opts:  %s\n",
	     enc->ffve.enc_name, rc_mode, bitrate, profile, enc->ffve.context->width, enc->ffve.height, ffmpeg_opts);

	enc->ffve.context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	return ffmpeg_video_encoder_init_codec(&enc->ffve);
}

static void h264_destroy(void *data)
{
	struct h264_encoder *enc = data;

	ffmpeg_video_encoder_free(&enc->ffve);
	da_free(enc->header);
	bfree(enc);
}

static void on_first_packet(void *data, AVPacket *pkt, struct darray *da)
{
	struct h264_encoder *enc = data;

	da_copy_array(enc->header, enc->ffve.context->extradata, enc->ffve.context->extradata_size);

	darray_copy_array(1, da, pkt->data, pkt->size);
}

static void *h264_create_internal(obs_data_t *settings, obs_encoder_t *encoder, const char *enc_lib,
				  const char *enc_name)
{
	video_t *video = obs_encoder_video(encoder);
	const struct video_output_info *voi = video_output_get_info(video);

	switch (voi->format) {
	// planar 4:2:0 formats
	case VIDEO_FORMAT_I420: // three-plane
	case VIDEO_FORMAT_NV12: // two-plane, luma and packed chroma
	// packed 4:2:2 formats
	case VIDEO_FORMAT_YVYU:
	case VIDEO_FORMAT_YUY2: // YUYV
	case VIDEO_FORMAT_UYVY:
	// packed uncompressed formats
	case VIDEO_FORMAT_RGBA:
	case VIDEO_FORMAT_BGRA:
	case VIDEO_FORMAT_BGRX:
	case VIDEO_FORMAT_BGR3:
	case VIDEO_FORMAT_Y800: // grayscale
	// planar 4:4:4
	case VIDEO_FORMAT_I444:
	// planar 4:2:2
	case VIDEO_FORMAT_I422:
	// planar 4:2:0 with alpha
	case VIDEO_FORMAT_I40A:
	// planar 4:2:2 with alpha
	case VIDEO_FORMAT_I42A:
	// planar 4:4:4 with alpha
	case VIDEO_FORMAT_YUVA:
	// packed 4:4:4 with alpha
	case VIDEO_FORMAT_AYUV:
		break;
	default:; // Make the compiler do the right thing
		const char *const text = obs_module_text("H264.UnsupportedVideoFormat");
		obs_encoder_set_last_error(encoder, text);
		blog(LOG_ERROR, "[H.264 encoder] %s", text);
		return NULL;
	}

	switch (voi->colorspace) {
	case VIDEO_CS_DEFAULT:
	case VIDEO_CS_709:
		break;
	default:; // Make the compiler do the right thing
		const char *const text = obs_module_text("H264.UnsupportedColorSpace");
		obs_encoder_set_last_error(encoder, text);
		blog(LOG_ERROR, "[H.264 encoder] %s", text);
		return NULL;
	}

	struct h264_encoder *enc = bzalloc(sizeof(*enc));

	if (strcmp(enc_lib, "libopenh264") == 0)
		enc->type = H264_ENCODER_TYPE_OH264;

	if (!ffmpeg_video_encoder_init(&enc->ffve, enc, encoder, enc_lib, NULL, enc_name, NULL, on_first_packet))
		goto fail;
	if (!h264_update(enc, settings))
		goto fail;

	return enc;

fail:
	h264_destroy(enc);
	return NULL;
}

static void *oh264_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	return h264_create_internal(settings, encoder, "libopenh264", "OpenH264");
}

static bool h264_encode(void *data, struct encoder_frame *frame, struct encoder_packet *packet, bool *received_packet)
{
	struct h264_encoder *enc = data;
	return ffmpeg_video_encode(&enc->ffve, frame, packet, received_packet);
}

void h264_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "bitrate", 2500);
	obs_data_set_default_string(settings, "profile", "main");
}

obs_properties_t *h264_properties(enum h264_encoder_type type)
{
	UNUSED_PARAMETER(type); // Only one encoder right now...
	obs_properties_t *props = obs_properties_create();
	obs_property_t *p;

	p = obs_properties_add_list(props, "profile", obs_module_text("Profile"), OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(p, "constrained_baseline", "constrained_baseline");
	obs_property_list_add_string(p, "main", "main");
	obs_property_list_add_string(p, "high", "high");

	p = obs_properties_add_int(props, "bitrate", obs_module_text("Bitrate"), 50, 300000, 50);
	obs_property_int_set_suffix(p, " Kbps");

	obs_properties_add_text(props, "ffmpeg_opts", obs_module_text("FFmpegOpts"), OBS_TEXT_DEFAULT);

	return props;
}

obs_properties_t *oh264_properties(void *unused)
{
	UNUSED_PARAMETER(unused);
	return h264_properties(H264_ENCODER_TYPE_OH264);
}

static bool h264_extra_data(void *data, uint8_t **extra_data, size_t *size)
{
	struct h264_encoder *enc = data;

	*extra_data = enc->header.array;
	*size = enc->header.num;
	return true;
}

struct obs_encoder_info oh264_encoder_info = {
	.id = "ffmpeg_openh264",
	.type = OBS_ENCODER_VIDEO,
	.codec = "h264",
	.get_name = oh264_getname,
	.create = oh264_create,
	.destroy = h264_destroy,
	.encode = h264_encode,
	.get_defaults = h264_defaults,
	.get_properties = oh264_properties,
	.get_extra_data = h264_extra_data,
	.get_video_info = h264_video_info,
};
