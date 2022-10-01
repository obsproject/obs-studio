/******************************************************************************
    Copyright (C) 2014 by Hugh Bailey <obs.jim@gmail.com>

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

#include <stdio.h>
#include <string.h>
#include <util/bmem.h>
#include <util/dstr.h>
#include <util/darray.h>
#include <util/platform.h>
#include <obs-module.h>
#include <opts-parser.h>

#ifndef _STDINT_H_INCLUDED
#define _STDINT_H_INCLUDED
#endif

#include <x265.h>

#define do_log_enc(level, encoder, format, ...)     \
	blog(level, "[x265 encoder: '%s'] " format, \
	     obs_encoder_get_name(encoder), ##__VA_ARGS__)
#define do_log(level, format, ...) \
	do_log_enc(level, obsx265->encoder, format, ##__VA_ARGS__)

#define warn(format, ...) do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define warn_enc(encoder, format, ...) \
	do_log_enc(LOG_WARNING, encoder, format, ##__VA_ARGS__)
#define info(format, ...) do_log(LOG_INFO, format, ##__VA_ARGS__)
#define debug(format, ...) do_log(LOG_DEBUG, format, ##__VA_ARGS__)

/* ------------------------------------------------------------------------- */

struct obs_x265 {
	obs_encoder_t *encoder;

	x265_param params;
	x265_encoder *context;

	DARRAY(uint8_t) packet_data;

	uint8_t *extra_data;
	uint8_t *sei;

	size_t extra_data_size;
	size_t sei_size;

	int bitDepth;

	os_performance_token_t *performance_token;
};

/* ------------------------------------------------------------------------- */

static const char *obs_x265_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "x265";
}

static void obs_x265_stop(void *data);

static void clear_data(struct obs_x265 *obsx265)
{
	if (obsx265->context) {
		x265_encoder_close(obsx265->context);
		bfree(obsx265->sei);
		bfree(obsx265->extra_data);

		obsx265->context = NULL;
		obsx265->sei = NULL;
		obsx265->extra_data = NULL;
	}
}

static void obs_x265_destroy(void *data)
{
	struct obs_x265 *obsx265 = data;

	if (obsx265) {
		os_end_high_performance(obsx265->performance_token);
		clear_data(obsx265);
		da_free(obsx265->packet_data);
		bfree(obsx265);
	}
}

static void obs_x265_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "bitrate", 2500);
	obs_data_set_default_bool(settings, "use_bufsize", false);
	obs_data_set_default_int(settings, "buffer_size", 2500);
	obs_data_set_default_int(settings, "keyint_sec", 0);
	obs_data_set_default_int(settings, "crf", 23);
	obs_data_set_default_string(settings, "rate_control", "CBR");

	obs_data_set_default_string(settings, "preset", "veryfast");
	obs_data_set_default_string(settings, "profile", "");
	obs_data_set_default_string(settings, "tune", "");
	obs_data_set_default_string(settings, "x265opts", "");
	obs_data_set_default_bool(settings, "repeat_headers", false);
}

static inline void add_strings(obs_property_t *list, const char *const *strings)
{
	while (*strings) {
		obs_property_list_add_string(list, *strings, *strings);
		strings++;
	}
}

#define TEXT_RATE_CONTROL obs_module_text("RateControl")
#define TEXT_BITRATE obs_module_text("Bitrate")
#define TEXT_CUSTOM_BUF obs_module_text("CustomBufsize")
#define TEXT_BUF_SIZE obs_module_text("BufferSize")
#define TEXT_VFR obs_module_text("VFR")
#define TEXT_CRF obs_module_text("CRF")
#define TEXT_KEYINT_SEC obs_module_text("KeyframeIntervalSec")
#define TEXT_PRESET obs_module_text("CPUPreset")
#define TEXT_PROFILE obs_module_text("Profile")
#define TEXT_TUNE obs_module_text("Tune")
#define TEXT_NONE obs_module_text("None")
#define TEXT_X265_OPTS obs_module_text("EncoderOptions")

