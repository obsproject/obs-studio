// Copyright (c) 2018-2020 Intel Corporation
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
#ifndef __MFXSTRUCTURES_H__
#define __MFXSTRUCTURES_H__
#include "mfxcommon.h"

#if !defined (__GNUC__)
#pragma warning(disable: 4201)
#endif

#ifdef __cplusplus
extern "C" {
#endif


/* Frame ID for SVC and MVC */
MFX_PACK_BEGIN_USUAL_STRUCT()
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
MFX_PACK_END()

/* This struct has 4-byte alignment for binary compatibility with previously released versions of API */
MFX_PACK_BEGIN_USUAL_STRUCT()
/* Frame Info */
typedef struct {
    mfxU32  reserved[4];
    mfxU16  reserved4;
    mfxU16  BitDepthLuma;
    mfxU16  BitDepthChroma;
    mfxU16  Shift;

    mfxFrameId FrameId;

    mfxU32  FourCC;
    union {
        struct { /* Frame parameters */
            mfxU16  Width;
            mfxU16  Height;

            mfxU16  CropX;
            mfxU16  CropY;
            mfxU16  CropW;
            mfxU16  CropH;
        };
        struct { /* Buffer parameters (for plain formats like P8) */
            mfxU64 BufferSize;
            mfxU32 reserved5;
        };
    };

    mfxU32  FrameRateExtN;
    mfxU32  FrameRateExtD;
    mfxU16  reserved3;

    mfxU16  AspectRatioW;
    mfxU16  AspectRatioH;

    mfxU16  PicStruct;
    mfxU16  ChromaFormat;
    mfxU16  reserved2;
} mfxFrameInfo;
MFX_PACK_END()

/* FourCC */
enum {
    MFX_FOURCC_NV12         = MFX_MAKEFOURCC('N','V','1','2'),   /* Native Format */
    MFX_FOURCC_YV12         = MFX_MAKEFOURCC('Y','V','1','2'),
    MFX_FOURCC_NV16         = MFX_MAKEFOURCC('N','V','1','6'),
    MFX_FOURCC_YUY2         = MFX_MAKEFOURCC('Y','U','Y','2'),
#if (MFX_VERSION >= 1028)
    MFX_FOURCC_RGB565       = MFX_MAKEFOURCC('R','G','B','2'),   /* 2 bytes per pixel, uint16 in little-endian format, where 0-4 bits are blue, bits 5-10 are green and bits 11-15 are red */
    MFX_FOURCC_RGBP         = MFX_MAKEFOURCC('R','G','B','P'),
#endif
    MFX_FOURCC_RGB3         = MFX_MAKEFOURCC('R','G','B','3'),   /* deprecated */
    MFX_FOURCC_RGB4         = MFX_MAKEFOURCC('R','G','B','4'),   /* ARGB in that order, A channel is 8 MSBs */
    MFX_FOURCC_P8           = 41,                                /*  D3DFMT_P8   */
    MFX_FOURCC_P8_TEXTURE   = MFX_MAKEFOURCC('P','8','M','B'),
    MFX_FOURCC_P010         = MFX_MAKEFOURCC('P','0','1','0'),
#if (MFX_VERSION >= 1031)
    MFX_FOURCC_P016         = MFX_MAKEFOURCC('P','0','1','6'),
#endif
    MFX_FOURCC_P210         = MFX_MAKEFOURCC('P','2','1','0'),
    MFX_FOURCC_BGR4         = MFX_MAKEFOURCC('B','G','R','4'),   /* ABGR in that order, A channel is 8 MSBs */
    MFX_FOURCC_A2RGB10      = MFX_MAKEFOURCC('R','G','1','0'),   /* ARGB in that order, A channel is two MSBs */
    MFX_FOURCC_ARGB16       = MFX_MAKEFOURCC('R','G','1','6'),   /* ARGB in that order, 64 bits, A channel is 16 MSBs */
    MFX_FOURCC_ABGR16       = MFX_MAKEFOURCC('B','G','1','6'),   /* ABGR in that order, 64 bits, A channel is 16 MSBs */
    MFX_FOURCC_R16          = MFX_MAKEFOURCC('R','1','6','U'),
    MFX_FOURCC_AYUV         = MFX_MAKEFOURCC('A','Y','U','V'),   /* YUV 4:4:4, AYUV in that order, A channel is 8 MSBs */
    MFX_FOURCC_AYUV_RGB4    = MFX_MAKEFOURCC('A','V','U','Y'),   /* ARGB in that order, A channel is 8 MSBs stored in AYUV surface*/
    MFX_FOURCC_UYVY         = MFX_MAKEFOURCC('U','Y','V','Y'),
#if (MFX_VERSION >= 1027)
    MFX_FOURCC_Y210         = MFX_MAKEFOURCC('Y','2','1','0'),
    MFX_FOURCC_Y410         = MFX_MAKEFOURCC('Y','4','1','0'),
#endif
#if (MFX_VERSION >= 1031)
    MFX_FOURCC_Y216         = MFX_MAKEFOURCC('Y','2','1','6'),
    MFX_FOURCC_Y416         = MFX_MAKEFOURCC('Y','4','1','6'),
#endif
    MFX_FOURCC_NV21         = MFX_MAKEFOURCC('N', 'V', '2', '1'), /* Same as NV12 but with weaved V and U values. */
    MFX_FOURCC_IYUV         = MFX_MAKEFOURCC('I', 'Y', 'U', 'V'), /* Same as  YV12 except that the U and V plane order is reversed. */
    MFX_FOURCC_I010         = MFX_MAKEFOURCC('I', '0', '1', '0'), /* 10-bit YUV 4:2:0, each component has its own plane. */
};

/* PicStruct */
enum {
    MFX_PICSTRUCT_UNKNOWN       =0x00,
    MFX_PICSTRUCT_PROGRESSIVE   =0x01,
    MFX_PICSTRUCT_FIELD_TFF     =0x02,
    MFX_PICSTRUCT_FIELD_BFF     =0x04,

    MFX_PICSTRUCT_FIELD_REPEATED=0x10,  /* first field repeated, pic_struct=5 or 6 in H.264 */
    MFX_PICSTRUCT_FRAME_DOUBLING=0x20,  /* pic_struct=7 in H.264 */
    MFX_PICSTRUCT_FRAME_TRIPLING=0x40,  /* pic_struct=8 in H.264 */

    MFX_PICSTRUCT_FIELD_SINGLE      =0x100,
    MFX_PICSTRUCT_FIELD_TOP         =MFX_PICSTRUCT_FIELD_SINGLE | MFX_PICSTRUCT_FIELD_TFF,
    MFX_PICSTRUCT_FIELD_BOTTOM      =MFX_PICSTRUCT_FIELD_SINGLE | MFX_PICSTRUCT_FIELD_BFF,
    MFX_PICSTRUCT_FIELD_PAIRED_PREV =0x200,
    MFX_PICSTRUCT_FIELD_PAIRED_NEXT =0x400,
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
    MFX_CHROMAFORMAT_YUV422V    = 5,
    MFX_CHROMAFORMAT_RESERVED1  = 6
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

#if (MFX_VERSION >= 1027)
MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct
{
    mfxU32 U : 10;
    mfxU32 Y : 10;
    mfxU32 V : 10;
    mfxU32 A :  2;
} mfxY410;
MFX_PACK_END()
#endif

#if (MFX_VERSION >= 1025)
MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct
{
    mfxU32 B : 10;
    mfxU32 G : 10;
    mfxU32 R : 10;
    mfxU32 A :  2;
} mfxA2RGB10;
MFX_PACK_END()
#endif

/* Frame Data Info */
MFX_PACK_BEGIN_STRUCT_W_L_TYPE()
typedef struct {
    union {
        mfxExtBuffer **ExtParam;
        mfxU64       reserved2;
    };
    mfxU16  NumExtParam;

    mfxU16      reserved[9];
    mfxU16      MemType;
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
#if (MFX_VERSION >= 1027)
        mfxY410 *Y410;          /* for Y410 format (merged AVYU) */
#endif
    };
    union {
        mfxU8   *Cr;
        mfxU8   *V;
        mfxU16  *V16;
        mfxU8   *B;
#if (MFX_VERSION >= 1025)
        mfxA2RGB10 *A2RGB10;    /* for A2RGB10 format (merged ARGB) */
#endif
    };
    mfxU8       *A;
    mfxMemId    MemId;

    /* Additional Flags */
    mfxU16  Corrupted;
    mfxU16  DataFlag;
} mfxFrameData;
MFX_PACK_END()

/* Frame Surface */
MFX_PACK_BEGIN_STRUCT_W_L_TYPE()
typedef struct {
    mfxU32  reserved[4];
    mfxFrameInfo    Info;
    mfxFrameData    Data;
} mfxFrameSurface1;
MFX_PACK_END()

enum {
    MFX_TIMESTAMPCALC_UNKNOWN = 0,
    MFX_TIMESTAMPCALC_TELECINE = 1,
};

/* Transcoding Info */
MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxU32  reserved[7];

    mfxU16  LowPower;
    mfxU16  BRCParamMultiplier;

    mfxFrameInfo    FrameInfo;
    mfxU32  CodecId;
    mfxU16  CodecProfile;
    mfxU16  CodecLevel;
    mfxU16  NumThread;

    union {
        struct {   /* Encoding Options */
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
        struct {   /* Decoding Options */
            mfxU16  DecodedOrder;
            mfxU16  ExtendedPicStruct;
            mfxU16  TimeStampCalc;
            mfxU16  SliceGroupsPresent;
            mfxU16  MaxDecFrameBuffering;
            mfxU16  EnableReallocRequest;
#if (MFX_VERSION >= 1034)
            mfxU16  FilmGrain;
            mfxU16  IgnoreLevelConstrain;
            mfxU16  reserved2[5];
#else
            mfxU16  reserved2[7];
#endif
        };
        struct {   /* JPEG Decoding Options */
            mfxU16  JPEGChromaFormat;
            mfxU16  Rotation;
            mfxU16  JPEGColorFormat;
            mfxU16  InterleavedDec;
            mfxU8   SamplingFactorH[4];
            mfxU8   SamplingFactorV[4];
            mfxU16  reserved3[5];
        };
        struct {   /* JPEG Encoding Options */
            mfxU16  Interleaved;
            mfxU16  Quality;
            mfxU16  RestartInterval;
            mfxU16  reserved5[10];
        };
    };
} mfxInfoMFX;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxU32  reserved[8];
    mfxFrameInfo    In;
    mfxFrameInfo    Out;
} mfxInfoVPP;
MFX_PACK_END()

