#include <AMF/core/Factory.h>
#include <AMF/core/Trace.h>
#include <AMF/components/VideoEncoderVCE.h>
#include <AMF/components/VideoEncoderHEVC.h>
#include <AMF/components/VideoEncoderAV1.h>

#include <dlfcn.h>
#include <vulkan/vulkan.hpp>

#include <string>
#include <map>

using namespace amf;

struct adapter_caps {
	bool is_amd = false;
	bool supports_avc = false;
	bool supports_hevc = false;
	bool supports_av1 = false;
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

static bool get_adapter_caps(uint32_t adapter_idx)
{
	if (adapter_idx)
		return false;

	adapter_caps &caps = adapter_info[adapter_idx];

	AMF_RESULT res;
	AMFContextPtr amf_context;
	res = amf_factory->CreateContext(&amf_context);
	if (res != AMF_OK)
		return true;

	AMFContext1 *context1 = NULL;
	res = amf_context->QueryInterface(AMFContext1::IID(),
					  (void **)&context1);
	if (res != AMF_OK)
		return false;
	res = context1->InitVulkan(nullptr);
	context1->Release();
	if (res != AMF_OK)
		return false;

	caps.is_amd = true;
	caps.supports_avc = has_encoder(amf_context, AMFVideoEncoderVCE_AVC);
	caps.supports_hevc = has_encoder(amf_context, AMFVideoEncoder_HEVC);
	caps.supports_av1 = has_encoder(amf_context, AMFVideoEncoder_AV1);

	return true;
}

int main(void)
try {
	AMF_RESULT res;
	VkResult vkres;

	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "obs-amf-test";
	app_info.apiVersion = VK_API_VERSION_1_2;

	VkInstanceCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	info.pApplicationInfo = &app_info;

	VkInstance instance;
	vkres = vkCreateInstance(&info, nullptr, &instance);
	if (vkres != VK_SUCCESS)
		throw "Failed to initialize Vulkan";

	uint32_t device_count;
	vkres = vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
	if (vkres != VK_SUCCESS || !device_count)
		throw "Failed to enumerate Vulkan devices";

	VkPhysicalDevice *devices = new VkPhysicalDevice[device_count];
	vkres = vkEnumeratePhysicalDevices(instance, &device_count, devices);
	if (vkres != VK_SUCCESS)
		throw "Failed to enumerate Vulkan devices";

	VkPhysicalDeviceDriverProperties driver_props = {};
	driver_props.sType =
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;
	VkPhysicalDeviceProperties2 device_props = {};
	device_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	device_props.pNext = &driver_props;
	vkGetPhysicalDeviceProperties2(devices[0], &device_props);

	if (strcmp(driver_props.driverName, "AMD proprietary driver"))
		throw "Not running AMD proprietary driver";

	vkDestroyInstance(instance, nullptr);

	/* --------------------------------------------------------- */
	/* try initializing amf, I guess                             */

	void *amf_module = dlopen(AMF_DLL_NAMEA, RTLD_LAZY);
	if (!amf_module)
		throw "Failed to load AMF lib";

	auto init = (AMFInit_Fn)dlsym(amf_module, AMF_INIT_FUNCTION_NAME);
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
		printf("supports_avc=%s\n",
		       caps.supports_avc ? "true" : "false");
		printf("supports_hevc=%s\n",
		       caps.supports_hevc ? "true" : "false");
		printf("supports_av1=%s\n",
		       caps.supports_av1 ? "true" : "false");
	}

	return 0;
} catch (const char *text) {
	printf("[error]\nstring=%s\n", text);
	return 0;
}
