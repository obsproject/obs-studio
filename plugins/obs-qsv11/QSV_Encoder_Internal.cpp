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

#include "QSV_Encoder_Internal.h"
#include "QSV_Encoder.h"
#include <vpl/mfxstructures.h>
#include <vpl/mfxvideo++.h>
#include <vpl/mfxdispatcher.h>
#include <obs-module.h>

#define do_log(level, format, ...) blog(level, "[qsv encoder: '%s'] " format, "msdk_impl", ##__VA_ARGS__)

#define warn(format, ...) do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...) do_log(LOG_INFO, format, ##__VA_ARGS__)
#define debug(format, ...) do_log(LOG_DEBUG, format, ##__VA_ARGS__)

mfxHDL QSV_Encoder_Internal::g_GFX_Handle = NULL;
mfxU16 QSV_Encoder_Internal::g_numEncodersOpen = 0;

QSV_Encoder_Internal::QSV_Encoder_Internal(mfxVersion &version, bool useTexAlloc)
	: m_pmfxSurfaces(NULL),
	  m_pmfxENC(NULL),
	  m_nSPSBufferSize(1024),
	  m_nPPSBufferSize(1024),
	  m_nTaskPool(0),
	  m_pTaskPool(NULL),
	  m_nTaskIdx(0),
	  m_nFirstSyncTask(0),
	  m_outBitstream(),
	  m_bUseD3D11(false),
	  m_bUseTexAlloc(useTexAlloc),
	  m_sessionData(NULL),
	  m_ver(version)
{
	mfxVariant tempImpl;
	mfxStatus sts;

	mfxLoader loader = MFXLoad();
	mfxConfig cfg = MFXCreateConfig(loader);

	tempImpl.Type = MFX_VARIANT_TYPE_U32;
	tempImpl.Data.U32 = MFX_IMPL_TYPE_HARDWARE;
	MFXSetConfigFilterProperty(cfg, (const mfxU8 *)"mfxImplDescription.Impl", tempImpl);

	tempImpl.Type = MFX_VARIANT_TYPE_U32;
	tempImpl.Data.U32 = INTEL_VENDOR_ID;
	MFXSetConfigFilterProperty(cfg, (const mfxU8 *)"mfxImplDescription.VendorID", tempImpl);
#if defined(_WIN32)
	m_bUseD3D11 = true;
	m_bUseTexAlloc = true;

	tempImpl.Type = MFX_VARIANT_TYPE_U32;
	tempImpl.Data.U32 = MFX_ACCEL_MODE_VIA_D3D11;
	MFXSetConfigFilterProperty(cfg, (const mfxU8 *)"mfxImplDescription.AccelerationMode", tempImpl);
#else
	tempImpl.Type = MFX_VARIANT_TYPE_U32;
	tempImpl.Data.U32 = MFX_ACCEL_MODE_VIA_VAAPI;
	MFXSetConfigFilterProperty(cfg, (const mfxU8 *)"mfxImplDescription.AccelerationMode", tempImpl);
#endif
	sts = MFXCreateSession(loader, 0, &m_session);
	if (sts == MFX_ERR_NONE) {
		MFXQueryVersion(m_session, &version);
		MFXClose(m_session);
		MFXUnload(loader);

		blog(LOG_INFO, "\tsurf:           %s", m_bUseTexAlloc ? "Texture" : "SysMem");

		m_ver = version;
		return;
	}
}

QSV_Encoder_Internal::~QSV_Encoder_Internal()
{
	if (m_pmfxENC)
		ClearData();
}

