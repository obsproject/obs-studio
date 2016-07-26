#include <stdint.h>
#include <inttypes.h>

#include <obs-module.h>
#include <speex/speex_preprocess.h>

/* -------------------------------------------------------- */

#define do_log(level, format, ...) \
	blog(level, "[noise suppress: '%s'] " format, \
			obs_source_get_name(ng->context), ##__VA_ARGS__)

#define warn(format, ...)  do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...)  do_log(LOG_INFO,    format, ##__VA_ARGS__)

#ifdef _DEBUG
#define debug(format, ...) do_log(LOG_DEBUG,   format, ##__VA_ARGS__)
#else
#define debug(format, ...)
#endif

/* -------------------------------------------------------- */

#define S_SUPPRESS_LEVEL                "suppress_level"

#define MT_ obs_module_text
#define TEXT_SUPPRESS_LEVEL             MT_("NoiseSuppress.SuppressLevel")

#define MAX_PREPROC_CHANNELS            2

/* -------------------------------------------------------- */

struct noise_suppress_data {
	obs_source_t *context;

	/* Speex preprocessor state */
	SpeexPreprocessState *states[MAX_PREPROC_CHANNELS];

	/* 16 bit PCM buffers */
	spx_int16_t *segment_buffers[MAX_PREPROC_CHANNELS];

	int suppress_level;
};

/* -------------------------------------------------------- */

#define SUP_MIN -60
#define SUP_MAX 0

static const float c_32_to_16 = (float)INT16_MAX;
static const float c_16_to_32 = ((float)INT16_MAX + 1.0f);

/* -------------------------------------------------------- */

static const char *noise_suppress_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("NoiseSuppress");
}

static void noise_suppress_destroy(void *data)
{
	struct noise_suppress_data *ng = data;

	for (size_t i = 0; i < MAX_PREPROC_CHANNELS; i++) {
		speex_preprocess_state_destroy(ng->states[i]);
		bfree(ng->segment_buffers[i]);
	}

	bfree(ng);
}

static inline void alloc_channel(struct noise_suppress_data *ng,
		uint32_t sample_rate, size_t channel, uint32_t size)
{
	if (ng->states[channel])
		return;

	debug("Create channel %d speex state", (int)channel);
	ng->states[channel] = speex_preprocess_state_init(size, sample_rate);

	if (!ng->segment_buffers[channel]) {
		ng->segment_buffers[channel] =
			bzalloc(size * sizeof(spx_int16_t));
		debug("Create channel %d speex buffer: size = %"PRIu32,
				(int)channel, size);
	}
}

static void noise_suppress_update(void *data, obs_data_t *s)
{
	struct noise_suppress_data *ng = data;

	int suppress_level = (int)obs_data_get_int(s, S_SUPPRESS_LEVEL);
	uint32_t sample_rate = audio_output_get_sample_rate(obs_get_audio());
	uint32_t segment_size = sample_rate / 100;
	size_t channels = audio_output_get_channels(obs_get_audio());

	ng->suppress_level = suppress_level;

	debug("channels = %d", (int)channels);
	debug("sample_rate = %"PRIu32, sample_rate);
	debug("segment_size = %u"PRIu32, segment_size);
	debug("block size = %d",
			(int)audio_output_get_block_size(obs_get_audio()));

	/* One speex state for each channel (limit 2) */
	for (size_t i = 0; i < channels; i++)
		alloc_channel(ng, sample_rate, i, segment_size);
}

static void *noise_suppress_create(obs_data_t *settings, obs_source_t *filter)
{
	struct noise_suppress_data *ng =
		bzalloc(sizeof(struct noise_suppress_data));

	ng->context = filter;
	noise_suppress_update(ng, settings);
	return ng;
}

static inline void process_channel(struct noise_suppress_data *ng,
		size_t channel, size_t frames, float *adata[2])
{
	/* Set args */
	speex_preprocess_ctl(ng->states[channel],
			SPEEX_PREPROCESS_SET_NOISE_SUPPRESS,
			&ng->suppress_level);

	/* Convert to 16bit */
	for (size_t i = 0; i < frames; i++)
		ng->segment_buffers[channel][i] =
			(spx_int16_t)(adata[channel][i] * c_32_to_16);

	/* Execute */
	speex_preprocess_run(ng->states[channel], ng->segment_buffers[channel]);

	/* Convert back to 32bit */
	for (size_t i = 0; i < frames; i++)
		adata[channel][i] =
			(float)ng->segment_buffers[channel][i] / c_16_to_32;
}

static struct obs_audio_data *noise_suppress_filter_audio(void *data,
	struct obs_audio_data *audio)
{
	struct noise_suppress_data *ng = data;
	float *adata[2] = {(float*)audio->data[0], (float*)audio->data[1]};

	/* Execute for each available channel */
	for (size_t i = 0; i < MAX_PREPROC_CHANNELS; i++) {
		if (ng->states[i])
			process_channel(ng, i, audio->frames, adata);
	}

	return audio;
}

static void noise_suppress_defaults(obs_data_t *s)
{
	obs_data_set_default_int(s, S_SUPPRESS_LEVEL, -30);
}

static obs_properties_t *noise_suppress_properties(void *data)
{
	obs_properties_t *ppts = obs_properties_create();

	obs_properties_add_int_slider(ppts, S_SUPPRESS_LEVEL,
			TEXT_SUPPRESS_LEVEL, SUP_MIN, SUP_MAX, 0);

	UNUSED_PARAMETER(data);
	return ppts;
}

struct obs_source_info noise_suppress_filter = {
	.id = "noise_suppress_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_AUDIO,
	.get_name = noise_suppress_name,
	.create = noise_suppress_create,
	.destroy = noise_suppress_destroy,
	.update = noise_suppress_update,
	.filter_audio = noise_suppress_filter_audio,
	.get_defaults = noise_suppress_defaults,
	.get_properties = noise_suppress_properties,
};
