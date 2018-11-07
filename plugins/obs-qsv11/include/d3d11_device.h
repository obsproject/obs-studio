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

#include "common_utils.h" // defines MFX_D3D11_SUPPORT

#if MFX_D3D11_SUPPORT
#include "hw_device.h"
#include <windows.h>
#include <d3d11.h>
#include <atlbase.h>

#include <dxgi1_2.h>

class CD3D11Device: public CHWDevice
{
public:
    CD3D11Device();
    virtual ~CD3D11Device();
    virtual mfxStatus Init(
        mfxHDL hWindow,
        mfxU16 nViews,
        mfxU32 nAdapterNum);
    virtual mfxStatus Reset();
    virtual mfxStatus GetHandle(mfxHandleType type, mfxHDL *pHdl);
    virtual mfxStatus SetHandle(mfxHandleType type, mfxHDL hdl);
    virtual mfxStatus RenderFrame(mfxFrameSurface1 * pSurface, mfxFrameAllocator * pmfxAlloc);
    virtual void      UpdateTitle(double /*fps*/) { }
    virtual void      Close();
            void      DefineFormat(bool isA2rgb10) { m_bIsA2rgb10 = (isA2rgb10) ? TRUE : FALSE; }
    virtual void      SetMondelloInput(bool /*isMondelloInputEnabled*/ ) { }
protected:
    virtual mfxStatus FillSCD(mfxHDL hWindow, DXGI_SWAP_CHAIN_DESC& scd);
    virtual mfxStatus FillSCD1(DXGI_SWAP_CHAIN_DESC1& scd);
    mfxStatus CreateVideoProcessor(mfxFrameSurface1 * pSrf);

    CComPtr<ID3D11Device>                   m_pD3D11Device;
    CComPtr<ID3D11DeviceContext>            m_pD3D11Ctx;
    CComQIPtr<ID3D11VideoDevice>            m_pDX11VideoDevice;
    CComQIPtr<ID3D11VideoContext>           m_pVideoContext;
    CComPtr<ID3D11VideoProcessorEnumerator> m_VideoProcessorEnum;

    CComQIPtr<IDXGIDevice1>                 m_pDXGIDev;
    CComQIPtr<IDXGIAdapter>                 m_pAdapter;

    CComPtr<IDXGIFactory2>                  m_pDXGIFactory;

    CComPtr<IDXGISwapChain1>                m_pSwapChain;
    CComPtr<ID3D11VideoProcessor>           m_pVideoProcessor;

private:
    CComPtr<ID3D11VideoProcessorInputView>  m_pInputViewLeft;
    CComPtr<ID3D11VideoProcessorInputView>  m_pInputViewRight;
    CComPtr<ID3D11VideoProcessorOutputView> m_pOutputView;

    CComPtr<ID3D11Texture2D>                m_pDXGIBackBuffer;
    CComPtr<ID3D11Texture2D>                m_pTempTexture;
    CComPtr<IDXGIDisplayControl>            m_pDisplayControl;
    CComPtr<IDXGIOutput>                    m_pDXGIOutput;
    mfxU16                                  m_nViews;
    BOOL                                    m_bDefaultStereoEnabled;
    BOOL                                    m_bIsA2rgb10;
    HWND                                    m_HandleWindow;
};

#endif //#if defined( _WIN32 ) || defined ( _WIN64 )
#endif //#if MFX_D3D11_SUPPORT
