#include <obs-module.h>
#include <graphics/matrix4.h>

#define SETTING_WHITE_BALANCE_RED "white_balance_red"
#define SETTING_WHITE_BALANCE_GREEN "white_balance_green"
#define SETTING_WHITE_BALANCE_BLUE "white_balance_blue"

#define TEXT_WHITE_BALANCE_RED obs_module_text("Red")
#define TEXT_WHITE_BALANCE_GREEN obs_module_text("Green")
#define TEXT_WHITE_BALANCE_BLUE obs_module_text("Blue")

struct white_balance_filter_data {
	obs_source_t *context;
	gs_effect_t *effect;
	gs_eparam_t *final_matrix_param;
	struct matrix4 final_matrix;
};

static const char *white_balance_filter_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("WhiteBalance");
}

static void white_balance_filter_update(void *data, obs_data_t *settings)
{
	struct white_balance_filter_data *filter = data;

	/* Retrieve white balance values */
	float white_balance_red = (float)obs_data_get_double(settings, SETTING_WHITE_BALANCE_RED);
	float white_balance_green = (float)obs_data_get_double(settings, SETTING_WHITE_BALANCE_GREEN);
	float white_balance_blue = (float)obs_data_get_double(settings, SETTING_WHITE_BALANCE_BLUE);

	/* Compute white balance matrix */
	struct matrix4 white_balance_matrix = {1.0f + white_balance_red,
					       0.0f,
					       0.0f,
					       0.0f,
					       0.0f,
					       1.0f + white_balance_green,
					       0.0f,
					       0.0f,
					       0.0f,
					       0.0f,
					       1.0f + white_balance_blue,
					       0.0f,
					       0.0f,
					       0.0f,
					       0.0f,
					       1.0f};

	filter->final_matrix = white_balance_matrix;
}

static void *white_balance_filter_create(obs_data_t *settings, obs_source_t *context)
{
	struct white_balance_filter_data *filter = bzalloc(sizeof(struct white_balance_filter_data));
	filter->context = context;
	char *effect_path = obs_module_file("white_balance_filter.effect");

	obs_enter_graphics();
	filter->effect = gs_effect_create_from_file(effect_path, NULL);
	if (filter->effect)
		filter->final_matrix_param = gs_effect_get_param_by_name(filter->effect, "color_matrix");
	obs_leave_graphics();

	bfree(effect_path);
	if (!filter->effect) {
		bfree(filter);
		return NULL;
	}

	white_balance_filter_update(filter, settings);
	return filter;
}

static void white_balance_filter_destroy(void *data)
{
	struct white_balance_filter_data *filter = data;
	if (filter->effect) {
		obs_enter_graphics();
		gs_effect_destroy(filter->effect);
		obs_leave_graphics();
	}
	bfree(filter);
}

static void white_balance_filter_render(void *data, gs_effect_t *effect)
{
	struct white_balance_filter_data *filter = data;
	if (!obs_source_process_filter_begin(filter->context, GS_RGBA, OBS_ALLOW_DIRECT_RENDERING))
		return;

	gs_effect_set_matrix4(filter->final_matrix_param, &filter->final_matrix);
	obs_source_process_filter_end(filter->context, filter->effect, 0, 0);

	UNUSED_PARAMETER(effect);
}

static obs_properties_t *white_balance_filter_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();
	obs_properties_add_float_slider(props, SETTING_WHITE_BALANCE_RED, TEXT_WHITE_BALANCE_RED, -1.0, 1.0, 0.01);
	obs_properties_add_float_slider(props, SETTING_WHITE_BALANCE_GREEN, TEXT_WHITE_BALANCE_GREEN, -1.0, 1.0, 0.01);
	obs_properties_add_float_slider(props, SETTING_WHITE_BALANCE_BLUE, TEXT_WHITE_BALANCE_BLUE, -1.0, 1.0, 0.01);
	UNUSED_PARAMETER(data);
	return props;
}

static void white_balance_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_double(settings, SETTING_WHITE_BALANCE_RED, 0.0);
	obs_data_set_default_double(settings, SETTING_WHITE_BALANCE_GREEN, 0.0);
	obs_data_set_default_double(settings, SETTING_WHITE_BALANCE_BLUE, 0.0);
}

struct obs_source_info white_balance_filter = {
	.id = "white_balance_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_SRGB,
	.get_name = white_balance_filter_name,
	.create = white_balance_filter_create,
	.destroy = white_balance_filter_destroy,
	.video_render = white_balance_filter_render,
	.update = white_balance_filter_update,
	.get_properties = white_balance_filter_properties,
	.get_defaults = white_balance_filter_defaults,
};