MFX_PACK_BEGIN_STRUCT_W_PTR()
typedef struct {
    mfxU32  AllocId;
    mfxU32  reserved[2];
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
MFX_PACK_END()

/* IOPattern */
enum {
    MFX_IOPATTERN_IN_VIDEO_MEMORY   = 0x01,
    MFX_IOPATTERN_IN_SYSTEM_MEMORY  = 0x02,
    MFX_DEPRECATED_ENUM_FIELD_INSIDE(MFX_IOPATTERN_IN_OPAQUE_MEMORY)  = 0x04,
    MFX_IOPATTERN_OUT_VIDEO_MEMORY  = 0x10,
    MFX_IOPATTERN_OUT_SYSTEM_MEMORY = 0x20,
    MFX_DEPRECATED_ENUM_FIELD_INSIDE(MFX_IOPATTERN_OUT_OPAQUE_MEMORY) = 0x40
};

MFX_DEPRECATED_ENUM_FIELD_OUTSIDE(MFX_IOPATTERN_IN_OPAQUE_MEMORY);
MFX_DEPRECATED_ENUM_FIELD_OUTSIDE(MFX_IOPATTERN_OUT_OPAQUE_MEMORY);

/* CodecId */
enum {
    MFX_CODEC_AVC         =MFX_MAKEFOURCC('A','V','C',' '),
    MFX_CODEC_HEVC        =MFX_MAKEFOURCC('H','E','V','C'),
    MFX_CODEC_MPEG2       =MFX_MAKEFOURCC('M','P','G','2'),
    MFX_CODEC_VC1         =MFX_MAKEFOURCC('V','C','1',' '),
    MFX_CODEC_CAPTURE     =MFX_MAKEFOURCC('C','A','P','T'),
    MFX_CODEC_VP9         =MFX_MAKEFOURCC('V','P','9',' '),
    MFX_CODEC_AV1         =MFX_MAKEFOURCC('A','V','1',' ')
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
    MFX_PROFILE_AVC_HIGH10                  =110,
    MFX_PROFILE_AVC_HIGH_422                =122,
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
#if (MFX_VERSION >= 1035)
    MFX_LEVEL_AVC_6                         =60,
    MFX_LEVEL_AVC_61                        =61,
    MFX_LEVEL_AVC_62                        =62,
#endif

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
    MFX_PROFILE_HEVC_REXT             =4,
#if (MFX_VERSION >= 1032)
    MFX_PROFILE_HEVC_SCC              =9,
#endif

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

    /* VP9 Profiles */
    MFX_PROFILE_VP9_0                       = 1,
    MFX_PROFILE_VP9_1                       = 2,
    MFX_PROFILE_VP9_2                       = 3,
    MFX_PROFILE_VP9_3                       = 4,

#if (MFX_VERSION >= 1034)
    /* AV1 Profiles */
    MFX_PROFILE_AV1_MAIN                    = 1,
    MFX_PROFILE_AV1_HIGH                    = 2,
    MFX_PROFILE_AV1_PRO                     = 3,

    MFX_LEVEL_AV1_2                         = 20,
    MFX_LEVEL_AV1_21                        = 21,
    MFX_LEVEL_AV1_22                        = 22,
    MFX_LEVEL_AV1_23                        = 23,
    MFX_LEVEL_AV1_3                         = 30,
    MFX_LEVEL_AV1_31                        = 31,
    MFX_LEVEL_AV1_32                        = 32,
    MFX_LEVEL_AV1_33                        = 33,
    MFX_LEVEL_AV1_4                         = 40,
    MFX_LEVEL_AV1_41                        = 41,
    MFX_LEVEL_AV1_42                        = 42,
    MFX_LEVEL_AV1_43                        = 43,
    MFX_LEVEL_AV1_5                         = 50,
    MFX_LEVEL_AV1_51                        = 51,
    MFX_LEVEL_AV1_52                        = 52,
    MFX_LEVEL_AV1_53                        = 53,
    MFX_LEVEL_AV1_6                         = 60,
    MFX_LEVEL_AV1_61                        = 61,
    MFX_LEVEL_AV1_62                        = 62,
    MFX_LEVEL_AV1_63                        = 63,
#endif
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
    MFX_RATECONTROL_CBR       =1,
    MFX_RATECONTROL_VBR       =2,
    MFX_RATECONTROL_CQP       =3,
    MFX_RATECONTROL_AVBR      =4,
    MFX_RATECONTROL_RESERVED1 =5,
    MFX_RATECONTROL_RESERVED2 =6,
    MFX_RATECONTROL_RESERVED3 =100,
    MFX_RATECONTROL_RESERVED4 =7,
    MFX_RATECONTROL_LA        =8,
    MFX_RATECONTROL_ICQ       =9,
    MFX_RATECONTROL_VCM       =10,
    MFX_RATECONTROL_LA_ICQ    =11,
    MFX_RATECONTROL_LA_EXT    =12,
    MFX_RATECONTROL_LA_HRD    =13,
    MFX_RATECONTROL_QVBR      =14,
};

/* Trellis control*/
enum {
    MFX_TRELLIS_UNKNOWN =0,
    MFX_TRELLIS_OFF     =0x01,
    MFX_TRELLIS_I       =0x02,
    MFX_TRELLIS_P       =0x04,
    MFX_TRELLIS_B       =0x08
};

MFX_PACK_BEGIN_USUAL_STRUCT()
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
MFX_PACK_END()

enum {
    MFX_B_REF_UNKNOWN = 0,
    MFX_B_REF_OFF     = 1,
    MFX_B_REF_PYRAMID = 2
};

enum {
    MFX_DEPRECATED_ENUM_FIELD_INSIDE(MFX_LOOKAHEAD_DS_UNKNOWN) = 0,
    MFX_DEPRECATED_ENUM_FIELD_INSIDE(MFX_LOOKAHEAD_DS_OFF)     = 1,
    MFX_DEPRECATED_ENUM_FIELD_INSIDE(MFX_LOOKAHEAD_DS_2x)      = 2,
    MFX_DEPRECATED_ENUM_FIELD_INSIDE(MFX_LOOKAHEAD_DS_4x)      = 3
};

MFX_DEPRECATED_ENUM_FIELD_OUTSIDE(MFX_LOOKAHEAD_DS_UNKNOWN);
MFX_DEPRECATED_ENUM_FIELD_OUTSIDE(MFX_LOOKAHEAD_DS_OFF);
MFX_DEPRECATED_ENUM_FIELD_OUTSIDE(MFX_LOOKAHEAD_DS_2x);
MFX_DEPRECATED_ENUM_FIELD_OUTSIDE(MFX_LOOKAHEAD_DS_4x);

enum {
    MFX_BPSEI_DEFAULT = 0x00,
    MFX_BPSEI_IFRAME  = 0x01
};

enum {
    MFX_SKIPFRAME_NO_SKIP         = 0,
    MFX_SKIPFRAME_INSERT_DUMMY    = 1,
    MFX_SKIPFRAME_INSERT_NOTHING  = 2,
    MFX_SKIPFRAME_BRC_ONLY        = 3,
};

/* Intra refresh types */
enum {
        MFX_REFRESH_NO             = 0,
        MFX_REFRESH_VERTICAL       = 1,
        MFX_REFRESH_HORIZONTAL     = 2,
        MFX_REFRESH_SLICE          = 3
};

MFX_PACK_BEGIN_USUAL_STRUCT()
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
    mfxU16      DisableVUI;
    mfxU16      BufferingPeriodSEI;
    mfxU16      EnableMAD;              /* tri-state option */
    mfxU16      UseRawRef;              /* tri-state option */
} mfxExtCodingOption2;
MFX_PACK_END()

/* WeightedPred */
enum {
    MFX_WEIGHTED_PRED_UNKNOWN  = 0,
    MFX_WEIGHTED_PRED_DEFAULT  = 1,
    MFX_WEIGHTED_PRED_EXPLICIT = 2,
    MFX_WEIGHTED_PRED_IMPLICIT = 3
};

/* ScenarioInfo */
enum {
    MFX_SCENARIO_UNKNOWN             = 0,
    MFX_SCENARIO_DISPLAY_REMOTING    = 1,
    MFX_SCENARIO_VIDEO_CONFERENCE    = 2,
    MFX_SCENARIO_ARCHIVE             = 3,
    MFX_SCENARIO_LIVE_STREAMING      = 4,
    MFX_SCENARIO_CAMERA_CAPTURE      = 5,
    MFX_SCENARIO_VIDEO_SURVEILLANCE  = 6,
    MFX_SCENARIO_GAME_STREAMING      = 7,
    MFX_SCENARIO_REMOTE_GAMING       = 8
};

/* ContentInfo */
enum {
    MFX_CONTENT_UNKNOWN              = 0,
    MFX_CONTENT_FULL_SCREEN_VIDEO    = 1,
    MFX_CONTENT_NON_VIDEO_SCREEN     = 2
};

/* PRefType */
enum {
    MFX_P_REF_DEFAULT = 0,
    MFX_P_REF_SIMPLE  = 1,
    MFX_P_REF_PYRAMID = 2
};

