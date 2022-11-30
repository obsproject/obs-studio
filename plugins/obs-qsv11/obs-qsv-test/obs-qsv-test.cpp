#include "mfxstructures.h"
#include "mfxadapter.h"
#include "mfxvideo++.h"
#include "../common_utils.h"

#include <util/windows/ComPtr.hpp>

#include <dxgi.h>
#include <d3d11.h>
#include <d3d11_1.h>

#include <string>
#include <map>

#ifdef _MSC_VER
extern "C" __declspec(dllexport) DWORD NvOptimusEnablement = 1;
extern "C" __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
#endif

#define INTEL_VENDOR_ID 0x8086

struct adapter_caps {
	bool is_intel = false;
	bool is_dgpu = false;
	bool supports_av1 = false;
	bool supports_hevc = false;
};

static std::map<uint32_t, adapter_caps> adapter_info;

static bool has_encoder(mfxIMPL impl, mfxU32 codec_id)
{
	MFXVideoSession session;
	mfxInitParam init_param = {};
	init_param.Implementation = impl;
	init_param.Version.Major = 1;
	init_param.Version.Minor = 0;
	mfxStatus sts = session.InitEx(init_param);

	mfxVideoParam video_param;
	memset(&video_param, 0, sizeof(video_param));
	video_param.mfx.CodecId = codec_id;
	video_param.mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;
	video_param.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
	video_param.mfx.FrameInfo.Width = MSDK_ALIGN16(1280);
	video_param.mfx.FrameInfo.Height = MSDK_ALIGN16(720);
	sts = MFXVideoENCODE_Query(session, &video_param, &video_param);
	session.Close();

	return sts == MFX_ERR_NONE;
}

static bool get_adapter_caps(IDXGIFactory *factory, uint32_t adapter_idx)
{
	mfxIMPL impls[4] = {MFX_IMPL_HARDWARE, MFX_IMPL_HARDWARE2,
			    MFX_IMPL_HARDWARE3, MFX_IMPL_HARDWARE4};
	HRESULT hr;

	ComPtr<IDXGIAdapter> adapter;
	hr = factory->EnumAdapters(adapter_idx, &adapter);
	if (FAILED(hr))
		return false;

	adapter_caps &caps = adapter_info[adapter_idx];

	DXGI_ADAPTER_DESC desc;
	adapter->GetDesc(&desc);

	if (desc.VendorId != INTEL_VENDOR_ID)
		return true;

	bool dgpu = desc.DedicatedVideoMemory > 512 * 1024 * 1024;
	mfxIMPL impl = impls[adapter_idx];

	caps.is_intel = true;
	caps.is_dgpu = dgpu;
	caps.supports_av1 = has_encoder(impl, MFX_CODEC_AV1);
#if ENABLE_HEVC
	caps.supports_hevc = has_encoder(impl, MFX_CODEC_HEVC);
#endif

	return true;
}

DWORD WINAPI TimeoutThread(LPVOID param)
{
	HANDLE hMainThread = (HANDLE)param;

	DWORD ret = WaitForSingleObject(hMainThread, 8000);
	if (ret == WAIT_TIMEOUT)
		TerminateProcess(GetCurrentProcess(), STATUS_TIMEOUT);

	CloseHandle(hMainThread);

	return 0;
}

int main(void)
try {
	ComPtr<IDXGIFactory> factory;
	HRESULT hr;

	HANDLE hMainThread;
	DuplicateHandle(GetCurrentProcess(), GetCurrentThread(),
			GetCurrentProcess(), &hMainThread, 0, FALSE,
			DUPLICATE_SAME_ACCESS);
	DWORD threadId;
	HANDLE hThread;
	hThread =
		CreateThread(NULL, 0, TimeoutThread, hMainThread, 0, &threadId);
	CloseHandle(hThread);

	/* --------------------------------------------------------- */
	/* query qsv support                                         */

	hr = CreateDXGIFactory1(__uuidof(IDXGIFactory), (void **)&factory);
	if (FAILED(hr))
		throw "CreateDXGIFactory1 failed";

	uint32_t idx = 0;
	while (get_adapter_caps(factory, idx++) && idx < 4)
		;

	for (auto &[idx, caps] : adapter_info) {
		printf("[%u]\n", idx);
		printf("is_intel=%s\n", caps.is_intel ? "true" : "false");
		printf("is_dgpu=%s\n", caps.is_dgpu ? "true" : "false");
		printf("supports_av1=%s\n",
		       caps.supports_av1 ? "true" : "false");
		printf("supports_hevc=%s\n",
		       caps.supports_hevc ? "true" : "false");
	}

	return 0;
} catch (const char *text) {
	printf("[error]\nstring=%s\n", text);
	return 0;
}
