#include "ml/MLRuntime.h"
#include <iostream>

int main() {
    std::cout << "[TestML] Creating ML Runtime..." << std::endl;
    libvr::MLRuntime ml;

    std::cout << "[TestML] Initializing..." << std::endl;
    if (ml.Initialize()) {
        std::cout << "[TestML] Runtime Initialized." << std::endl;
    } else {
        std::cerr << "[TestML] Failed to initialize runtime (Checking for HAS_ONNX)." << std::endl;
        // Check if fallback was compiled
        // If mocked, it might return true or false depending on policy.
        // Assuming we want strict check if ONNX is supposed to be there.
    }
    
    // Future: Load dummy model
    
    return 0;
}
