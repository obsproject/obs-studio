#include "mfcapture.hpp"
#include "DeviceControlChangeListener.hpp"
#include "PhysicalCamera.hpp"
#include <d3d11.h>
#include <mfidl.h>
#include <mfvirtualcamera.h>

#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "mfsensorgroup.lib")

extern void ControlNotification(WS_CONTROL_TYPE type, WS_CONTROL_VALUE value,
				void *pUserData);

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

	HRESULT LockBuffer(LONG lDefaultStride, DWORD dwHeightInPixels,
			   BYTE **ppbScanLine0, LONG *plStride)
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
					*ppbScanLine0 =
						pData +
						abs(lDefaultStride) *
							(dwHeightInPixels - 1);
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

PhysicalCamera::PhysicalCamera() {}

PhysicalCamera::~PhysicalCamera()
{
	printf("%s, Begin\n", __FUNCSIG__);
	Uninitialize();
	printf("%s, End\n", __FUNCSIG__);
}

HRESULT __stdcall PhysicalCamera::QueryInterface(REFIID iid, void **ppv)
{

	if (iid == IID_IUnknown) {
		*ppv = static_cast<IUnknown *>(this);
	} else if (iid == IID_IMFSourceReaderCallback) {
		*ppv = static_cast<IMFSourceReaderCallback *>(this);
	} else {
		*ppv = NULL;
		return E_NOINTERFACE;
	}
	reinterpret_cast<IUnknown *>(*ppv)->AddRef();
	return S_OK;
}

ULONG __stdcall PhysicalCamera::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}

ULONG __stdcall PhysicalCamera::Release()
{

	if (InterlockedDecrement(&m_cRef) == 0) {
		delete this;
		return 0;
	}
	return m_cRef;
}

/*
*  The function creates source reader and enables HW decode
*/
HRESULT
PhysicalCamera::CreateSourceReader(IMFMediaSource *pSource,
				   IMFDXGIDeviceManager *pDxgiDevIManager,
				   IMFSourceReader **ppSourceReader)
{
	printf("%s, Begin\n", __FUNCSIG__);
	HRESULT hr = S_OK;

	if (!pSource) {
		return E_INVALIDARG;
	}

	// Create attributes for source reader creation
	UINT32 cInitialSize = 5;

	wil::com_ptr_nothrow<IMFAttributes> spAttributes;
	RETURN_IF_FAILED(MFCreateAttributes(&spAttributes, cInitialSize));

	if (pDxgiDevIManager) {
		RETURN_IF_FAILED(spAttributes->SetUnknown(
			MF_SOURCE_READER_D3D_MANAGER, pDxgiDevIManager));
		RETURN_IF_FAILED(spAttributes->SetUINT32(
			MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE));
	}

	RETURN_IF_FAILED(spAttributes->SetUINT32(
		MF_READWRITE_DISABLE_CONVERTERS, FALSE));
	RETURN_IF_FAILED(spAttributes->SetUINT32(
		MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, TRUE));
	RETURN_IF_FAILED(spAttributes->SetUnknown(
		MF_SOURCE_READER_ASYNC_CALLBACK, this));

	// Create source reader
	RETURN_IF_FAILED(MFCreateSourceReaderFromMediaSource(
		pSource, spAttributes.get(), ppSourceReader));

	printf("%s, End\n", __FUNCSIG__);
	return hr;
}

HRESULT PhysicalCamera::Initialize(LPCWSTR pwszSymLink)
{
	printf("%s, Begin\n", __FUNCSIG__);
	printf("%s, %ws\n", __FUNCSIG__, pwszSymLink);

	HRESULT hr = S_OK;
	winrt::slim_lock_guard lock(m_Lock);

	// Create physical camera source
	wil::com_ptr_nothrow<IMFAttributes> spDeviceAttributes;
	RETURN_IF_FAILED(MFCreateAttributes(&spDeviceAttributes, 3));
	RETURN_IF_FAILED(spDeviceAttributes->SetGUID(
		MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
		MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID));
	RETURN_IF_FAILED(spDeviceAttributes->SetString(
		MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,
		pwszSymLink));

	RETURN_IF_FAILED(
		MFCreateDeviceSource(spDeviceAttributes.get(), &m_spDevSource));

	wil::com_ptr_nothrow<IMFGetService> spGetService = nullptr;
	RETURN_IF_FAILED(m_spDevSource->QueryInterface(&spGetService));
	RETURN_IF_FAILED(spGetService->GetService(
		GUID_NULL, IID_PPV_ARGS(&m_spExtController)));

	m_pDevCtrlNotify = new DeviceControlChangeListener();
	if (!m_pDevCtrlNotify) {
		return E_OUTOFMEMORY;
	}

	hr = m_pDevCtrlNotify->Start(pwszSymLink, m_spExtController.get());
	if (FAILED(hr)) {
		delete m_pDevCtrlNotify;
		return hr;
	}

	m_wsSymbolicName = pwszSymLink;

	printf("%s, End\n", __FUNCSIG__);

	return hr;
}

HRESULT PhysicalCamera::Uninitialize()
{

	printf("%s, Begin\n", __FUNCSIG__);

	Stop();
	{
		winrt::slim_lock_guard lock(m_Lock);

		if (m_pDevCtrlNotify) {
			m_pDevCtrlNotify->Stop();
			m_pDevCtrlNotify->Release();
			m_pDevCtrlNotify = nullptr;
		}
		m_spSourceReader = nullptr;
		if (m_spDevSource.get() != nullptr) {
			m_spDevSource->Shutdown();
		}
		m_spDevSource = nullptr;
		m_spDxgiDevManager = nullptr;
		m_spExtController = nullptr;
	}

	printf("%s, End\n", __FUNCSIG__);
	return S_OK;
}

HRESULT PhysicalCamera::SetD3dManager(IMFDXGIDeviceManager *pD3dManager)
{
	if (!pD3dManager) {
		return E_INVALIDARG;
	}

	HRESULT hr = S_OK;
	winrt::slim_lock_guard lock(m_Lock);

	m_spDxgiDevManager = nullptr;
	m_spDxgiDevManager = pD3dManager;

	return hr;
}

HRESULT PhysicalCamera::Prepare()
{
	printf("%s, Begin\n", __FUNCSIG__);

	winrt::slim_lock_guard lock(m_Lock);
	m_spSourceReader = nullptr;
	RETURN_IF_FAILED(CreateSourceReader(m_spDevSource.get(),
					    m_spDxgiDevManager.get(),
					    &m_spSourceReader));

	printf("%s, End\n", __FUNCSIG__);
	return S_OK;
}

