#include "nvenc-helpers.h"

#include <util/dstr.h>

/*
 * Compatibility encoder objects for pre-31.0 encoder compatibility.
 *
 * All they do is update the settings object, and then reroute to one of the
 * new encoder implementations.
 *
 * This should be removed once NVENC settings are migrated directly and
 * backwards-compatibility is no longer required.
 */

/* ------------------------------------------------------------------------- */
/* Actual redirector implementation.                                         */

static void migrate_settings(obs_data_t *settings)
{
	const char *preset = obs_data_get_string(settings, "preset2");
	obs_data_set_string(settings, "preset", preset);

	obs_data_set_bool(settings, "adaptive_quantization",
			  obs_data_get_bool(settings, "psycho_aq"));

	if (obs_data_has_user_value(settings, "gpu") &&
	    num_encoder_devices() > 1) {
		obs_data_set_int(settings, "device",
				 obs_data_get_int(settings, "gpu"));
	}
}

static void *nvenc_reroute(enum codec_type codec, obs_data_t *settings,
			   obs_encoder_t *encoder, bool texture)
{
	/* Update settings object to v2 encoder configuration */
	migrate_settings(settings);

	switch (codec) {
	case CODEC_H264:
		return obs_encoder_create_rerouted(
			encoder,
			texture ? "obs_nvenc_h264_tex" : "obs_nvenc_h264_soft");
	case CODEC_HEVC:
		return obs_encoder_create_rerouted(
			encoder,
			texture ? "obs_nvenc_hevc_tex" : "obs_nvenc_hevc_soft");
	case CODEC_AV1:
		return obs_encoder_create_rerouted(
			encoder,
			texture ? "obs_nvenc_av1_tex" : "obs_nvenc_av1_soft");
	}

	return NULL;
}

/* ------------------------------------------------------------------------- */

static const char *h264_nvenc_get_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return "NVIDIA NVENC H.264";
}

#ifdef ENABLE_HEVC
static const char *hevc_nvenc_get_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return "NVIDIA NVENC HEVC";
}
#endif

static const char *av1_nvenc_get_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return "NVIDIA NVENC AV1";
}

static void *h264_nvenc_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	return nvenc_reroute(CODEC_H264, settings, encoder, true);
}

#ifdef ENABLE_HEVC
static void *hevc_nvenc_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	return nvenc_reroute(CODEC_HEVC, settings, encoder, true);
}
#endif

static void *av1_nvenc_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	return nvenc_reroute(CODEC_AV1, settings, encoder, true);
}

static void *h264_nvenc_soft_create(obs_data_t *settings,
				    obs_encoder_t *encoder)
{
	return nvenc_reroute(CODEC_H264, settings, encoder, false);
}

#ifdef ENABLE_HEVC
static void *hevc_nvenc_soft_create(obs_data_t *settings,
				    obs_encoder_t *encoder)
{
	return nvenc_reroute(CODEC_HEVC, settings, encoder, false);
}
#endif

static void *av1_nvenc_soft_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	return nvenc_reroute(CODEC_AV1, settings, encoder, false);
}

