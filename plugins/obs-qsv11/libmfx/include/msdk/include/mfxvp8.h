/******************************************************************************* *\

Copyright (C) 2007-2013 Intel Corporation.  All rights reserved.

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

File Name: mfxvp8.h

*******************************************************************************/
#ifndef __MFXVP8_H__
#define __MFXVP8_H__

#include "mfxdefs.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    MFX_CODEC_VP8 = MFX_MAKEFOURCC('V','P','8',' '),
};

/* CodecProfile*/
enum {
    MFX_PROFILE_VP8_0                       = 0+1, 
    MFX_PROFILE_VP8_1                       = 1+1,
    MFX_PROFILE_VP8_2                       = 2+1,
    MFX_PROFILE_VP8_3                       = 3+1,
};

/* Extended Buffer Ids */
enum {
    MFX_EXTBUFF_VP8_CODING_OPTION =   MFX_MAKEFOURCC('V','P','8','E'),
};

typedef struct { 
    mfxExtBuffer    Header;

    mfxU16   Version;
    mfxU16   EnableMultipleSegments;
    mfxU16   LoopFilterType;
    mfxU16   LoopFilterLevel[4];
    mfxU16   SharpnessLevel;
    mfxU16   NumTokenPartitions;
    mfxI16   LoopFilterRefTypeDelta[4];
    mfxI16   LoopFilterMbModeDelta[4];
    mfxI16   SegmentQPDelta[4];
    mfxI16   CoeffTypeQPDelta[5];
    mfxU16   WriteIVFHeaders;
    mfxU32   NumFramesForIVFHeader;
    mfxU16   reserved[223];
} mfxExtVP8CodingOption;

#ifdef __cplusplus
} // extern "C"
#endif

#endif

