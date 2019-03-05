/* ****************************************************************************** *\

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2013-2014 Intel Corporation. All Rights Reserved.

File Name: mfxhdcp.h

\* ****************************************************************************** */
#ifndef __MFXHDCP_H__
#define __MFXHDCP_H__
#include "mfxstructures.h"
#include "mfxpcp.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/* Protected in mfxVideoParam */
enum {
    MFX_PROTECTION_HDCP     =   0x0004,
};

#ifdef __cplusplus
} // extern "C"
#endif /* __cplusplus */
#endif

