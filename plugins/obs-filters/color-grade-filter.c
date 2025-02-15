#include <obs-module.h>
#include <graphics/half.h>
#include <graphics/image-file.h>
#include <util/dstr.h>
#include <util/platform.h>

/* clang-format off */

#define SETTING_SDR_ONLY_INFO          "sdr_only_info"
#define SETTING_IMAGE_PATH             "image_path"
#define SETTING_CLUT_AMOUNT            "clut_amount"
#define SETTING_PASSTHROUGH_ALPHA      "passthrough_alpha"

#define TEXT_SDR_ONLY_INFO             obs_module_text("SdrOnlyInfo")
#define TEXT_IMAGE_PATH                obs_module_text("Path")
#define TEXT_AMOUNT                    obs_module_text("Amount")
#define TEXT_PASSTHROUGH_ALPHA         obs_module_text("PassthroughAlpha")

/* clang-format on */

static const uint32_t LUT_WIDTH = 64;

enum clut_dimension {
	CLUT_1D,
	CLUT_3D,
};

struct lut_filter_data {
	obs_source_t *context;
	gs_effect_t *effect;
	gs_texture_t *target;

	gs_image_file_t image;

	uint32_t cube_width;
	void *cube_data;

	char *file;
	float clut_amount;
	struct vec3 clut_scale;
	struct vec3 clut_offset;
	struct vec3 domain_min;
	struct vec3 domain_max;
	const char *clut_texture_name;
	const char *tech_name;
};

static const char *color_grade_filter_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("ColorGradeFilter");
}

static gs_texture_t *make_clut_texture_png(const enum gs_color_format format, const uint32_t image_width,
					   const uint32_t image_height, const uint8_t *data)
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
				memcpy(cursor, &data[pixel_size * index], pixel_size);

				cursor += pixel_size;
			}
		}
	}

	gs_texture_t *const texture =
		gs_voltexture_create(LUT_WIDTH, LUT_WIDTH, LUT_WIDTH, format, 1, (const uint8_t **)&buffer, 0);
	bfree(buffer);

	return texture;
}

static bool get_cube_entry(FILE *const file, float *const red, float *const green, float *const blue)
{
	bool data_found = false;

	char line[256];
	while (fgets(line, sizeof(line), file)) {
		if (sscanf(line, "%f %f %f", red, green, blue) == 3) {
			data_found = true;
			break;
		}
	}

	return data_found;
}

static void *load_1d_lut(FILE *const file, const uint32_t width, float red, float green, float blue)
{
	const uint32_t data_size = 4 * width * width * width * sizeof(struct half);
	struct half *values = bmalloc(data_size);

	size_t offset = 0;
	bool data_found = true;
	for (uint32_t index = 0; index < width; ++index) {
		if (!data_found) {
			bfree(values);
			values = NULL;
			break;
		}

		values[offset++] = half_from_float(gs_srgb_nonlinear_to_linear(red));
		values[offset++] = half_from_float(gs_srgb_nonlinear_to_linear(green));
		values[offset++] = half_from_float(gs_srgb_nonlinear_to_linear(blue));
		values[offset++] = half_from_bits(0x3c00); // 1.0

		data_found = get_cube_entry(file, &red, &green, &blue);
	}

	return values;
}

static void *load_3d_lut(FILE *const file, const uint32_t width, float red, float green, float blue)
{
	const uint32_t data_size = 4 * width * width * width * sizeof(struct half);
	struct half *values = bmalloc(data_size);

	size_t offset = 0;
	bool data_found = true;
	for (uint32_t z = 0; z < width; ++z) {
		for (uint32_t y = 0; y < width; ++y) {
			for (uint32_t x = 0; x < width; ++x) {
				if (!data_found) {
					bfree(values);
					values = NULL;
					break;
				}

				values[offset++] = half_from_float(gs_srgb_nonlinear_to_linear(red));
				values[offset++] = half_from_float(gs_srgb_nonlinear_to_linear(green));
				values[offset++] = half_from_float(gs_srgb_nonlinear_to_linear(blue));
				values[offset++] = half_from_bits(0x3c00); // 1.0

				data_found = get_cube_entry(file, &red, &green, &blue);
			}
		}
	}

	return values;
}

static void *load_cube_file(const char *const path, uint32_t *const width, struct vec3 *domain_min,
			    struct vec3 *domain_max, enum clut_dimension *dim)
{
	void *data = NULL;

