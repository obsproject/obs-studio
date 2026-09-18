#pragma once

#include "../c99defs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef bool (*device_luid_cb)(void *param, uint32_t idx, uint64_t luid);
EXPORT void enum_graphics_device_luids(device_luid_cb device_luid, void *param);

struct IMMDevice;
EXPORT char *get_audio_device_id(struct IMMDevice *device);
EXPORT char *get_audio_device_id_from_id(const char *id);

#ifdef __cplusplus
}
#endif
