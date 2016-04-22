#include <obs-module.h>
#include <graphics/vec2.h>
#include <graphics/vec4.h>
#include <graphics/image-file.h>
#include <util/dstr.h>

#define SETTING_TYPE                   "type"
#define SETTING_IMAGE_PATH             "image_path"
#define SETTING_COLOR                  "color"
#define SETTING_OPACITY                "opacity"
#define SETTING_STRETCH                "stretch"

#define TEXT_TYPE                      obs_module_text("Type")
#define TEXT_IMAGE_PATH                obs_module_text("Path")
#define TEXT_COLOR                     obs_module_text("Color")
#define TEXT_OPACITY                   obs_module_text("Opacity")
#define TEXT_STRETCH                   obs_module_text("StretchImage")
#define TEXT_PATH_IMAGES               obs_module_text("BrowsePath.Images")
#define TEXT_PATH_ALL_FILES            obs_module_text("BrowsePath.AllFiles")

struct mask_filter_data {
	uint64_t                       last_time;

	obs_source_t                   *context;
	gs_effect_t                    *effect;

	gs_texture_t                   *target;
	gs_image_file_t                image;
	struct vec4                    color;
	bool                           lock_aspect;
};

static const char *mask_filter_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("MaskFilter");
}

static void mask_filter_update(void *data, obs_data_t *settings)
{
	struct mask_filter_data *filter = data;

	const char *path = obs_data_get_string(settings, SETTING_IMAGE_PATH);
	const char *effect_file = obs_data_get_string(settings, SETTING_TYPE);
	uint32_t color = (uint32_t)obs_data_get_int(settings, SETTING_COLOR);
	int opacity = (int)obs_data_get_int(settings, SETTING_OPACITY);
	char *effect_path;

	color |= (uint32_t)(((double)opacity) * 2.55) << 24;

	vec4_from_rgba(&filter->color, color);

	obs_enter_graphics();
	gs_image_file_free(&filter->image);
	obs_leave_graphics();

	gs_image_file_init(&filter->image, path);

	obs_enter_graphics();

	gs_image_file_init_texture(&filter->image);

	filter->target = filter->image.texture;
	filter->lock_aspect = !obs_data_get_bool(settings, SETTING_STRETCH);

	effect_path = obs_module_file(effect_file);
	gs_effect_destroy(filter->effect);
	filter->effect = gs_effect_create_from_file(effect_path, NULL);
	bfree(effect_path);

	obs_leave_graphics();
}

static void mask_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_string(settings, SETTING_TYPE,
			"mask_color_filter.effect");
	obs_data_set_default_int(settings, SETTING_COLOR, 0xFFFFFF);
	obs_data_set_default_int(settings, SETTING_OPACITY, 100);
}

#define IMAGE_FILTER_EXTENSIONS \
	" (*.bmp *.jpg *.jpeg *.tga *.gif *.png)"

static obs_properties_t *mask_filter_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();
	struct dstr filter_str = {0};
	obs_property_t *p;

	dstr_copy(&filter_str, TEXT_PATH_IMAGES);
	dstr_cat(&filter_str, IMAGE_FILTER_EXTENSIONS ";;");
	dstr_cat(&filter_str, TEXT_PATH_ALL_FILES);
	dstr_cat(&filter_str, " (*.*)");

	p = obs_properties_add_list(props, SETTING_TYPE, TEXT_TYPE,
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	obs_property_list_add_string(p,
			obs_module_text("MaskBlendType.MaskColor"),
			"mask_color_filter.effect");
	obs_property_list_add_string(p,
			obs_module_text("MaskBlendType.MaskAlpha"),
			"mask_alpha_filter.effect");
	obs_property_list_add_string(p,
			obs_module_text("MaskBlendType.BlendMultiply"),
			"blend_mul_filter.effect");
	obs_property_list_add_string(p,
			obs_module_text("MaskBlendType.BlendAddition"),
			"blend_add_filter.effect");
	obs_property_list_add_string(p,
			obs_module_text("MaskBlendType.BlendSubtraction"),
			"blend_sub_filter.effect");

	obs_properties_add_path(props, SETTING_IMAGE_PATH, TEXT_IMAGE_PATH,
			OBS_PATH_FILE, filter_str.array, NULL);
	obs_properties_add_color(props, SETTING_COLOR, TEXT_COLOR);
	obs_properties_add_int(props, SETTING_OPACITY, TEXT_OPACITY, 0, 100, 1);
	obs_properties_add_bool(props, SETTING_STRETCH, TEXT_STRETCH);

	dstr_free(&filter_str);

	UNUSED_PARAMETER(data);
	return props;
}

