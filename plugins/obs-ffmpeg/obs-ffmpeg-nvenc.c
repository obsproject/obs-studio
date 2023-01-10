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

#include <obs-avc.h>
#ifdef ENABLE_HEVC
#include <obs-hevc.h>
#endif

#include "obs-ffmpeg-video-encoders.h"

#define do_log(level, format, ...)                          \
	blog(level, "[FFmpeg NVENC encoder: '%s'] " format, \
	     obs_encoder_get_name(enc->ffve.encoder), ##__VA_ARGS__)

#define warn(format, ...) do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...) do_log(LOG_INFO, format, ##__VA_ARGS__)
#define debug(format, ...) do_log(LOG_DEBUG, format, ##__VA_ARGS__)

struct nvenc_encoder {
	struct ffmpeg_video_encoder ffve;
#ifdef ENABLE_HEVC
	bool hevc;
#endif
	DARRAY(uint8_t) header;
	DARRAY(uint8_t) sei;
};

#ifdef __linux__
extern bool ubuntu_20_04_nvenc_fallback;
#endif

#define ENCODER_NAME_H264 "NVIDIA NVENC H.264 (FFmpeg)"
static const char *h264_nvenc_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return ENCODER_NAME_H264;
}

#ifdef ENABLE_HEVC
#define ENCODER_NAME_HEVC "NVIDIA NVENC HEVC (FFmpeg)"
static const char *hevc_nvenc_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return ENCODER_NAME_HEVC;
}
#endif

static inline bool valid_format(enum video_format format)
{
	switch (format) {
	case VIDEO_FORMAT_I420:
	case VIDEO_FORMAT_NV12:
	case VIDEO_FORMAT_I444:
	case VIDEO_FORMAT_I010:
	case VIDEO_FORMAT_P010:
		return true;
	default:
		return false;
	}
}

static void nvenc_video_info(void *data, struct video_scale_info *info)
{
	struct nvenc_encoder *enc = data;
	enum video_format pref_format;

	pref_format = obs_encoder_get_preferred_video_format(enc->ffve.encoder);

	if (!valid_format(pref_format)) {
		pref_format = valid_format(info->format) ? info->format
							 : VIDEO_FORMAT_NV12;
	}

	info->format = pref_format;
}

static void set_psycho_aq(struct nvenc_encoder *enc, bool psycho_aq)
{
	av_opt_set_int(enc->ffve.context->priv_data, "spatial-aq", psycho_aq,
		       0);
	av_opt_set_int(enc->ffve.context->priv_data, "temporal-aq", psycho_aq,
		       0);
}

