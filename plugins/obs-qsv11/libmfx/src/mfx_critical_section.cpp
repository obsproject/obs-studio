// Copyright (c) 2012-2019 Intel Corporation
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

#include "mfx_critical_section.h"

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
