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

// QSV_Encoder.cpp : Defines the exported functions for the DLL application.
//

#include "QSV_Encoder.h"
#include "QSV_Encoder_Internal.h"

QSV_Encoder_Internal *g_pEncoder = NULL;
mfxIMPL				impl = MFX_IMPL_HARDWARE_ANY;
mfxVersion			ver = { { 3, 1 } }; // for backward compatibility

qsv_t *qsv_encoder_open(qsv_param_t *pParams)
{
	QSV_Encoder_Internal *pEncoder = new QSV_Encoder_Internal(impl, ver);
	mfxStatus sts = pEncoder->Open(pParams);
	if (sts != MFX_ERR_NONE)
	{
		delete pEncoder;
		return NULL;
	}

	return (qsv_t *) pEncoder;
}

int qsv_encoder_headers(qsv_t *pContext, uint8_t **pSPS, uint8_t **pPPS, uint16_t *pnSPS, uint16_t *pnPPS)
{
	QSV_Encoder_Internal *pEncoder = (QSV_Encoder_Internal *)pContext;
	pEncoder->GetSPSPPS(pSPS, pPPS, pnSPS, pnPPS);

	return 0;
}

int qsv_encoder_encode(qsv_t * pContext, uint64_t ts, uint8_t *pDataY, uint8_t *pDataUV, uint32_t strideY, uint32_t strideUV, mfxBitstream **pBS)
{
	QSV_Encoder_Internal *pEncoder = (QSV_Encoder_Internal *)pContext;
	mfxFrameSurface1 *pSurface = NULL;
	mfxStatus sts = MFX_ERR_NONE;

	if (pDataY != NULL && pDataUV != NULL)
		sts = pEncoder->Encode(ts, pDataY, pDataUV, strideY, strideUV, pBS);
	
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
	
	return 0;
}

/*
int qsv_param_default_preset(qsv_param_t *pParams, const char *preset, const char *tune)
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