#if (MFX_VERSION >= MFX_VERSION_NEXT)

/* QuantScaleType */
enum {
    MFX_MPEG2_QUANT_SCALE_TYPE_DEFAULT    = 0,
    MFX_MPEG2_QUANT_SCALE_TYPE_LINEAR     = 1, /* q_scale_type = 0 */
    MFX_MPEG2_QUANT_SCALE_TYPE_NONLINEAR  = 2  /* q_scale_type = 1 */
};

/* IntraVLCFormat */
enum {
    MFX_MPEG2_INTRA_VLC_FORMAT_DEFAULT    = 0,
    MFX_MPEG2_INTRA_VLC_FORMAT_B14        = 1, /* use table B.14 */
    MFX_MPEG2_INTRA_VLC_FORMAT_B15        = 2  /* use table B.15 */
};

/* ScanType */
enum {
    MFX_MPEG2_SCAN_TYPE_DEFAULT   = 0,
    MFX_MPEG2_SCAN_TYPE_ZIGZAG    = 1, /* alternate_scan = 0 */
    MFX_MPEG2_SCAN_TYPE_ALTERNATE = 2  /* alternate_scan = 1 */
};

#endif

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer Header;

    mfxU16      NumSliceI;
    mfxU16      NumSliceP;
    mfxU16      NumSliceB;

    mfxU16      WinBRCMaxAvgKbps;
    mfxU16      WinBRCSize;

    mfxU16      QVBRQuality;
    mfxU16      EnableMBQP;
    mfxU16      IntRefCycleDist;
    mfxU16      DirectBiasAdjustment;          /* tri-state option */
    mfxU16      GlobalMotionBiasAdjustment;    /* tri-state option */
    mfxU16      MVCostScalingFactor;
    mfxU16      MBDisableSkipMap;              /* tri-state option */

    mfxU16      WeightedPred;
    mfxU16      WeightedBiPred;

    mfxU16      AspectRatioInfoPresent;         /* tri-state option */
    mfxU16      OverscanInfoPresent;            /* tri-state option */
    mfxU16      OverscanAppropriate;            /* tri-state option */
    mfxU16      TimingInfoPresent;              /* tri-state option */
    mfxU16      BitstreamRestriction;           /* tri-state option */
    mfxU16      LowDelayHrd;                    /* tri-state option */
    mfxU16      MotionVectorsOverPicBoundaries; /* tri-state option */
#if (MFX_VERSION >= MFX_VERSION_NEXT)
    mfxU16      Log2MaxMvLengthHorizontal;      /* 0..16 */
    mfxU16      Log2MaxMvLengthVertical;        /* 0..16 */
#else
    mfxU16      reserved1[2];
#endif

    mfxU16      ScenarioInfo;
    mfxU16      ContentInfo;

    mfxU16      PRefType;
    mfxU16      FadeDetection;            /* tri-state option */
#if (MFX_VERSION >= MFX_VERSION_NEXT)
    mfxI16      DeblockingAlphaTcOffset;  /* -12..12 (slice_alpha_c0_offset_div2 << 1) */
    mfxI16      DeblockingBetaOffset;     /* -12..12 (slice_beta_offset_div2 << 1) */
#else
    mfxU16      reserved2[2];
#endif
    mfxU16      GPB;                      /* tri-state option */

    mfxU32      MaxFrameSizeI;
    mfxU32      MaxFrameSizeP;
    mfxU32      reserved3[3];

    mfxU16      EnableQPOffset;           /* tri-state option */
    mfxI16      QPOffset[8];              /* FrameQP = QPX + QPOffset[pyramid_layer]; QPX = QPB for B-pyramid, QPP for P-pyramid */

    mfxU16      NumRefActiveP[8];
    mfxU16      NumRefActiveBL0[8];
    mfxU16      NumRefActiveBL1[8];

#if (MFX_VERSION >= MFX_VERSION_NEXT)
    mfxU16      ConstrainedIntraPredFlag;  /* tri-state option */
#else
    mfxU16      reserved6;
#endif
#if (MFX_VERSION >= 1026)
    mfxU16      TransformSkip;  /* tri-state option; HEVC transform_skip_enabled_flag */
#else
    mfxU16      reserved7;
#endif
#if (MFX_VERSION >= 1027)
    mfxU16      TargetChromaFormatPlus1;   /* Minus 1 specifies target encoding chroma format (see ColorFormat enum). May differ from input one. */
    mfxU16      TargetBitDepthLuma;        /* Target encoding bit depth for luma samples. May differ from input one. */
    mfxU16      TargetBitDepthChroma;      /* Target encoding bit depth for chroma samples. May differ from input one. */
#else
    mfxU16      reserved4[3];
#endif
    mfxU16      BRCPanicMode;              /* tri-state option */

    mfxU16      LowDelayBRC;               /* tri-state option */
    mfxU16      EnableMBForceIntra;        /* tri-state option */
    mfxU16      AdaptiveMaxFrameSize;      /* tri-state option */

    mfxU16      RepartitionCheckEnable;    /* tri-state option */
#if (MFX_VERSION >= MFX_VERSION_NEXT)
    mfxU16      QuantScaleType;            /* For MPEG2 specifies mapping between quantiser_scale_code and quantiser_scale (see QuantScaleType enum) */
    mfxU16      IntraVLCFormat;            /* For MPEG2 specifies which table shall be used for coding of DCT coefficients of intra macroblocks (see IntraVLCFormat enum) */
    mfxU16      ScanType;                  /* For MPEG2 specifies transform coefficients scan pattern (see ScanType enum) */
#else
    mfxU16      reserved5[3];
#endif
#if (MFX_VERSION >= 1025)
    mfxU16      EncodedUnitsInfo;          /* tri-state option */
    mfxU16      EnableNalUnitType;         /* tri-state option */
#else
    mfxU16      reserved8[2];
#endif
#if (MFX_VERSION >= 1026)
    mfxU16      ExtBrcAdaptiveLTR;         /* tri-state option for ExtBRC */
#else
    mfxU16      reserved9;
#endif
    mfxU16      reserved[163];
} mfxExtCodingOption3;
MFX_PACK_END()

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
    MFX_EXTBUFF_CODING_OPTION                   = MFX_MAKEFOURCC('C','D','O','P'),
    MFX_EXTBUFF_CODING_OPTION_SPSPPS            = MFX_MAKEFOURCC('C','O','S','P'),
    MFX_EXTBUFF_VPP_DONOTUSE                    = MFX_MAKEFOURCC('N','U','S','E'),
    MFX_EXTBUFF_VPP_AUXDATA                     = MFX_MAKEFOURCC('A','U','X','D'),
    MFX_EXTBUFF_VPP_DENOISE                     = MFX_MAKEFOURCC('D','N','I','S'),
    MFX_EXTBUFF_VPP_SCENE_ANALYSIS              = MFX_MAKEFOURCC('S','C','L','Y'),
    MFX_EXTBUFF_VPP_SCENE_CHANGE                = MFX_EXTBUFF_VPP_SCENE_ANALYSIS,
    MFX_EXTBUFF_VPP_PROCAMP                     = MFX_MAKEFOURCC('P','A','M','P'),
    MFX_EXTBUFF_VPP_DETAIL                      = MFX_MAKEFOURCC('D','E','T',' '),
    MFX_EXTBUFF_VIDEO_SIGNAL_INFO               = MFX_MAKEFOURCC('V','S','I','N'),
    MFX_EXTBUFF_VPP_DOUSE                       = MFX_MAKEFOURCC('D','U','S','E'),
    MFX_DEPRECATED_ENUM_FIELD_INSIDE(MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION)       = MFX_MAKEFOURCC('O','P','Q','S'),
    MFX_EXTBUFF_AVC_REFLIST_CTRL                = MFX_MAKEFOURCC('R','L','S','T'),
    MFX_EXTBUFF_VPP_FRAME_RATE_CONVERSION       = MFX_MAKEFOURCC('F','R','C',' '),
    MFX_EXTBUFF_PICTURE_TIMING_SEI              = MFX_MAKEFOURCC('P','T','S','E'),
    MFX_EXTBUFF_AVC_TEMPORAL_LAYERS             = MFX_MAKEFOURCC('A','T','M','L'),
    MFX_EXTBUFF_CODING_OPTION2                  = MFX_MAKEFOURCC('C','D','O','2'),
    MFX_EXTBUFF_VPP_IMAGE_STABILIZATION         = MFX_MAKEFOURCC('I','S','T','B'),
    MFX_EXTBUFF_VPP_PICSTRUCT_DETECTION         = MFX_MAKEFOURCC('I','D','E','T'),
    MFX_EXTBUFF_ENCODER_CAPABILITY              = MFX_MAKEFOURCC('E','N','C','P'),
    MFX_EXTBUFF_ENCODER_RESET_OPTION            = MFX_MAKEFOURCC('E','N','R','O'),
    MFX_EXTBUFF_ENCODED_FRAME_INFO              = MFX_MAKEFOURCC('E','N','F','I'),
    MFX_EXTBUFF_VPP_COMPOSITE                   = MFX_MAKEFOURCC('V','C','M','P'),
    MFX_EXTBUFF_VPP_VIDEO_SIGNAL_INFO           = MFX_MAKEFOURCC('V','V','S','I'),
    MFX_EXTBUFF_ENCODER_ROI                     = MFX_MAKEFOURCC('E','R','O','I'),
    MFX_EXTBUFF_VPP_DEINTERLACING               = MFX_MAKEFOURCC('V','P','D','I'),
    MFX_EXTBUFF_AVC_REFLISTS                    = MFX_MAKEFOURCC('R','L','T','S'),
    MFX_EXTBUFF_DEC_VIDEO_PROCESSING            = MFX_MAKEFOURCC('D','E','C','V'),
    MFX_EXTBUFF_VPP_FIELD_PROCESSING            = MFX_MAKEFOURCC('F','P','R','O'),
    MFX_EXTBUFF_CODING_OPTION3                  = MFX_MAKEFOURCC('C','D','O','3'),
    MFX_EXTBUFF_CHROMA_LOC_INFO                 = MFX_MAKEFOURCC('C','L','I','N'),
    MFX_EXTBUFF_MBQP                            = MFX_MAKEFOURCC('M','B','Q','P'),
    MFX_EXTBUFF_MB_FORCE_INTRA                  = MFX_MAKEFOURCC('M','B','F','I'),
    MFX_EXTBUFF_HEVC_TILES                      = MFX_MAKEFOURCC('2','6','5','T'),
    MFX_EXTBUFF_MB_DISABLE_SKIP_MAP             = MFX_MAKEFOURCC('M','D','S','M'),
    MFX_EXTBUFF_HEVC_PARAM                      = MFX_MAKEFOURCC('2','6','5','P'),
    MFX_EXTBUFF_DECODED_FRAME_INFO              = MFX_MAKEFOURCC('D','E','F','I'),
    MFX_EXTBUFF_TIME_CODE                       = MFX_MAKEFOURCC('T','M','C','D'),
    MFX_EXTBUFF_HEVC_REGION                     = MFX_MAKEFOURCC('2','6','5','R'),
    MFX_EXTBUFF_PRED_WEIGHT_TABLE               = MFX_MAKEFOURCC('E','P','W','T'),
    MFX_EXTBUFF_DIRTY_RECTANGLES                = MFX_MAKEFOURCC('D','R','O','I'),
    MFX_EXTBUFF_MOVING_RECTANGLES               = MFX_MAKEFOURCC('M','R','O','I'),
    MFX_EXTBUFF_CODING_OPTION_VPS               = MFX_MAKEFOURCC('C','O','V','P'),
    MFX_EXTBUFF_VPP_ROTATION                    = MFX_MAKEFOURCC('R','O','T',' '),
    MFX_EXTBUFF_ENCODED_SLICES_INFO             = MFX_MAKEFOURCC('E','N','S','I'),
    MFX_EXTBUFF_VPP_SCALING                     = MFX_MAKEFOURCC('V','S','C','L'),
    MFX_EXTBUFF_HEVC_REFLIST_CTRL               = MFX_EXTBUFF_AVC_REFLIST_CTRL,
    MFX_EXTBUFF_HEVC_REFLISTS                   = MFX_EXTBUFF_AVC_REFLISTS,
    MFX_EXTBUFF_HEVC_TEMPORAL_LAYERS            = MFX_EXTBUFF_AVC_TEMPORAL_LAYERS,
    MFX_EXTBUFF_VPP_MIRRORING                   = MFX_MAKEFOURCC('M','I','R','R'),
    MFX_EXTBUFF_MV_OVER_PIC_BOUNDARIES          = MFX_MAKEFOURCC('M','V','P','B'),
    MFX_EXTBUFF_VPP_COLORFILL                   = MFX_MAKEFOURCC('V','C','L','F'),