static bool nvenc_update(struct nvenc_encoder *enc, obs_data_t *settings,
			 bool psycho_aq)
{
	const char *rc = obs_data_get_string(settings, "rate_control");
	int bitrate = (int)obs_data_get_int(settings, "bitrate");
	int cqp = (int)obs_data_get_int(settings, "cqp");
	int keyint_sec = (int)obs_data_get_int(settings, "keyint_sec");
	const char *preset = obs_data_get_string(settings, "preset");
	const char *preset2 = obs_data_get_string(settings, "preset2");
	const char *tuning = obs_data_get_string(settings, "tune");
	const char *multipass = obs_data_get_string(settings, "multipass");
	const char *profile = obs_data_get_string(settings, "profile");
	int gpu = (int)obs_data_get_int(settings, "gpu");
	bool cbr_override = obs_data_get_bool(settings, "cbr");
	int bf = (int)obs_data_get_int(settings, "bf");

	video_t *video = obs_encoder_video(enc->ffve.encoder);
	const struct video_output_info *voi = video_output_get_info(video);
	struct video_scale_info info;

	/* XXX: "cbr" setting has been deprecated */
	if (cbr_override) {
		warn("\"cbr\" setting has been deprecated for all encoders!  "
		     "Please set \"rate_control\" to \"CBR\" instead.  "
		     "Forcing CBR mode.  "
		     "(Note to all: this is why you shouldn't use strings for "
		     "common settings)");
		rc = "CBR";
	}

#ifdef __linux__
	bool use_old_nvenc = ubuntu_20_04_nvenc_fallback;
#else
#define use_old_nvenc false
#endif

	info.format = voi->format;
	info.colorspace = voi->colorspace;
	info.range = voi->range;

	nvenc_video_info(enc, &info);

	av_opt_set_int(enc->ffve.context->priv_data, "cbr", false, 0);
	av_opt_set(enc->ffve.context->priv_data, "profile", profile, 0);

	if (use_old_nvenc || (obs_data_has_user_value(settings, "preset") &&
			      !obs_data_has_user_value(settings, "preset2"))) {

		if (astrcmpi(preset, "mq") == 0) {
			preset = "hq";
		}
		av_opt_set(enc->ffve.context->priv_data, "preset", preset, 0);
	} else {
		av_opt_set(enc->ffve.context->priv_data, "preset", preset2, 0);
		av_opt_set(enc->ffve.context->priv_data, "tune", tuning, 0);
		av_opt_set(enc->ffve.context->priv_data, "multipass", multipass,
			   0);
	}

	if (astrcmpi(rc, "cqp") == 0) {
		bitrate = 0;
		enc->ffve.context->global_quality = cqp;

	} else if (astrcmpi(rc, "lossless") == 0) {
		bitrate = 0;
		cqp = 0;

		av_opt_set(enc->ffve.context->priv_data, "tune", "lossless", 0);
		av_opt_set(enc->ffve.context->priv_data, "multipass",
			   "disabled", 0);

	} else if (astrcmpi(rc, "vbr") != 0) { /* CBR by default */
		av_opt_set_int(enc->ffve.context->priv_data, "cbr", true, 0);
		const int64_t rate = bitrate * INT64_C(1000);
		enc->ffve.context->rc_max_rate = rate;
		enc->ffve.context->rc_min_rate = rate;
		cqp = 0;
	}

	av_opt_set(enc->ffve.context->priv_data, "level", "auto", 0);
	av_opt_set_int(enc->ffve.context->priv_data, "gpu", gpu, 0);

	set_psycho_aq(enc, psycho_aq);

	enc->ffve.context->max_b_frames = bf;

	const char *ffmpeg_opts = obs_data_get_string(settings, "ffmpeg_opts");
	ffmpeg_video_encoder_update(&enc->ffve, bitrate, keyint_sec, voi, &info,
				    ffmpeg_opts);

	info("settings:\n"
	     "\tencoder:      %s\n"
	     "\trate_control: %s\n"
	     "\tbitrate:      %d\n"
	     "\tcqp:          %d\n"
	     "\tkeyint:       %d\n"
	     "\tpreset:       %s\n"
	     "\ttuning:       %s\n"
	     "\tmultipass:    %s\n"
	     "\tprofile:      %s\n"
	     "\twidth:        %d\n"
	     "\theight:       %d\n"
	     "\tb-frames:     %d\n"
	     "\tpsycho-aq:    %d\n"
	     "\tGPU:          %d\n",
	     enc->ffve.enc_name, rc, bitrate, cqp, enc->ffve.context->gop_size,
	     preset2, tuning, multipass, profile, enc->ffve.context->width,
	     enc->ffve.height, enc->ffve.context->max_b_frames, psycho_aq, gpu);

	return ffmpeg_video_encoder_init_codec(&enc->ffve);
}

static bool nvenc_reconfigure(void *data, obs_data_t *settings)
{
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(58, 19, 101)
	struct nvenc_encoder *enc = data;

	const int64_t bitrate = obs_data_get_int(settings, "bitrate");
	const char *rc = obs_data_get_string(settings, "rate_control");
	bool cbr = astrcmpi(rc, "CBR") == 0;
	bool vbr = astrcmpi(rc, "VBR") == 0;
	if (cbr || vbr) {
		const int64_t rate = bitrate * 1000;
		enc->ffve.context->bit_rate = rate;
		enc->ffve.context->rc_max_rate = rate;
	}
#else
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(settings);
#endif
	return true;
}

static void nvenc_destroy(void *data)
{
	struct nvenc_encoder *enc = data;
	ffmpeg_video_encoder_free(&enc->ffve);
	da_free(enc->header);
	da_free(enc->sei);
	bfree(enc);
}

