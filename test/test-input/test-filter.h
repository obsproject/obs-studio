#ifndef FILTER_TEST_H
#define FILTER_TEST_H

#include "obs.h"

#ifdef __cplusplus
extern "C" {
#endif

struct test_filter {
	source_t source;
	effect_t whatever;
	texrender_t texrender;
};

EXPORT struct test_filter *test_create(const char *settings, source_t source);
EXPORT void test_destroy(struct test_filter *rt);
EXPORT uint32_t test_get_output_flags(struct test_filter *rt);

EXPORT void test_video_tick(struct test_filter *rt, float seconds);
EXPORT void test_video_render(struct test_filter *rt);

#ifdef __cplusplus
}
#endif

#endif
