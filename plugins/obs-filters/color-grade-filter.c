#include <obs-module.h>
#include <graphics/image-file.h>
#include <util/dstr.h>

/* clang-format off */

#define SETTING_IMAGE_PATH             "image_path"
#define SETTING_CLUT_AMOUNT            "clut_amount"

#define TEXT_IMAGE_PATH                obs_module_text("Path")
#define TEXT_AMOUNT                    obs_module_text("Amount")

/* clang-format on */

struct lut_filter_data {
	obs_source_t *context;
	gs_effect_t *effect;
	gs_texture_t *target;
	gs_image_file_t image;

	char *file;
	float clut_amount;
};

static const char *color_grade_filter_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("ColorGradeFilter");
}

static void color_grade_filter_update(void *data, obs_data_t *settings)
{
	struct lut_filter_data *filter = data;

	const char *path = obs_data_get_string(settings, SETTING_IMAGE_PATH);
	double clut_amount = obs_data_get_double(settings, SETTING_CLUT_AMOUNT);

	bfree(filter->file);
	if (path)
		filter->file = bstrdup(path);
	else
		filter->file = NULL;

	obs_enter_graphics();
	gs_image_file_free(&filter->image);
	obs_leave_graphics();

	gs_image_file_init(&filter->image, path);

	obs_enter_graphics();

	gs_image_file_init_texture(&filter->image);

	filter->target = filter->image.texture;
	filter->clut_amount = (float)clut_amount;

	char *effect_path = obs_module_file("color_grade_filter.effect");
	gs_effect_destroy(filter->effect);
	filter->effect = gs_effect_create_from_file(effect_path, NULL);
	bfree(effect_path);

	obs_leave_graphics();
}

static void color_grade_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_double(settings, SETTING_CLUT_AMOUNT, 1);
}

static obs_properties_t *color_grade_filter_properties(void *data)
{
	struct lut_filter_data *s = data;
	struct dstr path = {0};
	const char *slash;

	obs_properties_t *props = obs_properties_create();
	struct dstr filter_str = {0};

	dstr_cat(&filter_str, "(*.png)");

	if (s && s->file && *s->file) {
		dstr_copy(&path, s->file);
	} else {
		char *lut_dir = obs_module_file("LUTs");
		dstr_copy(&path, lut_dir);
		dstr_cat_ch(&path, '/');
		bfree(lut_dir);
	}

	dstr_replace(&path, "\\", "/");
	slash = strrchr(path.array, '/');
	if (slash)
		dstr_resize(&path, slash - path.array + 1);

	obs_properties_add_path(props, SETTING_IMAGE_PATH, TEXT_IMAGE_PATH,
				OBS_PATH_FILE, filter_str.array, path.array);
	obs_properties_add_float_slider(props, SETTING_CLUT_AMOUNT, TEXT_AMOUNT,
					0, 1, 0.01);

	dstr_free(&filter_str);
	dstr_free(&path);

	UNUSED_PARAMETER(data);
	return props;
}

static void *color_grade_filter_create(obs_data_t *settings,
				       obs_source_t *context)
{
	struct lut_filter_data *filter =
		bzalloc(sizeof(struct lut_filter_data));
	filter->context = context;

	obs_source_update(context, settings);
	return filter;
}

static void color_grade_filter_destroy(void *data)
{
	struct lut_filter_data *filter = data;

	obs_enter_graphics();
	gs_effect_destroy(filter->effect);
	gs_image_file_free(&filter->image);
	obs_leave_graphics();

	bfree(filter->file);
	bfree(filter);
}

static void color_grade_filter_render(void *data, gs_effect_t *effect)
{
	struct lut_filter_data *filter = data;
	obs_source_t *target = obs_filter_get_target(filter->context);
	gs_eparam_t *param;

	if (!target || !filter->target || !filter->effect) {
		obs_source_skip_video_filter(filter->context);
		return;
	}

	if (!obs_source_process_filter_begin(filter->context, GS_RGBA_SRGB,
					     OBS_ALLOW_DIRECT_RENDERING))
		return;

	param = gs_effect_get_param_by_name(filter->effect, "clut");
	gs_effect_set_texture(param, filter->target);

	param = gs_effect_get_param_by_name(filter->effect, "clut_amount");
	gs_effect_set_float(param, filter->clut_amount);

	obs_source_process_filter_end(filter->context, filter->effect, 0, 0);

	UNUSED_PARAMETER(effect);
}

struct obs_source_info color_grade_filter = {
	.id = "clut_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = color_grade_filter_get_name,
	.create = color_grade_filter_create,
	.destroy = color_grade_filter_destroy,
	.update = color_grade_filter_update,
	.get_defaults = color_grade_filter_defaults,
	.get_properties = color_grade_filter_properties,
	.video_render = color_grade_filter_render,
};
