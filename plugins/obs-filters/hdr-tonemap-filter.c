#include <obs-module.h>

enum hdr_tonemap_transform {
	TRANSFORM_SDR_REINHARD,
	TRANSFORM_HDR_MAXRGB,
};

struct hdr_tonemap_filter_data {
	obs_source_t *context;

	gs_effect_t *effect;
	gs_eparam_t *param_multiplier;
	gs_eparam_t *param_hdr_input_maximum_nits;
	gs_eparam_t *param_hdr_output_maximum_nits;

	enum hdr_tonemap_transform transform;
	float sdr_white_level_nits_i;
	float hdr_input_maximum_nits;
	float hdr_output_maximum_nits;
};

static const char *hdr_tonemap_filter_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("HdrTonemapFilter");
}

static void *hdr_tonemap_filter_create(obs_data_t *settings,
				       obs_source_t *context)
{
	struct hdr_tonemap_filter_data *filter = bzalloc(sizeof(*filter));
	char *effect_path = obs_module_file("hdr_tonemap_filter.effect");

	filter->context = context;

	obs_enter_graphics();
	filter->effect = gs_effect_create_from_file(effect_path, NULL);
	obs_leave_graphics();

	bfree(effect_path);

	if (!filter->effect) {
		bfree(filter);
		return NULL;
	}

	filter->param_multiplier =
		gs_effect_get_param_by_name(filter->effect, "multiplier");
	filter->param_hdr_input_maximum_nits = gs_effect_get_param_by_name(
		filter->effect, "hdr_input_maximum_nits");
	filter->param_hdr_output_maximum_nits = gs_effect_get_param_by_name(
		filter->effect, "hdr_output_maximum_nits");

	obs_source_update(context, settings);
	return filter;
}

static void hdr_tonemap_filter_destroy(void *data)
{
	struct hdr_tonemap_filter_data *filter = data;

	obs_enter_graphics();
	gs_effect_destroy(filter->effect);
	obs_leave_graphics();

	bfree(filter);
}

static void hdr_tonemap_filter_update(void *data, obs_data_t *settings)
{
	struct hdr_tonemap_filter_data *filter = data;

	filter->transform = obs_data_get_int(settings, "transform");
	filter->sdr_white_level_nits_i =
		1.f / (float)obs_data_get_int(settings, "sdr_white_level_nits");
	filter->hdr_input_maximum_nits =
		(float)obs_data_get_int(settings, "hdr_input_maximum_nits");
	filter->hdr_output_maximum_nits =
		(float)obs_data_get_int(settings, "hdr_output_maximum_nits");
}

static bool transform_changed(obs_properties_t *props, obs_property_t *p,
			      obs_data_t *settings)
{
	enum hdr_tonemap_transform transform =
		obs_data_get_int(settings, "transform");

	const bool reinhard = transform == TRANSFORM_SDR_REINHARD;
	const bool maxrgb = transform == TRANSFORM_HDR_MAXRGB;
	obs_property_set_visible(
		obs_properties_get(props, "sdr_white_level_nits"), reinhard);
	obs_property_set_visible(
		obs_properties_get(props, "hdr_input_maximum_nits"), maxrgb);
	obs_property_set_visible(
		obs_properties_get(props, "hdr_output_maximum_nits"), maxrgb);

	UNUSED_PARAMETER(p);
	return true;
}

static obs_properties_t *hdr_tonemap_filter_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	obs_properties_add_text(props, "override_info",
				obs_module_text("HdrTonemap.Description"),
				OBS_TEXT_INFO);

	obs_property_t *p = obs_properties_add_list(
		props, "transform", obs_module_text("HdrTonemap.ToneTransform"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(p, obs_module_text("HdrTonemap.SdrReinhard"),
				  TRANSFORM_SDR_REINHARD);
	obs_property_list_add_int(p, obs_module_text("HdrTonemap.HdrMaxrgb"),
				  TRANSFORM_HDR_MAXRGB);
	obs_property_set_modified_callback(p, transform_changed);

	p = obs_properties_add_int(props, "sdr_white_level_nits",
				   obs_module_text("HdrTonemap.SdrWhiteLevel"),
				   80, 480, 1);
	obs_property_int_set_suffix(p, " nits");
	p = obs_properties_add_int(
		props, "hdr_input_maximum_nits",
		obs_module_text("HdrTonemap.HdrInputMaximum"), 5, 10000, 1);
	obs_property_int_set_suffix(p, " nits");
	p = obs_properties_add_int(
		props, "hdr_output_maximum_nits",
		obs_module_text("HdrTonemap.HdrOutputMaximum"), 5, 10000, 1);
	obs_property_int_set_suffix(p, " nits");

	UNUSED_PARAMETER(data);
	return props;
}

static void hdr_tonemap_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "transform", TRANSFORM_SDR_REINHARD);
	obs_data_set_default_int(settings, "sdr_white_level_nits", 300);
	obs_data_set_default_int(settings, "hdr_input_maximum_nits", 4000);
	obs_data_set_default_int(settings, "hdr_output_maximum_nits", 1000);
}

