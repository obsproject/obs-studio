#include <stdint.h>
#include <inttypes.h>

#include <util/deque.h>
#include <util/threading.h>
#include <obs-module.h>

#ifdef LIBSPEEXDSP_ENABLED
#include <speex/speex_preprocess.h>
#endif

#ifdef LIBRNNOISE_ENABLED
#ifdef _MSC_VER
#define ssize_t intptr_t
#endif
#include <rnnoise.h>
#include <media-io/audio-resampler.h>
#endif

bool nvafx_loaded = false;
#ifdef LIBNVAFX_ENABLED
#include "nvafx-load.h"
#include <pthread.h>
#endif

/* -------------------------------------------------------- */

#define do_log(level, format, ...) \
	blog(level, "[noise suppress: '%s'] " format, obs_source_get_name(ng->context), ##__VA_ARGS__)

#define warn(format, ...) do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...) do_log(LOG_INFO, format, ##__VA_ARGS__)

#ifdef _DEBUG
#define debug(format, ...) do_log(LOG_DEBUG, format, ##__VA_ARGS__)
#else
#define debug(format, ...)
#endif

/* -------------------------------------------------------- */

#define S_SUPPRESS_LEVEL "suppress_level"
#define S_NVAFX_INTENSITY "intensity"
#define S_METHOD "method"
#define S_METHOD_SPEEX "speex"
#define S_METHOD_RNN "rnnoise"
#define S_METHOD_NVAFX_DENOISER "denoiser"
#define S_METHOD_NVAFX_DEREVERB "dereverb"
#define S_METHOD_NVAFX_DEREVERB_DENOISER "dereverb_denoiser"

#define MT_ obs_module_text
#define TEXT_SUPPRESS_LEVEL MT_("NoiseSuppress.SuppressLevel")
#define TEXT_NVAFX_INTENSITY MT_("NoiseSuppress.Intensity")
#define TEXT_METHOD MT_("NoiseSuppress.Method")
#define TEXT_METHOD_SPEEX MT_("NoiseSuppress.Method.Speex")
#define TEXT_METHOD_RNN MT_("NoiseSuppress.Method.RNNoise")
#define TEXT_METHOD_NVAFX_DENOISER MT_("NoiseSuppress.Method.Nvafx.Denoiser")
#define TEXT_METHOD_NVAFX_DEREVERB MT_("NoiseSuppress.Method.Nvafx.Dereverb")
#define TEXT_METHOD_NVAFX_DEREVERB_DENOISER MT_("NoiseSuppress.Method.Nvafx.DenoiserPlusDereverb")
#define TEXT_METHOD_NVAFX_DEPRECATION MT_("NoiseSuppress.Method.Nvafx.Deprecation")
#define TEXT_METHOD_NVAFX_DEPRECATION2 MT_("NoiseSuppress.Method.Nvafx.Deprecation2")

#define MAX_PREPROC_CHANNELS 8

/* RNNoise constants, these can't be changed */
#define RNNOISE_SAMPLE_RATE 48000
#define RNNOISE_FRAME_SIZE 480

/* nvafx constants, these can't be changed */
#define NVAFX_SAMPLE_RATE 48000
#define NVAFX_FRAME_SIZE 480 /* the sdk does not explicitly set this as a constant though it relies on it*/

/* If the following constant changes, RNNoise breaks */
#define BUFFER_SIZE_MSEC 10

/* -------------------------------------------------------- */

struct noise_suppress_data {
	obs_source_t *context;
	int suppress_level;

	uint64_t last_timestamp;
	uint64_t latency;

	size_t frames;
	size_t channels;

	struct deque info_buffer;
	struct deque input_buffers[MAX_PREPROC_CHANNELS];
	struct deque output_buffers[MAX_PREPROC_CHANNELS];

	bool use_rnnoise;
	bool nvafx_enabled;
	bool nvafx_migrated;
#ifdef LIBNVAFX_ENABLED
	obs_source_t *migrated_filter;
#endif
	bool has_mono_src;
	volatile bool reinit_done;
#ifdef LIBSPEEXDSP_ENABLED
	/* Speex preprocessor state */
	SpeexPreprocessState *spx_states[MAX_PREPROC_CHANNELS];
#endif

#ifdef LIBRNNOISE_ENABLED
	/* RNNoise state */
	DenoiseState *rnn_states[MAX_PREPROC_CHANNELS];

	/* Resampler */
	audio_resampler_t *rnn_resampler;
	audio_resampler_t *rnn_resampler_back;
#endif
	/* PCM buffers */
	float *copy_buffers[MAX_PREPROC_CHANNELS];
#ifdef LIBSPEEXDSP_ENABLED
	spx_int16_t *spx_segment_buffers[MAX_PREPROC_CHANNELS];
#endif
#ifdef LIBRNNOISE_ENABLED
	float *rnn_segment_buffers[MAX_PREPROC_CHANNELS];
#endif
	/* output data */
	struct obs_audio_data output_audio;
	DARRAY(float) output_data;
};

/* -------------------------------------------------------- */

#define SUP_MIN -60
#define SUP_MAX 0

#ifdef LIBSPEEXDSP_ENABLED
static const float c_32_to_16 = (float)INT16_MAX;
static const float c_16_to_32 = ((float)INT16_MAX + 1.0f);
#endif

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
#ifdef LIBSPEEXDSP_ENABLED
		speex_preprocess_state_destroy(ng->spx_states[i]);
#endif
#ifdef LIBRNNOISE_ENABLED
		rnnoise_destroy(ng->rnn_states[i]);
#endif
		deque_free(&ng->input_buffers[i]);
		deque_free(&ng->output_buffers[i]);
	}

