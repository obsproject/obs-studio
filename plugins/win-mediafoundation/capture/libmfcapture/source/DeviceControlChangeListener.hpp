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

#include "framework.hpp"

#include <wil/cppwinrt.h> // must be before the first C++ WinRT header, ref:https://github.com/Microsoft/wil/wiki/Error-handling-helpers
#include <wil/result.h>
#include <wil/com.h>

#include <mfidl.h>
#include <string>

typedef enum _WS_CONTROL_TYPE {
	WS_EFFECT_UNKNOWN = -1,
	WS_BACKGROUND_STANDARD = 0,
	WS_BACKGROUND_PORTRAIT = 1,
	WS_BACKGROUND_MASK = 2,
	WS_BACKGROUND_NONE = 3,
	WS_AUTO_FRAMING = 4,
	WS_EYE_CONTACT = 5,
} WS_CONTROL_TYPE;

typedef enum _WS_CONTROL_VALUE {
	WS_EFFECT_OFF = 0,
	WS_EFFECT_ON = 1,
} WS_CONTROL_VALUE;

class DeviceControlChangeListener : public IMFCameraControlNotify {
	long m_cRef = 1;
	wil::critical_section m_lock;

	std::wstring m_wsDevId;
	wil::com_ptr_nothrow<IMFCameraControlMonitor> m_spMonitor = nullptr;
	wil::com_ptr_nothrow<IKsControl> m_spKsControl = nullptr;
	wil::com_ptr_nothrow<IMFExtendedCameraController> m_spExtCamController = nullptr;
	void *m_cbdata = nullptr;

	bool m_bStarted = false;

private:
	HRESULT HandleControlSet_ExtendedCameraControl(UINT32 id);

public:
	DeviceControlChangeListener();
	~DeviceControlChangeListener();

	// IUnknown methods
	IFACEMETHODIMP_(ULONG) AddRef(void) override;
	IFACEMETHODIMP_(ULONG) Release(void) override;
	IFACEMETHODIMP QueryInterface(_In_ REFIID riid, _Out_ LPVOID *ppvObject) override;

	// IMFCameraControlNotify methods
	void STDMETHODCALLTYPE OnChange(_In_ REFGUID controlSet, _In_ UINT32 id);
	void STDMETHODCALLTYPE OnError(_In_ HRESULT hrStatus);

	HRESULT Start(const wchar_t *devId, IMFExtendedCameraController *pExtCamController);

	HRESULT Stop();
};
