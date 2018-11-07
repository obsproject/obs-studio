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

#include "general_allocator.h"

#if defined(_WIN32) || defined(_WIN64)
#include "d3d_allocator.h"
#include "d3d11_allocator.h"
#else
#include <stdarg.h>
#include "vaapi_allocator.h"
#endif

#include "sysmem_allocator.h"
#include "common_utils.h"

// Wrapper on standard allocator for concurrent allocation of
// D3D and system surfaces
GeneralAllocator::GeneralAllocator()
{
};
GeneralAllocator::~GeneralAllocator()
{
};
mfxStatus GeneralAllocator::Init(mfxAllocatorParams *pParams)
{
    mfxStatus sts = MFX_ERR_NONE;

#if defined(_WIN32) || defined(_WIN64)
    D3DAllocatorParams *d3dAllocParams = dynamic_cast<D3DAllocatorParams*>(pParams);
    if (d3dAllocParams)
        m_D3DAllocator.reset(new D3DFrameAllocator);
#if MFX_D3D11_SUPPORT
    D3D11AllocatorParams *d3d11AllocParams = dynamic_cast<D3D11AllocatorParams*>(pParams);
    if (d3d11AllocParams)
        m_D3DAllocator.reset(new D3D11FrameAllocator);
#endif
#endif

#ifdef LIBVA_SUPPORT
    vaapiAllocatorParams *vaapiAllocParams = dynamic_cast<vaapiAllocatorParams*>(pParams);
    if (vaapiAllocParams)
        m_D3DAllocator.reset(new vaapiFrameAllocator);
#endif

    if (m_D3DAllocator.get())
    {
        sts = m_D3DAllocator.get()->Init(pParams);
        MSDK_CHECK_STATUS(sts, "m_D3DAllocator.get failed");
    }

    m_SYSAllocator.reset(new SysMemFrameAllocator);
    sts = m_SYSAllocator.get()->Init(0);
    MSDK_CHECK_STATUS(sts, "m_SYSAllocator.get failed");

    return sts;
}
mfxStatus GeneralAllocator::Close()
{
    mfxStatus sts = MFX_ERR_NONE;
    if (m_D3DAllocator.get())
    {
        sts = m_D3DAllocator.get()->Close();
        MSDK_CHECK_STATUS(sts, "m_D3DAllocator.get failed");
    }

    sts = m_SYSAllocator.get()->Close();
    MSDK_CHECK_STATUS(sts, "m_SYSAllocator.get failed");

   return sts;
}

mfxStatus GeneralAllocator::LockFrame(mfxMemId mid, mfxFrameData *ptr)
{
    if (isD3DMid(mid) && m_D3DAllocator.get())
        return m_D3DAllocator.get()->Lock(m_D3DAllocator.get(), mid, ptr);
    else
        return m_SYSAllocator.get()->Lock(m_SYSAllocator.get(),mid, ptr);
}
mfxStatus GeneralAllocator::UnlockFrame(mfxMemId mid, mfxFrameData *ptr)
{
    if (isD3DMid(mid) && m_D3DAllocator.get())
        return m_D3DAllocator.get()->Unlock(m_D3DAllocator.get(), mid, ptr);
    else
        return m_SYSAllocator.get()->Unlock(m_SYSAllocator.get(),mid, ptr);
}

mfxStatus GeneralAllocator::GetFrameHDL(mfxMemId mid, mfxHDL *handle)
{
    if (isD3DMid(mid) && m_D3DAllocator.get())
        return m_D3DAllocator.get()->GetHDL(m_D3DAllocator.get(), mid, handle);
    else
        return m_SYSAllocator.get()->GetHDL(m_SYSAllocator.get(), mid, handle);
}

mfxStatus GeneralAllocator::ReleaseResponse(mfxFrameAllocResponse *response)
{
    // try to ReleaseResponse via D3D allocator
    if (isD3DMid(response->mids[0]) && m_D3DAllocator.get())
        return m_D3DAllocator.get()->Free(m_D3DAllocator.get(),response);
    else
        return m_SYSAllocator.get()->Free(m_SYSAllocator.get(), response);
}
mfxStatus GeneralAllocator::AllocImpl(mfxFrameAllocRequest *request, mfxFrameAllocResponse *response)
{
    mfxStatus sts;
    if ((request->Type & MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET || request->Type & MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET) && m_D3DAllocator.get())
    {
        sts = m_D3DAllocator.get()->Alloc(m_D3DAllocator.get(), request, response);
        MSDK_CHECK_NOT_EQUAL(MFX_ERR_NONE, sts, sts);
        StoreFrameMids(true, response);
    }
    else
    {
        sts = m_SYSAllocator.get()->Alloc(m_SYSAllocator.get(), request, response);
        MSDK_CHECK_NOT_EQUAL(MFX_ERR_NONE, sts, sts);
        StoreFrameMids(false, response);
    }
    return sts;
}
void    GeneralAllocator::StoreFrameMids(bool isD3DFrames, mfxFrameAllocResponse *response)
{
    for (mfxU32 i = 0; i < response->NumFrameActual; i++)
        m_Mids.insert(std::pair<mfxHDL, bool>(response->mids[i], isD3DFrames));
}
bool GeneralAllocator::isD3DMid(mfxHDL mid)
{
    std::map<mfxHDL, bool>::iterator it;
    it = m_Mids.find(mid);
    if (it == m_Mids.end())
        return false; // sys mem allocator will check validity of mid further
    else
        return it->second;
}
