#pragma once

#include <obs-module.h>

#include <mfapi.h>
#include <functional>
#include <comdef.h>
#include <chrono>
#include <d3d11.h>
#include <wrl/client.h>

#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "D3D11.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "Winmm.lib")

#ifndef CHECK_HR_ERROR
#define CHECK_HR_ERROR(r)                                 \
	if (FAILED(hr = (r))) {                \
		MF_LOG_COM(LOG_ERROR, #r, hr); \
		goto fail;                     \
	}
#endif

#ifndef CHECK_HR_LEVEL
#define CHECK_HR_LEVEL(level, r)                 \
	if (FAILED(hr = (r))) {            \
		MF_LOG_COM(level, #r, hr); \
		goto fail;                 \
	}
#endif

#ifndef CHECK_HR_WARNING
#define CHECK_HR_WARNING(r)                \
	if (FAILED(hr = (r))) \
		MF_LOG_COM(LOG_WARNING, #r, hr);
#endif

namespace MF {
enum Status { FAILURE, SUCCESS, NOT_ACCEPTING, NEED_MORE_INPUT };

bool LogMediaType(IMFMediaType *mediaType);
void MF_LOG(int level, const char *format, ...);
void MF_LOG_COM(int level, const char *msg, HRESULT hr);
const GUID &GetMFVideoFormat(video_format format);
void Copy_Tex(void *tex, uint64_t lock_key, uint64_t *next_key, ID3D11Device *g_pD3D11Device_av1_1,
	      ID3D11DeviceContext *g_pD3D11Ctx_av1_1, ID3D11Texture2D *pSurface_av1_1);
template<typename T> using ComPtr_Dev = Microsoft::WRL::ComPtr<T>;
HRESULT CreateD3D11EncoderResources(const video_output_info *voi, ID3D11Device **ppDevice,
				    ID3D11DeviceContext **ppContext, ComPtr_Dev<IMFDXGIDeviceManager> &deviceManager,
				    ID3D11Texture2D **ppSurface);
} // namespace MF