	FILE *const file = os_fopen(path, "rb");
	if (file) {
		float red, green, blue;
		unsigned width_1d = 0;
		unsigned width_3d = 0;

		bool data_found = false;

		char line[256];
		unsigned u;
		float f[3];
		while (fgets(line, sizeof(line), file)) {
			if (sscanf(line, "%f %f %f", &red, &green, &blue) == 3) {
				/* no more metadata */
				data_found = true;
				break;
			} else if (sscanf(line, "DOMAIN_MIN %f %f %f", &f[0], &f[1], &f[2]) == 3) {
				vec3_set(domain_min, f[0], f[1], f[2]);
			} else if (sscanf(line, "DOMAIN_MAX %f %f %f", &f[0], &f[1], &f[2]) == 3) {
				vec3_set(domain_max, f[0], f[1], f[2]);
			} else if (sscanf(line, "LUT_1D_SIZE %u", &u) == 1) {
				width_1d = u;
			} else if (sscanf(line, "LUT_3D_SIZE %u", &u) == 1) {
				width_3d = u;
			}
		}

		if (domain_min->x >= domain_max->x || domain_min->y >= domain_max->y ||
		    domain_min->z >= domain_max->z) {
			blog(LOG_WARNING, "Invalid CUBE LUT domain: [%f, %f], [%f, %f], [%f, %f]", domain_min->x,
			     domain_max->x, domain_min->y, domain_max->y, domain_min->z, domain_max->z);
		} else if (data_found) {
			if (width_1d > 0) {
				data = load_1d_lut(file, width_1d, red, green, blue);
				if (data) {
					*width = width_1d;
					*dim = CLUT_1D;
				}
			} else if (width_3d > 0) {
				data = load_3d_lut(file, width_3d, red, green, blue);
				if (data) {
					*width = width_3d;
					*dim = CLUT_3D;
				}
			}
		}

		fclose(file);
	}

	return data;
}

static void color_grade_filter_update(void *data, obs_data_t *settings)
{
	struct lut_filter_data *filter = data;

	const char *path = obs_data_get_string(settings, SETTING_IMAGE_PATH);
	if (path && (*path == '\0'))
		path = NULL;

	const double clut_amount = obs_data_get_double(settings, SETTING_CLUT_AMOUNT);
	const bool passthrough_alpha = obs_data_get_bool(settings, SETTING_PASSTHROUGH_ALPHA);

	bfree(filter->file);
	if (path)
		filter->file = bstrdup(path);
	else
		filter->file = NULL;

	bfree(filter->cube_data);
	filter->cube_data = NULL;

	obs_enter_graphics();
	gs_image_file_free(&filter->image);
	gs_voltexture_destroy(filter->target);
	filter->target = NULL;
	obs_leave_graphics();

	enum clut_dimension clut_dim = CLUT_3D;
	const char *clut_texture_name = "clut_3d";
	const char *tech_name = "Draw3D";

	if (path) {
		vec3_set(&filter->domain_min, 0.0f, 0.0f, 0.0f);
		vec3_set(&filter->domain_max, 1.0f, 1.0f, 1.0f);

		const char *const ext = os_get_path_extension(path);
		if (ext && astrcmpi(ext, ".cube") == 0) {
			filter->cube_data = load_cube_file(path, &filter->cube_width, &filter->domain_min,
							   &filter->domain_max, &clut_dim);
		} else {
			gs_image_file_init(&filter->image, path);
			filter->cube_width = LUT_WIDTH;
		}

		if (clut_dim == CLUT_1D) {
			clut_texture_name = "clut_1d";
			tech_name = "Draw1D";
		} else if ((filter->domain_min.x > 0.f) || (filter->domain_min.y > 0.f) ||
			   (filter->domain_min.z > 0.f) || (filter->domain_max.x < 1.f) ||
			   (filter->domain_max.y < 1.f) || (filter->domain_max.z < 1.f)) {
			tech_name = "DrawDomain3D";
		} else if (clut_amount < 1.0) {
			tech_name = "DrawAmount3D";
		} else if (!passthrough_alpha) {
			tech_name = "DrawAlpha3D";
		}
	}

	obs_enter_graphics();

	if (path) {
		if (filter->image.loaded) {
			filter->target = make_clut_texture_png(filter->image.format, filter->image.cx, filter->image.cy,
							       filter->image.texture_data);
			const float width_i = 1.0f / (float)LUT_WIDTH;
			const float clut_scale = 1.0f - width_i;
			const float offset = 0.5f * width_i;
			vec3_set(&filter->clut_scale, clut_scale, clut_scale, clut_scale);
			vec3_set(&filter->clut_offset, offset, offset, offset);
		} else if (filter->cube_data) {
			const uint32_t width = filter->cube_width;
			if (clut_dim == CLUT_1D) {
				filter->target = gs_texture_create(width, 1, GS_RGBA16F, 1,
								   (const uint8_t **)&filter->cube_data, 0);
			} else {
				filter->target = gs_voltexture_create(width, width, width, GS_RGBA16F, 1,
								      (const uint8_t **)&filter->cube_data, 0);
			}

			struct vec3 domain_scale;
			vec3_sub(&domain_scale, &filter->domain_max, &filter->domain_min);

			const float width_minus_one = (float)(width - 1);
			vec3_set(&filter->clut_scale, width_minus_one, width_minus_one, width_minus_one);
			vec3_div(&filter->clut_scale, &filter->clut_scale, &domain_scale);

			vec3_neg(&filter->clut_offset, &filter->domain_min);
			vec3_mul(&filter->clut_offset, &filter->clut_offset, &filter->clut_scale);

			/* want normalized UVW */
			vec3_divf(&filter->clut_scale, &filter->clut_scale, (float)width);
			vec3_addf(&filter->clut_offset, &filter->clut_offset, 0.5f);
			vec3_divf(&filter->clut_offset, &filter->clut_offset, (float)width);
		}
	}

	filter->clut_amount = (float)clut_amount;
	filter->clut_texture_name = clut_texture_name;
	filter->tech_name = tech_name;

	char *effect_path = obs_module_file("color_grade_filter.effect");
	gs_effect_destroy(filter->effect);
	filter->effect = gs_effect_create_from_file(effect_path, NULL);
	bfree(effect_path);

	obs_leave_graphics();
}

