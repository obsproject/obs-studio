#include <AMF/core/Factory.h>
#include <AMF/core/Trace.h>
#include <AMF/components/VideoEncoderVCE.h>
#include <AMF/components/VideoEncoderHEVC.h>
#include <AMF/components/VideoEncoderAV1.h>

#if !defined(_WIN32)
#include <dlfcn.h>
#include <unistd.h>
#endif

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <chrono>
#include <future>
#include <cstring>

#define AMFOBS_ENCODER "amf_encoder"
#define AMFOBS_ENCODER_NAME "amf_vulkan_impl"

#include "../amf_gpu.hpp"

using namespace std;
using namespace std::chrono_literals;
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

static inline uint32_t get_adapter_idx(uint32_t adapter_idx, uint64_t luid)
{
	for (size_t i = 0; i < luid_order.size(); i++) {
		if (luid_order[i] == luid) {
			return (uint32_t)i;
		}
	}
	return adapter_idx;
}

static bool get_adapter_caps(uint32_t adapter_idx)
{
	AMF_RESULT res;

	AMFContextPtr amf_context;
	res = amf_factory->CreateContext(&amf_context);

	std::unique_ptr<amf_gpu> amf_gpu_obj = std::unique_ptr<amf_gpu>(amf_gpu::create());
	if (amf_gpu_obj == nullptr)
		throw "amf_gpu::create failed";

	res = amf_gpu_obj->init(amf_context, adapter_idx);
	if (res == AMF_NOT_SUPPORTED)
		return true; // device created but AMF cannot use it

	if (res != AMF_OK)
		return false;

	uint32_t vendor = 0;
	res = amf_gpu_obj->get_vendor(&vendor);
	if (res != AMF_OK)
		return false;

	if (vendor != AMD_VENDOR_ID)
		return true;

	uint64_t luid = 0;
	res = amf_gpu_obj->get_luid(&luid);
	if (res != AMF_OK)
		return false;

	uint32_t luid_idx = get_adapter_idx(adapter_idx, luid);
	adapter_caps &caps = adapter_info[luid_idx];

	caps.is_amd = true;

	caps.supports_avc = has_encoder(amf_context, AMFVideoEncoderVCE_AVC);
	caps.supports_hevc = has_encoder(amf_context, AMFVideoEncoder_HEVC);
	caps.supports_av1 = has_encoder(amf_context, AMFVideoEncoder_AV1);

	// cleanup in this order
	amf_context = nullptr;
	amf_gpu_obj = nullptr;

	return true;
}

int check_thread()
{
	try {
		amf_handle amf_module;
		AMF_RESULT res;

#if defined(_WIN32)

		amf_module = LoadLibraryW(AMF_DLL_NAME);
		if (!amf_module)
			throw "Failed to load AMF lib";
		auto init = (AMFInit_Fn)GetProcAddress((HMODULE)amf_module, AMF_INIT_FUNCTION_NAME);
#else
		amf_module = dlopen(AMF_DLL_NAMEA, RTLD_NOW | RTLD_GLOBAL);
		if (!amf_module)
			throw "Failed to load AMF lib";
		auto init = (AMFInit_Fn)dlsym(amf_module, AMF_INIT_FUNCTION_NAME);
#endif
		if (!init)
			throw "Failed to get init func";

		res = init(AMF_FULL_VERSION, &amf_factory);
		if (res != AMF_OK)
			throw "AMFInit failed";

		uint32_t idx = 0;
		while (get_adapter_caps(idx++))
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
}
int main(int argc, char **argv)
{
	/* --------------------------------------------------------- */
	/* parse expected LUID order                                 */
	for (int i = 1; i < argc; i++) {
		luid_order.push_back(strtoull(argv[i], NULL, 16));
	}

	future<int> f = async(launch::async, check_thread);
	future_status status = f.wait_for(2.5s);

	if (status == future_status::timeout)
		exit(1);
	return f.get();
}