HRESULT PhysicalCamera::Start(DWORD dwPhyStrmIndex, MF_VideoDataCallback cb,
			      void *pUserData)
{
	printf("%s, Begin\n", __FUNCSIG__);

	HRESULT hr = S_OK;
	winrt::slim_lock_guard lock(m_Lock);

	if (!m_spSourceReader) {
		return E_POINTER;
	}

	// SetOutputResolution was not called
	if (!m_uiWidth || !m_uiHeight) {
		return E_UNEXPECTED;
	}

	m_cbVideoData = cb;
	m_pUserData = pUserData;

	hr = m_spSourceReader->ReadSample(
		dwPhyStrmIndex, // (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
		0, NULL, NULL, NULL, NULL);

	if (FAILED(hr)) {
		m_spDevSource->Shutdown();
	} else {
		hr = m_spSourceReader->SetStreamSelection(dwPhyStrmIndex, TRUE);
	}

	printf("%s, End\n", __FUNCSIG__);
	return hr;
}

HRESULT PhysicalCamera::Stop()
{

	printf("%s, Begin\n", __FUNCSIG__);
	HRESULT hr = S_OK;

	SaveSettingsToDefault();

	{
		winrt::slim_lock_guard lock(m_Lock);

		m_spSourceReader = nullptr;
		m_cbVideoData = nullptr;
		m_pUserData = nullptr;

		m_uiWidth = 0;
		m_uiHeight = 0;
		m_lDefaultStride = 0;
		m_fmt = MF_COLOR_FORMAT_UNKNOWN;

		if (m_pSegMaskBuf) {
			delete[] m_pSegMaskBuf;
		}
		m_pSegMaskBuf = nullptr;
		m_uiSegMaskBufSize = 0;

#ifdef ARGB_SEG_MASK_WORKAROUND
		if (m_pBufferArgbOut) {
			delete[] m_pBufferArgbOut;
		}
		m_pBufferArgbOut = nullptr;
#endif
	}

	printf("%s, End\n", __FUNCSIG__);
	return hr;
}

HRESULT PhysicalCamera::CreateInstance(PhysicalCamera **ppPhysicalCamera)
{
	if (!ppPhysicalCamera) {
		return E_POINTER;
	}

	*ppPhysicalCamera = nullptr;
	PhysicalCamera *pPhysicalCamera = new (std::nothrow) PhysicalCamera;
	if (!pPhysicalCamera) {
		return E_OUTOFMEMORY;
	}

	*ppPhysicalCamera = pPhysicalCamera;

	return S_OK;
}

void resizeBilinearAlpha(BYTE *mask, int w, int h, DWORD *dest, int w2, int h2)
{
	int A, B, C, D, x, y, index, gray;
	float x_ratio = ((float)(w - 1)) / w2;
	float y_ratio = ((float)(h - 1)) / h2;
	float x_diff, y_diff;
	int offset = 0;
	for (int i = 0; i < h2; i++) {
		for (int j = 0; j < w2; j++) {
			x = (int)(x_ratio * j);
			y = (int)(y_ratio * i);
			x_diff = (x_ratio * j) - x;
			y_diff = (y_ratio * i) - y;
			index = y * w + x;

			// range is 0 to 255 thus bitwise AND with 0xff
			A = mask[index] & 0xff;
			B = mask[index + 1] & 0xff;
			C = mask[index + w] & 0xff;
			D = mask[index + w + 1] & 0xff;

			// Y = A(1-w)(1-h) + B(w)(1-h) + C(h)(1-w) + Dwh
			gray = (int)(A * (1 - x_diff) * (1 - y_diff) +
				     B * (x_diff) * (1 - y_diff) +
				     C * (y_diff) * (1 - x_diff) +
				     D * (x_diff * y_diff));

			BYTE *destbyte = (BYTE *)&dest[offset];
			destbyte[3] = gray;
			offset++;
		}
	}
}

HRESULT PhysicalCamera::FillSegMask(IMFSample *pSample)
{

	if (!pSample) {
		return E_POINTER;
	}

	wil::com_ptr_nothrow<IMFAttributes> spMetadataAttri = nullptr;
	RETURN_IF_FAILED(pSample->GetUnknown(MFSampleExtension_CaptureMetadata,
					     IID_PPV_ARGS(&spMetadataAttri)));

	UINT32 cbBlobSize;
	RETURN_IF_FAILED(spMetadataAttri->GetBlobSize(
		MF_CAPTURE_METADATA_FRAME_BACKGROUND_MASK, &cbBlobSize));

	if (m_uiSegMaskBufSize < cbBlobSize) {
		if (m_pSegMaskBuf) {
			delete[] m_pSegMaskBuf;
			m_pSegMaskBuf = nullptr;
		}
	}

	if (!m_pSegMaskBuf) {
		m_pSegMaskBuf = new BYTE[cbBlobSize];
		if (!m_pSegMaskBuf) {
			return E_OUTOFMEMORY;
		}
		m_uiSegMaskBufSize = cbBlobSize;
	}

	UINT32 cbBlobSizeReal = 0;
	RETURN_IF_FAILED(spMetadataAttri->GetBlob(
		MF_CAPTURE_METADATA_FRAME_BACKGROUND_MASK, m_pSegMaskBuf,
		cbBlobSize, &cbBlobSizeReal));

	return S_OK;
}

HRESULT PhysicalCamera::FillAlphaWithSegMask(DWORD *pData)
{
	KSCAMERA_METADATA_BACKGROUNDSEGMENTATIONMASK *mask =
		(KSCAMERA_METADATA_BACKGROUNDSEGMENTATIONMASK *)m_pSegMaskBuf;

	if (!mask) {
		return E_FAIL;
	}

	resizeBilinearAlpha(mask->MaskData, mask->MaskResolution.cx,
			    mask->MaskResolution.cy, pData, m_uiWidth,
			    m_uiHeight);

	return S_OK;
}

#ifdef ARGB_SEG_MASK_WORKAROUND
__forceinline BYTE Clip(int clr)
{
	return (BYTE)(clr < 0 ? 0 : (clr > 255 ? 255 : clr));
}

