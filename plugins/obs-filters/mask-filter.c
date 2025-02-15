#include <obs-module.h>
#include <graphics/vec2.h>
#include <graphics/vec4.h>
#include <graphics/image-file.h>
#include <util/platform.h>
#include <util/dstr.h>
#include <sys/stat.h>

/* clang-format off */

#define SETTING_SDR_ONLY_INFO          "sdr_only_info"
#define SETTING_TYPE                   "type"
#define SETTING_IMAGE_PATH             "image_path"
#define SETTING_COLOR                  "color"
#define SETTING_OPACITY                "opacity"
#define SETTING_STRETCH                "stretch"

#define TEXT_SDR_ONLY_INFO             obs_module_text("SdrOnlyInfo")
#define TEXT_TYPE                      obs_module_text("Type")
#define TEXT_IMAGE_PATH                obs_module_text("Path")
#define TEXT_COLOR                     obs_module_text("Color")
#define TEXT_OPACITY                   obs_module_text("Opacity")
#define TEXT_STRETCH                   obs_module_text("StretchImage")
#define TEXT_PATH_IMAGES               obs_module_text("BrowsePath.Images")
#define TEXT_PATH_ALL_FILES            obs_module_text("BrowsePath.AllFiles")

/* clang-format on */

struct mask_filter_data {
	uint64_t last_time;

	obs_source_t *context;
	gs_effect_t *effect;

	char *image_file;
	time_t image_file_timestamp;
	float update_time_elapsed;

	gs_texture_t *target;
	gs_image_file_t image;
	struct vec4 color;
	bool lock_aspect;
};

static time_t get_modified_timestamp(const char *filename)
{
	struct stat stats;
	if (os_stat(filename, &stats) != 0)
		return -1;
	return stats.st_mtime;
}

static const char *mask_filter_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("MaskFilter");
}

static void mask_filter_image_unload(struct mask_filter_data *filter)
{
	obs_enter_graphics();
	gs_image_file_free(&filter->image);
	obs_leave_graphics();
}

static void mask_filter_image_load(struct mask_filter_data *filter)
{
	mask_filter_image_unload(filter);

	char *path = filter->image_file;

	if (path && *path) {
		filter->image_file_timestamp = get_modified_timestamp(path);
		gs_image_file_init(&filter->image, path);
		filter->update_time_elapsed = 0;

		obs_enter_graphics();
		gs_image_file_init_texture(&filter->image);
		obs_leave_graphics();
	}

	filter->target = filter->image.texture;
}

static void mask_filter_update_internal(void *data, obs_data_t *settings, float opacity, bool srgb)
{
	struct mask_filter_data *filter = data;

	const char *path = obs_data_get_string(settings, SETTING_IMAGE_PATH);
	const char *effect_file = obs_data_get_string(settings, SETTING_TYPE);
	uint32_t color = (uint32_t)obs_data_get_int(settings, SETTING_COLOR);
	char *effect_path;

	if (filter->image_file)
		bfree(filter->image_file);
	filter->image_file = bstrdup(path);

	if (srgb)
		vec4_from_rgba_srgb(&filter->color, color);
	else
		vec4_from_rgba(&filter->color, color);
	filter->color.w = opacity;

	mask_filter_image_load(filter);
	filter->lock_aspect = !obs_data_get_bool(settings, SETTING_STRETCH);

	obs_enter_graphics();

	effect_path = obs_module_file(effect_file);
	gs_effect_destroy(filter->effect);
	filter->effect = gs_effect_create_from_file(effect_path, NULL);
	bfree(effect_path);

	obs_leave_graphics();
}

static void mask_filter_update_v1(void *data, obs_data_t *settings)
{
	const float opacity = (float)(obs_data_get_int(settings, SETTING_OPACITY) * 0.01);
	mask_filter_update_internal(data, settings, opacity, false);
}

