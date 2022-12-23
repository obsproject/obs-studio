#include <media-io/audio-math.h>
#include <util/circlebuf.h>
#include <util/darray.h>
#include <obs-module.h>

#include <math.h>

// We will use 7 filters to split a signal s into 8 frequency bands
// |<----------------------------- s ----------------------------->|
// |<-------------s10------------->|<-------------s11------------->|
// |<-----s20----->|<-----s21----->|<-----s22----->|<-----s23----->|
// |<-s30->|<-s31->|<-s32->|<-s33->|<-s34->|<-s35->|<-s36->|<-s37->|
//         f1      f2      f3      f4      f5      f6      f7

struct eq8_channel_state {
	// odd numbered internal signals do not require storage across samples
	// because they are a simple difference of a signal and LPF output
	float s10, s20, s22, s30, s32, s34, s36;
	// second order filter will have 2 first order, so an internal state
	float f10, f20, f22, f30, f32, f34, f36;
};

struct eq8_data {
	obs_source_t *context;
	size_t channels;
	struct eq8_channel_state eqs[MAX_AUDIO_CHANNELS];
	float master;
	// sample frequency
	float freq;
	// user/slider gain settings
	float g[8];
	// internal gains for the 7 Low Pass Filters
	float f1, f2, f3, f4, f5, f6, f7;
};

static const char *eq8_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("Eq8band");
}

static void eq8_update(void *data, obs_data_t *settings)
{
	struct eq8_data *eq = data;
	eq->master = db_to_mul((float)obs_data_get_double(settings, "Master"));
	eq->g[0] = db_to_mul((float)obs_data_get_double(settings, "0-250hz"));
	eq->g[1] = db_to_mul((float)obs_data_get_double(settings, "250-500"));
	eq->g[2] = db_to_mul((float)obs_data_get_double(settings, "500-1k"));
	eq->g[3] = db_to_mul((float)obs_data_get_double(settings, "1k-2k"));
	eq->g[4] = db_to_mul((float)obs_data_get_double(settings, "2k-4k"));
	eq->g[5] = db_to_mul((float)obs_data_get_double(settings, "4k-8k"));
	eq->g[6] = db_to_mul((float)obs_data_get_double(settings, "8k-16k"));
	eq->g[7] = db_to_mul((float)obs_data_get_double(settings, "16khz+"));
}

static void eq8_defaults(obs_data_t *defaults)
{
	obs_data_set_default_double(defaults, "Master", 0.0);
	obs_data_set_default_double(defaults, "16khz+", 0.0);
	obs_data_set_default_double(defaults, "8k-16k", 0.0);
	obs_data_set_default_double(defaults, "4k-8k", 0.0);
	obs_data_set_default_double(defaults, "2k-4k", 0.0);
	obs_data_set_default_double(defaults, "1k-2k", 0.0);
	obs_data_set_default_double(defaults, "500-1k", 0.0);
	obs_data_set_default_double(defaults, "250-500", 0.0);
	obs_data_set_default_double(defaults, "0-250hz", 0.0);
}

static obs_properties_t *eq8_properties(void *unused)
{
	obs_properties_t *props = obs_properties_create();
	obs_property_t *p;

#define make_db_slider(name)                                                  \
	p = obs_properties_add_float_slider(props, name,                      \
					    obs_module_text("Eq8band." name), \
					    -20.0f, 20.0, 0.1);               \
	obs_property_float_set_suffix(p, " dB");

	make_db_slider("Master");
	make_db_slider("16khz+");
	make_db_slider("8k-16k");
	make_db_slider("4k-8k");
	make_db_slider("2k-4k");
	make_db_slider("1k-2k");
	make_db_slider("500-1k");
	make_db_slider("250-500");
	make_db_slider("0-250hz");
#undef make_db_slider

	UNUSED_PARAMETER(unused);
	return props;
}

