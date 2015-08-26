#include <media-io/audio-math.h>
#include <obs-module.h>
#include <math.h>

#define do_log(level, format, ...) \
	blog(level, "[compressor: '%s'] " format, \
			obs_source_get_name(cd->context), ##__VA_ARGS__)

#define warn(format, ...)  do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...)  do_log(LOG_INFO,    format, ##__VA_ARGS__)

#define S_UPPER_THRESHOLD              "upper_threshold"
#define S_LOWER_THRESHOLD              "lower_threshold"
#define S_MULTIPLIER                   "multiplier"

#define MT_ obs_module_text
#define TEXT_UPPER_THRESHOLD           MT_("Compressor.UpperThreshold")
#define TEXT_LOWER_THRESHOLD           MT_("Compressor.LowerThreshold")
#define TEXT_MULTIPLIER                MT_("Compressor.Multiplier")

struct compressor_data {
	obs_source_t *context;

	size_t channels;

	float upper_threshold;
	float lower_threshold;
	float multiplier;
};

#define VOL_MIN -96.0f
#define VOL_MAX 0.0f

static const char *compressor_name(void)
{
	return obs_module_text("Compressor");
}

static void compressor_destroy(void *data)
{
	struct compressor_data *cd = data;
	bfree(cd);
}

static void compressor_update(void *data, obs_data_t *s)
{
	struct compressor_data *cd = data;

	float upper_threshold;
	float lower_threshold;
	float multiplier;

	upper_threshold = (float)obs_data_get_double(s, S_UPPER_THRESHOLD);
	lower_threshold = (float)obs_data_get_double(s, S_LOWER_THRESHOLD);
	multiplier = (float)obs_data_get_double(s, S_MULTIPLIER);

	cd->channels = audio_output_get_channels(obs_get_audio());
	cd->upper_threshold = upper_threshold;
	cd->lower_threshold = lower_threshold;
	cd->multiplier = multiplier;
}

static void *compressor_create(obs_data_t *settings, obs_source_t *filter)
{
	struct compressor_data *cd = bzalloc(sizeof(*cd));
	cd->context = filter;
	compressor_update(cd, settings);
	return cd;
}

static struct obs_audio_data *compressor_filter_audio(void *data,
		struct obs_audio_data *audio)
{
	struct compressor_data *cd = data;

	float *adata[2] = {(float*)audio->data[0], (float*)audio->data[1]};
	const size_t channels = cd->channels;
	const float upper_threshold = cd->upper_threshold;
	const float lower_threshold = cd->lower_threshold;
	const float multiplier = cd->multiplier;

	for (size_t i = 0; i < audio->frames; i++) {
		for (size_t c = 0; c < channels; ++c) {
			float over = adata[c][i] - upper_threshold;
			float under = adata[c][i] - lower_threshold;
			if (over > 0.0f)
				adata[c][i] = upper_threshold + multiplier * over;
			if (under < 0.0f)
				adata[c][i] = lower_threshold + multiplier * under;
		}
	}

	return audio;
}

static void compressor_defaults(obs_data_t *s)
{
	obs_data_set_default_double(s, S_UPPER_THRESHOLD, -7.0f);
	obs_data_set_default_double(s, S_LOWER_THRESHOLD, -10.0f);
	obs_data_set_default_double(s, S_MULTIPLIER, 0.25f);
}

static obs_properties_t *compressor_properties(void *data)
{
	obs_properties_t *ppts = obs_properties_create();

	obs_properties_add_float_slider(ppts, S_UPPER_THRESHOLD,
			TEXT_UPPER_THRESHOLD, VOL_MIN, VOL_MAX, 1.0f);
	obs_properties_add_float_slider(ppts, S_LOWER_THRESHOLD,
			TEXT_LOWER_THRESHOLD, VOL_MIN, VOL_MAX, 1.0f);
	obs_properties_add_float_slider(ppts, S_MULTIPLIER,
			TEXT_MULTIPLIER, 0.0f, 10.0f, 1.0f);

	UNUSED_PARAMETER(data);
	return ppts;
}

struct obs_source_info compressor_filter = {
	.id = "compressor_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_AUDIO,
	.get_name = compressor_name,
	.create = compressor_create,
	.destroy = compressor_destroy,
	.update = compressor_update,
	.filter_audio = compressor_filter_audio,
	.get_defaults = compressor_defaults,
	.get_properties = compressor_properties,
};
