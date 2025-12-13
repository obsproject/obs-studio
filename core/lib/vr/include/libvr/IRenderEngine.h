#pragma once

#include "frame.h"
#include <vector>
#include <memory>

namespace libvr {

struct RenderConfig {
    uint32_t width;
    uint32_t height;
    bool enable_validation_layers;
    std::vector<std::string> required_extensions;
};

class SceneManager; // Forward declaration

class IRenderEngine {
public:
    virtual ~IRenderEngine() = default;

    virtual bool Initialize(const RenderConfig& config) = 0;
    virtual void Shutdown() = 0;
    
    // Accessors for OpenXR Binding
    virtual void* GetInstance() const = 0; // Returns VkInstance
    virtual void* GetPhysicalDevice() const = 0; // Returns VkPhysicalDevice
    virtual void* GetDevice() const = 0; // Returns VkDevice
    virtual uint32_t GetGraphicsQueueFamily() const = 0;


    // Begin a new frame. Returns false if frame cannot be started.
    virtual bool BeginFrame() = 0;

    // Submit a list of draw commands or scene graph update (simplified for now).
    // In a real engine, this would take a command list or scene description.
    virtual void DrawScene(const SceneManager* scene) = 0;

    // End the frame and present/export.
    // Returns the view of the rendered frame (e.g., for encoding).
    virtual GPUFrameView GetOutputFrame() = 0;

    // Zero-Copy / Swapchain Utility
    // Copies the internal Main Render Target to the destination image (e.g. OpenXR swapchain)
    virtual void BlitToExternal(void* dstHandle, uint32_t w, uint32_t h) = 0;
};

// Factory
std::unique_ptr<IRenderEngine> CreateVulkanRenderEngine();

} // namespace libvr