#ifdef LIBSPEEXDSP_ENABLED
	bfree(ng->spx_segment_buffers[0]);
#endif
#ifdef LIBRNNOISE_ENABLED
	bfree(ng->rnn_segment_buffers[0]);

	if (ng->rnn_resampler) {
		audio_resampler_destroy(ng->rnn_resampler);
		audio_resampler_destroy(ng->rnn_resampler_back);
	}
#endif
	bfree(ng->copy_buffers[0]);
	deque_free(&ng->info_buffer);
	da_free(ng->output_data);
	bfree(ng);
}

static inline void alloc_channel(struct noise_suppress_data *ng, uint32_t sample_rate, size_t channel, size_t frames)
{
#ifdef LIBSPEEXDSP_ENABLED
	ng->spx_states[channel] = speex_preprocess_state_init((int)frames, sample_rate);
#else
	UNUSED_PARAMETER(sample_rate);
#endif
#ifdef LIBRNNOISE_ENABLED
	ng->rnn_states[channel] = rnnoise_create(NULL);
#endif
	deque_reserve(&ng->input_buffers[channel], frames * sizeof(float));
	deque_reserve(&ng->output_buffers[channel], frames * sizeof(float));
}

static inline enum speaker_layout convert_speaker_layout(uint8_t channels)
{
	switch (channels) {
	case 0:
		return SPEAKERS_UNKNOWN;
	case 1:
		return SPEAKERS_MONO;
	case 2:
		return SPEAKERS_STEREO;
	case 3:
		return SPEAKERS_2POINT1;
	case 4:
		return SPEAKERS_4POINT0;
	case 5:
		return SPEAKERS_4POINT1;
	case 6:
		return SPEAKERS_5POINT1;
	case 8:
		return SPEAKERS_7POINT1;
	default:
		return SPEAKERS_UNKNOWN;
	}
}

