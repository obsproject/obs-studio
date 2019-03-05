/******************************************************************************* *\

Copyright (C) 2017 Intel Corporation.  All rights reserved.

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

File Name: mfxscd.h

*******************************************************************************/
#ifndef __MFXSCD_H__
#define __MFXSCD_H__

#include "mfxenc.h"
#include "mfxplugin.h"

#define MFX_ENC_SCD_PLUGIN_VERSION 1

#ifdef __cplusplus
extern "C" {
#endif

static const mfxPluginUID MFX_PLUGINID_ENC_SCD = {{ 0xdf, 0xc2, 0x15, 0xb3, 0xe3, 0xd3, 0x90, 0x4d, 0x7f, 0xa5, 0x04, 0x12, 0x7e, 0xf5, 0x64, 0xd5 }};

/* SCD Extended Buffer Ids */
enum {
    MFX_EXTBUFF_SCD = MFX_MAKEFOURCC('S','C','D',' ')
};

/* SceneType */
enum {
    MFX_SCD_SCENE_SAME        = 0x00,
    MFX_SCD_SCENE_NEW_FIELD_1 = 0x01,
    MFX_SCD_SCENE_NEW_FIELD_2 = 0x02,
    MFX_SCD_SCENE_NEW_PICTURE = MFX_SCD_SCENE_NEW_FIELD_1 | MFX_SCD_SCENE_NEW_FIELD_2
};

typedef struct {
    mfxExtBuffer Header;

    mfxU16 SceneType;
    mfxU16 reserved[27];
} mfxExtSCD;

#ifdef __cplusplus
} // extern "C"
#endif

#endif

