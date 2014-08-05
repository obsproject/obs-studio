/*
Copyright (C) 2014 by Leonhard Oelke <leonhard@in-verted.de>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <util/platform.h>
#include <util/bmem.h>
#include <obs-module.h>

#include "pulse-wrapper.h"

#define PULSE_DATA(voidptr) struct pulse_data *data = voidptr;

struct pulse_data {
	obs_source_t source;
	pa_stream *stream;

	/* user settings */
	char *device;

	/* server info */
	enum speaker_layout speakers;
	pa_sample_format_t format;
	uint_fast32_t samples_per_sec;
	uint_fast32_t bytes_per_frame;
	uint_fast8_t channels;

	/* statistics */
	uint_fast32_t packets;
	uint_fast64_t frames;
};

static void pulse_stop_recording(struct pulse_data *data);

/**
 * get obs from pulse audio format
 */
static enum audio_format pulse_to_obs_audio_format(
	pa_sample_format_t format)
{
	switch (format) {
		case PA_SAMPLE_U8:
			return AUDIO_FORMAT_U8BIT;
		case PA_SAMPLE_S16LE:
			return AUDIO_FORMAT_16BIT;
		case PA_SAMPLE_S24_32LE:
			return AUDIO_FORMAT_32BIT;
		case PA_SAMPLE_FLOAT32LE:
			return AUDIO_FORMAT_FLOAT;
		default:
			return AUDIO_FORMAT_UNKNOWN;
	}

	return AUDIO_FORMAT_UNKNOWN;
}

/**
 * Get latency for a pulse audio stream
 */
static int pulse_get_stream_latency(pa_stream *stream, int64_t *latency)
{
	int ret;
	int sign;
	pa_usec_t abs;

	ret = pa_stream_get_latency(stream, &abs, &sign);
	*latency = (sign) ? -(int64_t) abs : (int64_t) abs;
	return ret;
}

/**
 * Callback for pulse which gets executed when new audio data is available
 *
 * @warning The function may be called even after disconnecting the stream
 */
static void pulse_stream_read(pa_stream *p, size_t nbytes, void *userdata)
{
	UNUSED_PARAMETER(p);
	UNUSED_PARAMETER(nbytes);
	PULSE_DATA(userdata);

	const void *frames;
	size_t bytes;
	int64_t latency;

	if (!data->stream)
		goto exit;

	pa_stream_peek(data->stream, &frames, &bytes);

	// check if we got data
	if (!bytes)
		goto exit;

	if (!frames) {
		blog(LOG_ERROR, "pulse-input: Got audio hole of %u bytes",
			(unsigned int) bytes);
		pa_stream_drop(data->stream);
		goto exit;
	}

	if (pulse_get_stream_latency(data->stream, &latency) < 0) {
		blog(LOG_ERROR, "pulse-input: Failed to get timing info !");
		pa_stream_drop(data->stream);
		goto exit;
	}

	struct obs_source_audio out;
	out.speakers        = data->speakers;
	out.samples_per_sec = data->samples_per_sec;
	out.format          = pulse_to_obs_audio_format(data->format);
	out.data[0]         = (uint8_t *) frames;
	out.frames          = bytes / data->bytes_per_frame;
	out.timestamp       = os_gettime_ns() - (latency * 1000ULL);
	obs_source_output_audio(data->source, &out);

	data->packets++;
	data->frames += out.frames;

	pa_stream_drop(data->stream);
exit:
	pulse_signal(0);
}

/**
 * Server info callback
 */
static void pulse_server_info(pa_context *c, const pa_server_info *i,
	void *userdata)
{
	UNUSED_PARAMETER(c);
	PULSE_DATA(userdata);

	blog(LOG_INFO, "pulse-input: Server name: '%s %s'",
		i->server_name, i->server_version);

	data->format          = i->sample_spec.format;
	data->samples_per_sec = i->sample_spec.rate;
	data->channels        = i->sample_spec.channels;

	blog(LOG_INFO, "pulse-input: "
		"Audio format: %s, %u Hz, %u channels",
		pa_sample_format_to_string(i->sample_spec.format),
		i->sample_spec.rate,
		i->sample_spec.channels);

	pulse_signal(0);
}

