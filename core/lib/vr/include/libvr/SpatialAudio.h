#pragma once

#include "libvr/IAudioAdapter.h"

// Extension interface for 3D Audio
typedef struct ISpatialAudioAdapter_Vtbl {
    // Base Vtbl pointer logic would be needed for true polymorphism in C
    // Or we just add methods to IAudioAdapter if we control it.
    // Let's assume we modify IAudioAdapter directly for this pivot.
} ISpatialAudioAdapter_Vtbl;

#ifdef __cplusplus
extern "C" {
#endif

// Add positioning to IAudioAdapter
// In a real C-ABI, we'd either version the struct or have extension functions.
// For this MVP, we will declare a helper function that casts.

void AudioAdapter_SetPosition(struct IAudioAdapter* self, float x, float y, float z);

#ifdef __cplusplus
}
#endif
