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

#define SETTING_HARD_CUT_OFF           "hard_cut_off"
#define SETTING_SOFT_CUT_OFF           "soft_cut_off"
#define SETTING_LUM_CO_EFF             "lum_co_eff"

#define OMT                             obs_module_text
#define TEXT_HARD_CUT_OFF               OMT("LumaKey.HardCutOff")
#define TEXT_SOFT_CUT_OFF               OMT("LumaKey.SoftCutOff")


struct luma_key_filter_data {
	obs_source_t                   *context;

	gs_effect_t                    *effect;

	gs_eparam_t                    *hard_cut_off_param;
	gs_eparam_t                    *soft_cut_off_param;
	gs_eparam_t                    *lum_co_eff_param;

	float                           hard_cut_off;
	float                           soft_cut_off;

	struct vec3                     lum_co_eff;
};


/*
 * As the function's name implies, this provides the user facing name
 * of your filter.
 */
static const char *luma_key_filter_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("LumaKey");
}

/*
 * This function is called (see bottom of this file for more details)
 * whenever the OBS filter interface changes. So when the user is messing
 * with a slider this function is called to update the internal settings
 * in OBS, and hence the settings being passed to the CPU/GPU.
 */
static void luma_key_filter_update(void *data, obs_data_t *settings)
{
	struct luma_key_filter_data *filter = data;
	filter->hard_cut_off = (float)obs_data_get_double(settings,
			SETTING_HARD_CUT_OFF);
	filter->soft_cut_off = (float)obs_data_get_double(settings,
			SETTING_SOFT_CUT_OFF);
}

/*
 * Since this is C we have to be careful when destroying/removing items from
 * OBS. Jim has added several useful functions to help keep memory leaks to
 * a minimum, and handle the destruction and construction of these filters.
 */
static void luma_key_filter_destroy(void *data)
{
	struct luma_key_filter_data *filter = data;

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
static void *luma_key_filter_create(obs_data_t *settings,
		obs_source_t *context)
{
	/*
	 * Because of limitations of pre-C99 compilers, you can't create an
	 * array that doesn't have a known size at compile time. The below
	 * function calculates the size needed and allocates memory to
	 * handle the source.
	 */
	struct luma_key_filter_data *filter =
			bzalloc(sizeof(struct luma_key_filter_data));

	/*
	 * By default the effect file is stored in the ./data directory that
	 * your filter resides in.
	 */
	char *effect_path = obs_module_file("luma_key_filter.effect");

	filter->context = context;

	/* Here we enter the GPU drawing/shader portion of our code. */
	obs_enter_graphics();

	/* Load the shader on the GPU. */
	filter->effect = gs_effect_create_from_file(effect_path, NULL);

	/* If the filter is active pass the parameters to the filter. */
	if (filter->effect) {
		filter->hard_cut_off_param = gs_effect_get_param_by_name(
				filter->effect, SETTING_HARD_CUT_OFF);
		filter->soft_cut_off_param = gs_effect_get_param_by_name(
				filter->effect, SETTING_SOFT_CUT_OFF);
		filter->lum_co_eff_param =  gs_effect_get_param_by_name(
				filter->effect, SETTING_LUM_CO_EFF);
	}

	obs_leave_graphics();

	bfree(effect_path);

	if (!filter->effect) {
		luma_key_filter_destroy(filter);
		return NULL;
	}

	/* Now add our 'constants' to the struct. */
	vec3_set(&filter->lum_co_eff, 0.299f, 0.587f, 0.114f);

	/*
	 * It's important to call the update function here. if we don't
	 * we could end up with the user controlled sliders and values
	 * updating, but the visuals not updating to match.
	 */
	luma_key_filter_update(filter, settings);
	return filter;
}

/* This is where the actual rendering of the filter takes place. */
static void luma_key_filter_render(void *data, gs_effect_t *effect)
{
	struct luma_key_filter_data *filter = data;

	if (!obs_source_process_filter_begin(filter->context, GS_RGBA,
			OBS_ALLOW_DIRECT_RENDERING))
		return;

	/* Now pass the interface variables to the .shader file. */
	gs_effect_set_float(filter->hard_cut_off_param,
			(filter->hard_cut_off / 255));
	gs_effect_set_float(filter->soft_cut_off_param,
			(filter->soft_cut_off / 255));
	gs_effect_set_vec3(filter->lum_co_eff_param, &filter->lum_co_eff);

	obs_source_process_filter_end(filter->context, filter->effect, 0, 0);

	UNUSED_PARAMETER(effect);
}

/*
 * This function sets the interface. the types (add_*_Slider), the type of
 * data collected (int), the internal name, user-facing name, minimum,
 * maximum and step values. While a custom interface can be built, for a
 * simple filter like this it's better to use the supplied functions.
 */
static obs_properties_t *luma_key_filter_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	obs_properties_add_float_slider(props, SETTING_HARD_CUT_OFF,
			TEXT_HARD_CUT_OFF, 0.01f, 255.0f, 0.01f);
	obs_properties_add_float_slider(props, SETTING_SOFT_CUT_OFF,
			TEXT_SOFT_CUT_OFF, 0.01f, 255.0f, 0.01f);

	UNUSED_PARAMETER(data);
	return props;
}

/*
 * As the function's name implies, this provides the default settings for any
 * options you wish to provide a default for. *NOTE* this function is
 * completely optional, as is providing a default for any particular
 * option.
 */
static void luma_key_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_double(settings, SETTING_HARD_CUT_OFF, 25.0);
	obs_data_set_default_double(settings, SETTING_SOFT_CUT_OFF, 50.0);
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
struct obs_source_info luma_key_filter = {
	.id                            = "luma_key_filter",
	.type                          = OBS_SOURCE_TYPE_FILTER,
	.output_flags                  = OBS_SOURCE_VIDEO,
	.get_name                      = luma_key_filter_name,
	.create                        = luma_key_filter_create,
	.destroy                       = luma_key_filter_destroy,
	.video_render                  = luma_key_filter_render,
	.update                        = luma_key_filter_update,
	.get_properties                = luma_key_filter_properties,
	.get_defaults                  = luma_key_filter_defaults
};
