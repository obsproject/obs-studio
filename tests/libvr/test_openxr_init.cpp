#include "libvr/OpenXRRuntime.h"
#include <iostream>

int main() {
    std::cout << "[TestOpenXR] Creating Runtime..." << std::endl;
    libvr::OpenXRRuntime runtime;
    libvr::OpenXRRuntime::Config config;
    config.app_name = "TestApp";
    config.enable_debug = true;

    if (runtime.Initialize(config)) {
        std::cout << "[TestOpenXR] Initialization Passed!" << std::endl;
        runtime.Shutdown();
        return 0;
    } else {
        std::cerr << "[TestOpenXR] Initialization Failed! (Is a VR runtime like Monado/SteamVR active?)" << std::endl;
        return 1; // Return failure but this might fail in CI if no runtime. 
        // For scaffold verification, we want to know if it compiles mainly.
    }
}