#if (MFX_VERSION >= 1025)
    MFX_EXTBUFF_DECODE_ERROR_REPORT             = MFX_MAKEFOURCC('D', 'E', 'R', 'R'),
    MFX_EXTBUFF_VPP_COLOR_CONVERSION            = MFX_MAKEFOURCC('V', 'C', 'S', 'C'),
    MFX_EXTBUFF_CONTENT_LIGHT_LEVEL_INFO        = MFX_MAKEFOURCC('L', 'L', 'I', 'S'),
    MFX_EXTBUFF_MASTERING_DISPLAY_COLOUR_VOLUME = MFX_MAKEFOURCC('D', 'C', 'V', 'S'),
    MFX_EXTBUFF_MULTI_FRAME_PARAM               = MFX_MAKEFOURCC('M', 'F', 'R', 'P'),
    MFX_EXTBUFF_MULTI_FRAME_CONTROL             = MFX_MAKEFOURCC('M', 'F', 'R', 'C'),
    MFX_EXTBUFF_ENCODED_UNITS_INFO              = MFX_MAKEFOURCC('E', 'N', 'U', 'I'),
#endif
#if (MFX_VERSION >= 1026)
    MFX_EXTBUFF_VPP_MCTF                        = MFX_MAKEFOURCC('M', 'C', 'T', 'F'),
    MFX_EXTBUFF_VP9_SEGMENTATION                = MFX_MAKEFOURCC('9', 'S', 'E', 'G'),
    MFX_EXTBUFF_VP9_TEMPORAL_LAYERS             = MFX_MAKEFOURCC('9', 'T', 'M', 'L'),
    MFX_EXTBUFF_VP9_PARAM                       = MFX_MAKEFOURCC('9', 'P', 'A', 'R'),
#endif
#if (MFX_VERSION >= 1027)
    MFX_EXTBUFF_AVC_ROUNDING_OFFSET             = MFX_MAKEFOURCC('R','N','D','O'),
#endif
#if (MFX_VERSION >= MFX_VERSION_NEXT)
    MFX_EXTBUFF_DPB                             = MFX_MAKEFOURCC('E','D','P','B'),
    MFX_EXTBUFF_TEMPORAL_LAYERS                 = MFX_MAKEFOURCC('T','M','P','L'),
    MFX_EXTBUFF_AVC_SCALING_MATRIX              = MFX_MAKEFOURCC('A','V','S','M'),
    MFX_EXTBUFF_MPEG2_QUANT_MATRIX              = MFX_MAKEFOURCC('M','2','Q','M'),
    MFX_EXTBUFF_TASK_DEPENDENCY                 = MFX_MAKEFOURCC('S','Y','N','C'),
#endif
#if (MFX_VERSION >= 1031)
    MFX_EXTBUFF_PARTIAL_BITSTREAM_PARAM         = MFX_MAKEFOURCC('P','B','O','P'),
#endif
    MFX_EXTBUFF_ENCODER_IPCM_AREA               = MFX_MAKEFOURCC('P', 'C', 'M', 'R'),
    MFX_EXTBUFF_INSERT_HEADERS                  = MFX_MAKEFOURCC('S', 'P', 'R', 'E'),
#if (MFX_VERSION >= 1034)
    MFX_EXTBUFF_AV1_FILM_GRAIN_PARAM            = MFX_MAKEFOURCC('A','1','F','G'),
    MFX_EXTBUFF_AV1_LST_PARAM                   = MFX_MAKEFOURCC('A', '1', 'L', 'S'),
    MFX_EXTBUFF_AV1_SEGMENTATION                = MFX_MAKEFOURCC('1', 'S', 'E', 'G'),
    MFX_EXTBUFF_AV1_PARAM                       = MFX_MAKEFOURCC('1', 'P', 'A', 'R'),
    MFX_EXTBUFF_AV1_AUXDATA                     = MFX_MAKEFOURCC('1', 'A', 'U', 'X'),
    MFX_EXTBUFF_AV1_TEMPORAL_LAYERS             = MFX_MAKEFOURCC('1', 'T', 'M', 'L')
#endif
};

MFX_DEPRECATED_ENUM_FIELD_OUTSIDE(MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);

/* VPP Conf: Do not use certain algorithms  */
MFX_PACK_BEGIN_STRUCT_W_PTR()
typedef struct {
    mfxExtBuffer    Header;
    mfxU32          NumAlg;
    mfxU32*         AlgList;
} mfxExtVPPDoNotUse;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer    Header;
    mfxU16  DenoiseFactor;
} mfxExtVPPDenoise;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer    Header;
    mfxU16  DetailFactor;
} mfxExtVPPDetail;
MFX_PACK_END()

MFX_PACK_BEGIN_STRUCT_W_L_TYPE()
typedef struct {
    mfxExtBuffer    Header;
    mfxF64   Brightness;
    mfxF64   Contrast;
    mfxF64   Hue;
    mfxF64   Saturation;
} mfxExtVPPProcAmp;
MFX_PACK_END()

/* statistics collected for decode, encode and vpp */
MFX_PACK_BEGIN_STRUCT_W_L_TYPE()
typedef struct {
    mfxU32  reserved[16];
    mfxU32  NumFrame;
    mfxU64  NumBit;
    mfxU32  NumCachedFrame;
} mfxEncodeStat;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxU32  reserved[16];
    mfxU32  NumFrame;
    mfxU32  NumSkippedFrame;
    mfxU32  NumError;
    mfxU32  NumCachedFrame;
} mfxDecodeStat;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxU32  reserved[16];
    mfxU32  NumFrame;
    mfxU32  NumCachedFrame;
} mfxVPPStat;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
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
MFX_PACK_END()

/* CtrlFlags */
enum {
    MFX_PAYLOAD_CTRL_SUFFIX = 0x00000001 /* HEVC suffix SEI */
};

MFX_PACK_BEGIN_STRUCT_W_PTR()
typedef struct {
    mfxU32      CtrlFlags;
    mfxU32      reserved[3];
    mfxU8       *Data;      /* buffer pointer */
    mfxU32      NumBit;     /* number of bits */
    mfxU16      Type;       /* SEI message type in H.264 or user data start_code in MPEG-2 */
    mfxU16      BufSize;    /* payload buffer size in bytes */
} mfxPayload;
MFX_PACK_END()

MFX_PACK_BEGIN_STRUCT_W_PTR()
typedef struct {
    mfxExtBuffer    Header;
#if (MFX_VERSION >= 1025)
    mfxU32  reserved[4];
    mfxU16  reserved1;
    mfxU16  MfxNalUnitType;
#else
    mfxU32  reserved[5];
#endif
    mfxU16  SkipFrame;

    mfxU16  QP; /* per frame QP */

    mfxU16  FrameType;
    mfxU16  NumExtParam;
    mfxU16  NumPayload;     /* MPEG-2 user data or H.264 SEI message(s) */
    mfxU16  reserved2;

    mfxExtBuffer    **ExtParam;
    mfxPayload      **Payload;      /* for field pair, first field uses even payloads and second field uses odd payloads */
} mfxEncodeCtrl;
MFX_PACK_END()