static void noise_suppress_update(void *data, obs_data_t *s)
{
	struct noise_suppress_data *ng = data;

	uint32_t sample_rate = audio_output_get_sample_rate(obs_get_audio());
	size_t channels = audio_output_get_channels(obs_get_audio());
	size_t frames = (size_t)sample_rate / (1000 / BUFFER_SIZE_MSEC);
	const char *method = obs_data_get_string(s, S_METHOD);

	ng->suppress_level = (int)obs_data_get_int(s, S_SUPPRESS_LEVEL);
	ng->latency = 1000000000LL / (1000 / BUFFER_SIZE_MSEC);
	ng->use_rnnoise = strcmp(method, S_METHOD_RNN) == 0;

	/* Process 10 millisecond segments to keep latency low. */
	/* Also RNNoise only supports buffers of this exact size. */
	ng->frames = frames;
	ng->channels = channels;

	/* Ignore if already allocated */
#if defined(LIBSPEEXDSP_ENABLED)
	if (!ng->use_rnnoise && ng->spx_states[0])
		return;
#endif
#ifdef LIBRNNOISE_ENABLED
	if (ng->use_rnnoise && ng->rnn_states[0])
		return;
#endif
	/* One speex/rnnoise state for each channel (limit 2) */
	ng->copy_buffers[0] = bmalloc(frames * channels * sizeof(float));
#ifdef LIBSPEEXDSP_ENABLED
	ng->spx_segment_buffers[0] = bmalloc(frames * channels * sizeof(spx_int16_t));
#endif
#ifdef LIBRNNOISE_ENABLED
	ng->rnn_segment_buffers[0] = bmalloc(RNNOISE_FRAME_SIZE * channels * sizeof(float));
#endif
	for (size_t c = 1; c < channels; ++c) {
		ng->copy_buffers[c] = ng->copy_buffers[c - 1] + frames;
#ifdef LIBSPEEXDSP_ENABLED
		ng->spx_segment_buffers[c] = ng->spx_segment_buffers[c - 1] + frames;
#endif
#ifdef LIBRNNOISE_ENABLED
		ng->rnn_segment_buffers[c] = ng->rnn_segment_buffers[c - 1] + RNNOISE_FRAME_SIZE;
#endif
	}
	for (size_t i = 0; i < channels; i++)
		alloc_channel(ng, sample_rate, i, frames);

#ifdef LIBRNNOISE_ENABLED
	if (sample_rate == RNNOISE_SAMPLE_RATE) {
		ng->rnn_resampler = NULL;
		ng->rnn_resampler_back = NULL;
	} else {
		struct resample_info src, dst;
		src.samples_per_sec = sample_rate;
		src.format = AUDIO_FORMAT_FLOAT_PLANAR;
		src.speakers = convert_speaker_layout((uint8_t)channels);

		dst.samples_per_sec = RNNOISE_SAMPLE_RATE;
		dst.format = AUDIO_FORMAT_FLOAT_PLANAR;
		dst.speakers = convert_speaker_layout((uint8_t)channels);

		ng->rnn_resampler = audio_resampler_create(&dst, &src);
		ng->rnn_resampler_back = audio_resampler_create(&src, &dst);
	}
#endif
}

static void *noise_suppress_create(obs_data_t *settings, obs_source_t *filter)
{
	struct noise_suppress_data *ng = bzalloc(sizeof(struct noise_suppress_data));

	ng->context = filter;
	ng->nvafx_enabled = false;
	ng->nvafx_migrated = false;
#ifdef LIBNVAFX_ENABLED
	ng->migrated_filter = NULL;
	// If a NVAFX entry is detected, create a new instance of NVAFX filter.
	const char *method = obs_data_get_string(settings, S_METHOD);
	ng->nvafx_enabled = strcmp(method, S_METHOD_NVAFX_DENOISER) == 0 ||
			    strcmp(method, S_METHOD_NVAFX_DEREVERB) == 0 ||
			    strcmp(method, S_METHOD_NVAFX_DEREVERB_DENOISER) == 0;
	if (ng->nvafx_enabled) {
		const char *str1 = obs_source_get_name(filter);
		char *str2 = "_ported";
		char *new_name = (char *)bmalloc(1 + strlen(str1) + strlen(str2));
		strcpy(new_name, str1);
		strcat(new_name, str2);
		obs_data_t *new_settings = obs_data_create();
		obs_data_set_string(new_settings, S_METHOD, method);
		double intensity = obs_data_get_double(settings, S_NVAFX_INTENSITY);
		obs_data_set_double(new_settings, S_NVAFX_INTENSITY, intensity);
		ng->migrated_filter = obs_source_create("nvidia_audiofx_filter", new_name, new_settings, NULL);
		obs_data_release(new_settings);
		bfree(new_name);
	}
#endif
	noise_suppress_update(ng, settings);
	return ng;
}

