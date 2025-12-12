#include <iostream>
#include <libvr/OpenXRRuntime.h>
#include <libvr/IRenderEngine.h>
#include <libvr/pipeline/FrameRouter.h>
#include <memory>

// Factory from libvr
int main(int argc, char** argv) {
    std::cout << "[obs-vr] Starting VR Engine..." << std::endl;

    // 1. Initialize OpenXR Runtime
    libvr::OpenXRRuntime xrRuntime;
    libvr::OpenXRRuntime::Config xrConfig;
    xrConfig.app_name = "OBS-VR-Fork";
    xrConfig.enable_debug = true;
    
    if (!xrRuntime.Initialize(xrConfig)) {
        std::cerr << "[obs-vr] Failed to initialize OpenXR. (Is Monado/SteamVR running?)" << std::endl;
        // In production, we might fallback to 2D mode, but for now error out.
        // For development scaffold, we proceed to test rest of logic if config allows, 
        // but RunLoop requires init.
        // return 1; 
    }

    // 2. Query OpenXR Requirements
    std::vector<std::string> requiredExtensions;
    if (!xrRuntime.GetRequiredVulkanExtensions(requiredExtensions)) {
        std::cerr << "[obs-vr] Failed to get OpenXR Vulkan requirements." << std::endl;
        return 1;
    }

    // 3. Initialize Renderer with Requirements
    auto renderer = libvr::CreateVulkanRenderEngine();
    libvr::RenderConfig renderConfig;
    renderConfig.width = 1920; 
    renderConfig.height = 1080;
    renderConfig.enable_validation_layers = true;
    renderConfig.required_extensions = requiredExtensions;
    
    if (!renderer->Initialize(renderConfig)) {
        std::cerr << "[obs-vr] Failed to initialize Vulkan Renderer." << std::endl;
        return 1;
    }

    // 4. Bind OpenXR Session
    if (!xrRuntime.CreateVulkanSession(
        (VkInstance)renderer->GetInstance(),
        (VkPhysicalDevice)renderer->GetPhysicalDevice(),
        (VkDevice)renderer->GetDevice(),
        renderer->GetGraphicsQueueFamily()
    )) {
        std::cerr << "[obs-vr] Failed to bind OpenXR session to Vulkan." << std::endl;
        return 1;
    }

    // 5. Initialize Router
    libvr::FrameRouter router(renderer.get());

    // 6. Run Loop
    std::cout << "[obs-vr] Entering Main Loop..." << std::endl;
    xrRuntime.RunLoop([&]() {
        // This callback runs every frame synchronized with VR
        router.ProcessFrame();
    });

    std::cout << "[obs-vr] Shutdown." << std::endl;
    renderer->Shutdown();
    xrRuntime.Shutdown();
    return 0;
}
