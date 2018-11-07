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

#include "d3d_device.h"
#include "d3d_allocator.h"

#include "d3d11_device.h"
#include "d3d11_allocator.h"


const struct {
		mfxIMPL impl;       // actual implementation
		mfxU32  adapterID;  // device adapter number
} implTypes[] = {
		{ MFX_IMPL_HARDWARE, 0 },
		{ MFX_IMPL_HARDWARE2, 1 },
		{ MFX_IMPL_HARDWARE3, 2 },
		{ MFX_IMPL_HARDWARE4, 3 }
};

CComPtr<IDirect3DDeviceManager9> m_manager;
HANDLE m_hDecoder;
HANDLE m_hProcessor;
DWORD m_surfaceUsage;


enum MemType {
		SYSTEM_MEMORY = 0x00,
		D3D9_MEMORY = 0x01,
		D3D11_MEMORY = 0x02,
};
/* =======================================================
 * Windows implementation of OS-specific utility functions
 */

CHWDevice *m_hwdev = NULL;

mfxStatus Initialize(mfxIMPL impl, mfxVersion ver, MFXVideoSession* pSession, GeneralAllocator* pmfxAllocator, mfxHDL deviceHandle, bool bCreateSharedHandles, bool dx9hack)
{
	mfxStatus sts = MFX_ERR_NONE;

	// If mfxFrameAllocator is provided it means we need to setup DirectX device and memory allocator
	if (pmfxAllocator && !dx9hack) {

		// Initialize Intel Media SDK Session
		sts = pSession->Init(impl, &ver);
		MSDK_CHECK_STATUS(sts, "failed to initialize session");

		// Create DirectX device context
		if (m_hwdev == NULL)
			m_hwdev = new CD3D11Device();

		mfxU32  adapterNum = 0;
		mfxIMPL impl;

		MFXQueryIMPL(*pSession, &impl);

		mfxIMPL baseImpl = MFX_IMPL_BASETYPE(impl); // Extract Media SDK base implementation type

		for (mfxU8 i = 0; i < sizeof(implTypes) / sizeof(implTypes[0]); i++) {
			if (implTypes[i].impl == baseImpl) {
				adapterNum = implTypes[i].adapterID; // get corresponding adapter number
				break;
			}
		}

		POINT point = { 0, 0 };
		HWND window = WindowFromPoint(point);

		if (deviceHandle == NULL)
		{
			sts = m_hwdev->Init(window, 0, adapterNum);
			MSDK_CHECK_STATUS(sts, "failed to initialize hw device");

			sts = m_hwdev->GetHandle(MFX_HANDLE_D3D11_DEVICE, &deviceHandle);
			MSDK_CHECK_STATUS(sts, "failed to create directx device session");
		}

		if (deviceHandle == NULL)
			return MFX_ERR_DEVICE_FAILED;

		// Provide device manager to Media SDK
		sts = pSession->SetHandle(MFX_HANDLE_D3D11_DEVICE, deviceHandle);
		MSDK_CHECK_STATUS(sts, "failed to set session handle")

		sts = pSession->SetFrameAllocator(pmfxAllocator);
		MSDK_CHECK_STATUS(sts, "pSession->SetFrameAllocator failed");

		D3D11AllocatorParams *pd3dAllocParams = new D3D11AllocatorParams;
		MSDK_CHECK_POINTER(pd3dAllocParams, MFX_ERR_MEMORY_ALLOC);
		pd3dAllocParams->pDevice = reinterpret_cast<ID3D11Device *>(deviceHandle);

		mfxAllocatorParams* m_pmfxAllocatorParams = pd3dAllocParams;

		sts = pmfxAllocator->Init(m_pmfxAllocatorParams);
		MSDK_CHECK_STATUS(sts, "GeneralAllocator->Init failed");

	}
	else if (pmfxAllocator && dx9hack) {

		// Initialize Intel Media SDK Session
		sts = pSession->Init(impl, &ver);
		MSDK_CHECK_STATUS(sts, "failed to initialize session");

		m_hwdev = new CD3D9Device();

		if (NULL == m_hwdev)
			return MFX_ERR_MEMORY_ALLOC;

		mfxU32  adapterNum = 0;
		mfxIMPL impl;

		MFXQueryIMPL(*pSession, &impl);

		mfxIMPL baseImpl = MFX_IMPL_BASETYPE(impl); // Extract Media SDK base implementation type

		for (mfxU8 i = 0; i < sizeof(implTypes) / sizeof(implTypes[0]); i++) {
			if (implTypes[i].impl == baseImpl) {
				adapterNum = implTypes[i].adapterID; // get corresponding adapter number
				break;
			}
		}

		POINT point = { 0, 0 };
		HWND window = WindowFromPoint(point);

		if (deviceHandle == NULL)
		{
			sts = m_hwdev->Init(window, 0, adapterNum);
			MSDK_CHECK_STATUS(sts, "failed to initialize hw device");

			m_hwdev->GetHandle(MFX_HANDLE_D3D9_DEVICE_MANAGER, &deviceHandle);
			MSDK_CHECK_STATUS(sts, "failed to create directx device session");
		}

		if (deviceHandle == NULL)
			return MFX_ERR_DEVICE_FAILED;


		// Provide device manager to Media SDK
		sts = pSession->SetHandle(MFX_HANDLE_D3D9_DEVICE_MANAGER, deviceHandle);
		MSDK_CHECK_STATUS(sts, "failed to set session handle")

		sts = pSession->SetFrameAllocator(pmfxAllocator);
		MSDK_CHECK_STATUS(sts, "xSession.SetFrameAllocator failed");

		D3DAllocatorParams *pd3dAllocParams = new D3DAllocatorParams;
		MSDK_CHECK_POINTER(pd3dAllocParams, MFX_ERR_MEMORY_ALLOC);
		pd3dAllocParams->pManager = reinterpret_cast<IDirect3DDeviceManager9 *>(deviceHandle);

		mfxAllocatorParams* m_pmfxAllocatorParams = pd3dAllocParams;

		sts = pmfxAllocator->Init(m_pmfxAllocatorParams);
		MSDK_CHECK_STATUS(sts, "GeneralAllocator Init failed");
	}
	else {
		// Initialize Intel Media SDK Session
		sts = pSession->Init(impl, &ver);
		MSDK_CHECK_STATUS(sts, "failed to set session handle")

	}
	return sts;
}

void Release()
{
#if defined(DX9_D3D) || defined(DX11_D3D)
	if (m_hwdev)
	{
		m_hwdev->Close();
		m_hwdev = NULL;
	}
#endif
}



/* (Hugh) Functions currently unused */
#if 0
void ClearYUVSurfaceVMem(mfxMemId memId)
{
#if defined(DX9_D3D) || defined(DX11_D3D)
    ClearYUVSurfaceD3D(memId);
#endif
}

void ClearRGBSurfaceVMem(mfxMemId memId)
{
#if defined(DX9_D3D) || defined(DX11_D3D)
    ClearRGBSurfaceD3D(memId);
#endif
}
#endif
