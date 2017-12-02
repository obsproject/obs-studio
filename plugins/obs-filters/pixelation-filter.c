/*****************************************************************************
Copyright (C) 2016 by c3r1c3 <c3r1c3d@nevermindonline.com>

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

#define SETTING_HORIZ_WEIGHT           "horiz_weight"
#define SETTING_VERT_WEIGHT            "vert_weight"
#define SETTING_BLOCK_HEIGHT           "block_height"
#define SETTING_BLOCK_WIDTH            "block_width"

#define BLOCK_CONSTANT                 0.001953125 /* i.e. 1 / 512 */

#define OMT                            obs_module_text
#define TEXT_HORIZ_WEIGHT              OMT("Pixelation.HorizontalWeight")
#define TEXT_VERT_WEIGHT               OMT("Pixelation.VerticalWeight")
#define TEXT_BLOCK_HEIGHT              OMT("Pixelation.BlockHeight")
#define TEXT_BLOCK_WIDTH               OMT("Pixelation.BlockWidth")


struct pixelation_filter_data {
	obs_source_t                   *context;

	gs_effect_t                    *effect;

	gs_eparam_t                    *horiz_weight_param;
	gs_eparam_t                    *vert_weight_param;
	gs_eparam_t                    *block_height_param;
	gs_eparam_t                    *block_width_param;

	float                           horiz_weight;
	float                           vert_weight;
	float                           block_height;
	float                           block_width;
};


/*
 * As the function's name implies, this provides the user facing name
 * of your filter.
 */
static const char *pixelation_filter_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("Pixelation");
}

/*
 * This function is called (see bottom of this file for more details)
 * whenever the OBS filter interface changes. So when the user is messing
 * with a slider this function is called to update the internal settings
 * in OBS, and hence the settings being passed to the CPU/GPU.
 */
static void pixelation_filter_update(void *data, obs_data_t *settings)
{
	struct pixelation_filter_data *filter = data;
	filter->block_height = (float)(obs_data_get_double(settings,
			SETTING_BLOCK_HEIGHT) * BLOCK_CONSTANT);
	filter->block_width = (float)(obs_data_get_double(settings,
			SETTING_BLOCK_WIDTH) * BLOCK_CONSTANT);
	filter->horiz_weight = (float)(obs_data_get_double(settings,
			SETTING_HORIZ_WEIGHT) * filter->block_width);
	filter->vert_weight = (float)(obs_data_get_double(settings,
			SETTING_VERT_WEIGHT) * filter->block_height);
}

/*
 * Since this is C we have to be careful when destroying/removing items from
 * OBS. Jim has added several useful functions to help keep memory leaks to
 * a minimum, and handle the destruction and construction of these filters.
 */
