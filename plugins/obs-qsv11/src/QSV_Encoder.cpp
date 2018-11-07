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

// QSV_Encoder.cpp : Defines the exported functions for the DLL application.
//

#include "QSV_Encoder.h"
#include "QSV_Encoder_Internal.h"
#include <obs-module.h>
#include <string>
#include <atomic>
#include <intrin.h>

#define do_log(level, format, ...) \
	blog(level, "[qsv encoder: '%s'] " format, \
			"msdk_impl", ##__VA_ARGS__)

mfxIMPL              impl = MFX_IMPL_HARDWARE_ANY;
mfxVersion           ver = {{0, 1}}; // for backward compatibility
std::atomic<bool>    is_active{false};

void qsv_encoder_version(unsigned short *major, unsigned short *minor)
{
	*major = ver.Major;
	*minor = ver.Minor;
}

qsv_t *qsv_encoder_open(qsv_param_t *pParams)
{
	QSV_Encoder_Internal *pEncoder = new QSV_Encoder_Internal(impl, ver);
	mfxStatus sts = pEncoder->Open(pParams);
	if (sts != MFX_ERR_NONE) {
		delete pEncoder;
		if (pEncoder)
			is_active.store(false);
		return NULL;
	}

	return (qsv_t *) pEncoder;
}

int qsv_encoder_headers(qsv_t *pContext, uint8_t **pSPS, uint8_t **pPPS,
		uint16_t *pnSPS, uint16_t *pnPPS)
{
	QSV_Encoder_Internal *pEncoder = (QSV_Encoder_Internal *)pContext;
	pEncoder->GetSPSPPS(pSPS, pPPS, pnSPS, pnPPS);

	return 0;
}

int qsv_encoder_encode(qsv_t * pContext, uint64_t ts, uint8_t *pDataY,
		uint8_t *pDataUV, uint32_t strideY, uint32_t strideUV,
		mfxBitstream **pBS)
{
	QSV_Encoder_Internal *pEncoder = (QSV_Encoder_Internal *)pContext;
	mfxStatus sts = MFX_ERR_NONE;

	if (pDataY != NULL && pDataUV != NULL)
		sts = pEncoder->Encode(ts, pDataY, pDataUV, strideY, strideUV,
				pBS);

	if (sts == MFX_ERR_NONE)
		return 0;
	else if (sts == MFX_ERR_MORE_DATA)
		return 1;
	else
		return -1;
}

int qsv_encoder_close(qsv_t *pContext)
{
	QSV_Encoder_Internal *pEncoder = (QSV_Encoder_Internal *)pContext;
	delete pEncoder;

	if (pEncoder)
		is_active.store(false);

	return 0;
}

/*
int qsv_param_default_preset(qsv_param_t *pParams, const char *preset,
		const char *tune)
{
	return 0;
}

int qsv_param_parse(qsv_param_t *, const char *name, const char *value)
{
	return 0;
}

int qsv_param_apply_profile(qsv_param_t *, const char *profile)
{
	return 0;
}
*/

int qsv_encoder_reconfig(qsv_t *pContext, qsv_param_t *pParams)
{
	QSV_Encoder_Internal *pEncoder = (QSV_Encoder_Internal *)pContext;
	mfxStatus sts = pEncoder->Reset(pParams);

	if (sts == MFX_ERR_NONE)
		return 0;
	else
		return -1;
}

enum qsv_cpu_platform qsv_get_cpu_platform()
{
	using std::string;

	int cpuInfo[4];
	__cpuid(cpuInfo, 0);

	string vendor;
	vendor += string((char*)&cpuInfo[1], 4);
	vendor += string((char*)&cpuInfo[3], 4);
	vendor += string((char*)&cpuInfo[2], 4);

	if (vendor != "GenuineIntel")
		return QSV_CPU_PLATFORM_UNKNOWN;

	__cpuid(cpuInfo, 1);
	BYTE model = ((cpuInfo[0] >> 4) & 0xF) + ((cpuInfo[0] >> 12) & 0xF0);
	BYTE family = ((cpuInfo[0] >> 8) & 0xF) + ((cpuInfo[0] >> 20) & 0xFF);

	// See Intel 64 and IA-32 Architectures Software Developer's Manual,
	// Vol 3C Table 35-1
	if (family != 6)
		return QSV_CPU_PLATFORM_UNKNOWN;

	switch (model)
	{
	case 0x1C:
	case 0x26:
	case 0x27:
	case 0x35:
	case 0x36:
		return QSV_CPU_PLATFORM_BNL;

	case 0x2a:
	case 0x2d:
		return QSV_CPU_PLATFORM_SNB;

	case 0x3a:
	case 0x3e:
		return QSV_CPU_PLATFORM_IVB;

	case 0x37:
	case 0x4A:
	case 0x4D:
	case 0x5A:
	case 0x5D:
		return QSV_CPU_PLATFORM_SLM;

	case 0x4C:
		return QSV_CPU_PLATFORM_CHT;

	case 0x3c:
	case 0x3f:
	case 0x45:
	case 0x46:
		return QSV_CPU_PLATFORM_HSW;

	case 0x3d:
	case 0x47:
	case 0x4f:
	case 0x56:
		return QSV_CPU_PLATFORM_BDW;

	case 0x4e:
	case 0x5e:
		return QSV_CPU_PLATFORM_SKL;
	}
	//assume newer revisions are at least as capable as Skylake
	return QSV_CPU_PLATFORM_INTEL;
}
