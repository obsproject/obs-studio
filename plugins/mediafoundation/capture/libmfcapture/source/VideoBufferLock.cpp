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

#include <mfidl.h>

class VideoBufferLock {

public:
	VideoBufferLock(IMFMediaBuffer *pBuffer)
	{
		m_spBuffer = pBuffer;
		m_spBuffer->QueryInterface(IID_PPV_ARGS(&m_sp2DBuffer));
	}

	~VideoBufferLock() { UnlockBuffer(); }

	HRESULT LockBuffer(LONG lDefaultStride, DWORD dwHeightInPixels, BYTE **ppbScanLine0, LONG *plStride)
	{
		HRESULT hr = S_OK;
		if (m_sp2DBuffer) {
			hr = m_sp2DBuffer->Lock2D(ppbScanLine0, plStride);
		} else {
			BYTE *pData = NULL;
			hr = m_spBuffer->Lock(&pData, NULL, NULL);
			if (SUCCEEDED(hr)) {
				*plStride = lDefaultStride;
				if (lDefaultStride < 0) {
					*ppbScanLine0 = pData + abs(lDefaultStride) * (dwHeightInPixels - 1);
				} else {
					*ppbScanLine0 = pData;
				}
			}
		}

		m_bLocked = (SUCCEEDED(hr));
		return hr;
	}

	void UnlockBuffer()
	{
		if (m_bLocked) {
			if (m_sp2DBuffer) {
				m_sp2DBuffer->Unlock2D();
			} else {
				m_spBuffer->Unlock();
			}
			m_bLocked = FALSE;
		}
	}

private:
	wil::com_ptr_nothrow<IMFMediaBuffer> m_spBuffer;
	wil::com_ptr_nothrow<IMF2DBuffer> m_sp2DBuffer;

	BOOL m_bLocked = FALSE;
};
