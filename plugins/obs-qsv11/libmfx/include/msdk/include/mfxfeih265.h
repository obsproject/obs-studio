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

File Name: mfxfeih265.h

*******************************************************************************/
#ifndef __MFXFEIH265_H__
#define __MFXFEIH265_H__

#include "mfxdefs.h"
#include "mfxstructures.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/* FEI constants */
enum
{
    MFX_FEI_H265_INTERP_BORDER      = 4,
    MFX_FEI_H265_MAX_NUM_REF_FRAMES = 8,
    MFX_FEI_H265_MAX_INTRA_MODES    = 33,  /* intra angular prediction modes (excluding DC and planar) */
};

/* FEI block sizes */
enum
{
    MFX_FEI_H265_BLK_32x32 = 3,
    MFX_FEI_H265_BLK_32x16 = 4,
    MFX_FEI_H265_BLK_16x32 = 5,
    MFX_FEI_H265_BLK_16x16 = 6,
    MFX_FEI_H265_BLK_16x8  = 7,
    MFX_FEI_H265_BLK_8x16  = 8,
    MFX_FEI_H265_BLK_8x8   = 9,
};

/* FEI operations */
enum
{
    MFX_FEI_H265_OP_NOP         = 0x00,
    MFX_FEI_H265_OP_INTRA_MODE  = 0x01,
    MFX_FEI_H265_OP_INTRA_DIST  = 0x02,
    MFX_FEI_H265_OP_INTER_ME    = 0x04,
    MFX_FEI_H265_OP_INTERPOLATE = 0x08,
};

typedef struct
{
    mfxU16 Dist;
    mfxU16 reserved;
} mfxFEIH265IntraDist;

/* FEI output for current frame - can be used directly in SW encoder */
typedef struct
{
    /* intra processing pads to multiple of 16 if needed */
    mfxU32                PaddedWidth;
    mfxU32                PaddedHeight;

    /* ranked list of IntraMaxModes modes for each block size */
    mfxU32                IntraMaxModes;
    mfxU32              * IntraModes4x4;
    mfxU32              * IntraModes8x8;
    mfxU32              * IntraModes16x16;
    mfxU32              * IntraModes32x32;
    mfxU32                IntraPitch4x4;
    mfxU32                IntraPitch8x8;
    mfxU32                IntraPitch16x16;
    mfxU32                IntraPitch32x32;

    /* intra distortion estiamates (16x16 grid) */
    mfxU32                IntraPitch;
    mfxFEIH265IntraDist * IntraDist;

    /* motion vector estimates for multiple block sizes */
    mfxU32                PitchDist[64];
    mfxU32                PitchMV[64];
    mfxU32              * Dist[16][64];
    mfxI16Pair          * MV[16][64];
    
    /* half-pel interpolated planes - 4-pixel border added on all sides */
    mfxU32                InterpolateWidth;
    mfxU32                InterpolateHeight;
    mfxU32                InterpolatePitch;
    mfxU8               * Interp[16][3];

} mfxFEIH265Output;

enum 
{
    MFX_EXTBUFF_FEI_H265_PARAM  =   MFX_MAKEFOURCC('F','E','5','P'),
    MFX_EXTBUFF_FEI_H265_INPUT  =   MFX_MAKEFOURCC('F','E','5','I'),
    MFX_EXTBUFF_FEI_H265_OUTPUT =   MFX_MAKEFOURCC('F','E','5','O'),
};

typedef struct
{
    mfxExtBuffer       Header;

    mfxU32 MaxCUSize;
    mfxU32 MPMode;
    mfxU32 NumIntraModes;

    mfxU16             reserved[22];
} mfxExtFEIH265Param;

typedef struct
{
    mfxExtBuffer       Header;

    mfxU32             FEIOp;
    mfxU32             FrameType;
    mfxU32             RefIdx;

    mfxU16             reserved[22];
} mfxExtFEIH265Input;

typedef struct
{
    mfxExtBuffer       Header;

    mfxFEIH265Output  *feiOut;

    mfxU16             reserved[24];
} mfxExtFEIH265Output;


#ifdef __cplusplus
}
#endif

#endif  /* __MFXFEIH265_H__ */