static void nvenc_defaults_base(enum codec_type codec, obs_data_t *settings)
{
	/* Defaults from legacy FFmpeg encoder */
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

static void h264_nvenc_defaults(obs_data_t *settings)
{
	nvenc_defaults_base(CODEC_H264, settings);
}

#ifdef ENABLE_HEVC
static void hevc_nvenc_defaults(obs_data_t *settings)
{
	nvenc_defaults_base(CODEC_HEVC, settings);
}
#endif

static void av1_nvenc_defaults(obs_data_t *settings)
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

static obs_properties_t *nvenc_properties_internal(enum codec_type codec)
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

	obs_properties_add_int(props, "cqp", obs_module_text("CQLevel"), 1,
			       codec == CODEC_AV1 ? 63 : 51, 1);

	p = obs_properties_add_int(props, "keyint_sec",
				   obs_module_text("KeyframeIntervalSec"), 0,
				   10, 1);
	obs_property_int_set_suffix(p, " s");

	p = obs_properties_add_list(props, "preset2", obs_module_text("Preset"),
				    OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);

#define add_preset(val) \
	obs_property_list_add_string(p, obs_module_text("Preset." val), val)

	add_preset("p1");
	add_preset("p2");
	add_preset("p3");
	add_preset("p4");
	add_preset("p5");
	add_preset("p6");
	add_preset("p7");
#undef add_preset

	p = obs_properties_add_list(props, "tune", obs_module_text("Tuning"),
				    OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);

#define add_tune(val) \
	obs_property_list_add_string(p, obs_module_text("Tuning." val), val)
	add_tune("hq");
	add_tune("ll");
	add_tune("ull");
#undef add_tune

	p = obs_properties_add_list(props, "multipass",
				    obs_module_text("Multipass"),
				    OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);

#define add_multipass(val) \
	obs_property_list_add_string(p, obs_module_text("Multipass." val), val)
	add_multipass("disabled");
	add_multipass("qres");
	add_multipass("fullres");
#undef add_multipass

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

	p = obs_properties_add_bool(props, "lookahead",
				    obs_module_text("LookAhead"));
	obs_property_set_long_description(p,
					  obs_module_text("LookAhead.ToolTip"));
	p = obs_properties_add_bool(props, "repeat_headers", "repeat_headers");
	obs_property_set_visible(p, false);

	p = obs_properties_add_bool(props, "psycho_aq",
				    obs_module_text("PsychoVisualTuning"));
	obs_property_set_long_description(
		p, obs_module_text("PsychoVisualTuning.ToolTip"));

	obs_properties_add_int(props, "gpu", obs_module_text("GPU"), 0, 8, 1);

	obs_properties_add_int(props, "bf", obs_module_text("BFrames"), 0, 4,
			       1);

	return props;
}

static obs_properties_t *h264_nvenc_properties(void *unused)
{
	UNUSED_PARAMETER(unused);
	return nvenc_properties_internal(CODEC_H264);
}

#ifdef ENABLE_HEVC
static obs_properties_t *hevc_nvenc_properties(void *unused)
{
	UNUSED_PARAMETER(unused);
	return nvenc_properties_internal(CODEC_HEVC);
}
#endif

static obs_properties_t *av1_nvenc_properties(void *unused)
{
	UNUSED_PARAMETER(unused);
	return nvenc_properties_internal(CODEC_AV1);
}

/* ------------------------------------------------------------------------- */
/* Stubs for required - but unused - functions.                              */

static void fake_nvenc_destroy(void *p)
{
	UNUSED_PARAMETER(p);
}

static bool fake_encode(void *data, struct encoder_frame *frame,
			struct encoder_packet *packet, bool *received_packet)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(frame);
	UNUSED_PARAMETER(packet);
	UNUSED_PARAMETER(received_packet);

	return true;
}

static bool fake_encode_tex2(void *data, struct encoder_texture *texture,
			     int64_t pts, uint64_t lock_key, uint64_t *next_key,
			     struct encoder_packet *packet,
			     bool *received_packet)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(texture);
	UNUSED_PARAMETER(pts);
	UNUSED_PARAMETER(lock_key);
	UNUSED_PARAMETER(next_key);
	UNUSED_PARAMETER(packet);
	UNUSED_PARAMETER(received_packet);

	return true;
}

struct obs_encoder_info compat_h264_nvenc_info = {
	.id = "jim_nvenc",
	.codec = "h264",
	.type = OBS_ENCODER_VIDEO,
	.caps = OBS_ENCODER_CAP_PASS_TEXTURE | OBS_ENCODER_CAP_DYN_BITRATE |
		OBS_ENCODER_CAP_ROI | OBS_ENCODER_CAP_DEPRECATED,
	.get_name = h264_nvenc_get_name,
	.create = h264_nvenc_create,
	.destroy = fake_nvenc_destroy,
	.encode_texture2 = fake_encode_tex2,
	.get_defaults = h264_nvenc_defaults,
	.get_properties = h264_nvenc_properties,
};

