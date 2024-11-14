/*

This file is provided under a dual BSD/GPLv2 license.  When using or
redistributing this file, you may do so under either license.

GPL LICENSE SUMMARY

Copyright(c) Oct. 2015 Intel Corporation.

This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

Contact Information:

Seung-Woo Kim, seung-woo.kim@intel.com
705 5th Ave S #500, Seattle, WA 98104

BSD LICENSE

Copyright(c) <date> Intel Corporation.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in
the documentation and/or other materials provided with the
distribution.

* Neither the name of Intel Corporation nor the names of its
contributors may be used to endorse or promote products derived
from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <inttypes.h>
#include <util/dstr.h>
#include <util/darray.h>
#include <util/platform.h>
#include <util/threading.h>
#include <obs-module.h>
#include <obs-hevc.h>
#include <obs-avc.h>

#include "QSV_Encoder.h"
#include "common_utils.h"

#define do_log(level, format, ...) \
	blog(level, "[qsv encoder: '%s'] " format, obs_encoder_get_name(obsqsv->encoder), ##__VA_ARGS__)

#define error(format, ...) do_log(LOG_ERROR, format, ##__VA_ARGS__)
#define warn(format, ...) do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...) do_log(LOG_INFO, format, ##__VA_ARGS__)
#define debug(format, ...) do_log(LOG_DEBUG, format, ##__VA_ARGS__)

#ifndef GS_INVALID_HANDLE
#define GS_INVALID_HANDLE (uint32_t) - 1
#endif

/* ------------------------------------------------------------------------- */

struct obs_qsv {
	obs_encoder_t *encoder;

	enum qsv_codec codec;

	qsv_param_t params;
	qsv_t *context;

	DARRAY(uint8_t) packet_data;

	uint8_t *extra_data;
	uint8_t *sei;

	size_t extra_data_size;
	size_t sei_size;

	os_performance_token_t *performance_token;

	uint32_t roi_increment;
};

/* ------------------------------------------------------------------------- */

static pthread_mutex_t g_QsvLock = PTHREAD_MUTEX_INITIALIZER;
static unsigned short g_verMajor;
static unsigned short g_verMinor;

static const char *obs_qsv_getname_v1(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return "QuickSync H.264 (v1 deprecated)";
}

static const char *obs_qsv_getname(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return "QuickSync H.264";
}

static const char *obs_qsv_getname_av1(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return "QuickSync AV1";
}

static const char *obs_qsv_getname_hevc(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return "QuickSync HEVC";
}

static void clear_data(struct obs_qsv *obsqsv)
{
	if (obsqsv->context) {
		pthread_mutex_lock(&g_QsvLock);
		qsv_encoder_close(obsqsv->context);
		obsqsv->context = NULL;
		pthread_mutex_unlock(&g_QsvLock);

		// bfree(obsqsv->sei);
		bfree(obsqsv->extra_data);

		// obsqsv->sei = NULL;
		obsqsv->extra_data = NULL;
	}
}

static void obs_qsv_destroy(void *data)
{
	struct obs_qsv *obsqsv = (struct obs_qsv *)data;

	if (obsqsv) {
		os_end_high_performance(obsqsv->performance_token);
		clear_data(obsqsv);
		da_free(obsqsv->packet_data);
		bfree(obsqsv);
	}
}

static void obs_qsv_defaults(obs_data_t *settings, int ver, enum qsv_codec codec)
{
	obs_data_set_default_string(settings, "target_usage", "TU4");
	obs_data_set_default_int(settings, "bitrate", 2500);
	obs_data_set_default_int(settings, "max_bitrate", 3000);
	obs_data_set_default_string(settings, "profile", codec == QSV_CODEC_AVC ? "high" : "main");
	obs_data_set_default_string(settings, "rate_control", "CBR");

	obs_data_set_default_int(settings, "__ver", ver);

	obs_data_set_default_int(settings, "cqp", 23);
	obs_data_set_default_int(settings, "qpi", 23);
	obs_data_set_default_int(settings, "qpp", 23);
	obs_data_set_default_int(settings, "qpb", 23);
	obs_data_set_default_int(settings, "icq_quality", 23);

	obs_data_set_default_int(settings, "keyint_sec", 0);
	obs_data_set_default_string(settings, "latency", "normal");
	obs_data_set_default_int(settings, "bframes", 3);
	obs_data_set_default_bool(settings, "repeat_headers", false);
}

static void obs_qsv_defaults_h264_v1(obs_data_t *settings)
{
	obs_qsv_defaults(settings, 1, QSV_CODEC_AVC);
}

static void obs_qsv_defaults_h264_v2(obs_data_t *settings)
{
	obs_qsv_defaults(settings, 2, QSV_CODEC_AVC);
}

static void obs_qsv_defaults_av1(obs_data_t *settings)
{
	obs_qsv_defaults(settings, 2, QSV_CODEC_AV1);
}

static void obs_qsv_defaults_hevc(obs_data_t *settings)
{
	obs_qsv_defaults(settings, 2, QSV_CODEC_HEVC);
}

static inline void add_strings(obs_property_t *list, const char *const *strings)
{
	while (*strings) {
		obs_property_list_add_string(list, *strings, *strings);
		strings++;
	}
}

static inline void add_translated_strings(obs_property_t *list, const char *const *keys, const char *const *strings)
{
	while (*keys && *strings) {
		obs_property_list_add_string(list, obs_module_text(*keys), *strings);
		keys++;
		strings++;
	}
}

#define TEXT_SPEED obs_module_text("TargetUsage")
#define TEXT_TARGET_BITRATE obs_module_text("Bitrate")
#define TEXT_MAX_BITRATE obs_module_text("MaxBitrate")
#define TEXT_PROFILE obs_module_text("Profile")
#define TEXT_LATENCY obs_module_text("Latency")
#define TEXT_RATE_CONTROL obs_module_text("RateControl")
#define TEXT_ICQ_QUALITY obs_module_text("ICQQuality")
#define TEXT_KEYINT_SEC obs_module_text("KeyframeIntervalSec")
#define TEXT_BFRAMES obs_module_text("BFrames")

static bool update_latency(obs_data_t *settings)
{
	bool update = false;
	int async_depth = 4;
	if (obs_data_item_byname(settings, "async_depth") != NULL) {
		async_depth = (int)obs_data_get_int(settings, "async_depth");
		obs_data_erase(settings, "async_depth");
		update = true;
	}

	int la_depth = 15;
	if (obs_data_item_byname(settings, "la_depth") != NULL) {
		la_depth = (int)obs_data_get_int(settings, "la_depth");
		obs_data_erase(settings, "la_depth");
		update = true;
	}

	const char *rate_control = obs_data_get_string(settings, "rate_control");

	bool lookahead = false;
	if (astrcmpi(rate_control, "LA_CBR") == 0) {
		obs_data_set_string(settings, "rate_control", "CBR");
		lookahead = true;
	} else if (astrcmpi(rate_control, "LA_VBR") == 0) {
		obs_data_set_string(settings, "rate_control", "VBR");
		lookahead = true;
	} else if (astrcmpi(rate_control, "LA_ICQ") == 0) {
		obs_data_set_string(settings, "rate_control", "ICQ");
		lookahead = true;
	}

	if (update) {
		if (lookahead) {
			if (la_depth == 0 || la_depth >= 15)
				obs_data_set_string(settings, "latency", "normal");
			else
				obs_data_set_string(settings, "latency", "low");
		} else {
			if (async_depth != 1)
				obs_data_set_string(settings, "latency", "normal");
			else
				obs_data_set_string(settings, "latency", "ultra-low");
		}
	}

	return true;
}

