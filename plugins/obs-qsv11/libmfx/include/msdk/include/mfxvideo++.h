/* ****************************************************************************** *\

Copyright (C) 2007-2013 Intel Corporation.  All rights reserved.

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


File Name: mfxvideo++.h

\* ****************************************************************************** */

#ifndef __MFXVIDEOPLUSPLUS_H
#define __MFXVIDEOPLUSPLUS_H

#include "mfxvideo.h"

class MFXVideoSession
{
public:
    MFXVideoSession(void) { m_session = (mfxSession) 0; }
    virtual ~MFXVideoSession(void) { Close(); }

    virtual mfxStatus Init(mfxIMPL impl, mfxVersion *ver) { return MFXInit(impl, ver, &m_session); }
    virtual mfxStatus Close(void)
    {
        mfxStatus mfxRes;
        mfxRes = MFXClose(m_session); m_session = (mfxSession) 0;
        return mfxRes;
    }

    virtual mfxStatus QueryIMPL(mfxIMPL *impl) { return MFXQueryIMPL(m_session, impl); }
    virtual mfxStatus QueryVersion(mfxVersion *version) { return MFXQueryVersion(m_session, version); }

    virtual mfxStatus JoinSession(mfxSession child_session) { return MFXJoinSession(m_session, child_session);}
    virtual mfxStatus DisjoinSession( ) { return MFXDisjoinSession(m_session);}
    virtual mfxStatus CloneSession( mfxSession *clone) { return MFXCloneSession(m_session, clone);}
    virtual mfxStatus SetPriority( mfxPriority priority) { return MFXSetPriority(m_session, priority);}
    virtual mfxStatus GetPriority( mfxPriority *priority) { return MFXGetPriority(m_session, priority);}

    virtual mfxStatus SetBufferAllocator(mfxBufferAllocator *allocator) { return MFXVideoCORE_SetBufferAllocator(m_session, allocator); }
    virtual mfxStatus SetFrameAllocator(mfxFrameAllocator *allocator) { return MFXVideoCORE_SetFrameAllocator(m_session, allocator); }
    virtual mfxStatus SetHandle(mfxHandleType type, mfxHDL hdl) { return MFXVideoCORE_SetHandle(m_session, type, hdl); }
    virtual mfxStatus GetHandle(mfxHandleType type, mfxHDL *hdl) { return MFXVideoCORE_GetHandle(m_session, type, hdl); }

    virtual mfxStatus SyncOperation(mfxSyncPoint syncp, mfxU32 wait) { return MFXVideoCORE_SyncOperation(m_session, syncp, wait); }

    virtual operator mfxSession (void) { return m_session; }

protected:

    mfxSession m_session;                                       // (mfxSession) handle to the owning session
private:
    MFXVideoSession(const MFXVideoSession &);
    void operator=(MFXVideoSession &);
};

class MFXVideoENCODE
{
public:

    MFXVideoENCODE(mfxSession session) { m_session = session; }
    virtual ~MFXVideoENCODE(void) { Close(); }

    virtual mfxStatus Query(mfxVideoParam *in, mfxVideoParam *out) { return MFXVideoENCODE_Query(m_session, in, out); }
    virtual mfxStatus QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest *request) { return MFXVideoENCODE_QueryIOSurf(m_session, par, request); }
    virtual mfxStatus Init(mfxVideoParam *par) { return MFXVideoENCODE_Init(m_session, par); }
    virtual mfxStatus Reset(mfxVideoParam *par) { return MFXVideoENCODE_Reset(m_session, par); }
    virtual mfxStatus Close(void) { return MFXVideoENCODE_Close(m_session); }

    virtual mfxStatus GetVideoParam(mfxVideoParam *par) { return MFXVideoENCODE_GetVideoParam(m_session, par); }
    virtual mfxStatus GetEncodeStat(mfxEncodeStat *stat) { return MFXVideoENCODE_GetEncodeStat(m_session, stat); }

    virtual mfxStatus EncodeFrameAsync(mfxEncodeCtrl *ctrl, mfxFrameSurface1 *surface, mfxBitstream *bs, mfxSyncPoint *syncp) { return MFXVideoENCODE_EncodeFrameAsync(m_session, ctrl, surface, bs, syncp); }

