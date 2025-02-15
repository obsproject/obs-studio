#include <stdio.h>
#include <stdlib.h>
#include <util/dstr.h>
#include <obs-module.h>
#include <util/platform.h>
#include <graphics/vec2.h>
#include <graphics/math-defs.h>

/* clang-format off */

#define S_RESOLUTION                    "resolution"
#define S_SAMPLING                      "sampling"
#define S_UNDISTORT                     "undistort"

#define T_RESOLUTION                    obs_module_text("Resolution")
#define T_NONE                          obs_module_text("None")
#define T_SAMPLING                      obs_module_text("ScaleFiltering")
#define T_SAMPLING_POINT                obs_module_text("ScaleFiltering.Point")
#define T_SAMPLING_BILINEAR             obs_module_text("ScaleFiltering.Bilinear")
#define T_SAMPLING_BICUBIC              obs_module_text("ScaleFiltering.Bicubic")
#define T_SAMPLING_LANCZOS              obs_module_text("ScaleFiltering.Lanczos")
#define T_SAMPLING_AREA                 obs_module_text("ScaleFiltering.Area")
#define T_UNDISTORT                     obs_module_text("UndistortCenter")
#define T_BASE                          obs_module_text("Base.Canvas")

#define S_SAMPLING_POINT                "point"
#define S_SAMPLING_BILINEAR             "bilinear"
#define S_SAMPLING_BICUBIC              "bicubic"
#define S_SAMPLING_LANCZOS              "lanczos"
#define S_SAMPLING_AREA                 "area"

/* clang-format on */

struct scale_filter_data {
	obs_source_t *context;
	gs_effect_t *effect;
	gs_eparam_t *image_param;
	gs_eparam_t *dimension_param;
	gs_eparam_t *dimension_i_param;
	gs_eparam_t *undistort_factor_param;
	gs_eparam_t *multiplier_param;
	struct vec2 dimension;
	struct vec2 dimension_i;
	double undistort_factor;
	int cx_in;
	int cy_in;
	int cx_out;
	int cy_out;
	enum obs_scale_type sampling;
	gs_samplerstate_t *point_sampler;
	bool aspect_ratio_only;
	bool target_valid;
	bool valid;
	bool can_undistort;
	bool undistort;
	bool upscale;
	bool base_canvas_resolution;
};

static const char *scale_filter_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("ScaleFilter");
}

static void scale_filter_update(void *data, obs_data_t *settings)
{
	struct scale_filter_data *filter = data;
	int ret;

	const char *res_str = obs_data_get_string(settings, S_RESOLUTION);
	const char *sampling = obs_data_get_string(settings, S_SAMPLING);

	filter->valid = true;
	filter->base_canvas_resolution = false;

	if (strcmp(res_str, T_BASE) == 0) {
		struct obs_video_info ovi;
		obs_get_video_info(&ovi);
		filter->aspect_ratio_only = false;
		filter->base_canvas_resolution = true;
		filter->cx_in = ovi.base_width;
		filter->cy_in = ovi.base_height;
	} else {
		ret = sscanf(res_str, "%dx%d", &filter->cx_in, &filter->cy_in);
		if (ret == 2) {
			filter->aspect_ratio_only = false;
		} else {
			ret = sscanf(res_str, "%d:%d", &filter->cx_in, &filter->cy_in);
			if (ret != 2) {
				filter->valid = false;
				return;
			}

			filter->aspect_ratio_only = true;
		}
	}

	if (astrcmpi(sampling, S_SAMPLING_POINT) == 0) {
		filter->sampling = OBS_SCALE_POINT;

	} else if (astrcmpi(sampling, S_SAMPLING_BILINEAR) == 0) {
		filter->sampling = OBS_SCALE_BILINEAR;

	} else if (astrcmpi(sampling, S_SAMPLING_LANCZOS) == 0) {
		filter->sampling = OBS_SCALE_LANCZOS;

	} else if (astrcmpi(sampling, S_SAMPLING_AREA) == 0) {
		filter->sampling = OBS_SCALE_AREA;

	} else { /* S_SAMPLING_BICUBIC */
		filter->sampling = OBS_SCALE_BICUBIC;
	}

	filter->can_undistort = obs_data_get_bool(settings, S_UNDISTORT);
}

