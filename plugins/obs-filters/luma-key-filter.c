#include <obs-module.h>

#define SETTING_LUMA_MAX           "luma_max"
#define SETTING_LUMA_MIN           "luma_min"
#define SETTING_LUMA_MAX_SMOOTH    "luma_max_smooth"
#define SETTING_LUMA_MIN_SMOOTH    "luma_min_smooth"
#define SETTING_COLOR		   "color"
#define SETTING_INVERT_LUMA	   "invert_luma"
#define SETTING_INVERT_COLOR	   "invert_color"

#define TEXT_LUMA_MAX           obs_module_text("Luma.LumaMax")
#define TEXT_LUMA_MIN           obs_module_text("Luma.LumaMin")
#define TEXT_LUMA_MAX_SMOOTH    obs_module_text("Luma.LumaMaxSmooth")
#define TEXT_LUMA_MIN_SMOOTH    obs_module_text("Luma.LumaMinSmooth")
#define TEXT_COLOR              obs_module_text("Color")
#define TEXT_INVERT_LUMA	obs_module_text("Invert Luma")
#define TEXT_INVERT_COLOR	obs_module_text("Invert Color")


struct luma_key_filter_data {
	obs_source_t    *context;

	gs_effect_t     *effect;

	gs_eparam_t     *luma_max_param;
	gs_eparam_t     *luma_min_param;
	gs_eparam_t     *luma_max_smooth_param;
	gs_eparam_t     *luma_min_smooth_param;
	gs_eparam_t     *color_param;
	gs_eparam_t     *invert_color_param;
	gs_eparam_t     *invert_luma_param;


	float           luma_max;
	float           luma_min;
	float           luma_max_smooth;
	float           luma_min_smooth;
	struct vec4     color;
	bool		invert_color;
	bool		invert_luma;
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
	uint32_t color = (uint32_t)obs_data_get_int(settings, SETTING_COLOR);

	filter->luma_max = (float)lumaMax;
	filter->luma_min = (float)lumaMin;
	filter->luma_max_smooth = (float)lumaMaxSmooth;
	filter->luma_min_smooth = (float)lumaMinSmooth;
	vec4_from_rgba(&filter->color, color);
	filter->invert_color = obs_data_get_bool(settings, SETTING_INVERT_COLOR);
	filter->invert_luma = obs_data_get_bool(settings, SETTING_INVERT_LUMA);
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

static void *luma_key_create(obs_data_t *settings, obs_source_t *context)
{
	struct luma_key_filter_data *filter =
		bzalloc(sizeof(struct luma_key_filter_data));
	char *effect_path = obs_module_file("luma_key_filter.effect");

	filter->context = context;

	obs_enter_graphics();

	filter->effect = gs_effect_create_from_file(effect_path, NULL);
	if (filter->effect) {
		filter->luma_max_param = gs_effect_get_param_by_name(
			filter->effect, "lumaMax");
		filter->luma_min_param = gs_effect_get_param_by_name(
			filter->effect, "lumaMin");
		filter->luma_max_smooth_param = gs_effect_get_param_by_name(
			filter->effect, "lumaMaxSmooth");
		filter->luma_min_smooth_param = gs_effect_get_param_by_name(
			filter->effect, "lumaMinSmooth");
		filter->color_param = gs_effect_get_param_by_name(
			filter->effect, "color");
		filter->invert_color_param = gs_effect_get_param_by_name(
			filter->effect, "invertColor");
		filter->invert_luma_param = gs_effect_get_param_by_name(
			filter->effect, "invertLuma");
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

static void luma_key_render(void *data, gs_effect_t *effect)
{
	struct luma_key_filter_data *filter = data;

	if (!obs_source_process_filter_begin(filter->context, GS_RGBA,
		OBS_ALLOW_DIRECT_RENDERING))
		return;

	gs_effect_set_float(filter->luma_max_param, filter->luma_max);
	gs_effect_set_float(filter->luma_min_param, filter->luma_min);
	gs_effect_set_float(filter->luma_max_smooth_param, filter->luma_max_smooth);
	gs_effect_set_float(filter->luma_min_smooth_param, filter->luma_min_smooth);
	gs_effect_set_vec4(filter->color_param, &filter->color);
	gs_effect_set_bool(filter->invert_color_param, filter->invert_color);
	gs_effect_set_bool(filter->invert_luma_param, filter->invert_luma);

	obs_source_process_filter_end(filter->context, filter->effect, 0, 0);

	UNUSED_PARAMETER(effect);
}

static obs_properties_t *luma_key_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	obs_properties_add_float_slider(props, SETTING_LUMA_MAX,
		TEXT_LUMA_MAX, 0, 1, 0.01);
	obs_properties_add_float_slider(props, SETTING_LUMA_MAX_SMOOTH,
		TEXT_LUMA_MAX_SMOOTH, 0, 1, 0.01);
	obs_properties_add_float_slider(props, SETTING_LUMA_MIN,
		TEXT_LUMA_MIN, 0, 1, 0.01);
	obs_properties_add_float_slider(props, SETTING_LUMA_MIN_SMOOTH,
		TEXT_LUMA_MIN_SMOOTH, 0, 1, 0.01);
	obs_properties_add_color(props, SETTING_COLOR, TEXT_COLOR);
	obs_properties_add_bool(props, SETTING_INVERT_COLOR, TEXT_INVERT_COLOR);
	obs_properties_add_bool(props, SETTING_INVERT_LUMA, TEXT_INVERT_LUMA);


	UNUSED_PARAMETER(data);
	return props;
}

static void luma_key_defaults(obs_data_t *settings)
{
	obs_data_set_default_double(settings, SETTING_LUMA_MAX, 1.0);
	obs_data_set_default_double(settings, SETTING_LUMA_MIN, 0.0);
	obs_data_set_default_double(settings, SETTING_LUMA_MAX_SMOOTH, 0.0);
	obs_data_set_default_double(settings, SETTING_LUMA_MIN_SMOOTH, 0.0);
	obs_data_set_default_int(settings, SETTING_COLOR, 0xFFFFFF);
	obs_data_set_default_bool(settings, SETTING_INVERT_COLOR, false);
	obs_data_set_default_bool(settings, SETTING_INVERT_LUMA, false);
}


struct obs_source_info luma_key_filter = {
	.id = "luma_key_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = luma_key_name,
	.create = luma_key_create,
	.destroy = luma_key_destroy,
	.video_render = luma_key_render,
	.update = luma_key_update,
	.get_properties = luma_key_properties,
	.get_defaults = luma_key_defaults
};