static bool update_ratecontrol(obs_data_t *settings)
{
	const char *rate_control = obs_data_get_string(settings, "rate_control");

	if (astrcmpi(rate_control, "VCM") == 0) {
		obs_data_set_string(settings, "rate_control", "CBR");
	} else if (astrcmpi(rate_control, "AVBR") == 0) {
		obs_data_set_string(settings, "rate_control", "VBR");
	}

	return true;
}

static void update_targetusage(obs_data_t *settings)
{
	const char *target_usage = obs_data_get_string(settings, "target_usage");

	if (astrcmpi(target_usage, "veryslow") == 0 || astrcmpi(target_usage, "quality") == 0)
		obs_data_set_string(settings, "target_usage", "TU1");
	else if (astrcmpi(target_usage, "slower") == 0)
		obs_data_set_string(settings, "target_usage", "TU2");
	else if (astrcmpi(target_usage, "slow") == 0)
		obs_data_set_string(settings, "target_usage", "TU3");
	else if (astrcmpi(target_usage, "medium") == 0 || astrcmpi(target_usage, "balanced") == 0)
		obs_data_set_string(settings, "target_usage", "TU4");
	else if (astrcmpi(target_usage, "fast") == 0)
		obs_data_set_string(settings, "target_usage", "TU5");
	else if (astrcmpi(target_usage, "faster") == 0)
		obs_data_set_string(settings, "target_usage", "TU6");
	else if (astrcmpi(target_usage, "veryfast") == 0 || astrcmpi(target_usage, "speed") == 0)
		obs_data_set_string(settings, "target_usage", "TU7");
}

static bool rate_control_modified(obs_properties_t *ppts, obs_property_t *p, obs_data_t *settings)
{
	const char *rate_control = obs_data_get_string(settings, "rate_control");

	bool bVisible = astrcmpi(rate_control, "VBR") == 0;
	p = obs_properties_get(ppts, "max_bitrate");
	obs_property_set_visible(p, bVisible);

	bVisible = astrcmpi(rate_control, "CQP") == 0 || astrcmpi(rate_control, "ICQ") == 0;
	p = obs_properties_get(ppts, "bitrate");
	obs_property_set_visible(p, !bVisible);

	bVisible = astrcmpi(rate_control, "CQP") == 0;
	p = obs_properties_get(ppts, "qpi");
	if (p)
		obs_property_set_visible(p, bVisible);
	p = obs_properties_get(ppts, "qpb");
	if (p)
		obs_property_set_visible(p, bVisible);
	p = obs_properties_get(ppts, "qpp");
	if (p)
		obs_property_set_visible(p, bVisible);
	p = obs_properties_get(ppts, "cqp");
	if (p)
		obs_property_set_visible(p, bVisible);

	bVisible = astrcmpi(rate_control, "ICQ") == 0;
	p = obs_properties_get(ppts, "icq_quality");
	obs_property_set_visible(p, bVisible);

	update_latency(settings);
	update_targetusage(settings);
	update_ratecontrol(settings);

	return true;
}

static bool profile_modified(obs_properties_t *ppts, obs_property_t *p, obs_data_t *settings)
{
	const char *profile = obs_data_get_string(settings, "profile");
	enum qsv_cpu_platform plat = qsv_get_cpu_platform();
	bool bVisible = ((astrcmpi(profile, "high") == 0) &&
			 (plat >= QSV_CPU_PLATFORM_ICL || plat == QSV_CPU_PLATFORM_UNKNOWN));
	p = obs_properties_get(ppts, "CQM");
	obs_property_set_visible(p, bVisible);
	return true;
}

static inline void add_rate_controls(obs_property_t *list, const struct qsv_rate_control_info *rc)
{
	enum qsv_cpu_platform plat = qsv_get_cpu_platform();
	while (rc->name) {
		if (!rc->haswell_or_greater || (plat >= QSV_CPU_PLATFORM_HSW || plat == QSV_CPU_PLATFORM_UNKNOWN))
			obs_property_list_add_string(list, rc->name, rc->name);
		rc++;
	}
}

