/******************************************************************************* *\

Copyright (C) 2013-2017 Intel Corporation.  All rights reserved.

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

File Name: mfxfei.h

*******************************************************************************/
#ifndef __MFXFEI_H__
#define __MFXFEI_H__
#include "mfxdefs.h"
#include "mfxvstructures.h"
#include "mfxpak.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

typedef struct {
    mfxExtBuffer    Header;

    mfxU16    Qp;
    mfxU16    LenSP;
    mfxU16    SearchPath;
    mfxU16    SubMBPartMask;
    mfxU16    SubPelMode;
    mfxU16    InterSAD;
    mfxU16    IntraSAD;
    mfxU16    AdaptiveSearch;
    mfxU16    MVPredictor;
    mfxU16    MBQp;
    mfxU16    FTEnable;
    mfxU16    IntraPartMask;
    mfxU16    RefWidth;
    mfxU16    RefHeight;
    mfxU16    SearchWindow;
    mfxU16    DisableMVOutput;
    mfxU16    DisableStatisticsOutput;
    mfxU16    Enable8x8Stat;
    mfxU16    PictureType; /* Input picture type*/
    mfxU16    DownsampleInput;

    mfxU16    RefPictureType[2]; /* reference picture type, 0 -L0, 1 - L1 */
    mfxU16    DownsampleReference[2];
    mfxFrameSurface1 *RefFrame[2];
    mfxU16    reserved[28];
} mfxExtFeiPreEncCtrl;

typedef struct {
    mfxExtBuffer    Header;
    mfxU32  reserved1[3];
    mfxU32  NumMBAlloc; /* size of allocated memory in number of macroblocks */
    mfxU16  reserved2[20];

    struct  mfxExtFeiPreEncMVPredictorsMB {
        mfxI16Pair MV[2]; /* 0 for L0 and 1 for L1 */
    } *MB;
} mfxExtFeiPreEncMVPredictors;

typedef struct {
    mfxExtBuffer    Header;
    mfxU32  reserved1[3];
    mfxU32  NumMBAlloc;
    mfxU16  reserved2[20];

    mfxU8    *MB;
} mfxExtFeiEncQP;

/* PreENC output */
/* Layout is exactly the same as mfxExtFeiEncMVs, this buffer may be removed in future */
typedef struct {
    mfxExtBuffer    Header;
    mfxU32  reserved1[3];
    mfxU32  NumMBAlloc;
    mfxU16  reserved2[20];

    struct  mfxExtFeiPreEncMVMB {
        mfxI16Pair MV[16][2];
    } *MB;
} mfxExtFeiPreEncMV;

typedef struct {
    mfxExtBuffer    Header;
    mfxU32  reserved1[3];
    mfxU32  NumMBAlloc;
    mfxU16  reserved2[20];

    struct  mfxExtFeiPreEncMBStatMB {
        struct  mfxExtFeiPreEncMBStatMBInter {
            mfxU16  BestDistortion;
            mfxU16  Mode ;
        } Inter[2]; /*0 -L0, 1 - L1*/

        mfxU16  BestIntraDistortion;
        mfxU16  IntraMode ;

        mfxU16  NumOfNonZeroCoef;
        mfxU16  reserved1;
        mfxU32  SumOfCoef;

        mfxU32  reserved2;

        mfxU32  Variance16x16;
        mfxU32  Variance8x8[4];
        mfxU32  PixelAverage16x16;
        mfxU32  PixelAverage8x8[4];
    } *MB;
} mfxExtFeiPreEncMBStat;