/* Buffer Memory Types */
enum {
    /* Buffer types */
    MFX_MEMTYPE_PERSISTENT_MEMORY   =0x0002
};

/* Frame Memory Types */
#define MFX_MEMTYPE_BASE(x) (0x90ff & (x))

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
    MFX_MEMTYPE_FROM_ENC        = 0x2000,
    MFX_MEMTYPE_FROM_PAK        = 0x4000, //reserved

    MFX_MEMTYPE_INTERNAL_FRAME  = 0x0001,
    MFX_MEMTYPE_EXTERNAL_FRAME  = 0x0002,
    MFX_DEPRECATED_ENUM_FIELD_INSIDE(MFX_MEMTYPE_OPAQUE_FRAME)    = 0x0004,
    MFX_MEMTYPE_EXPORT_FRAME    = 0x0008,
    MFX_MEMTYPE_SHARED_RESOURCE = MFX_MEMTYPE_EXPORT_FRAME,
#if (MFX_VERSION >= 1025)
    MFX_MEMTYPE_VIDEO_MEMORY_ENCODER_TARGET = 0x1000
#else
    MFX_MEMTYPE_RESERVED2       = 0x1000
#endif
};

MFX_DEPRECATED_ENUM_FIELD_OUTSIDE(MFX_MEMTYPE_OPAQUE_FRAME);

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    union {
        mfxU32  AllocId;
        mfxU32  reserved[1];
    };
    mfxU32  reserved3[3];
    mfxFrameInfo    Info;
    mfxU16  Type;   /* decoder or processor render targets */
    mfxU16  NumFrameMin;
    mfxU16  NumFrameSuggested;
    mfxU16  reserved2;
} mfxFrameAllocRequest;
MFX_PACK_END()

MFX_PACK_BEGIN_STRUCT_W_PTR()
typedef struct {
    mfxU32      AllocId;
    mfxU32      reserved[3];
    mfxMemId    *mids;      /* the array allocated by application */
    mfxU16      NumFrameActual;
#if (MFX_VERSION >= MFX_VERSION_NEXT)
    mfxU16      MemType;
#else
    mfxU16      reserved2;
#endif
} mfxFrameAllocResponse;
MFX_PACK_END()

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

#if (MFX_VERSION >= 1025)
enum {
    MFX_HEVC_NALU_TYPE_UNKNOWN    =      0,
    MFX_HEVC_NALU_TYPE_TRAIL_N    = ( 0+1),
    MFX_HEVC_NALU_TYPE_TRAIL_R    = ( 1+1),
    MFX_HEVC_NALU_TYPE_RADL_N     = ( 6+1),
    MFX_HEVC_NALU_TYPE_RADL_R     = ( 7+1),
    MFX_HEVC_NALU_TYPE_RASL_N     = ( 8+1),
    MFX_HEVC_NALU_TYPE_RASL_R     = ( 9+1),
    MFX_HEVC_NALU_TYPE_IDR_W_RADL = (19+1),
    MFX_HEVC_NALU_TYPE_IDR_N_LP   = (20+1),
    MFX_HEVC_NALU_TYPE_CRA_NUT    = (21+1)
};
#endif

typedef enum {
    MFX_HANDLE_DIRECT3D_DEVICE_MANAGER9         =1,      /* IDirect3DDeviceManager9      */
    MFX_HANDLE_D3D9_DEVICE_MANAGER              = MFX_HANDLE_DIRECT3D_DEVICE_MANAGER9,
    MFX_HANDLE_RESERVED1                        = 2,
    MFX_HANDLE_D3D11_DEVICE                     = 3,
    MFX_HANDLE_VA_DISPLAY                       = 4,
    MFX_HANDLE_RESERVED3                        = 5,
#if (MFX_VERSION >= 1030)
    MFX_HANDLE_VA_CONFIG_ID                     = 6,
    MFX_HANDLE_VA_CONTEXT_ID                    = 7,
#endif
#if (MFX_VERSION >= MFX_VERSION_NEXT)
    MFX_HANDLE_CM_DEVICE                        = 8
#endif
} mfxHandleType;

typedef enum {
    MFX_SKIPMODE_NOSKIP=0,
    MFX_SKIPMODE_MORE=1,
    MFX_SKIPMODE_LESS=2
} mfxSkipMode;

MFX_PACK_BEGIN_STRUCT_W_PTR()
typedef struct {
    mfxExtBuffer    Header;
    mfxU8           *SPSBuffer;
    mfxU8           *PPSBuffer;
    mfxU16          SPSBufSize;
    mfxU16          PPSBufSize;
    mfxU16          SPSId;
    mfxU16          PPSId;
} mfxExtCodingOptionSPSPPS;
MFX_PACK_END()

MFX_PACK_BEGIN_STRUCT_W_L_TYPE()
typedef struct {
    mfxExtBuffer    Header;

    union {
        mfxU8       *VPSBuffer;
        mfxU64      reserved1;
    };
    mfxU16          VPSBufSize;
    mfxU16          VPSId;

    mfxU16          reserved[6];
} mfxExtCodingOptionVPS;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer    Header;
    mfxU16          VideoFormat;
    mfxU16          VideoFullRange;
    mfxU16          ColourDescriptionPresent;
    mfxU16          ColourPrimaries;
    mfxU16          TransferCharacteristics;
    mfxU16          MatrixCoefficients;
} mfxExtVideoSignalInfo;
MFX_PACK_END()

MFX_PACK_BEGIN_STRUCT_W_PTR()
typedef struct {
    mfxExtBuffer    Header;
    mfxU32          NumAlg;
    mfxU32          *AlgList;
} mfxExtVPPDoUse;
MFX_PACK_END()

MFX_PACK_BEGIN_STRUCT_W_PTR()
MFX_DEPRECATED typedef struct {
    mfxExtBuffer    Header;
    mfxU32      reserved1[2];
    struct {
        mfxFrameSurface1 **Surfaces;
        mfxU32  reserved2[5];
        mfxU16  Type;
        mfxU16  NumSurface;
    } In, Out;
} mfxExtOpaqueSurfaceAlloc;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
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
MFX_PACK_END()

enum {
    MFX_FRCALGM_PRESERVE_TIMESTAMP    = 0x0001,
    MFX_FRCALGM_DISTRIBUTED_TIMESTAMP = 0x0002,
    MFX_FRCALGM_FRAME_INTERPOLATION   = 0x0004
};

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer    Header;
    mfxU16      Algorithm;
    mfxU16      reserved;
    mfxU32      reserved2[15];
} mfxExtVPPFrameRateConversion;
MFX_PACK_END()

enum {
    MFX_IMAGESTAB_MODE_UPSCALE = 0x0001,
    MFX_IMAGESTAB_MODE_BOXING  = 0x0002
};

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer    Header;
    mfxU16  Mode;
    mfxU16  reserved[11];
} mfxExtVPPImageStab;
MFX_PACK_END()

#if (MFX_VERSION >= 1025)

enum {
    MFX_PAYLOAD_OFF = 0,
    MFX_PAYLOAD_IDR = 1
};

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer    Header;
    mfxU16      reserved[15];

    mfxU16 InsertPayloadToggle;
    mfxU16 DisplayPrimariesX[3];
    mfxU16 DisplayPrimariesY[3];
    mfxU16 WhitePointX;
    mfxU16 WhitePointY;
    mfxU32 MaxDisplayMasteringLuminance;
    mfxU32 MinDisplayMasteringLuminance;
} mfxExtMasteringDisplayColourVolume;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer    Header;
    mfxU16      reserved[9];

    mfxU16 InsertPayloadToggle;
    mfxU16 MaxContentLightLevel;
    mfxU16 MaxPicAverageLightLevel;
} mfxExtContentLightLevelInfo;
MFX_PACK_END()
#endif

MFX_PACK_BEGIN_USUAL_STRUCT()
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
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
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
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer Header;

    mfxU32      MBPerSec;
    mfxU16      reserved[58];
} mfxExtEncoderCapability;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer Header;

    mfxU16      StartNewSequence;
    mfxU16      reserved[11];
} mfxExtEncoderResetOption;
MFX_PACK_END()

/*LongTermIdx*/
enum {
    MFX_LONGTERM_IDX_NO_IDX = 0xFFFF
};

MFX_PACK_BEGIN_USUAL_STRUCT()
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
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
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
    mfxU16  PixelAlphaEnable;

    mfxU16  TileId;

    mfxU16  reserved2[17];
} mfxVPPCompInputStream;
MFX_PACK_END()

MFX_PACK_BEGIN_STRUCT_W_PTR()
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
    mfxU16       NumTiles;
    mfxU16       reserved1[23];

    mfxU16       NumInputStream;
    mfxVPPCompInputStream *InputStream;
} mfxExtVPPComposite;
MFX_PACK_END()

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

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer    Header;
    mfxU16          reserved1[4];

    union {
        struct { // Init
            struct  {
                mfxU16  TransferMatrix;
                mfxU16  NominalRange;
                mfxU16  reserved2[6];
            } In, Out;
        };
        struct { // Runtime
            mfxU16  TransferMatrix;
            mfxU16  NominalRange;
            mfxU16  reserved3[14];
        };
    };
} mfxExtVPPVideoSignalInfo;
MFX_PACK_END()

/* ROI encoding mode */
enum {
    MFX_ROI_MODE_PRIORITY =  0,
    MFX_ROI_MODE_QP_DELTA =  1,
    MFX_ROI_MODE_QP_VALUE =  2 
};

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer    Header;

    mfxU16  NumROI;
    mfxU16  ROIMode;
    mfxU16  reserved1[10];

    struct  {
        mfxU32  Left;
        mfxU32  Top;
        mfxU32  Right;
        mfxU32  Bottom;
        union {
            mfxI16  Priority;
            mfxI16  DeltaQP;
        };
        mfxU16  reserved2[7];
    } ROI[256];
} mfxExtEncoderROI;
MFX_PACK_END()

