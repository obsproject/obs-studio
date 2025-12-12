#pragma once

#include "libvr/frame.h"
#include <vulkan/vulkan.h>

namespace libvr {

// Helper to fill GPUFrameView from a known VkImage
inline void FillGPUFrameView(GPUFrameView& view, VkImage image, uint32_t width, uint32_t height, VkFormat format) {
    view.handle = (void*)image;
    view.width = width;
    view.height = height;
    view.stride = 0; // Linear stride undefined for optimal tiling images
    view.size = 0;   // Opaque
    
    // Basic color space guess based on format
    view.color.space = Colorspace_Rec709;
    view.color.is_hdr = false;
    view.color.max_cll = 0.0f;
    view.color.masters_max_luma = 0.0f;
    
    // TODO: Map VkFormat to more detailed color info
    if (format == VK_FORMAT_A2B10G10R10_UNORM_PACK32 || format == VK_FORMAT_R16G16B16A16_SFLOAT) {
        view.color.is_hdr = true; // Primitive heuristic
    }
}

} // namespace libvr
