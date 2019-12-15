#include <obs-module.h>
#include <graphics/image-file.h>
#include <util/dstr.h>

/* clang-format off */

#define SETTING_IMAGE_PATH             "image_path"
#define SETTING_CLUT_AMOUNT            "clut_amount"

#define TEXT_IMAGE_PATH                obs_module_text("Path")
#define TEXT_AMOUNT                    obs_module_text("Amount")

/* clang-format on */

static const uint32_t LUT_WIDTH = 64;

struct lut_filter_data {
	obs_source_t *context;
	gs_effect_t *effect;
	gs_texture_t *target;
	gs_image_file_t image;

	char *file;
	float clut_amount;
	float clut_scale;
	float clut_offset;
};

static const char *color_grade_filter_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("ColorGradeFilter");
}

static gs_texture_t *make_clut_texture(const enum gs_color_format format,
				       const uint32_t image_width,
				       const uint32_t image_height,
				       const uint8_t *data)
{
	if (image_width % LUT_WIDTH != 0)
		return NULL;

	if (image_height % LUT_WIDTH != 0)
		return NULL;

	const uint32_t pixel_count = LUT_WIDTH * LUT_WIDTH * LUT_WIDTH;
	if ((image_width * image_height) != pixel_count)
		return NULL;

	const uint32_t bpp = gs_get_format_bpp(format);
	if (bpp % 8 != 0)
		return NULL;

	const uint32_t pixel_size = bpp / 8;
	const uint32_t buffer_size = pixel_size * pixel_count;
	uint8_t *const buffer = bmalloc(buffer_size);
	const uint32_t macro_width = image_width / LUT_WIDTH;
	const uint32_t macro_height = image_height / LUT_WIDTH;
	uint8_t *cursor = buffer;
	for (uint32_t z = 0; z < LUT_WIDTH; ++z) {
		const int z_x = (z % macro_width) * LUT_WIDTH;
		const int z_y = (z / macro_height) * LUT_WIDTH;
		for (uint32_t y = 0; y < LUT_WIDTH; ++y) {
			const uint32_t row_index = image_width * (z_y + y);
			for (uint32_t x = 0; x < LUT_WIDTH; ++x) {
				const uint32_t index = row_index + z_x + x;
				memcpy(cursor, &data[pixel_size * index],
				       pixel_size);

				cursor += pixel_size;
			}
		}
	}

	gs_texture_t *const texture =
		gs_voltexture_create(LUT_WIDTH, LUT_WIDTH, LUT_WIDTH, format, 1,
				     (const uint8_t **)&buffer, 0);
	bfree(buffer);

	return texture;
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

	gs_voltexture_destroy(filter->target);
	if (filter->image.loaded) {
		filter->target = make_clut_texture(filter->image.format,
						   filter->image.cx,
						   filter->image.cy,
						   filter->image.texture_data);
	}

	filter->clut_amount = (float)clut_amount;
	filter->clut_scale = (float)(LUT_WIDTH - 1) / (float)LUT_WIDTH;
	filter->clut_offset = 0.5f / (float)LUT_WIDTH;

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
	gs_voltexture_destroy(filter->target);
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

	if (!obs_source_process_filter_begin(filter->context, GS_RGBA,
					     OBS_ALLOW_DIRECT_RENDERING))
		return;

	param = gs_effect_get_param_by_name(filter->effect, "clut");
	gs_effect_set_texture(param, filter->target);

	param = gs_effect_get_param_by_name(filter->effect, "clut_amount");
	gs_effect_set_float(param, filter->clut_amount);

	param = gs_effect_get_param_by_name(filter->effect, "clut_scale");
	gs_effect_set_float(param, filter->clut_scale);

	param = gs_effect_get_param_by_name(filter->effect, "clut_offset");
	gs_effect_set_float(param, filter->clut_offset);

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