static void pixelation_filter_destroy(void *data)
{
	struct pixelation_filter_data *filter = data;

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
static void *pixelation_filter_create(obs_data_t *settings,
		obs_source_t *context)
{
	/*
	 * Because of limitations of pre-C99 compilers, you can't create an
	 * array that doesn't have a know size at compile time. The below
	 * function calculates the size needed and allocates memory to
	 * handle the source.
	 */
	struct pixelation_filter_data *filter =
			bzalloc(sizeof(struct pixelation_filter_data));

	/*
	 * By default the effect file is stored in the ./data directory that
	 * your filter resides in.
	 */
	char *effect_path = obs_module_file("pixelation_filter.effect");


	filter->context = context;

	/* Here we enter the GPU drawing/shader portion of our code. */
	obs_enter_graphics();

	/* Load the shader on the GPU. */
	filter->effect = gs_effect_create_from_file(effect_path, NULL);

	/* If the filter is active pass the parameters to the filter. */
	if (filter->effect) {
		filter->horiz_weight_param = gs_effect_get_param_by_name(
				filter->effect, SETTING_HORIZ_WEIGHT);
		filter->vert_weight_param = gs_effect_get_param_by_name(
				filter->effect, SETTING_VERT_WEIGHT);
		filter->block_height_param = gs_effect_get_param_by_name(
				filter->effect, SETTING_BLOCK_HEIGHT);
		filter->block_width_param = gs_effect_get_param_by_name(
				filter->effect, SETTING_BLOCK_WIDTH);
	}

	obs_leave_graphics();

	bfree(effect_path);

	if (!filter->effect) {
		pixelation_filter_destroy(filter);
		return NULL;
	}

	/*
	 * It's important to call the update function here. if we don't
	 * we could end up with the user controlled sliders and values
	 * updating, but the visuals not updating to match.
	 */
	pixelation_filter_update(filter, settings);
	return filter;
}

/* This is where the actual rendering of the filter takes place. */
static void pixelation_filter_render(void *data, gs_effect_t *effect)
{
	struct pixelation_filter_data *filter = data;

	if (!obs_source_process_filter_begin(filter->context, GS_RGBA,
			OBS_ALLOW_DIRECT_RENDERING))
		return;

	/* Now pass the interface variables to the .shader file. */
	gs_effect_set_float(filter->horiz_weight_param, filter->horiz_weight);
	gs_effect_set_float(filter->vert_weight_param, filter->vert_weight);
	gs_effect_set_float(filter->block_height_param, filter->block_height);
	gs_effect_set_float(filter->block_width_param, filter->block_width);

	obs_source_process_filter_end(filter->context, filter->effect, 0, 0);

	UNUSED_PARAMETER(effect);
}

/*
 * This function sets the interface. the types (add_*_Slider), the type of
 * data collected (int), the internal name, user-facing name, minimum,
 * maximum and step values. While a custom interface can be built, for a
 * simple filter like this it's better to use the supplied functions.
 */
static obs_properties_t *pixelation_filter_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	obs_property_t *horiz_weight = obs_properties_add_list(props,
			SETTING_HORIZ_WEIGHT, TEXT_HORIZ_WEIGHT,
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_FLOAT);
	obs_property_list_add_float(horiz_weight,
			OMT("Pixelation.Left"), 0.0);
	obs_property_list_add_float(horiz_weight,
			OMT("Pixelation.Center"), 0.5);
	obs_property_list_add_float(horiz_weight,
			OMT("Pixelation.Right"), 1.0);

	obs_property_t *vert_weight = obs_properties_add_list(props,
			SETTING_VERT_WEIGHT, TEXT_VERT_WEIGHT,
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_FLOAT);
	obs_property_list_add_float(vert_weight,
			OMT("Pixelation.Top"), 0.0);
	obs_property_list_add_float(vert_weight,
			OMT("Pixelation.Middle"), 0.5);
	obs_property_list_add_float(vert_weight,
			OMT("Pixelation.Bottom"), 1.0);

	obs_properties_add_float_slider(props, SETTING_BLOCK_HEIGHT,
			TEXT_BLOCK_HEIGHT, 1.0f, 50.0f, 0.01f);
	obs_properties_add_float_slider(props, SETTING_BLOCK_WIDTH,
			TEXT_BLOCK_WIDTH, 1.0f, 50.0f, 0.01f);

	UNUSED_PARAMETER(data);
	return props;
}

/*
 * As the function's name implies, this provides the default settings for any
 * options you wish to provide a default for. *NOTE* this function is
 * completely optional, as is providing a default for any particular
 * option.
 */
static void pixelation_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_double(settings, SETTING_BLOCK_HEIGHT, 0.0);
	obs_data_set_default_double(settings, SETTING_BLOCK_WIDTH, 0.0);
	obs_data_set_default_double(settings, SETTING_BLOCK_HEIGHT, 6.0);
	obs_data_set_default_double(settings, SETTING_BLOCK_WIDTH, 6.0);
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
struct obs_source_info pixelation_filter = {
	.id                            = "pixelation_filter",
	.type                          = OBS_SOURCE_TYPE_FILTER,
	.output_flags                  = OBS_SOURCE_VIDEO,
	.get_name                      = pixelation_filter_name,
	.create                        = pixelation_filter_create,
	.destroy                       = pixelation_filter_destroy,
	.video_render                  = pixelation_filter_render,
	.update                        = pixelation_filter_update,
	.get_properties                = pixelation_filter_properties,
	.get_defaults                  = pixelation_filter_defaults
};
