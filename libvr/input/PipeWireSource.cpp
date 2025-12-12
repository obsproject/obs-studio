#include "libvr/ISourceAdapter.h"
#include "PipeWireProxy.h"
#include <iostream>

namespace libvr {

class PipeWireSource : public ISourceAdapter {
public:
    PipeWireSource() {
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
    
    ~PipeWireSource() {
        Shutdown();
    }

    bool Initialize(const SourceConfig* cfg) {
        if (!LoadPipeWireFunctions()) {
             std::cerr << "[PipeWireSource] Library not available. Falling back to dummy mode?" << std::endl;
             return false;
        }

        // Initialize PipeWire
        wrapper_pw_init(nullptr, nullptr);
        
        std::cout << "[PipeWireSource] Initialized for device: " << (cfg->device_id ? cfg->device_id : "Default") << std::endl;
        
        loop = wrapper_pw_thread_loop_new("video-capture", nullptr);
        if (!loop) return false;

        return true;
    }

    bool Start() {
        if (running) return true;
        
        if (wrapper_pw_thread_loop_start(loop) < 0) {
             return false;
        }
        
        running = true;
        std::cout << "[PipeWireSource] Started capture loop." << std::endl;
        return true;
    }

    bool Stop() {
        if (!running) return true;
        // wrapper_pw_thread_loop_stop(loop);
        running = false;
        return true;
    }

    GPUFrameView AcquireFrame() {
        // Real impl: Check if new buffer arrived in pw_stream_listener
        // For verify: return empty or last frame
        return {}; 
    }

    void ReleaseFrame(GPUFrameView* frame) {
        // cleanup
    }
    
    void Shutdown() {
        Stop();
        // destroy loop
    }

    // Trampolines
    static bool StaticInitialize(ISourceAdapter* self, const SourceConfig* cfg) { return static_cast<PipeWireSource*>(self->user_data)->Initialize(cfg); }
    static bool StaticStart(ISourceAdapter* self) { return static_cast<PipeWireSource*>(self->user_data)->Start(); }
    static bool StaticStop(ISourceAdapter* self) { return static_cast<PipeWireSource*>(self->user_data)->Stop(); }
    static GPUFrameView StaticAcquireFrame(ISourceAdapter* self) { return static_cast<PipeWireSource*>(self->user_data)->AcquireFrame(); }
    static void StaticReleaseFrame(ISourceAdapter* self, GPUFrameView* frame) { static_cast<PipeWireSource*>(self->user_data)->ReleaseFrame(frame); }
    static void StaticShutdown(ISourceAdapter* self) { static_cast<PipeWireSource*>(self->user_data)->Shutdown(); }

private:
    struct pw_thread_loop* loop = nullptr;
    bool running = false;
};

extern "C" ISourceAdapter* CreatePipeWireSource() {
    return new PipeWireSource();
}

} // namespace libvr