/*Deinterlacing Mode*/
enum {
    MFX_DEINTERLACING_BOB                    =  1,
    MFX_DEINTERLACING_ADVANCED               =  2,
    MFX_DEINTERLACING_AUTO_DOUBLE            =  3,
    MFX_DEINTERLACING_AUTO_SINGLE            =  4,
    MFX_DEINTERLACING_FULL_FR_OUT            =  5,
    MFX_DEINTERLACING_HALF_FR_OUT            =  6,
    MFX_DEINTERLACING_24FPS_OUT              =  7,
    MFX_DEINTERLACING_FIXED_TELECINE_PATTERN =  8,
    MFX_DEINTERLACING_30FPS_OUT              =  9,
    MFX_DEINTERLACING_DETECT_INTERLACE       = 10,
    MFX_DEINTERLACING_ADVANCED_NOREF         = 11,
    MFX_DEINTERLACING_ADVANCED_SCD           = 12,
    MFX_DEINTERLACING_FIELD_WEAVING          = 13
};

/*TelecinePattern*/
enum {
    MFX_TELECINE_PATTERN_32           = 0,
    MFX_TELECINE_PATTERN_2332         = 1,
    MFX_TELECINE_PATTERN_FRAME_REPEAT = 2,
    MFX_TELECINE_PATTERN_41           = 3,
    MFX_TELECINE_POSITION_PROVIDED    = 4
};

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer    Header;
    mfxU16  Mode;
    mfxU16  TelecinePattern;
    mfxU16  TelecineLocation;
    mfxU16  reserved[9];
} mfxExtVPPDeinterlacing;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
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
MFX_PACK_END()

enum {
    MFX_VPP_COPY_FRAME      =0x01,
    MFX_VPP_COPY_FIELD      =0x02,
    MFX_VPP_SWAP_FIELDS     =0x03
};

/*PicType*/
enum {
    MFX_PICTYPE_UNKNOWN     =0x00,
    MFX_PICTYPE_FRAME       =0x01,
    MFX_PICTYPE_TOPFIELD    =0x02,
    MFX_PICTYPE_BOTTOMFIELD =0x04
};

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer    Header;

    mfxU16          Mode;
    mfxU16          InField;
    mfxU16          OutField;
    mfxU16          reserved[25];
} mfxExtVPPFieldProcessing;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer    Header;

    struct mfxIn{
        mfxU16  CropX;
        mfxU16  CropY;
        mfxU16  CropW;
        mfxU16  CropH;
        mfxU16  reserved[12];
    }In;

    struct mfxOut{
        mfxU32  FourCC;
        mfxU16  ChromaFormat;
        mfxU16  reserved1;

        mfxU16  Width;
        mfxU16  Height;

        mfxU16  CropX;
        mfxU16  CropY;
        mfxU16  CropW;
        mfxU16  CropH;
        mfxU16  reserved[22];
    }Out;

    mfxU16  reserved[13];
} mfxExtDecVideoProcessing;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer Header;

    mfxU16       ChromaLocInfoPresentFlag;
    mfxU16       ChromaSampleLocTypeTopField;
    mfxU16       ChromaSampleLocTypeBottomField;
    mfxU16       reserved[9];
} mfxExtChromaLocInfo;
MFX_PACK_END()

/* MBQPMode */
enum {
    MFX_MBQP_MODE_QP_VALUE = 0, // supported in CQP mode only
    MFX_MBQP_MODE_QP_DELTA = 1,
    MFX_MBQP_MODE_QP_ADAPTIVE = 2
};

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct{
    union {
        mfxU8 QP;
        mfxI8 DeltaQP;
    };
    mfxU16 Mode;
} mfxQPandMode;
MFX_PACK_END()

MFX_PACK_BEGIN_STRUCT_W_L_TYPE()
typedef struct {
    mfxExtBuffer    Header;

    mfxU32 reserved[10];
    mfxU16 Mode;        // see MBQPMode enum
    mfxU16 BlockSize;   // QP block size, valid for HEVC only during Init and Runtime
    mfxU32 NumQPAlloc;  // Size of allocated by application QP or DeltaQP array
    union {
        mfxU8  *QP;         // Block QP value. Valid when Mode = MFX_MBQP_MODE_QP_VALUE
        mfxI8  *DeltaQP;    // For block i: QP[i] = BrcQP[i] + DeltaQP[i]. Valid when Mode = MFX_MBQP_MODE_QP_DELTA
#if (MFX_VERSION >= 1034)
        mfxQPandMode *QPmode; // Block-granularity modes when MFX_MBQP_MODE_QP_ADAPTIVE is set
#endif
        mfxU64 reserved2;
    };
} mfxExtMBQP;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer    Header; /* Extension buffer header. Header.BufferId must be equal to MFX_EXTBUFF_INSERT_HEADERS. */
    mfxU16          SPS;      /* tri-state option to insert SPS */
    mfxU16          PPS;      /* tri-state option to insert PPS */
    mfxU16          reserved[8];
} mfxExtInsertHeaders;
MFX_PACK_END()

MFX_PACK_BEGIN_STRUCT_W_PTR() 
typedef struct {
    mfxExtBuffer    Header; /* Extension buffer header. Header.BufferId must be equal to MFX_EXTBUFF_ENCODER_IPCM_AREA. */
    mfxU16          reserve1[10];

    mfxU16          NumArea;  /* Number of Area's */
    struct area {
        mfxU32      Left; /* Left Area's coordinate. */
        mfxU32      Top;  /* Top  Area's coordinate. */
        mfxU32      Right; /* Right Area's coordinate. */
        mfxU32      Bottom; /* Bottom Area's coordinate. */

        mfxU16      reserved2[8];
    } * Areas; /* Array of areas. */
} mfxExtEncoderIPCMArea;
MFX_PACK_END()

MFX_PACK_BEGIN_STRUCT_W_L_TYPE()
typedef struct {
    mfxExtBuffer Header;

    mfxU32 reserved[11];
    mfxU32 MapSize;
    union {
        mfxU8  *Map;
        mfxU64  reserved2;
    };
} mfxExtMBForceIntra;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer Header;

    mfxU16 NumTileRows;
    mfxU16 NumTileColumns;
    mfxU16 reserved[74];
}mfxExtHEVCTiles;
MFX_PACK_END()

MFX_PACK_BEGIN_STRUCT_W_L_TYPE()
typedef struct {
    mfxExtBuffer Header;

    mfxU32 reserved[11];
    mfxU32 MapSize;
    union {
        mfxU8  *Map;
        mfxU64  reserved2;
    };
} mfxExtMBDisableSkipMap;
MFX_PACK_END()

#if (MFX_VERSION >= MFX_VERSION_NEXT)
MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer    Header;

    mfxU16          DPBSize;
    mfxU16          reserved[11];

    struct {
        mfxU32      FrameOrder;
        mfxU16      PicType;
        mfxU16      LongTermIdx;
        mfxU16      reserved[4];
    } DPB[32];
} mfxExtDPB;
MFX_PACK_END()

#endif

/*GeneralConstraintFlags*/
enum {
    /* REXT Profile constraint flags*/
    MFX_HEVC_CONSTR_REXT_MAX_12BIT          = (1 << 0),
    MFX_HEVC_CONSTR_REXT_MAX_10BIT          = (1 << 1),
    MFX_HEVC_CONSTR_REXT_MAX_8BIT           = (1 << 2),
    MFX_HEVC_CONSTR_REXT_MAX_422CHROMA      = (1 << 3),
    MFX_HEVC_CONSTR_REXT_MAX_420CHROMA      = (1 << 4),
    MFX_HEVC_CONSTR_REXT_MAX_MONOCHROME     = (1 << 5),
    MFX_HEVC_CONSTR_REXT_INTRA              = (1 << 6),
    MFX_HEVC_CONSTR_REXT_ONE_PICTURE_ONLY   = (1 << 7),
    MFX_HEVC_CONSTR_REXT_LOWER_BIT_RATE     = (1 << 8)
};

#if (MFX_VERSION >= 1026)

/* SampleAdaptiveOffset */
enum {
    MFX_SAO_UNKNOWN       = 0x00,
    MFX_SAO_DISABLE       = 0x01,
    MFX_SAO_ENABLE_LUMA   = 0x02,
    MFX_SAO_ENABLE_CHROMA = 0x04
};

#endif

/* This struct has 4-byte alignment for binary compatibility with previously released versions of API */
MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer    Header;

    mfxU16          PicWidthInLumaSamples;
    mfxU16          PicHeightInLumaSamples;
    mfxU64          GeneralConstraintFlags;
#if (MFX_VERSION >= 1026)
    mfxU16          SampleAdaptiveOffset;   /* see enum SampleAdaptiveOffset, valid during Init and Runtime */
    mfxU16          LCUSize;
    mfxU16          reserved[116];
#else
    mfxU16          reserved[118];
#endif
} mfxExtHEVCParam;
MFX_PACK_END()

#if (MFX_VERSION >= 1025)
/*ErrorTypes in mfxExtDecodeErrorReport*/
enum {
    MFX_ERROR_PPS           = (1 << 0),
    MFX_ERROR_SPS           = (1 << 1),
    MFX_ERROR_SLICEHEADER   = (1 << 2),
    MFX_ERROR_SLICEDATA     = (1 << 3),
    MFX_ERROR_FRAME_GAP     = (1 << 4),
};

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer    Header;

    mfxU32          ErrorTypes;
    mfxU16          reserved[10];
} mfxExtDecodeErrorReport;
MFX_PACK_END()

