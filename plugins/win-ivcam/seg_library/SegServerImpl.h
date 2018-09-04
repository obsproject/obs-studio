/*
Copyright (c) 2015-2016, Intel Corporation

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef SEG_SERVER_IMPL_H_
#define SEG_SERVER_IMPL_H_

#include "Dependencies.h"
#include "SegImage.h"
#include "SegServer.h"

#include "seg_service.h"

typedef struct
{
	int width;
	int height;
	int pitch;
	long long timestamp;
	int frameNumber;
} FrameHeader;

class SegServerImpl : public SegServer
{
private:
    static SegServerImpl* instance;
    ISegProc* m_server;

    bool m_comInit;
    bool m_serviceConnected;

    LPWSTR m_bufferName;
    LPCTSTR m_sharedBuffer;

    SegServerImpl();
    SegServerImpl(SegServerImpl const& src) = delete;
    SegServerImpl& operator=(const SegServerImpl & src) = delete;

public:
    virtual ~SegServerImpl();
    static SegServer* CreateServer();
    static SegServer* GetServerInstance();

    ServiceStatus Init();
    ServiceStatus Stop();
    void SetFps(int fps) override;
    int GetFps() override;
    void SetIVCAMMotionRangeTradeOff(int value) override;
    int GetIVCAMMotionRangeTradeOff() override;

    ServiceStatus GetFrame(SegImage** image);
};

#endif // SEG_SERVER_IMPL_H_
