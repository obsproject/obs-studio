#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ColorSpace {
    Colorspace_Rec709,
    Colorspace_Rec2020,
    Colorspace_SRGB
} ColorSpace;

typedef struct ColorInfo {
    ColorSpace space;
    bool is_hdr;
    float masters_max_luma;
    float max_cll;
} ColorInfo;

typedef struct GPUFrameView {
    void* handle;          // Generic handle (e.g., VkImage or CUdeviceptr)
    uint64_t size;         // Size in bytes (if applicable/linear)
    uint32_t width;
    uint32_t height;
    uint32_t stride;       // Stride in bytes
    uint64_t timestamp;    // Nanoseconds
    ColorInfo color;
    void* user_data;       // Backing object (context)
    int fd;                // Opaque FD for interop (Linux) or Handle (Win32)
} GPUFrameView;

#ifdef __cplusplus
}
#endif