protected:

    mfxSession m_session;                                       // (mfxSession) handle to the owning session
};

class MFXVideoDECODE
{
public:

    MFXVideoDECODE(mfxSession session) { m_session = session; }
    virtual ~MFXVideoDECODE(void) { Close(); }

    virtual mfxStatus Query(mfxVideoParam *in, mfxVideoParam *out) { return MFXVideoDECODE_Query(m_session, in, out); }
    virtual mfxStatus DecodeHeader(mfxBitstream *bs, mfxVideoParam *par) { return MFXVideoDECODE_DecodeHeader(m_session, bs, par); }
    virtual mfxStatus QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest *request) { return MFXVideoDECODE_QueryIOSurf(m_session, par, request); }
    virtual mfxStatus Init(mfxVideoParam *par) { return MFXVideoDECODE_Init(m_session, par); }
    virtual mfxStatus Reset(mfxVideoParam *par) { return MFXVideoDECODE_Reset(m_session, par); }
    virtual mfxStatus Close(void) { return MFXVideoDECODE_Close(m_session); }

    virtual mfxStatus GetVideoParam(mfxVideoParam *par) { return MFXVideoDECODE_GetVideoParam(m_session, par); }

    virtual mfxStatus GetDecodeStat(mfxDecodeStat *stat) { return MFXVideoDECODE_GetDecodeStat(m_session, stat); }
    virtual mfxStatus GetPayload(mfxU64 *ts, mfxPayload *payload) {return MFXVideoDECODE_GetPayload(m_session, ts, payload); }
    virtual mfxStatus SetSkipMode(mfxSkipMode mode) { return MFXVideoDECODE_SetSkipMode(m_session, mode); }
    virtual mfxStatus DecodeFrameAsync(mfxBitstream *bs, mfxFrameSurface1 *surface_work, mfxFrameSurface1 **surface_out, mfxSyncPoint *syncp) { return MFXVideoDECODE_DecodeFrameAsync(m_session, bs, surface_work, surface_out, syncp); }

protected:

    mfxSession m_session;                                       // (mfxSession) handle to the owning session
};

class MFXVideoVPP
{
public:

    MFXVideoVPP(mfxSession session) { m_session = session; }
    virtual ~MFXVideoVPP(void) { Close(); }

    virtual mfxStatus Query(mfxVideoParam *in, mfxVideoParam *out) { return MFXVideoVPP_Query(m_session, in, out); }
    virtual mfxStatus QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest request[2]) { return MFXVideoVPP_QueryIOSurf(m_session, par, request); }
    virtual mfxStatus Init(mfxVideoParam *par) { return MFXVideoVPP_Init(m_session, par); }
    virtual mfxStatus Reset(mfxVideoParam *par) { return MFXVideoVPP_Reset(m_session, par); }
    virtual mfxStatus Close(void) { return MFXVideoVPP_Close(m_session); }

    virtual mfxStatus GetVideoParam(mfxVideoParam *par) { return MFXVideoVPP_GetVideoParam(m_session, par); }
    virtual mfxStatus GetVPPStat(mfxVPPStat *stat) { return MFXVideoVPP_GetVPPStat(m_session, stat); }
    virtual mfxStatus RunFrameVPPAsync(mfxFrameSurface1 *in, mfxFrameSurface1 *out, mfxExtVppAuxData *aux, mfxSyncPoint *syncp) { return MFXVideoVPP_RunFrameVPPAsync(m_session, in, out, aux, syncp); }

protected:

    mfxSession m_session;                                       // (mfxSession) handle to the owning session
};

#endif // __MFXVIDEOPLUSPLUS_H