#endif

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer Header;

    mfxU16       FrameType;
    mfxU16       reserved[59];
} mfxExtDecodedFrameInfo;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer Header;

    mfxU16       DropFrameFlag;
    mfxU16       TimeCodeHours;
    mfxU16       TimeCodeMinutes;
    mfxU16       TimeCodeSeconds;
    mfxU16       TimeCodePictures;
    mfxU16       reserved[7];
} mfxExtTimeCode;
MFX_PACK_END()

/*RegionType*/
enum {
    MFX_HEVC_REGION_SLICE = 0
};

/*RegionEncoding*/
enum {
    MFX_HEVC_REGION_ENCODING_ON  = 0,
    MFX_HEVC_REGION_ENCODING_OFF = 1
};

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer Header;

    mfxU32       RegionId;
    mfxU16       RegionType;
    mfxU16       RegionEncoding;
    mfxU16       reserved[24];
} mfxExtHEVCRegion;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer Header;

    mfxU16       LumaLog2WeightDenom;       // 0..7
    mfxU16       ChromaLog2WeightDenom;     // 0..7
    mfxU16       LumaWeightFlag[2][32];     // [list] 0,1
    mfxU16       ChromaWeightFlag[2][32];   // [list] 0,1
    mfxI16       Weights[2][32][3][2];      // [list][list entry][Y, Cb, Cr][weight, offset]
    mfxU16       reserved[58];
} mfxExtPredWeightTable;
MFX_PACK_END()

#if (MFX_VERSION >= 1027)
MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer Header;

    mfxU16       EnableRoundingIntra;       // tri-state option
    mfxU16       RoundingOffsetIntra;       // valid value [0,7]
    mfxU16       EnableRoundingInter;       // tri-state option
    mfxU16       RoundingOffsetInter;       // valid value [0,7]

    mfxU16       reserved[24];
} mfxExtAVCRoundingOffset;
MFX_PACK_END()
#endif

#if (MFX_VERSION >= MFX_VERSION_NEXT)
MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer Header;

    mfxU16       reserved[12];

    struct {
        mfxU16   Scale;
        mfxU16   QPI;
        mfxU16   QPP;
        mfxU16   QPB;
        mfxU32   TargetKbps;
        mfxU32   MaxKbps;
        mfxU32   BufferSizeInKB;
        mfxU32   InitialDelayInKB;
        mfxU16   reserved1[20];
    } Layer[8];
} mfxExtTemporalLayers;
MFX_PACK_END()

#endif

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer Header;

    mfxU16  NumRect;
    mfxU16  reserved1[11];

    struct {
        mfxU32  Left;
        mfxU32  Top;
        mfxU32  Right;
        mfxU32  Bottom;

        mfxU16  reserved2[8];
    } Rect[256];
} mfxExtDirtyRect;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer Header;

    mfxU16  NumRect;
    mfxU16  reserved1[11];

    struct {
        mfxU32  DestLeft;
        mfxU32  DestTop;
        mfxU32  DestRight;
        mfxU32  DestBottom;

        mfxU32  SourceLeft;
        mfxU32  SourceTop;
        mfxU16  reserved2[4];
    } Rect[256];
} mfxExtMoveRect;
MFX_PACK_END()

#if (MFX_VERSION >= MFX_VERSION_NEXT)

/* ScalingMatrixType */
enum {
    MFX_SCALING_MATRIX_SPS = 1,
    MFX_SCALING_MATRIX_PPS = 2
};

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer Header;

    mfxU16 Type;
    mfxU16 reserved[5];

    /* [4x4_Intra_Y,  4x4_Intra_Cb, 4x4_Intra_Cr,
        4x4_Inter_Y,  4x4_Inter_Cb, 4x4_Inter_Cr,
        8x8_Intra_Y,  8x8_Inter_Y,  8x8_Intra_Cb,
        8x8_Inter_Cb, 8x8_Intra_Cr, 8x8_Inter_Cr] */
    mfxU8  ScalingListPresent[12];

    /* [Intra_Y,  Intra_Cb, Intra_Cr,
        Inter_Y,  Inter_Cb, Inter_Cr] */
    mfxU8  ScalingList4x4[6][16];

    /* [Intra_Y,  Inter_Y,  Intra_Cb,
        Inter_Cb, Intra_Cr, Inter_Cr] */
    mfxU8  ScalingList8x8[6][64];
} mfxExtAVCScalingMatrix;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer Header;

    mfxU16 reserved[28];

    mfxU8  LoadMatrix[4]; // [LumaIntra, LumaInter, ChromaIntra, ChromaInter]
    mfxU8  Matrix[4][64]; // [LumaIntra, LumaInter, ChromaIntra, ChromaInter]
} mfxExtMPEG2QuantMatrix;
MFX_PACK_END()

#endif

/* Angle */
enum {
    MFX_ANGLE_0     =   0,
    MFX_ANGLE_90    =  90,
    MFX_ANGLE_180   = 180,
    MFX_ANGLE_270   = 270
};

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer Header;

    mfxU16 Angle;
    mfxU16 reserved[11];
} mfxExtVPPRotation;
MFX_PACK_END()

MFX_PACK_BEGIN_STRUCT_W_L_TYPE()
typedef struct {
    mfxExtBuffer Header;

    mfxU16  SliceSizeOverflow;
    mfxU16  NumSliceNonCopliant;
    mfxU16  NumEncodedSlice;
    mfxU16  NumSliceSizeAlloc;
    union {
        mfxU16  *SliceSize;
        mfxU64  reserved1;
    };

    mfxU16 reserved[20];
} mfxExtEncodedSlicesInfo;
MFX_PACK_END()

/* ScalingMode */
enum {
    MFX_SCALING_MODE_DEFAULT    = 0,
    MFX_SCALING_MODE_LOWPOWER   = 1,
    MFX_SCALING_MODE_QUALITY    = 2
};

#if (MFX_VERSION >= 1033)
/* Interpolation Method */
enum {
    MFX_INTERPOLATION_DEFAULT                = 0,
    MFX_INTERPOLATION_NEAREST_NEIGHBOR       = 1,
    MFX_INTERPOLATION_BILINEAR               = 2,
    MFX_INTERPOLATION_ADVANCED               = 3
};
#endif

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer Header;

    mfxU16 ScalingMode;
#if (MFX_VERSION >= 1033)
    mfxU16 InterpolationMethod;
    mfxU16 reserved[10];
#else
    mfxU16 reserved[11];
#endif
} mfxExtVPPScaling;
MFX_PACK_END()

#if (MFX_VERSION >= MFX_VERSION_NEXT)

/* SceneChangeType */
enum {
    MFX_SCENE_NO_CHANGE = 0,
    MFX_SCENE_START     = 1,
    MFX_SCENE_END       = 2
};

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer Header;

    mfxU16 Type;
    mfxU16 reserved[11];
} mfxExtSceneChange;
MFX_PACK_END()

#endif

typedef mfxExtAVCRefListCtrl mfxExtHEVCRefListCtrl;
typedef mfxExtAVCRefLists mfxExtHEVCRefLists;
typedef mfxExtAvcTemporalLayers mfxExtHEVCTemporalLayers;

/* MirroringType */
enum
{
    MFX_MIRRORING_DISABLED   = 0,
    MFX_MIRRORING_HORIZONTAL = 1,
    MFX_MIRRORING_VERTICAL   = 2
};

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer Header;

    mfxU16 Type;
    mfxU16 reserved[11];
} mfxExtVPPMirroring;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer Header;

    mfxU16 StickTop;     /* tri-state option */
    mfxU16 StickBottom;  /* tri-state option */
    mfxU16 StickLeft;    /* tri-state option */
    mfxU16 StickRight;   /* tri-state option */
    mfxU16 reserved[8];
} mfxExtMVOverPicBoundaries;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer Header;

    mfxU16 Enable;        /* tri-state option */
    mfxU16 reserved[11];
} mfxExtVPPColorFill;
MFX_PACK_END()

#if (MFX_VERSION >= 1025)

/* ChromaSiting */
enum {
    MFX_CHROMA_SITING_UNKNOWN             = 0x0000,
    MFX_CHROMA_SITING_VERTICAL_TOP        = 0x0001, /* Chroma samples are co-sited vertically on the top with the luma samples. */
    MFX_CHROMA_SITING_VERTICAL_CENTER     = 0x0002, /* Chroma samples are not co-sited vertically with the luma samples. */
    MFX_CHROMA_SITING_VERTICAL_BOTTOM     = 0x0004, /* Chroma samples are co-sited vertically on the bottom with the luma samples. */
    MFX_CHROMA_SITING_HORIZONTAL_LEFT     = 0x0010, /* Chroma samples are co-sited horizontally on the left with the luma samples. */
    MFX_CHROMA_SITING_HORIZONTAL_CENTER   = 0x0020  /* Chroma samples are not co-sited horizontally with the luma samples. */
};

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer Header;

    mfxU16 ChromaSiting;
    mfxU16 reserved[27];
} mfxExtColorConversion;
MFX_PACK_END()

#endif

#if (MFX_VERSION >= 1026)
/* VP9ReferenceFrame */
enum {
    MFX_VP9_REF_INTRA   = 0,
    MFX_VP9_REF_LAST    = 1,
    MFX_VP9_REF_GOLDEN  = 2,
    MFX_VP9_REF_ALTREF  = 3
};

/* SegmentIdBlockSize */
enum {
    MFX_VP9_SEGMENT_ID_BLOCK_SIZE_UNKNOWN =  0,
    MFX_VP9_SEGMENT_ID_BLOCK_SIZE_8x8     =  8,
    MFX_VP9_SEGMENT_ID_BLOCK_SIZE_16x16   = 16,
    MFX_VP9_SEGMENT_ID_BLOCK_SIZE_32x32   = 32,
    MFX_VP9_SEGMENT_ID_BLOCK_SIZE_64x64   = 64,
};

