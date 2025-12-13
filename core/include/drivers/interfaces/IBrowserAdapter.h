#pragma once

#include "drivers/interfaces/ISourceAdapter.h"

#ifdef __cplusplus
extern "C" {
#endif

    // Browser-specific configuration
    typedef struct BrowserConfig {
        SourceConfig base;  // Width, Height, FPS
        const char *initial_url;
        bool is_transparent;
    } BrowserConfig;

    struct IBrowserAdapter;

    // Extension of Source Vtbl
    typedef struct IBrowserAdapter_Vtbl {
        // Base Source methods (compatible layout ideally, or simpler: just composition)
        // For C-ABI inheritance, we often duplicate the vtbl start or have a GetSource() method.
        // Let's use Composition/casting for simplicity if we ensure layout.
        // Or just redefined specific browser methods.

        // Lifecycle
        bool (*Initialize)(struct IBrowserAdapter *self, const BrowserConfig *cfg);
        void (*Shutdown)(struct IBrowserAdapter *self);

        // Frame Access (Same as ISource)
        GPUFrameView (*AcquireFrame)(struct IBrowserAdapter *self);
        void (*ReleaseFrame)(struct IBrowserAdapter *self, GPUFrameView *frame);

        // Navigation
        void (*Navigate)(struct IBrowserAdapter *self, const char *url);
        void (*Reload)(struct IBrowserAdapter *self);
        void (*ExecuteJavascript)(struct IBrowserAdapter *self, const char *script);

        // Input
        void (*SendMouseMove)(struct IBrowserAdapter *self, int x, int y);
        void (*SendMouseClick)(struct IBrowserAdapter *self, int x, int y, bool down);
        void (*SendKey)(struct IBrowserAdapter *self, int key_code, bool down);

    } IBrowserAdapter_Vtbl;

    typedef struct IBrowserAdapter {
        const IBrowserAdapter_Vtbl *vtbl;
        void *user_data;
    } IBrowserAdapter;

#ifdef __cplusplus
}
#endif
