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

#ifndef __SYSMEM_ALLOCATOR_H__
#define __SYSMEM_ALLOCATOR_H__

#include <stdlib.h>
#include "base_allocator.h"

struct sBuffer
{
    mfxU32      id;
    mfxU32      nbytes;
    mfxU16      type;
};

struct sFrame
{
    mfxU32          id;
    mfxFrameInfo    info;
};

struct SysMemAllocatorParams : mfxAllocatorParams
{
    SysMemAllocatorParams()
        : mfxAllocatorParams(), pBufferAllocator(NULL) { }
    MFXBufferAllocator *pBufferAllocator;
};

class SysMemFrameAllocator: public BaseFrameAllocator
{
public:
    SysMemFrameAllocator();
    virtual ~SysMemFrameAllocator();

    virtual mfxStatus Init(mfxAllocatorParams *pParams);
    virtual mfxStatus Close();
    virtual mfxStatus LockFrame(mfxMemId mid, mfxFrameData *ptr);
    virtual mfxStatus UnlockFrame(mfxMemId mid, mfxFrameData *ptr);
    virtual mfxStatus GetFrameHDL(mfxMemId mid, mfxHDL *handle);

protected:
    virtual mfxStatus CheckRequestType(mfxFrameAllocRequest *request);
    virtual mfxStatus ReleaseResponse(mfxFrameAllocResponse *response);
    virtual mfxStatus AllocImpl(mfxFrameAllocRequest *request, mfxFrameAllocResponse *response);

    MFXBufferAllocator *m_pBufferAllocator;
    bool m_bOwnBufferAllocator;
};

class SysMemBufferAllocator : public MFXBufferAllocator
{
public:
    SysMemBufferAllocator();
    virtual ~SysMemBufferAllocator();
    virtual mfxStatus AllocBuffer(mfxU32 nbytes, mfxU16 type, mfxMemId *mid);
    virtual mfxStatus LockBuffer(mfxMemId mid, mfxU8 **ptr);
    virtual mfxStatus UnlockBuffer(mfxMemId mid);
    virtual mfxStatus FreeBuffer(mfxMemId mid);
};

#endif // __SYSMEM_ALLOCATOR_H__