__forceinline RGBQUAD ConvertYCrCbToRGB(int y, int cr, int cb)
{
	RGBQUAD rgbq;

	int c = y - 16;
	int d = cb - 128;
	int e = cr - 128;

	rgbq.rgbRed = Clip((298 * c + 409 * e + 128) >> 8);
	rgbq.rgbGreen = Clip((298 * c - 100 * d - 208 * e + 128) >> 8);
	rgbq.rgbBlue = Clip((298 * c + 516 * d + 128) >> 8);
	rgbq.rgbReserved = 0;

	return rgbq;
}

void ConvertNv12ToArgb(BYTE *pDst, LONG dstStride, const BYTE *pSrc,
		       LONG srcStride, DWORD dwWidthInPixels,
		       DWORD dwHeightInPixels)
{
	const BYTE *lpBitsY = pSrc;
	const BYTE *lpBitsCb = lpBitsY + (dwHeightInPixels * srcStride);
	;
	const BYTE *lpBitsCr = lpBitsCb + 1;

	for (UINT y = 0; y < dwHeightInPixels; y += 2) {
		const BYTE *lpLineY1 = lpBitsY;
		const BYTE *lpLineY2 = lpBitsY + srcStride;
		const BYTE *lpLineCr = lpBitsCr;
		const BYTE *lpLineCb = lpBitsCb;

		LPBYTE lpDibLine1 = pDst;
		LPBYTE lpDibLine2 = pDst + dstStride;

		for (UINT x = 0; x < dwWidthInPixels; x += 2) {
			int y0 = (int)lpLineY1[0];
			int y1 = (int)lpLineY1[1];
			int y2 = (int)lpLineY2[0];
			int y3 = (int)lpLineY2[1];
			int cb = (int)lpLineCb[0];
			int cr = (int)lpLineCr[0];

			RGBQUAD r = ConvertYCrCbToRGB(y0, cr, cb);
			lpDibLine1[0] = r.rgbBlue;
			lpDibLine1[1] = r.rgbGreen;
			lpDibLine1[2] = r.rgbRed;
			lpDibLine1[3] = 255; // Alpha

			r = ConvertYCrCbToRGB(y1, cr, cb);
			lpDibLine1[4] = r.rgbBlue;
			lpDibLine1[5] = r.rgbGreen;
			lpDibLine1[6] = r.rgbRed;
			lpDibLine1[7] = 255; // Alpha

			r = ConvertYCrCbToRGB(y2, cr, cb);
			lpDibLine2[0] = r.rgbBlue;
			lpDibLine2[1] = r.rgbGreen;
			lpDibLine2[2] = r.rgbRed;
			lpDibLine2[3] = 255; // Alpha

			r = ConvertYCrCbToRGB(y3, cr, cb);
			lpDibLine2[4] = r.rgbBlue;
			lpDibLine2[5] = r.rgbGreen;
			lpDibLine2[6] = r.rgbRed;
			lpDibLine2[7] = 255; // Alpha

			lpLineY1 += 2;
			lpLineY2 += 2;
			lpLineCr += 2;
			lpLineCb += 2;

			lpDibLine1 += 8;
			lpDibLine2 += 8;
		}

		pDst += (2 * dstStride);
		lpBitsY += (2 * srcStride);
		lpBitsCr += srcStride;
		lpBitsCb += srcStride;
	}
}
#endif

HRESULT PhysicalCamera::OnReadSample(HRESULT hrStatus, DWORD dwPhyStrmIndex,
				     DWORD dwStreamFlags, LONGLONG llTimestamp,
				     IMFSample *pSample // Can be NULL
)
{
	HRESULT hr = S_OK;
	winrt::slim_lock_guard lock(m_Lock);

	if (FAILED(hrStatus)) {
		hr = hrStatus;
	}

	if (SUCCEEDED(hr)) {

		bool bHasMask = SUCCEEDED(FillSegMask(pSample));
		if (!bHasMask) {
			printf("#");
		}

		if (pSample && m_cbVideoData && m_uiWidth && m_uiHeight) {

			wil::com_ptr_nothrow<IMFMediaBuffer> spBuffer = nullptr;
			RETURN_IF_FAILED(
				pSample->GetBufferByIndex(0, &spBuffer));

			VideoBufferLock buffer(spBuffer.get());

			LONG lStride = 0;
			BYTE *pbScanline0 = NULL;

			BYTE *pBufferOut = nullptr;
			RETURN_IF_FAILED(
				buffer.LockBuffer(m_lDefaultStride, m_uiHeight,
						  &pbScanline0, &lStride));

			pBufferOut = pbScanline0;

			int bufSize = 0;
			if (m_fmt == MF_COLOR_FORMAT_NV12) {
				bufSize = m_uiWidth * m_uiHeight * 3 / 2;
			} else if (m_fmt == MF_COLOR_FORMAT_ARGB ||
				   m_fmt == MF_COLOR_FORMAT_XRGB) {
				bufSize = m_uiWidth * m_uiHeight * 4;

#ifdef ARGB_SEG_MASK_WORKAROUND
				pBufferOut = m_pBufferArgbOut;
				ConvertNv12ToArgb(m_pBufferArgbOut,
						  m_uiWidth * 4, pbScanline0,
						  lStride, m_uiWidth,
						  m_uiHeight);
#endif
				if (bHasMask && m_transparent) {
					FillAlphaWithSegMask(
						(DWORD *)pBufferOut);
				}
			}

			m_cbVideoData(pBufferOut, bufSize, llTimestamp,
				      m_pUserData);
		}
	}

	// Request the next frame.
	if (SUCCEEDED(hr)) {
		if (m_spSourceReader) {
			hr = m_spSourceReader->ReadSample(
				dwPhyStrmIndex, // (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
				0,
				NULL, // actual
				NULL, // flags
				NULL, // timestamp
				NULL  // sample
			);
		}
	}

	return hr;
}

