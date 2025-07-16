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
