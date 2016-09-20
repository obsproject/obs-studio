#include <stdio.h>
#include <stdlib.h>
#include <util/dstr.h>
#include <obs-module.h>
#include <util/platform.h>
#include <graphics/vec2.h>
#include <graphics/math-defs.h>

#define S_RESOLUTION                    "resolution"
#define S_SAMPLING                      "sampling"

#define T_RESOLUTION                    obs_module_text("Resolution")
#define T_NONE                          obs_module_text("None")
#define T_SAMPLING                      obs_module_text("ScaleFiltering")
#define T_SAMPLING_POINT                obs_module_text("ScaleFiltering.Point")
#define T_SAMPLING_BILINEAR             obs_module_text("ScaleFiltering.Bilinear")
#define T_SAMPLING_BICUBIC              obs_module_text("ScaleFiltering.Bicubic")
#define T_SAMPLING_LANCZOS              obs_module_text("ScaleFiltering.Lanczos")

#define S_SAMPLING_POINT                "point"
#define S_SAMPLING_BILINEAR             "bilinear"
#define S_SAMPLING_BICUBIC              "bicubic"
#define S_SAMPLING_LANCZOS              "lanczos"

struct scale_filter_data {
	obs_source_t                    *context;
	gs_effect_t                     *effect;
	gs_eparam_t                     *image_param;
	gs_eparam_t                     *dimension_param;
	struct vec2                     dimension_i;
	int                             cx_in;
	int                             cy_in;
	int                             cx_out;
	int                             cy_out;
	enum obs_scale_type             sampling;
	gs_samplerstate_t               *point_sampler;
	bool                            aspect_ratio_only : 1;
	bool                            target_valid : 1;
	bool                            valid : 1;
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

	if (astrcmpi(sampling, S_SAMPLING_POINT) == 0) {
		filter->sampling = OBS_SCALE_POINT;

	} else if (astrcmpi(sampling, S_SAMPLING_BILINEAR) == 0) {
		filter->sampling = OBS_SCALE_BILINEAR;

	} else if (astrcmpi(sampling, S_SAMPLING_LANCZOS) == 0) {
		filter->sampling = OBS_SCALE_LANCZOS;

	} else { /* S_SAMPLING_BICUBIC */
		filter->sampling = OBS_SCALE_BICUBIC;
	}
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
	struct scale_filter_data *filter =
		bzalloc(sizeof(struct scale_filter_data));
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

	if (filter->aspect_ratio_only) {
		double old_aspect = cx_f / cy_f;
		double new_aspect =
			(double)filter->cx_in / (double)filter->cy_in;

		if (fabs(old_aspect - new_aspect) <= EPSILON) {
			filter->target_valid = false;
			return;
		} else {
			if (new_aspect > old_aspect) {
				filter->cx_out = (int)(cy_f * new_aspect);
				filter->cy_out = cy;
			} else {
				filter->cx_out = cx;
				filter->cy_out = (int)(cx_f / new_aspect);
			}
		}
	} else {
		filter->cx_out = filter->cx_in;
		filter->cy_out = filter->cy_in;
	}

	vec2_set(&filter->dimension_i,
			1.0f / (float)cx,
			1.0f / (float)cy);

	/* ------------------------- */

	lower_than_2x = filter->cx_out < cx / 2 || filter->cy_out < cy / 2;

	if (lower_than_2x && filter->sampling != OBS_SCALE_POINT) {
		type = OBS_EFFECT_BILINEAR_LOWRES;
	} else {
		switch (filter->sampling) {
		default:
		case OBS_SCALE_POINT:
		case OBS_SCALE_BILINEAR: type = OBS_EFFECT_DEFAULT; break;
		case OBS_SCALE_BICUBIC:  type = OBS_EFFECT_BICUBIC; break;
		case OBS_SCALE_LANCZOS:  type = OBS_EFFECT_LANCZOS; break;
		}
	}

