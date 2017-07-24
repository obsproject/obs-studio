
#include <stdio.h>
#include <stdlib.h>
#include <util/dstr.h>
#include <obs-module.h>
#include <obs-source.h>
#include <obs.h>
#include <util/platform.h>
#include <graphics/vec2.h>
#include <graphics/math-defs.h>

#define T_HORIZONTALKS          obs_module_text("TransformFiltering.Keystone.Horizontal")
#define T_VERTICALKS            obs_module_text("TransformFiltering.Keystone.Vertical")
#define T_HORIZONTALSHEAR       obs_module_text("TransformFiltering.Shear.Horizontal")
#define T_VERTICALSHEAR         obs_module_text("TransformFiltering.Shear.Vertical")
#define T_HORIZONTALOFFSET      obs_module_text("TransformFiltering.Offset.Horizontal")
#define T_VERTICALOFFSET        obs_module_text("TransformFiltering.Offset.Vertical")
#define T_HORIZONTALSCALE       obs_module_text("TransformFiltering.Scale.Horizontal")
#define T_VERTICALSCALE         obs_module_text("TransformFiltering.Scale.Vertical")
#define S_HORIZONTALKS          "horizontal_keystone"
#define S_VERTICALKS            "vertical_keystone"
#define S_HORIZONTALSHEAR       "horizontal_shear"
#define S_VERTICALSHEAR         "vertical_shear"
#define S_HORIZONTALOFFSET      "horizontal_offset"
#define S_VERTICALOFFSET        "vertical_offset"
#define S_HORIZONTALSCALE       "horizontal_scale"
#define S_VERTICALSCALE         "vertical_scale"


struct transform_filter_data {
	obs_source_t        *context;
	gs_effect_t         *effect;
	gs_eparam_t         *keystone_V_param;
	gs_eparam_t         *keystone_H_param;
	gs_eparam_t         *shear_V_param;
	gs_eparam_t         *shear_H_param;
	gs_eparam_t         *offset_V_param;
	gs_eparam_t         *offset_H_param;
	gs_eparam_t         *scale_V_param;
	gs_eparam_t         *scale_H_param;
	gs_eparam_t         *texture_width, *texture_height;
	double              keystone_horizontal;
	double              keystone_vertical;
    double              shear_vertical, shear_horizontal;
    double              offset_vertical, offset_horizontal;
    double              h_scale, v_scale;
	float               texwidth, texheight;
};

static const char *transform_filter_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("TransformFiltering");
}

static void transform_filter_update(void *data, obs_data_t *settings)
{
	struct transform_filter_data *filter = data;

	double vKS = obs_data_get_double(settings, S_VERTICALKS);
	double hKS = obs_data_get_double(settings, S_HORIZONTALKS);

	double vSH = obs_data_get_double(settings, S_VERTICALSHEAR);
	double hSH = obs_data_get_double(settings, S_HORIZONTALSHEAR);

	double vOF = obs_data_get_double(settings, S_VERTICALOFFSET);
	double hOF = obs_data_get_double(settings, S_HORIZONTALOFFSET);

    double h_scale = obs_data_get_double(settings, S_VERTICALSCALE);
    double v_scale = obs_data_get_double(settings, S_HORIZONTALSCALE);
    
	filter->keystone_vertical = vKS;
	filter->keystone_horizontal = hKS;
	filter->shear_horizontal = hSH;
	filter->shear_vertical = vSH;
	filter->offset_horizontal = hOF;
	filter->offset_vertical = vOF;
	filter->h_scale = h_scale;
	filter->v_scale = v_scale;
}

static void transform_filter_destroy(void *data)
{
	struct transform_filter_data *filter = data;

	if(filter->effect){
		obs_enter_graphics();
		gs_effect_destroy(filter->effect);
		obs_leave_graphics();
	}
	
	bfree(data);
}

static void *transform_filter_create(
	obs_data_t *settings, obs_source_t *context)
{
	struct transform_filter_data *filter =
		bzalloc(sizeof(struct transform_filter_data));
	char *effect_path = obs_module_file("transform.effect");
	filter->context = context;

	obs_enter_graphics();

	char* errorString = NULL;
	filter->effect = gs_effect_create_from_file(effect_path, &errorString);
	if(filter->effect){
		filter->keystone_H_param = gs_effect_get_param_by_name(
			filter->effect, "h_keystone");
		filter->keystone_V_param = gs_effect_get_param_by_name(
			filter->effect, "v_keystone");
		filter->texture_width = gs_effect_get_param_by_name(
			filter->effect, "texture_width");
		filter->texture_height = gs_effect_get_param_by_name(
			filter->effect, "texture_height");
		filter->shear_H_param = gs_effect_get_param_by_name(
			filter->effect, "horizontal_shear");
		filter->shear_V_param = gs_effect_get_param_by_name(
			filter->effect, "vertical_shear");
		filter->offset_H_param = gs_effect_get_param_by_name(
			filter->effect, "h_offset");
		filter->offset_V_param = gs_effect_get_param_by_name(
			filter->effect, "v_offset");
		filter->scale_V_param = gs_effect_get_param_by_name(
			filter->effect, "v_scale");
		filter->scale_H_param = gs_effect_get_param_by_name(
			filter->effect, "h_scale");
	}else{
		if(errorString)
			fprintf(stderr,errorString);
		else
			fprintf(stderr,"Unknown Error Occured in Keystone");
	}

	obs_leave_graphics();

	bfree(effect_path);

	if(!filter->effect){
		transform_filter_destroy(filter);
		return NULL;
	}

	transform_filter_update(filter,settings);

	return filter;
}