static void scale_filter_destroy(void *data)
{
	struct scale_filter_data *filter = data;

	obs_enter_graphics();
	gs_samplerstate_destroy(filter->point_sampler);
	obs_leave_graphics();
	bfree(data);
}

static void *scale_filter_create(obs_data_t *settings, obs_source_t *context)
{
	struct scale_filter_data *filter = bzalloc(sizeof(struct scale_filter_data));
	struct gs_sampler_info sampler_info = {0};

	filter->context = context;

	obs_enter_graphics();
	filter->point_sampler = gs_samplerstate_create(&sampler_info);
	obs_leave_graphics();

	scale_filter_update(filter, settings);
	return filter;
}

static void scale_filter_tick(void *data, float seconds)
{
	struct scale_filter_data *filter = data;
	enum obs_base_effect type;
	obs_source_t *target;
	bool lower_than_2x;
	double cx_f;
	double cy_f;
	int cx;
	int cy;

	if (filter->base_canvas_resolution) {
		struct obs_video_info ovi;
		obs_get_video_info(&ovi);
		filter->cx_in = ovi.base_width;
		filter->cy_in = ovi.base_height;
	}

	target = obs_filter_get_target(filter->context);
	filter->cx_out = 0;
	filter->cy_out = 0;

	filter->target_valid = !!target;
	if (!filter->target_valid)
		return;

	cx = obs_source_get_base_width(target);
	cy = obs_source_get_base_height(target);

	if (!cx || !cy) {
		filter->target_valid = false;
		return;
	}

	filter->cx_out = cx;
	filter->cy_out = cy;

	if (!filter->valid)
		return;

	/* ------------------------- */

	cx_f = (double)cx;
	cy_f = (double)cy;

	double old_aspect = cx_f / cy_f;
	double new_aspect = (double)filter->cx_in / (double)filter->cy_in;

	if (filter->aspect_ratio_only) {
		if (fabs(old_aspect - new_aspect) <= EPSILON) {
			filter->target_valid = false;
			return;
		} else {
			if (new_aspect > old_aspect) {
				filter->cx_out = (int)(cy_f * new_aspect);
			} else {
				filter->cy_out = (int)(cx_f / new_aspect);
			}
		}
	} else {
		filter->cx_out = filter->cx_in;
		filter->cy_out = filter->cy_in;
	}

	vec2_set(&filter->dimension, (float)cx, (float)cy);
	vec2_set(&filter->dimension_i, 1.0f / (float)cx, 1.0f / (float)cy);

	filter->undistort = false;
	filter->upscale = false;

	/* ------------------------- */

	lower_than_2x = filter->cx_out < cx / 2 || filter->cy_out < cy / 2;

	if (lower_than_2x && filter->sampling != OBS_SCALE_POINT) {
		type = OBS_EFFECT_BILINEAR_LOWRES;
	} else {
		switch (filter->sampling) {
		default:
		case OBS_SCALE_POINT:
		case OBS_SCALE_BILINEAR:
			type = OBS_EFFECT_DEFAULT;
			break;
		case OBS_SCALE_BICUBIC:
			type = OBS_EFFECT_BICUBIC;
			filter->undistort = filter->can_undistort;
			break;
		case OBS_SCALE_LANCZOS:
			type = OBS_EFFECT_LANCZOS;
			filter->undistort = filter->can_undistort;
			break;
		case OBS_SCALE_AREA:
			type = OBS_EFFECT_AREA;
			if ((filter->cx_out >= cx) && (filter->cy_out >= cy))
				filter->upscale = true;
			break;
		}
	}

	filter->undistort_factor = filter->undistort ? (new_aspect / old_aspect) : 1.0;

	filter->effect = obs_get_base_effect(type);
	filter->image_param = gs_effect_get_param_by_name(filter->effect, "image");

	if (type != OBS_EFFECT_DEFAULT) {
		filter->dimension_param = gs_effect_get_param_by_name(filter->effect, "base_dimension");
		filter->dimension_i_param = gs_effect_get_param_by_name(filter->effect, "base_dimension_i");
	} else {
		filter->dimension_param = NULL;
		filter->dimension_i_param = NULL;
	}

	if (type == OBS_EFFECT_BICUBIC || type == OBS_EFFECT_LANCZOS) {
		filter->undistort_factor_param = gs_effect_get_param_by_name(filter->effect, "undistort_factor");
	} else {
		filter->undistort_factor_param = NULL;
	}

	filter->multiplier_param = gs_effect_get_param_by_name(filter->effect, "multiplier");

	UNUSED_PARAMETER(seconds);
}

