#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct EncodedPacket {
    const uint8_t* data;
    size_t size;
    int64_t pts;
    bool is_key;
} EncodedPacket;

struct ITransportAdapter;

typedef struct ITransportAdapter_Vtbl {
    void (*Initialize)(struct ITransportAdapter* self, const char* url, const char* options);
    void (*SendPacket)(struct ITransportAdapter* self, const EncodedPacket* pkt);
    void (*Shutdown)(struct ITransportAdapter* self);
} ITransportAdapter_Vtbl;

typedef struct ITransportAdapter {
    const ITransportAdapter_Vtbl* vtbl;
    void* user_data;
} ITransportAdapter;

#ifdef __cplusplus
}
#endif
