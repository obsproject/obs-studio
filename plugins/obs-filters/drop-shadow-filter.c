#include <obs-module.h>
#include <graphics/graphics.h>
#include <graphics/matrix4.h>
#include <graphics/vec2.h>
#include <graphics/vec4.h>

#include <stdio.h>

#define SETTING_X_OFFSET               "x_offset"
#define SETTING_Y_OFFSET               "y_offset"
#define SETTING_BLUR_RADIUS            "blur_radius"
#define SETTING_OPACITY                "opacity"
#define SETTING_SHADOW_COLOR           "shadow_color"

#define TEXT_X_OFFSET                  obs_module_text("XOffset")
#define TEXT_Y_OFFSET                  obs_module_text("YOffset")
#define TEXT_BLUR_RADIUS               obs_module_text("BlurRadius")
#define TEXT_OPACITY                   obs_module_text("Opacity")
#define TEXT_SHADOW_COLOR              obs_module_text("ShadowColor")

#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define MAX_SHADOW_TARGETS		8

struct drop_shadow_filter_data {
	obs_source_t *context;

	gs_effect_t *effect;

	gs_texrender_t *shadow_target[MAX_SHADOW_TARGETS];

	gs_eparam_t *offset_param;
	gs_eparam_t *shadow_color_param;
	gs_eparam_t *image_param;
	gs_eparam_t *image_uv_offset_param;
	gs_eparam_t *image_uv_scale_param;
	gs_eparam_t *image_uv_pixel_interval_param;
	gs_eparam_t *shadow_uv_pixel_interval_param;
	gs_eparam_t *shadow_param;
	gs_eparam_t *kawase_distance_param;

	int blur_radius;
	int expand_left;
	int expand_right;
	int expand_top;
	int expand_bottom;
	int total_width;
	int total_height;

	struct vec2 offset;
	struct vec4 shadow_color;
	struct vec2 image_uv_offset;
	struct vec2 image_uv_scale;
	struct vec2 image_uv_pixel_interval;
	struct vec2 shadow_uv_pixel_interval;
};

static const char *drop_shadow_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("DropShadowFilter");
}

static void drop_shadow_update(void *data, obs_data_t *settings)
{
	struct drop_shadow_filter_data *filter = data;

	filter->offset.x = -obs_data_get_double(settings, SETTING_X_OFFSET);
	filter->offset.y = -obs_data_get_double(settings, SETTING_Y_OFFSET);
	filter->blur_radius = obs_data_get_int(settings, SETTING_BLUR_RADIUS);

	filter->expand_left = MAX(filter->blur_radius + filter->offset.x, 0);
	filter->expand_right = MAX(filter->blur_radius - filter->offset.x, 0);
	filter->expand_top = MAX(filter->blur_radius + filter->offset.y, 0);
	filter->expand_bottom = MAX(filter->blur_radius - filter->offset.y, 0);

	uint32_t opacity = (uint32_t)obs_data_get_int(settings,
			SETTING_OPACITY);
	uint32_t shadow_color = (uint32_t)obs_data_get_int(settings,
			SETTING_SHADOW_COLOR);

	shadow_color &= 0x00ffffff;
	shadow_color |= ((opacity * 255) / 100) << 24;
	vec4_from_rgba(&filter->shadow_color, shadow_color);
}

static void drop_shadow_alloc_targets(
	struct drop_shadow_filter_data *filter, int nr_targets)
{
	for (int i = 0; i < MAX_SHADOW_TARGETS; i++) {
		gs_texrender_t *target = filter->shadow_target[i];

		if (i < nr_targets) {
			if (!target) {
				target = gs_texrender_create(GS_RGBA,
						GS_ZS_NONE);
			}
		} else {
			if (target) {
				gs_texrender_destroy(target);
				target = NULL;
			}
		}

		filter->shadow_target[i] = target;
	}
}

static void drop_shadow_destroy(void *data)
{
	struct drop_shadow_filter_data *filter = data;

	if (filter->effect) {
		obs_enter_graphics();
		gs_effect_destroy(filter->effect);
		drop_shadow_alloc_targets(filter, 0);
		obs_leave_graphics();
	}

	bfree(data);
}

