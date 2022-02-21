// Copyright (c) 2017-2019 Intel Corporation
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
#ifndef __MFXENC_H__
#define __MFXENC_H__
#include "mfxdefs.h"
#include "mfxvstructures.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

MFX_PACK_BEGIN_STRUCT_W_PTR()
MFX_DEPRECATED typedef struct _mfxENCInput{
    mfxU32  reserved[32];

    mfxFrameSurface1 *InSurface;

    mfxU16  NumFrameL0;
    mfxFrameSurface1 **L0Surface;
    mfxU16  NumFrameL1;
    mfxFrameSurface1 **L1Surface;

    mfxU16  NumExtParam;
    mfxExtBuffer    **ExtParam;
} mfxENCInput;
MFX_PACK_END()

MFX_PACK_BEGIN_STRUCT_W_PTR()
MFX_DEPRECATED typedef struct _mfxENCOutput{
    mfxU32  reserved[32];

    mfxFrameSurface1 *OutSurface;

    mfxU16  NumExtParam;
    mfxExtBuffer    **ExtParam;
} mfxENCOutput;
MFX_PACK_END()


MFX_DEPRECATED mfxStatus MFX_CDECL MFXVideoENC_Query(mfxSession session, mfxVideoParam *in, mfxVideoParam *out);
MFX_DEPRECATED mfxStatus MFX_CDECL MFXVideoENC_QueryIOSurf(mfxSession session, mfxVideoParam *par, mfxFrameAllocRequest *request);
MFX_DEPRECATED mfxStatus MFX_CDECL MFXVideoENC_Init(mfxSession session, mfxVideoParam *par);
MFX_DEPRECATED mfxStatus MFX_CDECL MFXVideoENC_Reset(mfxSession session, mfxVideoParam *par);
MFX_DEPRECATED mfxStatus MFX_CDECL MFXVideoENC_Close(mfxSession session);

MFX_DEPRECATED mfxStatus MFX_CDECL MFXVideoENC_ProcessFrameAsync(mfxSession session, mfxENCInput *in, mfxENCOutput *out, mfxSyncPoint *syncp);

MFX_DEPRECATED mfxStatus MFX_CDECL MFXVideoENC_GetVideoParam(mfxSession session, mfxVideoParam *par);

#ifdef __cplusplus
} // extern "C"
#endif /* __cplusplus */


#endif

