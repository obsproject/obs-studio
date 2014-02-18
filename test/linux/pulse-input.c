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
	
	uint32_t samples_per_sec;
	enum speaker_layout speakers;
	pa_sample_format_t format;
	
	pa_mainloop *mainloop;
	pa_context *context;
	pa_stream *stream;
	pa_proplist *props;
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
static uint32_t get_frames_from_bytes(struct pulse_data *data, size_t bytes)
{
	uint32_t ret = bytes;
	ret /= get_audio_bytes_per_channel(
		pulse_to_obs_audio_format(data->format));
	ret /= get_audio_channels(data->speakers);
	
	return ret;
}

/*
 * get the buffer size needed for length msec with current settings
 */
static uint32_t get_buffer_size(struct pulse_data *data, uint32_t length)
{
	uint32_t ret = length;
	ret *= data->samples_per_sec;
	ret *= get_audio_bytes_per_channel(
		pulse_to_obs_audio_format(data->format));
	ret *= get_audio_channels(data->speakers);
	ret /= 1000;
	
	return ret;
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
	
	while (event_try(&data->event) == EAGAIN) {
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
	
	while (event_try(&data->event) == EAGAIN) {
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
		out.frames = get_frames_from_bytes(data, bytes);
		out.timestamp = (pa_time - pa_latency) * 1000;
		obs_source_output_audio(data->source, &out);
		
		pa_stream_drop(data->stream);
	}
	
	pulse_diconnect_stream(data);
	pulse_disconnect(data);
	
	return NULL;
}

/*
 * Returns the name of the plugin
 */
static const char *pulse_getname(const char *locale)
{
	UNUSED_PARAMETER(locale);
	return "Pulse Audio Input";
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
		event_signal(&data->event);
		pthread_join(data->thread, &ret);
	}
	
	event_destroy(&data->event);
	
	pa_proplist_free(data->props);
	
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
	data->samples_per_sec = 44100;
	data->speakers = SPEAKERS_STEREO;
	data->format = PA_SAMPLE_S16LE;
	
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

struct obs_source_info pulse_input = {
	.id           = "pulse_input",
	.type         = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_AUDIO,
	.getname      = pulse_getname,
	.create       = pulse_create,
	.destroy      = pulse_destroy
};
