// Copyright (c) 2013-2020 Intel Corporation
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

#pragma once
#include "mfxdefs.h"
#include <cstring>
#include <cstdio>

#if defined(MFX_DISPATCHER_LOG)
#include <string>
#include <string.h>
#endif

#define MAX_PLUGIN_PATH 4096
#define MAX_PLUGIN_NAME 4096

#if _MSC_VER < 1400
#define wcscpy_s(to,to_size, from) wcscpy(to, from)
#define wcscat_s(to,to_size, from) wcscat(to, from)
#endif

// declare library module's handle
typedef void * mfxModuleHandle;

typedef void (MFX_CDECL * mfxFunctionPointer)(void);

// Tracer uses lib loading from Program Files logic (via Dispatch reg key) to make dispatcher load tracer dll.
// With DriverStore loading put at 1st place, dispatcher loads real lib before it finds tracer dll.
// This workaround explicitly checks tracer presence in Dispatch reg key and loads tracer dll before the search for lib in all other places.
#define MFX_TRACER_WA_FOR_DS 1