static const char *get_tech_name_and_multiplier(const struct scale_filter_data *filter,
						enum gs_color_space current_space, enum gs_color_space source_space,
						float *multiplier)
{
	*multiplier = 1.f;
	switch (source_space) {
	case GS_CS_SRGB:
	case GS_CS_SRGB_16F:
	case GS_CS_709_EXTENDED:
		if (current_space == GS_CS_709_SCRGB)
			*multiplier = obs_get_video_sdr_white_level() / 80.f;
		break;
	case GS_CS_709_SCRGB:
		switch (current_space) {
		case GS_CS_SRGB:
		case GS_CS_SRGB_16F:
		case GS_CS_709_EXTENDED:
			*multiplier = 80.f / obs_get_video_sdr_white_level();
			break;
		case GS_CS_709_SCRGB:
			break;
		}
	}

	const char *tech_name = "Draw";
	if (filter->undistort) {
		tech_name = "DrawUndistort";
		switch (source_space) {
		case GS_CS_SRGB:
		case GS_CS_SRGB_16F:
			if (current_space == GS_CS_709_SCRGB)
				tech_name = "DrawUndistortMultiply";
			break;
		case GS_CS_709_EXTENDED:
			switch (current_space) {
			case GS_CS_SRGB:
			case GS_CS_SRGB_16F:
				tech_name = "DrawUndistortTonemap";
				break;
			case GS_CS_709_SCRGB:
				tech_name = "DrawUndistortMultiply";
				break;
			case GS_CS_709_EXTENDED:
				break;
			}
			break;
		case GS_CS_709_SCRGB:
			switch (current_space) {
			case GS_CS_SRGB:
			case GS_CS_SRGB_16F:
				tech_name = "DrawUndistortMultiplyTonemap";
				break;
			case GS_CS_709_EXTENDED:
				tech_name = "DrawUndistortMultiply";
				break;
			case GS_CS_709_SCRGB:
				break;
			}
		}
	} else if (filter->upscale) {
		tech_name = "DrawUpscale";
		switch (source_space) {
		case GS_CS_SRGB:
		case GS_CS_SRGB_16F:
			if (current_space == GS_CS_709_SCRGB)
				tech_name = "DrawUpscaleMultiply";
			break;
		case GS_CS_709_EXTENDED:
			switch (current_space) {
			case GS_CS_SRGB:
			case GS_CS_SRGB_16F:
				tech_name = "DrawUpscaleTonemap";
				break;
			case GS_CS_709_SCRGB:
				tech_name = "DrawUpscaleMultiply";
				break;
			case GS_CS_709_EXTENDED:
				break;
			}
			break;
		case GS_CS_709_SCRGB:
			switch (current_space) {
			case GS_CS_SRGB:
			case GS_CS_SRGB_16F:
				tech_name = "DrawUpscaleMultiplyTonemap";
				break;
			case GS_CS_709_EXTENDED:
				tech_name = "DrawUpscaleMultiply";
				break;
			case GS_CS_709_SCRGB:
				break;
			}
		}
	} else {
		switch (source_space) {
		case GS_CS_SRGB:
		case GS_CS_SRGB_16F:
			if (current_space == GS_CS_709_SCRGB)
				tech_name = "DrawMultiply";
			break;
		case GS_CS_709_EXTENDED:
			switch (current_space) {
			case GS_CS_SRGB:
			case GS_CS_SRGB_16F:
				tech_name = "DrawTonemap";
				break;
			case GS_CS_709_SCRGB:
				tech_name = "DrawMultiply";
				break;
			case GS_CS_709_EXTENDED:
				break;
			}
			break;
		case GS_CS_709_SCRGB:
			switch (current_space) {
			case GS_CS_SRGB:
			case GS_CS_SRGB_16F:
				tech_name = "DrawMultiplyTonemap";
				break;
			case GS_CS_709_EXTENDED:
				tech_name = "DrawMultiply";
				break;
			case GS_CS_709_SCRGB:
				break;
			}
		}
	}

	return tech_name;
}