static void color_grade_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_double(settings, SETTING_CLUT_AMOUNT, 1.0);
	obs_data_set_default_bool(settings, SETTING_PASSTHROUGH_ALPHA, false);
}

static obs_properties_t *color_grade_filter_properties(void *data)
{
	UNUSED_PARAMETER(data);
	struct dstr path = {0};
	const char *slash;

	obs_properties_t *props = obs_properties_create();
	struct dstr filter_str = {0};

	dstr_cat(&filter_str, "PNG/Cube (*.cube *.png)");

	char *lut_dir = obs_module_file("LUTs");
	dstr_copy(&path, lut_dir);
	dstr_cat_ch(&path, '/');
	bfree(lut_dir);

	dstr_replace(&path, "\\", "/");
	slash = strrchr(path.array, '/');
	if (slash)
		dstr_resize(&path, slash - path.array + 1);

	obs_properties_add_text(props, SETTING_SDR_ONLY_INFO, TEXT_SDR_ONLY_INFO, OBS_TEXT_INFO);
	obs_properties_add_path(props, SETTING_IMAGE_PATH, TEXT_IMAGE_PATH, OBS_PATH_FILE, filter_str.array,
				path.array);
	obs_properties_add_float_slider(props, SETTING_CLUT_AMOUNT, TEXT_AMOUNT, 0, 1, 0.0001);
	obs_properties_add_bool(props, SETTING_PASSTHROUGH_ALPHA, TEXT_PASSTHROUGH_ALPHA);

	dstr_free(&filter_str);
	dstr_free(&path);

	return props;
}

static void *color_grade_filter_create(obs_data_t *settings, obs_source_t *context)
{
	struct lut_filter_data *filter = bzalloc(sizeof(struct lut_filter_data));
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

	bfree(filter->cube_data);
	bfree(filter->file);
	bfree(filter);
}

static void color_grade_filter_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);

	struct lut_filter_data *filter = data;
	obs_source_t *target = obs_filter_get_target(filter->context);

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
		const enum gs_color_format format = gs_get_format_from_space(source_space);
		if (obs_source_process_filter_begin_with_color_space(filter->context, format, source_space,
								     OBS_ALLOW_DIRECT_RENDERING)) {
			gs_eparam_t *param = gs_effect_get_param_by_name(filter->effect, filter->clut_texture_name);
			gs_effect_set_texture_srgb(param, filter->target);

			param = gs_effect_get_param_by_name(filter->effect, "clut_amount");
			gs_effect_set_float(param, filter->clut_amount);

			param = gs_effect_get_param_by_name(filter->effect, "clut_scale");
			gs_effect_set_vec3(param, &filter->clut_scale);

			param = gs_effect_get_param_by_name(filter->effect, "clut_offset");
			gs_effect_set_vec3(param, &filter->clut_offset);

			param = gs_effect_get_param_by_name(filter->effect, "domain_min");
			gs_effect_set_vec3(param, &filter->domain_min);

			param = gs_effect_get_param_by_name(filter->effect, "domain_max");
			gs_effect_set_vec3(param, &filter->domain_max);

			gs_blend_state_push();
			gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);

			obs_source_process_filter_tech_end(filter->context, filter->effect, 0, 0, filter->tech_name);

			gs_blend_state_pop();
		}
	}
}

static enum gs_color_space color_grade_filter_get_color_space(void *data, size_t count,
							      const enum gs_color_space *preferred_spaces)
{
	UNUSED_PARAMETER(count);
	UNUSED_PARAMETER(preferred_spaces);

	const enum gs_color_space potential_spaces[] = {
		GS_CS_SRGB,
		GS_CS_SRGB_16F,
		GS_CS_709_EXTENDED,
	};

	struct lut_filter_data *const filter = data;
	const enum gs_color_space source_space = obs_source_get_color_space(
		obs_filter_get_target(filter->context), OBS_COUNTOF(potential_spaces), potential_spaces);

	return source_space;
}

struct obs_source_info color_grade_filter = {
	.id = "clut_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_SRGB,
	.get_name = color_grade_filter_get_name,
	.create = color_grade_filter_create,
	.destroy = color_grade_filter_destroy,
	.update = color_grade_filter_update,
	.get_defaults = color_grade_filter_defaults,
	.get_properties = color_grade_filter_properties,
	.video_render = color_grade_filter_render,
	.video_get_color_space = color_grade_filter_get_color_space,
};
