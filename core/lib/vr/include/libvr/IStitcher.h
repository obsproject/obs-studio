#pragma once

#include "libvr/frame.h"

namespace libvr {

struct StitchConfig {
    const char* stmap_path; // Path to STMap file
    bool enable;
};

// C-compatible interface for Stitching
struct IStitcher;

typedef struct IStitcher_Vtbl {
    void (*Initialize)(IStitcher* self, int width, int height);
    void (*Process)(IStitcher* self, const GPUFrameView* src, GPUFrameView* dst, const StitchConfig* cfg);
    void (*Shutdown)(IStitcher* self);
} IStitcher_Vtbl;

struct IStitcher {
    const IStitcher_Vtbl* vtbl;
    void* user_data;
};

// Factory
extern "C" IStitcher* CreateStitcher();

} // namespace libvr
