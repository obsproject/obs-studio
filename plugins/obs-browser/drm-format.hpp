#pragma once

#include <graphics/graphics.h>
#include <libdrm/drm_fourcc.h>

#include "cef-headers.hpp"

#ifdef ENABLE_BROWSER_SHARED_TEXTURE

struct obs_cef_video_format {
	cef_color_type_t cef_type;
	uint32_t drm_format;
	enum gs_color_format gs_format;
	const char *pretty_name;
};

bool obs_cef_all_drm_formats_supported(void);

struct obs_cef_video_format obs_cef_format_from_cef_type(cef_color_type_t cef_type);

#endif
