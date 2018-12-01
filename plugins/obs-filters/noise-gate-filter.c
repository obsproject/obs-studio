#include <media-io/audio-math.h>
#include <obs-module.h>
#include <math.h>

#define do_log(level, format, ...) \
	blog(level, "[noise gate: '%s'] " format, \
			obs_source_get_name(ng->context), ##__VA_ARGS__)

#define warn(format, ...)  do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...)  do_log(LOG_INFO,    format, ##__VA_ARGS__)

#define S_OPEN_THRESHOLD               "open_threshold"
#define S_CLOSE_THRESHOLD              "close_threshold"
#define S_ATTACK_TIME                  "attack_time"
#define S_HOLD_TIME                    "hold_time"
#define S_RELEASE_TIME                 "release_time"
#define S_PRESET                        "preset"
#define T_CUSTOM                        "{custom}"

#define MT_ obs_module_text
#define TEXT_OPEN_THRESHOLD            MT_("NoiseGate.OpenThreshold")
#define TEXT_CLOSE_THRESHOLD           MT_("NoiseGate.CloseThreshold")
#define TEXT_ATTACK_TIME               MT_("NoiseGate.AttackTime")
#define TEXT_HOLD_TIME                 MT_("NoiseGate.HoldTime")
#define TEXT_RELEASE_TIME              MT_("NoiseGate.ReleaseTime")
#define TEXT_PRESET                    MT_("NoiseGate.Preset")

struct noise_gate_data {
	obs_source_t *context;

	float sample_rate_i;
	size_t channels;

	float open_threshold;
	float close_threshold;
	float decay_rate;
	float attack_rate;
	float release_rate;
	float hold_time;

	bool is_open;
	float attenuation;
	float level;
	float held_time;
};

#define VOL_MIN -96.0
#define VOL_MAX 0.0

static const char *noise_gate_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("NoiseGate");
}

static void noise_gate_destroy(void *data)
{
	struct noise_gate_data *ng = data;
	bfree(ng);
}

static inline float ms_to_secf(int ms)
{
	return (float)ms / 1000.0f;
}

static void noise_gate_update(void *data, obs_data_t *s)
{
	struct noise_gate_data *ng = data;
	float open_threshold_db;
	float close_threshold_db;
	float sample_rate;
	int attack_time_ms;
	int hold_time_ms;
	int release_time_ms;

	open_threshold_db = (float)obs_data_get_double(s, S_OPEN_THRESHOLD);
	close_threshold_db = (float)obs_data_get_double(s, S_CLOSE_THRESHOLD);
	attack_time_ms = (int)obs_data_get_int(s, S_ATTACK_TIME);
	hold_time_ms = (int)obs_data_get_int(s, S_HOLD_TIME);
	release_time_ms = (int)obs_data_get_int(s, S_RELEASE_TIME);
	sample_rate = (float)audio_output_get_sample_rate(obs_get_audio());

	ng->sample_rate_i = 1.0f / sample_rate;
	ng->channels = audio_output_get_channels(obs_get_audio());
	ng->open_threshold = db_to_mul(open_threshold_db);
	ng->close_threshold = db_to_mul(close_threshold_db);
	ng->attack_rate = 1.0f / (ms_to_secf(attack_time_ms) * sample_rate);
	ng->release_rate = 1.0f / (ms_to_secf(release_time_ms) * sample_rate);

	const float threshold_diff = ng->open_threshold - ng->close_threshold;
	const float min_decay_period = (1.0f / 75.0f) * sample_rate;

	ng->decay_rate = threshold_diff / min_decay_period;
	ng->hold_time = ms_to_secf(hold_time_ms);
	ng->is_open = false;
	ng->attenuation = 0.0f;
	ng->level = 0.0f;
	ng->held_time = 0.0f;
}

static void *noise_gate_create(obs_data_t *settings, obs_source_t *filter)
{
	struct noise_gate_data *ng = bzalloc(sizeof(*ng));
	ng->context = filter;
	noise_gate_update(ng, settings);
	return ng;
}

static struct obs_audio_data *noise_gate_filter_audio(void *data,
		struct obs_audio_data *audio)
{
	struct noise_gate_data *ng = data;

	float **adata = (float**)audio->data;
	const float close_threshold = ng->close_threshold;
	const float open_threshold = ng->open_threshold;
	const float sample_rate_i = ng->sample_rate_i;
	const float release_rate = ng->release_rate;
	const float attack_rate = ng->attack_rate;
	const float decay_rate = ng->decay_rate;
	const float hold_time = ng->hold_time;
	const size_t channels = ng->channels;

	for (size_t i = 0; i < audio->frames; i++) {
		float cur_level = fabsf(adata[0][i]);
		for (size_t j = 0; j < channels; j++) {
			cur_level = fmaxf(cur_level, fabsf(adata[j][i]));
		}

		if (cur_level > open_threshold && !ng->is_open) {
			ng->is_open = true;
		}
		if (ng->level < close_threshold && ng->is_open) {
			ng->held_time = 0.0f;
			ng->is_open = false;
		}

		ng->level = fmaxf(ng->level, cur_level) - decay_rate;

		if (ng->is_open) {
			ng->attenuation = fminf(1.0f,
					ng->attenuation + attack_rate);
		} else {
			ng->held_time += sample_rate_i;
			if (ng->held_time > hold_time) {
				ng->attenuation = fmaxf(0.0f,
						ng->attenuation - release_rate);
			}
		}

		for (size_t c = 0; c < channels; c++)
			adata[c][i] *= ng->attenuation;
	}