static inline void process_speexdsp(struct noise_suppress_data *ng)
{
#ifdef LIBSPEEXDSP_ENABLED
	/* Set args */
	for (size_t i = 0; i < ng->channels; i++)
		speex_preprocess_ctl(ng->spx_states[i], SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &ng->suppress_level);

	/* Convert to 16bit */
	for (size_t i = 0; i < ng->channels; i++)
		for (size_t j = 0; j < ng->frames; j++) {
			float s = ng->copy_buffers[i][j];
			if (s > 1.0f)
				s = 1.0f;
			else if (s < -1.0f)
				s = -1.0f;
			ng->spx_segment_buffers[i][j] = (spx_int16_t)(s * c_32_to_16);
		}

	/* Execute */
	for (size_t i = 0; i < ng->channels; i++)
		speex_preprocess_run(ng->spx_states[i], ng->spx_segment_buffers[i]);

	/* Convert back to 32bit */
	for (size_t i = 0; i < ng->channels; i++)
		for (size_t j = 0; j < ng->frames; j++)
			ng->copy_buffers[i][j] = (float)ng->spx_segment_buffers[i][j] / c_16_to_32;
#else
	UNUSED_PARAMETER(ng);
#endif
}

static inline void process_rnnoise(struct noise_suppress_data *ng)
{
#ifdef LIBRNNOISE_ENABLED
	/* Adjust signal level to what RNNoise expects, resample if necessary */
	if (ng->rnn_resampler) {
		float *output[MAX_PREPROC_CHANNELS];
		uint32_t out_frames;
		uint64_t ts_offset;
		audio_resampler_resample(ng->rnn_resampler, (uint8_t **)output, &out_frames, &ts_offset,
					 (const uint8_t **)ng->copy_buffers, (uint32_t)ng->frames);

		for (size_t i = 0; i < ng->channels; i++) {
			for (ssize_t j = 0, k = (ssize_t)out_frames - RNNOISE_FRAME_SIZE; j < RNNOISE_FRAME_SIZE;
			     ++j, ++k) {
				if (k >= 0) {
					ng->rnn_segment_buffers[i][j] = output[i][k] * 32768.0f;
				} else {
					ng->rnn_segment_buffers[i][j] = 0;
				}
			}
		}
	} else {
		for (size_t i = 0; i < ng->channels; i++) {
			for (size_t j = 0; j < RNNOISE_FRAME_SIZE; ++j) {
				ng->rnn_segment_buffers[i][j] = ng->copy_buffers[i][j] * 32768.0f;
			}
		}
	}

	/* Execute */
	for (size_t i = 0; i < ng->channels; i++) {
		rnnoise_process_frame(ng->rnn_states[i], ng->rnn_segment_buffers[i], ng->rnn_segment_buffers[i]);
	}

	/* Revert signal level adjustment, resample back if necessary */
	if (ng->rnn_resampler) {
		float *output[MAX_PREPROC_CHANNELS];
		uint32_t out_frames;
		uint64_t ts_offset;
		audio_resampler_resample(ng->rnn_resampler_back, (uint8_t **)output, &out_frames, &ts_offset,
					 (const uint8_t **)ng->rnn_segment_buffers, RNNOISE_FRAME_SIZE);

		for (size_t i = 0; i < ng->channels; i++) {
			for (ssize_t j = 0, k = (ssize_t)out_frames - ng->frames; j < (ssize_t)ng->frames; ++j, ++k) {
				if (k >= 0) {
					ng->copy_buffers[i][j] = output[i][k] / 32768.0f;
				} else {
					ng->copy_buffers[i][j] = 0;
				}
			}
		}
	} else {
		for (size_t i = 0; i < ng->channels; i++) {
			for (size_t j = 0; j < RNNOISE_FRAME_SIZE; ++j) {
				ng->copy_buffers[i][j] = ng->rnn_segment_buffers[i][j] / 32768.0f;
			}
		}
	}
#else
	UNUSED_PARAMETER(ng);
#endif
}

static inline void process(struct noise_suppress_data *ng)
{
	if (ng->nvafx_enabled)
		return;
	/* Pop from input deque */
	for (size_t i = 0; i < ng->channels; i++)
		deque_pop_front(&ng->input_buffers[i], ng->copy_buffers[i], ng->frames * sizeof(float));

	if (ng->use_rnnoise) {
		process_rnnoise(ng);
	} else {
		process_speexdsp(ng);
	}

	/* Push to output deque */
	for (size_t i = 0; i < ng->channels; i++)
		deque_push_back(&ng->output_buffers[i], ng->copy_buffers[i], ng->frames * sizeof(float));
}

struct ng_audio_info {
	uint32_t frames;
	uint64_t timestamp;
};

static inline void clear_deque(struct deque *buf)
{
	deque_pop_front(buf, NULL, buf->size);
}

static void reset_data(struct noise_suppress_data *ng)
{
	for (size_t i = 0; i < ng->channels; i++) {
		clear_deque(&ng->input_buffers[i]);
		clear_deque(&ng->output_buffers[i]);
	}

	clear_deque(&ng->info_buffer);
}

#ifdef LIBNVAFX_ENABLED
static void noise_suppress_nvafx_migrate_task(void *param)
{
	struct noise_suppress_data *ng = param;
	obs_source_t *parent = obs_filter_get_parent(ng->context);
	int index = obs_source_filter_get_index(parent, ng->context);
	obs_source_filter_add(parent, ng->migrated_filter);
	obs_source_filter_set_index(parent, ng->migrated_filter, index);
	obs_source_set_enabled(ng->migrated_filter, obs_source_enabled(ng->context));
	obs_source_filter_remove(parent, ng->context);
}
#endif

static struct obs_audio_data *noise_suppress_filter_audio(void *data, struct obs_audio_data *audio)
{
	struct noise_suppress_data *ng = data;
	struct ng_audio_info info;
	size_t segment_size = ng->frames * sizeof(float);
	size_t out_size;
	obs_source_t *parent = obs_filter_get_parent(ng->context);
	enum speaker_layout layout = obs_source_get_speaker_layout(parent);
	ng->has_mono_src = layout == SPEAKERS_MONO && ng->channels == 2;
#ifdef LIBNVAFX_ENABLED
	/* Migrate nvafx to new filter. */
	if (ng->nvafx_enabled) {
		if (!ng->nvafx_migrated) {
			obs_queue_task(OBS_TASK_AUDIO, noise_suppress_nvafx_migrate_task, ng, false);
			ng->nvafx_migrated = true;
		}
		return audio;
	}
#endif
#ifdef LIBSPEEXDSP_ENABLED
	if (!ng->spx_states[0])
		return audio;
#endif
#ifdef LIBRNNOISE_ENABLED
	if (!ng->rnn_states[0])
		return audio;
#endif

	/* -----------------------------------------------
	 * if timestamp has dramatically changed, consider it a new stream of
	 * audio data.  clear all circular buffers to prevent old audio data
	 * from being processed as part of the new data. */
	if (ng->last_timestamp) {
		int64_t diff = llabs((int64_t)ng->last_timestamp - (int64_t)audio->timestamp);

		if (diff > 1000000000LL)
			reset_data(ng);
	}

