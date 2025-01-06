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
	blog(level, "[expander/gate/upward compressor: '%s'] " format, obs_source_get_name(cd->context), ##__VA_ARGS__)

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
#if defined(_M_ARM64)
	#undef S_THRESHOLD
#endif
#define S_THRESHOLD                     "threshold"
#define S_ATTACK_TIME                   "attack_time"
#define S_RELEASE_TIME                  "release_time"
#define S_OUTPUT_GAIN                   "output_gain"
#define S_DETECTOR                      "detector"
#define S_PRESETS                       "presets"
#define S_KNEE                          "knee_width"

#define MT_ obs_module_text
#define TEXT_RATIO                      MT_("Expander.Ratio")
#define TEXT_THRESHOLD                  MT_("Expander.Threshold")
#define TEXT_ATTACK_TIME                MT_("Expander.AttackTime")
#define TEXT_RELEASE_TIME               MT_("Expander.ReleaseTime")
#define TEXT_OUTPUT_GAIN                MT_("Expander.OutputGain")
#define TEXT_DETECTOR                   MT_("Expander.Detector")
#define TEXT_PEAK                       MT_("Expander.Peak")
#define TEXT_RMS                        MT_("Expander.RMS")
#define TEXT_PRESETS                    MT_("Expander.Presets")
#define TEXT_PRESETS_EXP                MT_("Expander.Presets.Expander")
#define TEXT_PRESETS_GATE               MT_("Expander.Presets.Gate")
#define TEXT_KNEE                       MT_("Expander.Knee.Width")

#define MIN_RATIO                       1.0f
#define MAX_RATIO                       20.0f
#define MIN_RATIO_UPW                   0.0f
#define MAX_RATIO_UPW                   1.0f
#define MIN_THRESHOLD_DB                -60.0f
#define MAX_THRESHOLD_DB                0.0f
#define MIN_OUTPUT_GAIN_DB              -32.0f
#define MAX_OUTPUT_GAIN_DB              32.0f
#define MIN_ATK_RLS_MS                  1
#define MAX_RLS_MS                      1000
#define MAX_ATK_MS                      100
#define DEFAULT_AUDIO_BUF_MS            10

#define MS_IN_S                         1000
#define MS_IN_S_F                       ((float)MS_IN_S)

/* clang-format on */

/* -------------------------------------------------------- */

struct expander_data {
	obs_source_t *context;
	float *envelope_buf[MAX_AUDIO_CHANNELS];
	size_t envelope_buf_len;

	float ratio;
	float threshold;
	float attack_gain;
	float release_gain;
	float output_gain;

	size_t num_channels;
	size_t sample_rate;
	float envelope[MAX_AUDIO_CHANNELS];
	float slope;
	int detector;
	float runave[MAX_AUDIO_CHANNELS];
	bool is_gate;
	float *runaverage[MAX_AUDIO_CHANNELS];
	size_t runaverage_len;
	float *gain_db[MAX_AUDIO_CHANNELS];
	size_t gain_db_len;
	float gain_db_buf[MAX_AUDIO_CHANNELS];
	float *env_in;
	size_t env_in_len;
	bool is_upwcomp;
	float knee;
};

enum {
	RMS_DETECT,
	RMS_STILLWELL_DETECT,
	PEAK_DETECT,
	NO_DETECT,
};
/* -------------------------------------------------------- */

static void resize_env_buffer(struct expander_data *cd, size_t len)
{
	cd->envelope_buf_len = len;
	for (int i = 0; i < MAX_AUDIO_CHANNELS; i++)
		cd->envelope_buf[i] = brealloc(cd->envelope_buf[i], cd->envelope_buf_len * sizeof(float));
}

static void resize_runaverage_buffer(struct expander_data *cd, size_t len)
{
	cd->runaverage_len = len;
	for (int i = 0; i < MAX_AUDIO_CHANNELS; i++)
		cd->runaverage[i] = brealloc(cd->runaverage[i], cd->runaverage_len * sizeof(float));
}

static void resize_env_in_buffer(struct expander_data *cd, size_t len)
{
	cd->env_in_len = len;
	cd->env_in = brealloc(cd->env_in, cd->env_in_len * sizeof(float));
}

static void resize_gain_db_buffer(struct expander_data *cd, size_t len)
{
	cd->gain_db_len = len;
	for (int i = 0; i < MAX_AUDIO_CHANNELS; i++)
		cd->gain_db[i] = brealloc(cd->gain_db[i], cd->gain_db_len * sizeof(float));
}

static inline float gain_coefficient(uint32_t sample_rate, float time)
{
	return expf(-1.0f / (sample_rate * time));
}

static const char *expander_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("Expander");
}

