#include <AMF/core/Factory.h>
#include <AMF/core/Trace.h>
#include <AMF/components/VideoEncoderVCE.h>
#include <AMF/components/VideoEncoderHEVC.h>
#include <AMF/components/VideoEncoderAV1.h>

#include <util/windows/ComPtr.hpp>

#include <dxgi.h>
#include <d3d11.h>
#include <d3d11_1.h>

#include <vector>
#include <string>
#include <map>

using namespace amf;

#ifdef _MSC_VER
extern "C" __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
#endif

#define AMD_VENDOR_ID 0x1002

struct adapter_caps {
	bool is_amd = false;
	bool supports_avc = false;
	bool supports_hevc = false;
	bool supports_av1 = false;
};

static AMFFactory *amf_factory = nullptr;
static std::vector<uint64_t> luid_order;
static std::map<uint32_t, adapter_caps> adapter_info;

static bool has_encoder(AMFContextPtr &amf_context, const wchar_t *encoder_name)
{
	AMFComponentPtr encoder;
	AMF_RESULT res = amf_factory->CreateComponent(amf_context, encoder_name, &encoder);
	return res == AMF_OK;
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

static bool get_adapter_caps(IDXGIFactory *factory, uint32_t adapter_idx)
{
	AMF_RESULT res;
	HRESULT hr;

	ComPtr<IDXGIAdapter> adapter;
	hr = factory->EnumAdapters(adapter_idx, &adapter);
	if (FAILED(hr))
		return false;

	DXGI_ADAPTER_DESC desc;
	adapter->GetDesc(&desc);

	uint32_t luid_idx = get_adapter_idx(adapter_idx, desc.AdapterLuid);
	adapter_caps &caps = adapter_info[luid_idx];

	if (desc.VendorId != AMD_VENDOR_ID)
		return true;

	caps.is_amd = true;

	ComPtr<ID3D11Device> device;
	ComPtr<ID3D11DeviceContext> context;
	hr = D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &device,
			       nullptr, &context);
	if (FAILED(hr))
		return true;

	AMFContextPtr amf_context;
	res = amf_factory->CreateContext(&amf_context);
	if (res != AMF_OK)
		return true;

	res = amf_context->InitDX11(device);
	if (res != AMF_OK)
		return true;

	caps.supports_avc = has_encoder(amf_context, AMFVideoEncoderVCE_AVC);
	caps.supports_hevc = has_encoder(amf_context, AMFVideoEncoder_HEVC);
	caps.supports_av1 = has_encoder(amf_context, AMFVideoEncoder_AV1);

	return true;
}

DWORD WINAPI TimeoutThread(LPVOID param)
{
	HANDLE hMainThread = (HANDLE)param;

	DWORD ret = WaitForSingleObject(hMainThread, 2500);
	if (ret == WAIT_TIMEOUT)
		TerminateProcess(GetCurrentProcess(), STATUS_TIMEOUT);

	CloseHandle(hMainThread);

	return 0;
}

int main(int argc, char *argv[])
try {
	ComPtr<IDXGIFactory> factory;
	AMF_RESULT res;
	HRESULT hr;

	HANDLE hMainThread;
	DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &hMainThread, 0, FALSE,
			DUPLICATE_SAME_ACCESS);
	DWORD threadId;
	HANDLE hThread;
	hThread = CreateThread(NULL, 0, TimeoutThread, hMainThread, 0, &threadId);
	CloseHandle(hThread);

	/* --------------------------------------------------------- */
	/* try initializing amf, I guess                             */

	HMODULE amf_module = LoadLibraryW(AMF_DLL_NAME);
	if (!amf_module)
		throw "Failed to load AMF lib";

	auto init = (AMFInit_Fn)GetProcAddress(amf_module, AMF_INIT_FUNCTION_NAME);
	if (!init)
		throw "Failed to get init func";

	res = init(AMF_FULL_VERSION, &amf_factory);
	if (res != AMF_OK)
		throw "AMFInit failed";

	/* --------------------------------------------------------- */
	/* parse expected LUID order                                 */

	for (int i = 1; i < argc; i++) {
		luid_order.push_back(strtoull(argv[i], NULL, 16));
	}

	/* --------------------------------------------------------- */
	/* obtain adapter compatibility information                  */

	hr = CreateDXGIFactory1(__uuidof(IDXGIFactory), (void **)&factory);
	if (FAILED(hr))
		throw "CreateDXGIFactory1 failed";

	uint32_t idx = 0;
	while (get_adapter_caps(factory, idx++))
		;

	for (auto &[idx, caps] : adapter_info) {
		printf("[%u]\n", idx);
		printf("is_amd=%s\n", caps.is_amd ? "true" : "false");
		printf("supports_avc=%s\n", caps.supports_avc ? "true" : "false");
		printf("supports_hevc=%s\n", caps.supports_hevc ? "true" : "false");
		printf("supports_av1=%s\n", caps.supports_av1 ? "true" : "false");
	}

	return 0;
} catch (const char *text) {
	printf("[error]\nstring=%s\n", text);
	return 0;
}
