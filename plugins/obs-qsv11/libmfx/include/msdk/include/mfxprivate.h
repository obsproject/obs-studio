/******************************************************************************* *\

Copyright (C) 2014 Intel Corporation.  All rights reserved.

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

File Name: mfxprivate.h

*******************************************************************************/
#ifndef __MFXPRIVATE_H__
#define __MFXPRIVATE_H__

#include "mfxdefs.h"

#ifdef __cplusplus
extern "C" {
#endif


/* Extended Buffer Ids */
enum {
    MFX_EXTBUFF_AVC_ENCODE_CTRL            = MFX_MAKEFOURCC('A','E','C','L')
};

enum
{
    MFX_SKIP_CHECK_DEFAULT          = 0x00,
    MFX_SKIP_CHECK_DISABLE          = 0x01,
    MFX_SKIP_CHECK_FTQ_ON           = 0x02,
    MFX_SKIP_CHECK_FTQ_OFF          = 0x03,
    MFX_SKIP_CHECK_SET_THRESHOLDS   = 0x10,
};

typedef struct {
    mfxExtBuffer Header;

    //mfxU16 UseRawPicForRef;                     => mfxExtCodingOption2::UseRawRef
    //mfxU16 EnableDirectBiasAdjustment;          => mfxExtCodingOption3::DirectBiasAdjustment
    //mfxU16 EnableGlobalMotionBiasAdjustment;    => mfxExtCodingOption3::GlobalMotionBiasAdjustment
    //mfxU16 HMEMVCostScalingFactor;              => mfxExtCodingOption3::MVCostScalingFactor
    mfxU16 deprecated[4];
    mfxU16 reserved[8];

    mfxU16 SkipCheck;           /* enum */
    mfxU16 SkipThreshold[52];   /* 0-255 if FTQ_ON, 0-65535 if FTQ_OFF */

    mfxU16 LambdaValueFlag;     /* tri-state option */
    mfxU8  LambdaValue[52];     /* 0-255 */

    //mfxU16 MbDisableSkipMapSize; 
    //mfxU8* MbDisableSkipMap;      => mfxExtCodingOption3::MBDisableSkipMap = ON (Init), mfxExtMBDisableSkipMap (RT, no 64 alignment required)
    mfxU16 deprecated1[5];

} mfxExtAVCEncodeCtrl;

#ifdef __cplusplus
} // extern "C"
#endif

#endif
