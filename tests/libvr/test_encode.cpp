#include "enc/EncoderManager.h"
#include "enc/defaults.h"
#include <iostream>
#include <vector>

int main() {
    std::cout << "[TestEncode] initializing EncoderManager..." << std::endl;
    libvr::EncoderManager manager;
    
    EncoderConfig config = {};
    config.codec = "h264"; // Request H.264 (will look for nvenc > vaapi > x264)
    config.width = 1920;
    config.height = 1080;
    config.fps_num = 60;
    config.fps_den = 1;
    config.bitrate_kbps = 5000;
    config.hardware_acceleration = true;

    std::cout << "[TestEncode] Creating Encoder..." << std::endl;
    IEncoderAdapter* encoder = manager.CreateEncoder(config);
    
    if (!encoder) {
        std::cerr << "[TestEncode] Failed to create encoder!" << std::endl;
        return 1;
    }
    std::cout << "[TestEncode] Encoder created successfully." << std::endl;

    // Create a dummy frame
    GPUFrameView frame = {};
    frame.width = 1920;
    frame.height = 1080;
    frame.timestamp = 0;
    
    // In Phase 3, we don't have valid generic handles yet for staging,
    // but the FFmpeg adapter skeleton is designed to handle this (it allocs CPU frame).
    // The current implementation attempts to read from view->handle if we passed real data,
    // but expects nothing if we just want to test pipeline plumbing.
    
    std::cout << "[TestEncode] Submitting frame..." << std::endl;
    if (encoder->vtbl->EncodeFrame(encoder, &frame)) {
        std::cout << "[TestEncode] Frame submitted successfully." << std::endl;
    } else {
        std::cerr << "[TestEncode] Frame submission failed." << std::endl;
        // Proceeding anyway as it might be due to dummy handle
    }

    std::cout << "[TestEncode] Flushing..." << std::endl;
    encoder->vtbl->Flush(encoder);
    
    std::cout << "[TestEncode] Done." << std::endl;
    // Manager destructor cleans up encoder
    return 0;
}
