#include <stdint.h>
#include <inttypes.h>
#include <math.h>

#include <obs-module.h>
#include <media-io/audio-math.h>

/* -------------------------------------------------------- */

#define do_log(level, format, ...) \
	blog(level, "[compressor: '%s'] " format, \
			obs_source_get_name(cd->context), ##__VA_ARGS__)

#define warn(format, ...)  do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...)  do_log(LOG_INFO,    format, ##__VA_ARGS__)

#ifdef _DEBUG
#define debug(format, ...) do_log(LOG_DEBUG,   format, ##__VA_ARGS__)
#else
#define debug(format, ...)
#endif

/* -------------------------------------------------------- */

#define S_RATIO             "ratio"
#define S_THRESHOLD         "threshold"
#define S_ATTACK_TIME       "attack_time"
#define S_RELEASE_TIME      "release_time"
#define S_OUTPUT_GAIN       "output_gain"

#define MT_ obs_module_text
#define TEXT_RATIO          MT_("Compressor.Ratio")
#define TEXT_THRESHOLD      MT_("Compressor.Threshold")
#define TEXT_ATTACK_TIME    MT_("Compressor.AttackTime")
#define TEXT_RELEASE_TIME   MT_("Compressor.ReleaseTime")
#define TEXT_OUTPUT_GAIN    MT_("Compressor.OutputGain")

#define MIN_RATIO           1.0f
#define MAX_RATIO           32.0f
#define MIN_THRESHOLD_DB    -60.0f
#define MAX_THRESHOLD_DB    0.0f
#define MIN_OUTPUT_GAIN_DB  -16.0f
#define MAX_OUTPUT_GAIN_DB  16.0f
#define MIN_ATK_RLS_MS      1
#define MAX_ATK_RLS_MS      100

/* -------------------------------------------------------- */

struct compressor_data {
    obs_source_t *context;

    float ratio;
    float threshold;
    float attack_gain;
    float release_gain;
    float output_gain;

    size_t num_channels;
    float envelope;
    float slope;
};

/* -------------------------------------------------------- */

static const char *compressor_name(void *unused)
{
    UNUSED_PARAMETER(unused);
    return obs_module_text("Compressor");
}

static void compressor_destroy(void *data)
{
    struct compressor_data *cd = data;
    bfree(cd);
}

static inline float gain_coefficient(uint32_t sample_rate, float time)
{
    return (float)exp(-1.0f / (sample_rate * time));
}

static void compressor_update(void *data, obs_data_t *s)
{
    struct compressor_data *cd = data;

    const uint32_t sample_rate = audio_output_get_sample_rate(obs_get_audio());
    const float attack_time_ms = (float)obs_data_get_int(s, S_ATTACK_TIME);
    const float release_time_ms = (float)obs_data_get_int(s, S_RELEASE_TIME);
    const float output_gain_db = (float)obs_data_get_double(s, S_OUTPUT_GAIN);

    cd->ratio = (float)obs_data_get_double(s, S_RATIO);
    cd->threshold = (float)obs_data_get_double(s, S_THRESHOLD);
    cd->attack_gain = gain_coefficient(sample_rate, attack_time_ms/1000.0f);
    cd->release_gain = gain_coefficient(sample_rate, release_time_ms/1000.0f);
    cd->output_gain = db_to_mul(output_gain_db);
    cd->num_channels = audio_output_get_channels(obs_get_audio());
    cd->slope = 1.0f - (1.0f / cd->ratio);
}

static void *compressor_create(obs_data_t *settings, obs_source_t *filter)
{
    struct compressor_data *cd = bzalloc(sizeof(struct compressor_data));
    cd->context = filter;
    compressor_update(cd, settings);
    return cd;
}

static inline void get_peak_envelope(struct compressor_data *cd,
    const float *audio_buf, const size_t audio_len, /* out */ float *env_buf)
{
    for (size_t i = 0; i < audio_len; ++i) {
        const float env_in = fabsf(audio_buf[i]);
        if (cd->envelope < env_in) {
            cd->envelope = env_in + cd->attack_gain * (cd->envelope - env_in);
        }
        else {
            cd->envelope = env_in + cd->release_gain * (cd->envelope - env_in);
        }

        env_buf[i] = cd->envelope;
    }
}

static struct obs_audio_data *compressor_filter_audio(void *data,
    struct obs_audio_data *audio)
{
    struct compressor_data *cd = data;
    const uint32_t num_samples = audio->frames;
    float *samples[2] = {(float*)audio->data[0], (float*)audio->data[1]};
    float *env_buf = bmalloc(cd->num_channels * num_samples * sizeof(float));

    if (cd->num_channels == 2) {
        get_peak_envelope(cd, samples[0], num_samples, &env_buf[0]);
        get_peak_envelope(cd, samples[1], num_samples, &env_buf[num_samples]);

        for (uint32_t i = 0; i < num_samples; ++i) {
            const float peak = fmaxf(env_buf[i], env_buf[num_samples + i]);
            env_buf[i] = peak;
        }

    } else if (cd->num_channels == 1) {
        get_peak_envelope(cd, samples[0], num_samples, env_buf);
    }

    for (size_t i = 0; i < num_samples; ++i) {
        float gain = cd->slope * (cd->threshold - mul_to_db(env_buf[i]));
        gain = db_to_mul(fminf(0, gain));

        for (size_t c = 0; c < cd->num_channels; ++c) {
            if (audio->data[c]) {
                samples[c][i] *= gain * cd->output_gain;
            }
        }
    }

    bfree(env_buf);
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
    obs_properties_t *props = obs_properties_create();

    obs_properties_add_float_slider(props, S_RATIO,
        TEXT_RATIO, MIN_RATIO, MAX_RATIO, 0.5f);
    obs_properties_add_float_slider(props, S_THRESHOLD,
        TEXT_THRESHOLD, MIN_THRESHOLD_DB, MAX_THRESHOLD_DB, 0.1f);
    obs_properties_add_int_slider(props, S_ATTACK_TIME,
        TEXT_ATTACK_TIME, MIN_ATK_RLS_MS, MAX_ATK_RLS_MS, 1);
    obs_properties_add_int_slider(props, S_RELEASE_TIME,
        TEXT_RELEASE_TIME, MIN_ATK_RLS_MS, MAX_ATK_RLS_MS, 1);
    obs_properties_add_float_slider(props, S_OUTPUT_GAIN,
        TEXT_OUTPUT_GAIN, MIN_OUTPUT_GAIN_DB, MAX_OUTPUT_GAIN_DB, 0.1f);

    UNUSED_PARAMETER(data);
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
    .get_defaults = compressor_defaults,
    .get_properties = compressor_properties,
};
