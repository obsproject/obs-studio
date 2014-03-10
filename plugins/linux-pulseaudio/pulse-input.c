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

#include <util/bmem.h>
#include <util/threading.h>
#include <util/platform.h>

#include <pulse/thread-mainloop.h>
#include <pulse/mainloop.h>
#include <pulse/context.h>
#include <pulse/introspect.h>
#include <pulse/stream.h>
#include <pulse/error.h>

#include <obs.h>

#define PULSE_DATA(voidptr) struct pulse_data *data = voidptr;

/*
 * delay in usecs before starting to record, this eliminates problems with
 * pulse audio sending weird data/timestamps when the stream is connected
 * 
 * for more information see:
 * github.com/MaartenBaert/ssr/blob/master/src/AV/Input/PulseAudioInput.cpp
 */
const uint64_t pulse_start_delay = 100000;

struct pulse_data {
	pthread_t thread;
	event_t event;
	obs_source_t source;
	
	enum speaker_layout speakers;
	pa_sample_format_t format;
	uint_fast32_t samples_per_sec;
	uint_fast8_t channels;
	
	uint_fast32_t bytes_per_frame;
	
	pa_mainloop *mainloop;
	pa_context *context;
	pa_stream *stream;
	pa_proplist *props;
};

struct pulse_context_change {
	pa_threaded_mainloop *mainloop;
	pa_context_state_t state;
};

struct pulse_enumerate {
	pa_threaded_mainloop *mainloop;
	obs_property_t devices;
	bool input;
};

/*
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

/*
 * get the number of frames from bytes and current format
 */
static uint_fast32_t frames_to_bytes(struct pulse_data *data, size_t bytes)
{
	return (bytes / data->bytes_per_frame);
}

/*
 * get the buffer size needed for length msec with current settings
 */
static uint_fast32_t get_buffer_size(struct pulse_data *data,
	uint_fast32_t length)
{
	return (length * data->samples_per_sec * data->bytes_per_frame) / 1000;
}

/*
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

/*
 * Iterate the mainloop
 * 
 * The custom implementation gives better performance than the function
 * provided by pulse audio, maybe due to the timeout set in prepare ?
 */
static void pulse_iterate(struct pulse_data *data)
{
	if (pa_mainloop_prepare(data->mainloop, 1000) < 0) {
		blog(LOG_ERROR, "Unable to prepare main loop");
		return;
	}
	if (pa_mainloop_poll(data->mainloop) < 0) {
		blog(LOG_ERROR, "Unable to poll main loop");
		return;
	}
	if (pa_mainloop_dispatch(data->mainloop) < 0)
		blog(LOG_ERROR, "Unable to dispatch main loop");
}

/*
 * Server info callback, this is called from pa_mainloop_dispatch
 * TODO: how to free the server info struct ?
 */
static void pulse_get_server_info_cb(pa_context *c, const pa_server_info *i,
	void *userdata)
{
	UNUSED_PARAMETER(c);
	PULSE_DATA(userdata);
	
	const pa_sample_spec *spec = &i->sample_spec;
	data->format = spec->format;
	data->samples_per_sec = spec->rate;
	
	blog(LOG_DEBUG, "pulse-input: Default format: %s, %u Hz, %u channels",
		pa_sample_format_to_string(spec->format),
		spec->rate,
		spec->channels);
}

/*
 * Request pulse audio server info
 * TODO: handle failures ?
 */
static int pulse_get_server_info(struct pulse_data *data)
{
	pa_server_info_cb_t cb = pulse_get_server_info_cb;
	pa_operation *op = pa_context_get_server_info(data->context, cb, data);
	
	for(;;) {
		pulse_iterate(data);
		pa_operation_state_t state = pa_operation_get_state(op);
		if (state == PA_OPERATION_DONE) {
			pa_operation_unref(op);
			break;
		}
	}
	
	return 0;
}

/*
 * Create a new pulse audio main loop and connect to the server
 * 
 * Returns a negative value on error
 */
