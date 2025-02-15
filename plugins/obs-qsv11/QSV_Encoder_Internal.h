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
#pragma once
#define ONEVPL_EXPERIMENTAL
#include <vpl/mfxstructures.h>
#include <vpl/mfxvideo++.h>
#include "QSV_Encoder.h"
#include "common_utils.h"

#include <vector>

class QSV_Encoder_Internal {
public:
	QSV_Encoder_Internal(mfxVersion &version, bool useTexAlloc);
	~QSV_Encoder_Internal();

	mfxStatus Open(qsv_param_t *pParams, enum qsv_codec codec);
	void GetSPSPPS(mfxU8 **pSPSBuf, mfxU8 **pPPSBuf, mfxU16 *pnSPSBuf, mfxU16 *pnPPSBuf);
	void GetVpsSpsPps(mfxU8 **pVPSBuf, mfxU8 **pSPSBuf, mfxU8 **pPPSBuf, mfxU16 *pnVPSBuf, mfxU16 *pnSPSBuf,
			  mfxU16 *pnPPSBuf);
	mfxStatus Encode(uint64_t ts, uint8_t *pDataY, uint8_t *pDataUV, uint32_t strideY, uint32_t strideUV,
			 mfxBitstream **pBS);
	mfxStatus Encode_tex(uint64_t ts, void *tex, uint64_t lock_key, uint64_t *next_key, mfxBitstream **pBS);
	mfxStatus ClearData();
	mfxStatus Reset(qsv_param_t *pParams, enum qsv_codec codec);
	mfxStatus ReconfigureEncoder();
	bool UpdateParams(qsv_param_t *pParams);
	void AddROI(mfxU32 left, mfxU32 top, mfxU32 right, mfxU32 bottom, mfxI16 delta);
	void ClearROI();

protected:
	mfxStatus InitParams(qsv_param_t *pParams, enum qsv_codec codec);
	mfxStatus AllocateSurfaces();
	mfxStatus GetVideoParam(enum qsv_codec codec);
	mfxStatus InitBitstream();
	mfxStatus LoadNV12(mfxFrameSurface1 *pSurface, uint8_t *pDataY, uint8_t *pDataUV, uint32_t strideY,
			   uint32_t strideUV);
	mfxStatus LoadP010(mfxFrameSurface1 *pSurface, uint8_t *pDataY, uint8_t *pDataUV, uint32_t strideY,
			   uint32_t strideUV);
	mfxStatus Drain();
	int GetFreeTaskIndex(Task *pTaskPool, mfxU16 nPoolSize);

private:
	mfxVersion m_ver;
	mfxSession m_session;
	void *m_sessionData;
	mfxFrameAllocator m_mfxAllocator;
	mfxVideoParam m_mfxEncParams;
	mfxFrameAllocResponse m_mfxResponse{};
	mfxFrameSurface1 **m_pmfxSurfaces;
	mfxU16 m_nSurfNum;
	MFXVideoENCODE *m_pmfxENC;
	mfxU8 m_VPSBuffer[1024];
	mfxU8 m_SPSBuffer[1024];
	mfxU8 m_PPSBuffer[1024];
	mfxU16 m_nVPSBufferSize;
	mfxU16 m_nSPSBufferSize;
	mfxU16 m_nPPSBufferSize;
	mfxVideoParam m_parameter;
	std::vector<mfxExtBuffer *> extendedBuffers;
	mfxExtCodingOption3 m_co3;
	mfxExtCodingOption2 m_co2;
	mfxExtCodingOption m_co;
	mfxExtHEVCParam m_ExtHEVCParam{};
	mfxExtAV1TileParam m_ExtAv1TileParam{};
#if (MFX_VERSION_MAJOR >= 2 && MFX_VERSION_MINOR >= 11) || MFX_VERSION_MAJOR > 2
	mfxExtAV1ScreenContentTools m_ExtAV1ScreenContentTools{};
#endif
	mfxExtVideoSignalInfo m_ExtVideoSignalInfo{};
	mfxExtChromaLocInfo m_ExtChromaLocInfo{};
	mfxExtMasteringDisplayColourVolume m_ExtMasteringDisplayColourVolume{};
	mfxExtContentLightLevelInfo m_ExtContentLightLevelInfo{};
	mfxU16 m_nTaskPool;
	Task *m_pTaskPool;
	int m_nTaskIdx;
	int m_nFirstSyncTask;
	mfxBitstream m_outBitstream;
	bool m_bUseD3D11;
	bool m_bUseTexAlloc;
	static mfxU16 g_numEncodersOpen;
	static mfxHDL g_GFX_Handle; // we only want one handle for all instances to use;

	mfxEncodeCtrl m_ctrl;
	mfxExtEncoderROI m_roi;
	std::vector<mfxExtBuffer *> m_extbuf;
};
