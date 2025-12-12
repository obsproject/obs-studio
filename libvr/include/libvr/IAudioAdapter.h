#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Audio formats
typedef enum AudioFormat {
    AudioFormat_Float32,
    AudioFormat_Int16,
    AudioFormat_Int32
} AudioFormat;

// Audio buffer packet
typedef struct AudioPacket {
    const void* data;
    size_t frames; // Number of samples per channel
    int channels;
    AudioFormat format;
    uint64_t timestamp;
} AudioPacket;

typedef struct AudioSourceConfig {
    const char* backend; // "pipewire", "alsa"
    const char* device_id;
    int sample_rate;
    int channels;
} AudioSourceConfig;

struct IAudioAdapter;

typedef struct IAudioAdapter_Vtbl {
    bool (*Initialize)(struct IAudioAdapter* self, const AudioSourceConfig* cfg);
    bool (*Start)(struct IAudioAdapter* self);
    bool (*Stop)(struct IAudioAdapter* self);
    // Poll for audio data. 
    // real-time audio usually pushes, but for this adapter we might poll or register a callback.
    // For simplicity in this pull-based architecture:
    bool (*ReadPacket)(struct IAudioAdapter* self, AudioPacket* out_packet);
    void (*Shutdown)(struct IAudioAdapter* self);
} IAudioAdapter_Vtbl;

typedef struct IAudioAdapter {
    const IAudioAdapter_Vtbl* vtbl;
    void* user_data;
} IAudioAdapter;

#ifdef __cplusplus
}
#endif
