#include <obs-module.h>
#include <graphics/matrix4.h>
#include <graphics/vec2.h>
#include <graphics/vec4.h>

/* clang-format off */

#define SETTING_SHAPE                 "shape"
#define SETTING_RADIUS		      "radius"
#define SETTING_RADIUS2		      "radius2"

#define TEXT_SHAPE                    obs_module_text("Shape")
#define TEXT_RADIUS		      obs_module_text("Radius")
#define TEXT_AXIS_A		      obs_module_text("AxisA")
#define TEXT_AXIS_B		      obs_module_text("AxisB")

/* clang-format on */
struct garbage_matte_filter_data {
	obs_source_t *context;

	gs_effect_t *effect;

	gs_eparam_t *point1_param;
	gs_eparam_t *point2_param;

	int shape;
	struct vec2 point1;
	struct vec2 point2;
};

static const char *garbage_matte_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("GarbageMatteFilter");
}

static void garbage_matte_update(void *data, obs_data_t *settings)
{
	struct garbage_matte_filter_data *filter = data;

	vec2_zero(&filter->point1);
	vec2_zero(&filter->point2);

	filter->shape = (int)obs_data_get_int(settings, SETTING_SHAPE);

	// Circle / Square
	if (filter->shape == 0) {
		filter->point1.x = (float)obs_data_get_int(settings, "X1");
		filter->point1.y = (float)obs_data_get_int(settings, "Y1");
		filter->point2.x =
			(float)obs_data_get_int(settings, SETTING_RADIUS);
	}

	// Ellipse
	else if (filter->shape == 1) {
		filter->point1.x = (float)obs_data_get_int(settings, "X1");
		filter->point1.y = (float)obs_data_get_int(settings, "Y1");
		filter->point2.x =
			(float)obs_data_get_int(settings, SETTING_RADIUS);
		filter->point2.y =
			(float)obs_data_get_int(settings, SETTING_RADIUS2);
	}

	// Rectangle
	else if (filter->shape == 2) {
		int x1 = (int)obs_data_get_int(settings, "X1");
		int y1 = (int)obs_data_get_int(settings, "Y1");
		int x2 = (int)obs_data_get_int(settings, "X2");
		int y2 = (int)obs_data_get_int(settings, "Y2");

		// Merge the lowest positions into point1, highest into point2
		filter->point1.x = (float)min(x1, x2);
		filter->point2.x = (float)max(x1, x2);
		filter->point1.y = (float)min(y1, y2);
		filter->point2.y = (float)max(y1, y2);
	}
}

static void garbage_matte_destroy(void *data)
{
	struct garbage_matte_filter_data *filter = data;

	if (filter->effect) {
		obs_enter_graphics();
		gs_effect_destroy(filter->effect);
		obs_leave_graphics();
	}

	bfree(data);
}

static void *garbage_matte_create(obs_data_t *settings, obs_source_t *context)
{
	struct garbage_matte_filter_data *filter =
		bzalloc(sizeof(struct garbage_matte_filter_data));
	char *effect_path = obs_module_file("garbage_matte_filter.effect");

	filter->context = context;

	obs_enter_graphics();

	filter->effect = gs_effect_create_from_file(effect_path, NULL);
	if (filter->effect) {
		filter->point1_param =
			gs_effect_get_param_by_name(filter->effect, "point1");
		filter->point2_param =
			gs_effect_get_param_by_name(filter->effect, "point2");
	}

	obs_leave_graphics();

	bfree(effect_path);

	if (!filter->effect) {
		garbage_matte_destroy(filter);
		return NULL;
	}

	garbage_matte_update(filter, settings);
	return filter;
}

static void garbage_matte_render(void *data, gs_effect_t *effect)
{
	struct garbage_matte_filter_data *filter = data;

	if (!obs_source_process_filter_begin(filter->context, GS_RGBA,
					     OBS_ALLOW_DIRECT_RENDERING))
		return;

	char *tech_name = NULL;
	switch (filter->shape) {
	case 0:
		tech_name = "DrawCircle";
		break;
	case 1:
		tech_name = "DrawEllipse";
		break;
	case 2:
		tech_name = "DrawRectangle";
		break;
	}

	gs_effect_set_vec2(filter->point1_param, &filter->point1);
	gs_effect_set_vec2(filter->point2_param, &filter->point2);

	obs_source_process_filter_tech_end(filter->context, filter->effect, 0,
					   0, tech_name);

	UNUSED_PARAMETER(effect);
}

