#include <obs-module.h>

#define S_COLOR "color"
#define S_SWITCH_POINT "switch_point"

#define S_COLOR_TEXT obs_module_text("Color")
#define S_SWITCH_POINT_TEXT obs_module_text("SwitchPoint")

struct fade2color_info {
	obs_source_t *source;

	gs_effect_t *effect;
	gs_eparam_t *ep_tex;
	gs_eparam_t *ep_swp;
	gs_eparam_t *ep_color;

	struct vec4 color;
	float switch_point;
};

static inline float lerp(float a, float b, float x)
{
	return (1.0f - x) * a + x * b;
}

static inline float clamp(float x, float min, float max)
{
	if (x < min)
		return min;
	else if (x > max)
		return max;
	return x;
}

static inline float smoothstep(float min, float max, float x)
{
	x = clamp((x - min) / (max - min), 0.0f, 1.0f);
	return x*x*(3 - 2 * x);
}

static const char *fade2color_get_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return obs_module_text("Fade2ColorTransition");
}

static void fade2color_update(void *data, obs_data_t *settings)
{
	struct fade2color_info *fade2color = data;
	uint32_t color = (uint32_t)obs_data_get_int(settings, S_COLOR);
	uint32_t swp   = (uint32_t)obs_data_get_int(settings, S_SWITCH_POINT);

	color |= 0xFF000000;
	
	vec4_from_rgba(&fade2color->color, color);

	fade2color->switch_point = (float)swp / 100.0f;
}

static void *fade2color_create(obs_data_t *settings, obs_source_t *source)
{
	struct fade2color_info *fade2color;
	char *file = obs_module_file("fade2color_transition.effect");
	gs_effect_t *effect;

	obs_enter_graphics();
	effect = gs_effect_create_from_file(file, NULL);
	obs_leave_graphics();

	bfree(file);

	if (!effect) {
		blog(LOG_ERROR, "Could not find fade2color_transition.effect");
		return NULL;
	}

	fade2color = bzalloc(sizeof(struct fade2color_info));

	fade2color->source = source;
	fade2color->effect = effect;

	fade2color->ep_tex = gs_effect_get_param_by_name(effect, "ep_tex");

	fade2color->ep_swp   = gs_effect_get_param_by_name(effect, "ep_swp");
	fade2color->ep_color = gs_effect_get_param_by_name(effect, "ep_color");

	obs_source_update(source, settings);

	return fade2color;
}

static void fade2color_destroy(void *data)
{
	struct fade2color_info *fade2color = data;
	bfree(fade2color);
}

static void fade2color_callback(void *data, gs_texture_t *a, gs_texture_t *b,
	float t, uint32_t cx, uint32_t cy)
{
	struct fade2color_info *fade2color = data;

	float sa = smoothstep(0.0f, fade2color->switch_point, t);
	float sb = smoothstep(fade2color->switch_point, 1.0f, t);

	float swp = t < fade2color->switch_point ? sa : 1.0f - sb;

	gs_effect_set_texture(fade2color->ep_tex,
			      t < fade2color->switch_point ? a : b);
	gs_effect_set_float(fade2color->ep_swp, swp);
	gs_effect_set_vec4(fade2color->ep_color, &fade2color->color);
	
	while (gs_effect_loop(fade2color->effect, "FadeToColor"))
		gs_draw_sprite(NULL, 0, cx, cy);
}

static void fade2color_video_render(void *data, gs_effect_t *effect)
{
	struct fade2color_info *fade2color = data;
	obs_transition_video_render(fade2color->source, fade2color_callback);
	UNUSED_PARAMETER(effect);
}

static float mix_a(void *data, float t)
{
	struct fade2color_info *fade2color = data;
	float sp = fade2color->switch_point;

	return lerp(1.0f - t , 0.0f, smoothstep(0.0f, sp, t));
}

static float mix_b(void *data, float t)
{
	struct fade2color_info *fade2color = data;
	float sp = fade2color->switch_point;

	return lerp(0.0f, t, smoothstep(sp, 1.0f, t));
}

static bool fade2color_audio_render(void *data, uint64_t *ts_out,
		struct obs_source_audio_mix *audio, uint32_t mixers,
		size_t channels, size_t sample_rate)
{
	struct fade2color_info *fade2color = data;
	return obs_transition_audio_render(fade2color->source, ts_out,
		audio, mixers, channels, sample_rate, mix_a, mix_b);
}

static obs_properties_t *fade2color_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	obs_properties_add_color(props, S_COLOR, S_COLOR_TEXT);
	obs_properties_add_int_slider(props, S_SWITCH_POINT,
		S_SWITCH_POINT_TEXT, 0, 100, 1);
	
	UNUSED_PARAMETER(data);
	return props;
}

static void fade2color_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, S_COLOR, 0xFF000000);
	obs_data_set_default_int(settings, S_SWITCH_POINT, 50);
}

struct obs_source_info fade2color_transition = {
	.id             = "fade2color_transition",
	.type           = OBS_SOURCE_TYPE_TRANSITION,
	.get_name       = fade2color_get_name,
	.create         = fade2color_create,
	.destroy        = fade2color_destroy,
	.update         = fade2color_update,
	.video_render   = fade2color_video_render,
	.audio_render   = fade2color_audio_render,
	.get_properties = fade2color_properties,
	.get_defaults   = fade2color_defaults
};