static void on_init_error(void *data, int ret)
{
	struct nvenc_encoder *enc = data;
	struct dstr error_message = {0};

	int64_t gpu;
	if (av_opt_get_int(enc->ffve.context->priv_data, "gpu", 0, &gpu) < 0) {
		gpu = -1;
	}

	dstr_copy(&error_message, obs_module_text("NVENC.Error"));
	dstr_replace(&error_message, "%1", av_err2str(ret));
	dstr_cat(&error_message, "\r\n\r\n");

	if (gpu > 0) {
		// if a non-zero GPU failed, almost always
		// user error. tell then to fix it.
		char gpu_str[16];
		snprintf(gpu_str, sizeof(gpu_str) - 1, "%d", (int)gpu);
		gpu_str[sizeof(gpu_str) - 1] = 0;

		dstr_cat(&error_message, obs_module_text("NVENC.BadGPUIndex"));
		dstr_replace(&error_message, "%1", gpu_str);
	} else if (ret == AVERROR_EXTERNAL) {
		// special case for common NVENC error
		dstr_cat(&error_message, obs_module_text("NVENC.GenericError"));
	} else {
		dstr_cat(&error_message, obs_module_text("NVENC.CheckDrivers"));
	}

	obs_encoder_set_last_error(enc->ffve.encoder, error_message.array);
	dstr_free(&error_message);
}

static void on_first_packet(void *data, AVPacket *pkt, struct darray *da)
{
	struct nvenc_encoder *enc = data;

	darray_free(da);
#ifdef ENABLE_HEVC
	if (enc->hevc) {
		obs_extract_hevc_headers(pkt->data, pkt->size,
					 (uint8_t **)&da->array, &da->num,
					 &enc->header.array, &enc->header.num,
					 &enc->sei.array, &enc->sei.num);
	} else
#endif
	{
		obs_extract_avc_headers(pkt->data, pkt->size,
					(uint8_t **)&da->array, &da->num,
					&enc->header.array, &enc->header.num,
					&enc->sei.array, &enc->sei.num);
	}
	da->capacity = da->num;
}

static void *nvenc_create_internal(obs_data_t *settings, obs_encoder_t *encoder,
				   bool psycho_aq, bool hevc)
{
	struct nvenc_encoder *enc = bzalloc(sizeof(*enc));

#ifdef ENABLE_HEVC
	enc->hevc = hevc;
	if (hevc) {
		if (!ffmpeg_video_encoder_init(&enc->ffve, enc, encoder,
					       "hevc_nvenc", "nvenc_hevc",
					       ENCODER_NAME_HEVC, on_init_error,
					       on_first_packet))
			goto fail;
	} else
#else
	UNUSED_PARAMETER(hevc);
#endif
	{
		if (!ffmpeg_video_encoder_init(&enc->ffve, enc, encoder,
					       "h264_nvenc", "nvenc_h264",
					       ENCODER_NAME_H264, on_init_error,
					       on_first_packet))
			goto fail;
	}

	if (!nvenc_update(enc, settings, psycho_aq))
		goto fail;

	return enc;

fail:
	nvenc_destroy(enc);
	return NULL;
}

static void *h264_nvenc_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	video_t *video = obs_encoder_video(encoder);
	const struct video_output_info *voi = video_output_get_info(video);
	switch (voi->format) {
	case VIDEO_FORMAT_I010:
	case VIDEO_FORMAT_P010: {
		const char *const text =
			obs_module_text("NVENC.10bitUnsupported");
		obs_encoder_set_last_error(encoder, text);
		blog(LOG_ERROR, "[NVENC encoder] %s", text);
		return NULL;
	}
	default:
		switch (voi->colorspace) {
		case VIDEO_CS_2100_PQ:
		case VIDEO_CS_2100_HLG: {
			const char *const text =
				obs_module_text("NVENC.8bitUnsupportedHdr");
			obs_encoder_set_last_error(encoder, text);
			blog(LOG_ERROR, "[NVENC encoder] %s", text);
			return NULL;
		}
		}
	}

	bool psycho_aq = obs_data_get_bool(settings, "psycho_aq");
	void *enc = nvenc_create_internal(settings, encoder, psycho_aq, false);
	if ((enc == NULL) && psycho_aq) {
		blog(LOG_WARNING,
		     "[NVENC encoder] nvenc_create_internal failed, "
		     "trying again without Psycho Visual Tuning");
		enc = nvenc_create_internal(settings, encoder, false, false);
	}

	return enc;
}

