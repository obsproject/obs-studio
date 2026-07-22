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

#pragma once

#include "win-mf.hpp"

class DeviceEnumerator {

	std::vector<MediaFoundationVideoDevice> _devices = {};

public:
	DeviceEnumerator() {}
	~DeviceEnumerator() {}

	void static __stdcall EnumerateCameraCallback(const wchar_t *Name, const wchar_t *DevId, void *pUserData)
	{
		DeviceEnumerator *pThis = (DeviceEnumerator *)pUserData;

		printf("Name: %ws, DevId: %ws\n", Name, DevId);
		MediaFoundationVideoDevice vd;
		vd.name = Name;
		vd.path = DevId;

		CAPTURE_DEVICE_HANDLE h = MF_Create(DevId);
		if (h) {
			HRESULT hr = MF_EnumerateStreamCapabilities(h, EnumerateStreamCapabilitiesCallback, &vd);
			if (SUCCEEDED(hr)) {
				pThis->_devices.push_back(vd);
			}
			MF_Destroy(h);
		}
	}

	void static __stdcall EnumerateStreamCapabilitiesCallback(UINT32 Width, UINT32 Height, UINT32 FpsN, UINT32 FpsD,
								  void *pUserData)
	{
		MediaFoundationVideoDevice *vd = (MediaFoundationVideoDevice *)pUserData;
		MediaFoundationVideoInfo vInfo;
		vInfo.minCX = Width;
		vInfo.maxCX = Width;
		vInfo.minCY = Height;
		vInfo.maxCY = Height;
		vInfo.granularityCX = 1;
		vInfo.granularityCY = 1;
		vInfo.minInterval = 10000000ll * FpsD / FpsN;
		vInfo.maxInterval = vInfo.minInterval;
		vd->caps.push_back(vInfo);
	}

	HRESULT Enumerate(std::vector<MediaFoundationVideoDevice> &devices)
	{
		devices.clear();
		_devices.clear();
		HRESULT hr = MF_EnumerateCameras(EnumerateCameraCallback, this);
		if (SUCCEEDED(hr)) {
			devices = _devices;
		}
		return hr;
	}
};
