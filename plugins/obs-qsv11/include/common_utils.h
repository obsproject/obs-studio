/******************************************************************************\
Copyright (c) 2005-2018, Intel Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
This sample was distributed or derived from the Intel's Media Samples package.
The original version of this sample may be obtained from https://software.intel.com/en-us/intel-media-server-studio
or https://software.intel.com/en-us/media-client-solutions-support.
\**********************************************************************************/

#ifndef __COMMON_UTILS_H__
#define __COMMON_UTILS_H__

#include <iostream>
#include <sstream>
#include "mfxvideo++.h"
#include "general_allocator.h"

// =================================================================
// OS-specific definitions of types, macro, etc...
// The following should be defined:
//  - mfxTime
//  - MSDK_FOPEN
//  - MSDK_SLEEP
#if defined(_WIN32) || defined(_WIN64)
#include "windows_defs.h"
#elif defined(__linux__)
#include <stdio.h>
#include "linux_defs.h"
#endif

typedef char msdk_char;
typedef std::basic_stringstream<msdk_char> msdk_stringstream;
typedef std::basic_string<msdk_char> msdk_string;
#define msdk_printf   printf
#define msdk_err std::cerr

#define MSDK_STRING(x)  x
#define MSDK_PRINT_RET_MSG(ERR,MSG)			{msdk_stringstream tmpStr1;tmpStr1<<std::endl<<"[ERROR], sts=" \
												<<StatusToString(ERR)<<"("<<ERR<<")"<<", "<<__FUNCTION__<<", "<<MSG<<" at "<<__FILE__<<":"<<__LINE__<<std::endl;msdk_err<<tmpStr1.str();}
#define MSDK_CHECK_STATUS(X, MSG)           {if ((X) < MFX_ERR_NONE) {MSDK_PRINT_RET_MSG(X, MSG); return X;}}
#define MSDK_CHECK_POINTER(P, ...)          {if (!(P)) {msdk_stringstream tmpStr4;tmpStr4<<MSDK_STRING(#P)<<MSDK_STRING(" pointer is NULL");MSDK_PRINT_RET_MSG(MFX_ERR_NULL_PTR, tmpStr4.str().c_str());return __VA_ARGS__;}}
#define MSDK_IGNORE_MFX_STS(P, X)           {if ((X) == (P)) {P = MFX_ERR_NONE;}}
#define MSDK_ARRAY_LEN(value)               (sizeof(value) / sizeof(value[0]))
#define MSDK_SAFE_DELETE_ARRAY(P)           {if (P) {delete[] P; P = NULL;}}
#define MSDK_SAFE_RELEASE(X)                {if (X) { X->Release(); X = NULL; }}
#define MSDK_SAFE_FREE(X)                   {if (X) { free(X); X = NULL; }}
#define MSDK_ZERO_MEMORY(VAR)               {memset(&VAR, 0, sizeof(VAR));}
#define MSDK_ALIGN16(value)                 (((value + 15) >> 4) << 4) // round up to a multiple of 16
#define MSDK_ALIGN32(value)                 (((value + 31) >> 5) << 5) // round up to a multiple of 32
#define MSDK_MEMCPY_VAR(dstVarName, src, count) memcpy(&(dstVarName), (src), (count))
#define MSDK_CHECK_NOT_EQUAL(P, X, ERR)     {if ((X) != (P)) {msdk_stringstream tmpStr3;tmpStr3<<MSDK_STRING(#X)<<MSDK_STRING("!=")<<MSDK_STRING(#P)<<MSDK_STRING(" error"); \
    MSDK_PRINT_RET_MSG(ERR, tmpStr3.str().c_str()); return ERR;}}

#define MSDK_INVALID_SURF_IDX 0xFFFF

// Deprecated
#define MSDK_PRINT_RET_MSG_(ERR) {msdk_printf(MSDK_STRING("\nReturn on error: error code %d,\t%s\t%d\n\n"), (int)ERR, MSDK_STRING(__FILE__), __LINE__);}
#define MSDK_CHECK_RESULT(P, X, ERR)             {if ((X) > (P)) {MSDK_PRINT_RET_MSG_(ERR); return ERR;}}
#define MSDK_CHECK_RESULT_SAFE(P, X, ERR, ADD)   {if ((X) > (P)) {ADD; MSDK_PRINT_RET_MSG_(ERR); return ERR;}}

// Usage of the following two macros are only required for certain Windows DirectX11 use cases
#define WILL_READ  0x1000
#define WILL_WRITE 0x2000

#if defined(WIN32) || defined(WIN64)

enum {
	MFX_HANDLE_DEVICEWINDOW = 0x101 /* A handle to the render window */
}; //mfxHandleType

#ifndef D3D_SURFACES_SUPPORT
#define D3D_SURFACES_SUPPORT 1
#endif

#if defined(_WIN32) && !defined(MFX_D3D11_SUPPORT)
#include <sdkddkver.h>
#if (NTDDI_VERSION >= NTDDI_VERSION_FROM_WIN32_WINNT2(0x0602)) // >= _WIN32_WINNT_WIN8
#define MFX_D3D11_SUPPORT 1 // Enable D3D11 support if SDK allows
#else
#define MFX_D3D11_SUPPORT 0
#endif
#endif // #if defined(WIN32) && !defined(MFX_D3D11_SUPPORT)
#endif // #if defined(WIN32) || defined(WIN64)

#if defined(_WIN32) || defined(_WIN64)

#include <windows.h>
#include <process.h>

struct msdkMutexHandle
{
	CRITICAL_SECTION m_CritSec;
};

#else // #if defined(_WIN32) || defined(_WIN64)

#include <pthread.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>

struct msdkMutexHandle
{
	pthread_mutex_t m_mutex;
};

#endif // #if defined(_WIN32) || defined(_WIN64)

class MSDKMutex : public msdkMutexHandle
{
public:
	MSDKMutex(void);
	~MSDKMutex(void);

	mfxStatus Lock(void);
	mfxStatus Unlock(void);
	int Try(void);

private:
	MSDKMutex(const MSDKMutex&);
	void operator=(const MSDKMutex&);
};

class AutomaticMutex
{
public:
	AutomaticMutex(MSDKMutex& mutex);
	~AutomaticMutex(void);

private:
	mfxStatus Lock(void);
	mfxStatus Unlock(void);

	MSDKMutex& m_rMutex;
	bool m_bLocked;

private:
	AutomaticMutex(const AutomaticMutex&);
	void operator=(const AutomaticMutex&);
};

// =================================================================
// Utility functions, not directly tied to Media SDK functionality
//
msdk_string StatusToString(mfxStatus sts);

mfxU16 GetFreeSurfaceIndex(mfxFrameSurface1* pSurfacesPool, mfxU16 nPoolSize);

// For use with asynchronous task management
typedef struct {
	mfxBitstream mfxBS;
	mfxSyncPoint syncp;
} Task;

// Initialize Intel Media SDK Session, device/display and memory manager
mfxStatus Initialize(mfxIMPL impl, mfxVersion ver, MFXVideoSession* pSession, GeneralAllocator* pmfxAllocator, mfxHDL deviceHandle = NULL, bool bCreateSharedHandles = false, bool dx9hack = false);

// Release resources (device/display)
void Release();
//
#endif //__COMMON_UTILS_H_
