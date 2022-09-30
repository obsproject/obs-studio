#include <obs-module.h>
#include <obs-source.h>
#include <obs.h>
#include <util/platform.h>

struct sharpness_data {
	obs_source_t *context;

	gs_effect_t *effect;
	gs_eparam_t *sharpness_param;
	gs_eparam_t *texture_width, *texture_height;

	float sharpness;
	float texwidth, texheight;
};

static const char *sharpness_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("SharpnessFilter");
}

static void sharpness_update(void *data, obs_data_t *settings)
{
	struct sharpness_data *filter = data;
	double sharpness = obs_data_get_double(settings, "sharpness");

	filter->sharpness = (float)sharpness;
}

static void sharpness_destroy(void *data)
{
	struct sharpness_data *filter = data;

	if (filter->effect) {
		obs_enter_graphics();
		gs_effect_destroy(filter->effect);
		obs_leave_graphics();
	}

	bfree(data);
}

static void *sharpness_create(obs_data_t *settings, obs_source_t *context)
{
	struct sharpness_data *filter = bzalloc(sizeof(struct sharpness_data));
	char *effect_path = obs_module_file("sharpness.effect");

	filter->context = context;

	obs_enter_graphics();

	filter->effect = gs_effect_create_from_file(effect_path, NULL);
	if (filter->effect) {
		filter->sharpness_param = gs_effect_get_param_by_name(
			filter->effect, "sharpness");
		filter->texture_width = gs_effect_get_param_by_name(
			filter->effect, "texture_width");
		filter->texture_height = gs_effect_get_param_by_name(
			filter->effect, "texture_height");
	}

	obs_leave_graphics();

	bfree(effect_path);

	if (!filter->effect) {
		sharpness_destroy(filter);
		return NULL;
	}

	sharpness_update(filter, settings);
	return filter;
}

static void sharpness_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);

	struct sharpness_data *filter = data;

	const enum gs_color_space preferred_spaces[] = {
		GS_CS_SRGB,
		GS_CS_SRGB_16F,
		GS_CS_709_EXTENDED,
	};

	const enum gs_color_space source_space = obs_source_get_color_space(
		obs_filter_get_target(filter->context),
		OBS_COUNTOF(preferred_spaces), preferred_spaces);
	if (source_space == GS_CS_709_EXTENDED) {
		obs_source_skip_video_filter(filter->context);
	} else {
		const enum gs_color_format format =
			gs_get_format_from_space(source_space);
		if (obs_source_process_filter_begin_with_color_space(
			    filter->context, format, source_space,
			    OBS_ALLOW_DIRECT_RENDERING)) {
			filter->texwidth = (float)obs_source_get_width(
				obs_filter_get_target(filter->context));
			filter->texheight = (float)obs_source_get_height(
				obs_filter_get_target(filter->context));

			gs_effect_set_float(filter->sharpness_param,
					    filter->sharpness);
			gs_effect_set_float(filter->texture_width,
					    filter->texwidth);
			gs_effect_set_float(filter->texture_height,
					    filter->texheight);

			gs_blend_state_push();
			gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);

			obs_source_process_filter_end(filter->context,
						      filter->effect, 0, 0);

			gs_blend_state_pop();
		}
	}
}

static obs_properties_t *sharpness_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	obs_properties_add_text(props, "sdr_only_info",
				obs_module_text("SdrOnlyInfo"), OBS_TEXT_INFO);
	obs_properties_add_float_slider(props, "sharpness",
					obs_module_text("Sharpness"), 0.0, 1.0,
					0.01);

	UNUSED_PARAMETER(data);
	return props;
}

static void sharpness_defaults(obs_data_t *settings)
{
	obs_data_set_default_double(settings, "sharpness", 0.08);
}

static enum gs_color_space
sharpness_get_color_space(void *data, size_t count,
			  const enum gs_color_space *preferred_spaces)
{
	const enum gs_color_space potential_spaces[] = {
		GS_CS_SRGB,
		GS_CS_SRGB_16F,
		GS_CS_709_EXTENDED,
	};

	struct sharpness_data *const filter = data;
	const enum gs_color_space source_space = obs_source_get_color_space(
		obs_filter_get_target(filter->context),
		OBS_COUNTOF(potential_spaces), potential_spaces);

	return source_space;
}

struct obs_source_info sharpness_filter = {
	.id = "sharpness_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CAP_OBSOLETE,
	.get_name = sharpness_getname,
	.create = sharpness_create,
	.destroy = sharpness_destroy,
	.update = sharpness_update,
	.video_render = sharpness_render,
	.get_properties = sharpness_properties,
	.get_defaults = sharpness_defaults,
};

struct obs_source_info sharpness_filter_v2 = {
	.id = "sharpness_filter",
	.version = 2,
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_SRGB,
	.get_name = sharpness_getname,
	.create = sharpness_create,
	.destroy = sharpness_destroy,
	.update = sharpness_update,
	.video_render = sharpness_render,
	.get_properties = sharpness_properties,
	.get_defaults = sharpness_defaults,
	.video_get_color_space = sharpness_get_color_space,
};
