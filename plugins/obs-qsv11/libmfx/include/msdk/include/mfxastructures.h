/*******************************************************************************

Copyright (C) 2013 Intel Corporation.  All rights reserved.

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

File Name: mfxastructures.h

*******************************************************************************/
#ifndef __MFXASTRUCTURES_H__
#define __MFXASTRUCTURES_H__
#include "mfxcommon.h"

#pragma warning(disable: 4201)

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/* CodecId */
enum {
    MFX_CODEC_AAC         =MFX_MAKEFOURCC('A','A','C',' '),
    MFX_CODEC_MP3         =MFX_MAKEFOURCC('M','P','3',' ')
};

enum {
    /* AAC Profiles & Levels */
    MFX_PROFILE_AAC_LC          =2,
    MFX_PROFILE_AAC_LTP         =4,
    MFX_PROFILE_AAC_MAIN        =1,
    MFX_PROFILE_AAC_SSR         =3,
    MFX_PROFILE_AAC_HE          =5,
    MFX_PROFILE_AAC_ALS         =0x20,
    MFX_PROFILE_AAC_BSAC        =22,
    MFX_PROFILE_AAC_PS          =29,

    /*MPEG AUDIO*/
    MFX_AUDIO_MPEG1_LAYER1      =0x00000110, 
    MFX_AUDIO_MPEG1_LAYER2      =0x00000120,
    MFX_AUDIO_MPEG1_LAYER3      =0x00000140,
    MFX_AUDIO_MPEG2_LAYER1      =0x00000210,
    MFX_AUDIO_MPEG2_LAYER2      =0x00000220,
    MFX_AUDIO_MPEG2_LAYER3      =0x00000240
};

/*AAC HE decoder down sampling*/
enum {
    MFX_AUDIO_AAC_HE_DWNSMPL_OFF=0,
    MFX_AUDIO_AAC_HE_DWNSMPL_ON= 1
};

/* AAC decoder support of PS */
enum {
    MFX_AUDIO_AAC_PS_DISABLE=   0,
    MFX_AUDIO_AAC_PS_PARSER=    1,
    MFX_AUDIO_AAC_PS_ENABLE_BL= 111,
    MFX_AUDIO_AAC_PS_ENABLE_UR= 411
};

/*AAC decoder SBR support*/
enum {
    MFX_AUDIO_AAC_SBR_DISABLE =  0,
    MFX_AUDIO_AAC_SBR_ENABLE=    1,
    MFX_AUDIO_AAC_SBR_UNDEF=     2
};

/*AAC header type*/
enum{
    MFX_AUDIO_AAC_ADTS=            1,
    MFX_AUDIO_AAC_ADIF=            2,
    MFX_AUDIO_AAC_RAW=             3,
};

/*AAC encoder stereo mode*/
enum 
{
    MFX_AUDIO_AAC_MONO=            0,
    MFX_AUDIO_AAC_LR_STEREO=       1,
    MFX_AUDIO_AAC_MS_STEREO=       2,
    MFX_AUDIO_AAC_JOINT_STEREO=    3
};

typedef struct {
    mfxU32                CodecId;
    mfxU16                CodecProfile;
    mfxU16                CodecLevel;

    mfxU32  Bitrate;
    mfxU32  SampleFrequency;
    mfxU16  NumChannel;
    mfxU16  BitPerSample;

    mfxU16                reserved1[22]; 

    union {    
        struct {   /* AAC Decoding Options */
            mfxU16       FlagPSSupportLev;
            mfxU16       Layer;
            mfxU16       AACHeaderDataSize;
            mfxU8        AACHeaderData[64];
        };
        struct {   /* AAC Encoding Options */
            mfxU16       OutputFormat;
            mfxU16       StereoMode;
            mfxU16       reserved2[61]; 
        };
    };
} mfxAudioInfoMFX;

typedef struct {
    mfxU16  AsyncDepth;
    mfxU16  Protected;
    mfxU16  reserved[14]; 

    mfxAudioInfoMFX   mfx;
    mfxExtBuffer**    ExtParam;
    mfxU16            NumExtParam;
} mfxAudioParam;

typedef struct {
    mfxU32  SuggestedInputSize;
    mfxU32  SuggestedOutputSize;
    mfxU32  reserved[6];
} mfxAudioAllocRequest;

typedef struct {
    mfxU64  TimeStamp; /* 1/90KHz */
    mfxU16  Locked;
    mfxU16  NumChannels;
    mfxU32  SampleFrequency;
    mfxU16  BitPerSample;
    mfxU16  reserved1[7]; 

    mfxU8*  Data;
    mfxU32  reserved2;
    mfxU32  DataLength;
    mfxU32  MaxLength;

    mfxU32  NumExtParam;
    mfxExtBuffer **ExtParam;
} mfxAudioFrame;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif


