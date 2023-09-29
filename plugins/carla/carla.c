/*
 * Carla plugin for OBS
 * Copyright (C) 2023 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

// for audio generator thread
#include <pthread.h>

#include <obs-module.h>
#include <util/platform.h>

#include "carla-wrapper.h"
#include "common.h"

#ifndef CARLA_MODULE_ID
#error CARLA_MODULE_ID undefined
#endif

#ifndef CARLA_MODULE_NAME
#error CARLA_MODULE_NAME undefined
#endif

// --------------------------------------------------------------------------------------------------------------------

struct carla_data {
	// carla host details, intentionally kept private so we can easily swap internals
	struct carla_priv *priv;

	// current OBS config
	bool activated;
	uint32_t sample_rate;
	obs_source_t *source;

	// filter related options
	size_t channels;

	// audio generator thread
	bool audiogen_enabled;
	volatile bool audiogen_running;
	pthread_t audiogen_thread;

	// internal buffering
	float *buffers[MAX_AV_PLANES];
	uint16_t buffer_head;
	uint16_t buffer_tail;
	enum buffer_size_mode buffer_size_mode;

	// dummy buffer for unused audio channels
	float *dummybuffer;
};

// --------------------------------------------------------------------------------------------------------------------
// private methods

static enum speaker_layout carla_obs_channels_to_speakers(const size_t channels)
{
	switch (channels) {
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
	// FIXME missing case for 7 channels
	case 8:
		return SPEAKERS_7POINT1;
	// use stereo as fallback
	default:
		return SPEAKERS_STEREO;
	}
}

static void *carla_obs_audio_gen_thread(void *data)
{
	struct carla_data *carla = data;

	struct obs_source_audio out = {
		.format = AUDIO_FORMAT_FLOAT_PLANAR,
		.samples_per_sec = carla->sample_rate,
	};

	for (uint8_t c = 0; c < MAX_AV_PLANES; ++c)
		out.data[c] = (const uint8_t *)carla->buffers[c];

	const uint32_t sample_rate = carla->sample_rate;
	const uint64_t start_time = out.timestamp = os_gettime_ns();
	uint64_t total_samples = 0;

	while (carla->audiogen_running) {
		const uint32_t buffer_size =
			bufsize_mode_to_frames(carla->buffer_size_mode);

		out.frames = buffer_size;
		out.speakers = carla_obs_channels_to_speakers(
			carla_priv_get_num_channels(carla->priv));
		carla_priv_process_audio(carla->priv, carla->buffers,
					 buffer_size);
		obs_source_output_audio(carla->source, &out);

		if (!carla->audiogen_running)
			break;

		total_samples += buffer_size;
		out.timestamp = start_time +
				audio_frames_to_ns(sample_rate, total_samples);

		os_sleepto_ns_fast(out.timestamp);
	}

	return NULL;
}

static void carla_obs_idle_callback(void *data, float unused)
{
	UNUSED_PARAMETER(unused);
	struct carla_data *carla = data;
	carla_priv_idle(carla->priv);
}

// --------------------------------------------------------------------------------------------------------------------
// obs plugin methods

static void carla_obs_deactivate(void *data);

static const char *carla_obs_get_name(void *data)
{
	return !strcmp(data, "filter")
		       ? obs_module_text(CARLA_MODULE_NAME " Filter")
		       : obs_module_text(CARLA_MODULE_NAME " Generator/Source");
}

static void *carla_obs_create(obs_data_t *settings, obs_source_t *source,
			      bool isFilter)
{
	UNUSED_PARAMETER(settings);

	const audio_t *audio = obs_get_audio();
	const size_t channels = audio_output_get_channels(audio);
	const uint32_t sample_rate = audio_output_get_sample_rate(audio);

	if (sample_rate == 0 || (isFilter && channels == 0))
		return NULL;

	struct carla_data *carla = bzalloc(sizeof(*carla));
	if (carla == NULL)
		return NULL;

	for (uint8_t c = 0; c < MAX_AV_PLANES; ++c) {
		carla->buffers[c] =
			bzalloc(sizeof(float) * MAX_AUDIO_BUFFER_SIZE);
		if (carla->buffers[c] == NULL)
			goto fail1;
	}

	carla->dummybuffer = bzalloc(sizeof(float) * MAX_AUDIO_BUFFER_SIZE);
	if (carla->dummybuffer == NULL)
		goto fail2;

	// prefer no-latency mode for filter, lowest latency for generator
	const enum buffer_size_mode bufsize =
		isFilter ? buffer_size_direct : buffer_size_buffered_128;

	struct carla_priv *priv =
		carla_priv_create(source, bufsize, sample_rate);
	if (carla == NULL)
		goto fail3;

	carla->priv = priv;
	carla->source = source;
	carla->channels = channels;
	carla->sample_rate = sample_rate;

	carla->buffer_head = 0;
	carla->buffer_tail = UINT16_MAX;
	carla->buffer_size_mode = bufsize;

	// audio generator, aka input source
	carla->audiogen_enabled = !isFilter;

	obs_add_tick_callback(carla_obs_idle_callback, carla);

	return carla;

fail3:
	bfree(carla->dummybuffer);

fail2:
	for (uint8_t c = 0; c < MAX_AV_PLANES; ++c)
		bfree(carla->buffers[c]);

fail1:
	bfree(carla);
	return NULL;
}

static void *carla_obs_create_filter(obs_data_t *settings, obs_source_t *source)
{
	return carla_obs_create(settings, source, true);
}

static void *carla_obs_create_input(obs_data_t *settings, obs_source_t *source)
{
	return carla_obs_create(settings, source, false);
}

static void carla_obs_destroy(void *data)
{
	struct carla_data *carla = data;

	if (carla->activated)
		carla_obs_deactivate(carla);

	obs_remove_tick_callback(carla_obs_idle_callback, carla);

	carla_priv_destroy(carla->priv);

	bfree(carla->dummybuffer);
	for (uint8_t c = 0; c < MAX_AV_PLANES; ++c)
		bfree(carla->buffers[c]);
	bfree(carla);
}

static bool carla_obs_bufsize_callback(void *data, obs_properties_t *props,
				       obs_property_t *list,
				       obs_data_t *settings)
{
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(list);

	struct carla_data *carla = data;

	enum buffer_size_mode bufsize;
	const char *const value =
		obs_data_get_string(settings, PROP_BUFFER_SIZE);

	/**/ if (!strcmp(value, "direct"))
		bufsize = buffer_size_direct;
	else if (!strcmp(value, "128"))
		bufsize = buffer_size_buffered_128;
	else if (!strcmp(value, "256"))
		bufsize = buffer_size_buffered_256;
	else if (!strcmp(value, "512"))
		bufsize = buffer_size_buffered_512;
	else
		return false;

	if (carla->buffer_size_mode == bufsize)
		return false;

	// deactivate first, to stop audio from processing
	carla_priv_deactivate(carla->priv);

	// safely change to new buffer size
	carla->buffer_size_mode = bufsize;
	carla_priv_set_buffer_size(carla->priv, bufsize);

	// activate again
	carla_priv_activate(carla->priv);

	return false;
}

