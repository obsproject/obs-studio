#include <obs-module.h>

/* clang-format off */

#define SETTING_SDR_ONLY_INFO      "sdr_only_info"
#define SETTING_LUMA_MAX           "luma_max"
#define SETTING_LUMA_MIN           "luma_min"
#define SETTING_LUMA_MAX_SMOOTH    "luma_max_smooth"
#define SETTING_LUMA_MIN_SMOOTH    "luma_min_smooth"

#define TEXT_SDR_ONLY_INFO      obs_module_text("SdrOnlyInfo")
#define TEXT_LUMA_MAX           obs_module_text("Luma.LumaMax")
#define TEXT_LUMA_MIN           obs_module_text("Luma.LumaMin")
#define TEXT_LUMA_MAX_SMOOTH    obs_module_text("Luma.LumaMaxSmooth")
#define TEXT_LUMA_MIN_SMOOTH    obs_module_text("Luma.LumaMinSmooth")

/* clang-format on */

struct luma_key_filter_data {
	obs_source_t *context;

	gs_effect_t *effect;

	gs_eparam_t *luma_max_param;
	gs_eparam_t *luma_min_param;
	gs_eparam_t *luma_max_smooth_param;
	gs_eparam_t *luma_min_smooth_param;

	float luma_max;
	float luma_min;
	float luma_max_smooth;
	float luma_min_smooth;
};

static const char *luma_key_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("LumaKeyFilter");
}

static void luma_key_update(void *data, obs_data_t *settings)
{
	struct luma_key_filter_data *filter = data;

	double lumaMax = obs_data_get_double(settings, SETTING_LUMA_MAX);
	double lumaMin = obs_data_get_double(settings, SETTING_LUMA_MIN);
	double lumaMaxSmooth = obs_data_get_double(settings, SETTING_LUMA_MAX_SMOOTH);
	double lumaMinSmooth = obs_data_get_double(settings, SETTING_LUMA_MIN_SMOOTH);

	filter->luma_max = (float)lumaMax;
	filter->luma_min = (float)lumaMin;
	filter->luma_max_smooth = (float)lumaMaxSmooth;
	filter->luma_min_smooth = (float)lumaMinSmooth;
}

static void luma_key_destroy(void *data)
{
	struct luma_key_filter_data *filter = data;

	if (filter->effect) {
		obs_enter_graphics();
		gs_effect_destroy(filter->effect);
		obs_leave_graphics();
	}

	bfree(data);
}

static void *luma_key_create_internal(obs_data_t *settings, obs_source_t *context, const char *effect_name)
{
	struct luma_key_filter_data *filter = bzalloc(sizeof(struct luma_key_filter_data));
	char *effect_path = obs_module_file(effect_name);

	filter->context = context;

	obs_enter_graphics();

	filter->effect = gs_effect_create_from_file(effect_path, NULL);
	if (filter->effect) {
		filter->luma_max_param = gs_effect_get_param_by_name(filter->effect, "lumaMax");
		filter->luma_min_param = gs_effect_get_param_by_name(filter->effect, "lumaMin");
		filter->luma_max_smooth_param = gs_effect_get_param_by_name(filter->effect, "lumaMaxSmooth");
		filter->luma_min_smooth_param = gs_effect_get_param_by_name(filter->effect, "lumaMinSmooth");
	}

	obs_leave_graphics();

	bfree(effect_path);

	if (!filter->effect) {
		luma_key_destroy(filter);
		return NULL;
	}

	luma_key_update(filter, settings);
	return filter;
}

static void *luma_key_create_v1(obs_data_t *settings, obs_source_t *context)
{
	return luma_key_create_internal(settings, context, "luma_key_filter.effect");
}

static void *luma_key_create_v2(obs_data_t *settings, obs_source_t *context)
{
	return luma_key_create_internal(settings, context, "luma_key_filter_v2.effect");
}

