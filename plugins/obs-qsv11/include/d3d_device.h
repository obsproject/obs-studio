/******************************************************************************\
Copyright (c) 2005-2018, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This sample was distributed or derived from the Intel's Media Samples package.
The original version of this sample may be obtained from https://software.intel.com/en-us/intel-media-server-studio
or https://software.intel.com/en-us/media-client-solutions-support.
\**********************************************************************************/

#pragma once

#if defined( _WIN32 ) || defined ( _WIN64 )

#include "hw_device.h"

#pragma warning(disable : 4201)
#include <d3d9.h>
#include <dxva2api.h>
#include <dxva.h>
#include <windows.h>

#define VIDEO_MAIN_FORMAT D3DFMT_YUY2

/** Direct3D 9 device implementation.
@note Device always set D3DPRESENT_PARAMETERS::Windowed to TRUE.
*/
class CD3D9Device : public CHWDevice
{
public:
    CD3D9Device();
    virtual ~CD3D9Device();

    virtual mfxStatus Init(
        mfxHDL hWindow,
        mfxU16 nViews,
        mfxU32 nAdapterNum);
    virtual mfxStatus Reset();
    virtual mfxStatus GetHandle(mfxHandleType type, mfxHDL *pHdl);
    virtual mfxStatus SetHandle(mfxHandleType type, mfxHDL hdl);
    virtual mfxStatus RenderFrame(mfxFrameSurface1 * pSurface, mfxFrameAllocator * pmfxAlloc);
    virtual void      UpdateTitle(double /*fps*/) { }
    virtual void      Close() ;
            void      DefineFormat(bool isA2rgb10) { m_bIsA2rgb10 = (isA2rgb10) ? TRUE : FALSE; }
    virtual void      SetMondelloInput(bool /*isMondelloInputEnabled*/) { }
protected:
    mfxStatus CreateVideoProcessors();
    bool CheckOverlaySupport();
    virtual mfxStatus FillD3DPP(mfxHDL hWindow, mfxU16 nViews, D3DPRESENT_PARAMETERS &D3DPP);
private:
    IDirect3D9Ex*               m_pD3D9;
    IDirect3DDevice9Ex*         m_pD3DD9;
    IDirect3DDeviceManager9*    m_pDeviceManager9;
    D3DPRESENT_PARAMETERS       m_D3DPP;
    UINT                        m_resetToken;

    mfxU16                      m_nViews;

    D3DSURFACE_DESC                 m_backBufferDesc;

    // service required to create video processors
    IDirectXVideoProcessorService*  m_pDXVAVPS;
    //left channel processor
    IDirectXVideoProcessor*         m_pDXVAVP_Left;
    // right channel processor
    IDirectXVideoProcessor*         m_pDXVAVP_Right;

    // target rectangle
    RECT                            m_targetRect;

    // various structures for DXVA2 calls
    DXVA2_VideoDesc                 m_VideoDesc;
    DXVA2_VideoProcessBltParams     m_BltParams;
    DXVA2_VideoSample               m_Sample;

    BOOL                            m_bIsA2rgb10;
};

#endif // #if defined( _WIN32 ) || defined ( _WIN64 )