	ng->last_timestamp = audio->timestamp;

	/* -----------------------------------------------
	 * push audio packet info (timestamp/frame count) to info deque */
	info.frames = audio->frames;
	info.timestamp = audio->timestamp;
	deque_push_back(&ng->info_buffer, &info, sizeof(info));

	/* -----------------------------------------------
	 * push back current audio data to input deque */
	for (size_t i = 0; i < ng->channels; i++)
		deque_push_back(&ng->input_buffers[i], audio->data[i], audio->frames * sizeof(float));

	/* -----------------------------------------------
	 * pop/process each 10ms segments, push back to output deque */
	while (ng->input_buffers[0].size >= segment_size)
		process(ng);

	/* -----------------------------------------------
	 * peek front of info deque, check to see if we have enough to
	 * pop the expected packet size, if not, return null */
	memset(&info, 0, sizeof(info));
	deque_peek_front(&ng->info_buffer, &info, sizeof(info));
	out_size = info.frames * sizeof(float);

	if (ng->output_buffers[0].size < out_size)
		return NULL;

	/* -----------------------------------------------
	 * if there's enough audio data buffered in the output deque,
	 * pop and return a packet */
	deque_pop_front(&ng->info_buffer, NULL, sizeof(info));
	da_resize(ng->output_data, out_size * ng->channels);

	for (size_t i = 0; i < ng->channels; i++) {
		ng->output_audio.data[i] = (uint8_t *)&ng->output_data.array[i * out_size];

		deque_pop_front(&ng->output_buffers[i], ng->output_audio.data[i], out_size);
	}

	ng->output_audio.frames = info.frames;
	ng->output_audio.timestamp = info.timestamp - ng->latency;
	return &ng->output_audio;
}

static bool noise_suppress_method_modified(obs_properties_t *props, obs_property_t *property, obs_data_t *settings)
{
	obs_property_t *p_suppress_level = obs_properties_get(props, S_SUPPRESS_LEVEL);
	obs_property_t *p_navfx_intensity = obs_properties_get(props, S_NVAFX_INTENSITY);

	const char *method = obs_data_get_string(settings, S_METHOD);
	bool enable_level = strcmp(method, S_METHOD_SPEEX) == 0;
	bool enable_intensity = strcmp(method, S_METHOD_NVAFX_DENOISER) == 0 ||
				strcmp(method, S_METHOD_NVAFX_DEREVERB) == 0 ||
				strcmp(method, S_METHOD_NVAFX_DEREVERB_DENOISER) == 0;
	obs_property_set_visible(p_suppress_level, enable_level);
	obs_property_set_visible(p_navfx_intensity, enable_intensity);

	UNUSED_PARAMETER(property);
	return true;
}

static void noise_suppress_defaults_v1(obs_data_t *s)
{
	obs_data_set_default_int(s, S_SUPPRESS_LEVEL, -30);
#if defined(LIBRNNOISE_ENABLED) && !defined(LIBSPEEXDSP_ENABLED)
	obs_data_set_default_string(s, S_METHOD, S_METHOD_RNN);
#else
	obs_data_set_default_string(s, S_METHOD, S_METHOD_SPEEX);
#endif
#if defined(LIBNVAFX_ENABLED)
	obs_data_set_default_double(s, S_NVAFX_INTENSITY, 1.0);
#endif
}

static void noise_suppress_defaults_v2(obs_data_t *s)
{
	obs_data_set_default_int(s, S_SUPPRESS_LEVEL, -30);
#if defined(LIBRNNOISE_ENABLED)
	obs_data_set_default_string(s, S_METHOD, S_METHOD_RNN);
#else
	obs_data_set_default_string(s, S_METHOD, S_METHOD_SPEEX);
#endif
#if defined(LIBNVAFX_ENABLED)
	obs_data_set_default_double(s, S_NVAFX_INTENSITY, 1.0);
#endif
}