/* SegmentFeature */
enum {
    MFX_VP9_SEGMENT_FEATURE_QINDEX      = 0x0001,
    MFX_VP9_SEGMENT_FEATURE_LOOP_FILTER = 0x0002,
    MFX_VP9_SEGMENT_FEATURE_REFERENCE   = 0x0004,
    MFX_VP9_SEGMENT_FEATURE_SKIP        = 0x0008 /* (0,0) MV, no residual */
};

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxU16  FeatureEnabled;         /* see enum SegmentFeature */
    mfxI16  QIndexDelta;
    mfxI16  LoopFilterLevelDelta;
    mfxU16  ReferenceFrame;        /* see enum VP9ReferenceFrame */
    mfxU16  reserved[12];
} mfxVP9SegmentParam;
MFX_PACK_END()

MFX_PACK_BEGIN_STRUCT_W_L_TYPE()
typedef struct {
    mfxExtBuffer        Header;
    mfxU16              NumSegments;            /* 0..8 */
    mfxVP9SegmentParam  Segment[8];
    mfxU16              SegmentIdBlockSize;     /* see enum SegmentIdBlockSize */
    mfxU32              NumSegmentIdAlloc;      /* >= (Ceil(Width / SegmentIdBlockSize) * Ceil(Height / SegmentIdBlockSize)) */
    union {
        mfxU8           *SegmentId;             /*[NumSegmentIdAlloc] = 0..7, index in Segment array, blocks of SegmentIdBlockSize map */
        mfxU64          reserved1;
    };
    mfxU16  reserved[52];
} mfxExtVP9Segmentation;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxU16 FrameRateScale;  /* Layer[n].FrameRateScale = Layer[n - 1].FrameRateScale * (uint)m */
    mfxU16 TargetKbps;      /* affected by BRCParamMultiplier, Layer[n].TargetKbps > Layer[n - 1].TargetKbps */
    mfxU16 reserved[14];
} mfxVP9TemporalLayer;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer        Header;
    mfxVP9TemporalLayer Layer[8];
    mfxU16              reserved[60];
} mfxExtVP9TemporalLayers;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer Header;

    mfxU16  FrameWidth;
    mfxU16  FrameHeight;

    mfxU16  WriteIVFHeaders;        /* tri-state option */

#if (MFX_VERSION >= MFX_VERSION_NEXT)
    mfxI16  LoopFilterRefDelta[4];
    mfxI16  LoopFilterModeDelta[2];
#else // API 1.26
    mfxI16  reserved1[6];
#endif
    mfxI16  QIndexDeltaLumaDC;
    mfxI16  QIndexDeltaChromaAC;
    mfxI16  QIndexDeltaChromaDC;
#if (MFX_VERSION >= 1029)
    mfxU16  NumTileRows;
    mfxU16  NumTileColumns;
    mfxU16  reserved[110];
#else
    mfxU16  reserved[112];
#endif
} mfxExtVP9Param;
MFX_PACK_END()

#endif // #if (MFX_VERSION >= 1026)

#if (MFX_VERSION >= 1025)
/* Multi-Frame Mode */
enum {
    MFX_MF_DEFAULT = 0,
    MFX_MF_DISABLED = 1,
    MFX_MF_AUTO = 2,
    MFX_MF_MANUAL = 3
};

/* Multi-Frame Initialization parameters */
MFX_PACK_BEGIN_USUAL_STRUCT()
MFX_DEPRECATED typedef struct {
    mfxExtBuffer Header;

    mfxU16      MFMode;
    mfxU16      MaxNumFrames;

    mfxU16      reserved[58];
} mfxExtMultiFrameParam;
MFX_PACK_END()

/* Multi-Frame Run-time controls */
MFX_PACK_BEGIN_USUAL_STRUCT()
MFX_DEPRECATED typedef struct {
    mfxExtBuffer Header;

    mfxU32      Timeout;      /* timeout in millisecond */
    mfxU16      Flush;        /* Flush internal frame buffer, e.g. submit all collected frames. */

    mfxU16      reserved[57];
} mfxExtMultiFrameControl;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxU16 Type;
    mfxU16 reserved1;
    mfxU32 Offset;
    mfxU32 Size;
    mfxU32 reserved[5];
} mfxEncodedUnitInfo;
MFX_PACK_END()

MFX_PACK_BEGIN_STRUCT_W_L_TYPE()
typedef struct {
    mfxExtBuffer Header;

    union {
        mfxEncodedUnitInfo *UnitInfo;
        mfxU64  reserved1;
    };
    mfxU16 NumUnitsAlloc;
    mfxU16 NumUnitsEncoded;

    mfxU16 reserved[22];
} mfxExtEncodedUnitsInfo;
MFX_PACK_END()

#endif

#if (MFX_VERSION >= 1026)
#if (MFX_VERSION >= MFX_VERSION_NEXT)
/* MCTFTemporalMode */
enum {
    MFX_MCTF_TEMPORAL_MODE_UNKNOWN  = 0,
    MFX_MCTF_TEMPORAL_MODE_SPATIAL  = 1,
    MFX_MCTF_TEMPORAL_MODE_1REF     = 2,
    MFX_MCTF_TEMPORAL_MODE_2REF     = 3,
    MFX_MCTF_TEMPORAL_MODE_4REF     = 4
};
#endif

/* MCTF initialization & runtime */
MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer Header;
    mfxU16       FilterStrength;
#if (MFX_VERSION >= MFX_VERSION_NEXT)
    mfxU16       Overlap;               /* tri-state option */
    mfxU32       BitsPerPixelx100k;
    mfxU16       Deblocking;            /* tri-state option */
    mfxU16       TemporalMode;
    mfxU16       MVPrecision;
    mfxU16       reserved[21];
#else
    mfxU16       reserved[27];
#endif
} mfxExtVppMctf;
MFX_PACK_END()

#endif

#if (MFX_VERSION >= 1031)
/* Multi-adapters Querying structs */
typedef enum
{
    MFX_COMPONENT_ENCODE = 1,
    MFX_COMPONENT_DECODE = 2,
    MFX_COMPONENT_VPP    = 3
} mfxComponentType;

MFX_PACK_BEGIN_STRUCT_W_PTR()
typedef struct
{
    mfxComponentType Type;
    mfxVideoParam    Requirements;

    mfxU16           reserved[4];
} mfxComponentInfo;
MFX_PACK_END()

/* Adapter description */
MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct
{
    mfxPlatform Platform;
    mfxU32      Number;

    mfxU16      reserved[14];
} mfxAdapterInfo;
MFX_PACK_END()

MFX_PACK_BEGIN_STRUCT_W_PTR()
typedef struct
{
    mfxAdapterInfo * Adapters;
    mfxU32           NumAlloc;
    mfxU32           NumActual;

    mfxU16           reserved[4];
} mfxAdaptersInfo;
MFX_PACK_END()

#endif

#if (MFX_VERSION >= 1034)
/* FilmGrainFlags */
enum {
    MFX_FILM_GRAIN_APPLY                    = (1 << 0),
    MFX_FILM_GRAIN_UPDATE                   = (1 << 1),
    MFX_FILM_GRAIN_CHROMA_SCALING_FROM_LUMA = (1 << 2),
    MFX_FILM_GRAIN_OVERLAP                  = (1 << 3),
    MFX_FILM_GRAIN_CLIP_TO_RESTRICTED_RANGE = (1 << 4)
};

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxU8 Value;
    mfxU8 Scaling;
} mfxAV1FilmGrainPoint;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer Header;

    mfxU16 FilmGrainFlags;  /* FilmGrainFlags */
    mfxU16 GrainSeed;       /* 0..65535 */

    mfxU8  RefIdx;          /* 0..6  */
    mfxU8  NumYPoints;      /* 0..14 */
    mfxU8  NumCbPoints;     /* 0..10 */
    mfxU8  NumCrPoints;     /* 0..10 */

    mfxAV1FilmGrainPoint PointY[14];
    mfxAV1FilmGrainPoint PointCb[10];
    mfxAV1FilmGrainPoint PointCr[10];

    mfxU8 GrainScalingMinus8; /* 0..3 */
    mfxU8 ArCoeffLag;         /* 0..3 */

    mfxU8 ArCoeffsYPlus128[24];  /* 0..255 */
    mfxU8 ArCoeffsCbPlus128[25]; /* 0..255 */
    mfxU8 ArCoeffsCrPlus128[25]; /* 0..255 */

    mfxU8 ArCoeffShiftMinus6;  /* 0..3 */
    mfxU8 GrainScaleShift;     /* 0..3 */

    mfxU8  CbMult;     /* 0..255 */
    mfxU8  CbLumaMult; /* 0..255 */
    mfxU16 CbOffset;   /* 0..511 */

    mfxU8  CrMult;     /* 0..255 */
    mfxU8  CrLumaMult; /* 0..255 */
    mfxU16 CrOffset;   /* 0..511 */

    mfxU16 reserved[43];
} mfxExtAV1FilmGrainParam;
MFX_PACK_END()

#endif

#if (MFX_VERSION >= 1031)
/* PartialBitstreamOutput */
enum {
    MFX_PARTIAL_BITSTREAM_NONE    = 0,     /* Don't use partial output */
    MFX_PARTIAL_BITSTREAM_SLICE   = 1,     /* Partial bitstream output will be aligned to slice granularity */
    MFX_PARTIAL_BITSTREAM_BLOCK   = 2,     /* Partial bitstream output will be aligned to user-defined block size granularity */
    MFX_PARTIAL_BITSTREAM_ANY     = 3      /* Partial bitstream output will be return any coded data avilable at the end of SyncOperation timeout */
};

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer    Header;
    mfxU32          BlockSize;        /* output block granulatiry for Granularity = MFX_PARTIAL_BITSTREAM_BLOCK */
    mfxU16          Granularity;      /* granulatiry of the partial bitstream: slice/block/any */
    mfxU16          reserved[8];
} mfxExtPartialBitstreamParam;
MFX_PACK_END()
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif
