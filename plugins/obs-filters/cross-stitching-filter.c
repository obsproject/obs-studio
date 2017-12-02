/*****************************************************************************
Copyright (C) 2016 by c3r1c3 <c3r1c3@nevermindonline.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#include <obs-module.h>

#define SETTING_STITCH_GAP            "stitch_gap"
#define SETTING_NEGATIVE_BIAS         "negative_bias"
#define SETTING_STITCH_COLOR          "stitch_color"
#define SETTING_OPACITY               "stitch_opacity"
#define SETTING_INVERT                "invert"
#define SETTING_GRAYSCALE             "grayscale"

#define OMT                           obs_module_text
#define TEXT_STITCH_GAP               OMT("CrossStitching.StitchGap")
#define TEXT_NEGATIVE_BIAS            OMT("CrossStitching.NegativeBias")
#define TEXT_STITCH_COLOR             OMT("CrossStitching.StitchColor")
#define TEXT_OPACITY                  OMT("CrossStitching.StitchOpacity")
#define TEXT_INVERT                   OMT("CrossStitching.InvertStitching")
#define TEXT_GRAYSCALE                OMT("CrossStitching.Grayscale")


struct cross_stitching_filter_data {
	obs_source_t                   *context;

	gs_effect_t                    *effect;

	gs_eparam_t                    *stitch_gap_param;
	gs_eparam_t                    *negative_bias_param;
	gs_eparam_t                    *stitch_color_param;
	gs_eparam_t                    *invert_param;
	gs_eparam_t                    *grayscale_param;

	uint8_t                         stitch_gap;
	uint8_t                         negative_bias;
	struct vec4                     stitch_color;
	bool                            invert;
	bool                            grayscale;
};


/*
 * As the function's name implies, this provides the user facing name
 * of your filter.
 */
static const char *cross_stitching_filter_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("CrossStitching");
}

/*
 * This function is called (see bottom of this file for more details)
 * whenever the OBS filter interface changes. So when the user is messing
 * with a slider this function is called to update the internal settings
 * in OBS, and hence the settings being passed to the CPU/GPU.
 */
static void cross_stitching_filter_update(void *data, obs_data_t *settings)
{
	struct cross_stitching_filter_data *filter = data;
	filter->stitch_gap = (uint8_t)obs_data_get_int(settings,
			SETTING_STITCH_GAP);
	filter->negative_bias = (uint8_t)obs_data_get_int(settings,
			SETTING_NEGATIVE_BIAS);
	uint32_t stitch_color = (uint32_t)obs_data_get_int(settings,
			SETTING_STITCH_COLOR);
	uint32_t stitch_opacity = (uint32_t)obs_data_get_int(settings,
			SETTING_OPACITY);
	filter->invert = obs_data_get_bool(settings, SETTING_INVERT);
	filter->grayscale = obs_data_get_bool(settings, SETTING_GRAYSCALE);

	/* Convert Stitch Color from a uint32_t to a vec4 */
	stitch_color &= 0xFFFFFF;
	stitch_color |= ((stitch_opacity * 255) / 100) << 24;
	vec4_from_rgba(&filter->stitch_color, stitch_color);
}

/*
 * Since this is C we have to be careful when destroying/removing items from
 * OBS. Jim has added several useful functions to help keep memory leaks to
 * a minimum, and handle the destruction and construction of these filters.
 */
static void cross_stitching_filter_destroy(void *data)
{
	struct cross_stitching_filter_data *filter = data;

	if (filter->effect) {
		obs_enter_graphics();
		gs_effect_destroy(filter->effect);
		obs_leave_graphics();
	}

	bfree(data);
}

/*
 * When you apply a filter OBS creates it, and adds it to the source. OBS also
 * starts rendering it immediately. This function doesn't just 'create' the
 * filter, it also calls the render function (farther below) that contains the
 * actual rendering code.
 */
static void *cross_stitching_filter_create(obs_data_t *settings,
		obs_source_t *context)
{
	/*
	 * Because of limitations of pre-C99 compilers, you can't create an
	 * array that doesn't have a know size at compile time. The below
	 * function calculates the size needed and allocates memory to
	 * handle the source.
	 */
	struct cross_stitching_filter_data *filter =
			bzalloc(sizeof(struct cross_stitching_filter_data));

	/*
	 * By default the effect file is stored in the ./data directory that
	 * your filter resides in.
	 */
#ifdef WIN32
	char *effect_path = obs_module_file("cross_stitching_filter.effect");
#else
	char *effect_path = obs_module_file("cross_stitching_filter_gl.effect");
#endif

	filter->context = context;

	/* Here we enter the GPU drawing/shader portion of our code. */
	obs_enter_graphics();

	/* Load the shader on the GPU. */
	filter->effect = gs_effect_create_from_file(effect_path, NULL);

	/* If the filter is active pass the parameters to the filter. */
	if (filter->effect) {
		filter->stitch_gap_param = gs_effect_get_param_by_name(
				filter->effect, SETTING_STITCH_GAP);
		filter->negative_bias_param = gs_effect_get_param_by_name(
				filter->effect, SETTING_NEGATIVE_BIAS);
		filter->stitch_color_param = gs_effect_get_param_by_name(
				filter->effect, SETTING_STITCH_COLOR);
		filter->invert_param = gs_effect_get_param_by_name(
				filter->effect, SETTING_INVERT);
		filter->grayscale_param = gs_effect_get_param_by_name(
				filter->effect, SETTING_GRAYSCALE);
	}

	obs_leave_graphics();

	bfree(effect_path);

	if (!filter->effect) {
		cross_stitching_filter_destroy(filter);
		return NULL;
	}

	/*
	 * It's important to call the update function here. if we don't
	 * we could end up with the user controlled sliders and values
	 * updating, but the visuals not updating to match.
	 */
	cross_stitching_filter_update(filter, settings);
	return filter;
}

/* This is where the actual rendering of the filter takes place. */
static void cross_stitching_filter_render(void *data, gs_effect_t *effect)
{
	struct cross_stitching_filter_data *filter = data;

	if (!obs_source_process_filter_begin(filter->context, GS_RGBA,
			OBS_ALLOW_DIRECT_RENDERING))
		return;

	/* Now pass the interface variables to the .shader file. */
	gs_effect_set_float(filter->stitch_gap_param,
			(float)filter->stitch_gap);
	gs_effect_set_float(filter->negative_bias_param,
			(float)filter->negative_bias);
	gs_effect_set_vec4(filter->stitch_color_param, &filter->stitch_color);
	gs_effect_set_bool(filter->invert_param, filter->invert);
	gs_effect_set_bool(filter->grayscale_param, filter->grayscale);

	obs_source_process_filter_end(filter->context, filter->effect, 0, 0);

	UNUSED_PARAMETER(effect);
}

/*
 * This function sets the interface, the types (add_*_Slider), the type of
 * data collected (int), the internal name, user-facing name, minimum,
 * maximum and step values. While a custom interface can be built, for a
 * simple filter like this it's better to use the supplied functions.
 */
static obs_properties_t *cross_stitching_filter_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	obs_properties_add_int_slider(props, SETTING_STITCH_GAP,
			TEXT_STITCH_GAP, 2, 15, 1);
	obs_properties_add_int_slider(props, SETTING_NEGATIVE_BIAS,
			TEXT_NEGATIVE_BIAS, 1, 10, 1);
	obs_properties_add_color(props, SETTING_STITCH_COLOR,
			TEXT_STITCH_COLOR);
	obs_properties_add_bool(props, SETTING_INVERT, TEXT_INVERT);
	obs_properties_add_bool(props, SETTING_GRAYSCALE, TEXT_GRAYSCALE);

	UNUSED_PARAMETER(data);
	return props;
}

/*
 * As the function's name implies, this provides the default settings for any
 * options you wish to provide a default for. *NOTE* this function is
 * completely optional, as is providing a default for any particular
 * option.
 */
static void cross_stitching_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, SETTING_STITCH_GAP, 6);
	obs_data_set_default_int(settings, SETTING_NEGATIVE_BIAS, 1);
	obs_data_set_default_int(settings, SETTING_STITCH_COLOR, 0x000000);
	obs_data_set_default_int(settings, SETTING_OPACITY, 100);
	obs_data_set_default_bool(settings, SETTING_INVERT, 0);
	obs_data_set_default_bool(settings, SETTING_GRAYSCALE, 0);
}

/*
 * So how does OBS keep track of all these plug-ins/filters? How does OBS know
 * which function to call when it needs to update a setting? Or a source? Or
 * what type of source this is?
 *
 * OBS does it through the obs_source_info struct. Notice how variables are
 * assigned the name of a function? Notice how the function name has the
 * variable name in it? While not mandatory, it helps a ton for you (and those
 * reading your code) to follow this convention.
 */
struct obs_source_info cross_stitching_filter = {
	.id                            = "cross_stitching_filter",
	.type                          = OBS_SOURCE_TYPE_FILTER,
	.output_flags                  = OBS_SOURCE_VIDEO,
	.get_name                      = cross_stitching_filter_name,
	.create                        = cross_stitching_filter_create,
	.destroy                       = cross_stitching_filter_destroy,
	.video_render                  = cross_stitching_filter_render,
	.update                        = cross_stitching_filter_update,
	.get_properties                = cross_stitching_filter_properties,
	.get_defaults                  = cross_stitching_filter_defaults
};
