/****************************************************************************
Copyright (C) 2017 by c3r1c3 <c3r1c3@nevermindonline.com>

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

#define SETTING_BLUR_WIDTH             "blur_width"
#define SETTING_ZOOM                   "zoom"
#define SETTING_ITERATIONS             "iterations"
#define SETTING_XY_OFFSET              "xy_offset"
#define SETTING_X_OFFSET               "x_offset"
#define SETTING_Y_OFFSET               "y_offset"

#define OMT                            obs_module_text

#define TEXT_BLUR_WIDTH                OMT("RadialBlur.BlurWidth")
#define TEXT_ZOOM                      OMT("RadialBlur.Zoom")
#define TEXT_ITERATIONS                OMT("RadialBlur.Iterations")
#define TEXT_X_OFFSET                  OMT("RadialBlur.XOffset")
#define TEXT_Y_OFFSET                  OMT("RadialBlur.YOffset")

struct radial_blur_filter_data {
	obs_source_t                   *context;

	gs_effect_t                    *effect;

	gs_eparam_t                    *blur_width_param;
	gs_eparam_t                    *zoom_param;
	gs_eparam_t                    *iterations_param;
	gs_eparam_t                    *xy_offset_param;

	float                           blur_width;
	float                           zoom;
	float                           iterations;
	struct vec2                     xy_offset;
};

/*
 * As the function's name implies, this provides the user facing name
 * of your filter.
 */
static const char *radial_blur_filter_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("RadialBlur");
}

/*
 * This function is called (see bottom of this file for more details)
 * whenever the OBS filter interface changes. So when the user is messing
 * with a slider this function is called to update the internal settings
 * in OBS, and hence the settings being passed to the CPU/GPU.
 *
 * Here we do some special stuff with textures and whatnot.
 */
static void radial_blur_filter_update(void *data, obs_data_t *settings)
{
	struct radial_blur_filter_data *filter = data;

	filter->blur_width = (float)obs_data_get_double(settings,
			SETTING_BLUR_WIDTH);
	filter->zoom = (float)obs_data_get_double(settings,
			SETTING_ZOOM);
	filter->iterations = (float)obs_data_get_int(settings,
			SETTING_ITERATIONS);

	vec2_set(&filter->xy_offset, ((float)obs_data_get_double(settings,
			SETTING_X_OFFSET) / 100),
			((float)obs_data_get_double(settings,
			SETTING_Y_OFFSET) / 100));
}

/*
 * Since this is C we have to be careful when destroying/removing items from
 * OBS. Jim has added several useful functions to help keep memory leaks to
 * a minimum, and handle the destruction and construction of these filters.
 */
static void radial_blur_filter_destroy(void *data)
{
	struct radial_blur_filter_data *filter = data;

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
static void *radial_blur_filter_create(obs_data_t *settings,
		obs_source_t *context)
{
	/*
	 * Because of limitations of pre-C99 compilers, you can't create an
	 * array that doesn't have a know size at compile time. The below
	 * function calculates the size needed and allocates memory to
	 * handle the source.
	 */
	struct radial_blur_filter_data *filter =
			bzalloc(sizeof(struct radial_blur_filter_data));

	/* By default the effect file is stored in the ./data directory that
	* your filter resides in. */
	char *effect_path = obs_module_file("radial_blur_filter.effect");

	filter->context = context;

	/* Check for the effect and log if it's missing */
	if (!effect_path) {
		blog(LOG_ERROR, "Could not find radial_blur_filter.effect");
		return NULL;
	}

	/* Here we enter the GPU drawing/shader portion of our code. */
	obs_enter_graphics();

	/* Load the shader on the GPU. */
	filter->effect = gs_effect_create_from_file(effect_path, NULL);

	/*
	 * If the filter exists grab the param names and assign them to the
	 * struct for later recall/use.
	 */
	if (filter->effect) {
		filter->blur_width_param = gs_effect_get_param_by_name(
				filter->effect, SETTING_BLUR_WIDTH);
		filter->zoom_param = gs_effect_get_param_by_name(
				filter->effect, SETTING_ZOOM);
		filter->iterations_param = gs_effect_get_param_by_name(
				filter->effect, SETTING_ITERATIONS);
		filter->xy_offset_param = gs_effect_get_param_by_name(
				filter->effect, SETTING_XY_OFFSET);
	}

	obs_leave_graphics();

	bfree(effect_path);

	if (!filter->effect) {
		radial_blur_filter_destroy(filter);
		return NULL;
	}

	/*
	 * It's important to call the update function here. if we don't
	 * we could end up with the user controlled sliders and values
	 * updating, but the visuals not updating to match.
	 */
	radial_blur_filter_update(filter, settings);
	return filter;
}

/* This is where the actual rendering of the filter takes place. */
static void radial_blur_filter_render(void *data, gs_effect_t *effect)
{
	struct radial_blur_filter_data *filter = data;

	if (!obs_source_process_filter_begin(filter->context, GS_RGBA,
			OBS_ALLOW_DIRECT_RENDERING))
		return;

	/* Now pass the interface variables to the .shader file. */
	gs_effect_set_float(filter->blur_width_param, filter->blur_width);
	gs_effect_set_float(filter->zoom_param, filter->zoom);
	gs_effect_set_float(filter->iterations_param, filter->iterations);
	gs_effect_set_vec2(filter->xy_offset_param, &filter->xy_offset);

	obs_source_process_filter_end(filter->context, filter->effect, 0, 0);

	UNUSED_PARAMETER(effect);
}

/*
 * This function sets the interface. the types (add_*_Slider), the type of
 * data collected (int), the internal name, user-facing name, minimum,
 * maximum and step values. While a custom interface can be built, for a
 * simple filter like this it's better to use the supplied functions.
 */
static obs_properties_t *radial_blur_filter_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	obs_properties_add_float_slider(props, SETTING_BLUR_WIDTH,
			TEXT_BLUR_WIDTH, -5.0, 5.0, 0.01);
	obs_properties_add_float_slider(props, SETTING_ZOOM,
			TEXT_ZOOM, 0.01, 2.0, 0.01);
	obs_properties_add_int_slider(props, SETTING_ITERATIONS,
			TEXT_ITERATIONS, 2, 20, 1);
	obs_properties_add_float_slider(props, SETTING_X_OFFSET,
			TEXT_X_OFFSET, 0.01, 100.0, 0.01);
	obs_properties_add_float_slider(props, SETTING_Y_OFFSET,
			TEXT_Y_OFFSET, 0.01, 100.0, 0.01);

	UNUSED_PARAMETER(data);
	return props;
}

/*
 * As the function's name implies, this provides the default settings for any
 * options you wish to provide a default for. *NOTE* this function is
 * completely optional, as is providing a default for any particular
 * option.
 */
static void radial_blur_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_double(settings, SETTING_BLUR_WIDTH, -0.1);
	obs_data_set_default_double(settings, SETTING_ZOOM, 1.0);
	obs_data_set_default_int(settings, SETTING_ITERATIONS, 10);
	obs_data_set_default_double(settings, SETTING_X_OFFSET, 50.0);
	obs_data_set_default_double(settings, SETTING_Y_OFFSET, 50.0);
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
struct obs_source_info radial_blur_filter = {
	.id                            = "radial_blur_filter",
	.type                          = OBS_SOURCE_TYPE_FILTER,
	.output_flags                  = OBS_SOURCE_VIDEO,
	.get_name                      = radial_blur_filter_name,
	.create                        = radial_blur_filter_create,
	.destroy                       = radial_blur_filter_destroy,
	.video_render                  = radial_blur_filter_render,
	.update                        = radial_blur_filter_update,
	.get_properties                = radial_blur_filter_properties,
	.get_defaults                  = radial_blur_filter_defaults
};