static obs_properties_t *obs_qsv_props(enum qsv_codec codec, void *unused, int ver)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *props = obs_properties_create();
	obs_property_t *prop;

	prop = obs_properties_add_list(props, "rate_control", TEXT_RATE_CONTROL, OBS_COMBO_TYPE_LIST,
				       OBS_COMBO_FORMAT_STRING);

	add_rate_controls(prop, qsv_ratecontrols);

	obs_property_set_modified_callback(prop, rate_control_modified);

	prop = obs_properties_add_int(props, "bitrate", TEXT_TARGET_BITRATE, 50, 10000000, 50);
	obs_property_int_set_suffix(prop, " Kbps");

	prop = obs_properties_add_int(props, "max_bitrate", TEXT_MAX_BITRATE, 50, 10000000, 50);
	obs_property_int_set_suffix(prop, " Kbps");

	if (ver >= 2) {
		obs_properties_add_int(props, "cqp", "CQP", 1, codec == QSV_CODEC_AV1 ? 63 : 51, 1);
	} else {
		obs_properties_add_int(props, "qpi", "QPI", 1, 51, 1);
		obs_properties_add_int(props, "qpp", "QPP", 1, 51, 1);
		obs_properties_add_int(props, "qpb", "QPB", 1, 51, 1);
	}

	obs_properties_add_int(props, "icq_quality", TEXT_ICQ_QUALITY, 1, 51, 1);

	prop = obs_properties_add_list(props, "target_usage", TEXT_SPEED, OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	add_translated_strings(prop, qsv_usage_translation_keys, qsv_usage_names);

	prop = obs_properties_add_list(props, "profile", TEXT_PROFILE, OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	if (codec == QSV_CODEC_AVC)
		add_strings(prop, qsv_profile_names);
	else if (codec == QSV_CODEC_AV1)
		add_strings(prop, qsv_profile_names_av1);
	else if (codec == QSV_CODEC_HEVC)
		add_strings(prop, qsv_profile_names_hevc);

	obs_property_set_modified_callback(prop, profile_modified);

	prop = obs_properties_add_int(props, "keyint_sec", TEXT_KEYINT_SEC, 0, 20, 1);
	obs_property_int_set_suffix(prop, " s");

	prop = obs_properties_add_list(props, "latency", TEXT_LATENCY, OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	add_strings(prop, qsv_latency_names);
	obs_property_set_long_description(prop, obs_module_text("Latency.ToolTip"));

	obs_properties_add_int(props, "bframes", TEXT_BFRAMES, 0, 3, 1);

	return props;
}

static obs_properties_t *obs_qsv_props_h264(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_qsv_props(QSV_CODEC_AVC, unused, 1);
}

static obs_properties_t *obs_qsv_props_h264_v2(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_qsv_props(QSV_CODEC_AVC, unused, 2);
}

static obs_properties_t *obs_qsv_props_av1(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_qsv_props(QSV_CODEC_AV1, unused, 2);
}

static obs_properties_t *obs_qsv_props_hevc(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_qsv_props(QSV_CODEC_HEVC, unused, 2);
}

static void update_params(struct obs_qsv *obsqsv, obs_data_t *settings)
{
	video_t *video = obs_encoder_video(obsqsv->encoder);
	const struct video_output_info *voi = video_output_get_info(video);
	update_latency(settings);
	update_targetusage(settings);

	const char *target_usage = obs_data_get_string(settings, "target_usage");
	const char *profile = obs_data_get_string(settings, "profile");
	const char *rate_control = obs_data_get_string(settings, "rate_control");
	const char *latency = obs_data_get_string(settings, "latency");
	int target_bitrate = (int)obs_data_get_int(settings, "bitrate");
	int max_bitrate = (int)obs_data_get_int(settings, "max_bitrate");
	int qpi = (int)obs_data_get_int(settings, "qpi");
	int qpp = (int)obs_data_get_int(settings, "qpp");
	int qpb = (int)obs_data_get_int(settings, "qpb");
	int cqp = (int)obs_data_get_int(settings, "cqp");
	int ver = (int)obs_data_get_int(settings, "__ver");
	int icq_quality = (int)obs_data_get_int(settings, "icq_quality");
	int keyint_sec = (int)obs_data_get_int(settings, "keyint_sec");
	bool cbr_override = obs_data_get_bool(settings, "cbr");
	int bFrames = (int)obs_data_get_int(settings, "bframes");
	bool repeat_headers = obs_data_get_bool(settings, "repeat_headers");
	const char *codec = "";

	if (obs_data_has_user_value(settings, "bf"))
		bFrames = (int)obs_data_get_int(settings, "bf");

	enum qsv_cpu_platform plat = qsv_get_cpu_platform();
	if (plat == QSV_CPU_PLATFORM_IVB || plat == QSV_CPU_PLATFORM_SNB)
		bFrames = 0;

	int width = (int)obs_encoder_get_width(obsqsv->encoder);
	int height = (int)obs_encoder_get_height(obsqsv->encoder);
	if (astrcmpi(target_usage, "TU1") == 0)
		obsqsv->params.nTargetUsage = MFX_TARGETUSAGE_BEST_QUALITY;
	else if (astrcmpi(target_usage, "TU4") == 0)
		obsqsv->params.nTargetUsage = MFX_TARGETUSAGE_BALANCED;
	else if (astrcmpi(target_usage, "TU7") == 0)
		obsqsv->params.nTargetUsage = MFX_TARGETUSAGE_BEST_SPEED;
	else if (astrcmpi(target_usage, "TU2") == 0)
		obsqsv->params.nTargetUsage = MFX_TARGETUSAGE_2;
	else if (astrcmpi(target_usage, "TU3") == 0)
		obsqsv->params.nTargetUsage = MFX_TARGETUSAGE_3;
	else if (astrcmpi(target_usage, "TU5") == 0)
		obsqsv->params.nTargetUsage = MFX_TARGETUSAGE_5;
	else if (astrcmpi(target_usage, "TU6") == 0)
		obsqsv->params.nTargetUsage = MFX_TARGETUSAGE_6;

	if (obsqsv->codec == QSV_CODEC_AVC) {
		codec = "H.264";
		if (astrcmpi(profile, "baseline") == 0)
			obsqsv->params.nCodecProfile = MFX_PROFILE_AVC_BASELINE;
		else if (astrcmpi(profile, "main") == 0)
			obsqsv->params.nCodecProfile = MFX_PROFILE_AVC_MAIN;
		else if (astrcmpi(profile, "high") == 0)
			obsqsv->params.nCodecProfile = MFX_PROFILE_AVC_HIGH;

	} else if (obsqsv->codec == QSV_CODEC_HEVC) {
		codec = "HEVC";
		if (astrcmpi(profile, "main") == 0) {
			obsqsv->params.nCodecProfile = MFX_PROFILE_HEVC_MAIN;
			if (obs_p010_tex_active()) {
				blog(LOG_WARNING, "[qsv encoder] Forcing main10 for P010");
				obsqsv->params.nCodecProfile = MFX_PROFILE_HEVC_MAIN10;
			}

		} else if (astrcmpi(profile, "main10") == 0) {
			obsqsv->params.nCodecProfile = MFX_PROFILE_HEVC_MAIN10;
		}

	} else if (obsqsv->codec == QSV_CODEC_AV1) {
		codec = "AV1";
		obsqsv->params.nCodecProfile = MFX_PROFILE_AV1_MAIN;
	}

	obsqsv->params.VideoFormat = 5;
	obsqsv->params.VideoFullRange = voi->range == VIDEO_RANGE_FULL;

	switch (voi->colorspace) {
	case VIDEO_CS_601:
		obsqsv->params.ColourPrimaries = 6;
		obsqsv->params.TransferCharacteristics = 6;
		obsqsv->params.MatrixCoefficients = 6;
		obsqsv->params.ChromaSampleLocTypeTopField = 0;
		obsqsv->params.ChromaSampleLocTypeBottomField = 0;
		break;
	case VIDEO_CS_DEFAULT:
	case VIDEO_CS_709:
		obsqsv->params.ColourPrimaries = 1;
		obsqsv->params.TransferCharacteristics = 1;
		obsqsv->params.MatrixCoefficients = 1;
		obsqsv->params.ChromaSampleLocTypeTopField = 0;
		obsqsv->params.ChromaSampleLocTypeBottomField = 0;
		break;
	case VIDEO_CS_SRGB:
		obsqsv->params.ColourPrimaries = 1;
		obsqsv->params.TransferCharacteristics = 13;
		obsqsv->params.MatrixCoefficients = 1;
		obsqsv->params.ChromaSampleLocTypeTopField = 0;
		obsqsv->params.ChromaSampleLocTypeBottomField = 0;
		break;
	case VIDEO_CS_2100_PQ:
		obsqsv->params.ColourPrimaries = 9;
		obsqsv->params.TransferCharacteristics = 16;
		obsqsv->params.MatrixCoefficients = 9;
		obsqsv->params.ChromaSampleLocTypeTopField = 2;
		obsqsv->params.ChromaSampleLocTypeBottomField = 2;
		break;
	case VIDEO_CS_2100_HLG:
		obsqsv->params.ColourPrimaries = 9;
		obsqsv->params.TransferCharacteristics = 18;
		obsqsv->params.MatrixCoefficients = 9;
		obsqsv->params.ChromaSampleLocTypeTopField = 2;
		obsqsv->params.ChromaSampleLocTypeBottomField = 2;
	}

	const bool pq = voi->colorspace == VIDEO_CS_2100_PQ;
	const bool hlg = voi->colorspace == VIDEO_CS_2100_HLG;
	if (pq || hlg) {
		const int hdr_nominal_peak_level = pq ? (int)obs_get_video_hdr_nominal_peak_level() : (hlg ? 1000 : 0);

		obsqsv->params.DisplayPrimariesX[0] = 13250;
		obsqsv->params.DisplayPrimariesX[1] = 7500;
		obsqsv->params.DisplayPrimariesX[2] = 34000;
		obsqsv->params.DisplayPrimariesY[0] = 34500;
		obsqsv->params.DisplayPrimariesY[1] = 3000;
		obsqsv->params.DisplayPrimariesY[2] = 16000;
		obsqsv->params.WhitePointX = 15635;
		obsqsv->params.WhitePointY = 16450;
		obsqsv->params.MaxDisplayMasteringLuminance = hdr_nominal_peak_level * 10000;
		obsqsv->params.MinDisplayMasteringLuminance = 0;

		obsqsv->params.MaxContentLightLevel = hdr_nominal_peak_level;
		obsqsv->params.MaxPicAverageLightLevel = hdr_nominal_peak_level;
	}

	/* internal convenience parameter, overrides rate control param
	 * XXX: Deprecated */
	if (cbr_override) {
		warn("\"cbr\" setting has been deprecated for all encoders!  "
		     "Please set \"rate_control\" to \"CBR\" instead.  "
		     "Forcing CBR mode.  "
		     "(Note to all: this is why you shouldn't use strings for "
		     "common settings)");
		rate_control = "CBR";
	}

	if (astrcmpi(rate_control, "CBR") == 0)
		obsqsv->params.nRateControl = MFX_RATECONTROL_CBR;
	else if (astrcmpi(rate_control, "VBR") == 0)
		obsqsv->params.nRateControl = MFX_RATECONTROL_VBR;
	else if (astrcmpi(rate_control, "CQP") == 0)
		obsqsv->params.nRateControl = MFX_RATECONTROL_CQP;
	else if (astrcmpi(rate_control, "ICQ") == 0)
		obsqsv->params.nRateControl = MFX_RATECONTROL_ICQ;

	obsqsv->params.nLADEPTH = (mfxU16)0;
	if (astrcmpi(latency, "ultra-low") == 0) {
		obsqsv->params.nAsyncDepth = 1;
	} else if (astrcmpi(latency, "low") == 0) {
		obsqsv->params.nAsyncDepth = 4;
		if (obsqsv->params.nRateControl == MFX_RATECONTROL_CBR ||
		    obsqsv->params.nRateControl == MFX_RATECONTROL_VBR)
			obsqsv->params.nLADEPTH = 30;
	} else if (astrcmpi(latency, "normal") == 0) {
		obsqsv->params.nAsyncDepth = 4;
		if (obsqsv->params.nRateControl == MFX_RATECONTROL_CBR ||
		    obsqsv->params.nRateControl == MFX_RATECONTROL_VBR)
			obsqsv->params.nLADEPTH = 60;
	}

	if (obsqsv->params.nLADEPTH > 0) {
		if (obsqsv->params.nLADEPTH > 100)
			obsqsv->params.nLADEPTH = 100;
		else if (obsqsv->params.nLADEPTH < 10)
			obsqsv->params.nLADEPTH = 10;
	}

	if (ver == 1) {
		obsqsv->params.nQPI = (mfxU16)qpi;
		obsqsv->params.nQPP = (mfxU16)qpp;
		obsqsv->params.nQPB = (mfxU16)qpb;
	} else {
		int actual_cqp = cqp;
		if (obsqsv->codec == QSV_CODEC_AV1)
			actual_cqp *= 4;
		obsqsv->params.nQPI = actual_cqp;
		obsqsv->params.nQPP = actual_cqp;
		obsqsv->params.nQPB = actual_cqp;
	}
	obsqsv->params.nTargetBitRate = (mfxU16)target_bitrate;
	obsqsv->params.nMaxBitRate = (mfxU16)max_bitrate;
	obsqsv->params.nWidth = (mfxU16)width;
	obsqsv->params.nHeight = (mfxU16)height;
	obsqsv->params.nFpsNum = (mfxU16)voi->fps_num;
	obsqsv->params.nFpsDen = (mfxU16)voi->fps_den;
	obsqsv->params.nbFrames = (mfxU16)bFrames;
	obsqsv->params.nKeyIntSec = (mfxU16)keyint_sec;
	obsqsv->params.nICQQuality = (mfxU16)icq_quality;
	obsqsv->params.bRepeatHeaders = repeat_headers;

	info("settings:\n"
	     "\tcodec:          %s\n"
	     "\trate_control:   %s",
	     codec, rate_control);

	if (obsqsv->params.nRateControl != MFX_RATECONTROL_ICQ && obsqsv->params.nRateControl != MFX_RATECONTROL_CQP)
		blog(LOG_INFO, "\ttarget_bitrate: %d", (int)obsqsv->params.nTargetBitRate);

	if (obsqsv->params.nRateControl == MFX_RATECONTROL_VBR)
		blog(LOG_INFO, "\tmax_bitrate:    %d", (int)obsqsv->params.nMaxBitRate);

	if (obsqsv->params.nRateControl == MFX_RATECONTROL_ICQ)
		blog(LOG_INFO, "\tICQ Quality:    %d", (int)obsqsv->params.nICQQuality);

	if (obsqsv->params.nLADEPTH)
		blog(LOG_INFO, "\tLookahead Depth:%d", (int)obsqsv->params.nLADEPTH);

	if (obsqsv->params.nRateControl == MFX_RATECONTROL_CQP)
		blog(LOG_INFO,
		     "\tqpi:            %d\n"
		     "\tqpb:            %d\n"
		     "\tqpp:            %d",
		     qpi, qpb, qpp);

	blog(LOG_INFO,
	     "\ttarget_usage:   %s\n"
	     "\tprofile:        %s\n"
	     "\tkeyint:         %d\n"
	     "\tlatency:        %s\n"
	     "\tb-frames:       %d\n"
	     "\tfps_num:        %d\n"
	     "\tfps_den:        %d\n"
	     "\twidth:          %d\n"
	     "\theight:         %d",
	     target_usage, profile, keyint_sec, latency, bFrames, voi->fps_num, voi->fps_den, width, height);

	info("debug info:");
}

static bool update_settings(struct obs_qsv *obsqsv, obs_data_t *settings)
{
	update_params(obsqsv, settings);
	return true;
}

static void load_hevc_headers(struct obs_qsv *obsqsv)
{
	DARRAY(uint8_t) header;
	DARRAY(uint8_t) sei;

	da_init(header);
	da_init(sei);

	uint8_t *pVPS, *pSPS, *pPPS;
	uint16_t nVPS, nSPS, nPPS;
	qsv_hevc_encoder_headers(obsqsv->context, &pVPS, &pSPS, &pPPS, &nVPS, &nSPS, &nPPS);
	da_push_back_array(header, pVPS, nVPS);
	da_push_back_array(header, pSPS, nSPS);
	da_push_back_array(header, pPPS, nPPS);

	obsqsv->extra_data = header.array;
	obsqsv->extra_data_size = header.num;
	obsqsv->sei = sei.array;
	obsqsv->sei_size = sei.num;
}

static void load_headers(struct obs_qsv *obsqsv)
{
	DARRAY(uint8_t) header;
	static uint8_t sei = 0;

	// Not sure if SEI is needed.
	// Just filling in empty meaningless SEI message.
	// Seems to work fine.
	// DARRAY(uint8_t) sei;

	da_init(header);
	// da_init(sei);

	uint8_t *pSPS, *pPPS;
	uint16_t nSPS, nPPS;

	qsv_encoder_headers(obsqsv->context, &pSPS, &pPPS, &nSPS, &nPPS);
	da_push_back_array(header, pSPS, nSPS);

	// AV1 does not need PPS
	if (obsqsv->codec != QSV_CODEC_AV1)
		da_push_back_array(header, pPPS, nPPS);

	obsqsv->extra_data = header.array;
	obsqsv->extra_data_size = header.num;
	obsqsv->sei = &sei;
	obsqsv->sei_size = 1;
}

static bool obs_qsv_update(void *data, obs_data_t *settings)
{
	struct obs_qsv *obsqsv = data;
	obsqsv->params.nTargetBitRate = (mfxU16)obs_data_get_int(settings, "bitrate");

	if (!qsv_encoder_reconfig(obsqsv->context, &obsqsv->params)) {
		warn("Failed to reconfigure");
		return false;
	}

	return true;
}

static void *obs_qsv_create(enum qsv_codec codec, obs_data_t *settings, obs_encoder_t *encoder, bool useTexAlloc)
{
	struct obs_qsv *obsqsv = bzalloc(sizeof(struct obs_qsv));
	obsqsv->encoder = encoder;
	obsqsv->codec = codec;

	video_t *video = obs_encoder_video(encoder);
	const struct video_output_info *voi = video_output_get_info(video);
	switch (voi->format) {
	case VIDEO_FORMAT_I010:
	case VIDEO_FORMAT_P010:
		if (codec == QSV_CODEC_AVC) {
			const char *const text = obs_module_text("10bitUnsupportedAvc");
			obs_encoder_set_last_error(encoder, text);
			error("%s", text);
			bfree(obsqsv);
			return NULL;
		}
		obsqsv->params.video_fmt_10bit = true;
		break;
	case VIDEO_FORMAT_P216:
	case VIDEO_FORMAT_P416: {
		const char *const text = obs_module_text("16bitUnsupported");
		obs_encoder_set_last_error(encoder, text);
		error("%s", text);
		bfree(obsqsv);
		return NULL;
	}
	default:
		switch (voi->colorspace) {
		case VIDEO_CS_2100_PQ:
		case VIDEO_CS_2100_HLG: {
			const char *const text = obs_module_text("8bitUnsupportedHdr");
			obs_encoder_set_last_error(encoder, text);
			error("%s", text);
			bfree(obsqsv);
			return NULL;
		}
		}
	}

	if (update_settings(obsqsv, settings)) {
		pthread_mutex_lock(&g_QsvLock);
		obsqsv->context = qsv_encoder_open(&obsqsv->params, codec, useTexAlloc);
		pthread_mutex_unlock(&g_QsvLock);

		if (obsqsv->context == NULL)
			warn("qsv failed to load");
		else if (obsqsv->codec == QSV_CODEC_HEVC)
			load_hevc_headers(obsqsv);
		else
			load_headers(obsqsv);
	} else {
		warn("bad settings specified");
	}

	qsv_encoder_version(&g_verMajor, &g_verMinor);

	blog(LOG_INFO,
	     "\tmajor:          %d\n"
	     "\tminor:          %d",
	     g_verMajor, g_verMinor);

	if (!obsqsv->context) {
		bfree(obsqsv);
		return NULL;
	}

	obsqsv->performance_token = os_request_high_performance("qsv encoding");

	return obsqsv;
}

static void *obs_qsv_create_h264(obs_data_t *settings, obs_encoder_t *encoder)
{
	return obs_qsv_create(QSV_CODEC_AVC, settings, encoder, false);
}

static void *obs_qsv_create_av1(obs_data_t *settings, obs_encoder_t *encoder)
{
	return obs_qsv_create(QSV_CODEC_AV1, settings, encoder, false);
}

static void *obs_qsv_create_hevc(obs_data_t *settings, obs_encoder_t *encoder)
{
	return obs_qsv_create(QSV_CODEC_HEVC, settings, encoder, false);
}

static void *obs_qsv_create_tex(enum qsv_codec codec, obs_data_t *settings, obs_encoder_t *encoder,
				const char *fallback_id)
{
	struct obs_video_info ovi;
	obs_get_video_info(&ovi);

	if (!adapters[ovi.adapter].is_intel) {
		blog(LOG_INFO, ">>> app not on intel GPU, fall back to old qsv encoder");
		return obs_encoder_create_rerouted(encoder, (const char *)fallback_id);
	}

	if (codec == QSV_CODEC_AV1 && !adapters[ovi.adapter].supports_av1) {
		blog(LOG_INFO, ">>> cap on different device, fall back to non-texture sharing AV1 qsv encoder");
		return obs_encoder_create_rerouted(encoder, (const char *)fallback_id);
	}

	bool gpu_texture_active = obs_nv12_tex_active();

	if (codec != QSV_CODEC_AVC)
		gpu_texture_active = gpu_texture_active || obs_p010_tex_active();

	if (!gpu_texture_active) {
		blog(LOG_INFO, ">>> gpu tex not active, fall back to old qsv encoder");
		return obs_encoder_create_rerouted(encoder, (const char *)fallback_id);
	}

	if (obs_encoder_scaling_enabled(encoder)) {
		if (!obs_encoder_gpu_scaling_enabled(encoder)) {
			blog(LOG_INFO, ">>> encoder CPU scaling active, fall back to old qsv encoder");
			return obs_encoder_create_rerouted(encoder, (const char *)fallback_id);
		}
		blog(LOG_INFO, ">>> encoder GPU scaling active");
	}

	blog(LOG_INFO, ">>> new qsv encoder");
	return obs_qsv_create(codec, settings, encoder, true);
}

static void *obs_qsv_create_tex_h264(obs_data_t *settings, obs_encoder_t *encoder)
{
	return obs_qsv_create_tex(QSV_CODEC_AVC, settings, encoder, "obs_qsv11_soft");
}

static void *obs_qsv_create_tex_h264_v2(obs_data_t *settings, obs_encoder_t *encoder)
{
	return obs_qsv_create_tex(QSV_CODEC_AVC, settings, encoder, "obs_qsv11_soft_v2");
}

static void *obs_qsv_create_tex_av1(obs_data_t *settings, obs_encoder_t *encoder)
{
	return obs_qsv_create_tex(QSV_CODEC_AV1, settings, encoder, "obs_qsv11_av1_soft");
}

static void *obs_qsv_create_tex_hevc(obs_data_t *settings, obs_encoder_t *encoder)
{
	return obs_qsv_create_tex(QSV_CODEC_HEVC, settings, encoder, "obs_qsv11_hevc_soft");
}

static bool obs_qsv_extra_data(void *data, uint8_t **extra_data, size_t *size)
{
	struct obs_qsv *obsqsv = data;

	if (!obsqsv->context)
		return false;

	*extra_data = obsqsv->extra_data;
	*size = obsqsv->extra_data_size;
	return true;
}

static bool obs_qsv_sei(void *data, uint8_t **sei, size_t *size)
{
	struct obs_qsv *obsqsv = data;

	if (!obsqsv->context)
		return false;

	*sei = obsqsv->sei;
	*size = obsqsv->sei_size;
	return true;
}

static inline bool valid_format(enum video_format format)
{
	return format == VIDEO_FORMAT_NV12;
}

static inline bool valid_av1_format(enum video_format format)
{
	return format == VIDEO_FORMAT_NV12 || format == VIDEO_FORMAT_P010;
}

static inline void cap_resolution(struct obs_qsv *obsqsv, struct video_scale_info *info)
{
	enum qsv_cpu_platform qsv_platform = qsv_get_cpu_platform();
	uint32_t width = obs_encoder_get_width(obsqsv->encoder);
	uint32_t height = obs_encoder_get_height(obsqsv->encoder);

	if (adapters[adapter_index].is_dgpu == true)
		qsv_platform = QSV_CPU_PLATFORM_UNKNOWN;

	info->height = height;
	info->width = width;

	if (qsv_platform <= QSV_CPU_PLATFORM_IVB && qsv_platform != QSV_CPU_PLATFORM_UNKNOWN) {
		if (width > 1920) {
			info->width = 1920;
		}

		if (height > 1200) {
			info->height = 1200;
		}
	}
}

static void obs_qsv_video_info(void *data, struct video_scale_info *info)
{
	struct obs_qsv *obsqsv = data;
	enum video_format pref_format;

	pref_format = obs_encoder_get_preferred_video_format(obsqsv->encoder);

	if (!valid_format(pref_format)) {
		pref_format = valid_format(info->format) ? info->format : VIDEO_FORMAT_NV12;
	}

	info->format = pref_format;
	cap_resolution(obsqsv, info);
}

static void obs_qsv_video_plus_hdr_info(void *data, struct video_scale_info *info)
{
	struct obs_qsv *obsqsv = data;
	enum video_format pref_format;

	pref_format = obs_encoder_get_preferred_video_format(obsqsv->encoder);

	if (!valid_av1_format(pref_format)) {
		pref_format = valid_av1_format(info->format) ? info->format : VIDEO_FORMAT_NV12;
	}

	info->format = pref_format;
	cap_resolution(obsqsv, info);
}

static mfxU64 ts_obs_to_mfx(int64_t ts, const struct video_output_info *voi)
{
	return ts * 90000 / voi->fps_num;
}

static int64_t ts_mfx_to_obs(mfxI64 ts, const struct video_output_info *voi)
{
	int64_t div = 90000 * (int64_t)voi->fps_den;
	/* Round to the nearest integer multiple of `voi->fps_den`. */
	if (ts < 0)
		return (ts * voi->fps_num - div / 2) / div * voi->fps_den;
	else
		return (ts * voi->fps_num + div / 2) / div * voi->fps_den;
}

static void parse_packet(struct obs_qsv *obsqsv, struct encoder_packet *packet, mfxBitstream *pBS,
			 const struct video_output_info *voi, bool *received_packet)
{
	uint8_t *start, *end;
	int type;

	if (pBS == NULL || pBS->DataLength == 0) {
		*received_packet = false;
		return;
	}

	da_resize(obsqsv->packet_data, 0);
	da_push_back_array(obsqsv->packet_data, &pBS->Data[pBS->DataOffset], pBS->DataLength);

	packet->data = obsqsv->packet_data.array;
	packet->size = obsqsv->packet_data.num;
	packet->type = OBS_ENCODER_VIDEO;
	packet->pts = ts_mfx_to_obs((mfxI64)pBS->TimeStamp, voi);
	packet->keyframe = (pBS->FrameType & MFX_FRAMETYPE_IDR);

	uint16_t frameType = pBS->FrameType;
	uint8_t priority;

	if (frameType & MFX_FRAMETYPE_I)
		priority = OBS_NAL_PRIORITY_HIGHEST;
	else if ((frameType & MFX_FRAMETYPE_P) || (frameType & MFX_FRAMETYPE_REF))
		priority = OBS_NAL_PRIORITY_HIGH;
	else
		priority = 0;

	packet->priority = priority;

	/* ------------------------------------ */

	start = obsqsv->packet_data.array;
	end = start + obsqsv->packet_data.num;

	start = (uint8_t *)obs_avc_find_startcode(start, end);
	while (true) {
		while (start < end && !*(start++))
			;

		if (start == end)
			break;

		type = start[0] & 0x1F;
		if (type == OBS_NAL_SLICE_IDR || type == OBS_NAL_SLICE) {
			start[0] &= ~(3 << 5);
			start[0] |= priority << 5; //0 for non-ref frames and not equal to 0 for ref frames
		}

		start = (uint8_t *)obs_avc_find_startcode(start, end);
	}

	/* ------------------------------------ */

	packet->dts = ts_mfx_to_obs(pBS->DecodeTimeStamp, voi);

#if 0
	bool iFrame = pBS->FrameType & MFX_FRAMETYPE_I;
	bool bFrame = pBS->FrameType & MFX_FRAMETYPE_B;
	bool pFrame = pBS->FrameType & MFX_FRAMETYPE_P;
	int iType = iFrame ? 0 : (bFrame ? 1 : (pFrame ? 2 : -1));

	info("parse packet:\n"
		"\tFrameType: %d\n"
		"\tpts:       %d\n"
		"\tdts:       %d",
		iType, packet->pts, packet->dts);
#endif

	*received_packet = true;
	pBS->DataLength = 0;
}

static void parse_packet_av1(struct obs_qsv *obsqsv, struct encoder_packet *packet, mfxBitstream *pBS,
			     const struct video_output_info *voi, bool *received_packet)
{
	if (pBS == NULL || pBS->DataLength == 0) {
		*received_packet = false;
		return;
	}

	da_resize(obsqsv->packet_data, 0);
	da_push_back_array(obsqsv->packet_data, &pBS->Data[pBS->DataOffset], pBS->DataLength);

	packet->data = obsqsv->packet_data.array;
	packet->size = obsqsv->packet_data.num;
	packet->type = OBS_ENCODER_VIDEO;
	packet->pts = ts_mfx_to_obs((mfxI64)pBS->TimeStamp, voi);
	packet->keyframe = (pBS->FrameType & MFX_FRAMETYPE_IDR);

	uint16_t frameType = pBS->FrameType;
	uint8_t priority;

	if (frameType & MFX_FRAMETYPE_I)
		priority = OBS_NAL_PRIORITY_HIGHEST;
	else if ((frameType & MFX_FRAMETYPE_P) || (frameType & MFX_FRAMETYPE_REF))
		priority = OBS_NAL_PRIORITY_HIGH;
	else
		priority = OBS_NAL_PRIORITY_DISPOSABLE;

	packet->priority = priority;

	packet->dts = ts_mfx_to_obs(pBS->DecodeTimeStamp, voi);

#if 0
	info("parse packet:\n"
		"\tFrameType: %d\n"
		"\tpts:       %d\n"
		"\tdts:       %d",
		iType, packet->pts, packet->dts);
#endif

	*received_packet = true;
	pBS->DataLength = 0;
}

static void parse_packet_hevc(struct obs_qsv *obsqsv, struct encoder_packet *packet, mfxBitstream *pBS,
			      const struct video_output_info *voi, bool *received_packet)
{
	if (pBS == NULL || pBS->DataLength == 0) {
		*received_packet = false;
		return;
	}

	da_resize(obsqsv->packet_data, 0);
	da_push_back_array(obsqsv->packet_data, &pBS->Data[pBS->DataOffset], pBS->DataLength);

	packet->data = obsqsv->packet_data.array;
	packet->size = obsqsv->packet_data.num;
	packet->type = OBS_ENCODER_VIDEO;
	packet->pts = ts_mfx_to_obs((mfxI64)pBS->TimeStamp, voi);
	packet->keyframe = (pBS->FrameType & MFX_FRAMETYPE_IDR);

	uint16_t frameType = pBS->FrameType;
	uint8_t priority = OBS_NAL_PRIORITY_DISPOSABLE;

	if (frameType & MFX_FRAMETYPE_I)
		priority = OBS_NAL_PRIORITY_HIGHEST;
	else if ((frameType & MFX_FRAMETYPE_P) || (frameType & MFX_FRAMETYPE_REF))
		priority = OBS_NAL_PRIORITY_HIGH;

	packet->priority = priority;

	/* ------------------------------------ */

	packet->dts = ts_mfx_to_obs(pBS->DecodeTimeStamp, voi);

#if 0
	bool iFrame = pBS->FrameType & MFX_FRAMETYPE_I;
	bool bFrame = pBS->FrameType & MFX_FRAMETYPE_B;
	bool pFrame = pBS->FrameType & MFX_FRAMETYPE_P;

	int iType = iFrame ? 0 : (bFrame ? 1 : (pFrame ? 2 : -1));

	info("parse packet:\n"
		"\tFrameType: %d\n"
		"\tpts:       %d\n"
		"\tdts:       %d",
		iType, packet->pts, packet->dts);
#endif
	*received_packet = true;
	pBS->DataLength = 0;
}

static void roi_cb(void *param, struct obs_encoder_roi *roi)
{
	struct darray *da = param;
	darray_push_back(sizeof(struct obs_encoder_roi), da, roi);
}

static void obs_qsv_setup_rois(struct obs_qsv *obsqsv)
{
	const uint32_t increment = obs_encoder_get_roi_increment(obsqsv->encoder);
	if (obsqsv->roi_increment == increment)
		return;

	qsv_encoder_clear_roi(obsqsv->context);
	/* Because we pass-through the ROIs more or less directly we need to
	 * pass them in reverse order, so make a temporary copy and then use
	 * that instead. */
	DARRAY(struct obs_encoder_roi) rois;
	da_init(rois);

	obs_encoder_enum_roi(obsqsv->encoder, roi_cb, &rois);

	size_t idx = rois.num;
	while (idx)
		qsv_encoder_add_roi(obsqsv->context, &rois.array[--idx]);

	da_free(rois);

	obsqsv->roi_increment = increment;
}

static bool obs_qsv_encode(void *data, struct encoder_frame *frame, struct encoder_packet *packet,
			   bool *received_packet)
{
	struct obs_qsv *obsqsv = data;

	if (!frame || !packet || !received_packet)
		return false;

	pthread_mutex_lock(&g_QsvLock);

	video_t *video = obs_encoder_video(obsqsv->encoder);
	const struct video_output_info *voi = video_output_get_info(video);

	mfxBitstream *pBS = NULL;

	int ret;

	mfxU64 qsvPTS = ts_obs_to_mfx(frame->pts, voi);

	if (obs_encoder_has_roi(obsqsv->encoder))
		obs_qsv_setup_rois(obsqsv);

	// FIXME: remove null check from the top of this function
	// if we actually do expect null frames to complete output.
	if (frame)
		ret = qsv_encoder_encode(obsqsv->context, qsvPTS, frame->data[0], frame->data[1], frame->linesize[0],
					 frame->linesize[1], &pBS);
	else
		ret = qsv_encoder_encode(obsqsv->context, qsvPTS, NULL, NULL, 0, 0, &pBS);

	if (ret < 0) {
		warn("encode failed");
		pthread_mutex_unlock(&g_QsvLock);
		return false;
	}

	if (obsqsv->codec == QSV_CODEC_AVC)
		parse_packet(obsqsv, packet, pBS, voi, received_packet);
	else if (obsqsv->codec == QSV_CODEC_AV1)
		parse_packet_av1(obsqsv, packet, pBS, voi, received_packet);
	else if (obsqsv->codec == QSV_CODEC_HEVC)
		parse_packet_hevc(obsqsv, packet, pBS, voi, received_packet);

	pthread_mutex_unlock(&g_QsvLock);

	return true;
}

static bool obs_qsv_encode_tex(void *data, struct encoder_texture *tex, int64_t pts, uint64_t lock_key,
			       uint64_t *next_key, struct encoder_packet *packet, bool *received_packet)
{
	struct obs_qsv *obsqsv = data;

#ifdef _WIN32
	if (!tex || tex->handle == GS_INVALID_HANDLE) {
#else
	if (!tex || !tex->tex[0] || !tex->tex[1]) {
#endif
		warn("Encode failed: bad texture handle");
		*next_key = lock_key;
		return false;
	}

	if (!packet || !received_packet)
		return false;

	pthread_mutex_lock(&g_QsvLock);

	video_t *video = obs_encoder_video(obsqsv->encoder);
	const struct video_output_info *voi = video_output_get_info(video);

	mfxBitstream *pBS = NULL;

	int ret;

	mfxU64 qsvPTS = ts_obs_to_mfx(pts, voi);

	if (obs_encoder_has_roi(obsqsv->encoder))
		obs_qsv_setup_rois(obsqsv);

	ret = qsv_encoder_encode_tex(obsqsv->context, qsvPTS, (void *)tex, lock_key, next_key, &pBS);

	if (ret < 0) {
		warn("encode failed");
		pthread_mutex_unlock(&g_QsvLock);
		return false;
	}

	if (obsqsv->codec == QSV_CODEC_AVC)
		parse_packet(obsqsv, packet, pBS, voi, received_packet);
	else if (obsqsv->codec == QSV_CODEC_AV1)
		parse_packet_av1(obsqsv, packet, pBS, voi, received_packet);
	else if (obsqsv->codec == QSV_CODEC_HEVC)
		parse_packet_hevc(obsqsv, packet, pBS, voi, received_packet);

	pthread_mutex_unlock(&g_QsvLock);

	return true;
}

struct obs_encoder_info obs_qsv_encoder_tex = {
	.id = "obs_qsv11",
	.type = OBS_ENCODER_VIDEO,
	.codec = "h264",
	.get_name = obs_qsv_getname_v1,
	.create = obs_qsv_create_tex_h264,
	.destroy = obs_qsv_destroy,
	.caps = OBS_ENCODER_CAP_DYN_BITRATE | OBS_ENCODER_CAP_PASS_TEXTURE | OBS_ENCODER_CAP_DEPRECATED,
	.encode_texture2 = obs_qsv_encode_tex,
	.update = obs_qsv_update,
	.get_properties = obs_qsv_props_h264,
	.get_defaults = obs_qsv_defaults_h264_v1,
	.get_extra_data = obs_qsv_extra_data,
	.get_sei_data = obs_qsv_sei,
	.get_video_info = obs_qsv_video_info,
};

struct obs_encoder_info obs_qsv_encoder = {
	.id = "obs_qsv11_soft",
	.type = OBS_ENCODER_VIDEO,
	.codec = "h264",
	.get_name = obs_qsv_getname_v1,
	.create = obs_qsv_create_h264,
	.destroy = obs_qsv_destroy,
	.encode = obs_qsv_encode,
	.update = obs_qsv_update,
	.get_properties = obs_qsv_props_h264,
	.get_defaults = obs_qsv_defaults_h264_v1,
	.get_extra_data = obs_qsv_extra_data,
	.get_sei_data = obs_qsv_sei,
	.get_video_info = obs_qsv_video_info,
	.caps = OBS_ENCODER_CAP_DYN_BITRATE | OBS_ENCODER_CAP_INTERNAL | OBS_ENCODER_CAP_DEPRECATED,
};

struct obs_encoder_info obs_qsv_encoder_tex_v2 = {
	.id = "obs_qsv11_v2",
	.type = OBS_ENCODER_VIDEO,
	.codec = "h264",
	.get_name = obs_qsv_getname,
	.create = obs_qsv_create_tex_h264_v2,
	.destroy = obs_qsv_destroy,
	.caps = OBS_ENCODER_CAP_DYN_BITRATE | OBS_ENCODER_CAP_PASS_TEXTURE | OBS_ENCODER_CAP_ROI,
	.encode_texture2 = obs_qsv_encode_tex,
	.update = obs_qsv_update,
	.get_properties = obs_qsv_props_h264_v2,
	.get_defaults = obs_qsv_defaults_h264_v2,
	.get_extra_data = obs_qsv_extra_data,
	.get_sei_data = obs_qsv_sei,
	.get_video_info = obs_qsv_video_info,
};

struct obs_encoder_info obs_qsv_encoder_v2 = {
	.id = "obs_qsv11_soft_v2",
	.type = OBS_ENCODER_VIDEO,
	.codec = "h264",
	.get_name = obs_qsv_getname,
	.create = obs_qsv_create_h264,
	.destroy = obs_qsv_destroy,
	.encode = obs_qsv_encode,
	.update = obs_qsv_update,
	.get_properties = obs_qsv_props_h264_v2,
	.get_defaults = obs_qsv_defaults_h264_v2,
	.get_extra_data = obs_qsv_extra_data,
	.get_sei_data = obs_qsv_sei,
	.get_video_info = obs_qsv_video_info,
	.caps = OBS_ENCODER_CAP_DYN_BITRATE | OBS_ENCODER_CAP_INTERNAL | OBS_ENCODER_CAP_ROI,
};

struct obs_encoder_info obs_qsv_av1_encoder_tex = {
	.id = "obs_qsv11_av1",
	.type = OBS_ENCODER_VIDEO,
	.codec = "av1",
	.get_name = obs_qsv_getname_av1,
	.create = obs_qsv_create_tex_av1,
	.destroy = obs_qsv_destroy,
	.caps = OBS_ENCODER_CAP_DYN_BITRATE | OBS_ENCODER_CAP_PASS_TEXTURE | OBS_ENCODER_CAP_ROI,
	.encode_texture2 = obs_qsv_encode_tex,
	.update = obs_qsv_update,
	.get_properties = obs_qsv_props_av1,
	.get_defaults = obs_qsv_defaults_av1,
	.get_extra_data = obs_qsv_extra_data,
	.get_video_info = obs_qsv_video_plus_hdr_info,
};

struct obs_encoder_info obs_qsv_av1_encoder = {
	.id = "obs_qsv11_av1_soft",
	.type = OBS_ENCODER_VIDEO,
	.codec = "av1",
	.get_name = obs_qsv_getname_av1,
	.create = obs_qsv_create_av1,
	.destroy = obs_qsv_destroy,
	.encode = obs_qsv_encode,
	.update = obs_qsv_update,
	.get_properties = obs_qsv_props_av1,
	.get_defaults = obs_qsv_defaults_av1,
	.get_extra_data = obs_qsv_extra_data,
	.get_video_info = obs_qsv_video_plus_hdr_info,
	.caps = OBS_ENCODER_CAP_DYN_BITRATE | OBS_ENCODER_CAP_INTERNAL | OBS_ENCODER_CAP_ROI,
};

struct obs_encoder_info obs_qsv_hevc_encoder_tex = {
	.id = "obs_qsv11_hevc",
	.type = OBS_ENCODER_VIDEO,
	.codec = "hevc",
	.get_name = obs_qsv_getname_hevc,
	.create = obs_qsv_create_tex_hevc,
	.destroy = obs_qsv_destroy,
	.caps = OBS_ENCODER_CAP_DYN_BITRATE | OBS_ENCODER_CAP_PASS_TEXTURE | OBS_ENCODER_CAP_ROI,
	.encode_texture2 = obs_qsv_encode_tex,
	.update = obs_qsv_update,
	.get_properties = obs_qsv_props_hevc,
	.get_defaults = obs_qsv_defaults_hevc,
	.get_extra_data = obs_qsv_extra_data,
	.get_video_info = obs_qsv_video_plus_hdr_info,
};

struct obs_encoder_info obs_qsv_hevc_encoder = {
	.id = "obs_qsv11_hevc_soft",
	.type = OBS_ENCODER_VIDEO,
	.codec = "hevc",
	.get_name = obs_qsv_getname_hevc,
	.create = obs_qsv_create_hevc,
	.destroy = obs_qsv_destroy,
	.encode = obs_qsv_encode,
	.update = obs_qsv_update,
	.get_properties = obs_qsv_props_hevc,
	.get_defaults = obs_qsv_defaults_hevc,
	.get_extra_data = obs_qsv_extra_data,
	.get_video_info = obs_qsv_video_plus_hdr_info,
	.caps = OBS_ENCODER_CAP_DYN_BITRATE | OBS_ENCODER_CAP_INTERNAL | OBS_ENCODER_CAP_ROI,
};
