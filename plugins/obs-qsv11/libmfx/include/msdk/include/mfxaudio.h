// Copyright (c) 2017 Intel Corporation
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef __MFXAUDIO_H__
#define __MFXAUDIO_H__
#include "mfxsession.h"
#include "mfxastructures.h"

#define MFX_AUDIO_VERSION_MAJOR 1
#define MFX_AUDIO_VERSION_MINOR 15

#ifdef __cplusplus
extern "C"
{
#endif

/* AudioCORE */
MFX_DEPRECATED mfxStatus MFX_CDECL MFXAudioCORE_SyncOperation(mfxSession session, mfxSyncPoint syncp, mfxU32 wait);

/* AudioENCODE */
MFX_DEPRECATED mfxStatus MFX_CDECL MFXAudioENCODE_Query(mfxSession session, mfxAudioParam *in, mfxAudioParam *out);
MFX_DEPRECATED mfxStatus MFX_CDECL MFXAudioENCODE_QueryIOSize(mfxSession session, mfxAudioParam *par, mfxAudioAllocRequest *request);
MFX_DEPRECATED mfxStatus MFX_CDECL MFXAudioENCODE_Init(mfxSession session, mfxAudioParam *par);
MFX_DEPRECATED mfxStatus MFX_CDECL MFXAudioENCODE_Reset(mfxSession session, mfxAudioParam *par);
MFX_DEPRECATED mfxStatus MFX_CDECL MFXAudioENCODE_Close(mfxSession session);
MFX_DEPRECATED mfxStatus MFX_CDECL MFXAudioENCODE_GetAudioParam(mfxSession session, mfxAudioParam *par);
MFX_DEPRECATED mfxStatus MFX_CDECL MFXAudioENCODE_EncodeFrameAsync(mfxSession session, mfxAudioFrame *frame, mfxBitstream *bs, mfxSyncPoint *syncp);

/* AudioDECODE */
MFX_DEPRECATED mfxStatus MFX_CDECL MFXAudioDECODE_Query(mfxSession session, mfxAudioParam *in, mfxAudioParam *out);
MFX_DEPRECATED mfxStatus MFX_CDECL MFXAudioDECODE_DecodeHeader(mfxSession session, mfxBitstream *bs, mfxAudioParam* par);
MFX_DEPRECATED mfxStatus MFX_CDECL MFXAudioDECODE_Init(mfxSession session, mfxAudioParam *par);
MFX_DEPRECATED mfxStatus MFX_CDECL MFXAudioDECODE_Reset(mfxSession session, mfxAudioParam *par);
MFX_DEPRECATED mfxStatus MFX_CDECL MFXAudioDECODE_Close(mfxSession session);
MFX_DEPRECATED mfxStatus MFX_CDECL MFXAudioDECODE_QueryIOSize(mfxSession session, mfxAudioParam *par, mfxAudioAllocRequest *request);
MFX_DEPRECATED mfxStatus MFX_CDECL MFXAudioDECODE_GetAudioParam(mfxSession session, mfxAudioParam *par);
MFX_DEPRECATED mfxStatus MFX_CDECL MFXAudioDECODE_DecodeFrameAsync(mfxSession session, mfxBitstream *bs, mfxAudioFrame *frame, mfxSyncPoint *syncp);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