#ifdef ENABLE_HEVC
struct obs_encoder_info compat_hevc_nvenc_info = {
	.id = "jim_hevc_nvenc",
	.codec = "hevc",
	.type = OBS_ENCODER_VIDEO,
	.caps = OBS_ENCODER_CAP_PASS_TEXTURE | OBS_ENCODER_CAP_DYN_BITRATE |
		OBS_ENCODER_CAP_ROI | OBS_ENCODER_CAP_DEPRECATED,
	.get_name = hevc_nvenc_get_name,
	.create = hevc_nvenc_create,
	.destroy = fake_nvenc_destroy,
	.encode_texture2 = fake_encode_tex2,
	.get_defaults = hevc_nvenc_defaults,
	.get_properties = hevc_nvenc_properties,
};
#endif

struct obs_encoder_info compat_av1_nvenc_info = {
	.id = "jim_av1_nvenc",
	.codec = "av1",
	.type = OBS_ENCODER_VIDEO,
	.caps = OBS_ENCODER_CAP_PASS_TEXTURE | OBS_ENCODER_CAP_DYN_BITRATE |
		OBS_ENCODER_CAP_ROI | OBS_ENCODER_CAP_DEPRECATED,
	.get_name = av1_nvenc_get_name,
	.create = av1_nvenc_create,
	.destroy = fake_nvenc_destroy,
	.encode_texture2 = fake_encode_tex2,
	.get_defaults = av1_nvenc_defaults,
	.get_properties = av1_nvenc_properties,
};

struct obs_encoder_info compat_h264_nvenc_soft_info = {
	.id = "obs_nvenc_h264_cuda",
	.codec = "h264",
	.type = OBS_ENCODER_VIDEO,
	.caps = OBS_ENCODER_CAP_DYN_BITRATE | OBS_ENCODER_CAP_ROI |
		OBS_ENCODER_CAP_DEPRECATED,
	.get_name = h264_nvenc_get_name,
	.create = h264_nvenc_soft_create,
	.destroy = fake_nvenc_destroy,
	.encode = fake_encode,
	.get_defaults = h264_nvenc_defaults,
	.get_properties = h264_nvenc_properties,
};

#ifdef ENABLE_HEVC
struct obs_encoder_info compat_hevc_nvenc_soft_info = {
	.id = "obs_nvenc_hevc_cuda",
	.codec = "hevc",
	.type = OBS_ENCODER_VIDEO,
	.caps = OBS_ENCODER_CAP_DYN_BITRATE | OBS_ENCODER_CAP_ROI |
		OBS_ENCODER_CAP_DEPRECATED,
	.get_name = hevc_nvenc_get_name,
	.create = hevc_nvenc_soft_create,
	.destroy = fake_nvenc_destroy,
	.encode = fake_encode,
	.get_defaults = hevc_nvenc_defaults,
	.get_properties = hevc_nvenc_properties,
};
#endif

struct obs_encoder_info compat_av1_nvenc_soft_info = {
	.id = "obs_nvenc_av1_cuda",
	.codec = "av1",
	.type = OBS_ENCODER_VIDEO,
	.caps = OBS_ENCODER_CAP_DYN_BITRATE | OBS_ENCODER_CAP_ROI |
		OBS_ENCODER_CAP_DEPRECATED,
	.get_name = av1_nvenc_get_name,
	.create = av1_nvenc_soft_create,
	.destroy = fake_nvenc_destroy,
	.encode = fake_encode,
	.get_defaults = av1_nvenc_defaults,
	.get_properties = av1_nvenc_properties,
};

void register_compat_encoders(void)
{
	obs_register_encoder(&compat_h264_nvenc_info);
	obs_register_encoder(&compat_h264_nvenc_soft_info);
#ifdef ENABLE_HEVC
	obs_register_encoder(&compat_hevc_nvenc_info);
	obs_register_encoder(&compat_hevc_nvenc_soft_info);
#endif
	if (is_codec_supported(CODEC_AV1)) {
		obs_register_encoder(&compat_av1_nvenc_info);
		obs_register_encoder(&compat_av1_nvenc_soft_info);
	}

#ifdef REGISTER_FFMPEG_IDS
	compat_h264_nvenc_soft_info.id = "ffmpeg_nvenc";
	obs_register_encoder(&compat_h264_nvenc_soft_info);
#ifdef ENABLE_HEVC
	compat_hevc_nvenc_soft_info.id = "ffmpeg_hevc_nvenc";
	obs_register_encoder(&compat_hevc_nvenc_soft_info);
#endif
#endif
}