static void scale_filter_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);

	struct scale_filter_data *filter = data;

	if (!filter->valid || !filter->target_valid) {
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
	float multiplier;
	const char *technique = get_tech_name_and_multiplier(filter, gs_get_color_space(), source_space, &multiplier);
	const enum gs_color_format format = gs_get_format_from_space(source_space);
	if (obs_source_process_filter_begin_with_color_space(filter->context, format, source_space,
							     OBS_NO_DIRECT_RENDERING)) {
		if (filter->dimension_param)
			gs_effect_set_vec2(filter->dimension_param, &filter->dimension);

		if (filter->dimension_i_param)
			gs_effect_set_vec2(filter->dimension_i_param, &filter->dimension_i);

		if (filter->undistort_factor_param)
			gs_effect_set_float(filter->undistort_factor_param, (float)filter->undistort_factor);

		if (filter->multiplier_param)
			gs_effect_set_float(filter->multiplier_param, multiplier);

		if (filter->sampling == OBS_SCALE_POINT)
			gs_effect_set_next_sampler(filter->image_param, filter->point_sampler);

		gs_blend_state_push();
		gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);

		obs_source_process_filter_tech_end(filter->context, filter->effect, filter->cx_out, filter->cy_out,
						   technique);

		gs_blend_state_pop();
	}
}

static const double downscale_vals[] = {1.0, 1.25, (1.0 / 0.75), 1.5, (1.0 / 0.6), 1.75, 2.0, 2.25, 2.5, 2.75, 3.0};

#define NUM_DOWNSCALES (sizeof(downscale_vals) / sizeof(double))

static const char *aspects[] = {"16:9", "16:10", "4:3", "1:1"};

#define NUM_ASPECTS (sizeof(aspects) / sizeof(const char *))

static bool sampling_modified(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
	const char *sampling = obs_data_get_string(settings, S_SAMPLING);

	bool has_undistort;
	if (astrcmpi(sampling, S_SAMPLING_POINT) == 0) {
		has_undistort = false;
	} else if (astrcmpi(sampling, S_SAMPLING_BILINEAR) == 0) {
		has_undistort = false;
	} else if (astrcmpi(sampling, S_SAMPLING_LANCZOS) == 0) {
		has_undistort = true;
	} else if (astrcmpi(sampling, S_SAMPLING_AREA) == 0) {
		has_undistort = false;
	} else { /* S_SAMPLING_BICUBIC */
		has_undistort = true;
	}

	obs_property_set_visible(obs_properties_get(props, S_UNDISTORT), has_undistort);

	UNUSED_PARAMETER(p);
	return true;
}