#ifdef ENABLE_HEVC
static void *hevc_nvenc_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	video_t *video = obs_encoder_video(encoder);
	const struct video_output_info *voi = video_output_get_info(video);
	switch (voi->format) {
	case VIDEO_FORMAT_I010: {
		const char *const text =
			obs_module_text("NVENC.I010Unsupported");
		obs_encoder_set_last_error(encoder, text);
		blog(LOG_ERROR, "[NVENC encoder] %s", text);
		return NULL;
	}
	case VIDEO_FORMAT_P010:
		break;
	default:
		switch (voi->colorspace) {
		case VIDEO_CS_2100_PQ:
		case VIDEO_CS_2100_HLG: {
			const char *const text =
				obs_module_text("NVENC.8bitUnsupportedHdr");
			obs_encoder_set_last_error(encoder, text);
			blog(LOG_ERROR, "[NVENC encoder] %s", text);
			return NULL;
		}
		}
	}

	bool psycho_aq = obs_data_get_bool(settings, "psycho_aq");
	void *enc = nvenc_create_internal(settings, encoder, psycho_aq, true);
	if ((enc == NULL) && psycho_aq) {
		blog(LOG_WARNING,
		     "[NVENC encoder] nvenc_create_internal failed, "
		     "trying again without Psycho Visual Tuning");
		enc = nvenc_create_internal(settings, encoder, false, true);
	}

	return enc;
}
#endif

static bool nvenc_encode(void *data, struct encoder_frame *frame,
			 struct encoder_packet *packet, bool *received_packet)
{
	struct nvenc_encoder *enc = data;
	return ffmpeg_video_encode(&enc->ffve, frame, packet, received_packet);
}

enum codec_type {
	CODEC_H264,
	CODEC_HEVC,
	CODEC_AV1,
};

static void nvenc_defaults_base(enum codec_type codec, obs_data_t *settings)
{
	obs_data_set_default_int(settings, "bitrate", 2500);
	obs_data_set_default_int(settings, "max_bitrate", 5000);
	obs_data_set_default_int(settings, "keyint_sec", 0);
	obs_data_set_default_int(settings, "cqp", 20);
	obs_data_set_default_string(settings, "rate_control", "CBR");
	obs_data_set_default_string(settings, "preset2", "p5");
	obs_data_set_default_string(settings, "multipass", "qres");
	obs_data_set_default_string(settings, "tune", "hq");
	obs_data_set_default_string(settings, "profile",
				    codec != CODEC_H264 ? "main" : "high");
	obs_data_set_default_bool(settings, "psycho_aq", true);
	obs_data_set_default_int(settings, "gpu", 0);
	obs_data_set_default_int(settings, "bf", 2);
	obs_data_set_default_bool(settings, "repeat_headers", false);
}

void h264_nvenc_defaults(obs_data_t *settings)
{
	nvenc_defaults_base(CODEC_H264, settings);
}

void hevc_nvenc_defaults(obs_data_t *settings)
{
	nvenc_defaults_base(CODEC_HEVC, settings);
}

void av1_nvenc_defaults(obs_data_t *settings)
{
	nvenc_defaults_base(CODEC_AV1, settings);
}

static bool rate_control_modified(obs_properties_t *ppts, obs_property_t *p,
				  obs_data_t *settings)
{
	const char *rc = obs_data_get_string(settings, "rate_control");
	bool cqp = astrcmpi(rc, "CQP") == 0;
	bool vbr = astrcmpi(rc, "VBR") == 0;
	bool lossless = astrcmpi(rc, "lossless") == 0;

	p = obs_properties_get(ppts, "bitrate");
	obs_property_set_visible(p, !cqp && !lossless);
	p = obs_properties_get(ppts, "max_bitrate");
	obs_property_set_visible(p, vbr);
	p = obs_properties_get(ppts, "cqp");
	obs_property_set_visible(p, cqp);
	p = obs_properties_get(ppts, "preset2");
	obs_property_set_visible(p, !lossless);
	p = obs_properties_get(ppts, "tune");
	obs_property_set_visible(p, !lossless);

	return true;
}

