#include "libvr/ISuperResolutionAdapter.h"
#include "MLRuntime.h"
#include <iostream>

namespace libvr {

class SuperResolutionAdapter : public ISuperResolutionAdapter {
public:
    SuperResolutionAdapter() {
        runtime.Initialize();
        
        static const ISuperResolutionAdapter_Vtbl vtbl_impl = {
            StaticInitialize,
            StaticProcess,
            StaticShutdown
        };
        this->vtbl = &vtbl_impl;
        this->user_data = this;
    }
    
    ~SuperResolutionAdapter() = default;

    void Initialize(int width, int height) {
        std::cout << "[SRAdapter] Initializing for internal resolution: " << width << "x" << height << std::endl;
        // Pre-allocate things here if needed
    }

    void Process(const GPUFrameView* src, GPUFrameView* dst, const SRConfig* cfg) {
        if (!cfg) return;

        // Lazy load model if it changed
        if (current_model != cfg->model_path && cfg->model_path) {
             std::cout << "[SRAdapter] Loading model: " << cfg->model_path << std::endl;
             if (runtime.LoadModel(cfg->model_path)) {
                 current_model = cfg->model_path;
             }
        }

        // Mock upscale logic check
        if (dst && src) {
             // In real world, we'd check if dst->dims == src->dims * scale
        }
    }
    
    void Shutdown() {
        // cleanup
    }

    // Static Trampolines
    static void StaticInitialize(ISuperResolutionAdapter* self, int width, int height) {
        static_cast<SuperResolutionAdapter*>(self->user_data)->Initialize(width, height);
    }
    static void StaticProcess(ISuperResolutionAdapter* self, const GPUFrameView* src, GPUFrameView* dst, const SRConfig* cfg) {
        static_cast<SuperResolutionAdapter*>(self->user_data)->Process(src, dst, cfg);
    }
    static void StaticShutdown(ISuperResolutionAdapter* self) {
        static_cast<SuperResolutionAdapter*>(self->user_data)->Shutdown();
    }

private:
    MLRuntime runtime;
    std::string current_model;
};


extern "C" ISuperResolutionAdapter* CreateSRAdapter() {
    return new SuperResolutionAdapter();
}

} // namespace libvr
