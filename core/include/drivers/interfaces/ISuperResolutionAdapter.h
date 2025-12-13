#pragma once

#include "common/frame.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct SRConfig {
        float sharpness;
        int quality_level;
        const char *model_path;  // New field
        float scale_factor;      // New field
    } SRConfig;

    struct ISuperResolutionAdapter;

    typedef struct ISuperResolutionAdapter_Vtbl {
        void (*Initialize)(struct ISuperResolutionAdapter *self, int width, int height);
        void (*Process)(struct ISuperResolutionAdapter *self, const GPUFrameView *src, GPUFrameView *dst,
                        const SRConfig *cfg);
        void (*Shutdown)(struct ISuperResolutionAdapter *self);
    } ISuperResolutionAdapter_Vtbl;

    typedef struct ISuperResolutionAdapter {
        const ISuperResolutionAdapter_Vtbl *vtbl;
        void *user_data;
    } ISuperResolutionAdapter;

    ISuperResolutionAdapter *CreateSRAdapter();

#ifdef __cplusplus
}
#endif