static void hdr_tonemap_filter_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);

	struct hdr_tonemap_filter_data *filter = data;

	const enum gs_color_space preferred_spaces[] = {
		GS_CS_SRGB,
		GS_CS_SRGB_16F,
		GS_CS_709_EXTENDED,
	};

	enum gs_color_space source_space = obs_source_get_color_space(
		obs_filter_get_target(filter->context),
		OBS_COUNTOF(preferred_spaces), preferred_spaces);
	switch (source_space) {
	case GS_CS_709_EXTENDED:
	case GS_CS_709_SCRGB: {
		float multiplier = (source_space == GS_CS_709_EXTENDED)
					   ? obs_get_video_sdr_white_level()
					   : 80.f;
		multiplier *= (filter->transform == TRANSFORM_SDR_REINHARD)
				      ? filter->sdr_white_level_nits_i
				      : 0.0001f;

		const enum gs_color_format format =
			gs_get_format_from_space(source_space);
		if (obs_source_process_filter_begin_with_color_space(
			    filter->context, format, source_space,
			    OBS_NO_DIRECT_RENDERING)) {
			gs_effect_set_float(filter->param_multiplier,
					    multiplier);
			gs_effect_set_float(
				filter->param_hdr_input_maximum_nits,
				filter->hdr_input_maximum_nits);
			gs_effect_set_float(
				filter->param_hdr_output_maximum_nits,
				filter->hdr_output_maximum_nits);

			gs_blend_state_push();
			gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);

			const char *const tech_name =
				(filter->transform == TRANSFORM_HDR_MAXRGB)
					? "MaxRGB"
					: "Reinhard";
			obs_source_process_filter_tech_end(filter->context,
							   filter->effect, 0, 0,
							   tech_name);

			gs_blend_state_pop();
		}
		break;
	}
	default:
		obs_source_skip_video_filter(filter->context);
	}
}

static enum gs_color_space
hdr_tonemap_filter_get_color_space(void *data, size_t count,
				   const enum gs_color_space *preferred_spaces)
{
	const enum gs_color_space potential_spaces[] = {
		GS_CS_SRGB,
		GS_CS_SRGB_16F,
		GS_CS_709_EXTENDED,
	};

	struct hdr_tonemap_filter_data *const filter = data;
	const enum gs_color_space source_space = obs_source_get_color_space(
		obs_filter_get_target(filter->context),
		OBS_COUNTOF(potential_spaces), potential_spaces);

	enum gs_color_space space = source_space;
	switch (source_space) {
	case GS_CS_709_EXTENDED:
	case GS_CS_709_SCRGB:
		if (filter->transform == TRANSFORM_SDR_REINHARD) {
			space = GS_CS_SRGB;
			for (size_t i = 0; i < count; ++i) {
				if (preferred_spaces[i] != GS_CS_SRGB) {
					space = GS_CS_SRGB_16F;
					break;
				}
			}
		}
	}

	return space;
}

struct obs_source_info hdr_tonemap_filter = {
	.id = "hdr_tonemap_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_SRGB,
	.get_name = hdr_tonemap_filter_get_name,
	.create = hdr_tonemap_filter_create,
	.destroy = hdr_tonemap_filter_destroy,
	.update = hdr_tonemap_filter_update,
	.get_properties = hdr_tonemap_filter_properties,
	.get_defaults = hdr_tonemap_filter_defaults,
	.video_render = hdr_tonemap_filter_render,
	.video_get_color_space = hdr_tonemap_filter_get_color_space,
};
