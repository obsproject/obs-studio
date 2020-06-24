#include <obs-module.h>
#include <media-io/audio-math.h>
#include <media-io/audio-io.h>
#include <math.h>

#define do_log(level, format, ...)                 \
	blog(level, "[channel-matrix filter: '%s'] " format, \
	     obs_source_get_name(cf->context), ##__VA_ARGS__)

#define warn(format, ...) do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...) do_log(LOG_INFO, format, ##__VA_ARGS__)

#define CF_CH_CNT 8

#define S_CH1_BITSET "ch1bits"
#define S_CH2_BITSET "ch2bits"
#define S_CH3_BITSET "ch3bits"
#define S_CH4_BITSET "ch4bits"
#define S_CH5_BITSET "ch5bits"
#define S_CH6_BITSET "ch6bits"
#define S_CH7_BITSET "ch7bits"
#define S_CH8_BITSET "ch8bits"
#define S_DIVIDE_GAIN "divide_gain"

struct cm_data {
	obs_source_t *context;

	uint8_t channel_bitsets[CF_CH_CNT];
	bool divide_gain;

	size_t min_output_channels;

	struct obs_audio_data output_audio;
	DARRAY(float) output_data;
};

static const char *cm_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "Channel Matrix";
}

static void cm_destroy(void *data)
{
	struct cm_data *cf = data;
	da_free(cf->output_data);
	bfree(cf);
}

static void cm_update(void *data, obs_data_t *s)
{
	struct cm_data *cf = data;

	cf->channel_bitsets[0] = (uint8_t)obs_data_get_int(s, S_CH1_BITSET);
	cf->channel_bitsets[1] = (uint8_t)obs_data_get_int(s, S_CH2_BITSET);
	cf->channel_bitsets[2] = (uint8_t)obs_data_get_int(s, S_CH3_BITSET);
	cf->channel_bitsets[3] = (uint8_t)obs_data_get_int(s, S_CH4_BITSET);
	cf->channel_bitsets[4] = (uint8_t)obs_data_get_int(s, S_CH5_BITSET);
	cf->channel_bitsets[5] = (uint8_t)obs_data_get_int(s, S_CH6_BITSET);
	cf->channel_bitsets[6] = (uint8_t)obs_data_get_int(s, S_CH7_BITSET);
	cf->channel_bitsets[7] = (uint8_t)obs_data_get_int(s, S_CH8_BITSET);

	cf->divide_gain = obs_data_get_bool(s, S_DIVIDE_GAIN);

	cf->min_output_channels = audio_output_get_channels(obs_get_audio());
}

static void *cm_create(obs_data_t *settings, obs_source_t *filter)
{
	struct cm_data *cf = bzalloc(sizeof(*cf));
	cf->context = filter;
	cm_update(cf, settings);
	return cf;
}

static struct obs_audio_data *cm_filter_audio(void *data, struct obs_audio_data *audio)
{
	struct cm_data *cf = data;

	// Determine highest actually used output channel
	// and calculate input-channel count per output channel
	size_t max_channel = 0;
	float ic_mult[CF_CH_CNT];
	bool do_ic[CF_CH_CNT][CF_CH_CNT];
	for (int oc = 0; oc < CF_CH_CNT; oc++) {
		int ic_count = 0;

		for (int ic = 0; ic < CF_CH_CNT; ic++) {
			do_ic[oc][ic] = (cf->channel_bitsets[oc] & (1 << ic)) && audio->data[ic];
			if (do_ic[oc][ic]) {
				max_channel = oc;
				ic_count += 1;
			}
		}

		if (cf->divide_gain) {
			ic_mult[oc] = 1.0f / ((float)ic_count);
		} else {
			ic_mult[oc] = 1.0f;
		}
	}

	size_t channels = max_channel + 1;
	if (channels < cf->min_output_channels)
		channels = cf->min_output_channels;

	size_t out_size = audio->frames * sizeof(float);

	da_resize(cf->output_data, out_size * channels);
	memset(cf->output_audio.data, 0, MAX_AUDIO_CHANNELS * sizeof(uint8_t*));

	for (int oc = 0; oc < channels; oc++) {
		cf->output_audio.data[oc] = (uint8_t*)&cf->output_data.array[oc * out_size];
		float *odata = (float*)cf->output_audio.data[oc];

		for (uint32_t i = 0; i < audio->frames; i++) {
			odata[i] = 0.0f;
		}

		for (int ic = 0; ic < CF_CH_CNT; ic++) {
			if (!do_ic[oc][ic])
				continue;

			float *idata = (float*)audio->data[ic];

			for (uint32_t i = 0; i < audio->frames; i++) {
				odata[i] += idata[i] * ic_mult[oc];
			}
		}
	}

	cf->output_audio.frames = audio->frames;
	cf->output_audio.timestamp = audio->timestamp;
	return &cf->output_audio;
}

static void cm_defaults(obs_data_t *s)
{
	obs_data_set_default_int(s, S_CH1_BITSET, 1 << 0);
	obs_data_set_default_int(s, S_CH2_BITSET, 1 << 1);
	obs_data_set_default_int(s, S_CH3_BITSET, 1 << 2);
	obs_data_set_default_int(s, S_CH4_BITSET, 1 << 3);
	obs_data_set_default_int(s, S_CH5_BITSET, 1 << 4);
	obs_data_set_default_int(s, S_CH6_BITSET, 1 << 5);
	obs_data_set_default_int(s, S_CH7_BITSET, 1 << 6);
	obs_data_set_default_int(s, S_CH8_BITSET, 1 << 7);

	obs_data_set_default_bool(s, S_DIVIDE_GAIN, false);
}

static obs_properties_t *cm_properties(void *data)
{
	obs_properties_t *ppts = obs_properties_create();

	obs_properties_add_int(ppts, S_CH1_BITSET, "Output Channel 1", 0, 255, 1);
	obs_properties_add_int(ppts, S_CH2_BITSET, "Output Channel 2", 0, 255, 1);
	obs_properties_add_int(ppts, S_CH3_BITSET, "Output Channel 3", 0, 255, 1);
	obs_properties_add_int(ppts, S_CH4_BITSET, "Output Channel 4", 0, 255, 1);
	obs_properties_add_int(ppts, S_CH5_BITSET, "Output Channel 5", 0, 255, 1);
	obs_properties_add_int(ppts, S_CH6_BITSET, "Output Channel 6", 0, 255, 1);
	obs_properties_add_int(ppts, S_CH7_BITSET, "Output Channel 7", 0, 255, 1);
	obs_properties_add_int(ppts, S_CH8_BITSET, "Output Channel 8", 0, 255, 1);

	obs_properties_add_bool(ppts, S_DIVIDE_GAIN, "Divide Gain");

	UNUSED_PARAMETER(data);
	return ppts;
}

struct obs_source_info channel_matrix_filter = {
	.id = "channel_matrix_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_AUDIO,
	.get_name = cm_name,
	.create = cm_create,
	.destroy = cm_destroy,
	.update = cm_update,
	.filter_audio = cm_filter_audio,
	.get_defaults = cm_defaults,
	.get_properties = cm_properties,
};


OBS_DECLARE_MODULE()
OBS_MODULE_AUTHOR("BtbN")

const char *obs_module_description(void)
{
	return "OBS core filters";
}

bool obs_module_load(void)
{
	obs_register_source(&channel_matrix_filter);
	return true;
}
