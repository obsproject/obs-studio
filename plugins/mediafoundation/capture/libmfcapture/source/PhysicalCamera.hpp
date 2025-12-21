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
#include "mfcapture.hpp"
#include "DeviceControlChangeListener.hpp"

struct CameraInformation {
	std::wstring FriendlyName;
	std::wstring SymbolicLink;
};

struct StreamInformation {
	UINT32 uiWidth;
	UINT32 uiHeight;
	UINT32 uiFpsN;
	UINT32 uiFpsD;
	GUID guidSubtype;
};

struct MepSetting {
	std::wstring SymbolicLink;
	bool Blur = false;
	bool ShallowFocus = false;
	bool Mask = false;
	bool AutoFraming = false;
	bool EyeGazeCorrection = false;
};

class PhysicalCamera : public IMFSourceReaderCallback {
	static std::vector<MepSetting> s_MepSettings;

private:
	long m_cRef = 1;
	winrt::slim_mutex m_Lock;
	wil::com_ptr_nothrow<IMFMediaSource> m_spDevSource = nullptr;
	wil::com_ptr_nothrow<IMFSourceReader> m_spSourceReader = nullptr;
	wil::com_ptr_nothrow<IMFDXGIDeviceManager> m_spDxgiDevManager = nullptr;
	wil::com_ptr_nothrow<IMFExtendedCameraController> m_spExtController = nullptr;

	MF_VideoDataCallback m_cbVideoData = nullptr;
	void *m_pUserData = nullptr;

	MF_COLOR_FORMAT m_fmt = MF_COLOR_FORMAT_UNKNOWN;
	UINT32 m_uiWidth = 0;
	UINT32 m_uiHeight = 0;
	LONG m_lDefaultStride = 0;

	UINT32 m_uiSegMaskBufSize = 0;
	BYTE *m_pSegMaskBuf = nullptr;

	bool m_transparent = false;

	std::wstring m_wsSymbolicName;

	DeviceControlChangeListener *m_pDevCtrlNotify = nullptr;

private:
	HRESULT CreateSourceReader(IMFMediaSource *pSource, IMFDXGIDeviceManager *pDxgiDevIManager,
				   IMFSourceReader **ppSourceReader);

	HRESULT FindMatchingNativeMediaType(DWORD dwPhyStrmIndex, UINT32 uiWidth, UINT32 uiHeight, UINT32 uiFpsN,
					    UINT32 uiFpsD, IMFMediaType **ppMatchingType);

	HRESULT FindMatchingNativeMediaType(DWORD dwPhyStrmIndex, UINT32 uiWidth, UINT32 uiHeight, LONGLONG llInterval,
					    IMFMediaType **ppMatchingType);

	HRESULT GetMepSetting(MepSetting &setting);
	HRESULT SetMepSetting(MepSetting &setting);

	HRESULT SaveSettingsToDefault();

	HRESULT FillSegMask(IMFSample *pSample);
	HRESULT FillAlphaWithSegMask(DWORD *pData);

public:
	PhysicalCamera() = default;
	~PhysicalCamera();

	// IUnknown methods
	STDMETHODIMP QueryInterface(REFIID iid, void **ppv);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	// IMFSourceReaderCallback methods
	STDMETHODIMP OnReadSample(HRESULT hrStatus, DWORD dwPhyStrmIndex, DWORD dwStreamFlags, LONGLONG llTimestamp,
				  IMFSample *pSample);

	STDMETHODIMP OnEvent(DWORD, IMFMediaEvent *) { return S_OK; }

	STDMETHODIMP OnFlush(DWORD) { return S_OK; }

	HRESULT Initialize(LPCWSTR pwszSymLink);

	HRESULT Uninitialize();

	HRESULT SetD3dManager(IMFDXGIDeviceManager *pD3dManager);

	HRESULT Prepare();

	HRESULT Start(DWORD dwPhyStrmIndex, MF_VideoDataCallback cb, void *pUserData);

	HRESULT Stop();

	HRESULT GetStreamCapabilities(DWORD dwPhyStrmIndex, std::vector<StreamInformation> &strmCaps);

	void SetTransparent(bool flag) { m_transparent = flag; }

	bool GetTransparent() { return m_transparent; }

	// If llInterval is 0, set the camera to use the max FPS.
	// Otherwise, set camera with matching FPS.
	HRESULT SetOutputResolution(DWORD dwPhyStrmIndex, UINT32 uiWidth, UINT32 uiHeight, LONGLONG llInterval,
				    MF_COLOR_FORMAT fmt);

	HRESULT GetCurrentStreamInformation(DWORD dwPhyStrmIndex, StreamInformation &strmInfo);

	HRESULT SetBlur(bool blur, bool shallowFocus, bool mask);
	HRESULT GetBlur(bool &blur, bool &shallowFocus, bool &mask);

	HRESULT SetAutoFraming(bool enable);
	HRESULT GetAutoFraming(bool &enable);

	HRESULT SetEyeGazeCorrection(bool enable);
	HRESULT GetEyeGazeCorrection(bool &enable);

	HRESULT RestoreDefaultSettings();

	static HRESULT CreateInstance(PhysicalCamera **ppPhysicalCamera);

	static HRESULT GetPhysicalCameras(std::vector<CameraInformation> &cameras);
};