static int pulse_connect(struct pulse_data *data)
{
	data->mainloop = pa_mainloop_new();
	if (!data->mainloop) {
		blog(LOG_ERROR, "pulse-input: Unable to create main loop");
		return -1;
	}
	
	data->context = pa_context_new_with_proplist(
		pa_mainloop_get_api(data->mainloop), "OBS Studio", data->props);
	if (!data->context) {
		blog(LOG_ERROR, "pulse-input: Unable to create context");
		return -1;
	}
	
	int status = pa_context_connect(
		data->context, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL);
	if (status < 0) {
		blog(LOG_ERROR, "pulse-input: Unable to connect! Status: %d",
		     status);
		return -1;
	}
	
	// wait until connected
	for (;;) {
		pulse_iterate(data);
		pa_context_state_t state = pa_context_get_state(data->context);
		if (state == PA_CONTEXT_READY) {
			blog(LOG_DEBUG, "pulse-input: Context ready");
			break;
		}
		if (!PA_CONTEXT_IS_GOOD(state)) {
			blog(LOG_ERROR, "pulse-input: Context connect failed");
			return -1;
		}
	}
	
	return 0;
}

/*
 * Disconnect from the pulse audio server and destroy the main loop
 */
static void pulse_disconnect(struct pulse_data *data)
{
	if (data->context) {
		pa_context_disconnect(data->context);
		pa_context_unref(data->context);
	}
	
	if (data->mainloop)
		pa_mainloop_free(data->mainloop);
}

/*
 * Create a new pulse audio stream and connect to it
 * 
 * Return a negative value on error
 */
static int pulse_connect_stream(struct pulse_data *data)
{
	pa_sample_spec spec;
	spec.format = data->format;
	spec.rate = data->samples_per_sec;
	spec.channels = get_audio_channels(data->speakers);
	
	if (!pa_sample_spec_valid(&spec)) {
		blog(LOG_ERROR, "pulse-input: Sample spec is not valid");
		return -1;
	}
	
	data->bytes_per_frame = pa_frame_size(&spec);
	blog(LOG_DEBUG, "pulse-input: %u bytes per frame",
	     (unsigned int) data->bytes_per_frame);
	
	pa_buffer_attr attr;
	attr.fragsize = get_buffer_size(data, 250);
	attr.maxlength = (uint32_t) -1;
	attr.minreq = (uint32_t) -1;
	attr.prebuf = (uint32_t) -1;
	attr.tlength = (uint32_t) -1;
	
	data->stream = pa_stream_new_with_proplist(data->context,
		obs_source_getname(data->source), &spec, NULL, data->props);
	if (!data->stream) {
		blog(LOG_ERROR, "pulse-input: Unable to create stream");
		return -1;
	}
	pa_stream_flags_t flags =
		PA_STREAM_INTERPOLATE_TIMING
		| PA_STREAM_AUTO_TIMING_UPDATE
		| PA_STREAM_ADJUST_LATENCY;
	if (pa_stream_connect_record(data->stream, NULL, &attr, flags) < 0) {
		blog(LOG_ERROR, "pulse-input: Unable to connect to stream");
		return -1;
	}
	
	for (;;) {
		pulse_iterate(data);
		pa_stream_state_t state = pa_stream_get_state(data->stream);
		if (state == PA_STREAM_READY) {
			blog(LOG_DEBUG, "pulse-input: Stream ready");
			break;
		}
		if (!PA_STREAM_IS_GOOD(state)) {
			blog(LOG_ERROR, "pulse-input: Stream connect failed");
			return -1;
		}
	}
	
	return 0;
}

/*
 * Disconnect from the pulse audio stream
 */
static void pulse_diconnect_stream(struct pulse_data *data)
{
	if (data->stream) {
		pa_stream_disconnect(data->stream);
		pa_stream_unref(data->stream);
	}
}

/*
 * Loop to skip the first few samples of a stream
 */
static int pulse_skip(struct pulse_data *data)
{
	uint64_t skip = 1;
	const void *frames;
	size_t bytes;
	uint64_t pa_time;
	
	while (event_try(data->event) == EAGAIN) {
		pulse_iterate(data);
		pa_stream_peek(data->stream, &frames, &bytes);
		
		if (!bytes)
			continue;
		if (!frames || pa_stream_get_time(data->stream, &pa_time) < 0) {
			pa_stream_drop(data->stream);
			continue;
		}
		
		if (skip == 1 && pa_time)
			skip = pa_time;
		if (skip + pulse_start_delay < pa_time)
			return 0;
		
		pa_stream_drop(data->stream);
	}
	
	return -1;
}

