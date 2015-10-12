/*****************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or
nondisclosure agreement with Intel Corporation and may not be copied
or disclosed except in accordance with the terms of that agreement.
Copyright(c) 2005-2014 Intel Corporation. All Rights Reserved.

*****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MSDK_FOPEN(FH, FN, M)           { FH=fopen(FN,M); }
#define MSDK_SLEEP(X)                   { usleep(1000*(X)); }

typedef timespec mfxTime;
