#pragma once

#include "obs.h"

#ifdef __cplusplus
extern "C" {
#endif

struct test_filter {
	obs_source_t source;
	effect_t whatever;
	texrender_t texrender;
};

EXPORT const char *test_getname(const char *locale);

EXPORT struct test_filter *test_create(const char *settings, obs_source_t source);
EXPORT void test_destroy(struct test_filter *rt);
EXPORT uint32_t test_get_output_flags(struct test_filter *rt);

EXPORT void test_video_tick(struct test_filter *rt, float seconds);
EXPORT void test_video_render(struct test_filter *rt);

#ifdef __cplusplus
}
#endif
