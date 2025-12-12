#include "libvr/VirtualCamOutput.h"
#include <iostream>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <cstring>
#include <algorithm>

namespace libvr {

VirtualCamOutput::VirtualCamOutput() {}

VirtualCamOutput::~VirtualCamOutput() {
    Shutdown();
}

bool VirtualCamOutput::Initialize(const VirtualCamConfig& cfg) {
    this->config = cfg;
    
    // Open Device
    fd = open(config.devicePath.c_str(), O_RDWR);
    if (fd < 0) {
        std::cerr << "[VirtualCam] Failed to open " << config.devicePath << ". Is v4l2loopback loaded?" << std::endl;
        return false;
    }

    // Set Format
    struct v4l2_format fmt = {};
    fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    fmt.fmt.pix.width = config.width;
    fmt.fmt.pix.height = config.height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV; // Most compatible
    fmt.fmt.pix.sizeimage = config.width * config.height * 2;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;
    fmt.fmt.pix.colorspace = V4L2_COLORSPACE_SRGB;

    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
        std::cerr << "[VirtualCam] Failed to set format YUYV." << std::endl;
        close(fd);
        fd = -1;
        return false;
    }

    // Allocate buffer for conversion
    yuyv_buffer.resize(fmt.fmt.pix.sizeimage);

    std::cout << "[VirtualCam] Initialized on " << config.devicePath << std::endl;
    return true;
}

void VirtualCamOutput::Shutdown() {
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
}

// Simple RGBA -> YUYV Converter (CPU)
// Optimization: Move to Compute Shader later
void VirtualCamOutput::SendFrame(const GPUFrameView& frame) {
    if (fd < 0) return;
    
    // Check if we have valid CPU data (frame.handle might be GPU pointer or CPU pointer)
    // For this MVP, we assume frame.handle is a CPU-accessible RGBA buffer for testing.
    // In real Vulkan usage, we'd need to map the memory first.
    
    const uint8_t* rgba = (const uint8_t*)frame.handle; 
    if (!rgba) return;

    int w = config.width;
    int h = config.height;
    uint8_t* yuyv = yuyv_buffer.data();

    for (int i = 0; i < w * h; i += 2) {
        // Pixel 1
        int r1 = rgba[i * 4 + 0];
        int g1 = rgba[i * 4 + 1];
        int b1 = rgba[i * 4 + 2];

        // Pixel 2
        int r2 = rgba[(i + 1) * 4 + 0];
        int g2 = rgba[(i + 1) * 4 + 1];
        int b2 = rgba[(i + 1) * 4 + 2];

        // RGB -> YUV (BT.601)
        int y1 = ( ( 66 * r1 + 129 * g1 +  25 * b1 + 128) >> 8) +  16;
        int u1 = ( ( -38 * r1 -  74 * g1 + 112 * b1 + 128) >> 8) + 128;
        int v1 = ( ( 112 * r1 -  94 * g1 -  18 * b1 + 128) >> 8) + 128;

        int y2 = ( ( 66 * r2 + 129 * g2 +  25 * b2 + 128) >> 8) +  16;
        // U/V are shared for 2 pixels in YUYV (4:2:2)

        int u = u1; // Average or subsample (here point sample)
        int v = v1;

        // Pack YUYV
        int base = i * 2;
        yuyv[base + 0] = (uint8_t)std::clamp(y1, 0, 255);
        yuyv[base + 1] = (uint8_t)std::clamp(u, 0, 255);
        yuyv[base + 2] = (uint8_t)std::clamp(y2, 0, 255);
        yuyv[base + 3] = (uint8_t)std::clamp(v, 0, 255);
    }

    // Write to V4L2 device
    write(fd, yuyv_buffer.data(), yuyv_buffer.size());
}

} // namespace libvr