/**
 * Start recording
 *
 * We request the default format used by pulse here because the data will be
 * converted and possibly re-sampled by obs anyway.
 *
 * The targeted latency for recording is 25ms.
 */
static int_fast32_t pulse_start_recording(struct pulse_data *data)
{
	if (pulse_get_server_info(pulse_server_info, (void *) data) < 0) {
		blog(LOG_ERROR, "pulse-input: Unable to get server info !");
		return -1;
	}

	pa_sample_spec spec;
	spec.format   = data->format;
	spec.rate     = data->samples_per_sec;
	spec.channels = data->channels;

	if (!pa_sample_spec_valid(&spec)) {
		blog(LOG_ERROR, "pulse-input: Sample spec is not valid");
		return -1;
	}

	data->bytes_per_frame = pa_frame_size(&spec);

	data->stream = pulse_stream_new(obs_source_get_name(data->source),
		&spec, NULL);
	if (!data->stream) {
		blog(LOG_ERROR, "pulse-input: Unable to create stream");
		return -1;
	}

	pulse_lock();
	pa_stream_set_read_callback(data->stream, pulse_stream_read,
		(void *) data);
	pulse_unlock();

	pa_buffer_attr attr;
	attr.fragsize  = 25000;
	attr.maxlength = (uint32_t) -1;

	pa_stream_flags_t flags =
		PA_STREAM_INTERPOLATE_TIMING
		| PA_STREAM_AUTO_TIMING_UPDATE
		| PA_STREAM_ADJUST_LATENCY;

	pulse_lock();
	int_fast32_t ret = pa_stream_connect_record(data->stream, data->device,
		&attr, flags);
	pulse_unlock();
	if (ret < 0) {
		pulse_stop_recording(data);
		blog(LOG_ERROR, "pulse-input: Unable to connect to stream");
		return -1;
	}

	blog(LOG_INFO, "pulse-input: Started recording from '%s'",
		data->device);
	return 0;
}

/**
 * stop recording
 */
static void pulse_stop_recording(struct pulse_data *data)
{
	if (data->stream) {
		pulse_lock();
		pa_stream_disconnect(data->stream);
		pa_stream_unref(data->stream);
		data->stream = NULL;
		pulse_unlock();
	}

	blog(LOG_INFO, "pulse-input: Stopped recording from '%s'",
		data->device);
	blog(LOG_INFO, "pulse-input: Got %"PRIuFAST32
		" packets with %"PRIuFAST64" frames",
		data->packets, data->frames);
	data->packets = 0;
	data->frames = 0;
}

/**
 * input info callback
 */
static void pulse_input_info(pa_context *c, const pa_source_info *i, int eol,
	void *userdata)
{
	UNUSED_PARAMETER(c);
	if (eol != 0 || i->monitor_of_sink != PA_INVALID_INDEX)
		goto skip;

	obs_property_list_add_string((obs_property_t) userdata,
		i->description, i->name);

skip:
	pulse_signal(0);
}

/**
 * output info callback
 */
static void pulse_output_info(pa_context *c, const pa_source_info *i, int eol,
	void *userdata)
{
	UNUSED_PARAMETER(c);
	if (eol != 0 || i->monitor_of_sink == PA_INVALID_INDEX)
		goto skip;

	obs_property_list_add_string((obs_property_t) userdata,
		i->description, i->name);

skip:
	pulse_signal(0);
}

/**
 * Get plugin properties
 */
static obs_properties_t pulse_properties(bool input)
{
	obs_properties_t props = obs_properties_create();
	obs_property_t devices = obs_properties_add_list(props, "device_id",
		obs_module_text("Device"), OBS_COMBO_TYPE_LIST,
		OBS_COMBO_FORMAT_STRING);

	pulse_init();
	pa_source_info_cb_t cb = (input) ? pulse_input_info : pulse_output_info;
	pulse_get_source_info_list(cb, (void *) devices);
	pulse_unref();

	return props;
}

static obs_properties_t pulse_input_properties(void)
{
	return pulse_properties(true);
}

