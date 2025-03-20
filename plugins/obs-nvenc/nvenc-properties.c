#include "nvenc-internal.h"

void nvenc_properties_read(struct nvenc_properties *props, obs_data_t *settings)
{
	props->bitrate = obs_data_get_int(settings, "bitrate");
	props->max_bitrate = obs_data_get_int(settings, "max_bitrate");
	props->keyint_sec = obs_data_get_int(settings, "keyint_sec");
	props->cqp = obs_data_get_int(settings, "cqp");
	props->device = obs_data_get_int(settings, "device");
	props->bf = obs_data_get_int(settings, "bf");
	props->bframe_ref_mode = obs_data_get_int(settings, "bframe_ref_mode");
	props->split_encode = obs_data_get_int(settings, "split_encode");
	props->target_quality = obs_data_get_int(settings, "target_quality");

	props->rate_control = obs_data_get_string(settings, "rate_control");
	props->preset = obs_data_get_string(settings, "preset");
	props->profile = obs_data_get_string(settings, "profile");
	props->tune = obs_data_get_string(settings, "tune");
	props->multipass = obs_data_get_string(settings, "multipass");

	props->adaptive_quantization = obs_data_get_bool(settings, "adaptive_quantization");
	props->lookahead = obs_data_get_bool(settings, "lookahead");
	props->disable_scenecut = obs_data_get_bool(settings, "disable_scenecut");
	props->repeat_headers = obs_data_get_bool(settings, "repeat_headers");
	props->force_cuda_tex = obs_data_get_bool(settings, "force_cuda_tex");

	if (obs_data_has_user_value(settings, "opts")) {
		props->opts_str = obs_data_get_string(settings, "opts");
		props->opts = obs_parse_options(props->opts_str);
	}

	/* Retain settings object until destroyed since we use its strings. */
	obs_data_addref(settings);
	props->data = settings;
}

static void nvenc_defaults_base(enum codec_type codec, obs_data_t *settings)
{
	struct encoder_caps *caps = get_encoder_caps(codec);

	obs_data_set_default_int(settings, "bitrate", 10000);
	obs_data_set_default_int(settings, "max_bitrate", 10000);
	obs_data_set_default_int(settings, "target_quality", 20);
	obs_data_set_default_int(settings, "keyint_sec", 0);
	obs_data_set_default_int(settings, "cqp", 20);
	obs_data_set_default_int(settings, "device", -1);
	obs_data_set_default_int(settings, "bf", caps->bframes > 0 ? 2 : 0);

	obs_data_set_default_string(settings, "rate_control", "cbr");
	obs_data_set_default_string(settings, "preset", "p5");
	obs_data_set_default_string(settings, "multipass", "qres");
	obs_data_set_default_string(settings, "tune", "hq");
	obs_data_set_default_string(settings, "profile", codec != CODEC_H264 ? "main" : "high");

	obs_data_set_default_bool(settings, "adaptive_quantization", true);
	obs_data_set_default_bool(settings, "lookahead", caps->lookahead);

	/* Hidden options */
	obs_data_set_default_bool(settings, "repeat_headers", false);
	obs_data_set_default_bool(settings, "force_cuda_tex", false);
	obs_data_set_default_bool(settings, "disable_scenecut", false);
}

void h264_nvenc_defaults(obs_data_t *settings)
{
	nvenc_defaults_base(CODEC_H264, settings);
}

#ifdef ENABLE_HEVC
void hevc_nvenc_defaults(obs_data_t *settings)
{
	nvenc_defaults_base(CODEC_HEVC, settings);
}
#endif

void av1_nvenc_defaults(obs_data_t *settings)
{
	nvenc_defaults_base(CODEC_AV1, settings);
}

