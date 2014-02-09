#pragma once

#include <obs.h>

#ifdef __cplusplus
extern "C" {
#endif

struct msw_desktop_tex {
	texture_t texture;
	uint32_t width, height;
	BYTE *captureBits;
	HDC hdcCompatible;
	HBITMAP hbmpCompatible, hbmpOld;
};

EXPORT const char *msw_desktop_test_getname(const char *locale);

EXPORT struct msw_desktop_tex *msw_desktop_test_create(const char *settings,
		obs_source_t source);
EXPORT void msw_desktop_test_destroy(struct msw_desktop_tex *rt);
EXPORT uint32_t msw_desktop_test_get_output_flags(struct msw_desktop_tex *rt);

EXPORT void msw_desktop_test_video_tick(struct msw_desktop_tex *rt, float seconds);

EXPORT void msw_desktop_test_video_render(struct msw_desktop_tex *rt,
		obs_source_t filter_target);

EXPORT uint32_t msw_desktop_test_getwidth(struct msw_desktop_tex *rt);
EXPORT uint32_t msw_desktop_test_getheight(struct msw_desktop_tex *rt);

#ifdef __cplusplus
}
#endif