static void *drop_shadow_create(obs_data_t *settings, obs_source_t *context)
{
	struct drop_shadow_filter_data *filter =
			bzalloc(sizeof(struct drop_shadow_filter_data));
	char *effect_path = obs_module_file("drop_shadow_filter.effect");

	filter->context = context;

	obs_enter_graphics();

	char *error_string;
	filter->effect = gs_effect_create_from_file(effect_path, &error_string);
	if (filter->effect) {
		filter->offset_param = gs_effect_get_param_by_name(
				filter->effect, "offset");
		filter->shadow_color_param = gs_effect_get_param_by_name(
				filter->effect, "shadow_color");
		filter->image_param = gs_effect_get_param_by_name(
				filter->effect, "image");
		filter->image_uv_offset_param = gs_effect_get_param_by_name(
				filter->effect, "image_uv_offset");
		filter->image_uv_scale_param = gs_effect_get_param_by_name(
				filter->effect, "image_uv_scale");
		filter->image_uv_pixel_interval_param =
				gs_effect_get_param_by_name(
				filter->effect, "image_uv_pixel_interval");
		filter->shadow_uv_pixel_interval_param =
				gs_effect_get_param_by_name(
				filter->effect, "shadow_uv_pixel_interval");
		filter->shadow_param = gs_effect_get_param_by_name(
				filter->effect, "shadow");
		filter->kawase_distance_param = gs_effect_get_param_by_name(
				filter->effect, "kawase_distance");
	} else {
		fprintf(stderr, "Failed to compile '%s':\n%s\n",
				effect_path, error_string);
	}

	obs_leave_graphics();

	bfree(effect_path);

	if (!filter->effect) {
		drop_shadow_destroy(filter);
		return NULL;
	}

	drop_shadow_update(filter, settings);
	return filter;
}

static void set_parameters(
		struct drop_shadow_filter_data *filter,
		gs_texrender_t *shadow_source,
		float kawase_distance)
{
	gs_effect_set_vec2(filter->offset_param, &filter->offset);
	gs_effect_set_vec4(filter->shadow_color_param, &filter->shadow_color);
	gs_effect_set_vec2(filter->image_uv_offset_param,
			&filter->image_uv_offset);
	gs_effect_set_vec2(filter->image_uv_scale_param,
			&filter->image_uv_scale);
	gs_effect_set_vec2(filter->image_uv_pixel_interval_param,
			&filter->image_uv_pixel_interval);
	gs_effect_set_vec2(filter->shadow_uv_pixel_interval_param,
			&filter->shadow_uv_pixel_interval);
	gs_effect_set_float(filter->kawase_distance_param,
			kawase_distance);

	if (shadow_source) {
		gs_texture_t *shadow = gs_texrender_get_texture(shadow_source);
		gs_effect_set_texture(filter->shadow_param, shadow);
	} else {
		gs_effect_set_texture(filter->shadow_param, NULL);
	}
}

static void set_image_parameter(struct drop_shadow_filter_data *filter)
{
	gs_texture_t *image = obs_filter_get_texture(filter->context);
	gs_effect_set_texture(filter->image_param, image);
}

typedef struct {
	int passes;
	float distances[7];
} kawase_entry_t;

/*
 * In the Kawase blur algorithm four texture samples are taken in
 * each pass. Each sample takes the average of 4 pixels using the
 * GPU's fixed-function for linear texture sampling.
 *
 * In each pass the Kawase blur can take samples further away from the
 * sample to include the blurred samples from the previous pass.
 * In the table below you will see sample distances either equal to
 * or one further than the previous pass.
 *
 * The blur-distance is about equal to the sum of all sample-distances
 * plus 0.5. I've taken some liberty to select sequences of shorter number
 * of passes, so the blur-distance may not be exact.
 *
 * The 0.0 value would take 4 samples at the center pixel, so there would
 * be a no blurring on a 0.0 pass.
 */
