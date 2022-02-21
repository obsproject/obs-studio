// Copyright (c) 2017 Intel Corporation
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
#ifndef __MFXSESSION_H__
#define __MFXSESSION_H__
#include "mfxcommon.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/* Global Functions */
typedef struct _mfxSession *mfxSession;
mfxStatus MFX_CDECL MFXInit(mfxIMPL impl, mfxVersion *ver, mfxSession *session);
mfxStatus MFX_CDECL MFXInitEx(mfxInitParam par, mfxSession *session);
mfxStatus MFX_CDECL MFXClose(mfxSession session);

mfxStatus MFX_CDECL MFXQueryIMPL(mfxSession session, mfxIMPL *impl);
mfxStatus MFX_CDECL MFXQueryVersion(mfxSession session, mfxVersion *version);

mfxStatus MFX_CDECL MFXJoinSession(mfxSession session, mfxSession child);
mfxStatus MFX_CDECL MFXDisjoinSession(mfxSession session);
mfxStatus MFX_CDECL MFXCloneSession(mfxSession session, mfxSession *clone);
mfxStatus MFX_CDECL MFXSetPriority(mfxSession session, mfxPriority priority);
mfxStatus MFX_CDECL MFXGetPriority(mfxSession session, mfxPriority *priority);
MFX_DEPRECATED mfxStatus MFX_CDECL MFXDoWork(mfxSession session);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

