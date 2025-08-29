/*

This is provided under a dual MIT/GPLv2 license.  When using or
redistributing this, you may do so under either license.

GPL LICENSE SUMMARY

Copyright(c) 2025 Intel Corporation.

This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

Contact Information:

Thy-Lan Gale, thy-lan.gale@intel.com
5000 W Chandler Blvd, Chandler, AZ 85226

MIT License

Copyright (c) 2025 Microsoft Corporation.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE
*/

#include "mfcapture.hpp"
#include "DeviceControlChangeListener.hpp"
#include "PhysicalCamera.hpp"

MFCAPTURE_EXPORTS HRESULT MF_EnumerateCameras(MF_EnumerateCameraCallback cb, void *pUserData)
{
	std::vector<CameraInformation> camlist;
	RETURN_IF_FAILED(PhysicalCamera::GetPhysicalCameras(camlist));

	for (auto &cam : camlist) {
		if (cb) {
			cb(cam.FriendlyName.c_str(), cam.SymbolicLink.c_str(), pUserData);
		}
	}
	return S_OK;
}

MFCAPTURE_EXPORTS CAPTURE_DEVICE_HANDLE MF_Create(const wchar_t *DevId)
{
	wil::com_ptr_nothrow<PhysicalCamera> spCapture = nullptr;
	HRESULT hr = PhysicalCamera::CreateInstance(&spCapture);
	if (FAILED(hr)) {
		return NULL;
	}

	hr = spCapture->Initialize(DevId);
	if (FAILED(hr)) {
		return NULL;
	}

	return spCapture.detach();
}

MFCAPTURE_EXPORTS void MF_Destroy(CAPTURE_DEVICE_HANDLE h)
{
	PhysicalCamera *p = (PhysicalCamera *)h;
	if (p) {
		p->Release();
	}
}

MFCAPTURE_EXPORTS HRESULT MF_Prepare(CAPTURE_DEVICE_HANDLE h)
{
	PhysicalCamera *p = (PhysicalCamera *)h;
	if (!p) {
		return E_INVALIDARG;
	}

	return p->Prepare();
}

MFCAPTURE_EXPORTS HRESULT MF_EnumerateStreamCapabilities(CAPTURE_DEVICE_HANDLE h,
							 MF_EnumerateStreamCapabilitiesCallback cb, void *pUserData)
{
	PhysicalCamera *p = (PhysicalCamera *)h;
	if (!p) {
		return E_INVALIDARG;
	}

	std::vector<StreamInformation> caplist;
	HRESULT hr = p->GetStreamCapabilities(0, caplist);
	if (FAILED(hr)) {
		return hr;
	}

	for (auto &cap : caplist) {
		if (cb) {
			cb(cap.uiWidth, cap.uiHeight, cap.uiFpsN, cap.uiFpsD, pUserData);
		}
	}

	return S_OK;
}

MFCAPTURE_EXPORTS HRESULT MF_SetOutputResolution(CAPTURE_DEVICE_HANDLE h, UINT32 uiWidth, UINT32 uiHeight,
						 LONGLONG llInterval, MF_COLOR_FORMAT fmt)
{
	PhysicalCamera *p = (PhysicalCamera *)h;
	if (!p) {
		return E_INVALIDARG;
	}

	return p->SetOutputResolution(0, uiWidth, uiHeight, llInterval, fmt);
}

MFCAPTURE_EXPORTS HRESULT MF_GetOutputResolution(CAPTURE_DEVICE_HANDLE h, UINT32 *uiWidth, UINT32 *uiHeight,
						 UINT32 *uiFpsN, UINT32 *uiFpsD)
{
	PhysicalCamera *p = (PhysicalCamera *)h;
	if (!p) {
		return E_INVALIDARG;
	}

	StreamInformation info;
	RETURN_IF_FAILED(p->GetCurrentStreamInformation(0, info));

	if (uiWidth && uiHeight && uiFpsN && uiFpsD) {
		*uiWidth = info.uiWidth;
		*uiHeight = info.uiHeight;
		*uiFpsN = info.uiFpsN;
		*uiFpsD = info.uiFpsD;
	}

	return S_OK;
}

MFCAPTURE_EXPORTS HRESULT MF_Start(CAPTURE_DEVICE_HANDLE h, MF_VideoDataCallback cb, void *pUserData)
{
	PhysicalCamera *p = (PhysicalCamera *)h;
	if (!p) {
		return E_INVALIDARG;
	}
	return p->Start(0, cb, pUserData);
}

MFCAPTURE_EXPORTS HRESULT MF_Stop(CAPTURE_DEVICE_HANDLE h)
{
	PhysicalCamera *p = (PhysicalCamera *)h;
	if (!p) {
		return E_INVALIDARG;
	}
	return p->Stop();
}

MFCAPTURE_EXPORTS HRESULT MF_GetBlur(CAPTURE_DEVICE_HANDLE h, bool &blur, bool &shallowFocus, bool &mask)
{
	PhysicalCamera *p = (PhysicalCamera *)h;
	if (!p) {
		return E_INVALIDARG;
	}

	return p->GetBlur(blur, shallowFocus, mask);
}

MFCAPTURE_EXPORTS HRESULT MF_SetBlur(CAPTURE_DEVICE_HANDLE h, BOOL blur, BOOL shallowFocus, BOOL mask)
{
	PhysicalCamera *p = (PhysicalCamera *)h;
	if (!p) {
		return E_INVALIDARG;
	}

	p->SetTransparent(mask);

	return p->SetBlur(blur, shallowFocus, mask);
}

MFCAPTURE_EXPORTS HRESULT MF_GetTransparent(CAPTURE_DEVICE_HANDLE h, bool &enable)
{
	PhysicalCamera *p = (PhysicalCamera *)h;
	if (!p) {
		return E_INVALIDARG;
	}

	enable = p->GetTransparent();

	return S_OK;
}

MFCAPTURE_EXPORTS HRESULT MF_GetAutoFraming(CAPTURE_DEVICE_HANDLE h, bool &enable)
{
	PhysicalCamera *p = (PhysicalCamera *)h;
	if (!p) {
		return E_INVALIDARG;
	}

	return p->GetAutoFraming(enable);
}

MFCAPTURE_EXPORTS HRESULT MF_SetAutoFraming(CAPTURE_DEVICE_HANDLE h, BOOL enable)
{
	PhysicalCamera *p = (PhysicalCamera *)h;
	if (!p) {
		return E_INVALIDARG;
	}

	return p->SetAutoFraming(enable);
}

MFCAPTURE_EXPORTS HRESULT MF_GetEyeGazeCorrection(CAPTURE_DEVICE_HANDLE h, bool &enable)
{
	PhysicalCamera *p = (PhysicalCamera *)h;
	if (!p) {
		return E_INVALIDARG;
	}

	return p->GetEyeGazeCorrection(enable);
}

MFCAPTURE_EXPORTS HRESULT MF_SetEyeGazeCorrection(CAPTURE_DEVICE_HANDLE h, BOOL enable)
{
	PhysicalCamera *p = (PhysicalCamera *)h;
	if (!p) {
		return E_INVALIDARG;
	}

	return p->SetEyeGazeCorrection(enable);
}

MFCAPTURE_EXPORTS HRESULT MF_RestoreDefaultSettings(CAPTURE_DEVICE_HANDLE h)
{
	PhysicalCamera *p = (PhysicalCamera *)h;
	if (!p) {
		return E_INVALIDARG;
	}

	return p->RestoreDefaultSettings();
}
