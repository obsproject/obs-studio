#include "../external/AMF/include/core/Factory.h"
#include "../external/AMF/include/core/Trace.h"
#include "../external/AMF/include/components/VideoEncoderVCE.h"
#include "../external/AMF/include/components/VideoEncoderHEVC.h"

#include <util/windows/ComPtr.hpp>

#include <dxgi.h>
#include <d3d11.h>
#include <d3d11_1.h>

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
};

static AMFFactory *amf_factory = nullptr;
static std::map<uint32_t, adapter_caps> adapter_info;

static bool has_encoder(AMFContextPtr &amf_context, const wchar_t *encoder_name)
{
	AMFComponentPtr encoder;
	AMF_RESULT res = amf_factory->CreateComponent(amf_context, encoder_name,
						      &encoder);
	return res == AMF_OK;
}

static bool get_adapter_caps(IDXGIFactory *factory, uint32_t adapter_idx)
{
	AMF_RESULT res;
	HRESULT hr;

	ComPtr<IDXGIAdapter> adapter;
	hr = factory->EnumAdapters(adapter_idx, &adapter);
	if (FAILED(hr))
		return false;

	adapter_caps &caps = adapter_info[adapter_idx];

	DXGI_ADAPTER_DESC desc;
	adapter->GetDesc(&desc);

	if (desc.VendorId != AMD_VENDOR_ID)
		return true;

	caps.is_amd = true;

	ComPtr<IDXGIOutput> output;
	hr = adapter->EnumOutputs(0, &output);
	if (FAILED(hr))
		return true;

	ComPtr<ID3D11Device> device;
	ComPtr<ID3D11DeviceContext> context;
	hr = D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0,
			       nullptr, 0, D3D11_SDK_VERSION, &device, nullptr,
			       &context);
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

	return true;
}

int main(void)
try {
	ComPtr<IDXGIFactory> factory;
	AMF_RESULT res;
	HRESULT hr;

	/* --------------------------------------------------------- */
	/* try initializing amf, I guess                             */

	HMODULE amf_module = LoadLibraryW(AMF_DLL_NAME);
	if (!amf_module)
		throw "Failed to load AMF lib";

	auto init =
		(AMFInit_Fn)GetProcAddress(amf_module, AMF_INIT_FUNCTION_NAME);
	if (!init)
		throw "Failed to get init func";

	res = init(AMF_FULL_VERSION, &amf_factory);
	if (res != AMF_OK)
		throw "AMFInit failed";

	hr = CreateDXGIFactory1(__uuidof(IDXGIFactory), (void **)&factory);
	if (FAILED(hr))
		throw "CreateDXGIFactory1 failed";

	uint32_t idx = 0;
	while (get_adapter_caps(factory, idx++))
		;

	for (auto &[idx, caps] : adapter_info) {
		printf("[%d]\n", idx);
		printf("is_amd=%s\n", caps.is_amd ? "true" : "false");
		printf("supports_avc=%s\n",
		       caps.supports_avc ? "true" : "false");
		printf("supports_hevc=%s\n",
		       caps.supports_hevc ? "true" : "false");
	}

	return 0;
} catch (const char *text) {
	printf("[error]\nstring=%s\n", text);
	return 0;
}
