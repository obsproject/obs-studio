#include <stdint.h>
#include <inttypes.h>
#include <math.h>

#include <obs-module.h>
#include <media-io/audio-math.h>
#include <util/platform.h>
#include <util/deque.h>
#include <util/threading.h>

/* -------------------------------------------------------- */

#define do_log(level, format, ...) \
	blog(level, "[compressor: '%s'] " format, obs_source_get_name(cd->context), ##__VA_ARGS__)

#define warn(format, ...) do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...) do_log(LOG_INFO, format, ##__VA_ARGS__)

#ifdef _DEBUG
#define debug(format, ...) do_log(LOG_DEBUG, format, ##__VA_ARGS__)
#else
#define debug(format, ...)
#endif

/* -------------------------------------------------------- */

/* clang-format off */

#define S_RATIO                         "ratio"
#define S_THRESHOLD                     "threshold"
#define S_ATTACK_TIME                   "attack_time"
#define S_RELEASE_TIME                  "release_time"
#define S_OUTPUT_GAIN                   "output_gain"
#define S_SIDECHAIN_SOURCE              "sidechain_source"

#define MT_ obs_module_text
#define TEXT_RATIO                      MT_("Compressor.Ratio")
#define TEXT_THRESHOLD                  MT_("Compressor.Threshold")
#define TEXT_ATTACK_TIME                MT_("Compressor.AttackTime")
#define TEXT_RELEASE_TIME               MT_("Compressor.ReleaseTime")
#define TEXT_OUTPUT_GAIN                MT_("Compressor.OutputGain")
#define TEXT_SIDECHAIN_SOURCE           MT_("Compressor.SidechainSource")

#define MIN_RATIO                       1.0
#define MAX_RATIO                       32.0
#define MIN_THRESHOLD_DB                -60.0
#define MAX_THRESHOLD_DB                0.0f
#define MIN_OUTPUT_GAIN_DB              -32.0
#define MAX_OUTPUT_GAIN_DB              32.0
#define MIN_ATK_RLS_MS                  1
#define MAX_RLS_MS                      1000
#define MAX_ATK_MS                      500
#define DEFAULT_AUDIO_BUF_MS            10

#define MS_IN_S                         1000
#define MS_IN_S_F                       ((float)MS_IN_S)

/* clang-format on */

/* -------------------------------------------------------- */

struct sidechain {
    pthread_mutex_t mutex;
    struct deque data[MAX_AUDIO_CHANNELS];
    float *buf[MAX_AUDIO_CHANNELS];
    char *name;
    obs_weak_source_t *weak_ref;
    uint64_t check_time;
    size_t max_frames;
    size_t num_channels;
};

struct compressor_data {
	obs_source_t *context;
	float *envelope_buf;
	size_t envelope_buf_len;

	float ratio;
	float threshold;
	float attack_gain;
	float release_gain;
	float output_gain;

	size_t num_channels;
	size_t sample_rate;
	float envelope;
	float slope;

	pthread_mutex_t sidechain_update_mutex;

	size_t max_sidechain_frames;

	DARRAY(struct sidechain) sidechains;
};

/* -------------------------------------------------------- */

static inline void get_sidechain_data(struct compressor_data *cd, const uint32_t num_samples)
{
	size_t data_size = cd->envelope_buf_len * sizeof(float);
	if (!data_size)
		return;

	pthread_mutex_lock(&sidechain->mutex);
	if (sidechain->max_frames < num_samples)
		sidechain->max_frames = num_samples;

	if (sidechain->data[0].size < data_size) {
		for (size_t i = 0; i < cd->num_channels; i++) {
			memset(sidechain->buf[i], 0, data_size);
		}
		pthread_mutex_unlock(&sidechain->mutex);
		continue;
	}

	for (size_t i = 0; i < cd->num_channels; i++)
		deque_pop_front(&sidechain->data[i], sidechain->buf[i], data_size);

	pthread_mutex_unlock(&sidechain->mutex);
}

static void resize_env_buffer(struct compressor_data *cd, size_t len)
{
	cd->envelope_buf_len = len;
	cd->envelope_buf = brealloc(cd->envelope_buf, len * sizeof(float));

	for (size_t ix = 0; ix < cd->sidechains.num; ix += 1) {
		struct sidechain *sidechain = &cd->sidechains.array[ix];
		pthread_mutex_lock(&sidechain->mutex);
		for (size_t i = 0; i < cd->num_channels; i++) {

			sidechain->buf[i] = brealloc(sidechain->buf[i], len * sizeof(float));
		}
		pthread_mutex_unlock(&sidechain->mutex);
	}
}

static inline float gain_coefficient(uint32_t sample_rate, float time)
{
	return (float)exp(-1.0f / (sample_rate * time));
}

static const char *compressor_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("Compressor");
}