	filter->effect = obs_get_base_effect(type);
	filter->image_param = gs_effect_get_param_by_name(filter->effect,
			"image");

	if (type != OBS_EFFECT_DEFAULT) {
		filter->dimension_param = gs_effect_get_param_by_name(
				filter->effect, "base_dimension_i");
	} else {
		filter->dimension_param = NULL;
	}

	UNUSED_PARAMETER(seconds);
}

static void scale_filter_render(void *data, gs_effect_t *effect)
{
	struct scale_filter_data *filter = data;

	if (!filter->valid || !filter->target_valid) {
		obs_source_skip_video_filter(filter->context);
		return;
	}

	if (!obs_source_process_filter_begin(filter->context, GS_RGBA,
				OBS_NO_DIRECT_RENDERING))
		return;

	if (filter->dimension_param)
		gs_effect_set_vec2(filter->dimension_param,
				&filter->dimension_i);

	if (filter->sampling == OBS_SCALE_POINT)
		gs_effect_set_next_sampler(filter->image_param,
				filter->point_sampler);

	obs_source_process_filter_end(filter->context, filter->effect,
			filter->cx_out, filter->cy_out);

	UNUSED_PARAMETER(effect);
}

static const double downscale_vals[] = {
	1.0,
	1.25,
	(1.0/0.75),
	1.5,
	(1.0/0.6),
	1.75,
	2.0,
	2.25,
	2.5,
	2.75,
	3.0
};

#define NUM_DOWNSCALES (sizeof(downscale_vals) / sizeof(double))

static const char *aspects[] = {
	"16:9",
	"16:10",
	"4:3",
	"1:1"
};

#define NUM_ASPECTS (sizeof(aspects) / sizeof(const char *))

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

	p = obs_properties_add_list(props, S_SAMPLING, T_SAMPLING,
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(p, T_SAMPLING_POINT,    S_SAMPLING_POINT);
	obs_property_list_add_string(p, T_SAMPLING_BILINEAR, S_SAMPLING_BILINEAR);
	obs_property_list_add_string(p, T_SAMPLING_BICUBIC,  S_SAMPLING_BICUBIC);
	obs_property_list_add_string(p, T_SAMPLING_LANCZOS,  S_SAMPLING_LANCZOS);

	/* ----------------- */

	p = obs_properties_add_list(props, S_RESOLUTION, T_RESOLUTION,
			OBS_COMBO_TYPE_EDITABLE, OBS_COMBO_FORMAT_STRING);

	obs_property_list_add_string(p, T_NONE, T_NONE);

	for (size_t i = 0; i < NUM_ASPECTS; i++)
		obs_property_list_add_string(p, aspects[i], aspects[i]);

	for (size_t i = 0; i < NUM_DOWNSCALES; i++) {
		char str[32];
		snprintf(str, 32, "%dx%d", downscales[i].cx, downscales[i].cy);
		obs_property_list_add_string(p, str, str);
	}

	/* ----------------- */

	UNUSED_PARAMETER(data);
	return props;
}

static void scale_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_string(settings, S_SAMPLING, S_SAMPLING_BICUBIC);
	obs_data_set_default_string(settings, S_RESOLUTION, T_NONE);
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

struct obs_source_info scale_filter = {
	.id                            = "scale_filter",
	.type                          = OBS_SOURCE_TYPE_FILTER,
	.output_flags                  = OBS_SOURCE_VIDEO,
	.get_name                      = scale_filter_name,
	.create                        = scale_filter_create,
	.destroy                       = scale_filter_destroy,
	.video_tick                    = scale_filter_tick,
	.video_render                  = scale_filter_render,
	.update                        = scale_filter_update,
	.get_properties                = scale_filter_properties,
	.get_defaults                  = scale_filter_defaults,
	.get_width                     = scale_filter_width,
	.get_height                    = scale_filter_height
};
