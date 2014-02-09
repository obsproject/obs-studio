#pragma once

#include <obs.h>

#ifdef __cplusplus
extern "C" {
#endif

// forward declare struct, so we don't have to include X11 headers here
struct xshm_data;
    
EXPORT const char *xshm_input_getname(const char *locale);

EXPORT struct xshm_data *xshm_input_create(const char *settings,
        obs_source_t source);
EXPORT void xshm_input_destroy(struct xshm_data *data);
EXPORT uint32_t xshm_input_get_output_flags(struct xshm_data *data);

EXPORT void xshm_input_video_render(struct xshm_data *data,
        obs_source_t filter_target);

EXPORT uint32_t xshm_input_getwidth(struct xshm_data *data);
EXPORT uint32_t xshm_input_getheight(struct xshm_data *data);

#ifdef __cplusplus
}
#endif