#include <obs-module.h>
#include <media-io/audio-math.h>
#include <math.h>
#include "libebur128/ebur128/ebur128.h"

#define do_log(level, format, ...)                          \
	blog(level, "[normalization filter: '%s'] " format, \
	     obs_source_get_name(nf->context), ##__VA_ARGS__)

#define warn(format, ...) do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...) do_log(LOG_INFO, format, ##__VA_ARGS__)

#define S_NORMALIZATION_DB "db"
#define S_NORMALIZATION_CAP "cap"
#define S_NORMALIZATION_HISTORY "history"

#define MT_ obs_module_text
#define TEXT_NORMALIZATION_DB MT_("Normalization.NormalizationDB")
#define TEXT_NORMALIZATION_CAP MT_("Normalization.NormalizationCAP")
#define TEXT_NORMALIZATION_HISTORY MT_("Normalization.NormalizationHISTORY")

struct normalization_data {
	obs_source_t *context;
	ebur128_state **sts;
	size_t channels;
	uint32_t sample_rate;
	size_t history;
	double target;
	double cap;
};

static const char *normalization_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("Normalization");
}

static void normalization_destroy(void *data)
{
	struct normalization_data *nf = data;
	for (size_t c = 0; c < nf->channels; ++c)
		ebur128_destroy(nf->sts + c);
	bfree(nf->sts);
	bfree(nf);
}

static void normalization_update(void *data, obs_data_t *s)
{
	struct normalization_data *nf = data;
	nf->history = 1000 * obs_data_get_double(s, S_NORMALIZATION_HISTORY);
	for (size_t c = 0; c < nf->channels; ++c)
		ebur128_set_max_history(nf->sts[c], nf->history);
	nf->target = obs_data_get_double(s, S_NORMALIZATION_DB);
	nf->cap = obs_data_get_double(s, S_NORMALIZATION_CAP);
}

static void *normalization_create(obs_data_t *settings, obs_source_t *filter)
{
	struct normalization_data *nf = bzalloc(sizeof(*nf));
	nf->context = filter;
	nf->channels = audio_output_get_channels(obs_get_audio());
	nf->sample_rate = audio_output_get_sample_rate(obs_get_audio());
	nf->sts = bzalloc(nf->channels * sizeof(ebur128_state *));
	for (size_t c = 0; c < nf->channels; ++c)
		nf->sts[c] = ebur128_init(1, nf->sample_rate, EBUR128_MODE_I);
	normalization_update(nf, settings);
	return nf;
}

static struct obs_audio_data *
normalization_filter_audio(void *data, struct obs_audio_data *audio)
{
	struct normalization_data *nf = data;
	const size_t channels = nf->channels;
	float **adata = (float **)audio->data;
	double loudness, gain;
	for (size_t c = 0; c < channels; ++c)
		ebur128_add_frames_float(nf->sts[c], adata[c], audio->frames);
	ebur128_loudness_global_multiple(nf->sts, channels, &loudness);
	if (loudness == loudness && loudness != -HUGE_VAL) {
		gain = nf->target - loudness;
		gain = gain < nf->cap ? gain : nf->cap;
	} else {
		gain = 0;
	}
	for (size_t c = 0; c < channels; ++c) {
		for (size_t i = 0; i < audio->frames; i++)
			adata[c][i] *= db_to_mul(gain);
	}
	return audio;
}

static void normalization_defaults(obs_data_t *s)
{
	obs_data_set_default_double(s, S_NORMALIZATION_DB, -23.0f);
	obs_data_set_default_double(s, S_NORMALIZATION_CAP, 20.0f);
	obs_data_set_default_double(s, S_NORMALIZATION_HISTORY, 10.0f);
}

static obs_properties_t *normalization_properties(void *data)
{
	obs_properties_t *ppts = obs_properties_create();

	obs_properties_add_float_slider(ppts, S_NORMALIZATION_DB,
					TEXT_NORMALIZATION_DB, -60.0, 0.0, 0.1);
	obs_properties_add_float_slider(ppts, S_NORMALIZATION_CAP,
					TEXT_NORMALIZATION_CAP, 0.0, 30.0, 0.1);
	obs_properties_add_float(ppts, S_NORMALIZATION_HISTORY,
				 TEXT_NORMALIZATION_HISTORY, 3.0, 30.0, 0.1);

	UNUSED_PARAMETER(data);
	return ppts;
}

struct obs_source_info normalization_filter = {
	.id = "normalization_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_AUDIO,
	.get_name = normalization_name,
	.create = normalization_create,
	.destroy = normalization_destroy,
	.update = normalization_update,
	.filter_audio = normalization_filter_audio,
	.get_defaults = normalization_defaults,
	.get_properties = normalization_properties,
};
