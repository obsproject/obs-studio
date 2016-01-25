#include <obs-module.h>

struct slide_info {
	obs_source_t *source;

	gs_effect_t *effect;
	gs_eparam_t *a_param;
	gs_eparam_t *b_param;
	gs_eparam_t *progress_param;
	gs_eparam_t *direction_param;
	struct vec2 direction;
};

static const char *slide_get_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return obs_module_text("SlideTransition");
}

void *slide_create(obs_data_t *settings, obs_source_t *source)
{
	struct slide_info *slide;
	char *file = obs_module_file("slide_transition.effect");
	gs_effect_t *effect;

	obs_enter_graphics();
	effect = gs_effect_create_from_file(file, NULL);
	obs_leave_graphics();
	bfree(file);

	if (!effect) {
		blog(LOG_ERROR, "Could not find slide_transition.effect");
		return NULL;
	}

	slide = bmalloc(sizeof(*slide));
	slide->source = source;
	slide->effect = effect;
	slide->a_param = gs_effect_get_param_by_name(effect, "tex_a");
	slide->b_param = gs_effect_get_param_by_name(effect, "tex_b");
	slide->progress_param = gs_effect_get_param_by_name(effect, "progress");
	slide->direction_param = 
		gs_effect_get_param_by_name(effect, "direction");

	vec2_set(&slide->direction, -1.0f, 0.0f);

	UNUSED_PARAMETER(settings);
	return slide;
}

void slide_destroy(void *data)
{
	struct slide_info *slide = data;
	bfree(slide);
}

static void slide_callback(void *data, gs_texture_t *a, gs_texture_t *b, float t,
		uint32_t cx, uint32_t cy)
{
	struct slide_info *slide = data;

	gs_effect_set_texture(slide->a_param, a);
	gs_effect_set_texture(slide->b_param, b);
	gs_effect_set_float(slide->progress_param, t);
	gs_effect_set_vec2(slide->direction_param, &slide->direction);

	while (gs_effect_loop(slide->effect, "Slide"))
		gs_draw_sprite(NULL, 0, cx, cy);
}

void slide_video_render(void *data, gs_effect_t *effect)
{
	struct slide_info *slide = data;
	obs_transition_video_render(slide->source, slide_callback);
	UNUSED_PARAMETER(effect);
}

static float mix_a(void *data, float t)
{
	UNUSED_PARAMETER(data);
	return 1.0f - t;
}

static float mix_b(void *data, float t)
{
	UNUSED_PARAMETER(data);
	return t;
}

bool slide_audio_render(void *data, uint64_t *ts_out,
		struct obs_source_audio_mix *audio, uint32_t mixers,
		size_t channels, size_t sample_rate)
{
	struct slide_info *slide = data;
	return obs_transition_audio_render(slide->source, ts_out,
		audio, mixers, channels, sample_rate, mix_a, mix_b);
}

struct obs_source_info slide_transition = {
	.id = "slide_transition",
	.type = OBS_SOURCE_TYPE_TRANSITION,
	.get_name = slide_get_name,
	.create = slide_create,
	.destroy = slide_destroy,
	.video_render = slide_video_render,
	.audio_render = slide_audio_render
};