static bool rate_control_modified(obs_properties_t *ppts, obs_property_t *p, obs_data_t *settings)
{
	const char *rc = obs_data_get_string(settings, "rate_control");
	bool cqp = strcmp(rc, "CQP") == 0;
	bool vbr = strcmp(rc, "VBR") == 0;
	bool cqvbr = strcmp(rc, "CQVBR") == 0;
	bool lossless = strcmp(rc, "lossless") == 0;

	p = obs_properties_get(ppts, "bitrate");
	obs_property_set_visible(p, !cqp && !lossless && !cqvbr);
	p = obs_properties_get(ppts, "max_bitrate");
	obs_property_set_visible(p, vbr || cqvbr);
	p = obs_properties_get(ppts, "target_quality");
	obs_property_set_visible(p, cqvbr);
	p = obs_properties_get(ppts, "cqp");
	obs_property_set_visible(p, cqp);
	p = obs_properties_get(ppts, "preset");
	obs_property_set_visible(p, !lossless);
	p = obs_properties_get(ppts, "tune");
	obs_property_set_visible(p, !lossless);
	p = obs_properties_get(ppts, "adaptive_quantization");
	obs_property_set_visible(p, !lossless);

	return true;
}

obs_properties_t *nvenc_properties_internal(enum codec_type codec)
{
	obs_properties_t *props = obs_properties_create();
	obs_property_t *p;

	struct encoder_caps *caps = get_encoder_caps(codec);

	p = obs_properties_add_list(props, "rate_control", obs_module_text("RateControl"), OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(p, obs_module_text("CBR"), "CBR");
	obs_property_list_add_string(p, obs_module_text("CQP"), "CQP");
	obs_property_list_add_string(p, obs_module_text("VBR"), "VBR");
	obs_property_list_add_string(p, obs_module_text("CQVBR"), "CQVBR");
	if (caps->lossless) {
		obs_property_list_add_string(p, obs_module_text("Lossless"), "lossless");
	}

	obs_property_set_modified_callback(p, rate_control_modified);

	p = obs_properties_add_int(props, "bitrate", obs_module_text("Bitrate"), 50, UINT32_MAX / 1000, 50);
	obs_property_int_set_suffix(p, " Kbps");

	obs_properties_add_int(props, "target_quality", obs_module_text("TargetQuality"), 1,
			       codec == CODEC_AV1 ? 63 : 51, 1);

	p = obs_properties_add_int(props, "max_bitrate", obs_module_text("MaxBitrate"), 0, UINT32_MAX / 1000, 50);
	obs_property_int_set_suffix(p, " Kbps");

	/* AV1 uses 0-255 instead of 0-51 for QP, and most implementations just
	 * multiply the value by 4 to keep the range smaller. */
	obs_properties_add_int(props, "cqp", obs_module_text("CQP"), 1, codec == CODEC_AV1 ? 63 : 51, 1);

	p = obs_properties_add_int(props, "keyint_sec", obs_module_text("KeyframeIntervalSec"), 0, 10, 1);
	obs_property_int_set_suffix(p, " s");

	p = obs_properties_add_list(props, "preset", obs_module_text("Preset"), OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);

#define add_preset(val) obs_property_list_add_string(p, obs_module_text("Preset." val), val)

	add_preset("p1");
	add_preset("p2");
	add_preset("p3");
	add_preset("p4");
	add_preset("p5");
	add_preset("p6");
	add_preset("p7");
#undef add_preset

	p = obs_properties_add_list(props, "tune", obs_module_text("Tuning"), OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);

#define add_tune(val) obs_property_list_add_string(p, obs_module_text("Tuning." val), val)
	/* The UHQ tune is only supported on Turing or later. */
	if (caps->uhq)
		add_tune("uhq");

	add_tune("hq");
	add_tune("ll");
	add_tune("ull");
#undef add_tune

	p = obs_properties_add_list(props, "multipass", obs_module_text("Multipass"), OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);

#define add_multipass(val) obs_property_list_add_string(p, obs_module_text("Multipass." val), val)
	add_multipass("disabled");
	add_multipass("qres");
	add_multipass("fullres");
#undef add_multipass

	p = obs_properties_add_list(props, "profile", obs_module_text("Profile"), OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);

#define add_profile(val) obs_property_list_add_string(p, val, val)
	if (codec == CODEC_HEVC) {
		if (caps->ten_bit)
			add_profile("main10");
		add_profile("main");
	} else if (codec == CODEC_AV1) {
		add_profile("main");
	} else {
#ifdef NVENC_13_0_OR_LATER
		if (caps->ten_bit)
			add_profile("high10");
#endif
		add_profile("high");
		add_profile("main");
		add_profile("baseline");
	}
#undef add_profile

	p = obs_properties_add_bool(props, "lookahead", obs_module_text("LookAhead"));
	obs_property_set_long_description(p, obs_module_text("LookAhead.ToolTip"));

	p = obs_properties_add_bool(props, "adaptive_quantization", obs_module_text("AdaptiveQuantization"));
	obs_property_set_long_description(p, obs_module_text("AdaptiveQuantization.ToolTip"));

	if (num_encoder_devices() > 1) {
		obs_properties_add_int(props, "device", obs_module_text("GPU"), -1, num_encoder_devices(), 1);
	}

	if (caps->bframes > 0) {
		obs_properties_add_int(props, "bf", obs_module_text("BFrames"), 0, caps->bframes, 1);
	}

	/* H.264 supports this, but seems to cause issues with some decoders,
	 * so restrict it to the custom options field for now. */
	if (caps->bref_modes && codec != CODEC_H264) {
		p = obs_properties_add_list(props, "bframe_ref_mode", obs_module_text("BFrameRefMode"),
					    OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

		obs_property_list_add_int(p, obs_module_text("BframeRefMode.Disabled"),
					  NV_ENC_BFRAME_REF_MODE_DISABLED);

		if (caps->bref_modes & 1) {
			obs_property_list_add_int(p, obs_module_text("BframeRefMode.Each"),
						  NV_ENC_BFRAME_REF_MODE_EACH);
		}
		if (caps->bref_modes & 2) {
			obs_property_list_add_int(p, obs_module_text("BframeRefMode.Middle"),
						  NV_ENC_BFRAME_REF_MODE_MIDDLE);
		}
	}

#ifdef NVENC_12_1_OR_LATER
	/* Some older GPUs such as the 1080 Ti have 2 NVENC chips, but do not
	 * support split encoding. Therefore, we check for AV1 support here to
	 * make sure this option is only presented on 40-series and later. */
	if (is_codec_supported(CODEC_AV1) && caps->engines > 1 && !has_broken_split_encoding() &&
	    (codec == CODEC_HEVC || codec == CODEC_AV1)) {
		p = obs_properties_add_list(props, "split_encode", obs_module_text("SplitEncode"), OBS_COMBO_TYPE_LIST,
					    OBS_COMBO_FORMAT_INT);

		obs_property_list_add_int(p, obs_module_text("SplitEncode.Auto"), NV_ENC_SPLIT_AUTO_MODE);
		obs_property_list_add_int(p, obs_module_text("SplitEncode.Disabled"), NV_ENC_SPLIT_DISABLE_MODE);
		obs_property_list_add_int(p, obs_module_text("SplitEncode.Enabled"), NV_ENC_SPLIT_TWO_FORCED_MODE);
		if (caps->engines > 2) {
			obs_property_list_add_int(p, obs_module_text("SplitEncode.ThreeWay"),
						  NV_ENC_SPLIT_THREE_FORCED_MODE);
		}
#ifdef NVENC_13_0_OR_LATER
		if (caps->engines > 3) {
			obs_property_list_add_int(p, obs_module_text("SplitEncode.FourWay"),
						  NV_ENC_SPLIT_FOUR_FORCED_MODE);
		}
#endif
	}
#endif

	p = obs_properties_add_text(props, "opts", obs_module_text("Opts"), OBS_TEXT_DEFAULT);
	obs_property_set_long_description(p, obs_module_text("Opts.TT"));

	/* Invisible properties */
	p = obs_properties_add_bool(props, "repeat_headers", "repeat_headers");
	obs_property_set_visible(p, false);
	p = obs_properties_add_bool(props, "force_cuda_tex", "force_cuda_tex");
	obs_property_set_visible(p, false);
	p = obs_properties_add_bool(props, "disable_scenecut", "disable_scenecut");
	obs_property_set_visible(p, false);

	return props;
}

obs_properties_t *h264_nvenc_properties(void *unused)
{
	UNUSED_PARAMETER(unused);
	return nvenc_properties_internal(CODEC_H264);
}

#ifdef ENABLE_HEVC
obs_properties_t *hevc_nvenc_properties(void *unused)
{
	UNUSED_PARAMETER(unused);
	return nvenc_properties_internal(CODEC_HEVC);
}
#endif

obs_properties_t *av1_nvenc_properties(void *unused)
{
	UNUSED_PARAMETER(unused);
	return nvenc_properties_internal(CODEC_AV1);
}