HRESULT
PhysicalCamera::GetPhysicalCameras(std::vector<CameraInformation> &cameras)
{
	HRESULT hr = S_OK;
	UINT32 uiDeviceCount = 0;
	IMFActivate **ppDevices = nullptr;

	UINT32 cch = 0;
	wchar_t *wszSymbolicLink = NULL;
	wchar_t *wszFriendlyName = NULL;

	wil::com_ptr_nothrow<IMFAttributes> spAttributes = nullptr;
	RETURN_IF_FAILED(MFCreateAttributes(&spAttributes, 1));
	RETURN_IF_FAILED(spAttributes->SetGUID(
		MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
		MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID));

	RETURN_IF_FAILED(MFEnumDeviceSources(spAttributes.get(), &ppDevices,
					     &uiDeviceCount));
	if (!uiDeviceCount) {
		hr = E_FAIL;
		goto done;
	}

	for (size_t idx = 0; idx < uiDeviceCount; idx++) {

		hr = ppDevices[idx]->GetAllocatedString(
			MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,
			&wszSymbolicLink, &cch);
		if (FAILED(hr)) {
			goto done;
		}

		CameraInformation info;
		info.SymbolicLink = wszSymbolicLink;
		CoTaskMemFree(wszSymbolicLink);

		hr = ppDevices[idx]->GetAllocatedString(
			MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &wszFriendlyName,
			&cch);
		if (FAILED(hr)) {
			goto done;
		}

		info.FriendlyName = wszFriendlyName;
		CoTaskMemFree(wszFriendlyName);

		cameras.push_back(info);
	}

done:

	for (DWORD i = 0; i < uiDeviceCount; i++) {
		if (ppDevices[i]) {
			ppDevices[i]->Release();
		}
	}
	CoTaskMemFree(ppDevices);

	return hr;
}

HRESULT FindMatchingMediaType(IMFMediaType *pType,
			      IMFMediaTypeHandler *pPhyHandler,
			      bool bFindMJPGH264, bool bMatchAspectRatio,
			      IMFMediaType **ppMatchingType)
{
	if (!pType || !pPhyHandler || !ppMatchingType) {
		return E_INVALIDARG;
	}

	*ppMatchingType = nullptr;

	DWORD cTypes = 0;
	RETURN_IF_FAILED(pPhyHandler->GetMediaTypeCount(&cTypes));

	UINT32 _cx = 0, _cy = 0, _fpsN = 0, _fpsD = 0;
	RETURN_IF_FAILED(
		MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, &_cx, &_cy));
	RETURN_IF_FAILED(
		MFGetAttributeRatio(pType, MF_MT_FRAME_RATE, &_fpsN, &_fpsD));

	bool found = false;
	UINT32 cx = 0, cy = 0, xc = 0, yc = 0, fpsN = 0, fpsD = 0;

	GUID majorType, subType;
	wil::com_ptr_nothrow<IMFMediaType> spMatchingPhyType;

	long _area = _cx * _cy;
	long _delta = LONG_MAX;
	wil::com_ptr_nothrow<IMFMediaType> spPhyType;
	for (DWORD i = 0; i < cTypes; i++) {
		RETURN_IF_FAILED(
			pPhyHandler->GetMediaTypeByIndex(i, &spPhyType));
		RETURN_IF_FAILED(
			spPhyType->GetGUID(MF_MT_MAJOR_TYPE, &majorType));

		if (majorType != MFMediaType_Video) {
			return E_UNEXPECTED;
		}

		RETURN_IF_FAILED(spPhyType->GetGUID(MF_MT_SUBTYPE, &subType));

		if (bFindMJPGH264 && (subType != MFVideoFormat_MJPG &&
				      subType != MFVideoFormat_H264)) {
			spPhyType.reset();
			continue;
		}

		RETURN_IF_FAILED(MFGetAttributeRatio(
			spPhyType.get(), MF_MT_FRAME_RATE, &fpsN, &fpsD));

		if (fpsN != _fpsN && fpsD != _fpsD) {
			spPhyType.reset();
			continue;
		}

		RETURN_IF_FAILED(MFGetAttributeSize(
			spPhyType.get(), MF_MT_FRAME_SIZE, &cx, &cy));

		double aspect_ratio = (double)cx / (double)cy;
		double _aspect_ratio = (double)_cx / (double)_cy;
		if (bMatchAspectRatio &&
		    fabs(aspect_ratio - _aspect_ratio) > 0.1) {
			spPhyType.reset();
			continue;
		}

		long area = cx * cy;
		long delta = abs(_area - area);
		if (delta < _delta) {
			_delta = delta;
			spMatchingPhyType = nullptr;
			spMatchingPhyType = spPhyType;
			xc = cx;
			yc = cy;
		}

		spPhyType.reset();
	}

	if (!spMatchingPhyType) {
		return E_UNEXPECTED;
	}

	printf("%d x %d : %d x %d - %d / %d : %d / %d", xc, yc, _cx, _cy, fpsN,
	       fpsD, _fpsN, _fpsD);

	wil::com_ptr_nothrow<IMFMediaType> spMatchingPhyTypeClone;
	RETURN_IF_FAILED(MFCreateMediaType(&spMatchingPhyTypeClone));
	RETURN_IF_FAILED(
		spMatchingPhyType->CopyAllItems(spMatchingPhyTypeClone.get()));
	*ppMatchingType = spMatchingPhyTypeClone.detach();

	return S_OK;
}

HRESULT CheckCompressedMediaType(IMFMediaTypeHandler *pPhyHandler,
				 bool &bHasMjpgType, bool &bHasH264Type)
{
	bHasMjpgType = false;
	bHasH264Type = false;

	DWORD cTypes = 0;
	RETURN_IF_FAILED(pPhyHandler->GetMediaTypeCount(&cTypes));

	wil::com_ptr_nothrow<IMFMediaType> spPhyType;
	GUID majorType, subType;
	for (DWORD i = 0; i < cTypes; i++) {
		RETURN_IF_FAILED(
			pPhyHandler->GetMediaTypeByIndex(i, &spPhyType));
		RETURN_IF_FAILED(
			spPhyType->GetGUID(MF_MT_MAJOR_TYPE, &majorType));
		if (majorType != MFMediaType_Video) {
			return E_UNEXPECTED;
		}

		RETURN_IF_FAILED(spPhyType->GetGUID(MF_MT_SUBTYPE, &subType));
		if (subType == MFVideoFormat_MJPG) {
			bHasMjpgType = true;
		} else if (subType == MFVideoFormat_H264) {
			bHasMjpgType = true;
		}

		spPhyType.reset();
	}

	return S_OK;
}

HRESULT PhysicalCamera::FindMatchingNativeMediaType(
	DWORD dwPhyStrmIndex, UINT32 uiWidth, UINT32 uiHeight, UINT32 uiFpsN,
	UINT32 uiFpsD, IMFMediaType **ppMatchingType)
{
	LONGLONG llInterval = 10000000ll * uiFpsD / uiFpsN;
	return FindMatchingNativeMediaType(dwPhyStrmIndex, uiWidth, uiHeight,
					   llInterval, ppMatchingType);
}