static obs_properties_t *scale_filter_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();
	struct obs_video_info ovi;
	obs_property_t *p;
	uint32_t cx;
	uint32_t cy;

	struct {
		int cx;
		int cy;
	} downscales[NUM_DOWNSCALES];

	/* ----------------- */

	obs_get_video_info(&ovi);
	cx = ovi.base_width;
	cy = ovi.base_height;

	for (size_t i = 0; i < NUM_DOWNSCALES; i++) {
		downscales[i].cx = (int)((double)cx / downscale_vals[i]);
		downscales[i].cy = (int)((double)cy / downscale_vals[i]);
	}

	p = obs_properties_add_list(props, S_SAMPLING, T_SAMPLING, OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_set_modified_callback(p, sampling_modified);
	obs_property_list_add_string(p, T_SAMPLING_POINT, S_SAMPLING_POINT);
	obs_property_list_add_string(p, T_SAMPLING_BILINEAR, S_SAMPLING_BILINEAR);
	obs_property_list_add_string(p, T_SAMPLING_BICUBIC, S_SAMPLING_BICUBIC);
	obs_property_list_add_string(p, T_SAMPLING_LANCZOS, S_SAMPLING_LANCZOS);
	obs_property_list_add_string(p, T_SAMPLING_AREA, S_SAMPLING_AREA);

	/* ----------------- */

	p = obs_properties_add_list(props, S_RESOLUTION, T_RESOLUTION, OBS_COMBO_TYPE_EDITABLE,
				    OBS_COMBO_FORMAT_STRING);

	obs_property_list_add_string(p, T_NONE, T_NONE);
	obs_property_list_add_string(p, T_BASE, T_BASE);

	for (size_t i = 0; i < NUM_ASPECTS; i++)
		obs_property_list_add_string(p, aspects[i], aspects[i]);

	for (size_t i = 0; i < NUM_DOWNSCALES; i++) {
		char str[32];
		snprintf(str, sizeof(str), "%dx%d", downscales[i].cx, downscales[i].cy);
		obs_property_list_add_string(p, str, str);
	}

	obs_properties_add_bool(props, S_UNDISTORT, T_UNDISTORT);

	/* ----------------- */

	UNUSED_PARAMETER(data);
	return props;
}

static void scale_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_string(settings, S_SAMPLING, S_SAMPLING_BICUBIC);
	obs_data_set_default_string(settings, S_RESOLUTION, T_NONE);
	obs_data_set_default_bool(settings, S_UNDISTORT, 0);
}

static uint32_t scale_filter_width(void *data)
{
	struct scale_filter_data *filter = data;
	return (uint32_t)filter->cx_out;
}

static uint32_t scale_filter_height(void *data)
{
	struct scale_filter_data *filter = data;
	return (uint32_t)filter->cy_out;
}

static enum gs_color_space scale_filter_get_color_space(void *data, size_t count,
							const enum gs_color_space *preferred_spaces)
{
	const enum gs_color_space potential_spaces[] = {
		GS_CS_SRGB,
		GS_CS_SRGB_16F,
		GS_CS_709_EXTENDED,
	};

	struct scale_filter_data *const filter = data;
	const enum gs_color_space source_space = obs_source_get_color_space(
		obs_filter_get_target(filter->context), OBS_COUNTOF(potential_spaces), potential_spaces);

	enum gs_color_space space = source_space;
	for (size_t i = 0; i < count; ++i) {
		space = preferred_spaces[i];
		if (space == source_space)
			break;
	}

	return space;
}

struct obs_source_info scale_filter = {
	.id = "scale_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_SRGB,
	.get_name = scale_filter_name,
	.create = scale_filter_create,
	.destroy = scale_filter_destroy,
	.video_tick = scale_filter_tick,
	.video_render = scale_filter_render,
	.update = scale_filter_update,
	.get_properties = scale_filter_properties,
	.get_defaults = scale_filter_defaults,
	.get_width = scale_filter_width,
	.get_height = scale_filter_height,
	.video_get_color_space = scale_filter_get_color_space,
};
