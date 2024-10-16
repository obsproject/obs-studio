#include "mfcapture.hpp"
#include "DeviceControlChangeListener.hpp"
#include "PhysicalCamera.hpp"

MFCAPTURE_EXPORTS HRESULT MF_EnumerateCameras(MF_EnumerateCameraCallback cb,
					      void *pUserData)
{
	std::vector<CameraInformation> camlist;
	RETURN_IF_FAILED(PhysicalCamera::GetPhysicalCameras(camlist));

	for (auto &cam : camlist) {
		if (cb) {
			cb(cam.FriendlyName.c_str(), cam.SymbolicLink.c_str(),
			   pUserData);
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

MFCAPTURE_EXPORTS HRESULT MF_EnumerateStreamCapabilities(
	CAPTURE_DEVICE_HANDLE h, MF_EnumerateStreamCapabilitiesCallback cb,
	void *pUserData)
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
			cb(cap.uiWidth, cap.uiHeight, cap.uiFpsN, cap.uiFpsD,
			   pUserData);
		}
	}

	return S_OK;
}

MFCAPTURE_EXPORTS HRESULT MF_SetOutputResolution(CAPTURE_DEVICE_HANDLE h,
						 UINT32 uiWidth,
						 UINT32 uiHeight,
						 LONGLONG llInterval,
						 MF_COLOR_FORMAT fmt)
{
	PhysicalCamera *p = (PhysicalCamera *)h;
	if (!p) {
		return E_INVALIDARG;
	}

	return p->SetOutputResolution(0, uiWidth, uiHeight, llInterval, fmt);
}

MFCAPTURE_EXPORTS HRESULT MF_GetOutputResolution(CAPTURE_DEVICE_HANDLE h,
						 UINT32 *uiWidth,
						 UINT32 *uiHeight,
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

MFCAPTURE_EXPORTS HRESULT MF_Start(CAPTURE_DEVICE_HANDLE h,
				   MF_VideoDataCallback cb, void *pUserData)
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

MFCAPTURE_EXPORTS HRESULT MF_GetBlur(CAPTURE_DEVICE_HANDLE h, bool &blur,
				     bool &shallowFocus, bool &mask)
{
	PhysicalCamera *p = (PhysicalCamera *)h;
	if (!p) {
		return E_INVALIDARG;
	}

	return p->GetBlur(blur, shallowFocus, mask);
}

MFCAPTURE_EXPORTS HRESULT MF_SetBlur(CAPTURE_DEVICE_HANDLE h, BOOL blur,
				     BOOL shallowFocus, BOOL mask)
{
	PhysicalCamera *p = (PhysicalCamera *)h;
	if (!p) {
		return E_INVALIDARG;
	}

	p->SetTransparent(mask);

	return p->SetBlur(blur, shallowFocus, mask);
}

MFCAPTURE_EXPORTS HRESULT MF_GetTransparent(CAPTURE_DEVICE_HANDLE h,
					    bool &enable)
{
	PhysicalCamera *p = (PhysicalCamera *)h;
	if (!p) {
		return E_INVALIDARG;
	}

	enable = p->GetTransparent();

	return S_OK;
}

MFCAPTURE_EXPORTS HRESULT MF_GetAutoFraming(CAPTURE_DEVICE_HANDLE h,
					    bool &enable)
{
	PhysicalCamera *p = (PhysicalCamera *)h;
	if (!p) {
		return E_INVALIDARG;
	}

	return p->GetAutoFraming(enable);
}

MFCAPTURE_EXPORTS HRESULT MF_SetAutoFraming(CAPTURE_DEVICE_HANDLE h,
					    BOOL enable)
{
	PhysicalCamera *p = (PhysicalCamera *)h;
	if (!p) {
		return E_INVALIDARG;
	}

	return p->SetAutoFraming(enable);
}

MFCAPTURE_EXPORTS HRESULT MF_GetEyeGazeCorrection(CAPTURE_DEVICE_HANDLE h,
						  bool &enable)
{
	PhysicalCamera *p = (PhysicalCamera *)h;
	if (!p) {
		return E_INVALIDARG;
	}

	return p->GetEyeGazeCorrection(enable);
}

MFCAPTURE_EXPORTS HRESULT MF_SetEyeGazeCorrection(CAPTURE_DEVICE_HANDLE h,
						  BOOL enable)
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