static void *mask_filter_create(obs_data_t *settings, obs_source_t *context)
{
	struct mask_filter_data *filter =
		bzalloc(sizeof(struct mask_filter_data));
	filter->context = context;

	obs_source_update(context, settings);
	return filter;
}

static void mask_filter_destroy(void *data)
{
	struct mask_filter_data *filter = data;

	obs_enter_graphics();
	gs_effect_destroy(filter->effect);
	gs_image_file_free(&filter->image);
	obs_leave_graphics();

	bfree(filter);
}

static void mask_filter_tick(void *data, float t)
{
	struct mask_filter_data *filter = data;
	UNUSED_PARAMETER(t);

	if (filter->image.is_animated_gif) {
		uint64_t cur_time = obs_get_video_frame_time();

		if (!filter->last_time)
			filter->last_time = cur_time;

		gs_image_file_tick(&filter->image, cur_time - filter->last_time);
		obs_enter_graphics();
		gs_image_file_update_texture(&filter->image);
		obs_leave_graphics();

		filter->last_time = cur_time;
	}
}

static void mask_filter_render(void *data, gs_effect_t *effect)
{
	struct mask_filter_data *filter = data;
	obs_source_t *target = obs_filter_get_target(filter->context);
	gs_eparam_t *param;
	struct vec2 add_val = {0};
	struct vec2 mul_val = {1.0f, 1.0f};

	if (!target || !filter->target || !filter->effect) {
		obs_source_skip_video_filter(filter->context);
		return;
	}

	if (filter->lock_aspect) {
		struct vec2 source_size;
		struct vec2 mask_size;
		struct vec2 mask_temp;
		float source_aspect;
		float mask_aspect;
		bool size_to_x;
		float fix;

		source_size.x = (float)obs_source_get_base_width(target);
		source_size.y = (float)obs_source_get_base_height(target);
		mask_size.x = (float)gs_texture_get_width(filter->target);
		mask_size.y = (float)gs_texture_get_height(filter->target);

		source_aspect = source_size.x / source_size.y;
		mask_aspect = mask_size.x / mask_size.y;
		size_to_x = (source_aspect < mask_aspect);

		fix = size_to_x ?
			(source_size.x / mask_size.x) :
			(source_size.y / mask_size.y);

		vec2_mulf(&mask_size, &mask_size, fix);
		vec2_div(&mul_val, &source_size, &mask_size);
		vec2_mulf(&source_size, &source_size, 0.5f);
		vec2_mulf(&mask_temp, &mask_size, 0.5f);
		vec2_sub(&add_val, &source_size, &mask_temp);
		vec2_neg(&add_val, &add_val);
		vec2_div(&add_val, &add_val, &mask_size);
	}

	if (!obs_source_process_filter_begin(filter->context, GS_RGBA,
				OBS_ALLOW_DIRECT_RENDERING))
		return;

	param = gs_effect_get_param_by_name(filter->effect, "target");
	gs_effect_set_texture(param, filter->target);

	param = gs_effect_get_param_by_name(filter->effect, "color");
	gs_effect_set_vec4(param, &filter->color);

	param = gs_effect_get_param_by_name(filter->effect, "mul_val");
	gs_effect_set_vec2(param, &mul_val);

	param = gs_effect_get_param_by_name(filter->effect, "add_val");
	gs_effect_set_vec2(param, &add_val);

	obs_source_process_filter_end(filter->context, filter->effect, 0, 0);

	UNUSED_PARAMETER(effect);
}

struct obs_source_info mask_filter = {
	.id                            = "mask_filter",
	.type                          = OBS_SOURCE_TYPE_FILTER,
	.output_flags                  = OBS_SOURCE_VIDEO,
	.get_name                      = mask_filter_get_name,
	.create                        = mask_filter_create,
	.destroy                       = mask_filter_destroy,
	.update                        = mask_filter_update,
	.get_defaults                  = mask_filter_defaults,
	.get_properties                = mask_filter_properties,
	.video_tick                    = mask_filter_tick,
	.video_render                  = mask_filter_render
};
