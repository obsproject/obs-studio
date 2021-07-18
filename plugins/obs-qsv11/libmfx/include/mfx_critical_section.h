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