/*
 * Worker thread to get audio data
 * 
 * Will run until signaled
 */
static void *pulse_thread(void *vptr)
{
	PULSE_DATA(vptr);
	
	if (pulse_connect(data) < 0)
		return NULL;
	if (pulse_get_server_info(data) < 0)
		return NULL;
	if (pulse_connect_stream(data) < 0)
		return NULL;
	
	if (pulse_skip(data) < 0)
		return NULL;
	
	blog(LOG_DEBUG, "pulse-input: Start recording");
	
	const void *frames;
	size_t bytes;
	uint64_t pa_time;
	int64_t pa_latency;
	
	struct source_audio out;
	out.speakers = data->speakers;
	out.samples_per_sec = data->samples_per_sec;
	out.format = pulse_to_obs_audio_format(data->format);
	
	while (event_try(data->event) == EAGAIN) {
		pulse_iterate(data);
		
		pa_stream_peek(data->stream, &frames, &bytes);
		
		// check if we got data
		if (!bytes)
			continue;
		if (!frames) {
			blog(LOG_DEBUG,
				"pulse-input: Got audio hole of %u bytes",
				(unsigned int) bytes);
			pa_stream_drop(data->stream);
			continue;
		}
		
		if (pa_stream_get_time(data->stream, &pa_time) < 0) {
			blog(LOG_ERROR,
				"pulse-input: Failed to get timing info !");
			pa_stream_drop(data->stream);
			continue;
		}
		
		pulse_get_stream_latency(data->stream, &pa_latency);
		
		out.data[0] = (uint8_t *) frames;
		out.frames = frames_to_bytes(data, bytes);
		out.timestamp = (pa_time - pa_latency) * 1000;
		obs_source_output_audio(data->source, &out);
		
		pa_stream_drop(data->stream);
	}
	
	pulse_diconnect_stream(data);
	pulse_disconnect(data);
	
	return NULL;
}

/*
 * Create a new pulseaudio context
 */
static pa_context *pulse_context_create(pa_threaded_mainloop *m)
{
	pa_context *c;
	pa_proplist *p;
	
	p = pa_proplist_new();
	pa_proplist_sets(p, PA_PROP_APPLICATION_NAME, "OBS Studio");
	pa_proplist_sets(p, PA_PROP_APPLICATION_ICON_NAME, "application-exit");
	pa_proplist_sets(p, PA_PROP_MEDIA_ROLE, "production");
	
	pa_threaded_mainloop_lock(m);
	c = pa_context_new_with_proplist(pa_threaded_mainloop_get_api(m),
		"OBS Studio", p);
	pa_threaded_mainloop_unlock(m);
	
	pa_proplist_free(p);
	
	return c;
}

/**
 * Context state callback
 */
static void pulse_context_state_changed(pa_context *c, void *userdata)
{
	struct pulse_context_change *ctx =
		(struct pulse_context_change *) userdata;
	ctx->state = pa_context_get_state(c);
	
	pa_threaded_mainloop_signal(ctx->mainloop, 0);
}

/*
 * Connect context
 */
static int pulse_context_connect(pa_threaded_mainloop *m, pa_context *c)
{
	int status = 0;
	struct pulse_context_change ctx;
	ctx.mainloop = m;
	ctx.state = PA_CONTEXT_UNCONNECTED;
	
	pa_threaded_mainloop_lock(m);
	pa_context_set_state_callback(c, pulse_context_state_changed,
		(void *) &ctx);
	
	status = pa_context_connect(c, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL);
	if (status < 0) {
		blog(LOG_ERROR, "pulse-input: Unable to connect! Status: %d",
		     status);
	}
	else {
		for (;;) {
			if (ctx.state == PA_CONTEXT_READY) {
				blog(LOG_DEBUG, "pulse-input: Context Ready");
				break;
			}
			if (!PA_CONTEXT_IS_GOOD(ctx.state)) {
				blog(LOG_ERROR,
				     "pulse-input: Context connect failed !");
				status = -1;
				break;
			}
			pa_threaded_mainloop_wait(m);
		}
	}
	
	pa_threaded_mainloop_unlock(m);
	return status;
}

/*
 * Source properties callback
 */
