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

// SegProc.h : Declaration of the CSegProc

#pragma once
#include "resource.h"       // main symbols
#include "seg_service.h"

#include "pxcsession.h"
#include "pxcsensemanager.h"
#include "pxc3dseg.h"

#include <string>

#include <mutex>
#include <thread>
#include <condition_variable>

#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

using namespace ATL;


/* Number of milliseconds service waits for worker thread to start */
#define START_TIMEOUT 2000
#define USE_DEFAULT_PROPERTY_VALUE -1

// CSegProc

typedef struct _frameHeader
{
    int width;
    int height;
    int pitch;
    long long timestamp;
    int frameNumber;
} FrameHeader;

class ATL_NO_VTABLE CSegProc :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CSegProc, &CLSID_SegProc>,
    public IDispatchImpl<ISegProc, &IID_ISegProc, &LIBID_seg_serviceLib, /*wMajor =*/ 1, /*wMinor =*/ 0>,
    public PXCSenseManager::Handler
{
private:
    enum {
        DEPTH_PROPERTY_NORMAL_MODE = 0x03,
        DEPTH_PROPERTY_HDR_MODE = 0x200
    };
    // Service state section
    HANDLE m_loopThreadHandle;
    bool m_procRun;
    HANDLE m_hStartedEvt;

    // Shared memory section
    std::wstring m_bufferName;
    HANDLE m_sharedBufferHandle;
    LPCTSTR m_sharedBuffer;
    bool m_bufferRealloc;

    // Frames section
    int m_currentFrame;
    int m_frameToRead;
    size_t m_frameSize;
    const static size_t headerSize = sizeof(FrameHeader);
    CRITICAL_SECTION m_frameAccess[2];

    // RSSDK section
    PXCSenseManager* m_senseManager;
    PXC3DSeg* m_segModule;
    int m_fps;
    int m_motionRangeTradeOff;

    bool m_processing;
    bool m_isPause;
    std::condition_variable m_cvPause;
    std::mutex m_pauseMtx;

    pxcStatus PXCAPI OnModuleSetProfile(pxcUID /*mid*/, PXCBase* /*module*/) override
    {
        PXCCaptureManager* captureMgr = m_senseManager->QueryCaptureManager();
        if (!captureMgr || m_fps != 60)
            return PXC_STATUS_NO_ERROR;
        PXCCapture::Device* device = captureMgr->QueryDevice();
        PXCCapture::Device::PropertyInfo propInfo = device->QueryIVCAMMotionRangeTradeOffInfo();

        int value = m_motionRangeTradeOff;
        if (m_motionRangeTradeOff == USE_DEFAULT_PROPERTY_VALUE)
            value = (int)propInfo.defaultValue;

        device->SetIVCAMMotionRangeTradeOff(value);
        return PXC_STATUS_NO_ERROR;
    }

    pxcStatus senseMgrInit()
    {
        pxcStatus status = PXC_STATUS_NO_ERROR;
        status = m_senseManager->Enable3DSeg(nullptr);
        if (status != PXC_STATUS_NO_ERROR)
            return status;
        m_segModule = m_senseManager->Query3DSeg();
        if (!m_segModule)
            return PXC_STATUS_DATA_UNAVAILABLE;

        
        
        for (int i = 0; ; i++) {
            pxcStatus status = PXC_STATUS_NO_ERROR;
            PXCVideoModule::DataDesc currentProfile = {};
            status = m_segModule->QueryInstance<PXCVideoModule>()->QueryCaptureProfile(i, &currentProfile);
            if (status != PXC_STATUS_NO_ERROR)
                return status;
            if ((currentProfile.streams.depth.propertySet != DEPTH_PROPERTY_NORMAL_MODE)
                || (currentProfile.streams.depth.options & PXCCapture::Device::STREAM_OPTION_DEPTH_CONFIDENCE)) {
                continue;
            }

            m_senseManager->QueryCaptureManager()->FilterByStreamProfiles(nullptr);
            m_senseManager->QueryCaptureManager()->FilterByStreamProfiles(nullptr);

            m_senseManager->QueryCaptureManager()->FilterByStreamProfiles(PXCCapture::StreamType::STREAM_TYPE_COLOR, 0, 0, (pxcF32)m_fps);
            m_senseManager->QueryCaptureManager()->FilterByStreamProfiles(PXCCapture::StreamType::STREAM_TYPE_DEPTH, 0, 0, (pxcF32)m_fps);
            
            status = m_senseManager->EnableStreams(&currentProfile);
            if (status != PXC_STATUS_NO_ERROR)
                return status;
            status = m_senseManager->Init(this);
            if (status == PXC_STATUS_NO_ERROR) {
                m_isPause = false;
                break;
            }
            else {
                continue;
            }
        }

        return status;
    }

    HRESULT reinit()
    {
        m_isPause = true;
        //wait_for_end_processing
        while (m_processing)
            std::this_thread::yield();

        std::unique_lock<std::mutex> lck(m_pauseMtx);
        m_senseManager->Close();

        senseMgrInit();
        m_isPause = false;
        m_cvPause.notify_one();
        return S_OK;
    }

    /* -----------------------------------------------------------------
     * Modification by Jim
     *
     * To avoid linker issues with the RSSDK .lib files, load via
     * LoadLibrary
     * ----------------------------------------------------------------- */
    typedef int (WINAPI *PXCSessionCreateProc)(PXCSession **output);

    static HMODULE GetLib()
    {
        HMODULE lib = nullptr;
        HKEY key = nullptr;
        wchar_t path[1024];

        LONG res = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Intel\\RSSDK\\Dispatch", 0, KEY_QUERY_VALUE, &key);
        if (res != ERROR_SUCCESS) {
            res = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Intel\\RSSDK\\v10\\Dispatch", 0, KEY_QUERY_VALUE, &key);
        }

        if (res != ERROR_SUCCESS)
            return nullptr;

        DWORD size = 1024;
        res = RegQueryValueExW(key, L"Core", nullptr, nullptr, (LPBYTE)path, &size);
        if (res == ERROR_SUCCESS) {
            lib = LoadLibrary(path);
        }

        RegCloseKey(key);
        return lib;
    }

    static PXCSenseManager* CreateSessionInstance()
    {
        static bool initialized = false;
        static HMODULE lib = nullptr;
        static PXCSessionCreateProc create = nullptr;

        if (!initialized) {
            lib = GetLib();
            create = (PXCSessionCreateProc)GetProcAddress(lib, "PXCSession_Create");
            initialized = true;
        }

        if (!lib || !create)
            return nullptr;

        PXCSession *session = nullptr;
	int test = create(&session);
        if (test != 0 || !session)
            return nullptr;

        PXCSenseManager *sm = session->CreateSenseManager();
        session->Release();
        return sm;
    }

    /* -----------------------------------------------------------------
     * End Modification
     * ----------------------------------------------------------------- */

public:
    CSegProc()
        : m_isPause(true)
        , m_fps(0)
        , m_processing(false)
        , m_motionRangeTradeOff(USE_DEFAULT_PROPERTY_VALUE)
    {
    }

    DECLARE_REGISTRY_RESOURCEID(IDR_SEGPROC)

    DECLARE_NOT_AGGREGATABLE(CSegProc)

    BEGIN_COM_MAP(CSegProc)
        COM_INTERFACE_ENTRY(ISegProc)
        COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    HRESULT FinalConstruct()
    {
        return S_OK;
    }

    void FinalRelease()
    {
    }

    HRESULT STDMETHODCALLTYPE Init(LPCWSTR bufferName)
    {
        m_frameSize = 16;
        m_frameToRead = -1;

        m_bufferName = bufferName;

        m_sharedBufferHandle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 2 * m_frameSize, m_bufferName.c_str());
        if (m_sharedBufferHandle == NULL)
            return E_FAIL;
        m_sharedBuffer = (LPTSTR)MapViewOfFile(m_sharedBufferHandle, FILE_MAP_ALL_ACCESS, 0, 0, 32);
        if (m_sharedBuffer == NULL)
        {
            CloseHandle(m_sharedBufferHandle);
            return E_FAIL;
        }
        m_procRun = false;

        /* -----------------------------------------------------------------
         * Modification by Jim
         *
         * To avoid linker issues with the RSSDK .lib files, load via
         * LoadLibrary.
         * ----------------------------------------------------------------- */
        m_senseManager = CreateSessionInstance();
        /* -----------------------------------------------------------------
         * End Modification
         * ----------------------------------------------------------------- */
        if (!m_senseManager)
            return E_FAIL;

        pxcStatus status = senseMgrInit();
        if (status < PXC_STATUS_NO_ERROR)
            return E_FAIL;

        m_hStartedEvt = CreateEvent(NULL, FALSE, FALSE, TEXT("StartEvent"));
        if (m_hStartedEvt == NULL)
        {
            return E_FAIL;
        }

        if (m_procRun)
            return E_FAIL;
        m_loopThreadHandle = CreateThread(NULL, 0, &CSegProc::LoopStub, this, 0, NULL);
        if (m_loopThreadHandle == NULL)
        {
            return E_OUTOFMEMORY;
        }
        /* Waiting thread for start */
        DWORD dwWaitResult = WaitForSingleObject(m_hStartedEvt, INFINITE);
        switch (dwWaitResult)
        {
        case WAIT_OBJECT_0:
            return S_OK;
        }
        return E_FAIL;
    }

    static DWORD WINAPI LoopStub(LPVOID lpParam)
    {
        if (!lpParam) return (DWORD)-1;
        return ((CSegProc*)lpParam)->Loop(NULL);
    }

    DWORD WINAPI Loop(LPVOID /*lpParam*/)
    {
        static const int headerSize = sizeof(FrameHeader);
        InitializeCriticalSection(&m_frameAccess[0]);
        InitializeCriticalSection(&m_frameAccess[1]);
        // Loop through frames
        m_procRun = true;
        SetEvent(m_hStartedEvt);
        m_currentFrame = 0;
        int frameCounter = 0;
        while (m_procRun)
        {
            m_processing = false;

            if (m_isPause)
            {
                std::unique_lock<std::mutex> lck(m_pauseMtx);
                while (m_isPause)
                    m_cvPause.wait(lck);
            }

            m_processing = true;
            if (m_senseManager->AcquireFrame(true) != PXC_STATUS_NO_ERROR)
            {
                continue;
            }

            EnterCriticalSection(&m_frameAccess[m_currentFrame]);
            {
                PXCImage* segImage = m_segModule->AcquireSegmentedImage();
                if (segImage)
                {
                    PXCImage::ImageData segData;
                    ZeroMemory(&segData, sizeof(segData));
                    pxcStatus sts = segImage->AcquireAccess(PXCImage::ACCESS_READ, PXCImage::PixelFormat::PIXEL_FORMAT_RGB32, &segData);
                    if (sts >= PXC_STATUS_NO_ERROR)
                    {
                        int newFrameSize = segData.pitches[0] * segImage->QueryInfo().height + headerSize;
                        if (newFrameSize != (int)m_frameSize)
                        {
                            EnterCriticalSection(&m_frameAccess[1 - m_currentFrame]);
                            ResizeBuffer(newFrameSize * 2);
                            m_frameSize = newFrameSize;
                            m_bufferRealloc = true;
                            LeaveCriticalSection(&m_frameAccess[1 - m_currentFrame]);
                        }
                        int offset = m_frameSize*m_currentFrame;
                        //((int*)m_sharedBuffer)[offset+0] = segImage->QueryInfo().width;
                        //((int*)m_sharedBuffer)[offset+1] = segImage->QueryInfo().height;
                        //((int*)m_sharedBuffer)[offset+2] = segData.pitches[0];
                        char *ptr = ((char*)m_sharedBuffer) + offset;
                        PXCImage::ImageInfo info = segImage->QueryInfo();
                        FrameHeader *fhPtr = (FrameHeader*)ptr;
                        fhPtr->width = info.width;
                        fhPtr->height = info.height;
                        fhPtr->pitch = segData.pitches[0];
                        fhPtr->timestamp = segImage->QueryTimeStamp();
                        fhPtr->frameNumber = frameCounter;
                        memcpy_s((void*)((char*)m_sharedBuffer + offset + headerSize), m_frameSize - headerSize, segData.planes[0], m_frameSize - headerSize);
                        segImage->ReleaseAccess(&segData);
                    }
                    segImage->Release();
                }
            }
            m_currentFrame = 1 - m_currentFrame;
            LeaveCriticalSection(&m_frameAccess[m_currentFrame]);

            m_senseManager->ReleaseFrame();
            frameCounter++;
        }

        DeleteCriticalSection(&m_frameAccess[0]);
        DeleteCriticalSection(&m_frameAccess[1]);

        return 0;
    }

    // Bad function for the reason of outside syncronization
    HRESULT ResizeBuffer(size_t newSize)
    {
        UnmapViewOfFile(m_sharedBuffer);
        CloseHandle(m_sharedBufferHandle);
        m_sharedBufferHandle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, newSize, m_bufferName.c_str());

        if (!m_sharedBufferHandle)
            return E_ACCESSDENIED;
        m_sharedBuffer = (LPTSTR)MapViewOfFile(m_sharedBufferHandle, FILE_MAP_ALL_ACCESS, 0, 0, newSize);

        if (!m_sharedBuffer)
        {
            CloseHandle(m_sharedBufferHandle);
            return E_OUTOFMEMORY;
        }
        ZeroMemory((void*)m_sharedBuffer, newSize);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE LockBuffer(int* frameId, int* frameSize, int* bufferRealloc)
    {
        if (!m_procRun || m_frameToRead != -1)
            return E_FAIL;
        m_frameToRead = 1 - m_currentFrame;
        EnterCriticalSection(&m_frameAccess[m_frameToRead]);
        *frameId = m_frameToRead;
        if (m_bufferRealloc)
        {
            *bufferRealloc = (int)m_bufferRealloc;
            m_bufferRealloc = false;
        }
        else
            *bufferRealloc = false;
        *frameSize = m_frameSize;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE UnlockBuffer()
    {
        if (!m_procRun || m_frameToRead == -1)
        {
            return E_FAIL;
        }
        LeaveCriticalSection(&m_frameAccess[m_frameToRead]);
        m_frameToRead = -1;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Stop()
    {
        if (!m_procRun)
        {
            return E_FAIL;
        }
        m_procRun = false;
        WaitForSingleObject(m_loopThreadHandle, INFINITE);
        m_senseManager->Close();
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE SetFps(int fps)
    {
        if (m_fps == fps)
            return S_OK;
        m_fps = fps;
        if (m_procRun)
            return reinit();
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetFps(int* fps)
    {
        *fps = m_fps;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE SetIVCAMMotionRangeTradeOff(int value)
    {
        if (m_motionRangeTradeOff == value)
            return S_OK;
        m_motionRangeTradeOff = value;
        if (m_procRun)
            return reinit();
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetIVCAMMotionRangeTradeOff(int* value)
    {
        *value = m_motionRangeTradeOff;
        return S_OK;
    }
};

OBJECT_ENTRY_AUTO(__uuidof(SegProc), CSegProc)
