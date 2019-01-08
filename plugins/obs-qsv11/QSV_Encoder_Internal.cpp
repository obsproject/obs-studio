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
#include "mfxastructures.h"
#include "mfxvideo++.h"
#include <VersionHelpers.h>
#include <obs-module.h>

#define do_log(level, format, ...) \
	blog(level, "[qsv encoder: '%s'] " format, \
			"msdk_impl", ##__VA_ARGS__)

#define warn(format, ...)  do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...)  do_log(LOG_INFO,    format, ##__VA_ARGS__)
#define debug(format, ...) do_log(LOG_DEBUG,   format, ##__VA_ARGS__)

#define SYNC_OPERATION_WAIT_TIME	60000

mfxHDL QSV_Encoder_Internal::g_DX_Handle = NULL;
mfxU16 QSV_Encoder_Internal::g_numEncodersOpen = 0;

QSV_Encoder_Internal::QSV_Encoder_Internal(mfxIMPL& impl, mfxVersion& version) :
    m_pmfxEncInSurfaces(NULL),
    m_pmfxVPPInSurfaces(NULL),
	m_pmfxENC(NULL),
	m_pmfxVPP(NULL),
	m_nSPSBufferSize(100),
	m_nPPSBufferSize(100),
	m_nTaskPool(0),
	m_pTaskPool(NULL),
	m_nTaskIdx(0),
	m_nFirstSyncTask(0),
	m_nmfxEncInSurfNum(0),
	m_nmfxVPPInSurfNum(0),
    m_outBitstream()
{
	mfxIMPL tempImpl;
	mfxStatus sts;

	m_bIsWindows8OrGreater = IsWindows8OrGreater();
	m_bUseD3D11 = false;
	m_bD3D9HACK = true;

	if (m_bIsWindows8OrGreater) {
		tempImpl = impl | MFX_IMPL_VIA_D3D11;
		sts = m_session.Init(tempImpl, &version);
		if (sts == MFX_ERR_NONE) {
			m_session.QueryVersion(&version);
			m_session.Close();

			// Use D3D11 surface
			// m_bUseD3D11 = ((version.Major > 1) ||
			//	(version.Major == 1 && version.Minor >= 8));
			m_bUseD3D11 = true;
			if (m_bUseD3D11)
				blog(LOG_INFO, "\timpl:           D3D11\n"
				               "\tsurf:           D3D11");
			else
				blog(LOG_INFO, "\timpl:           D3D11\n"
				               "\tsurf:           SysMem");

			m_impl = tempImpl;
			m_ver = version;
			return;
		}
	}
	else if (m_bD3D9HACK) {
		tempImpl = impl | MFX_IMPL_VIA_D3D9;
		sts = m_session.Init(tempImpl, &version);
		if (sts == MFX_ERR_NONE) {
			m_session.QueryVersion(&version);
			m_session.Close();

			blog(LOG_INFO, "\timpl:           D3D09\n"
				       "\tsurf:           Hack");

			m_impl = tempImpl;
			m_ver = version;
			return;
		}
	}

	// Either windows 7 or D3D11 failed at this point.
	tempImpl = impl | MFX_IMPL_VIA_D3D9;
	sts = m_session.Init(tempImpl, &version);
	if (sts == MFX_ERR_NONE) {
		m_session.QueryVersion(&version);
		m_session.Close();

		blog(LOG_INFO, "\timpl:           D3D09\n"
		               "\tsurf:           SysMem");

		m_impl = tempImpl;
		m_ver = version;
	}

}

QSV_Encoder_Internal::~QSV_Encoder_Internal()
{
	if (m_pmfxENC)
		ClearData();
}

mfxStatus QSV_Encoder_Internal::Open(qsv_param_t * pParams)
{
	mfxStatus sts = MFX_ERR_NONE;
    bool processingIsNeeded = false;

	if (m_bUseD3D11)
		// Use D3D11 surface
		sts = Initialize(m_impl, m_ver, &m_session, &m_mfxAllocator, &g_DX_Handle, false, false);
	else if (m_bD3D9HACK)
		// Use hack
		sts = Initialize(m_impl, m_ver, &m_session, &m_mfxAllocator, &g_DX_Handle, false, true);
	else
		sts = Initialize(m_impl, m_ver, &m_session, NULL);
	MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    InitParams(pParams, processingIsNeeded);

	m_pmfxENC = new MFXVideoENCODE(m_session);
    sts = m_pmfxENC->Query(&m_mfxEncParams, &m_mfxEncParams);
    MSDK_IGNORE_MFX_STS(sts, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    if (processingIsNeeded)
    {
        m_pmfxVPP = new MFXVideoVPP(m_session);
        sts = m_pmfxVPP->Query(&m_mfxVPPParams, &m_mfxVPPParams);
        MSDK_IGNORE_MFX_STS(sts, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
    }

	sts = AllocateSurfaces();
	MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

	sts = m_pmfxENC->Init(&m_mfxEncParams);
	MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    if (m_pmfxVPP)
    {
        sts = m_pmfxVPP->Init(&m_mfxVPPParams);
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
    }

	sts = GetVideoParam();
	MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

	sts = InitBitstream();
	MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

	if (sts >= MFX_ERR_NONE) {
		g_numEncodersOpen++;
	}
	return sts;
}


bool QSV_Encoder_Internal::InitParams(qsv_param_t * pParams, bool& processingIsNeeded)
{
	mfxU32 encoded_width, encoded_height, input_width, input_height;
	input_width = pParams->nWidth;
	input_height = pParams->nHeight;
	encoded_width = pParams->nConversionWidth ? pParams->nConversionWidth : input_width;
	encoded_height = pParams->nConversionHeight ? pParams->nConversionHeight : input_height;

    if ((input_height != encoded_height) || (input_width != encoded_width))
        processingIsNeeded = true;

	memset(&m_mfxEncParams, 0, sizeof(m_mfxEncParams));


	m_mfxEncParams.mfx.CodecId = MFX_CODEC_AVC;
	m_mfxEncParams.mfx.GopOptFlag = MFX_GOP_STRICT;
	m_mfxEncParams.mfx.NumSlice = 1;
	m_mfxEncParams.mfx.TargetUsage = pParams->nTargetUsage;
	m_mfxEncParams.mfx.CodecProfile = pParams->nCodecProfile;
	m_mfxEncParams.mfx.FrameInfo.FrameRateExtN = pParams->nFpsNum;
	m_mfxEncParams.mfx.FrameInfo.FrameRateExtD = pParams->nFpsDen;
	m_mfxEncParams.mfx.FrameInfo.FourCC = pParams->nFourCC;
	m_mfxEncParams.mfx.FrameInfo.ChromaFormat = pParams->nChromaFormat;
	m_mfxEncParams.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
	m_mfxEncParams.mfx.FrameInfo.CropX = 0;
	m_mfxEncParams.mfx.FrameInfo.CropY = 0;
	m_mfxEncParams.mfx.FrameInfo.CropW = encoded_width;
	m_mfxEncParams.mfx.FrameInfo.CropH = encoded_height;
	// Width and height must be a multiple of 16
	m_mfxEncParams.mfx.FrameInfo.Width  = MSDK_ALIGN16(encoded_width);
	m_mfxEncParams.mfx.FrameInfo.Height = MSDK_ALIGN16(encoded_height);
	m_mfxEncParams.mfx.GopRefDist = pParams->nbFrames + 1;

	m_mfxEncParams.mfx.RateControlMethod = pParams->nRateControl;

	switch (pParams->nRateControl) {
	case MFX_RATECONTROL_CBR:
		m_mfxEncParams.mfx.TargetKbps = pParams->nTargetBitRate;
		break;
	case MFX_RATECONTROL_VBR:
	case MFX_RATECONTROL_VCM:
		m_mfxEncParams.mfx.TargetKbps = pParams->nTargetBitRate;
		m_mfxEncParams.mfx.MaxKbps = pParams->nMaxBitRate;
		break;
	case MFX_RATECONTROL_CQP:
		m_mfxEncParams.mfx.QPI = pParams->nQPI;
		m_mfxEncParams.mfx.QPB = pParams->nQPB;
		m_mfxEncParams.mfx.QPP = pParams->nQPP;
		break;
	case MFX_RATECONTROL_AVBR:
		m_mfxEncParams.mfx.TargetKbps = pParams->nTargetBitRate;
		m_mfxEncParams.mfx.Accuracy = pParams->nAccuracy;
		m_mfxEncParams.mfx.Convergence = pParams->nConvergence;
		break;
	case MFX_RATECONTROL_ICQ:
		m_mfxEncParams.mfx.ICQQuality = pParams->nICQQuality;
		break;
	case MFX_RATECONTROL_LA:
		m_mfxEncParams.mfx.TargetKbps = pParams->nTargetBitRate;
		break;
	case MFX_RATECONTROL_LA_ICQ:
		m_mfxEncParams.mfx.ICQQuality = pParams->nICQQuality;
		break;
	case MFX_RATECONTROL_LA_HRD:
		m_mfxEncParams.mfx.TargetKbps = pParams->nTargetBitRate;
		m_mfxEncParams.mfx.MaxKbps = pParams->nTargetBitRate;
		break;
	default:
		break;
	}

	m_mfxEncParams.AsyncDepth = pParams->nAsyncDepth;
	m_mfxEncParams.mfx.GopPicSize = (mfxU16)(pParams->nKeyIntSec *
			pParams->nFpsNum / (float)pParams->nFpsDen);

	static mfxExtBuffer* extendedBuffers[2];
	int iBuffers = 0;

	memset(&m_co2, 0, sizeof(mfxExtCodingOption2));
	m_co2.Header.BufferId = MFX_EXTBUFF_CODING_OPTION2;
	m_co2.Header.BufferSz = sizeof(m_co2);
	if (pParams->bMBBRC)
		m_co2.MBBRC = MFX_CODINGOPTION_ON;
	if (pParams->nRateControl == MFX_RATECONTROL_LA_ICQ ||
	    pParams->nRateControl == MFX_RATECONTROL_LA)

		memset(&m_co2, 0, sizeof(mfxExtCodingOption2));
		m_co2.Header.BufferId = MFX_EXTBUFF_CODING_OPTION2;
		m_co2.Header.BufferSz = sizeof(m_co2);
		m_co2.LookAheadDepth = pParams->nLADEPTH;
	extendedBuffers[iBuffers++] = (mfxExtBuffer*)& m_co2;

	if (iBuffers > 0) {
		m_mfxEncParams.ExtParam = extendedBuffers;
		m_mfxEncParams.NumExtParam = (mfxU16)iBuffers;
	}

	m_mfxEncParams.IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY;

	//init VPP params
	memset(&m_mfxVPPParams, 0, sizeof(m_mfxVPPParams));

    if (processingIsNeeded)
    {
        m_mfxVPPParams.vpp.In.Width = MSDK_ALIGN16(input_width);
        m_mfxVPPParams.vpp.In.Height = MSDK_ALIGN16(input_height);
        m_mfxVPPParams.vpp.In.CropX = 0;
        m_mfxVPPParams.vpp.In.CropY = 0;
        m_mfxVPPParams.vpp.In.CropW = input_width;
        m_mfxVPPParams.vpp.In.CropH = input_height;

        m_mfxVPPParams.vpp.In.FrameRateExtD = m_mfxEncParams.mfx.FrameInfo.FrameRateExtD;
        m_mfxVPPParams.vpp.In.FrameRateExtN = m_mfxEncParams.mfx.FrameInfo.FrameRateExtN;
        m_mfxVPPParams.vpp.In.PicStruct = m_mfxEncParams.mfx.FrameInfo.PicStruct;
        m_mfxVPPParams.vpp.In.ChromaFormat = m_mfxEncParams.mfx.FrameInfo.ChromaFormat;
        m_mfxVPPParams.vpp.In.FourCC = m_mfxEncParams.mfx.FrameInfo.FourCC;

        memcpy(&m_mfxVPPParams.vpp.Out, &m_mfxEncParams.mfx.FrameInfo, sizeof(mfxFrameInfo));

        m_mfxVPPParams.AsyncDepth = 1; //sync mode
        m_mfxVPPParams.IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY | MFX_IOPATTERN_OUT_VIDEO_MEMORY;

        static mfxExtBuffer* vppExtendedBuffers[1];
        memset(&m_vppScaling, 0, sizeof(m_vppScaling));
        m_vppScaling.Header.BufferId = MFX_EXTBUFF_VPP_SCALING;
        m_vppScaling.Header.BufferSz = sizeof(m_vppScaling);
        vppExtendedBuffers[0] = (mfxExtBuffer*)&m_vppScaling;

        m_vppScaling.ScalingMode = MFX_SCALING_MODE_LOWPOWER;

        m_mfxVPPParams.ExtParam = vppExtendedBuffers;
        m_mfxVPPParams.NumExtParam = 1;
    }

    static mfxExtBuffer* vppExtendedBuffers[1];
    memset(&m_vppScaling, 0,sizeof(m_vppScaling));
    m_vppScaling.Header.BufferId = MFX_EXTBUFF_VPP_SCALING;
    m_vppScaling.Header.BufferSz = sizeof(m_vppScaling);
    vppExtendedBuffers[0] = (mfxExtBuffer*)&m_vppScaling;

    m_vppScaling.ScalingMode = MFX_SCALING_MODE_LOWPOWER;

    m_mfxVPPParams.ExtParam = vppExtendedBuffers;
    m_mfxVPPParams.NumExtParam = 1;

	return true;
}

mfxStatus QSV_Encoder_Internal::AllocateSurfaces()
{
	// Query number of required surfaces for encoder
	mfxFrameAllocRequest EncRequest;
	memset(&EncRequest, 0, sizeof(EncRequest));
	mfxStatus sts = m_pmfxENC->QueryIOSurf(&m_mfxEncParams, &EncRequest);
	MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

	// SNB hack. On some SNB, it seems to require more surfaces
	EncRequest.NumFrameSuggested += m_mfxEncParams.AsyncDepth;

    EncRequest.Type |= (WILL_WRITE | MFX_MEMTYPE_FROM_VPPOUT);

	// Allocate required surfaces for VPP output and encoder input
	sts = m_mfxAllocator.Alloc(m_mfxAllocator.pthis, &EncRequest,
		&m_mfxResponse);
	MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

	m_nmfxEncInSurfNum = m_mfxResponse.NumFrameActual;

	m_pmfxEncInSurfaces = new mfxFrameSurface1 *[m_nmfxEncInSurfNum];
	MSDK_CHECK_POINTER(m_pmfxEncInSurfaces, MFX_ERR_MEMORY_ALLOC);

	for (int i = 0; i < m_nmfxEncInSurfNum; i++) {
		m_pmfxEncInSurfaces[i] = new mfxFrameSurface1;
		memset(m_pmfxEncInSurfaces[i], 0, sizeof(mfxFrameSurface1));
		memcpy(&(m_pmfxEncInSurfaces[i]->Info),
			&(m_mfxEncParams.mfx.FrameInfo),
			sizeof(mfxFrameInfo));
		m_pmfxEncInSurfaces[i]->Data.MemId = m_mfxResponse.mids[i];
	}

    if (m_pmfxVPP)
    {
        mfxFrameAllocRequest VPPRequest[2];     // [0] - in, [1] - out
        memset(&VPPRequest, 0, sizeof(mfxFrameAllocRequest) * 2);
        sts = m_pmfxVPP->QueryIOSurf(&m_mfxVPPParams, VPPRequest);
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

        VPPRequest[0].Type |= (WILL_WRITE | MFX_MEMTYPE_FROM_VPPIN);
        VPPRequest[0].NumFrameMin = 1;
        VPPRequest[1].NumFrameMin = 0;

        sts = m_mfxAllocator.Alloc(m_mfxAllocator.pthis, VPPRequest,
            &m_mfxVPPResponse);
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

        //Init surface for VPP sync input
        m_nmfxVPPInSurfNum = m_mfxVPPResponse.NumFrameActual; //sync mode only
        m_pmfxVPPInSurfaces = new mfxFrameSurface1*[m_nmfxVPPInSurfNum];

        for (int i = 0; i < m_nmfxVPPInSurfNum; i++) {
            m_pmfxVPPInSurfaces[i] = new mfxFrameSurface1;
            memset(m_pmfxVPPInSurfaces[i], 0, sizeof(mfxFrameSurface1));
            memcpy(&(m_pmfxVPPInSurfaces[i]->Info),
                &(m_mfxVPPParams.vpp.In),
                sizeof(mfxFrameInfo));
            m_pmfxVPPInSurfaces[i]->Data.MemId = m_mfxVPPResponse.mids[i];
        }
    }

	return sts;
}

mfxStatus QSV_Encoder_Internal::GetVideoParam()
{
	memset(&m_parameter, 0, sizeof(m_parameter));
	mfxExtCodingOptionSPSPPS opt;
	memset(&m_parameter, 0, sizeof(m_parameter));
	opt.Header.BufferId = MFX_EXTBUFF_CODING_OPTION_SPSPPS;
	opt.Header.BufferSz = sizeof(mfxExtCodingOptionSPSPPS);

	static mfxExtBuffer* extendedBuffers[1];
	extendedBuffers[0] = (mfxExtBuffer*)& opt;
	m_parameter.ExtParam = extendedBuffers;
	m_parameter.NumExtParam = 1;

	opt.SPSBuffer = m_SPSBuffer;
	opt.PPSBuffer = m_PPSBuffer;
	opt.SPSBufSize = 100; //  m_nSPSBufferSize;
	opt.PPSBufSize = 100; //  m_nPPSBufferSize;

	mfxStatus sts = m_pmfxENC->GetVideoParam(&m_parameter);
	MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

	m_nSPSBufferSize = opt.SPSBufSize;
	m_nPPSBufferSize = opt.PPSBufSize;

	return sts;
}

void QSV_Encoder_Internal::GetSPSPPS(mfxU8 **pSPSBuf, mfxU8 **pPPSBuf,
		mfxU16 *pnSPSBuf, mfxU16 *pnPPSBuf)
{
	*pSPSBuf = m_SPSBuffer;
	*pPPSBuf = m_PPSBuffer;
	*pnSPSBuf = m_nSPSBufferSize;
	*pnPPSBuf = m_nPPSBufferSize;
}

mfxStatus QSV_Encoder_Internal::InitBitstream()
{
	m_nTaskPool = m_parameter.AsyncDepth;
	m_nFirstSyncTask = 0;

	m_pTaskPool = new Task[m_nTaskPool];
	memset(m_pTaskPool, 0, sizeof(Task) * m_nTaskPool);

	for (int i = 0; i < m_nTaskPool; i++) {
		m_pTaskPool[i].mfxBS.MaxLength =
			m_parameter.mfx.BufferSizeInKB * 1000;
		m_pTaskPool[i].mfxBS.Data =
			new mfxU8[m_pTaskPool[i].mfxBS.MaxLength];
		m_pTaskPool[i].mfxBS.DataOffset = 0;
		m_pTaskPool[i].mfxBS.DataLength = 0;

		MSDK_CHECK_POINTER(m_pTaskPool[i].mfxBS.Data,
				MFX_ERR_MEMORY_ALLOC);
	}

	memset(&m_outBitstream, 0, sizeof(mfxBitstream));
	m_outBitstream.MaxLength = m_parameter.mfx.BufferSizeInKB * 1000;
	m_outBitstream.Data = new mfxU8[m_outBitstream.MaxLength];
	m_outBitstream.DataOffset = 0;
	m_outBitstream.DataLength = 0;

	blog(LOG_INFO, "\tm_nTaskPool:    %d", m_nTaskPool);

	return MFX_ERR_NONE;
}

mfxStatus QSV_Encoder_Internal::LoadSysMem(mfxFrameSurface1 *pSurface,
		uint8_t *pData1stPlane, uint8_t *pData2ndPlane, uint32_t stride1stPlane,
		uint32_t stride2ndPlane)
{
    if ((pData1stPlane == nullptr) || (!stride1stPlane) || (!pSurface))
        return MFX_ERR_NULL_PTR;

    mfxStatus sts = m_mfxAllocator.Lock(m_mfxAllocator.pthis, pSurface->Data.MemId, &pSurface->Data);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

	mfxU16 w, h, i, pitch;
	mfxU8* ptr;
	mfxFrameInfo* pInfo = &pSurface->Info;
	mfxFrameData* pData = &pSurface->Data;

	if (pInfo->CropH > 0 && pInfo->CropW > 0)
	{
		w = pInfo->CropW;
		h = pInfo->CropH;
	}
	else
	{
		w = pInfo->Width;
		h = pInfo->Height;
	}

	pitch = pData->Pitch;

    if (pInfo->FourCC == MFX_FOURCC_NV12)
    {
        ptr = pData->Y + pInfo->CropX + pInfo->CropY * pData->Pitch;

        if ((pData2ndPlane == nullptr) || (!stride2ndPlane))
        {
            sts =  MFX_ERR_NULL_PTR;
            goto exit;
        }

        // load Y plane
        for (i = 0; i < h; i++)
            memcpy(ptr + i * pitch, pData1stPlane + i * stride1stPlane, w);

        // load UV plane
        ptr = pData->UV + pInfo->CropX + (pInfo->CropY / 2) * pitch;

        for (i = 0; i < h / 2; i++)
            memcpy(ptr + i * pitch, pData2ndPlane + i * stride2ndPlane, w);
    }
    else if (pInfo->FourCC == MFX_FOURCC_RGB4)
    {
        ptr = pData->B + pInfo->CropX + pInfo->CropY * pData->Pitch;

        for (i = 0; i < h; i++)
            memcpy(ptr + i * pitch, pData1stPlane + i * stride1stPlane, 4 * w);
    }
    else
    {
        sts = MFX_ERR_UNDEFINED_BEHAVIOR;
        goto exit;
    }

exit:
    m_mfxAllocator.Unlock(m_mfxAllocator.pthis, pSurface->Data.MemId, &pSurface->Data);
	return sts;
}

int QSV_Encoder_Internal::GetFreeTaskIndex(Task* pTaskPool, mfxU16 nPoolSize)
{
	if (pTaskPool)
		for (int i = 0; i < nPoolSize; i++)
			if (!pTaskPool[i].syncp)
				return i;
	return MFX_ERR_NOT_FOUND;
}

mfxStatus QSV_Encoder_Internal::Encode(uint64_t ts, uint8_t *pData1stPlane,
		uint8_t *pData2ndPlane, uint32_t stride1stPlane, uint32_t stride2ndPlane,
		mfxBitstream **pBS)
{
	mfxStatus sts = MFX_ERR_NONE;
	*pBS = NULL;
	int nTaskIdx = GetFreeTaskIndex(m_pTaskPool, m_nTaskPool);

#if 0
	info("MSDK Encode:\n"
		"\tTaskIndex: %d",
		nTaskIdx);
#endif

	int nSurfIdx = GetFreeSurfaceIndex(m_pmfxEncInSurfaces, m_nmfxEncInSurfNum);
#if 0
	info("MSDK Encode:\n"
		"\tnSurfIdx: %d",
		nSurfIdx);
#endif

	while (MFX_ERR_NOT_FOUND == nTaskIdx || MFX_ERR_NOT_FOUND == nSurfIdx) {
		// No more free tasks or surfaces, need to sync
		sts = m_session.SyncOperation(m_pTaskPool[m_nFirstSyncTask].syncp,
			SYNC_OPERATION_WAIT_TIME);
		MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

		mfxU8 *pTemp = m_outBitstream.Data;
		memcpy(&m_outBitstream, &m_pTaskPool[m_nFirstSyncTask].mfxBS,
			sizeof(mfxBitstream));

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

		nSurfIdx = GetFreeSurfaceIndex(m_pmfxEncInSurfaces, m_nmfxEncInSurfNum);
#if 0
		info("MSDK Encode:\n"
			"\tnSurfIdx: %d",
			nSurfIdx);
#endif
	}

	mfxFrameSurface1 *pSurface = m_pmfxEncInSurfaces[nSurfIdx];

    if (m_pmfxVPP)
    {
        mfxFrameSurface1* inSurface = m_pmfxVPPInSurfaces[0]; //the only surface

        sts = LoadSysMem(inSurface, pData1stPlane, pData2ndPlane, stride1stPlane, stride2ndPlane);
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

        mfxSyncPoint VPPSyncPoint;
        sts = m_pmfxVPP->RunFrameVPPAsync(inSurface, pSurface, NULL, &VPPSyncPoint);

        sts = m_session.SyncOperation(VPPSyncPoint, SYNC_OPERATION_WAIT_TIME);
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    }
    else
        sts = LoadSysMem(pSurface, pData1stPlane, pData2ndPlane, stride1stPlane, stride2ndPlane);

	pSurface->Data.TimeStamp = ts;

	for (;;) {
		// Encode a frame asynchronously (returns immediately)
		sts = m_pmfxENC->EncodeFrameAsync(NULL, pSurface,
			&m_pTaskPool[nTaskIdx].mfxBS,
			&m_pTaskPool[nTaskIdx].syncp);

		if (MFX_ERR_NONE < sts && !m_pTaskPool[nTaskIdx].syncp) {
			// Repeat the call if warning and no output
			if (MFX_WRN_DEVICE_BUSY == sts)
				MSDK_SLEEP(1);  // Wait if device is busy, then repeat the same call
		}
		else if (MFX_ERR_NONE < sts && m_pTaskPool[nTaskIdx].syncp) {
			sts = MFX_ERR_NONE;     // Ignore warnings if output is available
			break;
		}
		else if (MFX_ERR_NOT_ENOUGH_BUFFER == sts) {
			// Allocate more bitstream buffer memory here if needed...
			break;
		}
		else
			break;
	}

	return sts;
}

mfxStatus QSV_Encoder_Internal::Drain()
{
	mfxStatus sts = MFX_ERR_NONE;

	while (m_pTaskPool && m_pTaskPool[m_nFirstSyncTask].syncp) {
		sts = m_session.SyncOperation(m_pTaskPool[m_nFirstSyncTask].syncp, SYNC_OPERATION_WAIT_TIME);
		MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

		m_pTaskPool[m_nFirstSyncTask].syncp = NULL;
		m_nFirstSyncTask = (m_nFirstSyncTask + 1) % m_nTaskPool;
	}

	return sts;
}

mfxStatus QSV_Encoder_Internal::ClearData()
{
	mfxStatus sts	    = MFX_ERR_NONE;
	mfxStatus commonSts = MFX_ERR_NONE;
	sts = Drain();
	MSDK_GET_SHARED_RESULT(sts, commonSts);

    if (m_pmfxENC)
    {
        sts = m_pmfxENC->Close();
        MSDK_GET_SHARED_RESULT(sts, commonSts);
        delete m_pmfxENC;
        m_pmfxENC = NULL;
    }
    if (m_pmfxVPP)
    {
        sts = m_pmfxVPP->Close();
        MSDK_GET_SHARED_RESULT(sts, commonSts);
        delete m_pmfxVPP;
        m_pmfxVPP = NULL;
    }

	m_mfxAllocator.Free(m_mfxAllocator.pthis, &m_mfxResponse);
    m_mfxAllocator.Free(m_mfxAllocator.pthis, &m_mfxVPPResponse);

	for (int i = 0; i < m_nmfxEncInSurfNum; i++)
	{
		delete m_pmfxEncInSurfaces[i];
	}
	MSDK_SAFE_DELETE_ARRAY(m_pmfxEncInSurfaces);

	for (int i = 0; i < m_nmfxVPPInSurfNum; i++)
	{
		delete m_pmfxVPPInSurfaces[i];
	}
	MSDK_SAFE_DELETE_ARRAY(m_pmfxVPPInSurfaces);

	for (int i = 0; i < m_nTaskPool; i++)
		delete m_pTaskPool[i].mfxBS.Data;
	MSDK_SAFE_DELETE_ARRAY(m_pTaskPool);

    if (m_outBitstream.Data)
    {
        delete m_outBitstream.Data;
        m_outBitstream.Data = NULL;
    }

	if (sts >= MFX_ERR_NONE) {
		g_numEncodersOpen--;
	}

	if ((m_bUseD3D11 || m_bD3D9HACK) && (g_numEncodersOpen <= 0)) {
		Release();
		g_DX_Handle = NULL;
	}
	m_session.Close();
	return sts;
}

mfxStatus QSV_Encoder_Internal::Reset(qsv_param_t *pParams)
{
	mfxStatus sts = ClearData();
	MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

	sts = Open(pParams);
	MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

	return sts;
}

mfxStatus QSV_Encoder_Internal::GetCurrentFourCC(mfxU32& fourCC)
{
    if (m_pmfxENC != nullptr)
        fourCC = m_mfxEncParams.mfx.FrameInfo.FourCC;
    else
        return MFX_ERR_NOT_INITIALIZED;

    return MFX_ERR_NONE;
}
