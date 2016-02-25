/*******************************************************************************

Copyright (C) 2013 Intel Corporation.  All rights reserved.

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

File Name: mfxsession.h

*******************************************************************************/
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
mfxStatus MFX_CDECL MFXDoWork(mfxSession session);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

