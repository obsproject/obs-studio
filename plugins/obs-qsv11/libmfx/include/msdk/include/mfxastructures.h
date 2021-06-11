// Copyright (c) 2017-2019 Intel Corporation
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
#ifndef __MFXASTRUCTURES_H__
#define __MFXASTRUCTURES_H__
#include "mfxcommon.h"

#if !defined (__GNUC__)
#pragma warning(disable: 4201)
#endif

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

MFX_PACK_BEGIN_USUAL_STRUCT()
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
MFX_PACK_END()

MFX_PACK_BEGIN_STRUCT_W_PTR()
typedef struct {
    mfxU16  AsyncDepth;
    mfxU16  Protected;
    mfxU16  reserved[14]; 

    mfxAudioInfoMFX   mfx;
    mfxExtBuffer**    ExtParam;
    mfxU16            NumExtParam;
} mfxAudioParam;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxU32  SuggestedInputSize;
    mfxU32  SuggestedOutputSize;
    mfxU32  reserved[6];
} mfxAudioAllocRequest;
MFX_PACK_END()

MFX_PACK_BEGIN_STRUCT_W_L_TYPE()
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
MFX_PACK_END()

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif


