#pragma once

#include "common/frame.h" mon / frame.h "

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct EncoderConfig {
        const char *codec;
        int width;
        int height;
        int fps_num;
        int fps_den;
        int bitrate_kbps;
        bool hardware_acceleration;
    } EncoderConfig;

    struct IEncoderAdapter;

    typedef struct IEncoderAdapter_Vtbl {
        void (*Initialize)(struct IEncoderAdapter *self, const EncoderConfig *cfg);
        bool (*EncodeFrame)(struct IEncoderAdapter *self, const GPUFrameView *frame);
        void (*Flush)(struct IEncoderAdapter *self);
        void (*Shutdown)(struct IEncoderAdapter *self);
    } IEncoderAdapter_Vtbl;

    typedef struct IEncoderAdapter {
        const IEncoderAdapter_Vtbl *vtbl;
        void *user_data;
    } IEncoderAdapter;

#ifdef __cplusplus
}
#endif
