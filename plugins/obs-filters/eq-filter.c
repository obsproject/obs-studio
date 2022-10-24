#include <media-io/audio-math.h>
#include <util/circlebuf.h>
#include <util/darray.h>
#include <obs-module.h>

#include <math.h>

#define LOW_FREQ 800.0f
#define HIGH_FREQ 5000.0f

struct eq_channel_state {
	float lf_delay0;
	float lf_delay1;
	float lf_delay2;
	float lf_delay3;

	float hf_delay0;
	float hf_delay1;
	float hf_delay2;
	float hf_delay3;

	float sample_delay1;
	float sample_delay2;
	float sample_delay3;
};

struct eq_data {
	obs_source_t *context;
	size_t channels;
	struct eq_channel_state eqs[MAX_AUDIO_CHANNELS];
	float lf;
	float hf;
	float low_gain;
	float mid_gain;
	float high_gain;
};

static const char *eq_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("3BandEq");
}

static void eq_update(void *data, obs_data_t *settings)
{
	struct eq_data *eq = data;
	eq->low_gain = db_to_mul((float)obs_data_get_double(settings, "low"));
	eq->mid_gain = db_to_mul((float)obs_data_get_double(settings, "mid"));
	eq->high_gain = db_to_mul((float)obs_data_get_double(settings, "high"));
}

static void eq_defaults(obs_data_t *defaults)
{
	obs_data_set_default_double(defaults, "low", 0.0);
	obs_data_set_default_double(defaults, "mid", 0.0);
	obs_data_set_default_double(defaults, "high", 0.0);
}

static obs_properties_t *eq_properties(void *unused)
{
	obs_properties_t *props = obs_properties_create();
	obs_property_t *p;

#define make_db_slider(name)                                                  \
	p = obs_properties_add_float_slider(props, name,                      \
					    obs_module_text("3BandEq." name), \
					    -20.0f, 20.0, 0.1);               \
	obs_property_float_set_suffix(p, " dB");

	make_db_slider("high");
	make_db_slider("mid");
	make_db_slider("low");
#undef make_db_slider

	UNUSED_PARAMETER(unused);
	return props;
}

static void *eq_create(obs_data_t *settings, obs_source_t *filter)
{
	struct eq_data *eq = bzalloc(sizeof(*eq));
	eq->channels = audio_output_get_channels(obs_get_audio());
	eq->context = filter;

	float freq = (float)audio_output_get_sample_rate(obs_get_audio());
	eq->lf = 2.0f * sinf(M_PI * LOW_FREQ / freq);
	eq->hf = 2.0f * sinf(M_PI * HIGH_FREQ / freq);

	eq_update(eq, settings);
	return eq;
}

static void eq_destroy(void *data)
{
	struct eq_data *eq = data;
	bfree(eq);
}

#define EQ_EPSILON (1.0f / 4294967295.0f)

static inline float eq_process(struct eq_data *eq, struct eq_channel_state *c,
			       float sample)
{
	float l, m, h;

	c->lf_delay0 += eq->lf * (sample - c->lf_delay0) + EQ_EPSILON;
	c->lf_delay1 += eq->lf * (c->lf_delay0 - c->lf_delay1);
	c->lf_delay2 += eq->lf * (c->lf_delay1 - c->lf_delay2);
	c->lf_delay3 += eq->lf * (c->lf_delay2 - c->lf_delay3);

	l = c->lf_delay3;

	c->hf_delay0 += eq->hf * (sample - c->hf_delay0) + EQ_EPSILON;
	c->hf_delay1 += eq->hf * (c->hf_delay0 - c->hf_delay1);
	c->hf_delay2 += eq->hf * (c->hf_delay1 - c->hf_delay2);
	c->hf_delay3 += eq->hf * (c->hf_delay2 - c->hf_delay3);

	h = c->sample_delay3 - c->hf_delay3;
	m = c->sample_delay3 - (h + l);

	l *= eq->low_gain;
	m *= eq->mid_gain;
	h *= eq->high_gain;

	c->sample_delay3 = c->sample_delay2;
	c->sample_delay2 = c->sample_delay1;
	c->sample_delay1 = sample;

	return l + m + h;
}

static struct obs_audio_data *eq_filter_audio(void *data,
					      struct obs_audio_data *audio)
{
	struct eq_data *eq = data;
	const uint32_t frames = audio->frames;

	for (size_t c = 0; c < eq->channels; c++) {
		float *adata = (float *)audio->data[c];
		struct eq_channel_state *channel = &eq->eqs[c];

		for (size_t i = 0; i < frames; i++) {
			adata[i] = eq_process(eq, channel, adata[i]);
		}
	}

	return audio;
}

struct obs_source_info eq_filter = {
	.id = "basic_eq_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_AUDIO,
	.get_name = eq_name,
	.create = eq_create,
	.destroy = eq_destroy,
	.update = eq_update,
	.filter_audio = eq_filter_audio,
	.get_defaults = eq_defaults,
	.get_properties = eq_properties,
};
