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

#include "SegServerImpl.h"

SegServerImpl* SegServerImpl::instance = nullptr;

SegServerImpl::SegServerImpl()
{
    m_comInit = false;
    m_serviceConnected = false;
    m_sharedBuffer = NULL;
}

SegServerImpl::~SegServerImpl() 
{
    if (m_server)
    {
        m_server->Release();
        m_server = nullptr;
    }
    instance = nullptr;
};

SegServer* SegServerImpl::CreateServer()
{
    if (instance == nullptr)
    {
        instance = new SegServerImpl();
        return instance;
    }
    else
    {
        return nullptr;
    }
}

SegServer* SegServerImpl::GetServerInstance()
{
    return instance;
}

SegServerImpl::ServiceStatus SegServerImpl::Init()
{
    if (m_comInit && m_serviceConnected)
        return SERVICE_REINIT_ERROR;
    GUID guid = { 0 };
    m_bufferName = new wchar_t[40]();
    CoCreateGuid(&guid);
    StringFromGUID2(guid, m_bufferName, 40);
    if (!m_comInit)
    {
        HRESULT comInit = CoInitializeEx(0, COINIT_MULTITHREADED);
        if (FAILED(comInit))
        {
            comInit = CoInitialize(0);
            if (FAILED(comInit))
            {
                return COM_LIB_INIT_ERROR;
            }
        }
        m_comInit = true;
    }

    if (!m_serviceConnected)
    {
        HRESULT instanceCreate = CoCreateInstance(CLSID_SegProc, NULL, CLSCTX_LOCAL_SERVER, IID_ISegProc, (void**)&m_server);
        if (FAILED(instanceCreate)) {
            return SERVICE_INIT_ERROR;
        }

        HRESULT serverInit = m_server->Init(m_bufferName);
        if (FAILED(serverInit)) {
            return SERVICE_INIT_ERROR;
        }
        m_serviceConnected = true;
    }
    return SERVICE_NO_ERROR;
}

SegServerImpl::ServiceStatus SegServerImpl::Stop()
{
    HRESULT errCode = m_server->Stop();
    if (FAILED(errCode)) {
        return SERVICE_FUNC_ERROR;
    }
    return SERVICE_NO_ERROR;
}

void SegServerImpl::SetFps(int fps)
{
    m_server->SetFps(fps);
}

int SegServerImpl::GetFps()
{
    int outFps = -1;
    m_server->GetFps(&outFps);
    return outFps;
}

void SegServerImpl::SetIVCAMMotionRangeTradeOff(int value)
{
    m_server->SetIVCAMMotionRangeTradeOff(value);
}

int SegServerImpl::GetIVCAMMotionRangeTradeOff()
{
    int outPropertyValue = -1;
    m_server->GetIVCAMMotionRangeTradeOff(&outPropertyValue);
    return outPropertyValue;
}

SegServerImpl::ServiceStatus SegServerImpl::GetFrame(SegImage** image)
{
    ServiceStatus res = SERVICE_NO_ERROR;
    int frameId = -1;
    int bufferRealloc = -1;
    int frameSize = -1;
    if (FAILED(m_server->LockBuffer(&frameId, &frameSize, &bufferRealloc)))
    {
        return SHARED_MEMORY_ERROR;
    }
    if (bufferRealloc)
    {
        HANDLE hMapFile = OpenFileMapping(FILE_MAP_READ, FALSE, m_bufferName);

        if (hMapFile == NULL)
        {
            m_server->UnlockBuffer();
            return SHARED_MEMORY_ERROR;
        }

        m_sharedBuffer = (LPTSTR)MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, frameSize * 2);

        if (m_sharedBuffer == NULL)
        {
            CloseHandle(hMapFile);
            m_server->UnlockBuffer();
            return SHARED_MEMORY_ERROR;
        }
        CloseHandle(hMapFile);
    }

	if(m_sharedBuffer && frameId != -1 && frameSize != -1)
	{
		size_t offset = (size_t)frameId * frameSize;
		//int width = ((int*)(m_sharedBuffer + offset))[0];
		//int height = ((int*)(m_sharedBuffer + offset))[1];
		//int pitch = ((int*)(m_sharedBuffer + offset))[2];
		FrameHeader* fhPtr = (FrameHeader*)(m_sharedBuffer + offset);
		int width = fhPtr->width;
		int height = fhPtr->height;
		int pitch = fhPtr->pitch;
		long long timestamp = fhPtr->timestamp;
		int frameNumber = fhPtr->frameNumber;
		void* data = (void*)(m_sharedBuffer + offset + sizeof(FrameHeader));
		SegImage* result = nullptr;
		try
		{
			result = new SegImage(width, height, pitch, data, timestamp, frameNumber);
		}
		catch (std::bad_alloc &/*e*/)
		{
			res = ALLOCATION_FAILURE;
			result = NULL;
		}
		*image = result;
	}
	else
	{
		res = SERVICE_NOT_READY;
	}
    m_server->UnlockBuffer();
    return res;
}