static const char *upward_compressor_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("Upward.Compressor");
}

static void expander_defaults(obs_data_t *s)
{
	const char *presets = obs_data_get_string(s, S_PRESETS);
	bool is_expander_preset = true;
	if (strcmp(presets, "gate") == 0)
		is_expander_preset = false;
	obs_data_set_default_string(s, S_PRESETS, is_expander_preset ? "expander" : "gate");
	obs_data_set_default_double(s, S_RATIO, is_expander_preset ? 2.0 : 10.0);
	obs_data_set_default_double(s, S_THRESHOLD, -40.0f);
	obs_data_set_default_int(s, S_ATTACK_TIME, 10);
	obs_data_set_default_int(s, S_RELEASE_TIME, is_expander_preset ? 50 : 125);
	obs_data_set_default_double(s, S_OUTPUT_GAIN, 0.0);
	obs_data_set_default_string(s, S_DETECTOR, "RMS");
}

static void upward_compressor_defaults(obs_data_t *s)
{
	obs_data_set_default_double(s, S_RATIO, 0.5);
	obs_data_set_default_double(s, S_THRESHOLD, -20.0f);
	obs_data_set_default_int(s, S_ATTACK_TIME, 10);
	obs_data_set_default_int(s, S_RELEASE_TIME, 50);
	obs_data_set_default_double(s, S_OUTPUT_GAIN, 0.0);
	obs_data_set_default_string(s, S_DETECTOR, "RMS");
	obs_data_set_default_int(s, S_KNEE, 10);
}

static void expander_update(void *data, obs_data_t *s)
{
	struct expander_data *cd = data;
	if (!cd->is_upwcomp) {
		const char *presets = obs_data_get_string(s, S_PRESETS);
		if (strcmp(presets, "expander") == 0 && cd->is_gate) {
			obs_data_clear(s);
			obs_data_set_string(s, S_PRESETS, "expander");
			expander_defaults(s);
			cd->is_gate = false;
		}
		if (strcmp(presets, "gate") == 0 && !cd->is_gate) {
			obs_data_clear(s);
			obs_data_set_string(s, S_PRESETS, "gate");
			expander_defaults(s);
			cd->is_gate = true;
		}
	}

	const uint32_t sample_rate = audio_output_get_sample_rate(obs_get_audio());
	const size_t num_channels = audio_output_get_channels(obs_get_audio());
	const float attack_time_ms = (float)obs_data_get_int(s, S_ATTACK_TIME);
	const float release_time_ms = (float)obs_data_get_int(s, S_RELEASE_TIME);
	const float output_gain_db = (float)obs_data_get_double(s, S_OUTPUT_GAIN);
	const float knee = cd->is_upwcomp ? (float)obs_data_get_int(s, S_KNEE) : 0.0f;

	cd->ratio = (float)obs_data_get_double(s, S_RATIO);

	cd->threshold = (float)obs_data_get_double(s, S_THRESHOLD);
	cd->attack_gain = gain_coefficient(sample_rate, attack_time_ms / MS_IN_S_F);
	cd->release_gain = gain_coefficient(sample_rate, release_time_ms / MS_IN_S_F);
	cd->output_gain = db_to_mul(output_gain_db);
	cd->num_channels = num_channels;
	cd->sample_rate = sample_rate;
	cd->slope = 1.0f - cd->ratio;
	cd->knee = knee;

	const char *detect_mode = obs_data_get_string(s, S_DETECTOR);
	if (strcmp(detect_mode, "RMS") == 0)
		cd->detector = RMS_DETECT;
	if (strcmp(detect_mode, "peak") == 0)
		cd->detector = PEAK_DETECT;

	size_t sample_len = sample_rate * DEFAULT_AUDIO_BUF_MS / MS_IN_S;
	if (cd->envelope_buf_len == 0)
		resize_env_buffer(cd, sample_len);
	if (cd->runaverage_len == 0)
		resize_runaverage_buffer(cd, sample_len);
	if (cd->env_in_len == 0)
		resize_env_in_buffer(cd, sample_len);
	if (cd->gain_db_len == 0)
		resize_gain_db_buffer(cd, sample_len);
}

static void *compressor_expander_create(obs_data_t *settings, obs_source_t *filter, bool is_compressor)
{
	struct expander_data *cd = bzalloc(sizeof(struct expander_data));
	cd->context = filter;
	for (int i = 0; i < MAX_AUDIO_CHANNELS; i++) {
		cd->runave[i] = 0;
		cd->envelope[i] = 0;
		cd->gain_db_buf[i] = 0;
	}
	cd->is_gate = false;
	const char *presets = obs_data_get_string(settings, S_PRESETS);
	if (strcmp(presets, "gate") == 0)
		cd->is_gate = true;
	cd->is_upwcomp = is_compressor;
	expander_update(cd, settings);
	return cd;
}

