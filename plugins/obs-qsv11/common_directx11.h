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

#include "common_utils.h"

#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <atlbase.h>

#define DEVICE_MGR_TYPE MFX_HANDLE_D3D11_DEVICE

// =================================================================
// DirectX functionality required to manage D3D surfaces
//

// Create DirectX 11 device context
// - Required when using D3D surfaces.
// - D3D Device created and handed to Intel Media SDK
// - Intel graphics device adapter will be determined automatically (does not have to be primary),
//   but with the following caveats:
//     - Device must be active (but monitor does NOT have to be attached)
//     - Device must be enabled in BIOS. Required for the case when used together with a discrete graphics card
//     - For switchable graphics solutions (mobile) make sure that Intel device is the active device
mfxStatus CreateHWDevice(mfxSession session, mfxHDL* deviceHandle, HWND hWnd, bool bCreateSharedHandles);
void CleanupHWDevice();
void SetHWDeviceContext(CComPtr<ID3D11DeviceContext> devCtx);
CComPtr<ID3D11DeviceContext> GetHWDeviceContext();
void ClearYUVSurfaceD3D(mfxMemId memId);
void ClearRGBSurfaceD3D(mfxMemId memId);
