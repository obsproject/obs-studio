#pragma once

#include "frame.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Configuration for a video source
typedef struct SourceConfig {
    const char* source_type; // e.g., "pipewire", "v4l2", "test"
    const char* device_id;   // path or node id
    int width;
    int height;
    int fps;
} SourceConfig;

struct ISourceAdapter;

typedef struct ISourceAdapter_Vtbl {
    bool (*Initialize)(struct ISourceAdapter* self, const SourceConfig* cfg);
    bool (*Start)(struct ISourceAdapter* self);
    bool (*Stop)(struct ISourceAdapter* self);
    // Acquire latest frame. Returns empty/null frame if no new frame.
    // The implementation owns the frame memory (or it's an exported handle).
    GPUFrameView (*AcquireFrame)(struct ISourceAdapter* self);
    // Release frame if necessary (unlocking, unrefing)
    void (*ReleaseFrame)(struct ISourceAdapter* self, GPUFrameView* frame);
    void (*Shutdown)(struct ISourceAdapter* self);
} ISourceAdapter_Vtbl;

typedef struct ISourceAdapter {
    const ISourceAdapter_Vtbl* vtbl;
    void* user_data;
} ISourceAdapter;

#ifdef __cplusplus
}
#endif
