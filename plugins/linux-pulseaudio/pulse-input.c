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
#include <obs.h>

#include "pulse-wrapper.h"

#define PULSE_DATA(voidptr) struct pulse_data *data = voidptr;

struct pulse_data {
	obs_source_t source;
	char *device;

	enum speaker_layout speakers;
	pa_sample_format_t format;
	uint_fast32_t samples_per_sec;
	uint_fast8_t channels;

	uint_fast32_t bytes_per_frame;

	pa_stream *stream;

	bool ostime;
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
 * Get the buffer size needed for length msec with current settings
 */
static uint_fast32_t get_buffer_size(struct pulse_data *data,
	uint_fast32_t length)
{
	return (length * data->samples_per_sec * data->bytes_per_frame) / 1000;
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
	uint64_t pa_time;
	int64_t pa_latency;

	if (!data->stream)
		goto exit;

	pa_stream_peek(data->stream, &frames, &bytes);

	// check if we got data
	if (!bytes)
		goto exit;

	if (!frames) {
		blog(LOG_DEBUG,
			"pulse-input: Got audio hole of %u bytes",
			(unsigned int) bytes);
		pa_stream_drop(data->stream);
		goto exit;
	}

	if (pa_stream_get_time(data->stream, &pa_time) < 0) {
		blog(LOG_ERROR,
			"pulse-input: Failed to get timing info !");
		pa_stream_drop(data->stream);
		goto exit;
	}
	pa_time = (!data->ostime) ? pa_time * 1000 : os_gettime_ns();

	pulse_get_stream_latency(data->stream, &pa_latency);

	struct source_audio out;
	out.speakers        = data->speakers;
	out.samples_per_sec = data->samples_per_sec;
	out.format          = pulse_to_obs_audio_format(data->format);
	out.data[0]         = (uint8_t *) frames;
	out.frames          = bytes / data->bytes_per_frame;
	out.timestamp       = pa_time - (pa_latency * 1000);
	obs_source_output_audio(data->source, &out);

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

	data->format          = i->sample_spec.format;
	data->samples_per_sec = i->sample_spec.rate;
	data->channels        = i->sample_spec.channels;

	blog(LOG_DEBUG, "pulse-input: Default format: %s, %u Hz, %u channels",
		pa_sample_format_to_string(i->sample_spec.format),
		i->sample_spec.rate,
		i->sample_spec.channels);

	pulse_signal(0);
}

/**
 * start recording
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
	blog(LOG_DEBUG, "pulse-input: %u bytes per frame",
	     (unsigned int) data->bytes_per_frame);

	data->stream = pulse_stream_new(obs_source_getname(data->source),
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
	attr.fragsize  = get_buffer_size(data, 250);
	attr.maxlength = (uint32_t) -1;
	attr.minreq    = (uint32_t) -1;
	attr.prebuf    = (uint32_t) -1;
	attr.tlength   = (uint32_t) -1;

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

	blog(LOG_DEBUG, "pulse-input: Recording started");
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
static obs_properties_t pulse_properties(const char *locale, bool input)
{
	obs_properties_t props = obs_properties_create(locale);
	obs_property_t devices = obs_properties_add_list(props, "device_id",
		"Device", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	pulse_init();
	pa_source_info_cb_t cb = (input) ? pulse_input_info : pulse_output_info;
	pulse_get_source_info_list(cb, (void *) devices);
	pulse_unref();

	obs_properties_add_bool(props, "ostime", "Use OS timestamps");

	return props;
}

static obs_properties_t pulse_input_properties(const char *locale)
{
	return pulse_properties(locale, true);
}

static obs_properties_t pulse_output_properties(const char *locale)
{
	return pulse_properties(locale, false);
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

	obs_data_set_default_bool(settings, "ostime", false);
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
static const char *pulse_input_getname(const char *locale)
{
	UNUSED_PARAMETER(locale);
	return "Pulse Audio Input Capture";
}

static const char *pulse_output_getname(const char *locale)
{
	UNUSED_PARAMETER(locale);
	return "Pulse Audio Output Capture";
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

	blog(LOG_DEBUG, "pulse-input: Input destroyed");
}

/**
 * Update the input settings
 */
static void pulse_update(void *vptr, obs_data_t settings)
{
	PULSE_DATA(vptr);
	bool restart = false;
	char *new_device;

	new_device = bstrdup(obs_data_getstring(settings, "device_id"));
	if (!data->device || strcmp(data->device, new_device) != 0) {
		if (data->device)
			bfree(data->device);
		data->device = new_device;
		restart = true;
	}

	if (data->ostime != obs_data_getbool(settings, "ostime")) {
		data->ostime = obs_data_getbool(settings, "ostime");
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
	.id           = "pulse_input_capture",
	.type         = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_AUDIO,
	.getname      = pulse_input_getname,
	.create       = pulse_create,
	.destroy      = pulse_destroy,
	.update       = pulse_update,
	.defaults     = pulse_input_defaults,
	.properties   = pulse_input_properties
};

struct obs_source_info pulse_output_capture = {
	.id           = "pulse_output_capture",
	.type         = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_AUDIO,
	.getname      = pulse_output_getname,
	.create       = pulse_create,
	.destroy      = pulse_destroy,
	.update       = pulse_update,
	.defaults     = pulse_output_defaults,
	.properties   = pulse_output_properties
};
