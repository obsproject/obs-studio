#include <stdint.h>
#include <inttypes.h>

#include <util/circlebuf.h>
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

#define MAX_PREPROC_CHANNELS            8

/* -------------------------------------------------------- */

struct noise_suppress_data {
	obs_source_t *context;
	int suppress_level;

	uint64_t last_timestamp;

	size_t frames;
	size_t channels;

	struct circlebuf info_buffer;
	struct circlebuf input_buffers[MAX_PREPROC_CHANNELS];
	struct circlebuf output_buffers[MAX_PREPROC_CHANNELS];

	/* Speex preprocessor state */
	SpeexPreprocessState *states[MAX_PREPROC_CHANNELS];

	/* 16 bit PCM buffers */
	float *copy_buffers[MAX_PREPROC_CHANNELS];
	spx_int16_t *segment_buffers[MAX_PREPROC_CHANNELS];

	/* output data */
	struct obs_audio_data output_audio;
	DARRAY(float) output_data;
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

	for (size_t i = 0; i < ng->channels; i++) {
		speex_preprocess_state_destroy(ng->states[i]);
		circlebuf_free(&ng->input_buffers[i]);
		circlebuf_free(&ng->output_buffers[i]);
	}

	bfree(ng->segment_buffers[0]);
	bfree(ng->copy_buffers[0]);
	circlebuf_free(&ng->info_buffer);
	da_free(ng->output_data);
	bfree(ng);
}

static inline void alloc_channel(struct noise_suppress_data *ng,
		uint32_t sample_rate, size_t channel, size_t frames)
{
	ng->states[channel] = speex_preprocess_state_init((int)frames,
			sample_rate);

	circlebuf_reserve(&ng->input_buffers[channel],  frames * sizeof(float));
	circlebuf_reserve(&ng->output_buffers[channel], frames * sizeof(float));
}

static void noise_suppress_update(void *data, obs_data_t *s)
{
	struct noise_suppress_data *ng = data;

	uint32_t sample_rate = audio_output_get_sample_rate(obs_get_audio());
	size_t channels = audio_output_get_channels(obs_get_audio());
	size_t frames = (size_t)sample_rate / 100;

	ng->suppress_level = (int)obs_data_get_int(s, S_SUPPRESS_LEVEL);

	/* Process 10 millisecond segments to keep latency low */
	ng->frames = frames;
	ng->channels = channels;

	/* Ignore if already allocated */
	if (ng->states[0])
		return;

	/* One speex state for each channel (limit 2) */
	ng->copy_buffers[0] = bmalloc(frames * channels * sizeof(float));
	ng->segment_buffers[0] = bmalloc(frames * channels * sizeof(spx_int16_t));
	for (size_t c = 1; c < channels; ++c) {
		ng->copy_buffers[c] = ng->copy_buffers[c-1] + frames;
		ng->segment_buffers[c] = ng->segment_buffers[c-1] + frames;
	}


	for (size_t i = 0; i < channels; i++)
		alloc_channel(ng, sample_rate, i, frames);
}

static void *noise_suppress_create(obs_data_t *settings, obs_source_t *filter)
{
	struct noise_suppress_data *ng =
		bzalloc(sizeof(struct noise_suppress_data));

	ng->context = filter;
	noise_suppress_update(ng, settings);
	return ng;
}

static inline void process(struct noise_suppress_data *ng)
{
	/* Pop from input circlebuf */
	for (size_t i = 0; i < ng->channels; i++)
		circlebuf_pop_front(&ng->input_buffers[i], ng->copy_buffers[i],
				ng->frames * sizeof(float));

	/* Set args */
	for (size_t i = 0; i < ng->channels; i++)
		speex_preprocess_ctl(ng->states[i],
				SPEEX_PREPROCESS_SET_NOISE_SUPPRESS,
				&ng->suppress_level);

	/* Convert to 16bit */
	for (size_t i = 0; i < ng->channels; i++)
		for (size_t j = 0; j < ng->frames; j++) {
			float s = ng->copy_buffers[i][j];
			if (s > 1.0f) s = 1.0f;
			else if (s < -1.0f) s = -1.0f;
			ng->segment_buffers[i][j] = (spx_int16_t)
				(s * c_32_to_16);
		}

	/* Execute */
	for (size_t i = 0; i < ng->channels; i++)
		speex_preprocess_run(ng->states[i], ng->segment_buffers[i]);

	/* Convert back to 32bit */
	for (size_t i = 0; i < ng->channels; i++)
		for (size_t j = 0; j < ng->frames; j++)
			ng->copy_buffers[i][j] =
				(float)ng->segment_buffers[i][j] / c_16_to_32;

	/* Push to output circlebuf */
	for (size_t i = 0; i < ng->channels; i++)
		circlebuf_push_back(&ng->output_buffers[i], ng->copy_buffers[i],
				ng->frames * sizeof(float));
}