HRESULT PhysicalCamera::FindMatchingNativeMediaType(
	DWORD dwPhyStrmIndex, UINT32 uiWidth, UINT32 uiHeight,
	LONGLONG llInterval, IMFMediaType **ppMatchingType)
{
	printf("%s, Begin\n", __FUNCSIG__);
	HRESULT hr = S_OK;

	GUID majorType;
	wil::com_ptr_nothrow<IMFPresentationDescriptor> spPhyPresDesc = nullptr;
	RETURN_IF_FAILED(
		m_spDevSource->CreatePresentationDescriptor(&spPhyPresDesc));

	DWORD dwPhyStrmDescCount = 0;
	RETURN_IF_FAILED(
		spPhyPresDesc->GetStreamDescriptorCount(&dwPhyStrmDescCount));

	*ppMatchingType = nullptr;
	wil::com_ptr_nothrow<IMFStreamDescriptor> spPhyStrmDesc;
	wil::com_ptr_nothrow<IMFMediaTypeHandler> spPhyHandler;

	printf("%s, To get stream descriptor\n", __FUNCSIG__);
	BOOL fSelected;
	RETURN_IF_FAILED(spPhyPresDesc->GetStreamDescriptorByIndex(
		dwPhyStrmIndex, &fSelected, &spPhyStrmDesc));
	RETURN_IF_FAILED(spPhyStrmDesc->GetMediaTypeHandler(&spPhyHandler));

	DWORD cTypes = 0;
	RETURN_IF_FAILED(spPhyHandler->GetMediaTypeCount(&cTypes));

	wil::com_ptr_nothrow<IMFMediaType> spMatchingPhyType = nullptr;
	wil::com_ptr_nothrow<IMFMediaType> spPhyType = nullptr;

	double _maxfps = -1.0;
	UINT32 fpsN = 0, fpsD = 0, cx = 0, cy = 0;
	for (DWORD i = 0; i < cTypes; i++) {
		RETURN_IF_FAILED(
			spPhyHandler->GetMediaTypeByIndex(i, &spPhyType));
		RETURN_IF_FAILED(
			spPhyType->GetGUID(MF_MT_MAJOR_TYPE, &majorType));

		if (majorType != MFMediaType_Video) {
			return E_UNEXPECTED;
		}

		RETURN_IF_FAILED(MFGetAttributeRatio(
			spPhyType.get(), MF_MT_FRAME_RATE, &fpsN, &fpsD));
		RETURN_IF_FAILED(MFGetAttributeSize(
			spPhyType.get(), MF_MT_FRAME_SIZE, &cx, &cy));

		if (cx == uiWidth && cy == uiHeight) {
			if (llInterval == 0) {
				double _fps = (double)fpsN / (double)fpsD;
				if (_fps > _maxfps) {
					spMatchingPhyType = nullptr;
					spMatchingPhyType = spPhyType;
					_maxfps = _fps;
				}
			} else {
				double _fps = (double)fpsN / (double)fpsD;
				double fps =
					(double)10000000.0 / (double)llInterval;
				if (fabs(fps - _fps) < 0.1) {
					spMatchingPhyType = nullptr;
					spMatchingPhyType = spPhyType;
					break;
				}
			}
		}

		spPhyType.reset();
	}

	if (!spMatchingPhyType) {
		return E_UNEXPECTED;
	}

	wil::com_ptr_nothrow<IMFMediaType> spMatchingPhyTypeClone;
	RETURN_IF_FAILED(MFCreateMediaType(&spMatchingPhyTypeClone));
	RETURN_IF_FAILED(
		spMatchingPhyType->CopyAllItems(spMatchingPhyTypeClone.get()));
	*ppMatchingType = spMatchingPhyTypeClone.detach();

	printf("%s, End\n", __FUNCSIG__);
	return hr;
}

HRESULT
PhysicalCamera::GetStreamCapabilities(DWORD dwPhyStrmIndex,
				      std::vector<StreamInformation> &strmCaps)
{
	if (!m_spDevSource) {
		return E_POINTER;
	}

	wil::com_ptr_nothrow<IMFPresentationDescriptor> spPhyPresDesc = nullptr;
	RETURN_IF_FAILED(
		m_spDevSource->CreatePresentationDescriptor(&spPhyPresDesc));

	DWORD dwPhyStrmDescCount = 0;
	RETURN_IF_FAILED(
		spPhyPresDesc->GetStreamDescriptorCount(&dwPhyStrmDescCount));

	wil::com_ptr_nothrow<IMFStreamDescriptor> spPhyStrmDesc;
	wil::com_ptr_nothrow<IMFMediaTypeHandler> spPhyHandler;

	BOOL fSelected;
	RETURN_IF_FAILED(spPhyPresDesc->GetStreamDescriptorByIndex(
		dwPhyStrmIndex, &fSelected, &spPhyStrmDesc));
	RETURN_IF_FAILED(spPhyStrmDesc->GetMediaTypeHandler(&spPhyHandler));

	DWORD cTypes = 0;
	RETURN_IF_FAILED(spPhyHandler->GetMediaTypeCount(&cTypes));

	wil::com_ptr_nothrow<IMFMediaType> spPhyType;
	GUID majorType, subType;
	for (DWORD i = 0; i < cTypes; i++) {
		RETURN_IF_FAILED(
			spPhyHandler->GetMediaTypeByIndex(i, &spPhyType));
		RETURN_IF_FAILED(
			spPhyType->GetGUID(MF_MT_MAJOR_TYPE, &majorType));
		if (majorType != MFMediaType_Video) {
			return E_UNEXPECTED;
		}

		RETURN_IF_FAILED(spPhyType->GetGUID(MF_MT_SUBTYPE, &subType));

		UINT32 _cx, _cy, _fpsN, _fpsD;
		RETURN_IF_FAILED(MFGetAttributeSize(
			spPhyType.get(), MF_MT_FRAME_SIZE, &_cx, &_cy));
		RETURN_IF_FAILED(MFGetAttributeRatio(
			spPhyType.get(), MF_MT_FRAME_RATE, &_fpsN, &_fpsD));

		StreamInformation cap;
		cap.guidSubtype = subType;
		cap.uiWidth = _cx;
		cap.uiHeight = _cy;
		cap.uiFpsN = _fpsN;
		cap.uiFpsD = _fpsD;
		strmCaps.push_back(cap);

		spPhyType.reset();
	}

	return S_OK;
}