kawase_entry_t kawase_table[] = {
	{2, {0.0, 0.0}},
	{2, {0.0, 0.5}},
	{2, {0.5, 0.5}},
	{2, {0.5, 1.5}},
	{3, {0.5, 1.5, 1.5}},
	{3, {0.5, 1.5, 2.5}},
	{4, {0.5, 0.5, 1.5, 2.5}},
	{4, {0.5, 1.5, 1.5, 2.5}},
	{4, {0.5, 1.5, 2.5, 2.5}},
	{5, {0.5, 0.5, 1.5, 2.5, 3.5}},
	{5, {0.5, 1.5, 1.5, 2.5, 3.5}},
	{5, {0.5, 1.5, 2.5, 2.5, 3.5}},
	{5, {0.5, 1.5, 2.5, 3.5, 3.5}},
	{5, {0.5, 1.5, 2.5, 3.5, 4.5}},
	{6, {0.5, 0.5, 1.5, 2.5, 3.5, 4.5}},
	{6, {0.5, 1.5, 1.5, 2.5, 3.5, 4.5}},
	{6, {0.5, 1.5, 2.5, 2.5, 3.5, 4.5}},
	{6, {0.5, 1.5, 2.5, 3.5, 3.5, 4.5}},
	{6, {0.5, 1.5, 2.5, 3.5, 4.5, 4.5}},
	{6, {0.5, 1.5, 2.5, 3.5, 4.5, 5.5}},
	{7, {0.5, 1.5, 1.5, 2.5, 3.5, 4.5, 5.5}},
	{7, {0.5, 1.5, 2.5, 2.5, 3.5, 4.5, 5.5}},
	{7, {0.5, 1.5, 2.5, 3.5, 3.5, 4.5, 5.5}},
	{7, {0.5, 1.5, 2.5, 3.5, 4.5, 4.5, 5.5}},
	{7, {0.5, 1.5, 2.5, 3.5, 4.5, 5.5, 5.5}},
	{7, {0.5, 1.5, 2.5, 3.5, 4.5, 5.5, 6.5}}
};

static kawase_entry_t get_kawase_entry(int blur_radius)
{
	if (blur_radius > 25)
		blur_radius = 25;

	return kawase_table[blur_radius];
}

static void drop_shadow_render_blur_pass(
		struct drop_shadow_filter_data *filter, const char *technique,
		gs_texrender_t *shadow_target, gs_texrender_t *shadow_source,
		float kawase_distance)
{
	gs_texrender_reset(shadow_target);
	if (gs_texrender_begin(shadow_target,
			filter->total_width, filter->total_height)) {

		gs_ortho(0, (float)filter->total_width, 0,
				(float)filter->total_height, -1, 1);
		struct vec4 black;
		vec4_zero(&black);
		gs_clear(GS_CLEAR_COLOR | GS_CLEAR_DEPTH, &black, 0, 0);

		set_parameters(filter, shadow_source, kawase_distance);
		set_image_parameter(filter);

		gs_texture_t *output_texture = gs_texrender_get_texture(
				shadow_target);
		while (gs_effect_loop(filter->effect, technique)) {
			gs_draw_sprite(output_texture, 0,
					filter->total_width,
					filter->total_height);
		}

		gs_texrender_end(shadow_target);
	}
}

static void drop_shadow_render(void *data, gs_effect_t *effect)
{
	struct drop_shadow_filter_data *filter = data;

	kawase_entry_t kawase_entry = get_kawase_entry(filter->blur_radius);

	/*
	 * Dynamically allocate and release target textures based
	 * on the ammount of passes that are needed.
	 */
	drop_shadow_alloc_targets(filter, kawase_entry.passes - 1);

	/*
	 * Start the rendering of the filter, first by processing
	 * the rest of the filter chain, filter->context->filter_texrender
	 * will now contain the result of the chain.
	 */
	if (obs_source_process_filter_begin(filter->context, GS_RGBA,
			OBS_NO_DIRECT_RENDERING)) {

		/*
		 * In the first pass we take the alpha value from the "image"
		 * texture received from the source or filters higher up in
		 * the chain. We also shift the pixels and do the first
		 * Kawase blur pass.
		 */
		int pass = 0;
		drop_shadow_render_blur_pass(filter, "FirstPass",
				filter->shadow_target[pass],
				NULL,
				kawase_entry.distances[pass]);

		/*
		 * In following passes we further blur the shadow texture from
		 * the previous blur pass.
		 */
		for (pass = 1; pass < (kawase_entry.passes - 1); pass++) {
			drop_shadow_render_blur_pass(filter, "BlurPass",
					filter->shadow_target[pass],
					filter->shadow_target[pass - 1],
					kawase_entry.distances[pass]);
		}

		/*
		 * The last pass will blur one more time, and then composit the
		 * original image on top of the blurred-shadow.
		 */
		set_parameters(filter, filter->shadow_target[pass - 1], 
				kawase_entry.distances[pass]);
		obs_source_process_filter_end(filter->context, filter->effect,
				filter->total_width, filter->total_height);
	}

	UNUSED_PARAMETER(effect);
}