	return audio;
}

static void noise_gate_defaults(obs_data_t *s)
{
	obs_data_set_default_string(s, S_PRESET, "low noise");

	char *preset_file = obs_module_file("noise_gate_preset.json");
	obs_data_t *presets = obs_data_create_from_json_file_safe(preset_file,
			"bak");
	obs_data_t *default_preset = obs_data_get_obj(presets, "vocals");

	obs_data_item_t *item = NULL;
	for (item = obs_data_first(default_preset); item;
		obs_data_item_next(&item)) {
		enum obs_data_type type = obs_data_item_gettype(item);
		const char *name = obs_data_item_get_name(item);

		if (!obs_data_item_has_user_value(item))
			continue;

		if (type == OBS_DATA_NUMBER) {
			enum obs_data_type n = obs_data_item_numtype(item);
			if (n == OBS_DATA_NUM_DOUBLE) {
				obs_data_set_default_int(s, name,
						obs_data_item_get_int(item));
			} else if (n == OBS_DATA_NUM_INT) {
				obs_data_set_default_double(s, name,
						obs_data_item_get_double(item));
			}
		}
	}

	obs_data_release(default_preset);
	obs_data_release(presets);
	bfree(preset_file);
}

static bool presets_changed(obs_properties_t *props, obs_property_t *prop,
		obs_data_t *settings)
{
	char *preset = bstrdup(obs_data_get_string(settings, S_PRESET));
	char *preset_file = obs_module_file("noise_gate_presets.json");
	obs_data_t *presets = obs_data_create_from_json_file_safe(preset_file,
			"bak");
	obs_data_t *preset_data = NULL;

	obs_property_list_clear(prop);
	obs_property_list_add_string(prop, MT_(T_CUSTOM), T_CUSTOM);

	obs_data_item_t *item = NULL;
	for (item = obs_data_first(presets); item; obs_data_item_next(&item)) {
		enum obs_data_type type = obs_data_item_gettype(item);
		const char *name = obs_data_item_get_name(item);

		if (!obs_data_item_has_user_value(item))
			continue;

		if (type == OBS_DATA_OBJECT)
			obs_property_list_add_string(prop, MT_(name), name);
	}

	if (preset && strcmp(preset, T_CUSTOM) != 0) {
		preset_data = obs_data_get_obj(presets, preset);
		obs_data_erase(preset_data, S_PRESET);
		obs_data_apply(settings, preset_data);
		obs_data_release(preset_data);
	}

	obs_data_release(presets);
	bfree(preset);
	bfree(preset_file);

	return true;
}

static bool settings_changed(obs_properties_t *props, obs_property_t *prop,
		obs_data_t *settings)
{
	obs_data_set_string(settings, S_PRESET, T_CUSTOM);
	return false;
}

static obs_properties_t *noise_gate_properties(void *data)
{
	obs_properties_t *ppts = obs_properties_create();
	obs_property_t *prop = NULL;

	obs_property_t *presets = obs_properties_add_list(ppts, S_PRESET,
			TEXT_PRESET, OBS_COMBO_TYPE_LIST,
			OBS_COMBO_FORMAT_STRING);

	obs_property_set_modified_callback(presets, presets_changed);

	prop = obs_properties_add_float_slider(ppts, S_CLOSE_THRESHOLD,
			TEXT_CLOSE_THRESHOLD, VOL_MIN, VOL_MAX, 1.0);
	obs_property_set_modified_callback(prop, settings_changed);

	prop = obs_properties_add_float_slider(ppts, S_OPEN_THRESHOLD,
			TEXT_OPEN_THRESHOLD, VOL_MIN, VOL_MAX, 1.0);
	obs_property_set_modified_callback(prop, settings_changed);

	prop = obs_properties_add_int(ppts, S_ATTACK_TIME, TEXT_ATTACK_TIME,
			0, 10000, 1);
	obs_property_set_modified_callback(prop, settings_changed);

	prop = obs_properties_add_int(ppts, S_HOLD_TIME, TEXT_HOLD_TIME,
			0, 10000, 1);
	obs_property_set_modified_callback(prop, settings_changed);

	prop = obs_properties_add_int(ppts, S_RELEASE_TIME, TEXT_RELEASE_TIME,
			0, 10000, 1);
	obs_property_set_modified_callback(prop, settings_changed);

	UNUSED_PARAMETER(data);
	return ppts;
}

struct obs_source_info noise_gate_filter = {
	.id = "noise_gate_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_AUDIO,
	.get_name = noise_gate_name,
	.create = noise_gate_create,
	.destroy = noise_gate_destroy,
	.update = noise_gate_update,
	.filter_audio = noise_gate_filter_audio,
	.get_defaults = noise_gate_defaults,
	.get_properties = noise_gate_properties,
};
