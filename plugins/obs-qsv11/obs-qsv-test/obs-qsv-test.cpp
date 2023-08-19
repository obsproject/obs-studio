#include <vpl/mfxstructures.h>
#include <vpl/mfxadapter.h>
#include <vpl/mfxvideo++.h>
#include "../common_utils.h"

#include <util/windows/ComPtr.hpp>

#include <dxgi.h>
#include <d3d11.h>
#include <d3d11_1.h>

#include <vector>
#include <string>
#include <map>

#ifdef _MSC_VER
extern "C" __declspec(dllexport) DWORD NvOptimusEnablement = 1;
extern "C" __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
#endif

struct adapter_caps {
	bool is_intel = false;
	bool is_dgpu = false;
	bool supports_av1 = false;
	bool supports_hevc = false;
};

static std::vector<uint64_t> luid_order;
static std::map<uint32_t, adapter_caps> adapter_info;

static bool has_encoder(mfxSession m_session, mfxU32 codec_id)
{
	MFXVideoENCODE *session = new MFXVideoENCODE(m_session);

	mfxVideoParam video_param;
	memset(&video_param, 0, sizeof(video_param));
	video_param.mfx.CodecId = codec_id;
	video_param.mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;
	video_param.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
	video_param.mfx.FrameInfo.Width = MSDK_ALIGN16(1280);
	video_param.mfx.FrameInfo.Height = MSDK_ALIGN16(720);
	mfxStatus sts = session->Query(&video_param, &video_param);
	session->Close();

	return sts == MFX_ERR_NONE;
}

static inline uint32_t get_adapter_idx(uint32_t adapter_idx, LUID luid)
{
	for (size_t i = 0; i < luid_order.size(); i++) {
		if (luid_order[i] == *(uint64_t *)&luid) {
			return (uint32_t)i;
		}
	}

	return adapter_idx;
}

static bool get_adapter_caps(IDXGIFactory *factory, mfxLoader loader,
			     mfxSession m_session, uint32_t adapter_idx)
{
	HRESULT hr;
	static uint32_t idx_adjustment = 0;

	ComPtr<IDXGIAdapter> adapter;
	hr = factory->EnumAdapters(adapter_idx, &adapter);
	if (FAILED(hr))
		return false;

	DXGI_ADAPTER_DESC desc;
	adapter->GetDesc(&desc);

	uint32_t luid_idx = get_adapter_idx(adapter_idx, desc.AdapterLuid);
	adapter_caps &caps = adapter_info[luid_idx];
	if (desc.VendorId != INTEL_VENDOR_ID) {
		idx_adjustment++;
		return true;
	}

	caps.is_intel = true;

	mfxImplDescription *idesc;
	mfxStatus sts =
		MFXEnumImplementations(loader, adapter_idx - idx_adjustment,
				       MFX_IMPLCAPS_IMPLDESCSTRUCTURE,
				       reinterpret_cast<mfxHDL *>(&idesc));

	if (sts != MFX_ERR_NONE)
		return false;

	caps.is_dgpu = false;
	if (idesc->Dev.MediaAdapterType == MFX_MEDIA_DISCRETE)
		caps.is_dgpu = true;

	caps.supports_av1 = false;
	caps.supports_hevc = false;
	mfxEncoderDescription *enc = &idesc->Enc;
	if (enc->NumCodecs != 0) {
		for (int codec = 0; codec < enc->NumCodecs; codec++) {
			if (enc->Codecs[codec].CodecID == MFX_CODEC_AV1)
				caps.supports_av1 = true;
#if ENABLE_HEVC
			if (enc->Codecs[codec].CodecID == MFX_CODEC_HEVC)
				caps.supports_hevc = true;
#endif
		}
	} else {
		// Encoder information is not available before TGL for VPL, so the MSDK legacy approach is taken
		caps.supports_av1 = has_encoder(m_session, MFX_CODEC_AV1);
#if ENABLE_HEVC
		caps.supports_hevc = has_encoder(m_session, MFX_CODEC_HEVC);
#endif
	}

	MFXDispReleaseImplDescription(loader, idesc);

	return true;
}

#define CHECK_TIMEOUT_MS 10000

DWORD WINAPI TimeoutThread(LPVOID param)
{
	HANDLE hMainThread = (HANDLE)param;

	DWORD ret = WaitForSingleObject(hMainThread, CHECK_TIMEOUT_MS);
	if (ret == WAIT_TIMEOUT)
		TerminateProcess(GetCurrentProcess(), STATUS_TIMEOUT);

	CloseHandle(hMainThread);

	return 0;
}

int main(int argc, char *argv[])
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
	/* parse expected LUID order                                 */

	for (int i = 1; i < argc; i++) {
		luid_order.push_back(strtoull(argv[i], NULL, 16));
	}

	/* --------------------------------------------------------- */
	/* query qsv support                                         */

	hr = CreateDXGIFactory1(__uuidof(IDXGIFactory), (void **)&factory);
	if (FAILED(hr))
		throw "CreateDXGIFactory1 failed";

	mfxLoader loader = MFXLoad();
	if (!loader)
		throw "MFXLoad failed";

	mfxConfig cfg = MFXCreateConfig(loader);
	if (!cfg)
		throw "MFXCreateConfig failed";

	mfxVariant impl;

	// Low latency is disabled due to encoding capabilities not being provided before TGL for VPL
	impl.Type = MFX_VARIANT_TYPE_U32;
	impl.Data.U32 = MFX_IMPL_TYPE_HARDWARE;
	MFXSetConfigFilterProperty(
		cfg, (const mfxU8 *)"mfxImplDescription.Impl", impl);

	mfxSession m_session = nullptr;
	mfxStatus sts = MFXCreateSession(loader, 0, &m_session);

	uint32_t idx = 0;
	while (get_adapter_caps(factory, loader, m_session, idx++) == true)
		;

	if (m_session)
		MFXClose(m_session);

	MFXUnload(loader);

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