static void drop_shadow_tick(void *data, float seconds)
{
	struct drop_shadow_filter_data *filter = data;
	obs_source_t *target = obs_filter_get_target(filter->context);

	int base_width = obs_source_get_base_width(target);
	int base_height = obs_source_get_base_height(target);

	filter->total_width = filter->expand_left + base_width +
			filter->expand_right;
	filter->total_height = filter->expand_top + base_height +
			filter->expand_bottom;

	filter->image_uv_scale.x = (float)filter->total_width / base_width;
	filter->image_uv_scale.y = (float)filter->total_height / base_height;
	filter->image_uv_offset.x = (float)(-filter->expand_left) / base_width;
	filter->image_uv_offset.y = (float)(-filter->expand_top) / base_height;
	filter->image_uv_pixel_interval.x = 1.0f / base_width;
	filter->image_uv_pixel_interval.y = 1.0f / base_height;
	filter->shadow_uv_pixel_interval.x = 1.0f / (float)filter->total_width;
	filter->shadow_uv_pixel_interval.y = 1.0f / (float)filter->total_height;

	UNUSED_PARAMETER(seconds);
}

static uint32_t drop_shadow_get_width(void *data)
{
	struct drop_shadow_filter_data *filter = data;

	return filter->total_width;
}

static uint32_t drop_shadow_get_height(void *data)
{
	struct drop_shadow_filter_data *filter = data;

	return filter->total_height;
}

static obs_properties_t *drop_shadow_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	obs_properties_add_color(props, SETTING_SHADOW_COLOR,
			TEXT_SHADOW_COLOR);
	obs_properties_add_int_slider(props, SETTING_OPACITY, TEXT_OPACITY,
			0, 100, 1);

	obs_properties_add_float_slider(props, SETTING_X_OFFSET,
			TEXT_X_OFFSET, -25.0, 25.0, 1.0);
	obs_properties_add_float_slider(props, SETTING_Y_OFFSET,
			TEXT_Y_OFFSET, -25.0, 25.0, 1.0);
	obs_properties_add_int_slider(props, SETTING_BLUR_RADIUS,
			TEXT_BLUR_RADIUS, 0, 25, 1);

	UNUSED_PARAMETER(data);
	return props;
}

static void drop_shadow_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, SETTING_OPACITY, 100);
	obs_data_set_default_double(settings, SETTING_X_OFFSET, 0.0);
	obs_data_set_default_double(settings, SETTING_Y_OFFSET, 0.0);
	obs_data_set_default_int(settings, SETTING_BLUR_RADIUS, 1);
	obs_data_set_default_int(settings, SETTING_SHADOW_COLOR, 0x000000);
}

struct obs_source_info drop_shadow_filter = {
	.id                            = "drop_shadow_filter",
	.type                          = OBS_SOURCE_TYPE_FILTER,
	.output_flags                  = OBS_SOURCE_VIDEO,
	.get_name                      = drop_shadow_name,
	.create                        = drop_shadow_create,
	.destroy                       = drop_shadow_destroy,
	.video_render                  = drop_shadow_render,
	.update                        = drop_shadow_update,
	.video_tick                    = drop_shadow_tick,
	.get_width                     = drop_shadow_get_width,
	.get_height                    = drop_shadow_get_height,
	.get_properties                = drop_shadow_properties,
	.get_defaults                  = drop_shadow_defaults
};