static void *expander_create(obs_data_t *settings, obs_source_t *filter)
{
	return compressor_expander_create(settings, filter, false);
}

static void *upward_compressor_create(obs_data_t *settings, obs_source_t *filter)
{
	return compressor_expander_create(settings, filter, true);
}

static void expander_destroy(void *data)
{
	struct expander_data *cd = data;

	for (int i = 0; i < MAX_AUDIO_CHANNELS; i++) {
		bfree(cd->envelope_buf[i]);
		bfree(cd->runaverage[i]);
		bfree(cd->gain_db[i]);
	}
	bfree(cd->env_in);
	bfree(cd);
}

// detection stage
static void analyze_envelope(struct expander_data *cd, float **samples, const uint32_t num_samples)
{
	if (cd->envelope_buf_len < num_samples)
		resize_env_buffer(cd, num_samples);
	if (cd->runaverage_len < num_samples)
		resize_runaverage_buffer(cd, num_samples);
	if (cd->env_in_len < num_samples)
		resize_env_in_buffer(cd, num_samples);

	// 10 ms RMS window
	const float rmscoef = exp2f(-100.0f / cd->sample_rate);

	for (int i = 0; i < MAX_AUDIO_CHANNELS; i++) {
		memset(cd->envelope_buf[i], 0, num_samples * sizeof(cd->envelope_buf[i][0]));
		memset(cd->runaverage[i], 0, num_samples * sizeof(cd->runaverage[i][0]));
	}
	memset(cd->env_in, 0, num_samples * sizeof(cd->env_in[0]));

	for (size_t chan = 0; chan < cd->num_channels; ++chan) {
		if (!samples[chan])
			continue;

		float *envelope_buf = cd->envelope_buf[chan];
		float *runave = cd->runaverage[chan];
		float *env_in = cd->env_in;

		if (cd->detector == RMS_DETECT) {
			runave[0] = rmscoef * cd->runave[chan] + (1 - rmscoef) * powf(samples[chan][0], 2.0f);
			env_in[0] = sqrtf(fmaxf(runave[0], 0));
			for (uint32_t i = 1; i < num_samples; ++i) {
				runave[i] = rmscoef * runave[i - 1] + (1 - rmscoef) * powf(samples[chan][i], 2.0f);
				env_in[i] = sqrtf(runave[i]);
			}
		} else if (cd->detector == PEAK_DETECT) {
			for (uint32_t i = 0; i < num_samples; ++i) {
				runave[i] = powf(samples[chan][i], 2);
				env_in[i] = fabsf(samples[chan][i]);
			}
		}

		cd->runave[chan] = runave[num_samples - 1];
		for (uint32_t i = 0; i < num_samples; ++i)
			envelope_buf[i] = fmaxf(envelope_buf[i], env_in[i]);
		cd->envelope[chan] = cd->envelope_buf[chan][num_samples - 1];
	}
}

static inline void process_sample(size_t idx, float *samples, float *env_buf, float *gain_db, bool is_upwcomp,
				  float channel_gain, float threshold, float slope, float attack_gain,
				  float inv_attack_gain, float release_gain, float inv_release_gain, float output_gain,
				  float knee)
{
	/* --------------------------------- */
	/* gain stage of expansion           */

	float env_db = mul_to_db(env_buf[idx]);
	float diff = threshold - env_db;

	if (is_upwcomp && env_db <= (threshold - 60.0f) / 2)
		diff = env_db + 60.0f > 0 ? env_db + 60.0f : 0.0f;

	float gain = 0.0f;
	float prev_gain = 0.0f;
	// Note that the gain is always >= 0 for the upward compressor
	// but is always <=0 for the expander.
	if (is_upwcomp) {
		prev_gain = idx > 0 ? fmaxf(gain_db[idx - 1], 0) : fmaxf(channel_gain, 0);
		// gain above knee (included for clarity):
		if (env_db >= threshold + knee / 2)
			gain = 0.0f;
		// gain below knee:
		if (threshold - knee / 2 >= env_db)
			gain = slope * diff;
		// gain in knee:
		if (env_db > threshold - knee / 2 && threshold + knee / 2 > env_db)
			gain = slope * powf(diff + knee / 2, 2) / (2.0f * knee);
	} else {
		prev_gain = idx > 0 ? gain_db[idx - 1] : channel_gain;
		gain = diff > 0.0f ? fmaxf(slope * diff, -60.0f) : 0.0f;
	}

	/* --------------------------------- */
	/* ballistics (attack/release)       */

	if (gain > prev_gain)
		gain_db[idx] = attack_gain * prev_gain + inv_attack_gain * gain;
	else
		gain_db[idx] = release_gain * prev_gain + inv_release_gain * gain;

	/* --------------------------------- */
	/* output                            */

	if (!is_upwcomp) {
		gain = db_to_mul(fminf(0, gain_db[idx]));
	} else {
		gain = db_to_mul(gain_db[idx]);
	}

	samples[idx] *= gain * output_gain;
}

