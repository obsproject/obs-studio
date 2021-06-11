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
#ifndef __MFXMVC_H__
#define __MFXMVC_H__

#include "mfxdefs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CodecProfile, CodecLevel */
enum {
    /* MVC profiles */
    MFX_PROFILE_AVC_MULTIVIEW_HIGH =118,
    MFX_PROFILE_AVC_STEREO_HIGH    =128
};

/* Extended Buffer Ids */
enum {
    MFX_EXTBUFF_MVC_SEQ_DESC =   MFX_MAKEFOURCC('M','V','C','D'),
    MFX_EXTBUFF_MVC_TARGET_VIEWS    =   MFX_MAKEFOURCC('M','V','C','T')
};

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct  {
    mfxU16 ViewId;

    mfxU16 NumAnchorRefsL0;
    mfxU16 NumAnchorRefsL1;
    mfxU16 AnchorRefL0[16];
    mfxU16 AnchorRefL1[16];

    mfxU16 NumNonAnchorRefsL0;
    mfxU16 NumNonAnchorRefsL1;
    mfxU16 NonAnchorRefL0[16];
    mfxU16 NonAnchorRefL1[16];
} mfxMVCViewDependency;
MFX_PACK_END()

MFX_PACK_BEGIN_STRUCT_W_PTR()
typedef struct {
    mfxU16 TemporalId;
    mfxU16 LevelIdc;

    mfxU16 NumViews;
    mfxU16 NumTargetViews;
    mfxU16 *TargetViewId;
} mfxMVCOperationPoint;
MFX_PACK_END()

MFX_PACK_BEGIN_STRUCT_W_PTR()
typedef struct  {
    mfxExtBuffer Header;

    mfxU32 NumView;
    mfxU32 NumViewAlloc;
    mfxMVCViewDependency *View;

    mfxU32 NumViewId;
    mfxU32 NumViewIdAlloc;
    mfxU16 *ViewId;

    mfxU32 NumOP;
    mfxU32 NumOPAlloc;
    mfxMVCOperationPoint *OP;

    mfxU16 NumRefsTotal;
    mfxU32 Reserved[16];

} mfxExtMVCSeqDesc;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer    Header;

    mfxU16 TemporalId;
    mfxU32 NumView;
    mfxU16 ViewId[1024];
} mfxExtMVCTargetViews ;
MFX_PACK_END()

#ifdef __cplusplus
} // extern "C"
#endif

#endif

