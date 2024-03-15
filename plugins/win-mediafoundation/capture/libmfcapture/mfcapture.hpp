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

#include <Windows.h>

#ifdef MFCAPTURE_EXPORTS
#define MFCAPTURE_EXPORTS __declspec(dllexport)
#else
#define MFCAPTURE_EXPORTS
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum MF_COLOR_FORMAT {
	MF_COLOR_FORMAT_UNKNOWN,
	MF_COLOR_FORMAT_NV12,
	MF_COLOR_FORMAT_ARGB,
	MF_COLOR_FORMAT_XRGB,
};

typedef HANDLE CAPTURE_DEVICE_HANDLE;
typedef void(__stdcall *MF_VideoDataCallback)(void *pData, int Size, long long llTimestamp, void *pUserData);
typedef void(__stdcall *MF_EnumerateCameraCallback)(const wchar_t *Name, const wchar_t *DevId, void *pUserData);
typedef void(__stdcall *MF_EnumerateStreamCapabilitiesCallback)(UINT32 Width, UINT32 Height, UINT32 FpsN, UINT32 FpsD,
								void *pUserData);

MFCAPTURE_EXPORTS HRESULT MF_EnumerateCameras(MF_EnumerateCameraCallback cb, void *pUserData);

MFCAPTURE_EXPORTS CAPTURE_DEVICE_HANDLE MF_Create(const wchar_t *DevId);

MFCAPTURE_EXPORTS void MF_Destroy(CAPTURE_DEVICE_HANDLE h);

MFCAPTURE_EXPORTS HRESULT MF_Prepare(CAPTURE_DEVICE_HANDLE h);

MFCAPTURE_EXPORTS HRESULT MF_EnumerateStreamCapabilities(CAPTURE_DEVICE_HANDLE h,
							 MF_EnumerateStreamCapabilitiesCallback cb, void *pUserData);

// If llInterval is 0, set the camera to use the max FPS.
// Otherwise, set camera with matching FPS.
MFCAPTURE_EXPORTS HRESULT MF_SetOutputResolution(CAPTURE_DEVICE_HANDLE h, UINT32 uiWidth, UINT32 uiHeight,
						 LONGLONG llInterval, MF_COLOR_FORMAT fmt);

// LONGLONG llInterval = 10000000ll * uiFpsD / uiFpsN;
MFCAPTURE_EXPORTS HRESULT MF_GetOutputResolution(CAPTURE_DEVICE_HANDLE h, UINT32 *uiWidth, UINT32 *uiHeight,
						 UINT32 *uiFpsN, UINT32 *uiFpsD);

MFCAPTURE_EXPORTS HRESULT MF_Start(CAPTURE_DEVICE_HANDLE h, MF_VideoDataCallback cb, void *pUserData);

MFCAPTURE_EXPORTS HRESULT MF_Stop(CAPTURE_DEVICE_HANDLE h);

MFCAPTURE_EXPORTS HRESULT MF_GetBlur(CAPTURE_DEVICE_HANDLE h, bool &blur, bool &shallowFocus, bool &mask);

MFCAPTURE_EXPORTS HRESULT MF_SetBlur(CAPTURE_DEVICE_HANDLE h, BOOL blur, BOOL shallowFocus, BOOL mask);

MFCAPTURE_EXPORTS HRESULT MF_GetTransparent(CAPTURE_DEVICE_HANDLE h, bool &enable);

MFCAPTURE_EXPORTS HRESULT MF_GetAutoFraming(CAPTURE_DEVICE_HANDLE h, bool &enable);

MFCAPTURE_EXPORTS HRESULT MF_SetAutoFraming(CAPTURE_DEVICE_HANDLE h, BOOL enable);

MFCAPTURE_EXPORTS HRESULT MF_GetEyeGazeCorrection(CAPTURE_DEVICE_HANDLE h, bool &enable);

MFCAPTURE_EXPORTS HRESULT MF_SetEyeGazeCorrection(CAPTURE_DEVICE_HANDLE h, BOOL enable);

MFCAPTURE_EXPORTS HRESULT MF_RestoreDefaultSettings(CAPTURE_DEVICE_HANDLE h);

#ifdef __cplusplus
}
#endif
