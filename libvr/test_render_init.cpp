#include "libvr/IRenderEngine.h"
#include <iostream>

namespace libvr {
    extern IRenderEngine* CreateVulkanRenderEngine();
}

int main() {
    std::cout << "[Test] Creating RenderEngine Instance..." << std::endl;
    libvr::IRenderEngine* engine = libvr::CreateVulkanRenderEngine();
    
    if (!engine) {
        std::cerr << "[Test] Failed to create engine instance." << std::endl;
        return 1;
    }

    libvr::RenderConfig config;
    config.width = 1920;
    config.height = 1080;
    config.enable_validation_layers = false; // Validation layers missing on host?

    std::cout << "[Test] Initializing Engine..." << std::endl;
    if (!engine->Initialize(config)) {
        std::cerr << "[Test] Initialization failed!" << std::endl;
        delete engine;
        return 1;
    }

    std::cout << "[Test] Engine initialized successfully!" << std::endl;

    engine->Shutdown();
    delete engine;
    
    std::cout << "[Test] Shutdown successful." << std::endl;
    return 0;
}
