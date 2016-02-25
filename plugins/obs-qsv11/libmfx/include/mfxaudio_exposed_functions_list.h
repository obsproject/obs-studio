/* ****************************************************************************** *\

Copyright (C) 2013-2014 Intel Corporation.  All rights reserved.

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

File Name: mfxaudio_exposed_function_list.h

\* ****************************************************************************** */

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
