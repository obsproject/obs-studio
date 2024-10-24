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
	MF_COLOR_FORMAT_XRGB
};

typedef HANDLE CAPTURE_DEVICE_HANDLE;
typedef void(__stdcall *MF_VideoDataCallback)(void *pData, int Size,
					      long long llTimestamp,
					      void *pUserData);
typedef void(__stdcall *MF_EnumerateCameraCallback)(const wchar_t *Name,
						    const wchar_t *DevId,
						    void *pUserData);
typedef void(__stdcall *MF_EnumerateStreamCapabilitiesCallback)(
	UINT32 Width, UINT32 Height, UINT32 FpsN, UINT32 FpsD, void *pUserData);

MFCAPTURE_EXPORTS HRESULT MF_EnumerateCameras(MF_EnumerateCameraCallback cb,
					      void *pUserData);

MFCAPTURE_EXPORTS CAPTURE_DEVICE_HANDLE MF_Create(const wchar_t *DevId);

MFCAPTURE_EXPORTS void MF_Destroy(CAPTURE_DEVICE_HANDLE h);

MFCAPTURE_EXPORTS HRESULT MF_Prepare(CAPTURE_DEVICE_HANDLE h);

MFCAPTURE_EXPORTS HRESULT MF_EnumerateStreamCapabilities(
	CAPTURE_DEVICE_HANDLE h, MF_EnumerateStreamCapabilitiesCallback cb,
	void *pUserData);

// if llInterval is 0, set the camera to us the max fps
// otherwise, set camera with matching fps.
MFCAPTURE_EXPORTS HRESULT MF_SetOutputResolution(CAPTURE_DEVICE_HANDLE h,
						 UINT32 uiWidth,
						 UINT32 uiHeight,
						 LONGLONG llInterval,
						 MF_COLOR_FORMAT fmt);

// LONGLONG llInterval = 10000000ll * uiFpsD / uiFpsN;
MFCAPTURE_EXPORTS HRESULT MF_GetOutputResolution(CAPTURE_DEVICE_HANDLE h,
						 UINT32 *uiWidth,
						 UINT32 *uiHeight,
						 UINT32 *uiFpsN,
						 UINT32 *uiFpsD);

MFCAPTURE_EXPORTS HRESULT MF_Start(CAPTURE_DEVICE_HANDLE h,
				   MF_VideoDataCallback cb, void *pUserData);

MFCAPTURE_EXPORTS HRESULT MF_Stop(CAPTURE_DEVICE_HANDLE h);

MFCAPTURE_EXPORTS HRESULT MF_GetBlur(CAPTURE_DEVICE_HANDLE h, bool &blur,
				     bool &shallowFocus, bool &mask);

MFCAPTURE_EXPORTS HRESULT MF_SetBlur(CAPTURE_DEVICE_HANDLE h, BOOL blur,
				     BOOL shallowFocus, BOOL mask);

MFCAPTURE_EXPORTS HRESULT MF_GetTransparent(CAPTURE_DEVICE_HANDLE h,
					    bool &enable);

MFCAPTURE_EXPORTS HRESULT MF_GetAutoFraming(CAPTURE_DEVICE_HANDLE h,
					    bool &enable);

MFCAPTURE_EXPORTS HRESULT MF_SetAutoFraming(CAPTURE_DEVICE_HANDLE h,
					    BOOL enable);

MFCAPTURE_EXPORTS HRESULT MF_GetEyeGazeCorrection(CAPTURE_DEVICE_HANDLE h,
						  bool &enable);

MFCAPTURE_EXPORTS HRESULT MF_SetEyeGazeCorrection(CAPTURE_DEVICE_HANDLE h,
						  BOOL enable);

MFCAPTURE_EXPORTS HRESULT MF_RestoreDefaultSettings(CAPTURE_DEVICE_HANDLE h);

#ifdef __cplusplus
}
#endif
