/******************************************************************************* *\

Copyright (C) 2018 Intel Corporation.  All rights reserved.

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

File Name: mfxfeihevc.h

*******************************************************************************/
#ifndef __MFXFEIHEVC_H__
#define __MFXFEIHEVC_H__
#include "mfxcommon.h"

#ifdef __cplusplus
extern "C"
{
#endif

#if (MFX_VERSION >= 1027)

typedef struct {
    mfxExtBuffer Header;

    mfxU16  SearchPath;
    mfxU16  LenSP;
    mfxU16  RefWidth;
    mfxU16  RefHeight;
    mfxU16  SearchWindow;

    mfxU16  NumMvPredictors[2]; /* 0 for L0 and 1 for L1 */
    mfxU16  MultiPred[2];       /* 0 for L0 and 1 for L1 */

    mfxU16  SubPelMode;
    mfxU16  AdaptiveSearch;
    mfxU16  MVPredictor;

    mfxU16  PerCuQp;
    mfxU16  PerCtuInput;
    mfxU16  ForceCtuSplit;
    mfxU16  NumFramePartitions;
    mfxU16  FastIntraMode;

    mfxU16  reserved0[107];
} mfxExtFeiHevcEncFrameCtrl;


typedef struct {
    struct {
        mfxU8   RefL0 : 4;
        mfxU8   RefL1 : 4;
    } RefIdx[4]; /* index is predictor number */

    mfxU32     BlockSize : 2;
    mfxU32     reserved0 : 30;

    mfxI16Pair MV[4][2]; /* first index is predictor number, second is 0 for L0 and 1 for L1 */
} mfxFeiHevcEncMVPredictors;

typedef struct {
    mfxExtBuffer  Header;
    mfxU32        VaBufferID;
    mfxU32        Pitch;
    mfxU32        Height;
    mfxU16        reserved0[54];

    mfxFeiHevcEncMVPredictors *Data;
} mfxExtFeiHevcEncMVPredictors;


typedef struct {
    mfxExtBuffer  Header;
    mfxU32        VaBufferID;
    mfxU32        Pitch;
    mfxU32        Height;
    mfxU16        reserved[6];

    mfxU8    *Data;
} mfxExtFeiHevcEncQP;


typedef struct {
    mfxU32  ForceToIntra    : 1;
    mfxU32  ForceToInter    : 1;
    mfxU32  reserved0       : 30;

    mfxU32  reserved1[3];
} mfxFeiHevcEncCtuCtrl;


typedef struct {
    mfxExtBuffer Header;
    mfxU32  VaBufferID;
    mfxU32  Pitch;
    mfxU32  Height;
    mfxU16  reserved0[54];

    mfxFeiHevcEncCtuCtrl *Data;
} mfxExtFeiHevcEncCtuCtrl;

typedef struct {
    mfxExtBuffer    Header;
    mfxU32      MaxFrameSize; /* in bytes */
    mfxU32      NumPasses;    /* up to 8 */
    mfxU16      reserved[8];
    mfxU8       DeltaQP[8];   /* list of delta QPs, only positive values */
} mfxExtFeiHevcRepackCtrl;

typedef struct {
    mfxExtBuffer    Header;
    mfxU32          NumPasses;
    mfxU16          reserved[58];
} mfxExtFeiHevcRepackStat;

#if MFX_VERSION >= MFX_VERSION_NEXT
typedef struct  {
    /* DWORD 0 */
    mfxU32    reserved0;

    /* DWORD 1 */
    mfxU32    SplitLevel2Part0       : 4;
    mfxU32    SplitLevel2Part1       : 4;
    mfxU32    SplitLevel2Part2       : 4;
    mfxU32    SplitLevel2Part3       : 4;
    mfxU32    SplitLevel1            : 4;
    mfxU32    SplitLevel0            : 1;
    mfxU32    reserved10             : 3;
    mfxU32    CuCountMinus1          : 6;
    mfxU32    LastCtuOfTileFlag      : 1;
    mfxU32    LastCtuOfSliceFlag     : 1;


    /* DWORD 2 */
    mfxU32    CtuAddrX               : 16;
    mfxU32    CtuAddrY               : 16;

    /* DWORD 3 */
    mfxU32    reserved3;
} mfxFeiHevcPakCtuRecordV0;


typedef struct {
    mfxExtBuffer  Header;
    mfxU32        VaBufferID;
    mfxU32        Pitch;
    mfxU32        Height;
    mfxU16        reserved0[54];

    mfxFeiHevcPakCtuRecordV0 *Data;
} mfxExtFeiHevcPakCtuRecordV0;


typedef struct  {
    /* DWORD 0 */
    mfxU32    CuSize               : 2;
    mfxU32    PredMode             : 1;
    mfxU32    TransquantBypass     : 1;
    mfxU32    PartMode             : 3;
    mfxU32    IpcmEnable           : 1;
    mfxU32    IntraChromaMode      : 3;
    mfxU32    ZeroOutCoeffs        : 1;
    mfxU32    reserved00           : 4;
    mfxU32    Qp                   : 7;
    mfxU32    QpSign               : 1;
    mfxU32    InterpredIdc         : 8;


    /* DWORD 1 */
    mfxU32    IntraMode0           : 6;
    mfxU32    reserved10           : 2;
    mfxU32    IntraMode1           : 6;
    mfxU32    reserved11           : 2;
    mfxU32    IntraMode2           : 6;
    mfxU32    reserved12           : 2;
    mfxU32    IntraMode3           : 6;
    mfxU32    reserved13           : 2;


    /* DWORD 2-9 */
    struct {
        mfxI16 x[4];
        mfxI16 y[4];
    } MVs[2]; /* 0-L0, 1-L1 */


    /* DWORD 10 */
    struct{
        mfxU16   Ref0 : 4;
        mfxU16   Ref1 : 4;
        mfxU16   Ref2 : 4;
        mfxU16   Ref3 : 4;
    } RefIdx[2]; /* 0-L0, 1-L1 */


    /* DWORD 11 */
    mfxU32    TuSize;


    /* DWORD 12 */
    mfxU32    TransformSkipY       : 16;
    mfxU32    reserved120          : 12;
    mfxU32    TuCountM1            : 4;


    /* DWORD 13 */
    mfxU32    TransformSkipU       : 16;
    mfxU32    TransformSkipV       : 16;
} mfxFeiHevcPakCuRecordV0;


typedef struct {
    mfxExtBuffer  Header;
    mfxU32        VaBufferID;
    mfxU32        Pitch;
    mfxU32        Height;
    mfxU16        reserved0[54];

    mfxFeiHevcPakCuRecordV0 *Data;
} mfxExtFeiHevcPakCuRecordV0;


typedef struct {
    mfxU32        BestDistortion;
    mfxU32        ColocatedCtuDistortion;
} mfxFeiHevcDistortionCtu;


typedef struct {
    mfxExtBuffer  Header;
    mfxU32        VaBufferID;
    mfxU32        Pitch;
    mfxU32        Height;
    mfxU16        reserved[6];

    mfxFeiHevcDistortionCtu *Data;
} mfxExtFeiHevcDistortion;
#endif


enum {
    MFX_EXTBUFF_HEVCFEI_ENC_CTRL       = MFX_MAKEFOURCC('F','H','C','T'),
    MFX_EXTBUFF_HEVCFEI_ENC_MV_PRED    = MFX_MAKEFOURCC('F','H','P','D'),
    MFX_EXTBUFF_HEVCFEI_ENC_QP         = MFX_MAKEFOURCC('F','H','Q','P'),
    MFX_EXTBUFF_HEVCFEI_ENC_CTU_CTRL   = MFX_MAKEFOURCC('F','H','E','C'),
    MFX_EXTBUFF_HEVCFEI_REPACK_CTRL    = MFX_MAKEFOURCC('F','H','R','P'),
    MFX_EXTBUFF_HEVCFEI_REPACK_STAT    = MFX_MAKEFOURCC('F','H','R','S'),

#if MFX_VERSION >= MFX_VERSION_NEXT
    MFX_EXTBUFF_HEVCFEI_PAK_CTU_REC    = MFX_MAKEFOURCC('F','H','T','B'),
    MFX_EXTBUFF_HEVCFEI_PAK_CU_REC     = MFX_MAKEFOURCC('F','H','C','U'),
    MFX_EXTBUFF_HEVCFEI_ENC_DIST       = MFX_MAKEFOURCC('F','H','D','S')
#endif
};

#endif // MFX_VERSION

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */


#endif // __MFXFEIHEVC_H__
