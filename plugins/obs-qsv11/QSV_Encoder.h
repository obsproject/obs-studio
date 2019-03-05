/******************************************************************************
Copyright (C) 2013-2014 Intel Corporation
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program.  If not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
******************************************************************************/

#pragma once

#include <Windows.h>
#include "mfxstructures.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct qsv_rate_control_info {
	const char *name;
	bool haswell_or_greater;
};

static const struct qsv_rate_control_info qsv_ratecontrols[] = {
	{"CBR", false},
	{"VBR", false},
	{"VCM", true},
	{"CQP", false},
	{"AVBR", false},
	{"ICQ", true},
	{"LA_CBR", true},
	{"LA_VBR", true},
	{"LA_ICQ", true},
	{0, false}
};
static const char * const qsv_profile_names[] = {
	"high",
	"main",
	"baseline",
	0
};
static const char * const qsv_usage_names[] = {
	"quality",
	"balanced",
	"speed",
	0
};

typedef struct qsv_t qsv_t;

typedef struct
{
	mfxU16 nTargetUsage; /* 1 through 7, 1 being best quality and 7
				being the best speed */
	mfxU16 nWidth;       /* source picture width */
	mfxU16 nHeight;      /* source picture height */
	mfxU16 nAsyncDepth;
	mfxU16 nFpsNum;
	mfxU16 nFpsDen;
	mfxU16 nTargetBitRate;
	mfxU16 nMaxBitRate;
	mfxU16 nCodecProfile;
	mfxU16 nRateControl;
	mfxU16 nAccuracy;
	mfxU16 nConvergence;
	mfxU16 nQPI;
	mfxU16 nQPP;
	mfxU16 nQPB;
	mfxU16 nLADEPTH;
	mfxU16 nKeyIntSec;
	mfxU16 nbFrames;
	mfxU16 nICQQuality;
	bool   bMBBRC;  
} qsv_param_t;

enum qsv_cpu_platform {
	QSV_CPU_PLATFORM_UNKNOWN,
	QSV_CPU_PLATFORM_BNL,
	QSV_CPU_PLATFORM_SNB,
	QSV_CPU_PLATFORM_IVB,
	QSV_CPU_PLATFORM_SLM,
	QSV_CPU_PLATFORM_CHT,
	QSV_CPU_PLATFORM_HSW,
	QSV_CPU_PLATFORM_BDW,
	QSV_CPU_PLATFORM_SKL,  
	QSV_CPU_PLATFORM_INTEL
};

int qsv_encoder_close(qsv_t *);
int qsv_param_parse(qsv_param_t *, const char *name, const char *value);
int qsv_param_apply_profile(qsv_param_t *, const char *profile);
int qsv_param_default_preset(qsv_param_t *, const char *preset,
		const char *tune);
int qsv_encoder_reconfig(qsv_t *, qsv_param_t *);
void qsv_encoder_version(unsigned short *major, unsigned short *minor);
qsv_t *qsv_encoder_open( qsv_param_t * );
int qsv_encoder_encode(qsv_t *, uint64_t, uint8_t *, uint8_t *, uint32_t,
		uint32_t, mfxBitstream **pBS);
int qsv_encoder_headers(qsv_t *, uint8_t **pSPS, uint8_t **pPPS,
		uint16_t *pnSPS, uint16_t *pnPPS);
enum qsv_cpu_platform qsv_get_cpu_platform();

#ifdef __cplusplus
}
#endif
