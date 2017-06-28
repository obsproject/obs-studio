#pragma once

#include "../c99defs.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mac_version_info {
	union {
		struct {
			uint8_t _;
			uint8_t bug_fix;
			uint8_t minor;
			uint8_t major;
		};
		uint32_t identifier;
	};
};
	
#define MACOSX_LEOPARD       ((10 << 24) | ( 5 << 16))
#define MACOSX_SNOW_LEOPARD  ((10 << 24) | ( 6 << 16))
#define MACOSX_LION          ((10 << 24) | ( 7 << 16))
#define OSX_MOUNTAIN_LION    ((10 << 24) | ( 8 << 16))
#define OSX_MAVERICKS        ((10 << 24) | ( 9 << 16))
#define OSX_YOSEMITE         ((10 << 24) | (10 << 16))
#define OSX_EL_CAPITAN       ((10 << 24) | (11 << 16))
#define MACOS_SIERRA         ((10 << 24) | (12 << 16))
#define MACOS_HIGH_SIERRA    ((10 << 24) | (13 << 16))

EXPORT void get_mac_ver(struct mac_version_info *info);

#ifdef __cplusplus
}
#endif
