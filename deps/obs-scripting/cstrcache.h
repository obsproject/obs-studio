#pragma once

/* simple constant string cache table using STL unordered_map as storage */

#ifdef __cplusplus
extern "C" {
#endif

extern const char *cstrcache_get(const char *str);

#ifdef __cplusplus
}
#endif
