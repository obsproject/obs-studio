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
#include <util/dstr.h>
#include <util/darray.h>
#include <util/platform.h>
#include <obs-module.h>
#include <obs-avc.h>

#ifndef _STDINT_H_INCLUDED
#define _STDINT_H_INCLUDED
#endif

#include "QSV_Encoder.h"
#include <Windows.h>

#define do_log(level, format, ...) \
	blog(level, "[qsv encoder: '%s'] " format, \
			obs_encoder_get_name(obsqsv->encoder), ##__VA_ARGS__)

#define warn(format, ...)  do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...)  do_log(LOG_INFO,    format, ##__VA_ARGS__)
#define debug(format, ...) do_log(LOG_DEBUG,   format, ##__VA_ARGS__)

/* ------------------------------------------------------------------------- */

struct obs_qsv {
	obs_encoder_t          *encoder;

	qsv_param_t            params;
	qsv_t                  *context;

	DARRAY(uint8_t)        packet_data;

	uint8_t                *extra_data;
	uint8_t                *sei;

	size_t                 extra_data_size;
	size_t                 sei_size;

	os_performance_token_t *performance_token;
};

/* ------------------------------------------------------------------------- */

static CRITICAL_SECTION g_QsvCs;
static unsigned short   g_verMajor;
static unsigned short   g_verMinor;
static int64_t          g_pts2dtsShift;
static int64_t          g_prevDts;
static bool             g_bFirst;

static const char *obs_qsv_getname(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return "QuickSync H.264";
}

static void obs_qsv_stop(void *data);

static void clear_data(struct obs_qsv *obsqsv)
{
	if (obsqsv->context) {
		qsv_encoder_close(obsqsv->context);
		// bfree(obsqsv->sei);
		bfree(obsqsv->extra_data);

		obsqsv->context = NULL;
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

static void obs_qsv_defaults(obs_data_t *settings)
{
	obs_data_set_default_string(settings, "target_usage", "balanced");
	obs_data_set_default_int(settings, "bitrate", 2500);
	obs_data_set_default_int(settings, "max_bitrate", 3000);
	obs_data_set_default_string(settings, "profile", "main");
	obs_data_set_default_int(settings, "async_depth", 4);
	obs_data_set_default_string(settings, "rate_control", "CBR");

	obs_data_set_default_int(settings, "accuracy", 1000);
	obs_data_set_default_int(settings, "convergence", 1);
	obs_data_set_default_int(settings, "qpi", 23);
	obs_data_set_default_int(settings, "qpp", 23);
	obs_data_set_default_int(settings, "qpb", 23);
	obs_data_set_default_int(settings, "icq_quality", 23);
	obs_data_set_default_int(settings, "la_depth", 40);

	obs_data_set_default_int(settings, "keyint_sec", 3);
}

static inline void add_strings(obs_property_t *list, const char *const *strings)
{
	while (*strings) {
		obs_property_list_add_string(list, *strings, *strings);
		strings++;
	}
}

#define TEXT_SPEED              obs_module_text("TargetUsage")
#define TEXT_TARGET_BITRATE     obs_module_text("Bitrate")
#define TEXT_MAX_BITRATE        obs_module_text("MaxBitrate")
#define TEXT_PROFILE            obs_module_text("Profile")
#define TEXT_ASYNC_DEPTH        obs_module_text("AsyncDepth")
#define TEXT_RATE_CONTROL       obs_module_text("RateControl")
#define TEXT_ACCURACY           obs_module_text("Accuracy")
#define TEXT_CONVERGENCE        obs_module_text("Convergence")
#define TEXT_ICQ_QUALITY        obs_module_text("ICQQuality")
#define TEXT_LA_DEPTH           obs_module_text("LookAheadDepth")
#define TEXT_KEYINT_SEC         obs_module_text("KeyframeIntervalSec")

static bool rate_control_modified(obs_properties_t *ppts, obs_property_t *p,
	obs_data_t *settings)
{
	const char *rate_control = obs_data_get_string(settings, "rate_control");

	bool bVisible =
		astrcmpi(rate_control, "VCM") == 0 ||
		astrcmpi(rate_control, "VBR") == 0;
	p = obs_properties_get(ppts, "max_bitrate");
	obs_property_set_visible(p, bVisible);

	bVisible =
		astrcmpi(rate_control, "CQP") == 0 ||
		astrcmpi(rate_control, "LA_ICQ") == 0 ||
		astrcmpi(rate_control, "ICQ") == 0;
	p = obs_properties_get(ppts, "bitrate");
	obs_property_set_visible(p, !bVisible);

	bVisible = astrcmpi(rate_control, "AVBR") == 0;
	p = obs_properties_get(ppts, "accuracy");
	obs_property_set_visible(p, bVisible);
	p = obs_properties_get(ppts, "convergence");
	obs_property_set_visible(p, bVisible);

	bVisible = astrcmpi(rate_control, "CQP") == 0;
	p = obs_properties_get(ppts, "qpi");
	obs_property_set_visible(p, bVisible);
	p = obs_properties_get(ppts, "qpb");
	obs_property_set_visible(p, bVisible);
	p = obs_properties_get(ppts, "qpp");
	obs_property_set_visible(p, bVisible);

	bVisible = astrcmpi(rate_control, "ICQ") == 0 ||
		astrcmpi(rate_control, "LA_ICQ") == 0;
	p = obs_properties_get(ppts, "icq_quality");
	obs_property_set_visible(p, bVisible);

	bVisible = astrcmpi(rate_control, "LA_ICQ") == 0 ||
		astrcmpi(rate_control, "LA") == 0;
	p = obs_properties_get(ppts, "la_depth");
	obs_property_set_visible(p, bVisible);

	return true;
}

static inline void add_rate_controls(obs_property_t *list,
		const struct qsv_rate_control_info *rc)
{
	enum qsv_cpu_platform plat = qsv_get_cpu_platform();
	while (rc->name) {
		if (!rc->haswell_or_greater || plat >= QSV_CPU_PLATFORM_HSW)
			obs_property_list_add_string(list, rc->name, rc->name);
		rc++;
	}
}

static obs_properties_t *obs_qsv_props(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *props = obs_properties_create();
	obs_property_t *list;

	list = obs_properties_add_list(props, "target_usage", TEXT_SPEED,
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	add_strings(list, qsv_usage_names);

	list = obs_properties_add_list(props, "profile", TEXT_PROFILE,
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	add_strings(list, qsv_profile_names);

	obs_properties_add_int(props, "keyint_sec", TEXT_KEYINT_SEC, 1, 20, 1);
	obs_properties_add_int(props, "async_depth", TEXT_ASYNC_DEPTH, 1, 7, 1);

	list = obs_properties_add_list(props, "rate_control", TEXT_RATE_CONTROL,
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	add_rate_controls(list, qsv_ratecontrols);
	obs_property_set_modified_callback(list, rate_control_modified);

	obs_properties_add_int(props, "bitrate", TEXT_TARGET_BITRATE, 50,
			10000000, 1);
	obs_properties_add_int(props, "max_bitrate", TEXT_MAX_BITRATE, 50,
			10000000, 1);
	obs_properties_add_int(props, "accuracy", TEXT_ACCURACY, 0, 10000, 1);
	obs_properties_add_int(props, "convergence", TEXT_CONVERGENCE, 0, 10, 1);
	obs_properties_add_int(props, "qpi", "QPI", 1, 51, 1);
	obs_properties_add_int(props, "qpp", "QPP", 1, 51, 1);
	obs_properties_add_int(props, "qpb", "QPB", 1, 51, 1);
	obs_properties_add_int(props, "icq_quality", TEXT_ICQ_QUALITY, 1, 51, 1);
	obs_properties_add_int(props, "la_depth", TEXT_LA_DEPTH, 10, 100, 1);

	return props;
}

static void update_params(struct obs_qsv *obsqsv, obs_data_t *settings)
{
	video_t *video = obs_encoder_video(obsqsv->encoder);
	const struct video_output_info *voi = video_output_get_info(video);

	const char *target_usage = obs_data_get_string(settings, "target_usage");
	const char *profile = obs_data_get_string(settings, "profile");
	const char *rate_control = obs_data_get_string(settings, "rate_control");
	int async_depth = (int)obs_data_get_int(settings, "async_depth");
	int target_bitrate = (int)obs_data_get_int(settings, "bitrate");
	int max_bitrate = (int)obs_data_get_int(settings, "max_bitrate");
	int accuracy = (int)obs_data_get_int(settings, "accuracy");
	int convergence = (int)obs_data_get_int(settings, "convergence");
	int qpi = (int)obs_data_get_int(settings, "qpi");
	int qpp = (int)obs_data_get_int(settings, "qpp");
	int qpb = (int)obs_data_get_int(settings, "qpb");
	int icq_quality = (int)obs_data_get_int(settings, "icq_quality");
	int la_depth = (int)obs_data_get_int(settings, "la_depth");
	int keyint_sec = (int)obs_data_get_int(settings, "keyint_sec");
	bool cbr_override = obs_data_get_bool(settings, "cbr");
	int bFrames = 7;

	int width = (int)obs_encoder_get_width(obsqsv->encoder);
	int height = (int)obs_encoder_get_height(obsqsv->encoder);
	if (astrcmpi(target_usage, "quality") == 0)
		obsqsv->params.nTargetUsage = MFX_TARGETUSAGE_BEST_QUALITY;
	else if (astrcmpi(target_usage, "balanced") == 0)
		obsqsv->params.nTargetUsage = MFX_TARGETUSAGE_BALANCED;
	else if (astrcmpi(target_usage, "speed") == 0)
		obsqsv->params.nTargetUsage = MFX_TARGETUSAGE_BEST_SPEED;

	if (astrcmpi(profile, "baseline") == 0)
		obsqsv->params.nCodecProfile = MFX_PROFILE_AVC_BASELINE;
	else if (astrcmpi(profile, "main") == 0)
		obsqsv->params.nCodecProfile = MFX_PROFILE_AVC_MAIN;
	else if (astrcmpi(profile, "high") == 0)
		obsqsv->params.nCodecProfile = MFX_PROFILE_AVC_HIGH;

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
	else if (astrcmpi(rate_control, "VCM") == 0)
		obsqsv->params.nRateControl = MFX_RATECONTROL_VCM;
	else if (astrcmpi(rate_control, "CQP") == 0)
		obsqsv->params.nRateControl = MFX_RATECONTROL_CQP;
	else if (astrcmpi(rate_control, "AVBR") == 0)
		obsqsv->params.nRateControl = MFX_RATECONTROL_AVBR;
	else if (astrcmpi(rate_control, "ICQ") == 0)
		obsqsv->params.nRateControl = MFX_RATECONTROL_ICQ;
	else if (astrcmpi(rate_control, "LA_ICQ") == 0)
		obsqsv->params.nRateControl = MFX_RATECONTROL_LA_ICQ;
	else if (astrcmpi(rate_control, "LA") == 0)
		obsqsv->params.nRateControl = MFX_RATECONTROL_LA;

	obsqsv->params.nAsyncDepth = (mfxU16)async_depth;
	obsqsv->params.nAccuracy = (mfxU16)accuracy;
	obsqsv->params.nConvergence = (mfxU16)convergence;
	obsqsv->params.nQPI = (mfxU16)qpi;
	obsqsv->params.nQPP = (mfxU16)qpp;
	obsqsv->params.nQPB = (mfxU16)qpb;
	obsqsv->params.nLADEPTH = (mfxU16)la_depth;
	obsqsv->params.nTargetBitRate = (mfxU16)target_bitrate;
	obsqsv->params.nMaxBitRate = (mfxU16)max_bitrate;
	obsqsv->params.nWidth = (mfxU16)width;
	obsqsv->params.nHeight = (mfxU16)height;
	obsqsv->params.nFpsNum = (mfxU16)voi->fps_num;
	obsqsv->params.nFpsDen = (mfxU16)voi->fps_den;
	obsqsv->params.nbFrames = (mfxU16)bFrames;
	obsqsv->params.nKeyIntSec = (mfxU16)keyint_sec;
	obsqsv->params.nICQQuality = (mfxU16)icq_quality;

	info("settings:\n\trate_control:   %s", rate_control);

	if (obsqsv->params.nRateControl != MFX_RATECONTROL_LA_ICQ &&
	    obsqsv->params.nRateControl != MFX_RATECONTROL_ICQ    &&
	    obsqsv->params.nRateControl != MFX_RATECONTROL_CQP)
		blog(LOG_INFO,
			"\ttarget_bitrate: %d",
			(int)obsqsv->params.nTargetBitRate);

	if (obsqsv->params.nRateControl == MFX_RATECONTROL_VBR ||
	    obsqsv->params.nRateControl == MFX_RATECONTROL_VCM)
		blog(LOG_INFO,
			"\tmax_bitrate:    %d",
			(int)obsqsv->params.nMaxBitRate);

	if (obsqsv->params.nRateControl == MFX_RATECONTROL_LA_ICQ ||
	    obsqsv->params.nRateControl == MFX_RATECONTROL_ICQ)
		blog(LOG_INFO,
			"\tICQ Quality:    %d",
			(int)obsqsv->params.nICQQuality);

	if (obsqsv->params.nRateControl == MFX_RATECONTROL_LA_ICQ ||
	    obsqsv->params.nRateControl == MFX_RATECONTROL_LA)
		blog(LOG_INFO,
			"\tLookahead Depth:%d",
			(int)obsqsv->params.nLADEPTH);

	if (obsqsv->params.nRateControl == MFX_RATECONTROL_CQP)
		blog(LOG_INFO,
			"\tqpi:            %d\n"
			"\tqpb:            %d\n"
			"\tqpp:            %d",
			qpi, qpb, qpp);

	blog(LOG_INFO,
		"\tfps_num:        %d\n"
		"\tfps_den:        %d\n"
		"\twidth:          %d\n"
		"\theight:         %d",
		voi->fps_num, voi->fps_den,
		width, height);

	info("debug info:");
}

static bool update_settings(struct obs_qsv *obsqsv, obs_data_t *settings)
{
	update_params(obsqsv, settings);
	return true;
}

static void load_headers(struct obs_qsv *obsqsv)
{
	DARRAY(uint8_t) header;
	uint8_t sei = 0;

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
	da_push_back_array(header, pPPS, nPPS);

	obsqsv->extra_data = header.array;
	obsqsv->extra_data_size = header.num;
	obsqsv->sei = &sei;
	obsqsv->sei_size = 1;
}

static bool obs_qsv_update(void *data, obs_data_t *settings)
{
	struct obs_qsv *obsqsv = data;
	bool success = update_settings(obsqsv, settings);
	int ret;


	if (success) {
		EnterCriticalSection(&g_QsvCs);

		ret = qsv_encoder_reconfig(obsqsv->context, &obsqsv->params);
		if (ret != 0)
			warn("Failed to reconfigure: %d", ret);

		LeaveCriticalSection(&g_QsvCs);

		return ret == 0;
	}

	return false;
}

static void *obs_qsv_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	InitializeCriticalSection(&g_QsvCs);

	struct obs_qsv *obsqsv = bzalloc(sizeof(struct obs_qsv));
	obsqsv->encoder = encoder;

	if (update_settings(obsqsv, settings)) {
		obsqsv->context = qsv_encoder_open(&obsqsv->params);

		if (obsqsv->context == NULL)
			warn("qsv failed to load");
		else
			load_headers(obsqsv);
	} else {
		warn("bad settings specified");
	}

	qsv_encoder_version(&g_verMajor, &g_verMinor);

	blog(LOG_INFO, "\tmajor:          %d\n"
	               "\tminor:          %d",
	               g_verMajor, g_verMinor);

	// MSDK 1.6 or less doesn't have automatic DTS calculation
	// including early SandyBridge.
	// Need to add manual DTS from PTS.
	if (g_verMajor == 1 && g_verMinor < 7) {
		int64_t interval = obsqsv->params.nbFrames + 1;
		int64_t GopPicSize = (int64_t)(obsqsv->params.nKeyIntSec *
				obsqsv->params.nFpsNum /
				(float)obsqsv->params.nFpsDen);
		g_pts2dtsShift = GopPicSize - (GopPicSize / interval) *
			interval;

		blog(LOG_INFO, "\tinterval:       %d\n"
		               "\tGopPictSize:    %d\n"
		               "\tg_pts2dtsShift: %d",
		               interval, GopPicSize, g_pts2dtsShift);
	}
	else
		g_pts2dtsShift = -1;

	if (!obsqsv->context) {
		bfree(obsqsv);
		return NULL;
	}

	obsqsv->performance_token =
		os_request_high_performance("qsv encoding");

	g_bFirst = true;

	return obsqsv;
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

static bool obs_qsv_sei(void *data, uint8_t **sei,size_t *size)
{
	struct obs_qsv *obsqsv = data;

	if (!obsqsv->context)
		return false;

	/* (Jim) Unused */
	UNUSED_PARAMETER(sei);
	UNUSED_PARAMETER(size);

	*sei = obsqsv->sei;
	*size = obsqsv->sei_size;
	return true;
}

static inline bool valid_format(enum video_format format)
{
	return format == VIDEO_FORMAT_NV12;
}

static inline void cap_resolution(obs_encoder_t *encoder,
		struct video_scale_info *info)
{
	enum qsv_cpu_platform qsv_platform = qsv_get_cpu_platform();
	uint32_t width = obs_encoder_get_width(encoder);
	uint32_t height = obs_encoder_get_height(encoder);

	info->height = height;
	info->width = width;

	if (qsv_platform <= QSV_CPU_PLATFORM_IVB) {
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
		pref_format = valid_format(info->format) ?
			info->format : VIDEO_FORMAT_NV12;
	}

	info->format = pref_format;
	cap_resolution(obsqsv->encoder, info);
}

static void parse_packet(struct obs_qsv *obsqsv, struct encoder_packet *packet, mfxBitstream *pBS, uint32_t fps_num, bool *received_packet)
{
	uint8_t *start, *end;
	int type;

	if (pBS == NULL || pBS->DataLength == 0) {
		*received_packet = false;
		return;
	}

	da_resize(obsqsv->packet_data, 0);
	da_push_back_array(obsqsv->packet_data, &pBS->Data[pBS->DataOffset],
			pBS->DataLength);

	packet->data = obsqsv->packet_data.array;
	packet->size = obsqsv->packet_data.num;
	packet->type = OBS_ENCODER_VIDEO;
	packet->pts = pBS->TimeStamp * fps_num / 90000;
	packet->keyframe = (pBS->FrameType &
			(MFX_FRAMETYPE_I | MFX_FRAMETYPE_REF));

	/* ------------------------------------ */

	start = obsqsv->packet_data.array;
	end = start + obsqsv->packet_data.num;

	start = (uint8_t*)obs_avc_find_startcode(start, end);
	while (true) {
		while (start < end && !*(start++));

		if (start == end)
			break;

		type = start[0] & 0x1F;
		if (type == OBS_NAL_SLICE_IDR || type == OBS_NAL_SLICE) {
			uint8_t prev_type = (start[0] >> 5) & 0x3;
			start[0] &= ~(3 << 5);

			if (pBS->FrameType & MFX_FRAMETYPE_I)
				start[0] |= OBS_NAL_PRIORITY_HIGHEST << 5;
			else if (pBS->FrameType & MFX_FRAMETYPE_P)
				start[0] |= OBS_NAL_PRIORITY_HIGH << 5;
			else
				start[0] |= prev_type << 5;
		}

		start = (uint8_t*)obs_avc_find_startcode(start, end);
	}

	/* ------------------------------------ */

	//bool iFrame = pBS->FrameType & MFX_FRAMETYPE_I;
	//bool bFrame = pBS->FrameType & MFX_FRAMETYPE_B;
	bool pFrame = pBS->FrameType & MFX_FRAMETYPE_P;
	//int iType = iFrame ? 0 : (bFrame ? 1 : (pFrame ? 2 : -1));
	//int64_t interval = obsqsv->params.nbFrames + 1;

	// In case MSDK doesn't support automatic DecodeTimeStamp, do manual
	// calculation
	if (g_pts2dtsShift >= 0)
	{
		if (g_bFirst) {
			packet->dts = packet->pts - 3 * obsqsv->params.nFpsDen;
		} else if (pFrame) {
			packet->dts = packet->pts - 10 * obsqsv->params.nFpsDen;
			g_prevDts = packet->dts;
		} else {
			packet->dts = g_prevDts + obsqsv->params.nFpsDen;
			g_prevDts = packet->dts;
		}
	} else {
		packet->dts = pBS->DecodeTimeStamp * fps_num / 90000;
	}

#if 0
	info("parse packet:\n"
		"\tFrameType: %d\n"
		"\tpts:       %d\n"
		"\tdts:       %d",
		iType, packet->pts, packet->dts);
#endif

	*received_packet = true;
	pBS->DataLength = 0;

	g_bFirst = false;
}

static bool obs_qsv_encode(void *data, struct encoder_frame *frame,
		struct encoder_packet *packet, bool *received_packet)
{
	struct obs_qsv *obsqsv = data;

	if (!frame || !packet || !received_packet)
		return false;

	EnterCriticalSection(&g_QsvCs);


	video_t *video = obs_encoder_video(obsqsv->encoder);
	const struct video_output_info *voi = video_output_get_info(video);

	mfxBitstream *pBS = NULL;

	int ret;

	mfxU64 qsvPTS = frame->pts * 90000 / voi->fps_num;

	// FIXME: remove null check from the top of this function
	// if we actually do expect null frames to complete output.
	if (frame)
		ret = qsv_encoder_encode(
			obsqsv->context,
			qsvPTS,
			frame->data[0], frame->data[1], frame->linesize[0],
			frame->linesize[1],
			&pBS);
	else
		ret = qsv_encoder_encode(
			obsqsv->context,
			qsvPTS,
			NULL, NULL, 0, 0, &pBS);

	if (ret < 0) {
		warn("encode failed");
		LeaveCriticalSection(&g_QsvCs);
		return false;
	}

	parse_packet(obsqsv, packet, pBS, voi->fps_num, received_packet);

	LeaveCriticalSection(&g_QsvCs);

	return true;
}

struct obs_encoder_info obs_qsv_encoder = {
	.id = "obs_qsv11",
	.type = OBS_ENCODER_VIDEO,
	.codec = "h264",
	.get_name = obs_qsv_getname,
	.create = obs_qsv_create,
	.destroy = obs_qsv_destroy,
	.encode = obs_qsv_encode,
	.update = obs_qsv_update,
	.get_properties = obs_qsv_props,
	.get_defaults = obs_qsv_defaults,
	.get_extra_data = obs_qsv_extra_data,
	.get_sei_data = obs_qsv_sei,
	.get_video_info = obs_qsv_video_info
};