static obs_properties_t pulse_output_properties(void)
{
	return pulse_properties(false);
}

/**
 * Server info callback
 */
static void pulse_input_device(pa_context *c, const pa_server_info *i,
	void *userdata)
{
	UNUSED_PARAMETER(c);
	obs_data_t settings = (obs_data_t) userdata;

	obs_data_set_default_string(settings, "device_id",
		i->default_source_name);
	blog(LOG_DEBUG, "pulse-input: Default input device: '%s'",
	     i->default_source_name);

	pulse_signal(0);
}

static void pulse_output_device(pa_context *c, const pa_server_info *i,
	void *userdata)
{
	UNUSED_PARAMETER(c);
	obs_data_t settings = (obs_data_t) userdata;

	char *monitor = bzalloc(strlen(i->default_sink_name) + 9);
	strcat(monitor, i->default_sink_name);
	strcat(monitor, ".monitor");

	obs_data_set_default_string(settings, "device_id", monitor);
	blog(LOG_DEBUG, "pulse-input: Default output device: '%s'", monitor);
	bfree(monitor);

	pulse_signal(0);
}

/**
 * Get plugin defaults
 */
static void pulse_defaults(obs_data_t settings, bool input)
{
	pulse_init();

	pa_server_info_cb_t cb = (input)
		? pulse_input_device : pulse_output_device;
	pulse_get_server_info(cb, (void *) settings);

	pulse_unref();
}

static void pulse_input_defaults(obs_data_t settings)
{
	return pulse_defaults(settings, true);
}

static void pulse_output_defaults(obs_data_t settings)
{
	return pulse_defaults(settings, false);
}

/**
 * Returns the name of the plugin
 */
static const char *pulse_input_getname(void)
{
	return obs_module_text("PulseInput");
}

static const char *pulse_output_getname(void)
{
	return obs_module_text("PulseOutput");
}

/**
 * Destroy the plugin object and free all memory
 */
static void pulse_destroy(void *vptr)
{
	PULSE_DATA(vptr);

	if (!data)
		return;

	if (data->stream)
		pulse_stop_recording(data);
	pulse_unref();

	if (data->device)
		bfree(data->device);
	bfree(data);
}

/**
 * Update the input settings
 */
static void pulse_update(void *vptr, obs_data_t settings)
{
	PULSE_DATA(vptr);
	bool restart = false;
	const char *new_device;

	new_device = obs_data_get_string(settings, "device_id");
	if (!data->device || strcmp(data->device, new_device) != 0) {
		if (data->device)
			bfree(data->device);
		data->device = bstrdup(new_device);
		restart = true;
	}

	if (!restart)
		return;

	if (data->stream)
		pulse_stop_recording(data);
	pulse_start_recording(data);
}

/**
 * Create the plugin object
 */
static void *pulse_create(obs_data_t settings, obs_source_t source)
{
	struct pulse_data *data = bzalloc(sizeof(struct pulse_data));

	data->source   = source;
	data->speakers = SPEAKERS_STEREO;

	pulse_init();
	pulse_update(data, settings);

	if (data->stream)
		return data;

	pulse_destroy(data);
	return NULL;
}

struct obs_source_info pulse_input_capture = {
	.id             = "pulse_input_capture",
	.type           = OBS_SOURCE_TYPE_INPUT,
	.output_flags   = OBS_SOURCE_AUDIO,
	.get_name       = pulse_input_getname,
	.create         = pulse_create,
	.destroy        = pulse_destroy,
	.update         = pulse_update,
	.get_defaults   = pulse_input_defaults,
	.get_properties = pulse_input_properties
};

struct obs_source_info pulse_output_capture = {
	.id             = "pulse_output_capture",
	.type           = OBS_SOURCE_TYPE_INPUT,
	.output_flags   = OBS_SOURCE_AUDIO,
	.get_name       = pulse_output_getname,
	.create         = pulse_create,
	.destroy        = pulse_destroy,
	.update         = pulse_update,
	.get_defaults   = pulse_output_defaults,
	.get_properties = pulse_output_properties
};