static void sidechain_capture(void *param, obs_source_t *source, const struct audio_data *audio_data, bool muted)
{
	struct sidechain *sidechain = param;

	UNUSED_PARAMETER(source);

	pthread_mutex_lock(&sidechain->mutex);

	if (sidechain->max_frames < audio_data->frames)
		sidechain->max_frames = audio_data->frames;

	size_t expected_size = sidechain->max_frames * sizeof(float);

	if (!expected_size)
		goto unlock;

	if (sidechain->data[0].size > expected_size * 2) {
		for (size_t i = 0; i < sidechain->num_channels; i++) {
			deque_pop_front(&sidechain->data[i], NULL, expected_size);
		}
	}

	if (muted) {
		for (size_t i = 0; i < sidechain->num_channels; i++) {
			deque_push_back_zero(&sidechain->data[i], audio_data->frames * sizeof(float));
		}
	} else {
		for (size_t i = 0; i < sidechain->num_channels; i++) {
			deque_push_back(&sidechain->data[i], audio_data->data[i], audio_data->frames * sizeof(float));
		}
	}

unlock:
	pthread_mutex_unlock(&sidechain->mutex);
}

static void clear_sidechains(struct compressor_data *cd)
{
	size_t sidechain_count = cd->sidechains.num;
	for (size_t ix = 0; ix < sidechain_count; ix += 1) {
		struct sidechain *sidechain = &cd->sidechains.array[ix];

		obs_source_t *old_source = obs_weak_source_get_source(sidechain->weak_ref);

		if (old_source) {
			obs_source_remove_audio_capture_callback(old_source, sidechain_capture, sidechain);
			obs_source_release(old_source);
		}

		if (sidechain->weak_ref) {
			obs_weak_source_release(sidechain->weak_ref);
			sidechain->weak_ref = NULL;
		}

		if (sidechain->name) {
			bfree(sidechain->name);
			sidechain->name = NULL;
		}

		for (size_t ix = 0; ix < MAX_AUDIO_CHANNELS; ix += 1) {
			deque_free(&sidechain->data[ix]);
			bfree(sidechain->buf[ix]);
		}

		pthread_mutex_destroy(&sidechain->mutex);
	}

	da_clear(cd->sidechains);
}

static void compressor_update(void *data, obs_data_t *s)
{
	struct compressor_data *cd = data;

	const uint32_t sample_rate = audio_output_get_sample_rate(obs_get_audio());
	const size_t num_channels = audio_output_get_channels(obs_get_audio());
	const float attack_time_ms = (float)obs_data_get_int(s, S_ATTACK_TIME);
	const float release_time_ms = (float)obs_data_get_int(s, S_RELEASE_TIME);
	const float output_gain_db = (float)obs_data_get_double(s, S_OUTPUT_GAIN);
	obs_data_array_t *sidechain_names = obs_data_get_array(s, S_SIDECHAIN_SOURCE);
	size_t sidechain_count = obs_data_array_count(sidechain_names);

	cd->ratio = (float)obs_data_get_double(s, S_RATIO);
	cd->threshold = (float)obs_data_get_double(s, S_THRESHOLD);
	cd->attack_gain = gain_coefficient(sample_rate, attack_time_ms / MS_IN_S_F);
	cd->release_gain = gain_coefficient(sample_rate, release_time_ms / MS_IN_S_F);
	cd->output_gain = db_to_mul(output_gain_db);
	cd->num_channels = num_channels;
	cd->sample_rate = sample_rate;
	cd->slope = 1.0f - (1.0f / cd->ratio);

	// reset callbacks and cleanup after ourselves before updating list
	pthread_mutex_lock(&cd->sidechain_update_mutex);
	clear_sidechains(cd);
	pthread_mutex_unlock(&cd->sidechain_update_mutex);

	size_t erased_count = 0;
	for (size_t ix = 0; ix < sidechain_count; ix += 1) {
		obs_data_t *source = obs_data_array_item(sidechain_names, ix);
		const char *sidechain_name = obs_data_get_string(source, "value");

		bool valid_sidechain = *sidechain_name && strcmp(sidechain_name, "none") != 0;
		if (!valid_sidechain) {
			continue;
		}

		pthread_mutex_lock(&cd->sidechain_update_mutex);

		struct sidechain *sidechain = da_push_back_new(cd->sidechains);

		if (pthread_mutex_init(&sidechain->mutex, NULL) != 0) {
			blog(LOG_ERROR, "Failed to create mutex for sidechain");
			da_erase(cd->sidechains, (ix - erased_count));
			erased_count += 1;
		}

		sidechain->num_channels = num_channels;
		sidechain->name = bstrdup(sidechain_name);
		sidechain->weak_ref = NULL;

		pthread_mutex_unlock(&cd->sidechain_update_mutex);
	}
	size_t sample_len = sample_rate * DEFAULT_AUDIO_BUF_MS / MS_IN_S;

	resize_env_buffer(cd, sample_len);
}

