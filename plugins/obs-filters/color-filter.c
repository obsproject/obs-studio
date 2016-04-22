#include <obs-module.h>
#include <graphics/vec4.h>

#define SETTING_COLOR                  "color"
#define SETTING_OPACITY                "opacity"
#define SETTING_CONTRAST               "contrast"
#define SETTING_BRIGHTNESS             "brightness"
#define SETTING_GAMMA                  "gamma"

#define TEXT_COLOR                     obs_module_text("Color")
#define TEXT_OPACITY                   obs_module_text("Opacity")
#define TEXT_CONTRAST                  obs_module_text("Contrast")
#define TEXT_BRIGHTNESS                obs_module_text("Brightness")
#define TEXT_GAMMA                     obs_module_text("Gamma")

#define MIN_CONTRAST 0.5f
#define MAX_CONTRAST 2.0f
#define MIN_BRIGHTNESS -1.0
#define MAX_BRIGHTNESS 1.0

struct color_filter_data {
	obs_source_t                   *context;

	gs_effect_t                    *effect;

	gs_eparam_t                    *color_param;
	gs_eparam_t                    *contrast_param;
	gs_eparam_t                    *brightness_param;
	gs_eparam_t                    *gamma_param;

	struct vec4                    color;
	float                          contrast;
	float                          brightness;
	float                          gamma;
};

static const char *color_filter_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("ColorFilter");
}

static void color_filter_update(void *data, obs_data_t *settings)
{
	struct color_filter_data *filter = data;
	uint32_t color = (uint32_t)obs_data_get_int(settings, SETTING_COLOR);
	uint32_t opacity = (uint32_t)obs_data_get_int(settings,
			SETTING_OPACITY);
	double contrast = obs_data_get_double(settings, SETTING_CONTRAST);
	double brightness = obs_data_get_double(settings, SETTING_BRIGHTNESS);
	double gamma = obs_data_get_double(settings, SETTING_GAMMA);

	color &= 0xFFFFFF;
	color |= ((opacity * 255) / 100) << 24;

	vec4_from_rgba(&filter->color, color);

	contrast = (contrast < 0.0) ?
		(1.0 / (-contrast + 1.0)) : (contrast + 1.0);

	brightness *= 0.5;

	gamma = (gamma < 0.0) ? (-gamma + 1.0) : (1.0 / (gamma + 1.0));

	filter->contrast = (float)contrast;
	filter->brightness = (float)brightness;
	filter->gamma = (float)gamma;
}

static void color_filter_destroy(void *data)
{
	struct color_filter_data *filter = data;

	if (filter->effect) {
		obs_enter_graphics();
		gs_effect_destroy(filter->effect);
		obs_leave_graphics();
	}

	bfree(data);
}

static void *color_filter_create(obs_data_t *settings, obs_source_t *context)
{
	struct color_filter_data *filter =
		bzalloc(sizeof(struct color_filter_data));
	char *effect_path = obs_module_file("color_filter.effect");

	filter->context = context;

	obs_enter_graphics();

	filter->effect = gs_effect_create_from_file(effect_path, NULL);
	if (filter->effect) {
		filter->color_param = gs_effect_get_param_by_name(
				filter->effect, "color");
		filter->contrast_param = gs_effect_get_param_by_name(
				filter->effect, "contrast");
		filter->brightness_param = gs_effect_get_param_by_name(
				filter->effect, "brightness");
		filter->gamma_param = gs_effect_get_param_by_name(
				filter->effect, "gamma");
	}

	obs_leave_graphics();

	bfree(effect_path);

	if (!filter->effect) {
		color_filter_destroy(filter);
		return NULL;
	}

	color_filter_update(filter, settings);
	return filter;
}

static void color_filter_render(void *data, gs_effect_t *effect)
{
	struct color_filter_data *filter = data;

	if (!obs_source_process_filter_begin(filter->context, GS_RGBA,
				OBS_ALLOW_DIRECT_RENDERING))
		return;

	gs_effect_set_vec4(filter->color_param, &filter->color);
	gs_effect_set_float(filter->contrast_param, filter->contrast);
	gs_effect_set_float(filter->brightness_param, filter->brightness);
	gs_effect_set_float(filter->gamma_param, filter->gamma);

	obs_source_process_filter_end(filter->context, filter->effect, 0, 0);

	UNUSED_PARAMETER(effect);
}

static obs_properties_t *color_filter_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	obs_properties_add_color(props, SETTING_COLOR, TEXT_COLOR);
	obs_properties_add_int(props, SETTING_OPACITY, TEXT_OPACITY,
			0, 100, 1);
	obs_properties_add_float_slider(props, SETTING_CONTRAST,
			TEXT_CONTRAST, -1.0, 1.0, 0.01);
	obs_properties_add_float_slider(props, SETTING_BRIGHTNESS,
			TEXT_BRIGHTNESS, -1.0, 1.0, 0.01);
	obs_properties_add_float_slider(props, SETTING_GAMMA,
			TEXT_GAMMA, -1.0, 1.0, 0.01);

	UNUSED_PARAMETER(data);
	return props;
}

static void color_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, SETTING_COLOR, 0xFFFFFF);
	obs_data_set_default_int(settings, SETTING_OPACITY, 100);
	obs_data_set_default_double(settings, SETTING_CONTRAST, 0.0);
	obs_data_set_default_double(settings, SETTING_BRIGHTNESS, 0.0);
	obs_data_set_default_double(settings, SETTING_GAMMA, 0.0);
}

struct obs_source_info color_filter = {
	.id                            = "color_filter",
	.type                          = OBS_SOURCE_TYPE_FILTER,
	.output_flags                  = OBS_SOURCE_VIDEO,
	.get_name                      = color_filter_name,
	.create                        = color_filter_create,
	.destroy                       = color_filter_destroy,
	.video_render                  = color_filter_render,
	.update                        = color_filter_update,
	.get_properties                = color_filter_properties,
	.get_defaults                  = color_filter_defaults
};