static void *eq8_create(obs_data_t *settings, obs_source_t *filter)
{
	struct eq8_data *eq = bzalloc(sizeof(*eq));
	eq->channels = audio_output_get_channels(obs_get_audio());
	eq->context = filter;

	// initialize all internal filter and signal state to 0.0
	for (int i = 0; i < MAX_AUDIO_CHANNELS; i++) {
		struct eq8_channel_state *s = &(eq->eqs[i]);
		s->s10 = 0.0;
		s->s20 = 0.0;
		s->s22 = 0.0;
		s->s30 = 0.0;
		s->s32 = 0.0;
		s->s34 = 0.0;
		s->s36 = 0.0;
		s->f10 = 0.0;
		s->f20 = 0.0;
		s->f22 = 0.0;
		s->f30 = 0.0;
		s->f32 = 0.0;
		s->f34 = 0.0;
		s->f36 = 0.0;
	}

	float f = (float)audio_output_get_sample_rate(obs_get_audio());
	eq->freq = f;
	// compute internal filter gains at each cross over frequency
	eq->f1 = (float)(1.0 - exp(-2 * 3.14159265 * 250.0 / f));
	eq->f2 = (float)(1.0 - exp(-2 * 3.14159265 * 500.0 / f));
	eq->f3 = (float)(1.0 - exp(-2 * 3.14159265 * 1000.0 / f));
	eq->f4 = (float)(1.0 - exp(-2 * 3.14159265 * 2000.0 / f));
	eq->f5 = (float)(1.0 - exp(-2 * 3.14159265 * 4000.0 / f));
	eq->f6 = (float)(1.0 - exp(-2 * 3.14159265 * 8000.0 / f));
	eq->f7 = (float)(1.0 - exp(-2 * 3.14159265 * 16000.0 / f));

	eq8_update(eq, settings);
	return eq;
}

static void eq8_destroy(void *data)
{
	struct eq_data *eq = data;
	bfree(eq);
}

static inline float eq8_process(struct eq8_data *eq,
				struct eq8_channel_state *c, float sample)
{
	float s11, s21, s23, s31, s33, s35, s37;

	float s = sample;
	// first LPF / HPF combination
	c->f10 += (s - c->f10) * eq->f4;      // first order filter
	c->s10 += (c->f10 - c->s10) * eq->f4; // second * filter output s10
	s11 = s - c->s10;

	// second layer of filters
	c->f20 += (c->s10 - c->f20) * eq->f2;
	c->s20 += (c->f20 - c->s20) * eq->f2;

	c->f22 += (s11 - c->f22) * eq->f6;
	c->s22 += (c->f22 - c->s22) * eq->f6;

	s21 = c->s10 - c->s20;
	s23 = s11 - c->s22;

	// third filter layer
	c->f30 += (c->s20 - c->f30) * eq->f1;
	c->s30 += (c->f30 - c->s30) * eq->f1;

	c->f32 += (s21 - c->f32) * eq->f3;
	c->s32 += (c->f32 - c->s32) * eq->f3;

	c->f34 += (c->s22 - c->f34) * eq->f5;
	c->s34 += (c->f34 - c->s34) * eq->f5;

	c->f36 += (s23 - c->f36) * eq->f7;
	c->s36 += (c->f36 - c->s36) * eq->f7;

	s31 = c->s20 - c->s30;
	s33 = s21 - c->s32;
	s35 = c->s22 - c->s34;
	s37 = s23 - c->s36;

	// apply pass-band gains and sum
	float out = c->s30 * eq->g[0] + s31 * eq->g[1] + c->s32 * eq->g[2];
	out += s33 * eq->g[3] + c->s34 * eq->g[4] + s35 * eq->g[5];
	out += c->s36 * eq->g[6] + s37 * eq->g[7];

	return (out * eq->master);
}

static struct obs_audio_data *eq8_filter_audio(void *data,
					       struct obs_audio_data *audio)
{
	struct eq8_data *eq = data;
	const uint32_t frames = audio->frames;

	for (size_t c = 0; c < eq->channels; c++) {
		float *adata = (float *)audio->data[c];
		struct eq8_channel_state *channel = &eq->eqs[c];

		for (size_t i = 0; i < frames; i++) {
			adata[i] = eq8_process(eq, channel, adata[i]);
		}
	}

	return audio;
}

struct obs_source_info eq8_filter = {
	.id = "basic_eq8_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_AUDIO,
	.get_name = eq8_name,
	.create = eq8_create,
	.destroy = eq8_destroy,
	.update = eq8_update,
	.filter_audio = eq8_filter_audio,
	.get_defaults = eq8_defaults,
	.get_properties = eq8_properties,
};
