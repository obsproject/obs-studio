// Copyright (c) 2013-2019 Intel Corporation
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

//
// WARNING:
// this file doesn't contain an include guard by intension.
// The file may be included into a source file many times.
// That is why this header doesn't contain any include directive.
// Please, do no try to fix it.
//

//
// API version 1.8 functions
//

// Minor value should precedes the major value
#define API_VERSION {{8, 1}}

// CORE interface functions
FUNCTION(mfxStatus, MFXAudioCORE_SyncOperation, (mfxSession session, mfxSyncPoint syncp, mfxU32 wait), (session, syncp, wait))

// ENCODE interface functions
FUNCTION(mfxStatus, MFXAudioENCODE_Query, (mfxSession session, mfxAudioParam *in, mfxAudioParam *out), (session, in, out))
FUNCTION(mfxStatus, MFXAudioENCODE_QueryIOSize, (mfxSession session, mfxAudioParam *par, mfxAudioAllocRequest *request), (session, par, request))
FUNCTION(mfxStatus, MFXAudioENCODE_Init, (mfxSession session, mfxAudioParam *par), (session, par))
FUNCTION(mfxStatus, MFXAudioENCODE_Reset, (mfxSession session, mfxAudioParam *par), (session, par))
FUNCTION(mfxStatus, MFXAudioENCODE_Close, (mfxSession session), (session))
FUNCTION(mfxStatus, MFXAudioENCODE_GetAudioParam, (mfxSession session, mfxAudioParam *par), (session, par))
FUNCTION(mfxStatus, MFXAudioENCODE_EncodeFrameAsync, (mfxSession session, mfxAudioFrame *frame, mfxBitstream *buffer_out, mfxSyncPoint *syncp), (session, frame, buffer_out, syncp))

// DECODE interface functions
FUNCTION(mfxStatus, MFXAudioDECODE_Query, (mfxSession session, mfxAudioParam *in, mfxAudioParam *out), (session, in, out))
FUNCTION(mfxStatus, MFXAudioDECODE_DecodeHeader, (mfxSession session, mfxBitstream *bs, mfxAudioParam *par), (session, bs, par))
FUNCTION(mfxStatus, MFXAudioDECODE_Init, (mfxSession session, mfxAudioParam *par), (session, par))
FUNCTION(mfxStatus, MFXAudioDECODE_Reset, (mfxSession session, mfxAudioParam *par), (session, par))
FUNCTION(mfxStatus, MFXAudioDECODE_Close, (mfxSession session), (session))
FUNCTION(mfxStatus, MFXAudioDECODE_QueryIOSize, (mfxSession session, mfxAudioParam *par, mfxAudioAllocRequest *request), (session, par, request))
FUNCTION(mfxStatus, MFXAudioDECODE_GetAudioParam, (mfxSession session, mfxAudioParam *par), (session, par))
FUNCTION(mfxStatus, MFXAudioDECODE_DecodeFrameAsync, (mfxSession session, mfxBitstream *bs, mfxAudioFrame *frame_out, mfxSyncPoint *syncp), (session, bs, frame_out, syncp))

#undef API_VERSION

//
// API version 1.9 functions
//

#define API_VERSION {{9, 1}}

FUNCTION(mfxStatus, MFXAudioUSER_Register, (mfxSession session, mfxU32 type, const mfxPlugin *par), (session, type, par))
FUNCTION(mfxStatus, MFXAudioUSER_Unregister, (mfxSession session, mfxU32 type), (session, type))
FUNCTION(mfxStatus, MFXAudioUSER_ProcessFrameAsync, (mfxSession session, const mfxHDL *in, mfxU32 in_num, const mfxHDL *out, mfxU32 out_num, mfxSyncPoint *syncp), (session, in, in_num, out, out_num, syncp))


#undef API_VERSION