static void mask_filter_update_v2(void *data, obs_data_t *settings)
{
	const float opacity = (float)obs_data_get_double(settings, SETTING_OPACITY);
	mask_filter_update_internal(data, settings, opacity, true);
}

static void mask_filter_defaults_v1(obs_data_t *settings)
{
	obs_data_set_default_string(settings, SETTING_TYPE, "mask_color_filter.effect");
	obs_data_set_default_int(settings, SETTING_COLOR, 0xFFFFFF);
	obs_data_set_default_int(settings, SETTING_OPACITY, 100);
}

static void mask_filter_defaults_v2(obs_data_t *settings)
{
	obs_data_set_default_string(settings, SETTING_TYPE, "mask_color_filter.effect");
	obs_data_set_default_int(settings, SETTING_COLOR, 0xFFFFFF);
	obs_data_set_default_double(settings, SETTING_OPACITY, 1.0);
}

#define IMAGE_FILTER_EXTENSIONS " (*.bmp *.jpg *.jpeg *.tga *.gif *.png)"

static obs_properties_t *mask_filter_properties_internal(bool use_float_opacity)
{
	obs_properties_t *props = obs_properties_create();
	struct dstr filter_str = {0};
	obs_property_t *p;

	dstr_copy(&filter_str, TEXT_PATH_IMAGES);
	dstr_cat(&filter_str, IMAGE_FILTER_EXTENSIONS ";;");
	dstr_cat(&filter_str, TEXT_PATH_ALL_FILES);
	dstr_cat(&filter_str, " (*.*)");

	obs_properties_add_text(props, SETTING_SDR_ONLY_INFO, TEXT_SDR_ONLY_INFO, OBS_TEXT_INFO);

	p = obs_properties_add_list(props, SETTING_TYPE, TEXT_TYPE, OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	obs_property_list_add_string(p, obs_module_text("MaskBlendType.MaskColor"), "mask_color_filter.effect");
	obs_property_list_add_string(p, obs_module_text("MaskBlendType.MaskAlpha"), "mask_alpha_filter.effect");
	obs_property_list_add_string(p, obs_module_text("MaskBlendType.BlendMultiply"), "blend_mul_filter.effect");
	obs_property_list_add_string(p, obs_module_text("MaskBlendType.BlendAddition"), "blend_add_filter.effect");
	obs_property_list_add_string(p, obs_module_text("MaskBlendType.BlendSubtraction"), "blend_sub_filter.effect");

	obs_properties_add_path(props, SETTING_IMAGE_PATH, TEXT_IMAGE_PATH, OBS_PATH_FILE, filter_str.array, NULL);
	obs_properties_add_color(props, SETTING_COLOR, TEXT_COLOR);
	if (use_float_opacity) {
		obs_properties_add_float_slider(props, SETTING_OPACITY, TEXT_OPACITY, 0.0, 1.0, 0.0001);
	} else {
		obs_properties_add_int_slider(props, SETTING_OPACITY, TEXT_OPACITY, 0, 100, 1);
	}
	obs_properties_add_bool(props, SETTING_STRETCH, TEXT_STRETCH);

	dstr_free(&filter_str);

	return props;
}

static obs_properties_t *mask_filter_properties_v1(void *data)
{
	UNUSED_PARAMETER(data);

	return mask_filter_properties_internal(false);
}

static obs_properties_t *mask_filter_properties_v2(void *data)
{
	UNUSED_PARAMETER(data);

	return mask_filter_properties_internal(true);
}

static void *mask_filter_create(obs_data_t *settings, obs_source_t *context)
{
	struct mask_filter_data *filter = bzalloc(sizeof(struct mask_filter_data));
	filter->context = context;

	obs_source_update(context, settings);
	return filter;
}

static void mask_filter_destroy(void *data)
{
	struct mask_filter_data *filter = data;

	if (filter->image_file)
		bfree(filter->image_file);

	obs_enter_graphics();
	gs_effect_destroy(filter->effect);
	gs_image_file_free(&filter->image);
	obs_leave_graphics();

	bfree(filter);
}

static void mask_filter_tick(void *data, float seconds)
{
	struct mask_filter_data *filter = data;
	filter->update_time_elapsed += seconds;

	if (filter->update_time_elapsed >= 1.0f) {
		time_t t = get_modified_timestamp(filter->image_file);
		filter->update_time_elapsed = 0.0f;

		if (filter->image_file_timestamp != t) {
			mask_filter_image_load(filter);
		}
	}

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
	UNUSED_PARAMETER(effect);

	struct mask_filter_data *filter = data;
	obs_source_t *target = obs_filter_get_target(filter->context);
	gs_eparam_t *param;
	struct vec2 add_val = {0};
	struct vec2 mul_val = {1.0f, 1.0f};

	if (!target || !filter->target || !filter->effect) {
		obs_source_skip_video_filter(filter->context);
		return;
	}

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

			fix = size_to_x ? (source_size.x / mask_size.x) : (source_size.y / mask_size.y);

			vec2_mulf(&mask_size, &mask_size, fix);
			vec2_div(&mul_val, &source_size, &mask_size);
			vec2_mulf(&source_size, &source_size, 0.5f);
			vec2_mulf(&mask_temp, &mask_size, 0.5f);
			vec2_sub(&add_val, &source_size, &mask_temp);
			vec2_neg(&add_val, &add_val);
			vec2_div(&add_val, &add_val, &mask_size);
		}

		const enum gs_color_format format = gs_get_format_from_space(source_space);
		if (obs_source_process_filter_begin_with_color_space(filter->context, format, source_space,
								     OBS_ALLOW_DIRECT_RENDERING)) {
			param = gs_effect_get_param_by_name(filter->effect, "target");
			gs_effect_set_texture_srgb(param, filter->target);

			param = gs_effect_get_param_by_name(filter->effect, "color");
			gs_effect_set_vec4(param, &filter->color);

			param = gs_effect_get_param_by_name(filter->effect, "mul_val");
			gs_effect_set_vec2(param, &mul_val);

			param = gs_effect_get_param_by_name(filter->effect, "add_val");
			gs_effect_set_vec2(param, &add_val);

			gs_blend_state_push();
			gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);

			obs_source_process_filter_end(filter->context, filter->effect, 0, 0);

			gs_blend_state_pop();
		}
	}
}

