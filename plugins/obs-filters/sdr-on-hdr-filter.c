#include <obs-module.h>

enum sdr_on_hdr_gamma {
	GAMMA_INVALID,
	GAMMA_SRGB,
	GAMMA_POWER,
};

struct sdr_on_hdr_filter_data {
	obs_source_t *context;

	gs_effect_t *effect;
	gs_eparam_t *param_multiplier;
	gs_eparam_t *param_power;

	enum sdr_on_hdr_gamma gamma;
	float multiplier;
	float power;
};

static const char *sdr_on_hdr_filter_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("SdrOnHdrFilter");
}

static void *sdr_on_hdr_filter_create(obs_data_t *settings, obs_source_t *context)
{
	struct sdr_on_hdr_filter_data *filter = bzalloc(sizeof(*filter));
	char *effect_path = obs_module_file("sdr_on_hdr_filter.effect");

	filter->context = context;

	obs_enter_graphics();
	filter->effect = gs_effect_create_from_file(effect_path, NULL);
	obs_leave_graphics();

	bfree(effect_path);

	if (!filter->effect) {
		bfree(filter);
		return NULL;
	}

	filter->param_multiplier = gs_effect_get_param_by_name(filter->effect, "multiplier");
	filter->param_power = gs_effect_get_param_by_name(filter->effect, "power");

	obs_source_update(context, settings);
	return filter;
}

static void sdr_on_hdr_filter_destroy(void *data)
{
	struct sdr_on_hdr_filter_data *filter = data;

	obs_enter_graphics();
	gs_effect_destroy(filter->effect);
	obs_leave_graphics();

	bfree(filter);
}

static void sdr_on_hdr_filter_update(void *data, obs_data_t *settings)
{
	struct sdr_on_hdr_filter_data *filter = data;

	filter->gamma = obs_data_get_int(settings, "gamma");
	filter->multiplier = (float)obs_data_get_int(settings, "sdr_white_level_nits");
	filter->power = (float)obs_data_get_double(settings, "power");
}

static bool gamma_changed(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
	enum sdr_on_hdr_gamma gamma = obs_data_get_int(settings, "gamma");

	obs_property_set_visible(obs_properties_get(props, "power"), gamma == GAMMA_POWER);

	UNUSED_PARAMETER(p);
	return true;
}

static obs_properties_t *sdr_on_hdr_filter_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	obs_properties_add_text(props, "override_info", obs_module_text("SdrOnHdr.Description"), OBS_TEXT_INFO);

	obs_property_t *p = obs_properties_add_list(props, "gamma", obs_module_text("SdrOnHdr.Gamma"),
						    OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(p, obs_module_text("SdrOnHdr.GammaSrgb"), GAMMA_SRGB);
	obs_property_list_add_int(p, obs_module_text("SdrOnHdr.GammaPower"), GAMMA_POWER);
	obs_property_set_modified_callback(p, gamma_changed);

	p = obs_properties_add_int(props, "sdr_white_level_nits", obs_module_text("SdrOnHdr.SdrWhiteLevel"), 80, 480,
				   1);
	obs_property_int_set_suffix(p, " nits");

	obs_properties_add_float(props, "power", obs_module_text("SdrOnHdr.Power"), 1.8, 2.6, 0.1);

	UNUSED_PARAMETER(data);
	return props;
}

static void sdr_on_hdr_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "gamma", GAMMA_SRGB);
	obs_data_set_default_int(settings, "sdr_white_level_nits", 300);
	obs_data_set_default_double(settings, "power", 2.4);
}

static void sdr_on_hdr_filter_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);

	struct sdr_on_hdr_filter_data *filter = data;

	const enum gs_color_space preferred_spaces[] = {
		GS_CS_SRGB,
		GS_CS_SRGB_16F,
		GS_CS_709_EXTENDED,
	};

	enum gs_color_space source_space = obs_source_get_color_space(obs_filter_get_target(filter->context),
								      OBS_COUNTOF(preferred_spaces), preferred_spaces);
	switch (source_space) {
	case GS_CS_SRGB:
	case GS_CS_SRGB_16F: {
		const enum gs_color_format format = gs_get_format_from_space(source_space);
		if (obs_source_process_filter_begin_with_color_space(filter->context, format, source_space,
								     OBS_NO_DIRECT_RENDERING)) {
			gs_effect_set_float(filter->param_multiplier,
					    filter->multiplier / obs_get_video_sdr_white_level());
			gs_effect_set_float(filter->param_power, filter->power);

			gs_blend_state_push();
			gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);

			const enum sdr_on_hdr_gamma gamma = filter->gamma;
			const char *const tech_name = (gamma == GAMMA_SRGB) ? "GammaSrgb" : "GammaPower";
			obs_source_process_filter_tech_end(filter->context, filter->effect, 0, 0, tech_name);

			gs_blend_state_pop();
		}
		break;
	}
	default:
		obs_source_skip_video_filter(filter->context);
	}
}

static enum gs_color_space sdr_on_hdr_filter_get_color_space(void *data, size_t count,
							     const enum gs_color_space *preferred_spaces)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(count);
	UNUSED_PARAMETER(preferred_spaces);

	return GS_CS_709_EXTENDED;
}

struct obs_source_info sdr_on_hdr_filter = {
	.id = "sdr_on_hdr_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_SRGB,
	.get_name = sdr_on_hdr_filter_get_name,
	.create = sdr_on_hdr_filter_create,
	.destroy = sdr_on_hdr_filter_destroy,
	.update = sdr_on_hdr_filter_update,
	.get_properties = sdr_on_hdr_filter_properties,
	.get_defaults = sdr_on_hdr_filter_defaults,
	.video_render = sdr_on_hdr_filter_render,
	.video_get_color_space = sdr_on_hdr_filter_get_color_space,
};
