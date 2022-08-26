#include <obs-module.h>

#include "webrtc-call.h"

struct webrtc_source {
	int tmp;
};

static const char *webrtc_source_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("WebRTCSource");
}

static void *webrtc_source_create(obs_data_t *settings, obs_source_t *source)
{
	struct webrtc_source *webrtcSource =
		bzalloc(sizeof(struct webrtc_source));
	return webrtcSource;
}

static void webrtc_source_destroy(void *data)
{
	struct webrtc_source *webrtcSource = data;

	if (webrtcSource) {
		bfree(webrtcSource);
	}
}

static obs_properties_t *webrtc_source_get_properties(void *data)
{
	return obs_properties_create();
}

static void webrtc_source_render(void *data, gs_effect_t *effect)
{

	if (!webrtcTexture)
		return;

	const bool previous = gs_framebuffer_srgb_enabled();
	gs_enable_framebuffer_srgb(true);

	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);

	gs_eparam_t *const param = gs_effect_get_param_by_name(effect, "image");
	gs_effect_set_texture_srgb(param, webrtcTexture);

	gs_draw_sprite(webrtcTexture, 0, 1920, 1080);

	gs_blend_state_pop();

	gs_enable_framebuffer_srgb(previous);
}

uint32_t webrtc_source_get_width(void *data)
{
	return 1920;
}

uint32_t webrtc_source_get_height(void *data)
{
	return 1080;
}

struct obs_source_info webrtc_source_info = {
	.id = "webrtc_source",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_ASYNC_VIDEO | OBS_SOURCE_AUDIO |
			OBS_SOURCE_DO_NOT_DUPLICATE,
	.get_name = webrtc_source_getname,
	.create = webrtc_source_create,
	.destroy = webrtc_source_destroy,
	.get_properties = webrtc_source_get_properties,
	.icon_type = OBS_ICON_TYPE_MEDIA,
	.video_render = webrtc_source_render,
	.get_width = webrtc_source_get_width,
	.get_height = webrtc_source_get_height,
};