mfxStatus QSV_Encoder_Internal::Open(qsv_param_t *pParams, enum qsv_codec codec)
{
	mfxStatus sts = MFX_ERR_NONE;

	if (m_bUseD3D11 | m_bUseTexAlloc)
		// Use texture surface
		sts = Initialize(m_ver, &m_session, &m_mfxAllocator, &g_GFX_Handle, false, codec, &m_sessionData);
	else
		sts = Initialize(m_ver, &m_session, NULL, NULL, false, codec, &m_sessionData);

	MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

	m_pmfxENC = new MFXVideoENCODE(m_session);

	InitParams(pParams, codec);
	sts = m_pmfxENC->Query(&m_mfxEncParams, &m_mfxEncParams);
	MSDK_IGNORE_MFX_STS(sts, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
	MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

	sts = AllocateSurfaces();
	MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

	sts = m_pmfxENC->Init(&m_mfxEncParams);
	MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

	sts = GetVideoParam(codec);
	MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

	sts = InitBitstream();
	MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

	if (sts >= MFX_ERR_NONE) {
		g_numEncodersOpen++;
	}
	return sts;
}

PRAGMA_WARN_PUSH
PRAGMA_WARN_DEPRECATION
static inline bool HasOptimizedBRCSupport(const mfxPlatform &platform, const mfxVersion &version, mfxU16 rateControl)
{
#if (MFX_VERSION_MAJOR >= 2 && MFX_VERSION_MINOR >= 13) || MFX_VERSION_MAJOR > 2
	if ((version.Major >= 2 && version.Minor >= 13) || version.Major > 2)
		if (rateControl == MFX_RATECONTROL_CBR &&
		    (platform.CodeName >= MFX_PLATFORM_BATTLEMAGE && platform.CodeName != MFX_PLATFORM_ALDERLAKE_N))
			return true;
#endif
	UNUSED_PARAMETER(platform);
	UNUSED_PARAMETER(version);
	UNUSED_PARAMETER(rateControl);
	return false;
}

static inline bool HasAV1ScreenContentSupport(const mfxPlatform &platform, const mfxVersion &version)
{
#if (MFX_VERSION_MAJOR >= 2 && MFX_VERSION_MINOR >= 12) || MFX_VERSION_MAJOR > 2
	// Platform enums needed are introduced in VPL version 2.12
	if ((version.Major >= 2 && version.Minor >= 12) || version.Major > 2)
		if (platform.CodeName >= MFX_PLATFORM_LUNARLAKE && platform.CodeName != MFX_PLATFORM_ALDERLAKE_N &&
		    platform.CodeName != MFX_PLATFORM_ARROWLAKE)
			return true;
#endif
	UNUSED_PARAMETER(platform);
	UNUSED_PARAMETER(version);
	return false;
}
PRAGMA_WARN_POP

mfxStatus QSV_Encoder_Internal::InitParams(qsv_param_t *pParams, enum qsv_codec codec)
{
	memset(&m_mfxEncParams, 0, sizeof(m_mfxEncParams));

	if (codec == QSV_CODEC_AVC)
		m_mfxEncParams.mfx.CodecId = MFX_CODEC_AVC;
	else if (codec == QSV_CODEC_AV1)
		m_mfxEncParams.mfx.CodecId = MFX_CODEC_AV1;
	else if (codec == QSV_CODEC_HEVC)
		m_mfxEncParams.mfx.CodecId = MFX_CODEC_HEVC;

	if (codec == QSV_CODEC_HEVC) {
		m_mfxEncParams.mfx.NumSlice = 0;
		m_mfxEncParams.mfx.IdrInterval = 1;
	} else {
		m_mfxEncParams.mfx.NumSlice = 1;
	}
	m_mfxEncParams.mfx.TargetUsage = pParams->nTargetUsage;
	m_mfxEncParams.mfx.CodecProfile = pParams->nCodecProfile;
	m_mfxEncParams.mfx.FrameInfo.FrameRateExtN = pParams->nFpsNum;
	m_mfxEncParams.mfx.FrameInfo.FrameRateExtD = pParams->nFpsDen;
	if (pParams->video_fmt_10bit) {
		m_mfxEncParams.mfx.FrameInfo.FourCC = MFX_FOURCC_P010;
		m_mfxEncParams.mfx.FrameInfo.BitDepthChroma = 10;
		m_mfxEncParams.mfx.FrameInfo.BitDepthLuma = 10;
		m_mfxEncParams.mfx.FrameInfo.Shift = 1;
	} else {
		m_mfxEncParams.mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;
	}
	m_mfxEncParams.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
	m_mfxEncParams.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
	m_mfxEncParams.mfx.FrameInfo.CropX = 0;
	m_mfxEncParams.mfx.FrameInfo.CropY = 0;
	m_mfxEncParams.mfx.FrameInfo.CropW = pParams->nWidth;
	m_mfxEncParams.mfx.FrameInfo.CropH = pParams->nHeight;
	m_mfxEncParams.mfx.GopRefDist = pParams->nbFrames + 1;

	mfxPlatform platform;
	MFXVideoCORE_QueryPlatform(m_session, &platform);

	PRAGMA_WARN_PUSH
	PRAGMA_WARN_DEPRECATION
	if (codec == QSV_CODEC_AVC || codec == QSV_CODEC_HEVC) {
		if (platform.CodeName >= MFX_PLATFORM_DG2)
			m_mfxEncParams.mfx.LowPower = MFX_CODINGOPTION_ON;
	} else if (codec == QSV_CODEC_AV1) {
		m_mfxEncParams.mfx.LowPower = MFX_CODINGOPTION_ON;
	}
	PRAGMA_WARN_POP

	m_mfxEncParams.mfx.RateControlMethod = pParams->nRateControl;

	switch (pParams->nRateControl) {
	case MFX_RATECONTROL_CBR:
		m_mfxEncParams.mfx.TargetKbps = pParams->nTargetBitRate;

		if (HasOptimizedBRCSupport(platform, m_ver, pParams->nRateControl)) {
			m_mfxEncParams.mfx.BufferSizeInKB = (pParams->nTargetBitRate / 8) * 1;
		} else {
			m_mfxEncParams.mfx.BufferSizeInKB = (pParams->nTargetBitRate / 8) * 2;
		}

		m_mfxEncParams.mfx.InitialDelayInKB = m_mfxEncParams.mfx.BufferSizeInKB / 2;
		break;
	case MFX_RATECONTROL_VBR:
		m_mfxEncParams.mfx.TargetKbps = pParams->nTargetBitRate;
		m_mfxEncParams.mfx.MaxKbps = pParams->nMaxBitRate;
		m_mfxEncParams.mfx.BufferSizeInKB = (pParams->nTargetBitRate / 8) * 2;
		m_mfxEncParams.mfx.InitialDelayInKB = (pParams->nTargetBitRate / 8) * 1;
		break;
	case MFX_RATECONTROL_CQP:
		m_mfxEncParams.mfx.QPI = pParams->nQPI;
		m_mfxEncParams.mfx.QPB = pParams->nQPB;
		m_mfxEncParams.mfx.QPP = pParams->nQPP;
		break;
	case MFX_RATECONTROL_ICQ:
		m_mfxEncParams.mfx.ICQQuality = pParams->nICQQuality;
		break;
	default:
		break;
	}

	m_mfxEncParams.AsyncDepth = pParams->nAsyncDepth;
	m_mfxEncParams.mfx.GopPicSize =
		(pParams->nKeyIntSec) ? (mfxU16)(pParams->nKeyIntSec * pParams->nFpsNum / (float)pParams->nFpsDen)
				      : 240;

	memset(&m_co2, 0, sizeof(mfxExtCodingOption2));
	m_co2.Header.BufferId = MFX_EXTBUFF_CODING_OPTION2;
	m_co2.Header.BufferSz = sizeof(m_co2);
	if (pParams->bRepeatHeaders)
		m_co2.RepeatPPS = MFX_CODINGOPTION_ON;
	else
		m_co2.RepeatPPS = MFX_CODINGOPTION_OFF;

	if (pParams->nbFrames > 1)
		m_co2.BRefType = MFX_B_REF_PYRAMID;

	PRAGMA_WARN_PUSH
	PRAGMA_WARN_DEPRECATION
	// LA VME/ENC case for older platforms
	if (pParams->nLADEPTH && codec == QSV_CODEC_AVC && m_mfxEncParams.mfx.LowPower != MFX_CODINGOPTION_ON &&
	    platform.CodeName >= MFX_PLATFORM_ICELAKE) {
		if (pParams->nRateControl == MFX_RATECONTROL_CBR) {
			pParams->nRateControl = MFX_RATECONTROL_LA_HRD;
		} else if (pParams->nRateControl == MFX_RATECONTROL_VBR) {
			pParams->nRateControl = MFX_RATECONTROL_LA;
		} else if (pParams->nRateControl == MFX_RATECONTROL_ICQ) {
			pParams->nRateControl = MFX_RATECONTROL_LA_ICQ;
		}
		m_co2.LookAheadDepth = pParams->nLADEPTH;
	}
	PRAGMA_WARN_POP

	// LA VDENC case for newer platform, works only under CBR / VBR
	if (pParams->nRateControl == MFX_RATECONTROL_CBR || pParams->nRateControl == MFX_RATECONTROL_VBR) {
		if (pParams->nLADEPTH && m_mfxEncParams.mfx.LowPower == MFX_CODINGOPTION_ON) {
			m_co2.LookAheadDepth = pParams->nLADEPTH;
		}
	}

	extendedBuffers.push_back((mfxExtBuffer *)&m_co2);

	if (HasOptimizedBRCSupport(platform, m_ver, pParams->nRateControl)) {
		memset(&m_co3, 0, sizeof(mfxExtCodingOption3));
		m_co3.Header.BufferId = MFX_EXTBUFF_CODING_OPTION3;
		m_co3.Header.BufferSz = sizeof(m_co3);
		m_co3.WinBRCSize = pParams->nFpsNum / pParams->nFpsDen;

		if (codec == QSV_CODEC_AVC || codec == QSV_CODEC_HEVC) {
			m_co3.WinBRCMaxAvgKbps = mfxU16(1.3 * pParams->nTargetBitRate);
		} else if (codec == QSV_CODEC_AV1) {
			m_co3.WinBRCMaxAvgKbps = mfxU16(1.2 * pParams->nTargetBitRate);
		}
		extendedBuffers.push_back((mfxExtBuffer *)&m_co3);
	}

	if (codec == QSV_CODEC_HEVC) {
		if ((pParams->nWidth & 15) || (pParams->nHeight & 15)) {
			memset(&m_ExtHEVCParam, 0, sizeof(m_ExtHEVCParam));
			m_ExtHEVCParam.Header.BufferId = MFX_EXTBUFF_HEVC_PARAM;
			m_ExtHEVCParam.Header.BufferSz = sizeof(m_ExtHEVCParam);
			m_ExtHEVCParam.PicWidthInLumaSamples = pParams->nWidth;
			m_ExtHEVCParam.PicHeightInLumaSamples = pParams->nHeight;
			extendedBuffers.push_back((mfxExtBuffer *)&m_ExtHEVCParam);
		}
	}

	constexpr uint32_t pixelcount_4k = 3840 * 2160;
	/* If size is 4K+, set tile columns per frame to 2. */
	if (codec == QSV_CODEC_AV1 && (pParams->nWidth * pParams->nHeight) >= pixelcount_4k) {
		memset(&m_ExtAv1TileParam, 0, sizeof(m_ExtAv1TileParam));
		m_ExtAv1TileParam.Header.BufferId = MFX_EXTBUFF_AV1_TILE_PARAM;
		m_ExtAv1TileParam.Header.BufferSz = sizeof(m_ExtAv1TileParam);
		m_ExtAv1TileParam.NumTileColumns = 2;
		extendedBuffers.push_back((mfxExtBuffer *)&m_ExtAv1TileParam);
	}

	// AV1_SCREEN_CONTENT_TOOLS API is introduced in VPL version 2.11
#if (MFX_VERSION_MAJOR >= 2 && MFX_VERSION_MINOR >= 11) || MFX_VERSION_MAJOR > 2
	if (codec == QSV_CODEC_AV1 && HasAV1ScreenContentSupport(platform, m_ver)) {
		memset(&m_ExtAV1ScreenContentTools, 0, sizeof(m_ExtAV1ScreenContentTools));
		m_ExtAV1ScreenContentTools.Header.BufferId = MFX_EXTBUFF_AV1_SCREEN_CONTENT_TOOLS;
		m_ExtAV1ScreenContentTools.Header.BufferSz = sizeof(m_ExtAV1ScreenContentTools);
		m_ExtAV1ScreenContentTools.Palette = MFX_CODINGOPTION_ON;
		extendedBuffers.push_back((mfxExtBuffer *)&m_ExtAV1ScreenContentTools);
	}
#endif

#if defined(_WIN32)
	// TODO: Ask about this one on VAAPI too.
	memset(&m_ExtVideoSignalInfo, 0, sizeof(m_ExtVideoSignalInfo));
	m_ExtVideoSignalInfo.Header.BufferId = MFX_EXTBUFF_VIDEO_SIGNAL_INFO;
	m_ExtVideoSignalInfo.Header.BufferSz = sizeof(m_ExtVideoSignalInfo);
	m_ExtVideoSignalInfo.VideoFormat = pParams->VideoFormat;
	m_ExtVideoSignalInfo.VideoFullRange = pParams->VideoFullRange;
	m_ExtVideoSignalInfo.ColourDescriptionPresent = 1;
	m_ExtVideoSignalInfo.ColourPrimaries = pParams->ColourPrimaries;
	m_ExtVideoSignalInfo.TransferCharacteristics = pParams->TransferCharacteristics;
	m_ExtVideoSignalInfo.MatrixCoefficients = pParams->MatrixCoefficients;
	extendedBuffers.push_back((mfxExtBuffer *)&m_ExtVideoSignalInfo);
#endif

	// CLL and Chroma location in HEVC only supported by VPL
	if (m_ver.Major >= 2) {
		// Chroma location is HEVC only
		if (codec == QSV_CODEC_HEVC) {
			memset(&m_ExtChromaLocInfo, 0, sizeof(m_ExtChromaLocInfo));
			m_ExtChromaLocInfo.Header.BufferId = MFX_EXTBUFF_CHROMA_LOC_INFO;
			m_ExtChromaLocInfo.Header.BufferSz = sizeof(m_ExtChromaLocInfo);
			m_ExtChromaLocInfo.ChromaLocInfoPresentFlag = 1;
			m_ExtChromaLocInfo.ChromaSampleLocTypeTopField = pParams->ChromaSampleLocTypeTopField;
			m_ExtChromaLocInfo.ChromaSampleLocTypeBottomField = pParams->ChromaSampleLocTypeBottomField;
			extendedBuffers.push_back((mfxExtBuffer *)&m_ExtChromaLocInfo);
		}
	}

	// AV1 HDR meta data is now supported by VPL.
	if (pParams->MaxContentLightLevel > 0) {
		memset(&m_ExtMasteringDisplayColourVolume, 0, sizeof(m_ExtMasteringDisplayColourVolume));
		m_ExtMasteringDisplayColourVolume.Header.BufferId = MFX_EXTBUFF_MASTERING_DISPLAY_COLOUR_VOLUME;
		m_ExtMasteringDisplayColourVolume.Header.BufferSz = sizeof(m_ExtMasteringDisplayColourVolume);
		m_ExtMasteringDisplayColourVolume.InsertPayloadToggle = MFX_PAYLOAD_IDR;
		m_ExtMasteringDisplayColourVolume.DisplayPrimariesX[0] = pParams->DisplayPrimariesX[0];
		m_ExtMasteringDisplayColourVolume.DisplayPrimariesX[1] = pParams->DisplayPrimariesX[1];
		m_ExtMasteringDisplayColourVolume.DisplayPrimariesX[2] = pParams->DisplayPrimariesX[2];
		m_ExtMasteringDisplayColourVolume.DisplayPrimariesY[0] = pParams->DisplayPrimariesY[0];
		m_ExtMasteringDisplayColourVolume.DisplayPrimariesY[1] = pParams->DisplayPrimariesY[1];
		m_ExtMasteringDisplayColourVolume.DisplayPrimariesY[2] = pParams->DisplayPrimariesY[2];
		m_ExtMasteringDisplayColourVolume.WhitePointX = pParams->WhitePointX;
		m_ExtMasteringDisplayColourVolume.WhitePointY = pParams->WhitePointY;
		m_ExtMasteringDisplayColourVolume.MaxDisplayMasteringLuminance = pParams->MaxDisplayMasteringLuminance;
		m_ExtMasteringDisplayColourVolume.MinDisplayMasteringLuminance = pParams->MinDisplayMasteringLuminance;
		extendedBuffers.push_back((mfxExtBuffer *)&m_ExtMasteringDisplayColourVolume);

		memset(&m_ExtContentLightLevelInfo, 0, sizeof(m_ExtContentLightLevelInfo));
		m_ExtContentLightLevelInfo.Header.BufferId = MFX_EXTBUFF_CONTENT_LIGHT_LEVEL_INFO;
		m_ExtContentLightLevelInfo.Header.BufferSz = sizeof(m_ExtContentLightLevelInfo);
		m_ExtContentLightLevelInfo.InsertPayloadToggle = MFX_PAYLOAD_IDR;
		m_ExtContentLightLevelInfo.MaxContentLightLevel = pParams->MaxContentLightLevel;
		m_ExtContentLightLevelInfo.MaxPicAverageLightLevel = pParams->MaxPicAverageLightLevel;
		extendedBuffers.push_back((mfxExtBuffer *)&m_ExtContentLightLevelInfo);
	}

	// Width must be a multiple of 16
	// Height must be a multiple of 16 in case of frame picture and a
	// multiple of 32 in case of field picture
	m_mfxEncParams.mfx.FrameInfo.Width = MSDK_ALIGN16(pParams->nWidth);
	m_mfxEncParams.mfx.FrameInfo.Height = MSDK_ALIGN16(pParams->nHeight);

	if (m_bUseTexAlloc)
		m_mfxEncParams.IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY;
	else
		m_mfxEncParams.IOPattern = MFX_IOPATTERN_IN_SYSTEM_MEMORY;

	m_mfxEncParams.ExtParam = extendedBuffers.data();
	m_mfxEncParams.NumExtParam = (mfxU16)extendedBuffers.size();

	// We don't check what was valid or invalid here, just try changing LowPower.
	// Ensure set values are not overwritten so in case it wasn't lowPower we fail
	// during the parameter check.
	mfxVideoParam validParams = {0};
	memcpy(&validParams, &m_mfxEncParams, sizeof(validParams));
	mfxStatus sts = m_pmfxENC->Query(&m_mfxEncParams, &validParams);
	if (sts == MFX_ERR_UNSUPPORTED || sts == MFX_ERR_UNDEFINED_BEHAVIOR) {
		if (m_mfxEncParams.mfx.LowPower == MFX_CODINGOPTION_ON) {
			m_mfxEncParams.mfx.LowPower = MFX_CODINGOPTION_OFF;
			m_co2.LookAheadDepth = 0;
		}
	}

	memset(&m_ctrl, 0, sizeof(m_ctrl));
	memset(&m_roi, 0, sizeof(m_roi));

	return sts;
}

bool QSV_Encoder_Internal::UpdateParams(qsv_param_t *pParams)
{
	switch (pParams->nRateControl) {
	case MFX_RATECONTROL_CBR:
		m_mfxEncParams.mfx.TargetKbps = pParams->nTargetBitRate;
	default:
		break;
	}

	return true;
}

mfxStatus QSV_Encoder_Internal::ReconfigureEncoder()
{
	return m_pmfxENC->Reset(&m_mfxEncParams);
}

mfxStatus QSV_Encoder_Internal::AllocateSurfaces()
{
	// Query number of required surfaces for encoder
	mfxFrameAllocRequest EncRequest;
	memset(&EncRequest, 0, sizeof(EncRequest));
	mfxStatus sts = m_pmfxENC->QueryIOSurf(&m_mfxEncParams, &EncRequest);
	MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

	EncRequest.Type |= WILL_WRITE;

	// SNB hack. On some SNB, it seems to require more surfaces
	EncRequest.NumFrameSuggested += m_mfxEncParams.AsyncDepth;

	// Allocate required surfaces
	if (m_bUseTexAlloc) {
		sts = m_mfxAllocator.Alloc(m_mfxAllocator.pthis, &EncRequest, &m_mfxResponse);
		MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

		m_nSurfNum = m_mfxResponse.NumFrameActual;

		m_pmfxSurfaces = new mfxFrameSurface1 *[m_nSurfNum];
		MSDK_CHECK_POINTER(m_pmfxSurfaces, MFX_ERR_MEMORY_ALLOC);

		for (int i = 0; i < m_nSurfNum; i++) {
			m_pmfxSurfaces[i] = new mfxFrameSurface1;
			memset(m_pmfxSurfaces[i], 0, sizeof(mfxFrameSurface1));
			memcpy(&(m_pmfxSurfaces[i]->Info), &(m_mfxEncParams.mfx.FrameInfo), sizeof(mfxFrameInfo));
			m_pmfxSurfaces[i]->Data.MemId = m_mfxResponse.mids[i];
		}
	} else {
		mfxU16 width = (mfxU16)MSDK_ALIGN32(EncRequest.Info.Width);
		mfxU16 height = (mfxU16)MSDK_ALIGN32(EncRequest.Info.Height);
		mfxU8 bitsPerPixel = 12;
		mfxU32 surfaceSize = width * height * bitsPerPixel / 8;
		m_nSurfNum = EncRequest.NumFrameSuggested;

		m_pmfxSurfaces = new mfxFrameSurface1 *[m_nSurfNum];
		for (int i = 0; i < m_nSurfNum; i++) {
			m_pmfxSurfaces[i] = new mfxFrameSurface1;
			memset(m_pmfxSurfaces[i], 0, sizeof(mfxFrameSurface1));
			memcpy(&(m_pmfxSurfaces[i]->Info), &(m_mfxEncParams.mfx.FrameInfo), sizeof(mfxFrameInfo));

			mfxU8 *pSurface = (mfxU8 *)new mfxU8[surfaceSize];
			m_pmfxSurfaces[i]->Data.Y = pSurface;
			m_pmfxSurfaces[i]->Data.U = pSurface + width * height;
			m_pmfxSurfaces[i]->Data.V = pSurface + width * height + 1;
			m_pmfxSurfaces[i]->Data.Pitch = width;
		}
	}

	blog(LOG_INFO, "\tm_nSurfNum:     %d", m_nSurfNum);

	return sts;
}

mfxStatus QSV_Encoder_Internal::GetVideoParam(enum qsv_codec codec)
{
	memset(&m_parameter, 0, sizeof(m_parameter));
	mfxExtCodingOptionSPSPPS opt;
	memset(&m_parameter, 0, sizeof(m_parameter));
	opt.Header.BufferId = MFX_EXTBUFF_CODING_OPTION_SPSPPS;
	opt.Header.BufferSz = sizeof(mfxExtCodingOptionSPSPPS);

	std::vector<mfxExtBuffer *> extendedBuffers;
	extendedBuffers.reserve(2);

	opt.SPSBuffer = m_SPSBuffer;
	opt.PPSBuffer = m_PPSBuffer;
	opt.SPSBufSize = 1024; //  m_nSPSBufferSize;
	opt.PPSBufSize = 1024; //  m_nPPSBufferSize;

	mfxExtCodingOptionVPS opt_vps{};
	if (codec == QSV_CODEC_HEVC) {
		opt_vps.Header.BufferId = MFX_EXTBUFF_CODING_OPTION_VPS;
		opt_vps.Header.BufferSz = sizeof(mfxExtCodingOptionVPS);
		opt_vps.VPSBuffer = m_VPSBuffer;
		opt_vps.VPSBufSize = 1024;

		extendedBuffers.push_back((mfxExtBuffer *)&opt_vps);
	}

	extendedBuffers.push_back((mfxExtBuffer *)&opt);

	m_parameter.ExtParam = extendedBuffers.data();
	m_parameter.NumExtParam = (mfxU16)extendedBuffers.size();

	mfxStatus sts = m_pmfxENC->GetVideoParam(&m_parameter);
	MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

	if (codec == QSV_CODEC_HEVC)
		m_nVPSBufferSize = opt_vps.VPSBufSize;
	m_nSPSBufferSize = opt.SPSBufSize;
	m_nPPSBufferSize = opt.PPSBufSize;

	return sts;
}

void QSV_Encoder_Internal::GetSPSPPS(mfxU8 **pSPSBuf, mfxU8 **pPPSBuf, mfxU16 *pnSPSBuf, mfxU16 *pnPPSBuf)
{
	*pSPSBuf = m_SPSBuffer;
	*pPPSBuf = m_PPSBuffer;
	*pnSPSBuf = m_nSPSBufferSize;
	*pnPPSBuf = m_nPPSBufferSize;
}

void QSV_Encoder_Internal::GetVpsSpsPps(mfxU8 **pVPSBuf, mfxU8 **pSPSBuf, mfxU8 **pPPSBuf, mfxU16 *pnVPSBuf,
					mfxU16 *pnSPSBuf, mfxU16 *pnPPSBuf)
{
	*pVPSBuf = m_VPSBuffer;
	*pnVPSBuf = m_nVPSBufferSize;

	*pSPSBuf = m_SPSBuffer;
	*pnSPSBuf = m_nSPSBufferSize;

	*pPPSBuf = m_PPSBuffer;
	*pnPPSBuf = m_nPPSBufferSize;
}

mfxStatus QSV_Encoder_Internal::InitBitstream()
{
	m_nTaskPool = m_parameter.AsyncDepth;
	m_nFirstSyncTask = 0;

	m_pTaskPool = new Task[m_nTaskPool];
	memset(m_pTaskPool, 0, sizeof(Task) * m_nTaskPool);

	for (int i = 0; i < m_nTaskPool; i++) {
		m_pTaskPool[i].mfxBS.MaxLength = m_parameter.mfx.BufferSizeInKB * 1000;
		m_pTaskPool[i].mfxBS.Data = new mfxU8[m_pTaskPool[i].mfxBS.MaxLength];
		m_pTaskPool[i].mfxBS.DataOffset = 0;
		m_pTaskPool[i].mfxBS.DataLength = 0;

		MSDK_CHECK_POINTER(m_pTaskPool[i].mfxBS.Data, MFX_ERR_MEMORY_ALLOC);
	}

	memset(&m_outBitstream, 0, sizeof(mfxBitstream));
	m_outBitstream.MaxLength = m_parameter.mfx.BufferSizeInKB * 1000;
	m_outBitstream.Data = new mfxU8[m_outBitstream.MaxLength];
	m_outBitstream.DataOffset = 0;
	m_outBitstream.DataLength = 0;

	blog(LOG_INFO, "\tm_nTaskPool:    %d", m_nTaskPool);

	return MFX_ERR_NONE;
}

mfxStatus QSV_Encoder_Internal::LoadP010(mfxFrameSurface1 *pSurface, uint8_t *pDataY, uint8_t *pDataUV,
					 uint32_t strideY, uint32_t strideUV)
{
	mfxU16 w, h, i, pitch;
	mfxU8 *ptr;
	mfxFrameInfo *pInfo = &pSurface->Info;
	mfxFrameData *pData = &pSurface->Data;

	if (pInfo->CropH > 0 && pInfo->CropW > 0) {
		w = pInfo->CropW;
		h = pInfo->CropH;
	} else {
		w = pInfo->Width;
		h = pInfo->Height;
	}

	pitch = pData->Pitch;
	ptr = pData->Y + pInfo->CropX + pInfo->CropY * pData->Pitch;
	const size_t line_size = w * 2;

	// load Y plane
	for (i = 0; i < h; i++)
		memcpy(ptr + i * pitch, pDataY + i * strideY, line_size);

	// load UV plane
	h /= 2;
	ptr = pData->UV + pInfo->CropX + (pInfo->CropY / 2) * pitch;

	for (i = 0; i < h; i++)
		memcpy(ptr + i * pitch, pDataUV + i * strideUV, line_size);

	return MFX_ERR_NONE;
}

mfxStatus QSV_Encoder_Internal::LoadNV12(mfxFrameSurface1 *pSurface, uint8_t *pDataY, uint8_t *pDataUV,
					 uint32_t strideY, uint32_t strideUV)
{
	mfxU16 w, h, i, pitch;
	mfxU8 *ptr;
	mfxFrameInfo *pInfo = &pSurface->Info;
	mfxFrameData *pData = &pSurface->Data;

	if (pInfo->CropH > 0 && pInfo->CropW > 0) {
		w = pInfo->CropW;
		h = pInfo->CropH;
	} else {
		w = pInfo->Width;
		h = pInfo->Height;
	}

	pitch = pData->Pitch;
	ptr = pData->Y + pInfo->CropX + pInfo->CropY * pData->Pitch;

	// load Y plane
	for (i = 0; i < h; i++)
		memcpy(ptr + i * pitch, pDataY + i * strideY, w);

	// load UV plane
	h /= 2;
	ptr = pData->UV + pInfo->CropX + (pInfo->CropY / 2) * pitch;

	for (i = 0; i < h; i++)
		memcpy(ptr + i * pitch, pDataUV + i * strideUV, w);

	return MFX_ERR_NONE;
}

int QSV_Encoder_Internal::GetFreeTaskIndex(Task *pTaskPool, mfxU16 nPoolSize)
{
	if (pTaskPool)
		for (int i = 0; i < nPoolSize; i++)
			if (!pTaskPool[i].syncp)
				return i;
	return MFX_ERR_NOT_FOUND;
}

mfxStatus QSV_Encoder_Internal::Encode(uint64_t ts, uint8_t *pDataY, uint8_t *pDataUV, uint32_t strideY,
				       uint32_t strideUV, mfxBitstream **pBS)
{
	mfxStatus sts = MFX_ERR_NONE;
	*pBS = NULL;
	int nTaskIdx = GetFreeTaskIndex(m_pTaskPool, m_nTaskPool);

#if 0
	info("MSDK Encode:\n"
		"\tTaskIndex: %d",
		nTaskIdx);
#endif

	int nSurfIdx = GetFreeSurfaceIndex(m_pmfxSurfaces, m_nSurfNum);
#if 0
	info("MSDK Encode:\n"
		"\tnSurfIdx: %d",
		nSurfIdx);
#endif

	while (MFX_ERR_NOT_FOUND == nTaskIdx || MFX_ERR_NOT_FOUND == nSurfIdx) {
		// No more free tasks or surfaces, need to sync
		sts = MFXVideoCORE_SyncOperation(m_session, m_pTaskPool[m_nFirstSyncTask].syncp, 60000);
		MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

		mfxU8 *pTemp = m_outBitstream.Data;
		memcpy(&m_outBitstream, &m_pTaskPool[m_nFirstSyncTask].mfxBS, sizeof(mfxBitstream));

		m_pTaskPool[m_nFirstSyncTask].mfxBS.Data = pTemp;
		m_pTaskPool[m_nFirstSyncTask].mfxBS.DataLength = 0;
		m_pTaskPool[m_nFirstSyncTask].mfxBS.DataOffset = 0;
		m_pTaskPool[m_nFirstSyncTask].syncp = NULL;
		nTaskIdx = m_nFirstSyncTask;
		m_nFirstSyncTask = (m_nFirstSyncTask + 1) % m_nTaskPool;
		*pBS = &m_outBitstream;

#if 0
		info("MSDK Encode:\n"
			"\tnew FirstSyncTask: %d\n"
			"\tTaskIndex:         %d",
			m_nFirstSyncTask,
			nTaskIdx);
#endif

		nSurfIdx = GetFreeSurfaceIndex(m_pmfxSurfaces, m_nSurfNum);
#if 0
		info("MSDK Encode:\n"
			"\tnSurfIdx: %d",
			nSurfIdx);
#endif
	}

	mfxFrameSurface1 *pSurface = m_pmfxSurfaces[nSurfIdx];
	if (m_bUseTexAlloc) {
		sts = m_mfxAllocator.Lock(m_mfxAllocator.pthis, pSurface->Data.MemId, &(pSurface->Data));
		MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
	}

	sts = (pSurface->Info.FourCC == MFX_FOURCC_P010) ? LoadP010(pSurface, pDataY, pDataUV, strideY, strideUV)
							 : LoadNV12(pSurface, pDataY, pDataUV, strideY, strideUV);

	MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
	pSurface->Data.TimeStamp = ts;

	if (m_bUseTexAlloc) {
		sts = m_mfxAllocator.Unlock(m_mfxAllocator.pthis, pSurface->Data.MemId, &(pSurface->Data));
		MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
	}

	for (;;) {
		// Encode a frame asynchronously (returns immediately)
		sts = m_pmfxENC->EncodeFrameAsync(&m_ctrl, pSurface, &m_pTaskPool[nTaskIdx].mfxBS,
						  &m_pTaskPool[nTaskIdx].syncp);

		if (MFX_ERR_NONE < sts && !m_pTaskPool[nTaskIdx].syncp) {
			// Repeat the call if warning and no output
			if (MFX_WRN_DEVICE_BUSY == sts)
				MSDK_SLEEP(1); // Wait if device is busy, then repeat the same call
		} else if (MFX_ERR_NONE < sts && m_pTaskPool[nTaskIdx].syncp) {
			sts = MFX_ERR_NONE; // Ignore warnings if output is available
			break;
		} else if (MFX_ERR_NOT_ENOUGH_BUFFER == sts) {
			// Allocate more bitstream buffer memory here if needed...
			break;
		} else
			break;
	}

	return sts;
}

mfxStatus QSV_Encoder_Internal::Encode_tex(uint64_t ts, void *tex, uint64_t lock_key, uint64_t *next_key,
					   mfxBitstream **pBS)
{
	mfxStatus sts = MFX_ERR_NONE;
	*pBS = NULL;
	int nTaskIdx = GetFreeTaskIndex(m_pTaskPool, m_nTaskPool);
	int nSurfIdx = GetFreeSurfaceIndex(m_pmfxSurfaces, m_nSurfNum);

	while (MFX_ERR_NOT_FOUND == nTaskIdx || MFX_ERR_NOT_FOUND == nSurfIdx) {
		// No more free tasks or surfaces, need to sync
		sts = MFXVideoCORE_SyncOperation(m_session, m_pTaskPool[m_nFirstSyncTask].syncp, 60000);
		MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

		mfxU8 *pTemp = m_outBitstream.Data;
		memcpy(&m_outBitstream, &m_pTaskPool[m_nFirstSyncTask].mfxBS, sizeof(mfxBitstream));

		m_pTaskPool[m_nFirstSyncTask].mfxBS.Data = pTemp;
		m_pTaskPool[m_nFirstSyncTask].mfxBS.DataLength = 0;
		m_pTaskPool[m_nFirstSyncTask].mfxBS.DataOffset = 0;
		m_pTaskPool[m_nFirstSyncTask].syncp = NULL;
		nTaskIdx = m_nFirstSyncTask;
		m_nFirstSyncTask = (m_nFirstSyncTask + 1) % m_nTaskPool;
		*pBS = &m_outBitstream;

		nSurfIdx = GetFreeSurfaceIndex(m_pmfxSurfaces, m_nSurfNum);
	}

	mfxFrameSurface1 *pSurface = m_pmfxSurfaces[nSurfIdx];
	//copy to default surface directly
	pSurface->Data.TimeStamp = ts;
	if (m_bUseTexAlloc) {
		// mfxU64 isn't consistent with stdint, requiring a cast to be multi-platform.
		sts = simple_copytex(m_mfxAllocator.pthis, pSurface->Data.MemId, tex, lock_key, (mfxU64 *)next_key);
		MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
	}

	for (;;) {
		// Encode a frame asynchronously (returns immediately)
		sts = m_pmfxENC->EncodeFrameAsync(&m_ctrl, pSurface, &m_pTaskPool[nTaskIdx].mfxBS,
						  &m_pTaskPool[nTaskIdx].syncp);

		if (MFX_ERR_NONE < sts && !m_pTaskPool[nTaskIdx].syncp) {
			// Repeat the call if warning and no output
			if (MFX_WRN_DEVICE_BUSY == sts)
				MSDK_SLEEP(1); // Wait if device is busy, then repeat the same call
		} else if (MFX_ERR_NONE < sts && m_pTaskPool[nTaskIdx].syncp) {
			sts = MFX_ERR_NONE; // Ignore warnings if output is available
			break;
		} else if (MFX_ERR_NOT_ENOUGH_BUFFER == sts) {
			// Allocate more bitstream buffer memory here if needed...
			break;
		} else
			break;
	}

	return sts;
}

mfxStatus QSV_Encoder_Internal::Drain()
{
	mfxStatus sts = MFX_ERR_NONE;

	while (m_pTaskPool && m_pTaskPool[m_nFirstSyncTask].syncp) {
		sts = MFXVideoCORE_SyncOperation(m_session, m_pTaskPool[m_nFirstSyncTask].syncp, 60000);
		MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

		m_pTaskPool[m_nFirstSyncTask].syncp = NULL;
		m_nFirstSyncTask = (m_nFirstSyncTask + 1) % m_nTaskPool;
	}

	return sts;
}

mfxStatus QSV_Encoder_Internal::ClearData()
{
	mfxStatus sts = MFX_ERR_NONE;
	sts = Drain();

	if (m_pmfxENC) {
		sts = m_pmfxENC->Close();
		delete m_pmfxENC;
		m_pmfxENC = NULL;
	}

	if (m_bUseTexAlloc)
		m_mfxAllocator.Free(m_mfxAllocator.pthis, &m_mfxResponse);

	if (m_pmfxSurfaces) {
		for (int i = 0; i < m_nSurfNum; i++) {
			if (!m_bUseTexAlloc)
				delete m_pmfxSurfaces[i]->Data.Y;

			delete m_pmfxSurfaces[i];
		}
		MSDK_SAFE_DELETE_ARRAY(m_pmfxSurfaces);
	}

	if (m_pTaskPool) {
		for (int i = 0; i < m_nTaskPool; i++)
			delete m_pTaskPool[i].mfxBS.Data;
		MSDK_SAFE_DELETE_ARRAY(m_pTaskPool);
	}

	if (m_outBitstream.Data) {
		delete[] m_outBitstream.Data;
		m_outBitstream.Data = NULL;
	}

	if (sts >= MFX_ERR_NONE) {
		g_numEncodersOpen--;
	}

	if ((m_bUseTexAlloc) && (g_numEncodersOpen <= 0)) {
		Release();
		g_GFX_Handle = NULL;
	}
	MFXVideoENCODE_Close(m_session);
	ReleaseSessionData(m_sessionData);
	m_sessionData = NULL;
	return sts;
}

mfxStatus QSV_Encoder_Internal::Reset(qsv_param_t *pParams, enum qsv_codec codec)
{
	mfxStatus sts = ClearData();
	MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

	sts = Open(pParams, codec);
	MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

	return sts;
}

void QSV_Encoder_Internal::AddROI(mfxU32 left, mfxU32 top, mfxU32 right, mfxU32 bottom, mfxI16 delta)
{
	if (m_roi.NumROI == 256) {
		warn("Maximum number of ROIs hit, ignoring additional ROI!");
		return;
	}

	m_roi.Header.BufferId = MFX_EXTBUFF_ENCODER_ROI;
	m_roi.Header.BufferSz = sizeof(mfxExtEncoderROI);
	m_roi.ROIMode = MFX_ROI_MODE_QP_DELTA;
	/* The SDK will automatically align the values to block sizes so we
	 * don't have to do any maths here. */
	m_roi.ROI[m_roi.NumROI].Left = left;
	m_roi.ROI[m_roi.NumROI].Top = top;
	m_roi.ROI[m_roi.NumROI].Right = right;
	m_roi.ROI[m_roi.NumROI].Bottom = bottom;
	m_roi.ROI[m_roi.NumROI].DeltaQP = delta;
	m_roi.NumROI++;

	/* Right now ROI is the only thing we add so this is fine */
	if (m_extbuf.empty())
		m_extbuf.push_back((mfxExtBuffer *)&m_roi);

	m_ctrl.ExtParam = m_extbuf.data();
	m_ctrl.NumExtParam = (mfxU16)m_extbuf.size();
}

void QSV_Encoder_Internal::ClearROI()
{
	m_roi.NumROI = 0;
	m_ctrl.ExtParam = nullptr;
	m_ctrl.NumExtParam = 0;
	m_extbuf.clear();
}
