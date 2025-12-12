#include "libvr/IStitcher.h"
#include <iostream>

namespace libvr {

class Stitcher : public IStitcher {
public:
    Stitcher() {
        static const IStitcher_Vtbl vtbl_impl = {
            StaticInitialize,
            StaticProcess,
            StaticShutdown
        };
        this->vtbl = &vtbl_impl;
        this->user_data = this;
    }

    void Initialize(int width, int height) {
        std::cout << "[Stitcher] Initializing for res: " << width << "x" << height << std::endl;
    }

    void Process(const GPUFrameView* src, GPUFrameView* dst, const StitchConfig* cfg) {
        if (cfg && cfg->enable) {
             // Mock Stitching
             // std::cout << "[Stitcher] Processing frame w/ STMap: " << (cfg->stmap_path ? cfg->stmap_path : "None") << std::endl;
        }
    }

    void Shutdown() {
        std::cout << "[Stitcher] Shutdown." << std::endl;
    }

    // Static Trampolines
    static void StaticInitialize(IStitcher* self, int width, int height) {
        static_cast<Stitcher*>(self->user_data)->Initialize(width, height);
    }
    static void StaticProcess(IStitcher* self, const GPUFrameView* src, GPUFrameView* dst, const StitchConfig* cfg) {
        static_cast<Stitcher*>(self->user_data)->Process(src, dst, cfg);
    }
    static void StaticShutdown(IStitcher* self) {
        static_cast<Stitcher*>(self->user_data)->Shutdown();
    }
};

extern "C" IStitcher* CreateStitcher() {
    return new Stitcher();
}

} // namespace libvr
