#include "libvr/MediaSource.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    std::cout << "[Test] Initializing MediaSource..." << std::endl;
    libvr::MediaSource src;
    
    std::cout << "[Test] Attempting to load missing file..." << std::endl;
    // This should fail gracefully, logging errors from mp_media
    bool success = src.Load("non_existent_video.mp4");
    
    if (!success) {
        std::cout << "[Test] Load returned false as expected for missing file." << std::endl;
    } else {
        std::cout << "[Test] Load SUCCEEDED unexpectedly?" << std::endl;
    }

    // Try to get frame (should fail)
    GPUFrameView frame;
    if (src.GetFrame(frame)) {
        std::cout << "[Test] Got frame from nothing?" << std::endl;
    } else {
        std::cout << "[Test] GetFrame returned false as expected." << std::endl;
    }

    std::cout << "[Test] Unloading..." << std::endl;
    src.Unload();
    
    std::cout << "[Test] Done." << std::endl;
    return 0;
}
