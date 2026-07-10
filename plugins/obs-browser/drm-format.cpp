#include "drm-format.hpp"

#include <util/util.hpp>

#ifdef ENABLE_BROWSER_SHARED_TEXTURE

static const struct obs_cef_video_format supported_formats[] = {
	{
		CEF_COLOR_TYPE_RGBA_8888,
		DRM_FORMAT_ABGR8888,
		GS_RGBA_UNORM,
		"RGBA_8888",
	},
	{
		CEF_COLOR_TYPE_BGRA_8888,
		DRM_FORMAT_ARGB8888,
		GS_BGRA_UNORM,
		"BGRA_8888",
	},
};

constexpr size_t N_SUPPORTED_FORMATS = sizeof(supported_formats) / sizeof(supported_formats[0]);

bool obs_cef_all_drm_formats_supported(void)
{
	size_t n_supported = 0;
	size_t n_formats = 0;
	enum gs_dmabuf_flags dmabuf_flags;
	BPtr<uint32_t> drm_formats;

	if (!gs_query_dmabuf_capabilities(&dmabuf_flags, &drm_formats, &n_formats))
		return false;

	for (size_t i = 0; i < n_formats; i++) {
		for (size_t j = 0; j < N_SUPPORTED_FORMATS; j++) {
			if (drm_formats[i] != supported_formats[j].drm_format)
				continue;

			blog(LOG_DEBUG, "[obs-browser]: CEF color type %s supported", supported_formats[j].pretty_name);
			n_supported++;
		}
	}

	return n_supported == N_SUPPORTED_FORMATS;
}

struct obs_cef_video_format obs_cef_format_from_cef_type(cef_color_type_t cef_type)
{
	for (size_t i = 0; i < N_SUPPORTED_FORMATS; i++) {
		if (supported_formats[i].cef_type == cef_type)
			return supported_formats[i];
	}

	blog(LOG_ERROR, "[obs-browser]: Unsupported CEF color format (%d)", cef_type);

	return {cef_type, DRM_FORMAT_INVALID, GS_UNKNOWN, NULL};
}

#endif
