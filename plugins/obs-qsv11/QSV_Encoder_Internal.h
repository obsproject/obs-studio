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
#include "mfxastructures.h"
#include "mfxvideo++.h"
#include "QSV_Encoder.h"
#include "common_utils.h"

class QSV_Encoder_Internal
{
public:
	QSV_Encoder_Internal(mfxIMPL& impl, mfxVersion& version);
	~QSV_Encoder_Internal();

	mfxStatus    Open(qsv_param_t * pParams);
	void         GetSPSPPS(mfxU8 **pSPSBuf, mfxU8 **pPPSBuf,
			mfxU16 *pnSPSBuf, mfxU16 *pnPPSBuf);
	mfxStatus    Encode(uint64_t ts, uint8_t *pDataY, uint8_t *pDataUV,
			uint32_t strideY, uint32_t strideUV, mfxBitstream
			**pBS);
	mfxStatus    ClearData();
	mfxStatus    Reset(qsv_param_t *pParams);

protected:
	bool         InitParams(qsv_param_t * pParams);
	mfxStatus    AllocateSurfaces();
	mfxStatus    GetVideoParam();
	mfxStatus    InitBitstream();
	mfxStatus    LoadNV12(mfxFrameSurface1 *pSurface, uint8_t *pDataY,
			uint8_t *pDataUV, uint32_t strideY, uint32_t strideUV);
	mfxStatus    Drain();
	int          GetFreeTaskIndex(Task* pTaskPool, mfxU16 nPoolSize);

private:
	mfxIMPL                        m_impl;
	mfxVersion                     m_ver;
	MFXVideoSession                m_session;
	mfxFrameAllocator              m_mfxAllocator;
	mfxVideoParam                  m_mfxEncParams;
	mfxFrameAllocResponse          m_mfxResponse;
	mfxFrameSurface1**             m_pmfxSurfaces;
	mfxU16                         m_nSurfNum;
	MFXVideoENCODE*                m_pmfxENC;
	mfxU8                          m_SPSBuffer[100];
	mfxU8                          m_PPSBuffer[100];
	mfxU16                         m_nSPSBufferSize;
	mfxU16                         m_nPPSBufferSize;
	mfxVideoParam                  m_parameter;
	mfxExtCodingOption2            m_co2;
	mfxExtCodingOption             m_co;
	mfxU16                         m_nTaskPool;
	Task*                          m_pTaskPool;
	int                            m_nTaskIdx;
	int                            m_nFirstSyncTask;
	mfxBitstream                   m_outBitstream;
	bool                           m_bIsWindows8OrGreater;
	bool                           m_bUseD3D11;
	bool                           m_bD3D9HACK;
	static mfxU16                  g_numEncodersOpen;
	static mfxHDL                  g_DX_Handle; // we only want one handle for all instances to use;
};

