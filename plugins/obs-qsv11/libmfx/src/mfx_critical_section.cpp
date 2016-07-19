/* ****************************************************************************** *\

Copyright (C) 2012-2013 Intel Corporation.  All rights reserved.

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

File Name: mfx_critical_section.cpp

\* ****************************************************************************** */

#include "mfx_critical_section.h"

#if defined(_WIN32) || defined(_WIN64)

#include <windows.h>
// SDK re-declares the following functions with different call declarator.
// We don't need them. Just redefine them to nothing.
#define _interlockedbittestandset fake_set
#define _interlockedbittestandreset fake_reset
#define _interlockedbittestandset64 fake_set64
#define _interlockedbittestandreset64 fake_reset64
#include <intrin.h>

#define MFX_WAIT() SwitchToThread()

// static section of the file
namespace
{

enum
{
    MFX_SC_IS_FREE = 0,
    MFX_SC_IS_TAKEN = 1
};

} // namespace

namespace MFX
{

mfxU32 mfxInterlockedCas32(mfxCriticalSection *pCSection, mfxU32 value_to_exchange, mfxU32 value_to_compare)
{
    return _InterlockedCompareExchange(pCSection, value_to_exchange, value_to_compare);
}

mfxU32 mfxInterlockedXchg32(mfxCriticalSection *pCSection, mfxU32 value)  
{ 
    return _InterlockedExchange(pCSection, value);
}

void mfxEnterCriticalSection(mfxCriticalSection *pCSection)
{
    while (MFX_SC_IS_TAKEN == mfxInterlockedCas32(pCSection,
                                                  MFX_SC_IS_TAKEN,
                                                  MFX_SC_IS_FREE))
    {
        MFX_WAIT();
    }
} // void mfxEnterCriticalSection(mfxCriticalSection *pCSection)

void mfxLeaveCriticalSection(mfxCriticalSection *pCSection)
{
    mfxInterlockedXchg32(pCSection, MFX_SC_IS_FREE);
} // void mfxLeaveCriticalSection(mfxCriticalSection *pCSection)

} // namespace MFX

#endif // #if defined(_WIN32) || defined(_WIN64)
