#pragma once

#include "obs.h"
#import <CoreGraphics/CGDisplayStream.h>

#include <util/darray.h>


#ifdef __cplusplus
extern "C" {
#endif

struct desktop_tex {
	samplerstate_t sampler;
	effect_t whatever;
	CGDisplayStreamRef disp;
	uint32_t width, height;
};

EXPORT const char *osx_desktop_test_getname(const char *locale);

EXPORT struct desktop_tex *osx_desktop_test_create(const char *settings,
		obs_source_t source);
EXPORT void osx_desktop_test_destroy(struct desktop_tex *rt);
EXPORT uint32_t osx_desktop_test_get_output_flags(struct desktop_tex *rt);
EXPORT void osx_desktop_test_video_tick(struct desktop_tex *rt, float dt);

EXPORT void osx_desktop_test_video_render(struct desktop_tex *rt,
		obs_source_t filter_target);

EXPORT uint32_t osx_desktop_test_getwidth(struct desktop_tex *rt);
EXPORT uint32_t osx_desktop_test_getheight(struct desktop_tex *rt);

#ifdef __cplusplus
}
#endif
