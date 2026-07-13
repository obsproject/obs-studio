#include "captions/caption-filter.h"

#include <obs-module.h>
#include <util/bmem.h>

struct lcs_caption_filter {
	obs_source_t *context;
	bool enabled;
};

static const char *caption_filter_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("LCS.Filter.Name");
}

static void caption_filter_update(void *data, obs_data_t *settings)
{
	struct lcs_caption_filter *filter = data;
	filter->enabled = obs_data_get_bool(settings, "enabled");
}

static void *caption_filter_create(obs_data_t *settings, obs_source_t *source)
{
	struct lcs_caption_filter *filter = bzalloc(sizeof(struct lcs_caption_filter));
	filter->context = source;
	caption_filter_update(filter, settings);
	blog(LOG_INFO, "[Live Caption Studio] Caption filter created");
	return filter;
}

static void caption_filter_destroy(void *data)
{
	bfree(data);
}

static obs_properties_t *caption_filter_properties(void *data)
{
	UNUSED_PARAMETER(data);
	obs_properties_t *props = obs_properties_create();
	obs_properties_add_bool(props, "enabled", obs_module_text("LCS.Filter.Enabled"));
	return props;
}

static void caption_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_bool(settings, "enabled", true);
}

static struct obs_audio_data *caption_filter_audio(void *data, struct obs_audio_data *audio)
{
	struct lcs_caption_filter *filter = data;
	if (!filter->enabled || !audio)
		return audio;

	/* Hook point: forward PCM to EngineRegistry from the C++ side later.
	 * Kept pass-through so the filter is safe to attach today. */
	return audio;
}

static struct obs_source_info caption_filter_info = {
	.id = "lcs_caption_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_AUDIO,
	.get_name = caption_filter_name,
	.create = caption_filter_create,
	.destroy = caption_filter_destroy,
	.update = caption_filter_update,
	.get_properties = caption_filter_properties,
	.get_defaults = caption_filter_defaults,
	.filter_audio = caption_filter_audio,
};

void lcs_register_caption_filter(void)
{
	obs_register_source(&caption_filter_info);
}