static enum gs_color_space mask_filter_get_color_space(void *data, size_t count,
						       const enum gs_color_space *preferred_spaces)
{
	UNUSED_PARAMETER(count);
	UNUSED_PARAMETER(preferred_spaces);

	const enum gs_color_space potential_spaces[] = {
		GS_CS_SRGB,
		GS_CS_SRGB_16F,
		GS_CS_709_EXTENDED,
	};

	struct mask_filter_data *const filter = data;
	const enum gs_color_space source_space = obs_source_get_color_space(
		obs_filter_get_target(filter->context), OBS_COUNTOF(potential_spaces), potential_spaces);

	return source_space;
}

struct obs_source_info mask_filter = {
	.id = "mask_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CAP_OBSOLETE,
	.get_name = mask_filter_get_name,
	.create = mask_filter_create,
	.destroy = mask_filter_destroy,
	.update = mask_filter_update_v1,
	.get_defaults = mask_filter_defaults_v1,
	.get_properties = mask_filter_properties_v1,
	.video_tick = mask_filter_tick,
	.video_render = mask_filter_render,
};

struct obs_source_info mask_filter_v2 = {
	.id = "mask_filter",
	.version = 2,
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_SRGB,
	.get_name = mask_filter_get_name,
	.create = mask_filter_create,
	.destroy = mask_filter_destroy,
	.update = mask_filter_update_v2,
	.get_defaults = mask_filter_defaults_v2,
	.get_properties = mask_filter_properties_v2,
	.video_tick = mask_filter_tick,
	.video_render = mask_filter_render,
	.video_get_color_space = mask_filter_get_color_space,
};