// gain stage and ballistics in dB domain
static inline void process_expansion(struct expander_data *cd, float **samples, uint32_t num_samples)
{
	const float attack_gain = cd->attack_gain;
	const float release_gain = cd->release_gain;
	const float inv_attack_gain = 1.0f - attack_gain;
	const float inv_release_gain = 1.0f - release_gain;
	const float threshold = cd->threshold;
	const float slope = cd->slope;
	const float output_gain = cd->output_gain;
	const bool is_upwcomp = cd->is_upwcomp;
	const float knee = cd->knee;

	if (cd->gain_db_len < num_samples)
		resize_gain_db_buffer(cd, num_samples);

	for (size_t i = 0; i < cd->num_channels; i++)
		memset(cd->gain_db[i], 0, num_samples * sizeof(cd->gain_db[i][0]));

	for (size_t chan = 0; chan < cd->num_channels; chan++) {
		float *channel_samples = samples[chan];
		float *env_buf = cd->envelope_buf[chan];
		float *gain_db = cd->gain_db[chan];
		float channel_gain = cd->gain_db_buf[chan];

		for (size_t i = 0; i < num_samples; ++i) {
			process_sample(i, channel_samples, env_buf, gain_db, is_upwcomp, channel_gain, threshold, slope,
				       attack_gain, inv_attack_gain, release_gain, inv_release_gain, output_gain, knee);
		}
		cd->gain_db_buf[chan] = gain_db[num_samples - 1];
	}
}

static struct obs_audio_data *expander_filter_audio(void *data, struct obs_audio_data *audio)
{
	struct expander_data *cd = data;

	const uint32_t num_samples = audio->frames;
	if (num_samples == 0)
		return audio;

	float **samples = (float **)audio->data;

	analyze_envelope(cd, samples, num_samples);
	process_expansion(cd, samples, num_samples);
	return audio;
}

static bool presets_changed(obs_properties_t *props, obs_property_t *prop, obs_data_t *settings)
{
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(prop);
	UNUSED_PARAMETER(settings);
	return true;
}

static obs_properties_t *expander_properties(void *data)
{
	struct expander_data *cd = data;
	obs_properties_t *props = obs_properties_create();
	obs_property_t *p;
	if (!cd->is_upwcomp) {
		obs_property_t *presets = obs_properties_add_list(props, S_PRESETS, TEXT_PRESETS, OBS_COMBO_TYPE_LIST,
								  OBS_COMBO_FORMAT_STRING);
		obs_property_list_add_string(presets, TEXT_PRESETS_EXP, "expander");
		obs_property_list_add_string(presets, TEXT_PRESETS_GATE, "gate");
		obs_property_set_modified_callback(presets, presets_changed);
	}

	p = obs_properties_add_float_slider(props, S_RATIO, TEXT_RATIO, !cd->is_upwcomp ? MIN_RATIO : MIN_RATIO_UPW,
					    !cd->is_upwcomp ? MAX_RATIO : MAX_RATIO_UPW, 0.1);
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
	if (!cd->is_upwcomp) {
		obs_property_t *detect = obs_properties_add_list(props, S_DETECTOR, TEXT_DETECTOR, OBS_COMBO_TYPE_LIST,
								 OBS_COMBO_FORMAT_STRING);
		obs_property_list_add_string(detect, TEXT_RMS, "RMS");
		obs_property_list_add_string(detect, TEXT_PEAK, "peak");
	} else {
		p = obs_properties_add_int_slider(props, S_KNEE, TEXT_KNEE, 0, 20, 1);
		obs_property_float_set_suffix(p, " dB");
	}
	return props;
}

struct obs_source_info expander_filter = {
	.id = "expander_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_AUDIO,
	.get_name = expander_name,
	.create = expander_create,
	.destroy = expander_destroy,
	.update = expander_update,
	.filter_audio = expander_filter_audio,
	.get_defaults = expander_defaults,
	.get_properties = expander_properties,
};

struct obs_source_info upward_compressor_filter = {
	.id = "upward_compressor_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_AUDIO,
	.get_name = upward_compressor_name,
	.create = upward_compressor_create,
	.destroy = expander_destroy,
	.update = expander_update,
	.filter_audio = expander_filter_audio,
	.get_defaults = upward_compressor_defaults,
	.get_properties = expander_properties,
};