HRESULT CreateMediaTypeFrom(IMFMediaType *pType, MF_COLOR_FORMAT fmt,
			    IMFMediaType **ppType)
{
	wil::com_ptr_nothrow<IMFMediaType> spTypeClone;
	RETURN_IF_FAILED(MFCreateMediaType(&spTypeClone));
	RETURN_IF_FAILED(pType->CopyAllItems(spTypeClone.get()));

	UINT32 uiWidth, uiHeight;
	RETURN_IF_FAILED(MFGetAttributeSize(spTypeClone.get(), MF_MT_FRAME_SIZE,
					    &uiWidth, &uiHeight));

	UINT32 uiFpsN, uiFpsD;
	RETURN_IF_FAILED(MFGetAttributeRatio(
		spTypeClone.get(), MF_MT_FRAME_RATE, &uiFpsN, &uiFpsD));

	uint32_t bitrate;
	GUID mffmt = GUID_NULL;

#ifdef ARGB_SEG_MASK_WORKAROUND
	mffmt = MFVideoFormat_NV12;
	bitrate = (uint32_t)(uiHeight * 1.5 * uiWidth * 8 * uiFpsN / uiFpsD);
#else
	if (fmt == MF_COLOR_FORMAT::MF_COLOR_FORMAT_NV12) {
		mffmt = MFVideoFormat_NV12;
		bitrate = (uint32_t)(uiHeight * 1.5 * uiWidth * 8 * uiFpsN /
				     uiFpsD);
	} else if (fmt == MF_COLOR_FORMAT::MF_COLOR_FORMAT_ARGB) {
		mffmt = MFVideoFormat_ARGB32;
		bitrate = (uint32_t)(uiHeight * uiWidth * 8 * 4 * uiFpsN /
				     uiFpsD);
	} else if (fmt == MF_COLOR_FORMAT::MF_COLOR_FORMAT_XRGB) {
		mffmt = MFVideoFormat_RGB32;
		bitrate = (uint32_t)(uiHeight * uiWidth * 8 * 4 * uiFpsN /
				     uiFpsD);
	} else {
		return E_INVALIDARG;
	}
#endif

	RETURN_IF_FAILED(spTypeClone->SetGUID(MF_MT_SUBTYPE, mffmt));
	RETURN_IF_FAILED(spTypeClone->SetUINT32(MF_MT_INTERLACE_MODE,
						MFVideoInterlace_Progressive));
	RETURN_IF_FAILED(
		spTypeClone->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE));
	RETURN_IF_FAILED(MFSetAttributeRatio(spTypeClone.get(),
					     MF_MT_PIXEL_ASPECT_RATIO, 1, 1));

	RETURN_IF_FAILED(spTypeClone->SetUINT32(MF_MT_AVG_BITRATE, bitrate));

	*ppType = spTypeClone.detach();

	return S_OK;
}

HRESULT GetDefaultStride(IMFMediaType *pType, LONG *plStride)
{
	LONG lStride = 0;
	HRESULT hr = pType->GetUINT32(MF_MT_DEFAULT_STRIDE, (UINT32 *)&lStride);
	if (FAILED(hr)) {
		UINT32 cx = 0, cy = 0;
		GUID subType = GUID_NULL;
		RETURN_IF_FAILED(pType->GetGUID(MF_MT_SUBTYPE, &subType));
		RETURN_IF_FAILED(
			MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, &cx, &cy));
		RETURN_IF_FAILED(MFGetStrideForBitmapInfoHeader(subType.Data1,
								cx, &lStride));
	}
	*plStride = lStride;
	return S_OK;
}

HRESULT PhysicalCamera::SetOutputResolution(DWORD dwPhyStrmIndex,
					    UINT32 uiWidth, UINT32 uiHeight,
					    LONGLONG llInterval,
					    MF_COLOR_FORMAT fmt)
{
	printf("%s, Begin\n", __FUNCSIG__);
	winrt::slim_lock_guard lock(m_Lock);

	if (!m_spSourceReader) {
		return E_POINTER;
	}

	wil::com_ptr_nothrow<IMFMediaType> spMatchingType = nullptr;

	RETURN_IF_FAILED(FindMatchingNativeMediaType(dwPhyStrmIndex, uiWidth,
						     uiHeight, llInterval,
						     &spMatchingType));

	wil::com_ptr_nothrow<IMFMediaType> spOutputType = nullptr;
	RETURN_IF_FAILED(
		CreateMediaTypeFrom(spMatchingType.get(), fmt, &spOutputType));

	DWORD dwStreamFlags;
	wil::com_ptr_nothrow<IMFSourceReaderEx> spSourceReaderEx;
	RETURN_IF_FAILED(m_spSourceReader->QueryInterface(&spSourceReaderEx));
	RETURN_IF_FAILED(spSourceReaderEx->SetNativeMediaType(
		dwPhyStrmIndex, spMatchingType.get(), &dwStreamFlags));
	RETURN_IF_FAILED(m_spSourceReader->SetCurrentMediaType(
		dwPhyStrmIndex, NULL, spOutputType.get()));
	RETURN_IF_FAILED(
		m_spSourceReader->SetStreamSelection(dwPhyStrmIndex, TRUE));
	RETURN_IF_FAILED(
		GetDefaultStride(spOutputType.get(), &m_lDefaultStride));

	m_uiWidth = uiWidth;
	m_uiHeight = uiHeight;
	m_fmt = fmt;

#ifdef ARGB_SEG_MASK_WORKAROUND
	if (fmt == MF_COLOR_FORMAT::MF_COLOR_FORMAT_ARGB ||
	    fmt == MF_COLOR_FORMAT::MF_COLOR_FORMAT_XRGB) {
		m_pBufferArgbOut = new BYTE[m_uiWidth * m_uiHeight * 4 + 1024];
		if (!m_pBufferArgbOut) {
			return E_OUTOFMEMORY;
		}
	}
#endif

	printf("%s, End\n", __FUNCSIG__);
	return S_OK;
}

