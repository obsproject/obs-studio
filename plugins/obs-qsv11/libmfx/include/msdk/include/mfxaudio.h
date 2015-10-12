/*******************************************************************************

Copyright (C) 2013 Intel Corporation.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
- Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
- Neither the name of Intel Corporation nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY INTEL CORPORATION "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL INTEL CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

File Name: mfxaudio.h

*******************************************************************************/

#ifndef __MFXAUDIO_H__
#define __MFXAUDIO_H__
#include "mfxsession.h"
#include "mfxastructures.h"

#define MFX_AUDIO_VERSION_MAJOR 1
#define MFX_AUDIO_VERSION_MINOR 8

#ifdef __cplusplus
extern "C"
{
#endif

/* AudioCORE */
mfxStatus MFX_CDECL MFXAudioCORE_SyncOperation(mfxSession session, mfxSyncPoint syncp, mfxU32 wait);

/* AudioENCODE */
mfxStatus MFX_CDECL MFXAudioENCODE_Query(mfxSession session, mfxAudioParam *in, mfxAudioParam *out);
mfxStatus MFX_CDECL MFXAudioENCODE_QueryIOSize(mfxSession session, mfxAudioParam *par, mfxAudioAllocRequest *request);
mfxStatus MFX_CDECL MFXAudioENCODE_Init(mfxSession session, mfxAudioParam *par);
mfxStatus MFX_CDECL MFXAudioENCODE_Reset(mfxSession session, mfxAudioParam *par);
mfxStatus MFX_CDECL MFXAudioENCODE_Close(mfxSession session);
mfxStatus MFX_CDECL MFXAudioENCODE_GetAudioParam(mfxSession session, mfxAudioParam *par);
mfxStatus MFX_CDECL MFXAudioENCODE_EncodeFrameAsync(mfxSession session, mfxAudioFrame *frame, mfxBitstream *bs, mfxSyncPoint *syncp);

/* AudioDECODE */
mfxStatus MFX_CDECL MFXAudioDECODE_Query(mfxSession session, mfxAudioParam *in, mfxAudioParam *out);
mfxStatus MFX_CDECL MFXAudioDECODE_DecodeHeader(mfxSession session, mfxBitstream *bs, mfxAudioParam* par);
mfxStatus MFX_CDECL MFXAudioDECODE_Init(mfxSession session, mfxAudioParam *par);
mfxStatus MFX_CDECL MFXAudioDECODE_Reset(mfxSession session, mfxAudioParam *par);
mfxStatus MFX_CDECL MFXAudioDECODE_Close(mfxSession session);
mfxStatus MFX_CDECL MFXAudioDECODE_QueryIOSize(mfxSession session, mfxAudioParam *par, mfxAudioAllocRequest *request);
mfxStatus MFX_CDECL MFXAudioDECODE_GetAudioParam(mfxSession session, mfxAudioParam *par);
mfxStatus MFX_CDECL MFXAudioDECODE_DecodeFrameAsync(mfxSession session, mfxBitstream *bs, mfxAudioFrame *frame, mfxSyncPoint *syncp);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