/* 1  ENC_PAK input */
typedef struct {
    mfxExtBuffer    Header;

    mfxU16    SearchPath;
    mfxU16    LenSP;
    mfxU16    SubMBPartMask;
    mfxU16    IntraPartMask;
    mfxU16    MultiPredL0;
    mfxU16    MultiPredL1;
    mfxU16    SubPelMode;
    mfxU16    InterSAD;
    mfxU16    IntraSAD;
    mfxU16    DistortionType;
    mfxU16    RepartitionCheckEnable;
    mfxU16    AdaptiveSearch;
    mfxU16    MVPredictor;
    mfxU16    NumMVPredictors[2];
    mfxU16    PerMBQp;
    mfxU16    PerMBInput;
    mfxU16    MBSizeCtrl;
    mfxU16    RefWidth;
    mfxU16    RefHeight;
    mfxU16    SearchWindow;
    mfxU16    ColocatedMbDistortion;
    mfxU16    reserved[38];
} mfxExtFeiEncFrameCtrl;

typedef struct {
    mfxExtBuffer    Header;
    mfxU32  reserved1[3];
    mfxU32  NumMBAlloc;
    mfxU16  reserved2[20];

    struct  mfxExtFeiEncMVPredictorsMB {
        struct mfxExtFeiEncMVPredictorsMBRefIdx{
            mfxU8   RefL0: 4;
            mfxU8   RefL1: 4;
        } RefIdx[4]; /* index is predictor number */
        mfxU32      reserved;
        mfxI16Pair MV[4][2]; /* first index is predictor number, second is 0 for L0 and 1 for L1 */
    } *MB;
} mfxExtFeiEncMVPredictors;

typedef struct {
    mfxExtBuffer    Header;
    mfxU32  reserved1[3];
    mfxU32  NumMBAlloc;
    mfxU16  reserved2[20];

    struct  mfxExtFeiEncMBCtrlMB {
        mfxU32    ForceToIntra     : 1;
        mfxU32    ForceToSkip      : 1;
        mfxU32    ForceToNoneSkip  : 1;
#if (MFX_VERSION >= 1025)
        mfxU32    DirectBiasAdjustment        : 1;
        mfxU32    GlobalMotionBiasAdjustment  : 1;
        mfxU32    MVCostScalingFactor         : 3;

        mfxU32    reserved1        : 24;
#else
        mfxU32    reserved1        : 29;
#endif
        mfxU32    reserved2;
        mfxU32    reserved3;

        mfxU32    reserved4        : 16;
        mfxU32    TargetSizeInWord : 8;
        mfxU32    MaxSizeInWord    : 8;
    } *MB;
} mfxExtFeiEncMBCtrl;


/* 1 ENC_PAK output */
/* Buffer holds 32 MVs per MB. MVs are located in zigzag scan order.
Number in diagram below shows location of MV in memory.
For example, MV for right top 4x4 sub block is stored in 5-th element of the array.
========================
|| 00 | 01 || 04 | 05 ||
------------------------
|| 02 | 03 || 06 | 07 ||
========================
|| 08 | 09 || 12 | 13 ||
------------------------
|| 10 | 11 || 14 | 15 ||
========================
*/
typedef struct {
    mfxExtBuffer    Header;
    mfxU32  reserved1[3];
    mfxU32  NumMBAlloc;
    mfxU16  reserved2[20];

    struct  mfxExtFeiEncMVMB {
        mfxI16Pair MV[16][2]; /* first index is block (4x4 pixels) number, second is 0 for L0 and 1 for L1 */
    } *MB;
} mfxExtFeiEncMV;

typedef struct {
    mfxExtBuffer    Header;
    mfxU32  reserved1[3];
    mfxU32  NumMBAlloc;
    mfxU16  reserved2[20];

    struct mfxExtFeiEncMBStatMB {
        mfxU16  InterDistortion[16];
        mfxU16  BestInterDistortion;
        mfxU16  BestIntraDistortion;
        mfxU16  ColocatedMbDistortion;
        mfxU16  reserved;
        mfxU32  reserved1[2];
    } *MB;
} mfxExtFeiEncMBStat;

enum {
    MFX_PAK_OBJECT_HEADER = 0x7149000A
};