obs_properties_t *nvenc_properties_internal(enum codec_type codec, bool ffmpeg)
{
	obs_properties_t *props = obs_properties_create();
	obs_property_t *p;

#ifdef __linux__
	bool use_old_nvenc = ubuntu_20_04_nvenc_fallback;
#else
#define use_old_nvenc false
#endif

	p = obs_properties_add_list(props, "rate_control",
				    obs_module_text("RateControl"),
				    OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(p, "CBR", "CBR");
	obs_property_list_add_string(p, "CQP", "CQP");
	obs_property_list_add_string(p, "VBR", "VBR");
	obs_property_list_add_string(p, obs_module_text("Lossless"),
				     "lossless");

	obs_property_set_modified_callback(p, rate_control_modified);

	p = obs_properties_add_int(props, "bitrate", obs_module_text("Bitrate"),
				   50, 300000, 50);
	obs_property_int_set_suffix(p, " Kbps");
	p = obs_properties_add_int(props, "max_bitrate",
				   obs_module_text("MaxBitrate"), 50, 300000,
				   50);
	obs_property_int_set_suffix(p, " Kbps");

	obs_properties_add_int(props, "cqp", obs_module_text("NVENC.CQLevel"),
			       1, codec == CODEC_AV1 ? 63 : 51, 1);

	p = obs_properties_add_int(props, "keyint_sec",
				   obs_module_text("KeyframeIntervalSec"), 0,
				   10, 1);
	obs_property_int_set_suffix(p, " s");

	p = obs_properties_add_list(props, use_old_nvenc ? "preset" : "preset2",
				    obs_module_text("Preset"),
				    OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);

#define add_preset(val)                                                        \
	obs_property_list_add_string(p, obs_module_text("NVENC.Preset2." val), \
				     val)
	if (use_old_nvenc) {
		add_preset("mq");
		add_preset("hq");
		add_preset("default");
		add_preset("hp");
		add_preset("ll");
		add_preset("llhq");
		add_preset("llhp");
	} else {
		add_preset("p1");
		add_preset("p2");
		add_preset("p3");
		add_preset("p4");
		add_preset("p5");
		add_preset("p6");
		add_preset("p7");
	}
#undef add_preset

	if (!use_old_nvenc) {
		p = obs_properties_add_list(props, "tune",
					    obs_module_text("Tuning"),
					    OBS_COMBO_TYPE_LIST,
					    OBS_COMBO_FORMAT_STRING);

#define add_tune(val)                                                         \
	obs_property_list_add_string(p, obs_module_text("NVENC.Tuning." val), \
				     val)
		add_tune("hq");
		add_tune("ll");
		add_tune("ull");
#undef add_tune

		p = obs_properties_add_list(props, "multipass",
					    obs_module_text("NVENC.Multipass"),
					    OBS_COMBO_TYPE_LIST,
					    OBS_COMBO_FORMAT_STRING);

#define add_multipass(val)            \
	obs_property_list_add_string( \
		p, obs_module_text("NVENC.Multipass." val), val)
		add_multipass("disabled");
		add_multipass("qres");
		add_multipass("fullres");
#undef add_multipass
	}

	p = obs_properties_add_list(props, "profile",
				    obs_module_text("Profile"),
				    OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);

#define add_profile(val) obs_property_list_add_string(p, val, val)
	if (codec == CODEC_HEVC) {
		add_profile("main10");
		add_profile("main");
	} else if (codec == CODEC_AV1) {
		add_profile("main");
	} else {
		add_profile("high");
		add_profile("main");
		add_profile("baseline");
	}
#undef add_profile

	if (!ffmpeg) {
		p = obs_properties_add_bool(props, "lookahead",
					    obs_module_text("NVENC.LookAhead"));
		obs_property_set_long_description(
			p, obs_module_text("NVENC.LookAhead.ToolTip"));
		p = obs_properties_add_bool(props, "repeat_headers",
					    "repeat_headers");
		obs_property_set_visible(p, false);
	}
	p = obs_properties_add_bool(
		props, "psycho_aq",
		obs_module_text("NVENC.PsychoVisualTuning"));
	obs_property_set_long_description(
		p, obs_module_text("NVENC.PsychoVisualTuning.ToolTip"));

	obs_properties_add_int(props, "gpu", obs_module_text("GPU"), 0, 8, 1);

	obs_properties_add_int(props, "bf", obs_module_text("BFrames"), 0, 4,
			       1);

	return props;
}

obs_properties_t *h264_nvenc_properties(void *unused)
{
	UNUSED_PARAMETER(unused);
	return nvenc_properties_internal(CODEC_H264, false);
}

#ifdef ENABLE_HEVC
obs_properties_t *hevc_nvenc_properties(void *unused)
{
	UNUSED_PARAMETER(unused);
	return nvenc_properties_internal(CODEC_HEVC, false);
}
#endif

obs_properties_t *av1_nvenc_properties(void *unused)
{
	UNUSED_PARAMETER(unused);
	return nvenc_properties_internal(CODEC_AV1, false);
}

obs_properties_t *h264_nvenc_properties_ffmpeg(void *unused)
{
	UNUSED_PARAMETER(unused);
	return nvenc_properties_internal(CODEC_H264, true);
}

#ifdef ENABLE_HEVC
obs_properties_t *hevc_nvenc_properties_ffmpeg(void *unused)
{
	UNUSED_PARAMETER(unused);
	return nvenc_properties_internal(CODEC_HEVC, true);
}
#endif

static bool nvenc_extra_data(void *data, uint8_t **extra_data, size_t *size)
{
	struct nvenc_encoder *enc = data;

	*extra_data = enc->header.array;
	*size = enc->header.num;
	return true;
}

static bool nvenc_sei_data(void *data, uint8_t **extra_data, size_t *size)
{
	struct nvenc_encoder *enc = data;

	*extra_data = enc->sei.array;
	*size = enc->sei.num;
	return true;
}

struct obs_encoder_info h264_nvenc_encoder_info = {
	.id = "ffmpeg_nvenc",
	.type = OBS_ENCODER_VIDEO,
	.codec = "h264",
	.get_name = h264_nvenc_getname,
	.create = h264_nvenc_create,
	.destroy = nvenc_destroy,
	.encode = nvenc_encode,
	.update = nvenc_reconfigure,
	.get_defaults = h264_nvenc_defaults,
	.get_properties = h264_nvenc_properties_ffmpeg,
	.get_extra_data = nvenc_extra_data,
	.get_sei_data = nvenc_sei_data,
	.get_video_info = nvenc_video_info,
#ifdef _WIN32
	.caps = OBS_ENCODER_CAP_DYN_BITRATE | OBS_ENCODER_CAP_INTERNAL,
#else
	.caps = OBS_ENCODER_CAP_DYN_BITRATE,
#endif
};

#ifdef ENABLE_HEVC
struct obs_encoder_info hevc_nvenc_encoder_info = {
	.id = "ffmpeg_hevc_nvenc",
	.type = OBS_ENCODER_VIDEO,
	.codec = "hevc",
	.get_name = hevc_nvenc_getname,
	.create = hevc_nvenc_create,
	.destroy = nvenc_destroy,
	.encode = nvenc_encode,
	.update = nvenc_reconfigure,
	.get_defaults = hevc_nvenc_defaults,
	.get_properties = hevc_nvenc_properties_ffmpeg,
	.get_extra_data = nvenc_extra_data,
	.get_sei_data = nvenc_sei_data,
	.get_video_info = nvenc_video_info,
#ifdef _WIN32
	.caps = OBS_ENCODER_CAP_DYN_BITRATE | OBS_ENCODER_CAP_INTERNAL,
#else
	.caps = OBS_ENCODER_CAP_DYN_BITRATE,
#endif
};
#endif