static obs_properties_t *carla_obs_get_properties(void *data)
{
	struct carla_data *carla = data;

	obs_properties_t *props = obs_properties_create();

	obs_property_t *list = obs_properties_add_list(
		props, PROP_BUFFER_SIZE, obs_module_text("Buffer Size"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	if (carla->audiogen_enabled) {
		obs_property_list_add_string(
			list, obs_module_text("128 samples"), "128");
		obs_property_list_add_string(
			list, obs_module_text("256 samples"), "256");
		obs_property_list_add_string(
			list, obs_module_text("512 samples"), "512");
	} else {
		obs_property_list_add_string(
			list, obs_module_text("Direct (variable buffer)"),
			"direct");
		obs_property_list_add_string(
			list,
			obs_module_text(
				"128 samples (fixed buffer with latency)"),
			"128");
		obs_property_list_add_string(
			list,
			obs_module_text(
				"256 samples (fixed buffer with latency)"),
			"256");
		obs_property_list_add_string(
			list,
			obs_module_text(
				"512 samples (fixed buffer with latency)"),
			"512");
	}

	obs_property_set_modified_callback2(list, carla_obs_bufsize_callback,
					    carla);

	carla_priv_readd_properties(carla->priv, props, false);

	return props;
}

static void carla_obs_activate(void *data)
{
	struct carla_data *carla = data;
	assert(!carla->activated);

	if (carla->activated)
		return;

	carla->activated = true;

	carla_priv_activate(carla->priv);

	if (carla->audiogen_enabled) {
		assert(!carla->audiogen_running);
		carla->audiogen_running = true;
		pthread_create(&carla->audiogen_thread, NULL,
			       carla_obs_audio_gen_thread, carla);
	}
}

static void carla_obs_deactivate(void *data)
{
	struct carla_data *carla = data;
	assert(carla->activated);

	if (!carla->activated)
		return;

	carla->activated = false;

	if (carla->audiogen_running) {
		carla->audiogen_running = false;
		pthread_join(carla->audiogen_thread, NULL);
	}

	carla_priv_deactivate(carla->priv);
}

static void carla_obs_filter_audio_direct(struct carla_data *carla,
					  struct obs_audio_data *audio)
{
	uint32_t frames = audio->frames;
	float *obsbuffers[MAX_AV_PLANES];

	// process in blocks up to MAX_AUDIO_BUFFER_SIZE
	for (uint32_t i = 0; frames != 0;) {
		const uint32_t stepframes = frames >= MAX_AUDIO_BUFFER_SIZE
						    ? MAX_AUDIO_BUFFER_SIZE
						    : frames;

		for (uint8_t c = 0; c < MAX_AV_PLANES; ++c)
			obsbuffers[c] = audio->data[c]
						? ((float *)audio->data[c] + i)
						: carla->dummybuffer;

		carla_priv_process_audio(carla->priv, obsbuffers, stepframes);

		memset(carla->dummybuffer, 0, sizeof(float) * stepframes);

		i += stepframes;
		frames -= stepframes;
	}
}

static void carla_obs_filter_audio_buffered(struct carla_data *carla,
					    struct obs_audio_data *audio)
{
	const uint32_t buffer_size =
		bufsize_mode_to_frames(carla->buffer_size_mode);
	const size_t channels = carla->channels;
	const uint32_t frames = audio->frames;

	// cast audio buffers to correct type
	float *obsbuffers[MAX_AV_PLANES];

	for (uint8_t c = 0; c < MAX_AV_PLANES; ++c)
		obsbuffers[c] = audio->data[c] ? (float *)audio->data[c]
					       : carla->dummybuffer;

	// preload some variables before looping section
	uint16_t buffer_head = carla->buffer_head;
	uint16_t buffer_tail = carla->buffer_tail;

	for (uint32_t i = 0, h, t; i < frames; ++i) {
		// OBS -> plugin internal buffering
		h = buffer_head++;

		for (uint8_t c = 0; c < channels; ++c)
			carla->buffers[c][h] = obsbuffers[c][i];

		// when we reach the target buffer size, do audio processing
		if (buffer_head == buffer_size) {
			buffer_head = 0;
			carla_priv_process_audio(carla->priv, carla->buffers,
						 buffer_size);
			memset(carla->dummybuffer, 0,
			       sizeof(float) * buffer_size);

			// we can now begin to copy back the buffer into OBS
			if (buffer_tail == UINT16_MAX)
				buffer_tail = 0;
		}

		if (buffer_tail == UINT16_MAX) {
			// buffering still taking place, skip until first audio cycle
			for (uint8_t c = 0; c < channels; ++c)
				obsbuffers[c][i] = 0.f;
		} else {
			// plugin -> OBS buffer copy
			t = buffer_tail++;

			for (uint8_t c = 0; c < channels; ++c)
				obsbuffers[c][i] = carla->buffers[c][t];

			if (buffer_tail == buffer_size)
				buffer_tail = 0;
		}
	}

	carla->buffer_head = buffer_head;
	carla->buffer_tail = buffer_tail;
}

static struct obs_audio_data *
carla_obs_filter_audio(void *data, struct obs_audio_data *audio)
{
	struct carla_data *carla = data;

	switch (carla->buffer_size_mode) {
	case buffer_size_direct:
		carla_obs_filter_audio_direct(carla, audio);
		break;
	case buffer_size_buffered_128:
	case buffer_size_buffered_256:
	case buffer_size_buffered_512:
		carla_obs_filter_audio_buffered(carla, audio);
		break;
	}

	return audio;
}

static void carla_obs_save(void *data, obs_data_t *settings)
{
	struct carla_data *carla = data;
	carla_priv_save(carla->priv, settings);
}

static void carla_obs_load(void *data, obs_data_t *settings)
{
	struct carla_data *carla = data;
	carla_priv_load(carla->priv, settings);
}

// --------------------------------------------------------------------------------------------------------------------

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("carla", "en-US")
OBS_MODULE_AUTHOR("Filipe Coelho")
const char *obs_module_name(void)
{
	return CARLA_MODULE_NAME;
}

bool obs_module_load(void)
{
	const char *carla_bin_path = get_carla_bin_path();
	if (!carla_bin_path) {
		blog(LOG_WARNING,
		     "[" CARLA_MODULE_ID "]"
		     " failed to find binaries, will not load module");
		return false;
	}
	blog(LOG_INFO, "[" CARLA_MODULE_ID "] using binary path %s",
	     carla_bin_path);

	static const struct obs_source_info filter = {
		.id = CARLA_MODULE_ID "-filter",
		.type = OBS_SOURCE_TYPE_FILTER,
		.output_flags = OBS_SOURCE_AUDIO,
		.get_name = carla_obs_get_name,
		.create = carla_obs_create_filter,
		.destroy = carla_obs_destroy,
		.get_properties = carla_obs_get_properties,
		.activate = carla_obs_activate,
		.deactivate = carla_obs_deactivate,
		.filter_audio = carla_obs_filter_audio,
		.save = carla_obs_save,
		.load = carla_obs_load,
		.type_data = "filter",
		.icon_type = OBS_ICON_TYPE_PROCESS_AUDIO_OUTPUT,
	};
	obs_register_source(&filter);

	static const struct obs_source_info input = {
		.id = CARLA_MODULE_ID "-input",
		.type = OBS_SOURCE_TYPE_INPUT,
		.output_flags = OBS_SOURCE_AUDIO,
		.get_name = carla_obs_get_name,
		.create = carla_obs_create_input,
		.destroy = carla_obs_destroy,
		.get_properties = carla_obs_get_properties,
		.activate = carla_obs_activate,
		.deactivate = carla_obs_deactivate,
		.save = carla_obs_save,
		.load = carla_obs_load,
		.type_data = "input",
		.icon_type = OBS_ICON_TYPE_AUDIO_OUTPUT,
	};
	obs_register_source(&input);

	return true;
}

// --------------------------------------------------------------------------------------------------------------------