typedef struct {
    /* dword 0-2 */
    mfxU32    Header;  /* MFX_PAK_OBJECT_HEADER */
    mfxU32    MVDataLength;
    mfxU32    MVDataOffset;

    /* dword 3 */
    mfxU32    InterMbMode         : 2;
    mfxU32    MBSkipFlag          : 1;
    mfxU32    Reserved00          : 1;
    mfxU32    IntraMbMode         : 2;
    mfxU32    Reserved01          : 1;
    mfxU32    FieldMbPolarityFlag : 1;
    mfxU32    MbType              : 5;
    mfxU32    IntraMbFlag         : 1;
    mfxU32    FieldMbFlag         : 1;
    mfxU32    Transform8x8Flag    : 1;
    mfxU32    Reserved02          : 1;
    mfxU32    DcBlockCodedCrFlag  : 1;
    mfxU32    DcBlockCodedCbFlag  : 1;
    mfxU32    DcBlockCodedYFlag   : 1;
    mfxU32    MVFormat            : 3; /* layout and number of MVs, 0 - no MVs, 6 - 32 MVs, the rest are reserved */
    mfxU32    Reserved03          : 8;
    mfxU32    ExtendedFormat      : 1; /* should be 1, specifies that LumaIntraPredModes and RefIdx are replicated for 8x8 and 4x4 block/subblock */

    /* dword 4 */
    mfxU8     HorzOrigin;
    mfxU8     VertOrigin;
    mfxU16    CbpY;

    /* dword 5 */
    mfxU16    CbpCb;
    mfxU16    CbpCr;

    /* dword 6 */
    mfxU32    QpPrimeY               : 8;
    mfxU32    Reserved30             :17;
    mfxU32    MbSkipConvDisable      : 1;
    mfxU32    IsLastMB               : 1;
    mfxU32    EnableCoefficientClamp : 1;
    mfxU32    Direct8x8Pattern       : 4;

    union {
        struct {/* Intra MBs */
            /* dword 7-8 */
            mfxU16   LumaIntraPredModes[4];

            /* dword 9 */
            mfxU32   ChromaIntraPredMode : 2;
            mfxU32   IntraPredAvailFlags : 6;
            mfxU32   Reserved60          : 24;
        } IntraMB;
        struct {/* Inter MBs */
            /*dword 7 */
            mfxU8    SubMbShapes;
            mfxU8    SubMbPredModes;
            mfxU16   Reserved40;

            /* dword 8-9 */
            mfxU8    RefIdx[2][4]; /* first index is 0 for L0 and 1 for L1 */
        } InterMB;
    };

    /* dword 10 */
    mfxU16    Reserved70;
    mfxU8     TargetSizeInWord;
    mfxU8     MaxSizeInWord;

    mfxU32     reserved2[5];
}mfxFeiPakMBCtrl;

typedef struct {
    mfxExtBuffer    Header;
    mfxU32  reserved1[3];
    mfxU32  NumMBAlloc;
    mfxU16  reserved2[20];

    mfxFeiPakMBCtrl *MB;
} mfxExtFeiPakMBCtrl;

typedef struct {
    mfxExtBuffer    Header;
    mfxU32      MaxFrameSize; /* in bytes */
    mfxU32      NumPasses;    /* up to 8 */
    mfxU16      reserved[8];
    mfxU8       DeltaQP[8];   /* list of delta QPs, only positive values */
} mfxExtFeiRepackCtrl;

#if (MFX_VERSION >= 1025)
/* FEI repack status */
typedef struct {
    mfxExtBuffer    Header;
    mfxU32          NumPasses;
    mfxU16          reserved[58];
} mfxExtFeiRepackStat;
#endif