static void luma_key_render_internal(void *data, bool premultiplied)
{
	struct luma_key_filter_data *filter = data;

	const enum gs_color_space preferred_spaces[] = {
		GS_CS_SRGB,
		GS_CS_SRGB_16F,
		GS_CS_709_EXTENDED,
	};

	const enum gs_color_space source_space = obs_source_get_color_space(
		obs_filter_get_target(filter->context), OBS_COUNTOF(preferred_spaces), preferred_spaces);
	if (source_space == GS_CS_709_EXTENDED) {
		obs_source_skip_video_filter(filter->context);
	} else {
		const enum gs_color_format format = gs_get_format_from_space(source_space);
		if (obs_source_process_filter_begin_with_color_space(filter->context, format, source_space,
								     OBS_ALLOW_DIRECT_RENDERING)) {
			gs_effect_set_float(filter->luma_max_param, filter->luma_max);
			gs_effect_set_float(filter->luma_min_param, filter->luma_min);
			gs_effect_set_float(filter->luma_max_smooth_param, filter->luma_max_smooth);
			gs_effect_set_float(filter->luma_min_smooth_param, filter->luma_min_smooth);

			if (premultiplied) {
				gs_blend_state_push();
				gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);
			}

			obs_source_process_filter_end(filter->context, filter->effect, 0, 0);

			if (premultiplied) {
				gs_blend_state_pop();
			}
		}
	}
}

static void luma_key_render_v1(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);

	luma_key_render_internal(data, false);
}

static void luma_key_render_v2(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);

	luma_key_render_internal(data, true);
}

static obs_properties_t *luma_key_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	obs_properties_add_text(props, SETTING_SDR_ONLY_INFO, TEXT_SDR_ONLY_INFO, OBS_TEXT_INFO);
	obs_properties_add_float_slider(props, SETTING_LUMA_MAX, TEXT_LUMA_MAX, 0, 1, 0.0001);
	obs_properties_add_float_slider(props, SETTING_LUMA_MAX_SMOOTH, TEXT_LUMA_MAX_SMOOTH, 0, 1, 0.0001);
	obs_properties_add_float_slider(props, SETTING_LUMA_MIN, TEXT_LUMA_MIN, 0, 1, 0.0001);
	obs_properties_add_float_slider(props, SETTING_LUMA_MIN_SMOOTH, TEXT_LUMA_MIN_SMOOTH, 0, 1, 0.0001);

	UNUSED_PARAMETER(data);
	return props;
}

static void luma_key_defaults(obs_data_t *settings)
{
	obs_data_set_default_double(settings, SETTING_LUMA_MAX, 1.0);
	obs_data_set_default_double(settings, SETTING_LUMA_MIN, 0.0);
	obs_data_set_default_double(settings, SETTING_LUMA_MAX_SMOOTH, 0.0);
	obs_data_set_default_double(settings, SETTING_LUMA_MIN_SMOOTH, 0.0);
}

static enum gs_color_space luma_key_get_color_space(void *data, size_t count,
						    const enum gs_color_space *preferred_spaces)
{
	UNUSED_PARAMETER(count);
	UNUSED_PARAMETER(preferred_spaces);

	const enum gs_color_space potential_spaces[] = {
		GS_CS_SRGB,
		GS_CS_SRGB_16F,
		GS_CS_709_EXTENDED,
	};

	struct luma_key_filter_data *const filter = data;
	const enum gs_color_space source_space = obs_source_get_color_space(
		obs_filter_get_target(filter->context), OBS_COUNTOF(potential_spaces), potential_spaces);

	return source_space;
}

struct obs_source_info luma_key_filter = {
	.id = "luma_key_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CAP_OBSOLETE,
	.get_name = luma_key_name,
	.create = luma_key_create_v1,
	.destroy = luma_key_destroy,
	.video_render = luma_key_render_v1,
	.update = luma_key_update,
	.get_properties = luma_key_properties,
	.get_defaults = luma_key_defaults,
};

struct obs_source_info luma_key_filter_v2 = {
	.id = "luma_key_filter",
	.version = 2,
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_SRGB,
	.get_name = luma_key_name,
	.create = luma_key_create_v2,
	.destroy = luma_key_destroy,
	.video_render = luma_key_render_v2,
	.update = luma_key_update,
	.get_properties = luma_key_properties,
	.get_defaults = luma_key_defaults,
	.video_get_color_space = luma_key_get_color_space,
};