static obs_properties_t *noise_suppress_properties(void *data)
{
	obs_properties_t *ppts = obs_properties_create();
#ifdef LIBNVAFX_ENABLED
	struct noise_suppress_data *ng = (struct noise_suppress_data *)data;
#else
	UNUSED_PARAMETER(data);
#endif

#if defined(LIBRNNOISE_ENABLED) && defined(LIBSPEEXDSP_ENABLED)
	obs_property_t *method =
		obs_properties_add_list(ppts, S_METHOD, TEXT_METHOD, OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(method, TEXT_METHOD_SPEEX, S_METHOD_SPEEX);
	obs_property_list_add_string(method, TEXT_METHOD_RNN, S_METHOD_RNN);
#ifdef LIBNVAFX_ENABLED
	if (ng->nvafx_enabled) {
		obs_property_list_add_string(method, TEXT_METHOD_NVAFX_DENOISER, S_METHOD_NVAFX_DENOISER);
		obs_property_list_add_string(method, TEXT_METHOD_NVAFX_DEREVERB, S_METHOD_NVAFX_DEREVERB);
		obs_property_list_add_string(method, TEXT_METHOD_NVAFX_DEREVERB_DENOISER,
					     S_METHOD_NVAFX_DEREVERB_DENOISER);
		obs_property_list_item_disable(method, 2, true);
		obs_property_list_item_disable(method, 3, true);
		obs_property_list_item_disable(method, 4, true);
	}

#endif
	obs_property_set_modified_callback(method, noise_suppress_method_modified);
#endif

#ifdef LIBSPEEXDSP_ENABLED
	obs_property_t *speex_slider =
		obs_properties_add_int_slider(ppts, S_SUPPRESS_LEVEL, TEXT_SUPPRESS_LEVEL, SUP_MIN, SUP_MAX, 1);
	obs_property_int_set_suffix(speex_slider, " dB");
#endif

#ifdef LIBNVAFX_ENABLED
	if (ng->nvafx_enabled) {
		obs_properties_add_float_slider(ppts, S_NVAFX_INTENSITY, TEXT_NVAFX_INTENSITY, 0.0f, 1.0f, 0.01f);
		obs_property_t *warning2 = obs_properties_add_text(ppts, "deprecation2", NULL, OBS_TEXT_INFO);
		obs_property_text_set_info_type(warning2, OBS_TEXT_INFO_WARNING);
		obs_property_set_long_description(warning2, TEXT_METHOD_NVAFX_DEPRECATION2);
	}

#if defined(LIBRNNOISE_ENABLED) && defined(LIBSPEEXDSP_ENABLED)
	if (!nvafx_loaded) {
		obs_property_list_item_disable(method, 2, true);
		obs_property_list_item_disable(method, 3, true);
		obs_property_list_item_disable(method, 4, true);
	}
#endif

#endif
	return ppts;
}

struct obs_source_info noise_suppress_filter = {
	.id = "noise_suppress_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_AUDIO | OBS_SOURCE_CAP_OBSOLETE,
	.get_name = noise_suppress_name,
	.create = noise_suppress_create,
	.destroy = noise_suppress_destroy,
	.update = noise_suppress_update,
	.filter_audio = noise_suppress_filter_audio,
	.get_defaults = noise_suppress_defaults_v1,
	.get_properties = noise_suppress_properties,
};

struct obs_source_info noise_suppress_filter_v2 = {
	.id = "noise_suppress_filter",
	.version = 2,
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_AUDIO,
	.get_name = noise_suppress_name,
	.create = noise_suppress_create,
	.destroy = noise_suppress_destroy,
	.update = noise_suppress_update,
	.filter_audio = noise_suppress_filter_audio,
	.get_defaults = noise_suppress_defaults_v2,
	.get_properties = noise_suppress_properties,
};
