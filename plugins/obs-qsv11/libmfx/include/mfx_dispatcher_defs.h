// Copyright (c) 2013-2019 Intel Corporation
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

#if defined(_WIN32) || defined(_WIN64)
typedef wchar_t  msdk_disp_char;
#define MSDK2WIDE(x) x

#if _MSC_VER >= 1400
    #define msdk_disp_char_cpy_s(to, to_size, from) wcscpy_s(to,to_size, from)
#else
    #define msdk_disp_char_cpy_s(to, to_size, from) wcscpy(to, from)
#endif

#else
typedef char msdk_disp_char;
//#define msdk_disp_char_cpy_s(to, to_size, from) strcpy(to, from)

inline void msdk_disp_char_cpy_s(char * to, size_t to_size, const char * from)
{
    snprintf(to, to_size, "%s", from);
}

#if defined(MFX_DISPATCHER_LOG)
#define MSDK2WIDE(x) getWideString(x).c_str()

inline std::wstring getWideString(const char * string)
{
    size_t len = strlen(string);
    return std::wstring(string, string + len);
}
#else
    #define MSDK2WIDE(x) x  
#endif

#endif

#if defined(__GNUC__) && !defined(_WIN32) && !defined(_WIN64)
#define  sscanf_s  sscanf
#define  swscanf_s swscanf
#endif


// declare library module's handle
typedef void * mfxModuleHandle;

typedef void (MFX_CDECL * mfxFunctionPointer)(void);