static bool use_bufsize_modified(obs_properties_t *ppts, obs_property_t *p,
				 obs_data_t *settings)
{
	bool use_bufsize = obs_data_get_bool(settings, "use_bufsize");
	const char *rc = obs_data_get_string(settings, "rate_control");
	bool rc_crf = astrcmpi(rc, "CRF") == 0;

	p = obs_properties_get(ppts, "buffer_size");
	obs_property_set_visible(p, use_bufsize && !rc_crf);
	return true;
}

static bool rate_control_modified(obs_properties_t *ppts, obs_property_t *p,
				  obs_data_t *settings)
{
	const char *rc = obs_data_get_string(settings, "rate_control");
	bool use_bufsize = obs_data_get_bool(settings, "use_bufsize");
	bool abr = astrcmpi(rc, "CBR") == 0 || astrcmpi(rc, "ABR") == 0;
	bool rc_crf = astrcmpi(rc, "CRF") == 0;

	p = obs_properties_get(ppts, "crf");
	obs_property_set_visible(p, !abr);

	p = obs_properties_get(ppts, "bitrate");
	obs_property_set_visible(p, !rc_crf);
	p = obs_properties_get(ppts, "use_bufsize");
	obs_property_set_visible(p, !rc_crf);
	p = obs_properties_get(ppts, "buffer_size");
	obs_property_set_visible(p, !rc_crf && use_bufsize);
	return true;
}