static void *compressor_create(obs_data_t *settings, obs_source_t *filter)
{
	struct compressor_data *cd = bzalloc(sizeof(struct compressor_data));
	cd->context = filter;

	if (pthread_mutex_init(&cd->sidechain_update_mutex, NULL) != 0) {
		pthread_mutex_destroy(&cd->sidechain_update_mutex);
		blog(LOG_ERROR, "Failed to create mutex");
		bfree(cd);
		return NULL;
	}

	da_init(cd->sidechains);

	compressor_update(cd, settings);
	return cd;
}

static void compressor_destroy(void *data)
{

	struct compressor_data *cd = data;

	pthread_mutex_destroy(&cd->sidechain_update_mutex);

	clear_sidechains(cd);

	da_free(cd->sidechains);

	bfree(cd->envelope_buf);
	bfree(cd);
}

static void analyze_envelope(struct compressor_data *cd, float **samples, const uint32_t num_samples)
{
	if (cd->envelope_buf_len < num_samples) {
		resize_env_buffer(cd, num_samples);
	}

	const float attack_gain = cd->attack_gain;
	const float release_gain = cd->release_gain;

	memset(cd->envelope_buf, 0, num_samples * sizeof(cd->envelope_buf[0]));
	for (size_t chan = 0; chan < cd->num_channels; ++chan) {
		if (!samples[chan])
			continue;

		float *envelope_buf = cd->envelope_buf;
		float env = cd->envelope;
		for (uint32_t i = 0; i < num_samples; ++i) {
			const float env_in = fabsf(samples[chan][i]);
			if (env < env_in) {
				env = env_in + attack_gain * (env - env_in);
			} else {
				env = env_in + release_gain * (env - env_in);
			}
			envelope_buf[i] = fmaxf(envelope_buf[i], env);
		}
	}
	cd->envelope = cd->envelope_buf[num_samples - 1];
}

static void analyze_sidechain(struct compressor_data *cd, const uint32_t num_samples)
{
	if (cd->envelope_buf_len < num_samples) {
		resize_env_buffer(cd, num_samples);
	}

	get_sidechain_data(cd, num_samples);

	const float attack_gain = cd->attack_gain;
	const float release_gain = cd->release_gain;

	for (size_t chan = 0; chan < cd->num_channels; ++chan) {
		for (size_t sidechain_ix = 0; sidechain_ix < cd->sidechains.num; sidechain_ix += 1) {
			float **sidechain_buf = cd->sidechains.array[sidechain_ix].buf;
			if (!sidechain_buf[chan])
				continue;

			float *envelope_buf = cd->envelope_buf;
			float env = cd->envelope;
			for (uint32_t i = 0; i < num_samples; ++i) {
				const float env_in = fabsf(sidechain_buf[chan][i]);

				if (env < env_in) {
					env = env_in + attack_gain * (env - env_in);
				} else {
					env = env_in + release_gain * (env - env_in);
				}
				envelope_buf[i] = fmaxf(envelope_buf[i], env);
			}
			// we don't need to check any other sidechains if we have successfully found one.
			// TODO(Ben): the goal of this logic is to short-circuit the check, but this logic unfortunately
			// is applied to a /muted/ source too...
			//goto continue_outer;
		}
		//continue_outer:;
	}
	cd->envelope = cd->envelope_buf[num_samples - 1];
}

static inline void process_compression(const struct compressor_data *cd, float **samples, uint32_t num_samples)
{
	for (size_t i = 0; i < num_samples; ++i) {
		const float env_db = mul_to_db(cd->envelope_buf[i]);
		float gain = cd->slope * (cd->threshold - env_db);
		gain = db_to_mul(fminf(0, gain));

		for (size_t c = 0; c < cd->num_channels; ++c) {
			if (samples[c]) {
				samples[c][i] *= gain * cd->output_gain;
			}
		}
	}
}

