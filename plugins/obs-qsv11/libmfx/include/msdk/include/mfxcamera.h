/******************************************************************************* *\

Copyright (C) 2014-2016 Intel Corporation.  All rights reserved.

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

File Name: mfxcamera.h

*******************************************************************************/
#ifndef __MFXCAMERA_H__
#define __MFXCAMERA_H__
#include "mfxcommon.h"

#if !defined (__GNUC__)
#pragma warning(disable: 4201)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Camera Extended Buffer Ids */
enum {
    MFX_EXTBUF_CAM_GAMMA_CORRECTION             = MFX_MAKEFOURCC('C','G','A','M'),
    MFX_EXTBUF_CAM_WHITE_BALANCE                = MFX_MAKEFOURCC('C','W','B','L'),
    MFX_EXTBUF_CAM_HOT_PIXEL_REMOVAL            = MFX_MAKEFOURCC('C','H','P','R'),
    MFX_EXTBUF_CAM_BLACK_LEVEL_CORRECTION       = MFX_MAKEFOURCC('C','B','L','C'),
    MFX_EXTBUF_CAM_VIGNETTE_CORRECTION          = MFX_MAKEFOURCC('C','V','G','T'),
    MFX_EXTBUF_CAM_BAYER_DENOISE                = MFX_MAKEFOURCC('C','D','N','S'),
    MFX_EXTBUF_CAM_COLOR_CORRECTION_3X3         = MFX_MAKEFOURCC('C','C','3','3'),
    MFX_EXTBUF_CAM_PADDING                      = MFX_MAKEFOURCC('C','P','A','D'),
    MFX_EXTBUF_CAM_PIPECONTROL                  = MFX_MAKEFOURCC('C','P','P','C'),
    MFX_EXTBUF_CAM_FORWARD_GAMMA_CORRECTION     = MFX_MAKEFOURCC('C','F','G','C'),
    MFX_EXTBUF_CAM_LENS_GEOM_DIST_CORRECTION    = MFX_MAKEFOURCC('C','L','G','D'),
    MFX_EXTBUF_CAM_3DLUT                        = MFX_MAKEFOURCC('C','L','U','T'),
    MFX_EXTBUF_CAM_TOTAL_COLOR_CONTROL          = MFX_MAKEFOURCC('C','T','C','C'),
    MFX_EXTBUF_CAM_CSC_YUV_RGB                  = MFX_MAKEFOURCC('C','C','Y','R')
};

typedef enum {
    MFX_CAM_GAMMA_VALUE      = 0x0001,
    MFX_CAM_GAMMA_LUT        = 0x0002,
} mfxCamGammaParam;

typedef struct {
    mfxExtBuffer    Header;
    mfxU16  Mode;
    mfxU16  reserved1;
    mfxF64  GammaValue;

    mfxU16  reserved2[3];
    mfxU16  NumPoints;
    mfxU16  GammaPoint[1024];
    mfxU16  GammaCorrected[1024];
    mfxU32  reserved3[4]; 
} mfxExtCamGammaCorrection;

typedef enum {
    MFX_CAM_WHITE_BALANCE_MANUAL   = 0x0001,
    MFX_CAM_WHITE_BALANCE_AUTO     = 0x0002
} mfxCamWhiteBalanceMode;

typedef struct {
    mfxExtBuffer    Header;
    mfxU32          Mode;
    mfxF64          R;
    mfxF64          G0;
    mfxF64          B;
    mfxF64          G1;
    mfxU32          reserved[8];
} mfxExtCamWhiteBalance;

typedef struct {
    mfxExtBuffer Header;
    mfxU16        R;
    mfxU16        G;
    mfxU16        B;
    mfxU16        C;
    mfxU16        M;
    mfxU16        Y;
    mfxU16        reserved[6];
} mfxExtCamTotalColorControl;

