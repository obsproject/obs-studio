/******************************************************************************* *\

Copyright (C) 2007-2014 Intel Corporation.  All rights reserved.

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

File Name: mfxstructures.h

*******************************************************************************/
#ifndef __MFXSTRUCTURES_H__
#define __MFXSTRUCTURES_H__
#include "mfxcommon.h"

#pragma warning(disable: 4201)

#ifdef __cplusplus
extern "C" {
#endif


/* Frame ID for SVC and MVC */
typedef struct {
    mfxU16      TemporalId;
    mfxU16      PriorityId;
    union {
        struct {
            mfxU16  DependencyId;
            mfxU16  QualityId;
        };
        struct {
            mfxU16  ViewId;
        };
    };
} mfxFrameId;

/* Frame Info */
typedef struct {
    mfxU32  reserved[4];
    mfxU16  reserved4;
    mfxU16  BitDepthLuma;
    mfxU16  BitDepthChroma;
    mfxU16  Shift;

    mfxFrameId FrameId;

    mfxU32  FourCC;
    mfxU16  Width;
    mfxU16  Height;

    mfxU16  CropX;
    mfxU16  CropY;
    mfxU16  CropW;
    mfxU16  CropH;

    mfxU32  FrameRateExtN;
    mfxU32  FrameRateExtD;
    mfxU16  reserved3;

    mfxU16  AspectRatioW;
    mfxU16  AspectRatioH;

    mfxU16  PicStruct;
    mfxU16  ChromaFormat;
    mfxU16  reserved2;
} mfxFrameInfo;

/* FourCC */
enum {
    MFX_FOURCC_NV12         = MFX_MAKEFOURCC('N','V','1','2'),   /* Native Format */
    MFX_FOURCC_YV12         = MFX_MAKEFOURCC('Y','V','1','2'),
    MFX_FOURCC_YUY2         = MFX_MAKEFOURCC('Y','U','Y','2'),
    MFX_FOURCC_RGB3         = MFX_MAKEFOURCC('R','G','B','3'),   /* deprecated */
    MFX_FOURCC_RGB4         = MFX_MAKEFOURCC('R','G','B','4'),   /* ARGB in that order, A channel is 8 MSBs */
    MFX_FOURCC_P8           = 41,                                /*  D3DFMT_P8   */
    MFX_FOURCC_P8_TEXTURE   = MFX_MAKEFOURCC('P','8','M','B'),
    MFX_FOURCC_P010         = MFX_MAKEFOURCC('P','0','1','0'), 
    MFX_FOURCC_BGR4         = MFX_MAKEFOURCC('B','G','R','4'),   /* ABGR in that order, A channel is 8 MSBs */
    MFX_FOURCC_A2RGB10      = MFX_MAKEFOURCC('R','G','1','0'),   /* ARGB in that order, A channel is two MSBs */
    MFX_FOURCC_ARGB16       = MFX_MAKEFOURCC('R','G','1','6'),   /* ARGB in that order, 64 bits, A channel is 16 MSBs */
    MFX_FOURCC_R16          = MFX_MAKEFOURCC('R','1','6','U') 
};

/* PicStruct */
enum {
    MFX_PICSTRUCT_UNKNOWN       =0x00,
    MFX_PICSTRUCT_PROGRESSIVE   =0x01,
    MFX_PICSTRUCT_FIELD_TFF     =0x02,
    MFX_PICSTRUCT_FIELD_BFF     =0x04,

    MFX_PICSTRUCT_FIELD_REPEATED=0x10,  /* first field repeated, pic_struct=5 or 6 in H.264 */
    MFX_PICSTRUCT_FRAME_DOUBLING=0x20,  /* pic_struct=7 in H.264 */
    MFX_PICSTRUCT_FRAME_TRIPLING=0x40   /* pic_struct=8 in H.264 */
};

/* ColorFormat */
enum {
    MFX_CHROMAFORMAT_MONOCHROME =0,
    MFX_CHROMAFORMAT_YUV420     =1,
    MFX_CHROMAFORMAT_YUV422     =2,
    MFX_CHROMAFORMAT_YUV444     =3,
    MFX_CHROMAFORMAT_YUV400     = MFX_CHROMAFORMAT_MONOCHROME,
    MFX_CHROMAFORMAT_YUV411     = 4,
    MFX_CHROMAFORMAT_YUV422H    = MFX_CHROMAFORMAT_YUV422,
    MFX_CHROMAFORMAT_YUV422V    = 5
};

enum {
    MFX_TIMESTAMP_UNKNOWN = -1
};

enum {
    MFX_FRAMEORDER_UNKNOWN = -1
};

/* DataFlag in mfxFrameData */
enum {
    MFX_FRAMEDATA_ORIGINAL_TIMESTAMP = 0x0001
};

/* Corrupted in mfxFrameData */
enum {
    MFX_CORRUPTION_MINOR           = 0x0001,
    MFX_CORRUPTION_MAJOR           = 0x0002,
    MFX_CORRUPTION_ABSENT_TOP_FIELD           = 0x0004,
    MFX_CORRUPTION_ABSENT_BOTTOM_FIELD           = 0x0008,
    MFX_CORRUPTION_REFERENCE_FRAME = 0x0010,
    MFX_CORRUPTION_REFERENCE_LIST  = 0x0020
};

/* Frame Data Info */
typedef struct {
    mfxU32      reserved[7];
    mfxU16      reserved1;
    mfxU16      PitchHigh;

    mfxU64      TimeStamp;
    mfxU32      FrameOrder;
    mfxU16      Locked;
    union{
        mfxU16  Pitch;
        mfxU16  PitchLow;
    };

    /* color planes */
    union {
        mfxU8   *Y;
        mfxU16  *Y16;
        mfxU8   *R;
    };
    union {
        mfxU8   *UV;            /* for UV merged formats */
        mfxU8   *VU;            /* for VU merged formats */
        mfxU8   *CbCr;          /* for CbCr merged formats */
        mfxU8   *CrCb;          /* for CrCb merged formats */
        mfxU8   *Cb;
        mfxU8   *U;
        mfxU16  *U16;
        mfxU8   *G;
    };
    union {
        mfxU8   *Cr;
        mfxU8   *V;
        mfxU16  *V16;
        mfxU8   *B;
    };
    mfxU8       *A;
    mfxMemId    MemId;

    /* Additional Flags */
    mfxU16  Corrupted;
    mfxU16  DataFlag;
} mfxFrameData;

/* Frame Surface */
typedef struct {
    mfxU32  reserved[4];
    mfxFrameInfo    Info;
    mfxFrameData    Data;
} mfxFrameSurface1;

enum {
    MFX_TIMESTAMPCALC_UNKNOWN = 0,
    MFX_TIMESTAMPCALC_TELECINE = 1,
};

/* Transcoding Info */
typedef struct {
    mfxU32  reserved[7];

    mfxU16  reserved4;
    mfxU16  BRCParamMultiplier;

    mfxFrameInfo    FrameInfo;
    mfxU32  CodecId;
    mfxU16  CodecProfile;
    mfxU16  CodecLevel;
    mfxU16  NumThread;

    union {
        struct {   /* MPEG-2/H.264 Encoding Options */
            mfxU16  TargetUsage;

            mfxU16  GopPicSize;
            mfxU16  GopRefDist;
            mfxU16  GopOptFlag;
            mfxU16  IdrInterval;

            mfxU16  RateControlMethod;
            union {
                mfxU16  InitialDelayInKB;
                mfxU16  QPI;
                mfxU16  Accuracy;
            };
            mfxU16  BufferSizeInKB;
            union {
                mfxU16  TargetKbps;
                mfxU16  QPP;
                mfxU16  ICQQuality;
            };
            union {
                mfxU16  MaxKbps;
                mfxU16  QPB;
                mfxU16  Convergence;
            };

            mfxU16  NumSlice;
            mfxU16  NumRefFrame;
            mfxU16  EncodedOrder;
        };
        struct {   /* H.264, MPEG-2 and VC-1 Decoding Options */
            mfxU16  DecodedOrder;
            mfxU16  ExtendedPicStruct;
            mfxU16  TimeStampCalc;
            mfxU16  SliceGroupsPresent;
            mfxU16  reserved2[9];
        };
        struct {   /* JPEG Decoding Options */
            mfxU16  JPEGChromaFormat;
            mfxU16  Rotation;
            mfxU16  JPEGColorFormat;
            mfxU16  InterleavedDec;
            mfxU16  reserved3[9];
        };
        struct {   /* JPEG Encoding Options */
            mfxU16  Interleaved;
            mfxU16  Quality;
            mfxU16  RestartInterval;
            mfxU16  reserved5[10];
        };
    };
} mfxInfoMFX;

typedef struct {
    mfxU32  reserved[8];
    mfxFrameInfo    In;
    mfxFrameInfo    Out;
} mfxInfoVPP;

typedef struct {
    mfxU32  reserved[3];
    mfxU16  reserved3;
    mfxU16  AsyncDepth;

    union {
        mfxInfoMFX  mfx;
        mfxInfoVPP  vpp;
    };
    mfxU16  Protected;
    mfxU16  IOPattern;
    mfxExtBuffer** ExtParam;
    mfxU16  NumExtParam;
    mfxU16  reserved2;
} mfxVideoParam;

/* IOPattern */
enum {
    MFX_IOPATTERN_IN_VIDEO_MEMORY   = 0x01,
    MFX_IOPATTERN_IN_SYSTEM_MEMORY  = 0x02,
    MFX_IOPATTERN_IN_OPAQUE_MEMORY  = 0x04,
    MFX_IOPATTERN_OUT_VIDEO_MEMORY  = 0x10,
    MFX_IOPATTERN_OUT_SYSTEM_MEMORY = 0x20,
    MFX_IOPATTERN_OUT_OPAQUE_MEMORY = 0x40
};

/* CodecId */
enum {
    MFX_CODEC_AVC         =MFX_MAKEFOURCC('A','V','C',' '),
    MFX_CODEC_HEVC        =MFX_MAKEFOURCC('H','E','V','C'),
    MFX_CODEC_MPEG2       =MFX_MAKEFOURCC('M','P','G','2'),
    MFX_CODEC_VC1         =MFX_MAKEFOURCC('V','C','1',' ')
};

/* CodecProfile, CodecLevel */
enum {
    MFX_PROFILE_UNKNOWN                     =0,
    MFX_LEVEL_UNKNOWN                       =0,

    /* AVC Profiles & Levels */
    MFX_PROFILE_AVC_CONSTRAINT_SET0     = (0x100 << 0),
    MFX_PROFILE_AVC_CONSTRAINT_SET1     = (0x100 << 1),
    MFX_PROFILE_AVC_CONSTRAINT_SET2     = (0x100 << 2),
    MFX_PROFILE_AVC_CONSTRAINT_SET3     = (0x100 << 3),
    MFX_PROFILE_AVC_CONSTRAINT_SET4     = (0x100 << 4),
    MFX_PROFILE_AVC_CONSTRAINT_SET5     = (0x100 << 5),

    MFX_PROFILE_AVC_BASELINE                =66,
    MFX_PROFILE_AVC_MAIN                    =77,
    MFX_PROFILE_AVC_EXTENDED                =88,
    MFX_PROFILE_AVC_HIGH                    =100,
    MFX_PROFILE_AVC_CONSTRAINED_BASELINE    =MFX_PROFILE_AVC_BASELINE + MFX_PROFILE_AVC_CONSTRAINT_SET1,
    MFX_PROFILE_AVC_CONSTRAINED_HIGH        =MFX_PROFILE_AVC_HIGH     + MFX_PROFILE_AVC_CONSTRAINT_SET4
                                                                      + MFX_PROFILE_AVC_CONSTRAINT_SET5,
    MFX_PROFILE_AVC_PROGRESSIVE_HIGH        =MFX_PROFILE_AVC_HIGH     + MFX_PROFILE_AVC_CONSTRAINT_SET4,

    MFX_LEVEL_AVC_1                         =10,
    MFX_LEVEL_AVC_1b                        =9,
    MFX_LEVEL_AVC_11                        =11,
    MFX_LEVEL_AVC_12                        =12,
    MFX_LEVEL_AVC_13                        =13,
    MFX_LEVEL_AVC_2                         =20,
    MFX_LEVEL_AVC_21                        =21,
    MFX_LEVEL_AVC_22                        =22,
    MFX_LEVEL_AVC_3                         =30,
    MFX_LEVEL_AVC_31                        =31,
    MFX_LEVEL_AVC_32                        =32,
    MFX_LEVEL_AVC_4                         =40,
    MFX_LEVEL_AVC_41                        =41,
    MFX_LEVEL_AVC_42                        =42,
    MFX_LEVEL_AVC_5                         =50,
    MFX_LEVEL_AVC_51                        =51,
    MFX_LEVEL_AVC_52                        =52,

    /* MPEG-2 Profiles & Levels */
    MFX_PROFILE_MPEG2_SIMPLE                =0x50,
    MFX_PROFILE_MPEG2_MAIN                  =0x40,
    MFX_PROFILE_MPEG2_HIGH                  =0x10,

    MFX_LEVEL_MPEG2_LOW                     =0xA,
    MFX_LEVEL_MPEG2_MAIN                    =0x8,
    MFX_LEVEL_MPEG2_HIGH                    =0x4,
    MFX_LEVEL_MPEG2_HIGH1440                =0x6,

    /* VC1 Profiles & Levels */
    MFX_PROFILE_VC1_SIMPLE                  =(0+1),
    MFX_PROFILE_VC1_MAIN                    =(4+1),
    MFX_PROFILE_VC1_ADVANCED                =(12+1),

    /* VC1 levels for simple & main profiles */
    MFX_LEVEL_VC1_LOW                       =(0+1),
    MFX_LEVEL_VC1_MEDIAN                    =(2+1),
    MFX_LEVEL_VC1_HIGH                      =(4+1),

    /* VC1 levels for the advanced profile */
    MFX_LEVEL_VC1_0                         =(0x00+1),
    MFX_LEVEL_VC1_1                         =(0x01+1),
    MFX_LEVEL_VC1_2                         =(0x02+1),
    MFX_LEVEL_VC1_3                         =(0x03+1),
    MFX_LEVEL_VC1_4                         =(0x04+1),

    /* HEVC Profiles & Levels & Tiers */
    MFX_PROFILE_HEVC_MAIN             =1,
    MFX_PROFILE_HEVC_MAIN10           =2,
    MFX_PROFILE_HEVC_MAINSP           =3,

    MFX_LEVEL_HEVC_1   = 10,
    MFX_LEVEL_HEVC_2   = 20,
    MFX_LEVEL_HEVC_21  = 21,
    MFX_LEVEL_HEVC_3   = 30,
    MFX_LEVEL_HEVC_31  = 31,
    MFX_LEVEL_HEVC_4   = 40,
    MFX_LEVEL_HEVC_41  = 41,
    MFX_LEVEL_HEVC_5   = 50,
    MFX_LEVEL_HEVC_51  = 51,
    MFX_LEVEL_HEVC_52  = 52,
    MFX_LEVEL_HEVC_6   = 60,
    MFX_LEVEL_HEVC_61  = 61,
    MFX_LEVEL_HEVC_62  = 62,

    MFX_TIER_HEVC_MAIN  = 0,
    MFX_TIER_HEVC_HIGH  = 0x100,
};

/* GopOptFlag */
enum {
    MFX_GOP_CLOSED          =1,
    MFX_GOP_STRICT          =2
};

/* TargetUsages: from 1 to 7 inclusive */
enum {
    MFX_TARGETUSAGE_1    =1,
    MFX_TARGETUSAGE_2    =2,
    MFX_TARGETUSAGE_3    =3,
    MFX_TARGETUSAGE_4    =4,
    MFX_TARGETUSAGE_5    =5,
    MFX_TARGETUSAGE_6    =6,
    MFX_TARGETUSAGE_7    =7,

    MFX_TARGETUSAGE_UNKNOWN         =0,
    MFX_TARGETUSAGE_BEST_QUALITY    =MFX_TARGETUSAGE_1,
    MFX_TARGETUSAGE_BALANCED        =MFX_TARGETUSAGE_4,
    MFX_TARGETUSAGE_BEST_SPEED      =MFX_TARGETUSAGE_7
};

/* RateControlMethod */
enum {
    MFX_RATECONTROL_CBR     =1,
    MFX_RATECONTROL_VBR     =2,
    MFX_RATECONTROL_CQP     =3,
    MFX_RATECONTROL_AVBR    =4,
    MFX_RATECONTROL_RESERVED1 =5,
    MFX_RATECONTROL_RESERVED2 =6,
    MFX_RATECONTROL_RESERVED3 =100,
    MFX_RATECONTROL_RESERVED4 =7,
    MFX_RATECONTROL_LA        =8,
    MFX_RATECONTROL_ICQ       =9,
    MFX_RATECONTROL_VCM       =10,
    MFX_RATECONTROL_LA_ICQ    =11
};

/* Trellis control*/
enum {
    MFX_TRELLIS_UNKNOWN =0,
    MFX_TRELLIS_OFF     =0x01,
    MFX_TRELLIS_I       =0x02,
    MFX_TRELLIS_P       =0x04,
    MFX_TRELLIS_B       =0x08
};

typedef struct {
    mfxExtBuffer Header;

    mfxU16      reserved1;
    mfxU16      RateDistortionOpt;      /* tri-state option */
    mfxU16      MECostType;
    mfxU16      MESearchType;
    mfxI16Pair  MVSearchWindow;
    mfxU16      EndOfSequence;          /* tri-state option */
    mfxU16      FramePicture;           /* tri-state option */

    mfxU16      CAVLC;                  /* tri-state option */
    mfxU16      reserved2[2];
    mfxU16      RecoveryPointSEI;       /* tri-state option */
    mfxU16      ViewOutput;             /* tri-state option */
    mfxU16      NalHrdConformance;      /* tri-state option */
    mfxU16      SingleSeiNalUnit;       /* tri-state option */
    mfxU16      VuiVclHrdParameters;    /* tri-state option */

    mfxU16      RefPicListReordering;   /* tri-state option */
    mfxU16      ResetRefList;           /* tri-state option */
    mfxU16      RefPicMarkRep;          /* tri-state option */
    mfxU16      FieldOutput;            /* tri-state option */

    mfxU16      IntraPredBlockSize;
    mfxU16      InterPredBlockSize;
    mfxU16      MVPrecision;
    mfxU16      MaxDecFrameBuffering;

    mfxU16      AUDelimiter;            /* tri-state option */
    mfxU16      EndOfStream;            /* tri-state option */
    mfxU16      PicTimingSEI;           /* tri-state option */
    mfxU16      VuiNalHrdParameters;    /* tri-state option */
} mfxExtCodingOption;

enum {
    MFX_B_REF_UNKNOWN = 0,
    MFX_B_REF_OFF     = 1,
    MFX_B_REF_PYRAMID = 2
};

enum {
    MFX_LOOKAHEAD_DS_UNKNOWN = 0,
    MFX_LOOKAHEAD_DS_OFF     = 1,
    MFX_LOOKAHEAD_DS_2x      = 2,
    MFX_LOOKAHEAD_DS_4x      = 3
};

typedef struct {
    mfxExtBuffer Header;

    mfxU16      IntRefType;
    mfxU16      IntRefCycleSize;
    mfxI16      IntRefQPDelta;

    mfxU32      MaxFrameSize;
    mfxU32      MaxSliceSize;

    mfxU16      BitrateLimit;           /* tri-state option */
    mfxU16      MBBRC;                  /* tri-state option */
    mfxU16      ExtBRC;                 /* tri-state option */
    mfxU16      LookAheadDepth;
    mfxU16      Trellis;
    mfxU16      RepeatPPS;              /* tri-state option */
    mfxU16      BRefType;
    mfxU16      AdaptiveI;              /* tri-state option */
    mfxU16      AdaptiveB;              /* tri-state option */
    mfxU16      LookAheadDS;
    mfxU16      NumMbPerSlice;
    mfxU16      SkipFrame;
    mfxU8       MinQPI;                 /* 1..51, 0 = default */
    mfxU8       MaxQPI;                 /* 1..51, 0 = default */
    mfxU8       MinQPP;                 /* 1..51, 0 = default */
    mfxU8       MaxQPP;                 /* 1..51, 0 = default */
    mfxU8       MinQPB;                 /* 1..51, 0 = default */
    mfxU8       MaxQPB;                 /* 1..51, 0 = default */
    mfxU16      FixedFrameRate;         /* tri-state option */
    mfxU16      DisableDeblockingIdc;
    mfxU16      reserved2[4];
} mfxExtCodingOption2;

/* IntraPredBlockSize/InterPredBlockSize */
enum {
    MFX_BLOCKSIZE_UNKNOWN   = 0,
    MFX_BLOCKSIZE_MIN_16X16 = 1, /* 16x16              */
    MFX_BLOCKSIZE_MIN_8X8   = 2, /* 16x16, 8x8         */
    MFX_BLOCKSIZE_MIN_4X4   = 3  /* 16x16, 8x8, 4x4    */
};

/* MVPrecision */
enum {
    MFX_MVPRECISION_UNKNOWN    = 0,
    MFX_MVPRECISION_INTEGER    = (1 << 0),
    MFX_MVPRECISION_HALFPEL    = (1 << 1),
    MFX_MVPRECISION_QUARTERPEL = (1 << 2)
};

enum {
    MFX_CODINGOPTION_UNKNOWN    =0,
    MFX_CODINGOPTION_ON         =0x10,
    MFX_CODINGOPTION_OFF        =0x20,
    MFX_CODINGOPTION_ADAPTIVE   =0x30
};

/* Data Flag for mfxBitstream*/
enum {
    MFX_BITSTREAM_COMPLETE_FRAME    = 0x0001,        /* the bitstream contains a complete frame or field pair of data */
    MFX_BITSTREAM_EOS               = 0x0002
};

/* Extended Buffer Ids */
enum {
    MFX_EXTBUFF_CODING_OPTION       =   MFX_MAKEFOURCC('C','D','O','P'),
    MFX_EXTBUFF_CODING_OPTION_SPSPPS=   MFX_MAKEFOURCC('C','O','S','P'),
    MFX_EXTBUFF_VPP_DONOTUSE        =   MFX_MAKEFOURCC('N','U','S','E'),
    MFX_EXTBUFF_VPP_AUXDATA         =   MFX_MAKEFOURCC('A','U','X','D'),
    MFX_EXTBUFF_VPP_DENOISE         =   MFX_MAKEFOURCC('D','N','I','S'),
    MFX_EXTBUFF_VPP_SCENE_ANALYSIS  =   MFX_MAKEFOURCC('S','C','L','Y'),
    MFX_EXTBUFF_VPP_SCENE_CHANGE    =   MFX_EXTBUFF_VPP_SCENE_ANALYSIS,
    MFX_EXTBUFF_VPP_PROCAMP         =   MFX_MAKEFOURCC('P','A','M','P'),
    MFX_EXTBUFF_VPP_DETAIL          =   MFX_MAKEFOURCC('D','E','T',' '),
    MFX_EXTBUFF_VIDEO_SIGNAL_INFO   =   MFX_MAKEFOURCC('V','S','I','N'),
    MFX_EXTBUFF_VPP_DOUSE           =   MFX_MAKEFOURCC('D','U','S','E'),
    MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION  = MFX_MAKEFOURCC('O','P','Q','S'),
    MFX_EXTBUFF_AVC_REFLIST_CTRL       =   MFX_MAKEFOURCC('R','L','S','T'),
    MFX_EXTBUFF_VPP_FRAME_RATE_CONVERSION  = MFX_MAKEFOURCC('F','R','C',' '),
    MFX_EXTBUFF_PICTURE_TIMING_SEI         = MFX_MAKEFOURCC('P','T','S','E'),
    MFX_EXTBUFF_AVC_TEMPORAL_LAYERS        = MFX_MAKEFOURCC('A','T','M','L'),
    MFX_EXTBUFF_CODING_OPTION2             = MFX_MAKEFOURCC('C','D','O','2'),
    MFX_EXTBUFF_VPP_IMAGE_STABILIZATION    = MFX_MAKEFOURCC('I','S','T','B'),
    MFX_EXTBUFF_VPP_PICSTRUCT_DETECTION    = MFX_MAKEFOURCC('I','D','E','T'),
    MFX_EXTBUFF_ENCODER_CAPABILITY         = MFX_MAKEFOURCC('E','N','C','P'),
    MFX_EXTBUFF_ENCODER_RESET_OPTION       = MFX_MAKEFOURCC('E','N','R','O'),
    MFX_EXTBUFF_ENCODED_FRAME_INFO         = MFX_MAKEFOURCC('E','N','F','I'),
    MFX_EXTBUFF_VPP_COMPOSITE              = MFX_MAKEFOURCC('V','C','M','P'),
    MFX_EXTBUFF_VPP_VIDEO_SIGNAL_INFO      = MFX_MAKEFOURCC('V','V','S','I'),
    MFX_EXTBUFF_ENCODER_ROI                = MFX_MAKEFOURCC('E','R','O','I'),
    MFX_EXTBUFF_VPP_DEINTERLACING          = MFX_MAKEFOURCC('V','P','D','I'),
    MFX_EXTBUFF_AVC_REFLISTS               = MFX_MAKEFOURCC('R','L','T','S')
};

/* VPP Conf: Do not use certain algorithms  */
typedef struct {
    mfxExtBuffer    Header;
    mfxU32          NumAlg;
    mfxU32*         AlgList;
} mfxExtVPPDoNotUse;

typedef struct {
    mfxExtBuffer    Header;
    mfxU16  DenoiseFactor;
} mfxExtVPPDenoise;

typedef struct {
    mfxExtBuffer    Header;
    mfxU16  DetailFactor;
} mfxExtVPPDetail;

typedef struct {
    mfxExtBuffer    Header;
    mfxF64   Brightness;
    mfxF64   Contrast;
    mfxF64   Hue;
    mfxF64   Saturation;
} mfxExtVPPProcAmp;

/* statistics collected for decode, encode and vpp */
typedef struct {
    mfxU32  reserved[16];
    mfxU32  NumFrame;
    mfxU64  NumBit;
    mfxU32  NumCachedFrame;
} mfxEncodeStat;

typedef struct {
    mfxU32  reserved[16];
    mfxU32  NumFrame;
    mfxU32  NumSkippedFrame;
    mfxU32  NumError;
    mfxU32  NumCachedFrame;
} mfxDecodeStat;

typedef struct {
    mfxU32  reserved[16];
    mfxU32  NumFrame;
    mfxU32  NumCachedFrame;
} mfxVPPStat;

typedef struct {
    mfxExtBuffer    Header;

    union{
        struct{
            mfxU32  SpatialComplexity;
            mfxU32  TemporalComplexity;
        };
        struct{
            mfxU16  PicStruct;
            mfxU16  reserved[3];
        };
    };
    mfxU16          SceneChangeRate;
    mfxU16          RepeatedFrame;
} mfxExtVppAuxData;

typedef struct {
    mfxU32      reserved[4];
    mfxU8       *Data;      /* buffer pointer */
    mfxU32      NumBit;     /* number of bits */
    mfxU16      Type;       /* SEI message type in H.264 or user data start_code in MPEG-2 */
    mfxU16      BufSize;    /* payload buffer size in bytes */
} mfxPayload;

typedef struct {
    mfxExtBuffer    Header;
    mfxU32  reserved[5];
    mfxU16  SkipFrame;

    mfxU16  QP; /* per frame QP */

    mfxU16  FrameType;
    mfxU16  NumExtParam;
    mfxU16  NumPayload;     /* MPEG-2 user data or H.264 SEI message(s) */
    mfxU16  reserved2;

    mfxExtBuffer    **ExtParam;
    mfxPayload      **Payload;      /* for field pair, first field uses even payloads and second field uses odd payloads */
} mfxEncodeCtrl;

/* Buffer Memory Types */
enum {
    /* Buffer types */
    MFX_MEMTYPE_PERSISTENT_MEMORY   =0x0002
};

/* Frame Memory Types */
#define MFX_MEMTYPE_BASE(x) (0xf0ff & (x))

enum {
    MFX_MEMTYPE_DXVA2_DECODER_TARGET       =0x0010,
    MFX_MEMTYPE_DXVA2_PROCESSOR_TARGET     =0x0020,
    MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET   = MFX_MEMTYPE_DXVA2_DECODER_TARGET,
    MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET = MFX_MEMTYPE_DXVA2_PROCESSOR_TARGET,
    MFX_MEMTYPE_SYSTEM_MEMORY              =0x0040,
    MFX_MEMTYPE_RESERVED1                  =0x0080,

    MFX_MEMTYPE_FROM_ENCODE     = 0x0100,
    MFX_MEMTYPE_FROM_DECODE     = 0x0200,
    MFX_MEMTYPE_FROM_VPPIN      = 0x0400,
    MFX_MEMTYPE_FROM_VPPOUT     = 0x0800,

    MFX_MEMTYPE_INTERNAL_FRAME  = 0x0001,
    MFX_MEMTYPE_EXTERNAL_FRAME  = 0x0002,
    MFX_MEMTYPE_OPAQUE_FRAME    = 0x0004
};

typedef struct {
    mfxU32  reserved[4];
    mfxFrameInfo    Info;
    mfxU16  Type;   /* decoder or processor render targets */
    mfxU16  NumFrameMin;
    mfxU16  NumFrameSuggested;
    mfxU16  reserved2;
} mfxFrameAllocRequest;

typedef struct {
    mfxU32      reserved[4];
    mfxMemId    *mids;      /* the array allocated by application */
    mfxU16      NumFrameActual;
    mfxU16      reserved2;
} mfxFrameAllocResponse;

/* FrameType */
enum {
    MFX_FRAMETYPE_UNKNOWN       =0x0000,

    MFX_FRAMETYPE_I             =0x0001,
    MFX_FRAMETYPE_P             =0x0002,
    MFX_FRAMETYPE_B             =0x0004,
    MFX_FRAMETYPE_S             =0x0008,

    MFX_FRAMETYPE_REF           =0x0040,
    MFX_FRAMETYPE_IDR           =0x0080,

    MFX_FRAMETYPE_xI            =0x0100,
    MFX_FRAMETYPE_xP            =0x0200,
    MFX_FRAMETYPE_xB            =0x0400,
    MFX_FRAMETYPE_xS            =0x0800,

    MFX_FRAMETYPE_xREF          =0x4000,
    MFX_FRAMETYPE_xIDR          =0x8000
};

typedef enum {
    MFX_HANDLE_DIRECT3D_DEVICE_MANAGER9         =1,      /* IDirect3DDeviceManager9      */
    MFX_HANDLE_D3D9_DEVICE_MANAGER              = MFX_HANDLE_DIRECT3D_DEVICE_MANAGER9,
    MFX_HANDLE_RESERVED1                        = 2,
    MFX_HANDLE_D3D11_DEVICE                     = 3,
    MFX_HANDLE_VA_DISPLAY                       = 4,
    MFX_HANDLE_RESERVED3                        = 5
} mfxHandleType;

typedef enum {
    MFX_SKIPMODE_NOSKIP=0,
    MFX_SKIPMODE_MORE=1,
    MFX_SKIPMODE_LESS=2
} mfxSkipMode;

typedef struct {
    mfxExtBuffer    Header;
    mfxU8           *SPSBuffer;
    mfxU8           *PPSBuffer;
    mfxU16          SPSBufSize;
    mfxU16          PPSBufSize;
    mfxU16          SPSId;
    mfxU16          PPSId;
} mfxExtCodingOptionSPSPPS;

typedef struct {
    mfxExtBuffer    Header;
    mfxU16          VideoFormat;
    mfxU16          VideoFullRange;
    mfxU16          ColourDescriptionPresent;
    mfxU16          ColourPrimaries;
    mfxU16          TransferCharacteristics;
    mfxU16          MatrixCoefficients;
} mfxExtVideoSignalInfo;

typedef struct {
    mfxExtBuffer    Header;
    mfxU32          NumAlg;
    mfxU32          *AlgList;
} mfxExtVPPDoUse;

typedef struct {
    mfxExtBuffer    Header;
    mfxU32      reserved1[2];
    struct {
        mfxFrameSurface1 **Surfaces;
        mfxU32  reserved2[5];
        mfxU16  Type;
        mfxU16  NumSurface;
    } In, Out;
} mfxExtOpaqueSurfaceAlloc;

typedef struct {
    mfxExtBuffer    Header;
    mfxU16          NumRefIdxL0Active;
    mfxU16          NumRefIdxL1Active;

    struct {
        mfxU32      FrameOrder;
        mfxU16      PicStruct;
        mfxU16      ViewId;
        mfxU16      LongTermIdx;
        mfxU16      reserved[3];
    } PreferredRefList[32], RejectedRefList[16], LongTermRefList[16];

    mfxU16      ApplyLongTermIdx;
    mfxU16      reserved[15];
} mfxExtAVCRefListCtrl;

enum {
    MFX_FRCALGM_PRESERVE_TIMESTAMP    = 0x0001,
    MFX_FRCALGM_DISTRIBUTED_TIMESTAMP = 0x0002,
    MFX_FRCALGM_FRAME_INTERPOLATION   = 0x0004
};

typedef struct {
    mfxExtBuffer    Header;
    mfxU16      Algorithm;
    mfxU16      reserved;
    mfxU32      reserved2[15];
} mfxExtVPPFrameRateConversion;

enum {
    MFX_IMAGESTAB_MODE_UPSCALE = 0x0001,
    MFX_IMAGESTAB_MODE_BOXING  = 0x0002
};

typedef struct {
    mfxExtBuffer    Header;
    mfxU16  Mode;
    mfxU16  reserved[11];
} mfxExtVPPImageStab;

typedef struct {
  mfxExtBuffer    Header;
  mfxU32      reserved[14];

  struct {
      mfxU16    ClockTimestampFlag;
      mfxU16    CtType;
      mfxU16    NuitFieldBasedFlag;
      mfxU16    CountingType;
      mfxU16    FullTimestampFlag;
      mfxU16    DiscontinuityFlag;
      mfxU16    CntDroppedFlag;
      mfxU16    NFrames;
      mfxU16    SecondsFlag;
      mfxU16    MinutesFlag;
      mfxU16    HoursFlag;
      mfxU16    SecondsValue;
      mfxU16    MinutesValue;
      mfxU16    HoursValue;
      mfxU32    TimeOffset;
  } TimeStamp[3];
} mfxExtPictureTimingSEI;

typedef struct {
    mfxExtBuffer    Header;
    mfxU32          reserved1[4];
    mfxU16          reserved2;
    mfxU16          BaseLayerPID;

    struct {
        mfxU16 Scale;
        mfxU16 reserved[3];
    }Layer[8];
} mfxExtAvcTemporalLayers;

typedef struct {
    mfxExtBuffer Header;

    mfxU32      MBPerSec;
    mfxU16      reserved[58];
} mfxExtEncoderCapability;

typedef struct {
    mfxExtBuffer Header;

    mfxU16      StartNewSequence;
    mfxU16      reserved[11];
} mfxExtEncoderResetOption;

typedef struct {
    mfxExtBuffer    Header; 

    mfxU32          FrameOrder;
    mfxU16          PicStruct;
    mfxU16          LongTermIdx;
    mfxU32          MAD;
    mfxU16          BRCPanicMode;
    mfxU16          QP;
    mfxU32          SecondFieldOffset;
    mfxU16          reserved[2];

    struct {
            mfxU32      FrameOrder;
            mfxU16      PicStruct;
            mfxU16      LongTermIdx;
            mfxU16      reserved[4];
    } UsedRefListL0[32], UsedRefListL1[32];
} mfxExtAVCEncodedFrameInfo;

typedef struct mfxVPPCompInputStream {
        mfxU32  DstX;
        mfxU32  DstY;
        mfxU32  DstW;
        mfxU32  DstH;

        mfxU16  LumaKeyEnable;
        mfxU16  LumaKeyMin;
        mfxU16  LumaKeyMax;

        mfxU16  GlobalAlphaEnable;
        mfxU16  GlobalAlpha;

        mfxU16 PixelAlphaEnable;

        mfxU16  reserved2[18];
} mfxVPPCompInputStream;     

typedef struct {
    mfxExtBuffer    Header;

    /* background color*/
    union {
        mfxU16   Y;
        mfxU16   R;
    };
    union {
        mfxU16   U;
        mfxU16   G;
    };
    union {
        mfxU16   V;
        mfxU16   B;
    };

    mfxU16      reserved1[24];

    mfxU16      NumInputStream;
    mfxVPPCompInputStream *InputStream;     
} mfxExtVPPComposite;

/* TransferMatrix */
enum {
    MFX_TRANSFERMATRIX_UNKNOWN = 0,
    MFX_TRANSFERMATRIX_BT709   = 1,
    MFX_TRANSFERMATRIX_BT601   = 2
};

/* NominalRange */
enum {
    MFX_NOMINALRANGE_UNKNOWN   = 0,
    MFX_NOMINALRANGE_0_255     = 1,
    MFX_NOMINALRANGE_16_235    = 2
};

typedef struct {
    mfxExtBuffer    Header;
    mfxU16          reserved1[4];

    struct  {
        mfxU16  TransferMatrix;
        mfxU16  NominalRange;
        mfxU16  reserved2[6];
    } In, Out;
} mfxExtVPPVideoSignalInfo;

typedef struct {
    mfxExtBuffer    Header;

    mfxU16  NumROI;
    mfxU16  reserved1[11];

    struct  {
        mfxU32  Left;
        mfxU32  Top;
        mfxU32  Right;
        mfxU32  Bottom;

        mfxI16  Priority;
        mfxU16  reserved2[7];
    } ROI[256];
} mfxExtEncoderROI;

enum {
    MFX_DEINTERLACING_BOB      = 0x0001,
    MFX_DEINTERLACING_ADVANCED = 0x0002
};

typedef struct {
    mfxExtBuffer    Header;
    mfxU16  Mode;
    mfxU16  reserved[11];
} mfxExtVPPDeinterlacing;

typedef struct {
    mfxExtBuffer    Header;
    mfxU16          NumRefIdxL0Active;
    mfxU16          NumRefIdxL1Active;
    mfxU16          reserved[2];

    struct mfxRefPic{
        mfxU32      FrameOrder;
        mfxU16      PicStruct;
        mfxU16      reserved[5];
    } RefPicList0[32], RefPicList1[32];

}mfxExtAVCRefLists;

#ifdef __cplusplus
} // extern "C"
#endif

#endif

