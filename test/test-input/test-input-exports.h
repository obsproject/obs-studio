#ifndef OBS_CAPTURE_EXPORTS_H
#define OBS_CAPTURE_EXPORTS_H

#include "util/c99defs.h"

#ifdef __cplusplus
extern "C" {
#endif

EXPORT bool enum_inputs(size_t idx, const char **name);
EXPORT bool enum_filters(size_t idx, const char **name);

#ifdef __cplusplus
}
#endif

#endif