static void compressor_tick(void *data, float seconds)
{
	UNUSED_PARAMETER(seconds);

	struct compressor_data *cd = data;
	size_t sidechain_count = cd->sidechains.num;
	for (size_t ix = 0; ix < sidechain_count; ix += 1) {
		struct sidechain *sidechain = &cd->sidechains.array[ix];
		// is this a sidechain just created by update()?
		if (sidechain->name && !sidechain->weak_ref) {
			pthread_mutex_lock(&cd->sidechain_update_mutex);

			obs_source_t *source = obs_get_source_by_name(sidechain->name);
			obs_weak_source_t *weak_sidechain = obs_source_get_weak_source(source);

			sidechain->weak_ref = weak_sidechain;

			pthread_mutex_unlock(&cd->sidechain_update_mutex);

			obs_source_add_audio_capture_callback(source, sidechain_capture, sidechain);
			obs_source_release(source);
		}
	}
}

static struct obs_audio_data *compressor_filter_audio(void *data, struct obs_audio_data *audio)
{
	struct compressor_data *cd = data;

	const uint32_t num_samples = audio->frames;
	if (num_samples == 0)
		return audio;

	float **samples = (float **)audio->data;

	pthread_mutex_lock(&cd->sidechain_update_mutex);
	bool has_sidechain = false;
	const size_t sidechain_count = cd->sidechains.num;
	for (size_t ix = 0; ix < sidechain_count; ix += 1) {
		struct sidechain *sidechain = &cd->sidechains.array[ix];
		if (sidechain->weak_ref) {
			has_sidechain = true;
			break;
		}
	}
	pthread_mutex_unlock(&cd->sidechain_update_mutex);

	if (has_sidechain)
		analyze_sidechain(cd, num_samples);
	else
		analyze_envelope(cd, samples, num_samples);

	process_compression(cd, samples, num_samples);
	return audio;
}

static void compressor_defaults(obs_data_t *s)
{
	obs_data_set_default_double(s, S_RATIO, 10.0f);
	obs_data_set_default_double(s, S_THRESHOLD, -18.0f);
	obs_data_set_default_int(s, S_ATTACK_TIME, 6);
	obs_data_set_default_int(s, S_RELEASE_TIME, 60);
	obs_data_set_default_double(s, S_OUTPUT_GAIN, 0.0f);
}

static obs_properties_t *compressor_properties(void *data)
{
	struct compressor_data *cd = data;
	obs_properties_t *props = obs_properties_create();
	obs_source_t *parent = NULL;
	obs_property_t *p;

	if (cd)
		parent = obs_filter_get_parent(cd->context);

	p = obs_properties_add_float_slider(props, S_RATIO, TEXT_RATIO, MIN_RATIO, MAX_RATIO, 0.5);
	obs_property_float_set_suffix(p, ":1");
	p = obs_properties_add_float_slider(props, S_THRESHOLD, TEXT_THRESHOLD, MIN_THRESHOLD_DB, MAX_THRESHOLD_DB,
					    0.1);
	obs_property_float_set_suffix(p, " dB");
	p = obs_properties_add_int_slider(props, S_ATTACK_TIME, TEXT_ATTACK_TIME, MIN_ATK_RLS_MS, MAX_ATK_MS, 1);
	obs_property_int_set_suffix(p, " ms");
	p = obs_properties_add_int_slider(props, S_RELEASE_TIME, TEXT_RELEASE_TIME, MIN_ATK_RLS_MS, MAX_RLS_MS, 1);
	obs_property_int_set_suffix(p, " ms");
	p = obs_properties_add_float_slider(props, S_OUTPUT_GAIN, TEXT_OUTPUT_GAIN, MIN_OUTPUT_GAIN_DB,
					    MAX_OUTPUT_GAIN_DB, 0.1);
	obs_property_float_set_suffix(p, " dB");

	obs_properties_add_editable_list(props, S_SIDECHAIN_SOURCE, TEXT_SIDECHAIN_SOURCE,
					 OBS_EDITABLE_LIST_TYPE_SOURCES, "", "");

	return props;
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
	.video_tick = compressor_tick,
	.get_defaults = compressor_defaults,
	.get_properties = compressor_properties,
};
