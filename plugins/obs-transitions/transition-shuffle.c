#include <obs-module.h>
#include <graphics/vec2.h>
#include "easings.h"

// Assuming the timer speed or duration might be configurable in the future
#define S_SPEED "speed"

struct shuffle_info {
	obs_source_t *source;

	gs_effect_t *effect;
	gs_eparam_t *a_param;
	gs_eparam_t *b_param;
	gs_eparam_t *timer_param;
	gs_eparam_t *speed_param;

	float speed;
};

static const char *shuffle_get_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return obs_module_text("ShuffleTransition");
}

static void shuffle_update(void *data, obs_data_t *settings)
{
	struct shuffle_info *pm = data;
	pm->speed = (float)obs_data_get_double(settings, S_SPEED);
}

void *shuffle_create(obs_data_t *settings, obs_source_t *source)
{
	struct shuffle_info *pm;
	gs_effect_t *effect;

	char *file = obs_module_file("shuffle_transition.effect");

	obs_enter_graphics();
	effect = gs_effect_create_from_file(file, NULL);
	obs_leave_graphics();

	bfree(file);

	if (!effect) {
		blog(LOG_ERROR, "Could not find shuffle_transition.effect");
		return NULL;
	}

	pm = bzalloc(sizeof(*pm));
	pm->source = source;
	pm->effect = effect;

	pm->a_param = gs_effect_get_param_by_name(effect, "tex_a");
	pm->b_param = gs_effect_get_param_by_name(effect, "tex_b");
	pm->timer_param = gs_effect_get_param_by_name(effect, "timer");
	pm->speed_param = gs_effect_get_param_by_name(effect, "speed");

	obs_source_update(source, settings);

	return pm;
}

void shuffle_destroy(void *data)
{
	struct shuffle_info *pm = data;
	bfree(pm);
}

static void shuffle_callback(void *data, gs_texture_t *a, gs_texture_t *b,
			     float t, uint32_t cx, uint32_t cy)
{
	struct shuffle_info *pm = data;

	const bool previous = gs_framebuffer_srgb_enabled();
	gs_enable_framebuffer_srgb(true);

	gs_effect_set_texture_srgb(pm->a_param, a);
	gs_effect_set_texture_srgb(pm->b_param, b);
	gs_effect_set_float(pm->timer_param, t);
	gs_effect_set_float(pm->speed_param, pm->speed);

	while (gs_effect_loop(pm->effect, "Shuffle"))
		gs_draw_sprite(NULL, 0, cx, cy);

	gs_enable_framebuffer_srgb(previous);
}

void shuffle_video_render(void *data, gs_effect_t *effect)
{
	struct shuffle_info *pm = data;
	obs_transition_video_render(pm->source, shuffle_callback);
	UNUSED_PARAMETER(effect);
}

static float mix_a(void *data, float t)
{
	UNUSED_PARAMETER(data);
	return 1.0f - cubic_ease_in_out(t);
}

static float mix_b(void *data, float t)
{
	UNUSED_PARAMETER(data);
	return cubic_ease_in_out(t);
}

static bool shuffle_audio_render(void *data, uint64_t *ts_out,
				 struct obs_source_audio_mix *audio,
				 uint32_t mixers, size_t channels,
				 size_t sample_rate)
{
	struct shuffle_info *pm = data;
	return obs_transition_audio_render(pm->source, ts_out, audio, mixers,
					   channels, sample_rate, mix_a, mix_b);
}
static bool shuffle_audio_render_do(void *data, uint64_t *ts_out,
				    struct audio_data_mixes_outputs *audio,
				    uint32_t mixers, size_t channels,
				    size_t sample_rate)
{
	struct shuffle_info *pm = data;
	return obs_transition_audio_render_do(pm->source, ts_out, audio, mixers,
					      channels, sample_rate, mix_a,
					      mix_b);
}

static enum gs_color_space
shuffle_video_get_color_space(void *data, size_t count,
			      const enum gs_color_space *preferred_spaces)
{
	UNUSED_PARAMETER(count);
	UNUSED_PARAMETER(preferred_spaces);

	struct shuffle_info *pm = data;
	return obs_transition_video_get_color_space(pm->source);
}

static obs_properties_t *shuffle_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();
	obs_properties_add_int_slider(props, S_SPEED, "Speed", 1, 10, 1);

	UNUSED_PARAMETER(data);
	return props;
}

static void shuffle_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, S_SPEED, 2);
}

struct obs_source_info shuffle_transition = {
	.id = "shuffle_transition",
	.type = OBS_SOURCE_TYPE_TRANSITION,
	.get_name = shuffle_get_name,
	.create = shuffle_create,
	.destroy = shuffle_destroy,
	.update = shuffle_update,
	.video_render = shuffle_video_render,
	.audio_render = shuffle_audio_render,
	.audio_render_do = shuffle_audio_render_do,
	.get_properties = shuffle_properties,
	.get_defaults = shuffle_defaults,
	.video_get_color_space = shuffle_video_get_color_space,
};
