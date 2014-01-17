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

EXPORT const char *desktop_getname(const char *locale);

EXPORT struct desktop_tex *desktop_create(const char *settings, obs_source_t source);
EXPORT void desktop_destroy(struct desktop_tex *rt);
EXPORT uint32_t desktop_get_output_flags(struct desktop_tex *rt);
EXPORT void desktop_video_tick(struct desktop_tex *rt, float dt);

EXPORT void desktop_video_render(struct desktop_tex *rt, obs_source_t filter_target);

EXPORT uint32_t desktop_getwidth(struct desktop_tex *rt);
EXPORT uint32_t desktop_getheight(struct desktop_tex *rt);

#ifdef __cplusplus
}
#endif