HRESULT PhysicalCamera::GetCurrentStreamInformation(DWORD dwPhyStrmIndex,
						    StreamInformation &strmInfo)
{
	printf("%s, Begin\n", __FUNCSIG__);

	strmInfo = {0};

	winrt::slim_lock_guard lock(m_Lock);
	if (!m_spSourceReader) {
		return E_POINTER;
	}

	wil::com_ptr_nothrow<IMFMediaType> spType;
	RETURN_IF_FAILED(
		m_spSourceReader->GetCurrentMediaType(dwPhyStrmIndex, &spType));

	GUID majorType, subType;
	RETURN_IF_FAILED(spType->GetGUID(MF_MT_MAJOR_TYPE, &majorType));
	if (majorType != MFMediaType_Video) {
		printf("%s, Not Video Type\n", __FUNCSIG__);
		return E_UNEXPECTED;
	}

	RETURN_IF_FAILED(spType->GetGUID(MF_MT_SUBTYPE, &subType));

	UINT32 _cx, _cy, _fpsN, _fpsD;
	RETURN_IF_FAILED(
		MFGetAttributeSize(spType.get(), MF_MT_FRAME_SIZE, &_cx, &_cy));
	RETURN_IF_FAILED(MFGetAttributeRatio(spType.get(), MF_MT_FRAME_RATE,
					     &_fpsN, &_fpsD));

	strmInfo.guidSubtype = subType;
	strmInfo.uiWidth = _cx;
	strmInfo.uiHeight = _cy;
	strmInfo.uiFpsN = _fpsN;
	strmInfo.uiFpsD = _fpsD;

	printf("%s, End\n", __FUNCSIG__);
	return S_OK;
}

HRESULT PhysicalCamera::SetBlur(bool blur, bool shallowFocus, bool mask)
{

	printf("%s, %d, Begin\n", __FUNCSIG__, __LINE__);
	winrt::slim_lock_guard lock(m_Lock);

	ULONG ulPropertyId =
		KSPROPERTY_CAMERACONTROL_EXTENDED_BACKGROUNDSEGMENTATION;
	wil::com_ptr<IMFExtendedCameraControl> spExtControl = nullptr;

	ULONGLONG ulFlags = KSCAMERA_EXTENDEDPROP_BACKGROUNDSEGMENTATION_OFF;

	HRESULT hr = m_spExtController->GetExtendedCameraControl(
		KSCAMERA_EXTENDEDPROP_FILTERSCOPE, ulPropertyId, &spExtControl);
	if (FAILED(hr)) {
		printf("Failed to GetExtendedCameraControl, hr = 0x%x\n", hr);
		goto done;
	}

	if (blur) {
		ulFlags = KSCAMERA_EXTENDEDPROP_BACKGROUNDSEGMENTATION_BLUR;
		if (shallowFocus) {
			ulFlags |=
				KSCAMERA_EXTENDEDPROP_BACKGROUNDSEGMENTATION_SHALLOWFOCUS;
		}
	}

	if (mask) {
		ulFlags |= KSCAMERA_EXTENDEDPROP_BACKGROUNDSEGMENTATION_MASK;
	}

	hr = spExtControl->SetFlags(ulFlags);
	if (FAILED(hr)) {
		printf("Failed to SetFlags, hr = 0x%x\n", hr);
		goto done;
	}

	hr = spExtControl->CommitSettings();
	if (FAILED(hr)) {
		printf("Failed to CommitSettings, hr = 0x%x\n", hr);
		goto done;
	}

	for (auto &setting : s_MepSettings) {
		if (setting.SymbolicLink == m_wsSymbolicName) {
			setting.Blur = blur;
			setting.ShallowFocus = shallowFocus;
			setting.Mask = mask;
			break;
		}
	}

done:

	printf("%s, %d, End, hr = 0x%x\n", __FUNCSIG__, __LINE__, hr);
	return hr;
}

HRESULT PhysicalCamera::GetBlur(bool &blur, bool &shallowFocus, bool &mask)
{
	printf("%s, %d, Begin\n", __FUNCSIG__, __LINE__);
	winrt::slim_lock_guard lock(m_Lock);

	ULONG ulPropertyId =
		KSPROPERTY_CAMERACONTROL_EXTENDED_BACKGROUNDSEGMENTATION;
	wil::com_ptr<IMFExtendedCameraControl> spExtControl = nullptr;

	ULONGLONG ulFlags = KSCAMERA_EXTENDEDPROP_BACKGROUNDSEGMENTATION_OFF;

	HRESULT hr = m_spExtController->GetExtendedCameraControl(
		KSCAMERA_EXTENDEDPROP_FILTERSCOPE, ulPropertyId, &spExtControl);
	if (FAILED(hr)) {
		printf("Failed to GetExtendedCameraControl, hr = 0x%x\n", hr);
		return hr;
	}

	ulFlags = spExtControl->GetFlags();

	blur = ulFlags & KSCAMERA_EXTENDEDPROP_BACKGROUNDSEGMENTATION_BLUR;
	shallowFocus =
		ulFlags &
		KSCAMERA_EXTENDEDPROP_BACKGROUNDSEGMENTATION_SHALLOWFOCUS;
	mask = ulFlags & KSCAMERA_EXTENDEDPROP_BACKGROUNDSEGMENTATION_MASK;

	printf("%s, %d, End, hr = 0x%x\n", __FUNCSIG__, __LINE__, hr);
	return hr;
}

HRESULT PhysicalCamera::SetAutoFraming(bool enable)
{
	printf("%s, %d, Begin\n", __FUNCSIG__, __LINE__);
	winrt::slim_lock_guard lock(m_Lock);

	ULONG ulPropertyId = KSPROPERTY_CAMERACONTROL_EXTENDED_DIGITALWINDOW;
	wil::com_ptr<IMFExtendedCameraControl> spExtControl = nullptr;

	ULONGLONG ulFlags = KSCAMERA_EXTENDEDPROP_DIGITALWINDOW_MANUAL;

	HRESULT hr = m_spExtController->GetExtendedCameraControl(
		KSCAMERA_EXTENDEDPROP_FILTERSCOPE, ulPropertyId, &spExtControl);
	if (FAILED(hr)) {
		printf("Failed to GetExtendedCameraControl, hr = 0x%x\n", hr);
		goto done;
	}

	printf("%s, Succeeded to GetExtendedCameraControl\n", __FUNCSIG__);

	if (enable) {
		ulFlags = KSCAMERA_EXTENDEDPROP_DIGITALWINDOW_AUTOFACEFRAMING;
	}

	hr = spExtControl->SetFlags(ulFlags);
	if (FAILED(hr)) {
		printf("Failed to SetFlags, hr = 0x%x\n", hr);
		goto done;
	}

	printf("%s, Succeeded to SetFlags\n", __FUNCSIG__);

	hr = spExtControl->CommitSettings();
	if (FAILED(hr)) {
		printf("Failed to CommitSettings, hr = 0x%x\n", hr);
		goto done;
	}

	for (auto &setting : s_MepSettings) {
		if (setting.SymbolicLink == m_wsSymbolicName) {
			setting.AutoFraming = enable;
			break;
		}
	}

	printf("%s, Succeeded to CommitSettings\n", __FUNCSIG__);

done:

	printf("%s, %d, End, hr = 0x%x\n", __FUNCSIG__, __LINE__, hr);
	return hr;
}

