/* ****************************************************************************** *\

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2013 Intel Corporation. All Rights Reserved.

File Name: mfxwidi.h

\* ****************************************************************************** */


/* This file contains declaration of private Media SDK API extension for WiDi support. 
IT SHOULD NOT be released as part of public development package. */

#ifndef __MFXWIDI_H__
#define __MFXWIDI_H__
#include "mfxstructures.h"

extern "C" {
enum {
    MFX_EXTBUFF_ENCODER_WIDI_USAGE = MFX_MAKEFOURCC('W','I','D','I')
};

typedef struct {
    mfxExtBuffer    Header; 
    mfxU32          reserved[30];
} mfxExtAVCEncoderWiDiUsage;

#ifdef __cplusplus
} // extern "C"
#endif

#endif
