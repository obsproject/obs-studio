#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <obs-module.h>

#ifdef __cplusplus
extern "C" {
#endif

EXPORT bool winrt_capture_supported();
EXPORT struct winrt_capture *winrt_capture_init(bool cursor, HWND window);
EXPORT void winrt_capture_free(struct winrt_capture *capture);

EXPORT void winrt_capture_render(struct winrt_capture *capture,
				 gs_effect_t *effect);
EXPORT int32_t winrt_capture_width(const struct winrt_capture *capture);
EXPORT int32_t winrt_capture_height(const struct winrt_capture *capture);

#ifdef __cplusplus
}
#endif