static void transform_filter_render(void *data, gs_effect_t *effect)
{
	struct transform_filter_data *filter = data;

	if (!obs_source_process_filter_begin(filter->context, GS_RGBA,
				OBS_NO_DIRECT_RENDERING))
		return;

	filter->texwidth = (float)obs_source_get_width(
	obs_filter_get_target(filter->context));
	filter->texheight = (float)obs_source_get_height(
		obs_filter_get_target(filter->context));

	gs_effect_set_float(filter->keystone_H_param,filter->keystone_horizontal/10.0);
	gs_effect_set_float(filter->keystone_V_param,filter->keystone_vertical/10.0);
	gs_effect_set_float(filter->shear_H_param,filter->shear_horizontal/10.0);
	gs_effect_set_float(filter->shear_V_param,filter->shear_vertical/10.0);
	gs_effect_set_float(filter->offset_H_param,filter->offset_horizontal/10.0);
	gs_effect_set_float(filter->offset_V_param,filter->offset_vertical/10.0);
	gs_effect_set_float(filter->scale_H_param,filter->h_scale/10.0);
	gs_effect_set_float(filter->scale_V_param,filter->v_scale/10.0);
	gs_effect_set_float(filter->texture_height,filter->texheight/10.0);
	gs_effect_set_float(filter->texture_width,filter->texwidth/10.0);

	obs_source_process_filter_end(filter->context, filter->effect, 0,0);

	UNUSED_PARAMETER(effect);
}

static obs_properties_t *transform_filter_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

    float accuracy = 0.01f;
    
	obs_properties_add_float_slider(
		props, S_HORIZONTALKS, T_HORIZONTALKS,-10.0f,10.0f,accuracy);
	obs_properties_add_float_slider(
		props, S_VERTICALKS, T_VERTICALKS,-10.0f,10.0f,accuracy);
    obs_properties_add_float_slider(
		props, S_HORIZONTALSHEAR, T_HORIZONTALSHEAR,-10.0f,10.0f,accuracy);
	obs_properties_add_float_slider(
		props, S_VERTICALSHEAR, T_VERTICALSHEAR,-10.0f,10.0f,accuracy);
    
	obs_properties_add_float_slider(
		props, S_VERTICALOFFSET, T_VERTICALOFFSET,-10.0f,10.0f,accuracy);
	obs_properties_add_float_slider(
		props, S_HORIZONTALOFFSET, T_HORIZONTALOFFSET,-10.0f,10.0f,accuracy);
	obs_properties_add_float_slider(
		props, S_HORIZONTALSCALE, T_HORIZONTALSCALE,0.0,20.0f,accuracy);
    obs_properties_add_float_slider(
		props, S_VERTICALSCALE, T_VERTICALSCALE,0.0,20.0f,accuracy);


	UNUSED_PARAMETER(data);
	return props;
}

static void transform_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_double(settings, S_HORIZONTALKS, 0.0);
	obs_data_set_default_double(settings, S_VERTICALKS, 0.0);
	obs_data_set_default_double(settings, S_HORIZONTALSHEAR, 0.0);
	obs_data_set_default_double(settings, S_VERTICALSHEAR, 0.0);
	obs_data_set_default_double(settings, S_HORIZONTALOFFSET, 0.0);
	obs_data_set_default_double(settings, S_VERTICALOFFSET, 0.0);
	obs_data_set_default_double(settings, S_VERTICALSCALE, 10.0);
	obs_data_set_default_double(settings, S_HORIZONTALSCALE, 10.0);
}

struct obs_source_info transform_filter = {
	.id                     = "transform_filter",
	.type                   = OBS_SOURCE_TYPE_FILTER,
	.output_flags           = OBS_SOURCE_VIDEO,
	.get_name               = transform_filter_name,
	.create                 = transform_filter_create,
	.destroy                = transform_filter_destroy,
	.video_render           = transform_filter_render,
	.update                 = transform_filter_update,
	.get_properties         = transform_filter_properties,
	.get_defaults            = transform_filter_defaults,
};