/* 1 decode stream out */
typedef struct {
    /* dword 0 */
    mfxU32    InterMbMode         : 2;
    mfxU32    MBSkipFlag          : 1;
    mfxU32    Reserved00          : 1;
    mfxU32    IntraMbMode         : 2;
    mfxU32    Reserved01          : 1;
    mfxU32    FieldMbPolarityFlag : 1;
    mfxU32    MbType              : 5;
    mfxU32    IntraMbFlag         : 1;
    mfxU32    FieldMbFlag         : 1;
    mfxU32    Transform8x8Flag    : 1;
    mfxU32    Reserved02          : 1;
    mfxU32    DcBlockCodedCrFlag  : 1;
    mfxU32    DcBlockCodedCbFlag  : 1;
    mfxU32    DcBlockCodedYFlag   : 1;
    mfxU32    Reserved03          :12;

    /* dword 1 */
    mfxU16     HorzOrigin;
    mfxU16     VertOrigin;

    /* dword 2 */
    mfxU32    CbpY                :16;
    mfxU32    CbpCb               : 4;
    mfxU32    CbpCr               : 4;
    mfxU32    Reserved20          : 6;
    mfxU32    IsLastMB            : 1;
    mfxU32    ConcealMB           : 1;

    /* dword 3 */
    mfxU32    QpPrimeY            : 7;
    mfxU32    Reserved30          : 1;
    mfxU32    Reserved31          : 8;
    mfxU32    NzCoeffCount        : 9;
    mfxU32    Reserved32          : 3;
    mfxU32    Direct8x8Pattern    : 4;

    /* dword 4-6 */
    union {
        struct {/* Intra MBs */
            /* dword 4-5 */
            mfxU16   LumaIntraPredModes[4];

            /* dword 6 */
            mfxU32   ChromaIntraPredMode : 2;
            mfxU32   IntraPredAvailFlags : 6;
            mfxU32   Reserved60          : 24;
        } IntraMB;

        struct {/* Inter MBs */
            /* dword 4 */
            mfxU8    SubMbShapes;
            mfxU8    SubMbPredModes;
            mfxU16   Reserved40;

            /* dword 5-6 */
            mfxU8    RefIdx[2][4]; /* first index is 0 for L0 and 1 for L1 */
        } InterMB;
    };

    /* dword 7 */
    mfxU32     Reserved70;

    /* dword 8-15 */
    mfxI16Pair MV[4][2]; /* L0 - 0, L1 - 1 */
}mfxFeiDecStreamOutMBCtrl;

typedef struct {
    mfxExtBuffer    Header;
    mfxU32  reserved1[3];
    mfxU32  NumMBAlloc;
    mfxU16  RemapRefIdx; /* tri-state option, default is OFF */
    mfxU16  PicStruct;
    mfxU16  reserved2[18];

    mfxFeiDecStreamOutMBCtrl *MB;
} mfxExtFeiDecStreamOut;

/* SPS, PPS, Slice Header */
typedef struct {
    mfxExtBuffer    Header;

    mfxU16    SPSId;

    mfxU16    PicOrderCntType;
    mfxU16    Log2MaxPicOrderCntLsb;
    mfxU16    reserved[121];
} mfxExtFeiSPS;

typedef struct {
    mfxExtBuffer    Header;

    mfxU16    SPSId;
    mfxU16    PPSId;

    mfxU16    PictureType;
    mfxU16    FrameType;
    mfxU16    PicInitQP;
    mfxU16    NumRefIdxL0Active;
    mfxU16    NumRefIdxL1Active;
    mfxI16    ChromaQPIndexOffset;
    mfxI16    SecondChromaQPIndexOffset;
    mfxU16    Transform8x8ModeFlag;
    mfxU16    reserved[114];

    struct mfxExtFeiPpsDPB {
        mfxU16    Index;    /* index in mfxPAKInput::L0Surface array */
        mfxU16    PicType;
        mfxI32    FrameNumWrap;
        mfxU16    LongTermFrameIdx;
        mfxU16    reserved[3];
    } DpbBefore[16], DpbAfter[16];
} mfxExtFeiPPS;