static obs_properties_t *obs_x265_props(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *props = obs_properties_create();
	obs_property_t *list;
	obs_property_t *p;
	obs_property_t *headers;

	list = obs_properties_add_list(props, "rate_control", TEXT_RATE_CONTROL,
				       OBS_COMBO_TYPE_LIST,
				       OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(list, "CBR", "CBR");
	obs_property_list_add_string(list, "ABR", "ABR");
	obs_property_list_add_string(list, "VBR", "VBR");
	obs_property_list_add_string(list, "CRF", "CRF");

	obs_property_set_modified_callback(list, rate_control_modified);

	p = obs_properties_add_int(props, "bitrate", TEXT_BITRATE, 50, 10000000,
				   50);
	obs_property_int_set_suffix(p, " Kbps");

	p = obs_properties_add_bool(props, "use_bufsize", TEXT_CUSTOM_BUF);
	obs_property_set_modified_callback(p, use_bufsize_modified);
	obs_properties_add_int(props, "buffer_size", TEXT_BUF_SIZE, 0, 10000000,
			       1);

	obs_properties_add_int(props, "crf", TEXT_CRF, 0, 51, 1);

	p = obs_properties_add_int(props, "keyint_sec", TEXT_KEYINT_SEC, 0, 20,
				   1);
	obs_property_int_set_suffix(p, " s");

	list = obs_properties_add_list(props, "preset", TEXT_PRESET,
				       OBS_COMBO_TYPE_LIST,
				       OBS_COMBO_FORMAT_STRING);
	add_strings(list, x265_preset_names);

	list = obs_properties_add_list(props, "profile", TEXT_PROFILE,
				       OBS_COMBO_TYPE_LIST,
				       OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(list, TEXT_NONE, "");
	add_strings(list, x265_profile_names);

	list = obs_properties_add_list(props, "tune", TEXT_TUNE,
				       OBS_COMBO_TYPE_LIST,
				       OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(list, TEXT_NONE, "");
	add_strings(list, x265_tune_names);

	obs_properties_add_text(props, "x265opts", TEXT_X265_OPTS,
				OBS_TEXT_DEFAULT);

	headers = obs_properties_add_bool(props, "repeat_headers",
					  "repeat_headers");
	obs_property_set_visible(headers, false);

	return props;
}

static bool getparam(const char *param, char **name, const char **value)
{
	const char *assign;

	if (!param || !*param || (*param == '='))
		return false;

	assign = strchr(param, '=');
	if (!assign || !*assign || !*(assign + 1))
		return false;

	*name = bstrdup_n(param, assign - param);
	*value = assign + 1;
	return true;
}

static const char *validate(struct obs_x265 *obsx265, const char *val,
			    const char *name, const char *const *list)
{
	if (!val || !*val)
		return val;

	while (*list) {
		if (strcmp(val, *list) == 0)
			return val;

		list++;
	}

	warn("Invalid %s: %s", name, val);
	return NULL;
}

static void override_base_param(struct obs_x265 *obsx265,
				struct obs_option option, char **preset,
				char **profile, char **tune)
{
	const char *name = option.name;
	const char *val = option.value;
	if (astrcmpi(name, "preset") == 0) {
		const char *valid_name =
			validate(obsx265, val, "preset", x265_preset_names);
		if (valid_name) {
			bfree(*preset);
			*preset = bstrdup(val);
		}

	} else if (astrcmpi(name, "profile") == 0) {
		const char *valid_name =
			validate(obsx265, val, "profile", x265_profile_names);
		if (valid_name) {
			bfree(*profile);
			*profile = bstrdup(val);
		}

	} else if (astrcmpi(name, "tune") == 0) {
		const char *valid_name =
			validate(obsx265, val, "tune", x265_tune_names);
		if (valid_name) {
			bfree(*tune);
			*tune = bstrdup(val);
		}
	}
}

static inline void override_base_params(struct obs_x265 *obsx265,
					const struct obs_options *options,
					char **preset, char **profile,
					char **tune)
{
	for (size_t i = 0; i < options->count; ++i)
		override_base_param(obsx265, options->options[i], preset,
				    profile, tune);
}

#define OPENCL_ALIAS "opencl_is_experimental_and_potentially_unstable"

static inline void set_param(struct obs_x265 *obsx265, struct obs_option option)
{
	const char *name = option.name;
	const char *val = option.value;
	if (strcmp(name, "preset") != 0 && strcmp(name, "profile") != 0 &&
	    strcmp(name, "tune") != 0 && strcmp(name, "fps") != 0 &&
	    strcmp(name, "force-cfr") != 0 && strcmp(name, "width") != 0 &&
	    strcmp(name, "height") != 0 && strcmp(name, "opencl") != 0) {
		if (strcmp(option.name, OPENCL_ALIAS) == 0)
			name = "opencl";
		if (x265_param_parse(&obsx265->params, name, val) != 0)
			warn("x265 param: %s=%s failed", name, val);
	}
}

static inline void apply_x265_profile(struct obs_x265 *obsx265,
				      const char *profile)
{
	if (!obsx265->context && profile && *profile) {
		int ret = x265_param_apply_profile(&obsx265->params, profile);
		if (ret != 0)
			warn("Failed to set x265 profile '%s'", profile);
	}
}

static inline const char *validate_preset(struct obs_x265 *obsx265,
					  const char *preset)
{
	const char *new_preset =
		validate(obsx265, preset, "preset", x265_preset_names);
	return new_preset ? new_preset : "veryfast";
}

static bool reset_x265_params(struct obs_x265 *obsx265, const char *preset,
			      const char *tune)
{
	int ret = x265_param_default_preset(
		&obsx265->params, validate_preset(obsx265, preset),
		validate(obsx265, tune, "tune", x265_tune_names));
	return ret == 0;
}

static void log_x265(void *param, int level, const char *format, va_list args)
{
	struct obs_x265 *obsx265 = param;
	char str[1024];

	vsnprintf(str, 1024, format, args);
	info("%s", str);

	UNUSED_PARAMETER(level);
}

static inline int get_x265_cs_val(const char *const name,
				  const char *const names[])
{
	int idx = 0;
	do {
		if (strcmp(names[idx], name) == 0)
			return idx;
	} while (!!names[++idx]);

	return 0;
}

static void obs_x265_video_info(void *data, struct video_scale_info *info);

enum rate_control {
	RATE_CONTROL_CBR,
	RATE_CONTROL_VBR,
	RATE_CONTROL_ABR,
	RATE_CONTROL_CRF
};

static void update_params(struct obs_x265 *obsx265, obs_data_t *settings,
			  const struct obs_options *options, bool update)
{
	video_t *video = obs_encoder_video(obsx265->encoder);
	const struct video_output_info *voi = video_output_get_info(video);
	struct video_scale_info info;

	info.format = voi->format;
	info.colorspace = voi->colorspace;
	info.range = voi->range;

	obs_x265_video_info(obsx265, &info);

	const char *rate_control =
		obs_data_get_string(settings, "rate_control");

	int bitrate = (int)obs_data_get_int(settings, "bitrate");
	int buffer_size = (int)obs_data_get_int(settings, "buffer_size");
	int keyint_sec = (int)obs_data_get_int(settings, "keyint_sec");
	int crf = (int)obs_data_get_int(settings, "crf");
	int width = (int)obs_encoder_get_width(obsx265->encoder);
	int height = (int)obs_encoder_get_height(obsx265->encoder);
	int bf = (int)obs_data_get_int(settings, "bf");
	bool use_bufsize = obs_data_get_bool(settings, "use_bufsize");
	bool cbr_override = obs_data_get_bool(settings, "cbr");
	enum rate_control rc;

	/* XXX: "cbr" setting has been deprecated */
	if (cbr_override) {
		warn("\"cbr\" setting has been deprecated for all encoders!  "
		     "Please set \"rate_control\" to \"CBR\" instead.  "
		     "Forcing CBR mode.  "
		     "(Note to all: this is why you shouldn't use strings for "
		     "common settings)");
		rate_control = "CBR";
	}

	if (astrcmpi(rate_control, "ABR") == 0) {
		rc = RATE_CONTROL_ABR;
		crf = 0;

	} else if (astrcmpi(rate_control, "VBR") == 0) {
		rc = RATE_CONTROL_VBR;

	} else if (astrcmpi(rate_control, "CRF") == 0) {
		rc = RATE_CONTROL_CRF;
		bitrate = 0;
		buffer_size = 0;

	} else { /* CBR */
		rc = RATE_CONTROL_CBR;
		crf = 0;
	}

	if (keyint_sec)
		obsx265->params.keyframeMax =
			keyint_sec * voi->fps_num / voi->fps_den;

	if (!use_bufsize)
		buffer_size = bitrate;

	obsx265->params.rc.vbvMaxBitrate = bitrate;
	obsx265->params.rc.vbvBufferSize = buffer_size;
	obsx265->params.rc.bitrate = bitrate;
	obsx265->params.sourceWidth = width;
	obsx265->params.sourceHeight = height;
	obsx265->params.fpsNum = voi->fps_num;
	obsx265->params.fpsDenom = voi->fps_den;
	obsx265->params.logLevel = X265_LOG_WARNING;

	if (obs_data_has_user_value(settings, "bf"))
		obsx265->params.bframes = bf;

	static const char *const smpte170m = "smpte170m";
	static const char *const bt709 = "bt709";
	static const char *const bt2020 = "bt2020";
	static const char *const bt2020nc = "bt2020nc";
	const char *colorprim = bt709;
	const char *transfer = bt709;
	const char *colmatrix = bt709;
	switch (info.colorspace) {
	case VIDEO_CS_DEFAULT:
	case VIDEO_CS_709:
		colorprim = bt709;
		transfer = bt709;
		colmatrix = bt709;
		break;
	case VIDEO_CS_601:
		colorprim = smpte170m;
		transfer = smpte170m;
		colmatrix = smpte170m;
		break;
	case VIDEO_CS_SRGB:
		colorprim = bt709;
		transfer = "iec61966-2-1";
		colmatrix = bt709;
		break;
	case VIDEO_CS_2100_PQ:
		colorprim = bt2020;
		transfer = "smpte2084";
		colmatrix = bt2020nc;
		break;
	case VIDEO_CS_2100_HLG:
		colorprim = bt2020;
		transfer = "arib-std-b67";
		colmatrix = bt2020nc;
	}

	obsx265->params.vui.sarHeight = 1;
	obsx265->params.vui.sarWidth = 1;
	obsx265->params.vui.bEnableVideoFullRangeFlag = info.range ==
							VIDEO_RANGE_FULL;
	obsx265->params.vui.colorPrimaries =
		get_x265_cs_val(colorprim, x265_colorprim_names);
	obsx265->params.vui.transferCharacteristics =
		get_x265_cs_val(transfer, x265_transfer_names);
	obsx265->params.vui.matrixCoeffs =
		get_x265_cs_val(colmatrix, x265_colmatrix_names);

	/* use the new filler method for CBR to allow real-time adjusting of
	 * the bitrate */
	if (rc == RATE_CONTROL_CBR || rc == RATE_CONTROL_ABR) {
		obsx265->params.rc.rateControlMode = X265_RC_ABR;

		if (rc == RATE_CONTROL_CBR) {
			obsx265->params.rc.bStrictCbr = true;
		}
	} else {
		obsx265->params.rc.rateControlMode = X265_RC_CRF;
		obsx265->params.rc.rfConstant = (float)crf;
	}

	if (info.format == VIDEO_FORMAT_I010 ||
	    info.format == VIDEO_FORMAT_P010) {
		obsx265->params.internalCsp = X265_CSP_I420;
		obsx265->bitDepth = 10;
	} else if (info.format == VIDEO_FORMAT_I444) {
		obsx265->params.internalCsp = X265_CSP_I444;
		obsx265->bitDepth = 8;
	} else {
		obsx265->params.internalCsp = X265_CSP_I420;
		obsx265->bitDepth = 8;
	}

	for (size_t i = 0; i < options->ignored_word_count; ++i)
		warn("ignoring invalid x265 option: %s",
		     options->ignored_words[i]);
	for (size_t i = 0; i < options->count; ++i)
		set_param(obsx265, options->options[i]);

	if (!update) {
		info("settings:\n"
		     "\trate_control: %s\n"
		     "\tbitrate:      %d\n"
		     "\tbuffer size:  %d\n"
		     "\tcrf:          %d\n"
		     "\tfps_num:      %d\n"
		     "\tfps_den:      %d\n"
		     "\twidth:        %d\n"
		     "\theight:       %d\n"
		     "\tkeyint:       %d\n",
		     rate_control, obsx265->params.rc.vbvMaxBitrate,
		     obsx265->params.rc.vbvBufferSize,
		     (int)obsx265->params.rc.rfConstant, voi->fps_num,
		     voi->fps_den, width, height, obsx265->params.keyframeMax);
	}
}

static void log_custom_options(struct obs_x265 *obsx265,
			       const struct obs_options *options)
{
	if (options->count == 0) {
		return;
	}
	size_t settings_string_length = 0;
	for (size_t i = 0; i < options->count; ++i)
		settings_string_length += strlen(options->options[i].name) +
					  strlen(options->options[i].value) + 5;
	size_t buffer_size = settings_string_length + 1;
	char *settings_string = bmalloc(settings_string_length + 1);
	char *p = settings_string;
	size_t remaining_buffer_size = buffer_size;
	for (size_t i = 0; i < options->count; ++i) {
		int chars_written = snprintf(p, remaining_buffer_size,
					     "\n\t%s = %s",
					     options->options[i].name,
					     options->options[i].value);
		assert(chars_written >= 0);
		assert((size_t)chars_written <= remaining_buffer_size);
		p += chars_written;
		remaining_buffer_size -= chars_written;
	}
	assert(remaining_buffer_size == 1);
	assert(*p == '\0');
	info("custom settings: %s", settings_string);
	bfree(settings_string);
}

static bool update_settings(struct obs_x265 *obsx265, obs_data_t *settings,
			    bool update)
{
	char *preset = bstrdup(obs_data_get_string(settings, "preset"));
	char *profile = bstrdup(obs_data_get_string(settings, "profile"));
	char *tune = bstrdup(obs_data_get_string(settings, "tune"));
	struct obs_options options =
		obs_parse_options(obs_data_get_string(settings, "x265opts"));
	bool repeat_headers = obs_data_get_bool(settings, "repeat_headers");

	bool success = true;

	if (!update)
		blog(LOG_INFO, "---------------------------------");

	if (!obsx265->context) {
		override_base_params(obsx265, &options, &preset, &profile,
				     &tune);

		if (preset && *preset)
			info("preset: %s", preset);
		if (profile && *profile)
			info("profile: %s", profile);
		if (tune && *tune) {
			info("tune: %s", tune);
			success = reset_x265_params(obsx265, preset, tune);
		} else {
			success = reset_x265_params(obsx265, preset, NULL);
		}
	}

	if (repeat_headers) {
		obsx265->params.bRepeatHeaders = 1;
		obsx265->params.bAnnexB = 1;
		obsx265->params.bEnableAccessUnitDelimiters = 1;
	}

	if (success) {
		update_params(obsx265, settings, &options, update);
		if (!update) {
			log_custom_options(obsx265, &options);
		}

		if (!obsx265->context)
			apply_x265_profile(obsx265, profile);
	}

	obs_free_options(options);
	bfree(preset);
	bfree(profile);
	bfree(tune);

	return success;
}

static bool obs_x265_update(void *data, obs_data_t *settings)
{
	struct obs_x265 *obsx265 = data;
	bool success = update_settings(obsx265, settings, true);

	if (success) {
		x265_encoder_close(obsx265->context);
		obsx265->context = x265_encoder_open(&obsx265->params);
		if (obsx265->context == NULL)
			warn("Failed to reconfigure");
		return obsx265->context != NULL;
	}

	return false;
}

static void load_headers(struct obs_x265 *obsx265)
{
	x265_nal *nals;
	int nal_count;
	DARRAY(uint8_t) header;
	DARRAY(uint8_t) sei;

	da_init(header);
	da_init(sei);

	x265_encoder_headers(obsx265->context, &nals, &nal_count);

	for (int i = 0; i < nal_count; i++) {
		x265_nal *nal = nals + i;

		if (nal->type == NAL_UNIT_PREFIX_SEI ||
		    nal->type == NAL_UNIT_SUFFIX_SEI)
			da_push_back_array(sei, nal->payload, nal->sizeBytes);
		else
			da_push_back_array(header, nal->payload,
					   nal->sizeBytes);
	}

	obsx265->extra_data = header.array;
	obsx265->extra_data_size = header.num;
	obsx265->sei = sei.array;
	obsx265->sei_size = sei.num;
}

static void *obs_x265_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	struct obs_x265 *obsx265 = bzalloc(sizeof(struct obs_x265));
	obsx265->encoder = encoder;

	if (update_settings(obsx265, settings, false)) {
		obsx265->context = x265_encoder_open(&obsx265->params);

		if (obsx265->context == NULL)
			warn("x265 failed to load");
		else
			load_headers(obsx265);
	} else {
		warn("bad settings specified");
	}

	if (!obsx265->context) {
		bfree(obsx265);
		return NULL;
	}

	obsx265->performance_token =
		os_request_high_performance("x265 encoding");

	return obsx265;
}

static void parse_packet(struct obs_x265 *obsx265,
			 struct encoder_packet *packet, x265_nal *nals,
			 int nal_count, x265_picture *pic_out)
{
	if (!nal_count)
		return;

	da_resize(obsx265->packet_data, 0);

	for (int i = 0; i < nal_count; i++) {
		x265_nal *nal = nals + i;
		da_push_back_array(obsx265->packet_data, nal->payload,
				   nal->sizeBytes);
	}

	packet->data = obsx265->packet_data.array;
	packet->size = obsx265->packet_data.num;
	packet->type = OBS_ENCODER_VIDEO;
	packet->pts = pic_out->pts;
	packet->dts = pic_out->dts;
	packet->keyframe = IS_X265_TYPE_I(pic_out->sliceType);
}

static inline void init_pic_data(struct obs_x265 *obsx265, x265_picture *pic,
				 struct encoder_frame *frame)
{
	x265_picture_init(&obsx265->params, pic);

	pic->bitDepth = obsx265->bitDepth;
	pic->pts = frame->pts;
	pic->colorSpace = obsx265->params.internalCsp;

	for (int i = 0; i < 3; i++) {
		pic->stride[i] = (int)frame->linesize[i];
		pic->planes[i] = frame->data[i];
	}
}

static bool obs_x265_encode(void *data, struct encoder_frame *frame,
			    struct encoder_packet *packet,
			    bool *received_packet)
{
	struct obs_x265 *obsx265 = data;
	x265_nal *nals;
	int nal_count;
	int ret;

	if (!frame || !packet || !received_packet)
		return false;

	x265_picture *pic = x265_picture_alloc();
	x265_picture *pic_out = x265_picture_alloc();

	if (frame)
		init_pic_data(obsx265, pic, frame);

	ret = x265_encoder_encode(obsx265->context, &nals, &nal_count,
				  (frame ? pic : NULL), pic_out);
	if (ret < 0) {
		warn("encode failed");
	} else {
		*received_packet = (nal_count != 0);
		parse_packet(obsx265, packet, nals, nal_count, pic_out);
	}

	x265_picture_free(pic);
	x265_picture_free(pic_out);

	return ret >= 0;
}

static bool obs_x265_extra_data(void *data, uint8_t **extra_data, size_t *size)
{
	struct obs_x265 *obsx265 = data;

	if (!obsx265->context)
		return false;

	*extra_data = obsx265->extra_data;
	*size = obsx265->extra_data_size;
	return true;
}

static bool obs_x265_sei(void *data, uint8_t **sei, size_t *size)
{
	struct obs_x265 *obsx265 = data;

	if (!obsx265->context)
		return false;

	*sei = obsx265->sei;
	*size = obsx265->sei_size;
	return true;
}

static inline bool valid_format(enum video_format format)
{
	return format == VIDEO_FORMAT_I420 || format == VIDEO_FORMAT_I444 ||
	       format == VIDEO_FORMAT_I010;
}

static void obs_x265_video_info(void *data, struct video_scale_info *info)
{
	struct obs_x265 *obsx265 = data;
	enum video_format pref_format;

	pref_format = obs_encoder_get_preferred_video_format(obsx265->encoder);

	if (!valid_format(pref_format)) {
		if (info->format == VIDEO_FORMAT_P010) {
			pref_format = VIDEO_FORMAT_I010;
		} else {
			pref_format = valid_format(info->format)
					      ? info->format
					      : VIDEO_FORMAT_I420;
		}
	}

	info->format = pref_format;
}

struct obs_encoder_info obs_x265_encoder = {
	.id = "obs_x265",
	.type = OBS_ENCODER_VIDEO,
	.codec = "hevc",
	.get_name = obs_x265_getname,
	.create = obs_x265_create,
	.destroy = obs_x265_destroy,
	.encode = obs_x265_encode,
	.update = obs_x265_update,
	.get_properties = obs_x265_props,
	.get_defaults = obs_x265_defaults,
	.get_extra_data = obs_x265_extra_data,
	.get_sei_data = obs_x265_sei,
	.get_video_info = obs_x265_video_info,
	.caps = OBS_ENCODER_CAP_DYN_BITRATE,
};
