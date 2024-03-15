#include <mfidl.h>

class VideoBufferLock {

public:
	VideoBufferLock(IMFMediaBuffer *pBuffer)
	{
		m_spBuffer = pBuffer;
		m_spBuffer->QueryInterface(IID_PPV_ARGS(&m_sp2DBuffer));
	}

	~VideoBufferLock()
	{
		UnlockBuffer();
		m_spBuffer = nullptr;
		m_sp2DBuffer = nullptr;
	}

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