HRESULT PhysicalCamera::GetAutoFraming(bool &enable)
{
	printf("%s, %d, Begin\n", __FUNCSIG__, __LINE__);
	winrt::slim_lock_guard lock(m_Lock);

	ULONG ulPropertyId = KSPROPERTY_CAMERACONTROL_EXTENDED_DIGITALWINDOW;
	wil::com_ptr<IMFExtendedCameraControl> spExtControl = nullptr;

	ULONGLONG ulFlags = KSCAMERA_EXTENDEDPROP_DIGITALWINDOW_MANUAL;

	HRESULT hr = m_spExtController->GetExtendedCameraControl(
		KSCAMERA_EXTENDEDPROP_FILTERSCOPE, ulPropertyId, &spExtControl);
	if (FAILED(hr)) {
		printf("Failed to GetExtendedCameraControl, hr = 0x%x\n", hr);
		return hr;
	}

	enable = false;
	ulFlags = spExtControl->GetFlags();
	if (ulFlags & KSCAMERA_EXTENDEDPROP_DIGITALWINDOW_AUTOFACEFRAMING) {
		enable = true;
	}

	printf("%s, %d, End, hr = 0x%x\n", __FUNCSIG__, __LINE__, hr);
	return hr;
}

HRESULT PhysicalCamera::SetEyeGazeCorrection(bool enable)
{
	printf("%s, %d, Begin\n", __FUNCSIG__, __LINE__);
	winrt::slim_lock_guard lock(m_Lock);

	ULONG ulPropertyId =
		KSPROPERTY_CAMERACONTROL_EXTENDED_EYEGAZECORRECTION;
	wil::com_ptr<IMFExtendedCameraControl> spExtControl = nullptr;

	ULONGLONG ulFlags = KSCAMERA_EXTENDEDPROP_EYEGAZECORRECTION_OFF;

	HRESULT hr = m_spExtController->GetExtendedCameraControl(
		KSCAMERA_EXTENDEDPROP_FILTERSCOPE, ulPropertyId, &spExtControl);
	if (FAILED(hr)) {
		printf("Failed to GetExtendedCameraControl, hr = 0x%x\n", hr);
		goto done;
	}

	if (enable) {
		ulFlags = KSCAMERA_EXTENDEDPROP_EYEGAZECORRECTION_ON;
	}

	hr = spExtControl->SetFlags(ulFlags);
	if (FAILED(hr)) {
		printf("Failed to SetFlags, hr = 0x%x\n", hr);
		goto done;
	}

	hr = spExtControl->CommitSettings();
	if (FAILED(hr)) {
		printf("Failed to CommitSettings, hr = 0x%x\n", hr);
		goto done;
	}

	for (auto &setting : s_MepSettings) {
		if (setting.SymbolicLink == m_wsSymbolicName) {
			setting.EyeGazeCorrection = enable;
			break;
		}
	}

done:

	printf("%s, %d, End, hr = 0x%x\n", __FUNCSIG__, __LINE__, hr);
	return hr;
}

HRESULT PhysicalCamera::GetEyeGazeCorrection(bool &enable)
{
	printf("%s, %d, Begin\n", __FUNCSIG__, __LINE__);
	winrt::slim_lock_guard lock(m_Lock);

	ULONG ulPropertyId =
		KSPROPERTY_CAMERACONTROL_EXTENDED_EYEGAZECORRECTION;
	wil::com_ptr<IMFExtendedCameraControl> spExtControl = nullptr;

	ULONGLONG ulFlags = KSCAMERA_EXTENDEDPROP_EYEGAZECORRECTION_OFF;

	HRESULT hr = m_spExtController->GetExtendedCameraControl(
		KSCAMERA_EXTENDEDPROP_FILTERSCOPE, ulPropertyId, &spExtControl);
	if (FAILED(hr)) {
		printf("Failed to GetExtendedCameraControl, hr = 0x%x\n", hr);
		return hr;
	}

	enable = false;
	ulFlags = spExtControl->GetFlags();
	if (ulFlags & KSCAMERA_EXTENDEDPROP_EYEGAZECORRECTION_ON) {
		enable = true;
	}

	printf("%s, %d, End, hr = 0x%x\n", __FUNCSIG__, __LINE__, hr);
	return hr;
}

std::vector<MepSetting> PhysicalCamera::s_MepSettings;
HRESULT PhysicalCamera::GetMepSetting(MepSetting &setting)
{

	bool blur, shallowfocus, mask, autoframing, eyegaze;

	if (m_spExtController == nullptr)
		return S_OK;

	RETURN_IF_FAILED(GetBlur(blur, shallowfocus, mask));
	setting.Blur = blur;
	setting.ShallowFocus = shallowfocus;
	setting.Mask = mask;

	RETURN_IF_FAILED(GetAutoFraming(autoframing));
	setting.AutoFraming = autoframing;

	RETURN_IF_FAILED(GetEyeGazeCorrection(eyegaze));
	setting.EyeGazeCorrection = eyegaze;

	return S_OK;
}

HRESULT PhysicalCamera::SetMepSetting(MepSetting &setting)
{

	RETURN_IF_FAILED(
		SetBlur(setting.Blur, setting.ShallowFocus, setting.Mask));
	RETURN_IF_FAILED(SetAutoFraming(setting.AutoFraming));
	RETURN_IF_FAILED(SetEyeGazeCorrection(setting.EyeGazeCorrection));
	return S_OK;
}

HRESULT PhysicalCamera::SaveSettingsToDefault()
{
	bool found = false;
	for (auto &setting : s_MepSettings) {
		if (setting.SymbolicLink == m_wsSymbolicName) {
			RETURN_IF_FAILED(GetMepSetting(setting));
			found = true;
			break;
		}
	}

	if (!found) {
		MepSetting setting;
		setting.SymbolicLink = m_wsSymbolicName;
		RETURN_IF_FAILED(GetMepSetting(setting));
		s_MepSettings.push_back(setting);
	}
	return S_OK;
}

HRESULT PhysicalCamera::RestoreDefaultSettings()
{

	for (auto &setting : s_MepSettings) {
		if (setting.SymbolicLink == m_wsSymbolicName) {
			RETURN_IF_FAILED(SetMepSetting(setting));
			break;
		}
	}

	return S_OK;
}
