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

File Name: mfx_critical_section.h

\* ****************************************************************************** */

#if !defined(__MFX_CRITICAL_SECTION_H)
#define __MFX_CRITICAL_SECTION_H

#include <mfxdefs.h>

namespace MFX
{

// Just set "critical section" instance to zero for initialization.
typedef volatile mfxL32 mfxCriticalSection;

// Enter the global critical section.
void mfxEnterCriticalSection(mfxCriticalSection *pCSection);

// Leave the global critical section.
void mfxLeaveCriticalSection(mfxCriticalSection *pCSection);

class MFXAutomaticCriticalSection
{
public:
    // Constructor
    explicit MFXAutomaticCriticalSection(mfxCriticalSection *pCSection)
    {
        m_pCSection = pCSection;
        mfxEnterCriticalSection(m_pCSection);
    }

    // Destructor
    ~MFXAutomaticCriticalSection()
    {
        mfxLeaveCriticalSection(m_pCSection);
    }

protected:
    // Pointer to a critical section
    mfxCriticalSection *m_pCSection;

private:
    // unimplemented by intent to make this class non-copyable
    MFXAutomaticCriticalSection(const MFXAutomaticCriticalSection &);
    void operator=(const MFXAutomaticCriticalSection &);
};

} // namespace MFX

#endif // __MFX_CRITICAL_SECTION_H
