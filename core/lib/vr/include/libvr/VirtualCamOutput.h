#pragma once

#include "libvr/frame.h"
#include <string>
#include <vector>

namespace libvr {

struct VirtualCamConfig {
    std::string devicePath = "/dev/video20";
    int width = 1920;
    int height = 1080;
    int fps = 30;
};

class VirtualCamOutput {
public:
    VirtualCamOutput();
    ~VirtualCamOutput();

    bool Initialize(const VirtualCamConfig& config);
    void Shutdown();

    // Sends a raw RGBA frame to the virtual camera
    // Note: Performs CPU conversion to YUYV internally.
    void SendFrame(const GPUFrameView& frame);

private:
    int fd = -1;
    VirtualCamConfig config;
    std::vector<uint8_t> yuyv_buffer;
};

} // namespace libvr
