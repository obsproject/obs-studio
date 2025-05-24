static bool str_to_bool(const char *str)
{
	if (!str)
		return false;
	if (*str == '1')
		return true;
	if (*str == '0')
		return false;
	if (astrcmpi(str, "true") == 0)
		return true;
	if (astrcmpi(str, "false") == 0)
		return false;
	return false;
}

static void amf_apply_opt(amf_base *enc, obs_option *opt)
{
	bool avc = enc->codec == amf_codec_type::AVC;
	bool hevc = enc->codec == amf_codec_type::HEVC;
	bool av1 = enc->codec == amf_codec_type::AV1;

	if (strcmp(opt->name, "g") == 0 || strcmp(opt->name, "keyint") == 0) {

		int val = atoi(opt->value);
		if (avc)
			set_avc_opt(IDR_PERIOD, val);
		else if (hevc)
			set_hevc_opt(NUM_GOPS_PER_IDR, val);
		else if (av1)
			set_av1_opt(GOP_SIZE, val);

	} else if (strcmp(opt->name, "usage") == 0) {

		if (strcmp(opt->value, "transcoding") == 0) {
			set_enum_opt(USAGE, TRANSCODING);
		} else if (strcmp(opt->value, "ultralowlatency") == 0) {
			if (avc)
				set_avc_enum(USAGE, ULTRA_LOW_LATENCY);
			else if (hevc)
				set_hevc_enum(USAGE, ULTRA_LOW_LATENCY);
			else
				warn("Invalid value for %s: %s", opt->name, opt->value);
		} else if (strcmp(opt->value, "lowlatency") == 0) {
			set_enum_opt(USAGE, LOW_LATENCY);
		} else if (strcmp(opt->value, "webcam") == 0) {
			if (avc)
				set_avc_enum(USAGE, WEBCAM);
			else if (hevc)
				set_hevc_enum(USAGE, WEBCAM);
			else
				warn("Invalid value for %s: %s", opt->name, opt->value);
		} else {
			warn("Invalid value for %s: %s", opt->name, opt->value);
		}

	} else if (strcmp(opt->name, "profile") == 0) {

		if (strcmp(opt->value, "main") == 0) {
			set_enum_opt(PROFILE, MAIN);
		} else if (enc->codec != amf_codec_type::AVC) {
			warn("Invalid value for %s: %s", opt->name, opt->value);
			return;
		}

		if (strcmp(opt->value, "high") == 0) {
			set_opt(PROFILE, AMF_VIDEO_ENCODER_PROFILE_HIGH);
		} else if (strcmp(opt->value, "constrained_baseline") == 0) {
			set_opt(PROFILE, AMF_VIDEO_ENCODER_PROFILE_CONSTRAINED_BASELINE);
		} else if (strcmp(opt->value, "constrained_high") == 0) {
			set_opt(PROFILE, AMF_VIDEO_ENCODER_PROFILE_CONSTRAINED_HIGH);
		} else {
			warn("Invalid value for %s: %s", opt->name, opt->value);
		}

	} else if (strcmp(opt->name, "level") == 0) {

		std::string val = opt->value;
		size_t pos = val.find('.');
		if (pos != std::string::npos)
			val.erase(pos, 1);

		int level = std::stoi(val);
		if (avc)
			set_avc_opt(PROFILE_LEVEL, level);
		else if (hevc)
			set_hevc_opt(PROFILE_LEVEL, level);
		else
			warn("Invalid value for %s: %s", opt->name, opt->value);

	} else if (strcmp(opt->name, "quality") == 0) {

		if (strcmp(opt->value, "speed") == 0) {
			set_enum_opt(QUALITY_PRESET, SPEED);
		} else if (strcmp(opt->value, "balanced") == 0) {
			set_enum_opt(QUALITY_PRESET, BALANCED);
		} else if (strcmp(opt->value, "quality") == 0) {
			set_enum_opt(QUALITY_PRESET, QUALITY);
		} else {
			warn("Invalid value for %s: %s", opt->name, opt->value);
		}

	} else if (strcmp(opt->name, "rc") == 0) {

		if (strcmp(opt->value, "cqp") == 0) {
			set_enum_opt(RATE_CONTROL_METHOD, CONSTANT_QP);
		} else if (strcmp(opt->value, "cbr") == 0) {
			set_enum_opt(RATE_CONTROL_METHOD, CBR);
		} else if (strcmp(opt->value, "vbr_peak") == 0) {
			set_enum_opt(RATE_CONTROL_METHOD, PEAK_CONSTRAINED_VBR);
		} else if (strcmp(opt->value, "vbr_latency") == 0) {
			set_enum_opt(RATE_CONTROL_METHOD, LATENCY_CONSTRAINED_VBR);
		} else {
			warn("Invalid value for %s: %s", opt->name, opt->value);
		}

	} else if (strcmp(opt->name, "enforce_hrd") == 0) {

		bool val = str_to_bool(opt->value);
		set_opt(ENFORCE_HRD, val);

	} else if (strcmp(opt->name, "filler_data") == 0) {

		bool val = str_to_bool(opt->value);
		if (avc)
			set_avc_opt(FILLER_DATA_ENABLE, val);
		else if (hevc)
			set_hevc_opt(FILLER_DATA_ENABLE, val);
		else
			warn("Invalid value for %s: %s", opt->name, opt->value);

	} else if (strcmp(opt->name, "vbaq") == 0) {

		bool val = str_to_bool(opt->value);
		if (avc)
			set_avc_opt(ENABLE_VBAQ, val);
		else if (hevc)
			set_hevc_opt(ENABLE_VBAQ, val);
		else
			warn("Invalid value for %s: %s", opt->name, opt->value);

	} else if (strcmp(opt->name, "qp_i") == 0) {

		int val = atoi(opt->value);
		if (avc)
			set_avc_opt(QP_I, val);
		else if (hevc)
			set_hevc_opt(QP_I, val);
		else
			warn("Invalid value for %s: %s", opt->name, opt->value);

	} else if (strcmp(opt->name, "qp_p") == 0) {

		int val = atoi(opt->value);
		if (avc)
			set_avc_opt(QP_P, val);
		else if (hevc)
			set_hevc_opt(QP_P, val);
		else
			warn("Invalid value for %s: %s", opt->name, opt->value);

	} else if (strcmp(opt->name, "me_half_pel") == 0) {

		bool val = str_to_bool(opt->value);
		if (avc)
			set_avc_opt(MOTION_HALF_PIXEL, val);
		else if (hevc)
			set_hevc_opt(MOTION_HALF_PIXEL, val);
		else
			warn("Invalid value for %s: %s", opt->name, opt->value);

	} else if (strcmp(opt->name, "me_quarter_pel") == 0) {

		bool val = str_to_bool(opt->value);
		if (avc)
			set_avc_opt(MOTION_QUARTERPIXEL, val);
		else if (hevc)
			set_hevc_opt(MOTION_QUARTERPIXEL, val);
		else
			warn("Invalid value for %s: %s", opt->name, opt->value);

	} else if (strcmp(opt->name, "aud") == 0) {

		bool val = str_to_bool(opt->value);
		if (avc)
			set_avc_opt(INSERT_AUD, val);
		else if (hevc)
			set_hevc_opt(INSERT_AUD, val);
		else
			warn("Invalid value for %s: %s", opt->name, opt->value);

	} else if (strcmp(opt->name, "max_au_size") == 0) {

		int val = atoi(opt->value);
		if (avc)
			set_avc_opt(MAX_AU_SIZE, val);
		else if (hevc)
			set_hevc_opt(MAX_AU_SIZE, val);
		else
			warn("Invalid value for %s: %s", opt->name, opt->value);

	} else if (avc && strcmp(opt->name, "preanalysis") == 0) {

		bool val = str_to_bool(opt->value);
		set_avc_property(enc, PREENCODE_ENABLE, val);

	} else if (avc && strcmp(opt->name, "qp_b") == 0) {

		int val = atoi(opt->value);
		set_avc_property(enc, QP_B, val);

	} else if (avc && strcmp(opt->name, "frame_skipping") == 0) {

		bool val = str_to_bool(opt->value);
		set_avc_property(enc, RATE_CONTROL_SKIP_FRAME_ENABLE, val);

	} else if (avc && strcmp(opt->name, "header_spacing") == 0) {

		int val = atoi(opt->value);
		set_avc_property(enc, HEADER_INSERTION_SPACING, val);

	} else if (avc && strcmp(opt->name, "bf_delta_qp") == 0) {

		int val = atoi(opt->value);
		set_avc_property(enc, B_PIC_DELTA_QP, val);

	} else if (avc && strcmp(opt->name, "bf_ref") == 0) {

		bool val = str_to_bool(opt->value);
		set_avc_property(enc, B_REFERENCE_ENABLE, val);

	} else if (avc && strcmp(opt->name, "bf_ref_delta_qp") == 0) {

		int val = atoi(opt->value);
		set_avc_property(enc, REF_B_PIC_DELTA_QP, val);

	} else if (avc && strcmp(opt->name, "intra_refresh_mb") == 0) {

		int val = atoi(opt->value);
		set_avc_property(enc, INTRA_REFRESH_NUM_MBS_PER_SLOT, val);

	} else if (avc && strcmp(opt->name, "coder") == 0) {

		if (strcmp(opt->value, "auto") == 0) {
			set_avc_opt(CABAC_ENABLE, AMF_VIDEO_ENCODER_UNDEFINED);
		} else if (strcmp(opt->value, "cavlc") == 0) {
			set_avc_opt(CABAC_ENABLE, AMF_VIDEO_ENCODER_CALV);
		} else if (strcmp(opt->value, "cabac") == 0) {
			set_avc_opt(CABAC_ENABLE, AMF_VIDEO_ENCODER_CABAC);
		} else {
			warn("Invalid value for %s: %s", opt->name, opt->value);
		}

	} else if (hevc && strcmp(opt->name, "profile_tier") == 0) {

		if (strcmp(opt->value, "main") == 0) {
			set_hevc_enum(TIER, MAIN);
		} else if (strcmp(opt->value, "high") == 0) {
			set_hevc_enum(TIER, HIGH);
		} else {
			warn("Invalid value for %s: %s", opt->name, opt->value);
		}

	} else if (hevc && strcmp(opt->name, "header_insertion_mode") == 0) {

		if (strcmp(opt->value, "none") == 0) {
			set_hevc_enum(HEADER_INSERTION_MODE, NONE);
		} else if (strcmp(opt->value, "gop") == 0) {
			set_hevc_enum(HEADER_INSERTION_MODE, GOP_ALIGNED);
		} else if (strcmp(opt->value, "idr") == 0) {
			set_hevc_enum(HEADER_INSERTION_MODE, IDR_ALIGNED);
		} else {
			warn("Invalid value for %s: %s", opt->name, opt->value);
		}

	} else if (hevc && strcmp(opt->name, "skip_frame") == 0) {

		bool val = str_to_bool(opt->value);
		set_hevc_property(enc, RATE_CONTROL_SKIP_FRAME_ENABLE, val);

	} else if (hevc && strcmp(opt->name, "gops_per_idr") == 0) {

		int val = atoi(opt->value);
		set_hevc_property(enc, NUM_GOPS_PER_IDR, val);

	} else if (hevc && strcmp(opt->name, "min_qp_i") == 0) {

		int val = atoi(opt->value);
		set_hevc_property(enc, MIN_QP_I, val);

	} else if (hevc && strcmp(opt->name, "max_qp_i") == 0) {

		int val = atoi(opt->value);
		set_hevc_property(enc, MAX_QP_I, val);

	} else if (hevc && strcmp(opt->name, "min_qp_p") == 0) {

		int val = atoi(opt->value);
		set_hevc_property(enc, MIN_QP_P, val);

	} else if (hevc && strcmp(opt->name, "max_qp_p") == 0) {

		int val = atoi(opt->value);
		set_hevc_property(enc, MAX_QP_P, val);
	} else {
		wchar_t wname[256];
		int val;
		bool is_bool = false;

		if (astrcmpi(opt->value, "true") == 0) {
			is_bool = true;
			val = 1;
		} else if (astrcmpi(opt->value, "false") == 0) {
			is_bool = true;
			val = 0;
		} else {
			val = atoi(opt->value);
		}

		os_utf8_to_wcs(opt->name, 0, wname, _countof(wname));
		if (is_bool) {
			bool bool_val = (bool)val;
			set_amf_property(enc, wname, bool_val);
		} else {
			set_amf_property(enc, wname, val);
		}
	}
}