struct ng_audio_info {
	uint32_t frames;
	uint64_t timestamp;
};

static inline void clear_circlebuf(struct circlebuf *buf)
{
	circlebuf_pop_front(buf, NULL, buf->size);
}

static void reset_data(struct noise_suppress_data *ng)
{
	for (size_t i = 0; i < ng->channels; i++) {
		clear_circlebuf(&ng->input_buffers[i]);
		clear_circlebuf(&ng->output_buffers[i]);
	}

	clear_circlebuf(&ng->info_buffer);
}

static struct obs_audio_data *noise_suppress_filter_audio(void *data,
	struct obs_audio_data *audio)
{
	struct noise_suppress_data *ng = data;
	struct ng_audio_info info;
	size_t segment_size = ng->frames * sizeof(float);
	size_t out_size;

	if (!ng->states[0])
		return audio;

	/* -----------------------------------------------
	 * if timestamp has dramatically changed, consider it a new stream of
	 * audio data.  clear all circular buffers to prevent old audio data
	 * from being processed as part of the new data. */
	if (ng->last_timestamp) {
		int64_t diff = llabs((int64_t)ng->last_timestamp -
				(int64_t)audio->timestamp);

		if (diff > 1000000000LL)
			reset_data(ng);
	}

	ng->last_timestamp = audio->timestamp;

	/* -----------------------------------------------
	 * push audio packet info (timestamp/frame count) to info circlebuf */
	info.frames = audio->frames;
	info.timestamp = audio->timestamp;
	circlebuf_push_back(&ng->info_buffer, &info, sizeof(info));

	/* -----------------------------------------------
	 * push back current audio data to input circlebuf */
	for (size_t i = 0; i < ng->channels; i++)
		circlebuf_push_back(&ng->input_buffers[i], audio->data[i],
				audio->frames * sizeof(float));

	/* -----------------------------------------------
	 * pop/process each 10ms segments, push back to output circlebuf */
	while (ng->input_buffers[0].size >= segment_size)
		process(ng);

	/* -----------------------------------------------
	 * peek front of info circlebuf, check to see if we have enough to
	 * pop the expected packet size, if not, return null */
	memset(&info, 0, sizeof(info));
	circlebuf_peek_front(&ng->info_buffer, &info, sizeof(info));
	out_size = info.frames * sizeof(float);

	if (ng->output_buffers[0].size < out_size)
		return NULL;

	/* -----------------------------------------------
	 * if there's enough audio data buffered in the output circlebuf,
	 * pop and return a packet */
	circlebuf_pop_front(&ng->info_buffer, NULL, sizeof(info));
	da_resize(ng->output_data, out_size * ng->channels);

	for (size_t i = 0; i < ng->channels; i++) {
		ng->output_audio.data[i] =
			(uint8_t*)&ng->output_data.array[i * out_size];

		circlebuf_pop_front(&ng->output_buffers[i],
				ng->output_audio.data[i],
				out_size);
	}

	ng->output_audio.frames = info.frames;
	ng->output_audio.timestamp = info.timestamp;
	return &ng->output_audio;
}

static void noise_suppress_defaults(obs_data_t *s)
{
	obs_data_set_default_int(s, S_SUPPRESS_LEVEL, -30);
}

static obs_properties_t *noise_suppress_properties(void *data)
{
	obs_properties_t *ppts = obs_properties_create();

	obs_properties_add_int_slider(ppts, S_SUPPRESS_LEVEL,
			TEXT_SUPPRESS_LEVEL, SUP_MIN, SUP_MAX, 1);

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
