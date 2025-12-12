#include "libvr/IRenderEngine.h"
#include "gpu/vulkan_utils.h"
#include <iostream>

using namespace libvr;

// namespace libvr {
//    extern IRenderEngine* CreateVulkanRenderEngine();
// }

int main()
{
	std::cout << "[TestInterop] Creating Engine..." << std::endl;
	auto engine = libvr::CreateVulkanRenderEngine();

	RenderConfig config = {};
	config.width = 1920;
	config.height = 1080;
	// Disable validation for CI/headless robustness unless needed
	config.enable_validation_layers = false;

	if (!engine->Initialize(config)) {
		std::cerr << "[TestInterop] Failed to init engine." << std::endl;
		return 1;
	}

	std::cout << "[TestInterop] Requesting Frame..." << std::endl;
	// Simulate a frame cycle
	engine->BeginFrame();
	GPUFrameView view = engine->GetOutputFrame();

	if (view.handle == nullptr) {
		std::cerr << "[TestInterop] Frame handle is null." << std::endl;
		return 1;
	}

	VulkanImageHandle *handle = static_cast<VulkanImageHandle *>(view.handle);
	std::cout << "[TestInterop] Got Handle:" << std::endl;
	std::cout << "  Image: " << handle->image << std::endl;
	std::cout << "  FD: " << handle->fd << std::endl;

	if (handle->fd < 0) {
		std::cerr << "[TestInterop] Invalid FD!" << std::endl;
		return 1;
	}

	std::cout << "[TestInterop] Interop Success!" << std::endl;

	engine->Shutdown();
	// delete engine; // Managed by unique_ptr
	return 0;
}
