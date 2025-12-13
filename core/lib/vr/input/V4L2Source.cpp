#include "libvr/ISourceAdapter.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <cstring>

namespace libvr {

class V4L2Source : public ISourceAdapter {
public:
    V4L2Source() {
        static const ISourceAdapter_Vtbl vtbl_impl = {
            StaticInitialize,
            StaticStart,
            StaticStop,
            StaticAcquireFrame,
            StaticReleaseFrame,
            StaticShutdown
        };
        this->vtbl = &vtbl_impl;
        this->user_data = this;
    }

    ~V4L2Source() {
        Shutdown();
    }

    bool Initialize(const SourceConfig* cfg) {
        const char* devicePath = cfg->device_id ? cfg->device_id : "/dev/video0";
        fd = open(devicePath, O_RDWR);
        if (fd < 0) {
            std::cerr << "[V4L2] Failed to open device: " << devicePath << std::endl;
            return false;
        }

        struct v4l2_capability cap;
        if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
             std::cerr << "[V4L2] Not a V4L2 device." << std::endl;
             close(fd);
             fd = -1;
             return false;
        }

        if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
             std::cerr << "[V4L2] Device does not support capture." << std::endl;
             close(fd);
             fd = -1;
             return false;
        }

        struct v4l2_format fmt = {};
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.width = cfg->width > 0 ? cfg->width : 1280;
        fmt.fmt.pix.height = cfg->height > 0 ? cfg->height : 720;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV; // Common fallback
        fmt.fmt.pix.field = V4L2_FIELD_NONE;

        if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
             std::cerr << "[V4L2] Failed to set format." << std::endl;
             // Continue? Or fail? Let's fail for now.
             close(fd);
             fd = -1;
             return false;
        }

        std::cout << "[V4L2] Initialized " << devicePath << " (" << fmt.fmt.pix.width << "x" << fmt.fmt.pix.height << ")" << std::endl;
        return true;
    }

    bool Start() {
        if (fd < 0) return false;
        
        // MVP: Just streamon
        int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
             std::cerr << "[V4L2] Failed to start stream." << std::endl;
             return false;
        }
        running = true;
        return true;
    }

    bool Stop() {
        if (!running) return true;
        int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ioctl(fd, VIDIOC_STREAMOFF, &type);
        running = false;
        return true;
    }

    GPUFrameView AcquireFrame() {
        // Real impl: DQBUF -> Convert YUYV-RGB -> Upload
        // MVP: Return nothing
        return {};
    }

    void ReleaseFrame(GPUFrameView* frame) {
        // QBUF back
    }

    void Shutdown() {
        Stop();
        if (fd >= 0) {
            close(fd);
            fd = -1;
        }
    }

    // Trampolines
    static bool StaticInitialize(ISourceAdapter* self, const SourceConfig* cfg) { return static_cast<V4L2Source*>(self->user_data)->Initialize(cfg); }
    static bool StaticStart(ISourceAdapter* self) { return static_cast<V4L2Source*>(self->user_data)->Start(); }
    static bool StaticStop(ISourceAdapter* self) { return static_cast<V4L2Source*>(self->user_data)->Stop(); }
    static GPUFrameView StaticAcquireFrame(ISourceAdapter* self) { return static_cast<V4L2Source*>(self->user_data)->AcquireFrame(); }
    static void StaticReleaseFrame(ISourceAdapter* self, GPUFrameView* frame) { static_cast<V4L2Source*>(self->user_data)->ReleaseFrame(frame); }
    static void StaticShutdown(ISourceAdapter* self) { static_cast<V4L2Source*>(self->user_data)->Shutdown(); }

private:
    int fd = -1;
    bool running = false;
};

extern "C" ISourceAdapter* CreateV4L2Source() {
    return new V4L2Source();
}

} // namespace libvr
