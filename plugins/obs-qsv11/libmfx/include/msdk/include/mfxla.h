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

File Name: mfxla.h

*******************************************************************************/
#ifndef __MFXLA_H__
#define __MFXLA_H__
#include "mfxdefs.h"
#include "mfxvstructures.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


enum 
{
    MFX_EXTBUFF_LOOKAHEAD_CTRL  =   MFX_MAKEFOURCC('L','A','C','T'),
    MFX_EXTBUFF_LOOKAHEAD_STAT  =   MFX_MAKEFOURCC('L','A','S','T'),
};


typedef struct
{
    mfxExtBuffer    Header;
    mfxU16  LookAheadDepth;
    mfxU16  DependencyDepth;
    mfxU16  DownScaleFactor;
    mfxU16  BPyramid;

    mfxU16  reserved1[23];
    
    mfxU16  NumOutStream;
    struct  mfxStream{
        mfxU16  Width;
        mfxU16  Height;
        mfxU16  reserved2[14];
    } OutStream[16];
}mfxExtLAControl;

typedef struct
{
    mfxU16  Width;
    mfxU16  Height;

    mfxU32  FrameType;
    mfxU32  FrameDisplayOrder;
    mfxU32  FrameEncodeOrder;

    mfxU32  IntraCost;
    mfxU32  InterCost;
    mfxU32  DependencyCost; //aggregated cost, how this frame influences subsequent frames
    mfxU16  Layer;
    mfxU16  reserved[23];

    mfxU64 EstimatedRate[52];
}mfxLAFrameInfo; 

typedef struct  {
    mfxExtBuffer    Header;

    mfxU16  reserved[20];

    mfxU16  NumAlloc; //number of allocated mfxLAFrameInfo structures
    mfxU16  NumStream; //number of resolutions
    mfxU16  NumFrame; //number of frames for each resolution
    mfxLAFrameInfo   *FrameStat; //frame statistics 

    mfxFrameSurface1 *OutSurface; //reordered surface

} mfxExtLAFrameStatistics;

#ifdef __cplusplus
} // extern "C"
#endif /* __cplusplus */


#endif

