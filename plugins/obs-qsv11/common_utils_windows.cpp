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

#include "common_utils.h"

// ATTENTION: If D3D surfaces are used, DX9_D3D or DX11_D3D must be set in project settings or hardcoded here

#ifdef DX9_D3D
#include "common_directx.h"
#elif DX11_D3D
#include "common_directx11.h"
#include "common_directx9.h"
#endif

/* =======================================================
 * Windows implementation of OS-specific utility functions
 */

mfxStatus Initialize(mfxIMPL impl, mfxVersion ver, MFXVideoSession* pSession, mfxFrameAllocator* pmfxAllocator, mfxHDL *deviceHandle, bool bCreateSharedHandles, bool dx9hack)
{
    bCreateSharedHandles; // (Hugh) Currently unused
    pmfxAllocator; // (Hugh) Currently unused

    mfxStatus sts = MFX_ERR_NONE;

    // If mfxFrameAllocator is provided it means we need to setup DirectX device and memory allocator
    if (pmfxAllocator && !dx9hack) {
        // Initialize Intel Media SDK Session
        sts = pSession->Init(impl, &ver);
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

        // Create DirectX device context
        if (deviceHandle == NULL || *deviceHandle == NULL) {
            sts = CreateHWDevice(*pSession, deviceHandle, NULL, bCreateSharedHandles);
            MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
        }

        if (deviceHandle == NULL || *deviceHandle == NULL) return MFX_ERR_DEVICE_FAILED;

        // Provide device manager to Media SDK
        sts = pSession->SetHandle(DEVICE_MGR_TYPE, *deviceHandle);
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

        pmfxAllocator->pthis  = *pSession; // We use Media SDK session ID as the allocation identifier
        pmfxAllocator->Alloc  = simple_alloc;
        pmfxAllocator->Free   = simple_free;
        pmfxAllocator->Lock   = simple_lock;
        pmfxAllocator->Unlock = simple_unlock;
        pmfxAllocator->GetHDL = simple_gethdl;

        // Since we are using video memory we must provide Media SDK with an external allocator
        sts = pSession->SetFrameAllocator(pmfxAllocator);
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    } else if (pmfxAllocator && dx9hack) {
        // Initialize Intel Media SDK Session
        sts = pSession->Init(impl, &ver);
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

        // Create DirectX device context
        if (deviceHandle == NULL || *deviceHandle == NULL ) {
            sts = DX9_CreateHWDevice(*pSession, deviceHandle, NULL, false);
            MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
        }
        if (*deviceHandle == NULL) return MFX_ERR_DEVICE_FAILED;

        // Provide device manager to Media SDK
        sts = pSession->SetHandle(MFX_HANDLE_D3D9_DEVICE_MANAGER, *deviceHandle);
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

        pmfxAllocator->pthis  = *pSession; // We use Media SDK session ID as the allocation identifier
        pmfxAllocator->Alloc  = dx9_simple_alloc;
        pmfxAllocator->Free   = dx9_simple_free;
        pmfxAllocator->Lock   = dx9_simple_lock;
        pmfxAllocator->Unlock = dx9_simple_unlock;
        pmfxAllocator->GetHDL = dx9_simple_gethdl;

        // Since we are using video memory we must provide Media SDK with an external allocator
        sts = pSession->SetFrameAllocator(pmfxAllocator);
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    } else {
        // Initialize Intel Media SDK Session
        sts = pSession->Init(impl, &ver);
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
    }
    return sts;
}

void Release()
{
#if defined(DX9_D3D) || defined(DX11_D3D)
    CleanupHWDevice();
    DX9_CleanupHWDevice();
#endif
}

void mfxGetTime(mfxTime* timestamp)
{
    QueryPerformanceCounter(timestamp);
}

double TimeDiffMsec(mfxTime tfinish, mfxTime tstart)
{
    static LARGE_INTEGER tFreq = { 0 };

    if (!tFreq.QuadPart) QueryPerformanceFrequency(&tFreq);

    double freq = (double)tFreq.QuadPart;
    return 1000.0 * ((double)tfinish.QuadPart - (double)tstart.QuadPart) / freq;
}

/* (Hugh) Functions currently unused */
#if 0
void ClearYUVSurfaceVMem(mfxMemId memId)
{
#if defined(DX9_D3D) || defined(DX11_D3D)
    ClearYUVSurfaceD3D(memId);
#endif
}

void ClearRGBSurfaceVMem(mfxMemId memId)
{
#if defined(DX9_D3D) || defined(DX11_D3D)
    ClearRGBSurfaceD3D(memId);
#endif
}
#endif