static bool key_type_changed(obs_properties_t *props, obs_property_t *p,
			     obs_data_t *settings)
{
	obs_property_set_visible(obs_properties_get(props, SETTING_RADIUS), 0);
	obs_property_set_visible(obs_properties_get(props, SETTING_RADIUS2), 0);

	obs_property_set_visible(obs_properties_get(props, "X1"), 0);
	obs_property_set_visible(obs_properties_get(props, "Y1"), 0);
	obs_property_set_visible(obs_properties_get(props, "X2"), 0);
	obs_property_set_visible(obs_properties_get(props, "Y2"), 0);

	int type = (int)obs_data_get_int(settings, SETTING_SHAPE);

	// Circle
	if (type == 0) {
		obs_property_set_visible(obs_properties_get(props, "X1"), 1);
		obs_property_set_visible(obs_properties_get(props, "Y1"), 1);
		obs_property_set_visible(
			obs_properties_get(props, SETTING_RADIUS), 1);

		obs_property_set_description(
			obs_properties_get(props, SETTING_RADIUS), TEXT_RADIUS);
	}

	// Ellipse
	else if (type == 1) {
		obs_property_set_visible(obs_properties_get(props, "X1"), 1);
		obs_property_set_visible(obs_properties_get(props, "Y1"), 1);
		obs_property_set_visible(
			obs_properties_get(props, SETTING_RADIUS), 1);
		obs_property_set_description(
			obs_properties_get(props, SETTING_RADIUS), TEXT_AXIS_B);

		obs_property_set_visible(
			obs_properties_get(props, SETTING_RADIUS2), 1);
		obs_property_set_description(
			obs_properties_get(props, SETTING_RADIUS2),
			TEXT_AXIS_B);
	}

	// Rectangle
	else if (type == 2) {
		obs_property_set_visible(obs_properties_get(props, "X1"), 1);
		obs_property_set_visible(obs_properties_get(props, "Y1"), 1);

		obs_property_set_visible(obs_properties_get(props, "X2"), 1);
		obs_property_set_visible(obs_properties_get(props, "Y2"), 1);
	}

	UNUSED_PARAMETER(p);
	return true;
}

static obs_properties_t *garbage_matte_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	obs_property_t *p = obs_properties_add_list(props, SETTING_SHAPE,
						    TEXT_SHAPE,
						    OBS_COMBO_TYPE_LIST,
						    OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(p, "Circle", 0);
	obs_property_list_add_int(p, "Ellipse", 1);
	obs_property_list_add_int(p, "Rectangle", 2);
	obs_property_set_modified_callback(p, key_type_changed);

	obs_properties_add_int(props, "X1", "X1", 0, 8192, 1);
	obs_properties_add_int(props, "Y1", "Y1", 0, 8192, 1);

	obs_properties_add_int_slider(props, SETTING_RADIUS, TEXT_RADIUS, 1,
				      8196, 0);

	obs_properties_add_int(props, "X2", "X2", 0, 8192, 1);
	obs_properties_add_int(props, "Y2", "Y2", 0, 8192, 1);

	obs_properties_add_int_slider(props, SETTING_RADIUS2, TEXT_RADIUS, 1,
				      8196, 0);

	UNUSED_PARAMETER(data);
	return props;
}

static void garbage_matte_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, SETTING_SHAPE, 0);
	obs_data_set_default_int(settings, SETTING_RADIUS, 0);
	obs_data_set_default_int(settings, SETTING_RADIUS2, 0);

	obs_data_set_default_int(settings, "X1", 0);
	obs_data_set_default_int(settings, "Y1", 0);

	obs_data_set_default_int(settings, "X2", 0);
	obs_data_set_default_int(settings, "Y2", 0);
}

struct obs_source_info garbage_matte_filter = {
	.id = "garbage_matte_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = garbage_matte_name,
	.create = garbage_matte_create,
	.destroy = garbage_matte_destroy,
	.video_render = garbage_matte_render,
	.update = garbage_matte_update,
	.get_properties = garbage_matte_properties,
	.get_defaults = garbage_matte_defaults,
};