static void pulse_source_info(pa_context *c, const pa_source_info *i, int eol,
	void *userdata)
{
	UNUSED_PARAMETER(c);
	
	if (eol != 0)
		return;
	
	struct pulse_enumerate *e = (struct pulse_enumerate *) userdata;
	
	if ((e->input) ^ (i->monitor_of_sink == PA_INVALID_INDEX))
		return;
	
	blog(LOG_DEBUG, "pulse-input: Got source #%u '%s'",
	     i->index, i->description);
	
	obs_property_list_add_item(e->devices, i->description, i->name);
	
	pa_threaded_mainloop_signal(e->mainloop, 0);
}

/*
 * enumerate input/output devices
 */
static void pulse_enumerate_devices(obs_properties_t props, bool input)
{
	pa_context *c;
	pa_operation *op;
	pa_threaded_mainloop *m = pa_threaded_mainloop_new();
	struct pulse_enumerate e;
	
	e.mainloop = m;
	e.devices = obs_properties_add_list(props, "device_id", "Device",
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	e.input = input;
	
	pa_threaded_mainloop_start(m);
	c = pulse_context_create(m);
	
	if (pulse_context_connect(m, c) < 0)
		goto fail;
	
	pa_threaded_mainloop_lock(m);
	
	op = pa_context_get_source_info_list(c, pulse_source_info, (void *) &e);
	while (pa_operation_get_state(op) == PA_OPERATION_RUNNING)
		pa_threaded_mainloop_wait(m);
	pa_operation_unref(op);
	
	pa_threaded_mainloop_unlock(m);
	
	pa_context_disconnect(c);
fail:
	pa_context_unref(c);
	pa_threaded_mainloop_stop(m);
	pa_threaded_mainloop_free(m);
}

/*
 * get plugin properties
 */
static obs_properties_t pulse_properties(const char *locale, bool input)
{
	UNUSED_PARAMETER(locale);
	
	blog(LOG_DEBUG, "pulse-input: properties requested !");
	
	obs_properties_t props = obs_properties_create();
	
	pulse_enumerate_devices(props, input);
	
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

/*
 * get plugin defaults
 */
static void pulse_defaults(obs_data_t settings)
{
	obs_data_set_default_string(settings, "device_id", "default");
}

/*
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

/*
 * Destroy the plugin object and free all memory
 */
static void pulse_destroy(void *vptr)
{
	PULSE_DATA(vptr);
	
	if (!data)
		return;
	
	if (data->thread) {
		void *ret;
		event_signal(data->event);
		pthread_join(data->thread, &ret);
	}
	
	event_destroy(data->event);
	
	pa_proplist_free(data->props);
	
	blog(LOG_DEBUG, "pulse-input: Input destroyed");
	
	bfree(data);
}

/*
 * Create the plugin object
 */
static void *pulse_create(obs_data_t settings, obs_source_t source)
{
	UNUSED_PARAMETER(settings);
	
	struct pulse_data *data = bmalloc(sizeof(struct pulse_data));
	memset(data, 0, sizeof(struct pulse_data));
	
	data->source = source;
	data->speakers = SPEAKERS_STEREO;
	
	blog(LOG_DEBUG, "pulse-input: obs wants '%s'",
		obs_data_getstring(settings, "device_id"));
	
	/* TODO: use obs-studio icon */
	data->props = pa_proplist_new();
	pa_proplist_sets(data->props, PA_PROP_APPLICATION_NAME,
		"OBS Studio");
	pa_proplist_sets(data->props, PA_PROP_APPLICATION_ICON_NAME,
		"application-exit");
	pa_proplist_sets(data->props, PA_PROP_MEDIA_ROLE,
		"production");
	
	if (event_init(&data->event, EVENT_TYPE_MANUAL) != 0)
		goto fail;
	if (pthread_create(&data->thread, NULL, pulse_thread, data) != 0)
		goto fail;
	
	return data;
	
fail:
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
	.defaults     = pulse_defaults,
	.properties   = pulse_input_properties
};

struct obs_source_info pulse_output_capture = {
	.id           = "pulse_output_capture",
	.type         = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_AUDIO,
	.getname      = pulse_output_getname,
	.create       = pulse_create,
	.destroy      = pulse_destroy,
	.defaults     = pulse_defaults,
	.properties   = pulse_output_properties
};
