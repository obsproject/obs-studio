/******************************************************************************* *\

Copyright (C) 2010-2013 Intel Corporation.  All rights reserved.

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

File Name: mfxmvc.h

*******************************************************************************/
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

typedef struct {
    mfxU16 TemporalId;
    mfxU16 LevelIdc;

    mfxU16 NumViews;
    mfxU16 NumTargetViews;
    mfxU16 *TargetViewId;
} mfxMVCOperationPoint;

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

typedef struct {
    mfxExtBuffer    Header;

    mfxU16 TemporalId;
    mfxU32 NumView;
    mfxU16 ViewId[1024];
} mfxExtMVCTargetViews ;


#ifdef __cplusplus
} // extern "C"
#endif

#endif