typedef struct {
    mfxExtBuffer Header;

    mfxF32       PreOffset[3];
    mfxF32       Matrix[3][3];
    mfxF32       PostOffset[3];
    mfxU16       reserved[30];
} mfxExtCamCscYuvRgb;

typedef struct {
    mfxExtBuffer    Header;
    mfxU16          PixelThresholdDifference;
    mfxU16          PixelCountThreshold;
} mfxExtCamHotPixelRemoval;

typedef struct {
    mfxExtBuffer    Header;
    mfxU16          R;
    mfxU16          G0;
    mfxU16          B;
    mfxU16          G1;
    mfxU32          reserved[4];
} mfxExtCamBlackLevelCorrection;

typedef struct {
    mfxU8 integer;
    mfxU8 mantissa;
} mfxCamVignetteCorrectionElement;

typedef struct {
    mfxCamVignetteCorrectionElement R;
    mfxCamVignetteCorrectionElement G0;
    mfxCamVignetteCorrectionElement B;
    mfxCamVignetteCorrectionElement G1;
} mfxCamVignetteCorrectionParam;

typedef struct {
    mfxExtBuffer    Header;

    mfxU32          Width;
    mfxU32          Height;
    mfxU32          Pitch;
    mfxU32          reserved[7];

    mfxCamVignetteCorrectionParam *CorrectionMap;
    
} mfxExtCamVignetteCorrection;

typedef struct {
    mfxExtBuffer    Header;
    mfxU16          Threshold;
    mfxU16          reserved[27];
} mfxExtCamBayerDenoise;

typedef struct {
    mfxExtBuffer    Header;
    mfxF64          CCM[3][3];
    mfxU32          reserved[4];
} mfxExtCamColorCorrection3x3;

typedef struct {
    mfxExtBuffer    Header;
    mfxU16 Top;
    mfxU16 Bottom;
    mfxU16 Left;
    mfxU16 Right;
    mfxU32 reserved[4];
} mfxExtCamPadding;

typedef enum {
    MFX_CAM_BAYER_BGGR   = 0x0000,
    MFX_CAM_BAYER_RGGB   = 0x0001,
    MFX_CAM_BAYER_GBRG   = 0x0002,
    MFX_CAM_BAYER_GRBG   = 0x0003
} mfxCamBayerFormat;

typedef struct {
    mfxExtBuffer    Header;
    mfxU16          RawFormat;
    mfxU16          reserved1;
    mfxU32          reserved[5];
} mfxExtCamPipeControl;

typedef struct{
    mfxU16 Pixel;
    mfxU16 Red;
    mfxU16 Green;
    mfxU16 Blue;
} mfxCamFwdGammaSegment;

typedef struct {
    mfxExtBuffer Header;

    mfxU16       reserved[19];
    mfxU16       NumSegments;
    union {
        mfxCamFwdGammaSegment* Segment;
        mfxU64 reserved1;
    };
} mfxExtCamFwdGamma;

typedef struct {
    mfxExtBuffer Header;

    mfxF32       a[3]; // [R, G, B]
    mfxF32       b[3]; // [R, G, B]
    mfxF32       c[3]; // [R, G, B]
    mfxF32       d[3]; // [R, G, B]
    mfxU16       reserved[36];
} mfxExtCamLensGeomDistCorrection;

/* LUTSize */
enum {
    MFX_CAM_3DLUT17_SIZE = (17 * 17 * 17),
    MFX_CAM_3DLUT33_SIZE = (33 * 33 * 33),
    MFX_CAM_3DLUT65_SIZE = (65 * 65 * 65)
};

typedef struct {
    mfxU16 R;
    mfxU16 G;
    mfxU16 B;
    mfxU16 Reserved;
} mfxCam3DLutEntry;

typedef struct {
    mfxExtBuffer Header;

    mfxU16 reserved[10];
    mfxU32 Size;
    union
    {
        mfxCam3DLutEntry* Table;
        mfxU64 reserved1;
    };
} mfxExtCam3DLut;

#ifdef __cplusplus
} // extern "C"
#endif

#endif

