#pragma once

#include "frame.h"
#include <vector>

namespace libvr {

struct RenderConfig {
    uint32_t width;
    uint32_t height;
    bool enable_validation_layers;
};

class IRenderEngine {
public:
    virtual ~IRenderEngine() = default;

    virtual bool Initialize(const RenderConfig& config) = 0;
    virtual void Shutdown() = 0;

    // Begin a new frame. Returns false if frame cannot be started.
    virtual bool BeginFrame() = 0;

    // Submit a list of draw commands or scene graph update (simplified for now).
    // In a real engine, this would take a command list or scene description.
    virtual void DrawScene() = 0;

    // End the frame and present/export.
    // Returns the view of the rendered frame (e.g., for encoding).
    virtual GPUFrameView GetOutputFrame() = 0;
};

} // namespace libvr