typedef struct {
    mfxExtBuffer    Header;

    mfxU16    NumSlice; /* actual number of slices in the picture */
    mfxU16    reserved[11];

    struct mfxSlice{
        mfxU16    MBAddress;
        mfxU16    NumMBs;
        mfxU16    SliceType;
        mfxU16    PPSId;
        mfxU16    IdrPicId;

        mfxU16    CabacInitIdc;

        mfxU16    NumRefIdxL0Active;
        mfxU16    NumRefIdxL1Active;

        mfxI16    SliceQPDelta;
        mfxU16    DisableDeblockingFilterIdc;
        mfxI16    SliceAlphaC0OffsetDiv2;
        mfxI16    SliceBetaOffsetDiv2;
        mfxU16    reserved[20];

        struct mfxSliceRef{
            mfxU16   PictureType;
            mfxU16   Index;
            mfxU16   reserved[2];
        } RefL0[32], RefL1[32]; /* index in mfxPAKInput::L0Surface array */

    } *Slice;
}mfxExtFeiSliceHeader;


typedef struct {
    mfxExtBuffer    Header;

    mfxU16  DisableHME;  /* 0 - enable, any other value means disable */
    mfxU16  DisableSuperHME;
    mfxU16  DisableUltraHME;

    mfxU16     reserved[57];
} mfxExtFeiCodingOption;


/* 1 functions */
typedef enum {
    MFX_FEI_FUNCTION_PREENC     =1,
    MFX_FEI_FUNCTION_ENCODE     =2,
    MFX_FEI_FUNCTION_ENC        =3,
    MFX_FEI_FUNCTION_PAK        =4,
    MFX_FEI_FUNCTION_DEC        =5,
} mfxFeiFunction;

enum {
    MFX_EXTBUFF_FEI_PARAM          = MFX_MAKEFOURCC('F','E','P','R'),
    MFX_EXTBUFF_FEI_PREENC_CTRL    = MFX_MAKEFOURCC('F','P','C','T'),
    MFX_EXTBUFF_FEI_PREENC_MV_PRED = MFX_MAKEFOURCC('F','P','M','P'),
    MFX_EXTBUFF_FEI_PREENC_MV      = MFX_MAKEFOURCC('F','P','M','V'),
    MFX_EXTBUFF_FEI_PREENC_MB      = MFX_MAKEFOURCC('F','P','M','B'),
    MFX_EXTBUFF_FEI_ENC_CTRL       = MFX_MAKEFOURCC('F','E','C','T'),
    MFX_EXTBUFF_FEI_ENC_MV_PRED    = MFX_MAKEFOURCC('F','E','M','P'),
    MFX_EXTBUFF_FEI_ENC_QP         = MFX_MAKEFOURCC('F','E','Q','P'),
    MFX_EXTBUFF_FEI_ENC_MV         = MFX_MAKEFOURCC('F','E','M','V'),
    MFX_EXTBUFF_FEI_ENC_MB         = MFX_MAKEFOURCC('F','E','M','B'),
    MFX_EXTBUFF_FEI_ENC_MB_STAT    = MFX_MAKEFOURCC('F','E','S','T'),
    MFX_EXTBUFF_FEI_PAK_CTRL       = MFX_MAKEFOURCC('F','K','C','T'),
    MFX_EXTBUFF_FEI_SPS            = MFX_MAKEFOURCC('F','S','P','S'),
    MFX_EXTBUFF_FEI_PPS            = MFX_MAKEFOURCC('F','P','P','S'),
    MFX_EXTBUFF_FEI_SLICE          = MFX_MAKEFOURCC('F','S','L','C'),
    MFX_EXTBUFF_FEI_CODING_OPTION  = MFX_MAKEFOURCC('F','C','D','O'),
    MFX_EXTBUFF_FEI_DEC_STREAM_OUT = MFX_MAKEFOURCC('F','D','S','O'),
    MFX_EXTBUFF_FEI_REPACK_CTRL    = MFX_MAKEFOURCC('F','E','R','P'),
#if (MFX_VERSION >= 1025)
    MFX_EXTBUFF_FEI_REPACK_STAT    = MFX_MAKEFOURCC('F','E','R','S')
#endif
};

/* should be attached to mfxVideoParam during initialization to indicate FEI function */
typedef struct {
    mfxExtBuffer    Header;
    mfxFeiFunction  Func;
    mfxU16  SingleFieldProcessing;
    mfxU16 reserved[57];
} mfxExtFeiParam;


#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */


#endif
