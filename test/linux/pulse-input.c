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
 * delay in nanos before starting to record, this eliminates problems with
 * pulse audio sending weird data/timestamps when the stream is connected
 * 
 * for more information see:
 * github.com/MaartenBaert/ssr/blob/master/src/AV/Input/PulseAudioInput.cpp
 */
const uint64_t pulse_start_delay = 100000000;

struct pulse_data {
    pthread_t thread;
    event_t event;
    obs_source_t source;
    
    uint32_t frames;
    uint32_t samples_per_sec;
    uint32_t channels;
    enum speaker_layout speakers;
    
    pa_mainloop *mainloop;
    pa_context *context;
    pa_stream *stream;
};

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

static int pulse_connect(struct pulse_data *data)
{
    data->mainloop = pa_mainloop_new();
    if (!data->mainloop) {
        blog(LOG_ERROR, "Unable to create main loop");
        return 0;
    }
    
    data->context = pa_context_new(
        pa_mainloop_get_api(data->mainloop), "OBS Studio");
    if (!data->context) {
        blog(LOG_ERROR, "Unable to create context");
        return 0;
    }
    
    int status = pa_context_connect(
        data->context, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL);
    if (status < 0) {
        blog(LOG_ERROR, "Unable to connect! Status: %d", status);
        return 0;
    }
    
    // wait until connected
    for (;;) {
        pulse_iterate(data);
        pa_context_state_t state = pa_context_get_state(data->context);
        if (state == PA_CONTEXT_READY) {
            blog(LOG_DEBUG, "Context ready");
            break;
        }
        if (!PA_CONTEXT_IS_GOOD(state)) {
            blog(LOG_ERROR, "Connection attempt failed");
            return 0;
        }
    }
    
    return 1;
}

static void pulse_disconnect(struct pulse_data *data)
{
    if (data->context) {
        pa_context_disconnect(data->context);
        pa_context_unref(data->context);
    }
    
    if (data->mainloop)
        pa_mainloop_free(data->mainloop);
}

static int pulse_connect_stream(struct pulse_data *data)
{
    pa_sample_spec spec;
    spec.format = PA_SAMPLE_U8;
    spec.rate = data->samples_per_sec;
    spec.channels = data->channels;
    
    pa_buffer_attr attr;
    attr.fragsize = data->frames * spec.channels;
    attr.maxlength = (uint32_t) -1;
    attr.minreq = (uint32_t) -1;
    attr.prebuf = (uint32_t) -1;
    attr.tlength = (uint32_t) -1;
    
    data->stream = pa_stream_new(
        data->context, "OBS Audio Input", &spec, NULL);
    if (!data->stream) {
        blog(LOG_ERROR, "Unable to create stream");
        return 0;
    }
    pa_stream_flags_t flags =
        PA_STREAM_INTERPOLATE_TIMING
        | PA_STREAM_AUTO_TIMING_UPDATE
        | PA_STREAM_ADJUST_LATENCY;
    if (pa_stream_connect_record(data->stream, NULL, &attr, flags) < 0) {
        blog(LOG_ERROR, "Unable to connect to stream");
        return 0;
    }
    
    for (;;) {
        pulse_iterate(data);
        pa_stream_state_t state = pa_stream_get_state(data->stream);
        if (state == PA_STREAM_READY) {
            blog(LOG_DEBUG, "Stream ready");
            break;
        }
        if (!PA_STREAM_IS_GOOD(state)) {
            blog(LOG_ERROR, "Stream connection failed");
            return 0;
        }
    }
    
    return 1;
}

static void pulse_diconnect_stream(struct pulse_data *data)
{
    if (data->stream) {
        pa_stream_disconnect(data->stream);
        pa_stream_unref(data->stream);
    }
}

static void *pulse_thread(void *vptr)
{
    PULSE_DATA(vptr);
    
    if (!pulse_connect(data))
        return NULL;
    if (!pulse_connect_stream(data))
        return NULL;
    
    uint64_t skip = 1;

    while (event_try(&data->event) == EAGAIN) {
        uint64_t cur_time = os_gettime_ns();
        
        pulse_iterate(data);
        
        const void *frames;
        size_t bytes;
        pa_stream_peek(data->stream, &frames, &bytes);
        
        // check if we got data
        if (!bytes) {
            pa_stream_drop(data->stream);
            continue;
        }
        
        // skip the first frames received
        if (skip) {
            // start delay when we receive the first bytes
            if (skip == 1 && bytes)
                skip = os_gettime_ns();
            if (skip + pulse_start_delay < os_gettime_ns())
                skip = 0;
            pa_stream_drop(data->stream);
            continue;
        }
        
        // get stream latency
        pa_usec_t l_abs;
        int l_sign;
        pa_stream_get_latency(data->stream, &l_abs, &l_sign);
        int64_t latency = (l_sign) ? -(int64_t) l_abs : (int64_t) l_abs;
        
        // push structure
        struct source_audio out;
        out.data[0] = (uint8_t *) frames;
        out.frames = bytes / data->channels;
        out.speakers = data->speakers;
        out.samples_per_sec = data->samples_per_sec;
        out.timestamp = cur_time - (latency * 1000);
        out.format = AUDIO_FORMAT_U8BIT;
        
        // send data to obs
        obs_source_output_audio(data->source, &out);
        
        // clear pulse audio buffer
        pa_stream_drop(data->stream);
    }

    pulse_diconnect_stream(data);
    pulse_disconnect(data);
    
    return NULL;
}

static const char *pulse_getname(const char *locale)
{
    UNUSED_PARAMETER(locale);
    return "Pulse Audio Input";
}

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
    
    bfree(data);
}

static void *pulse_create(obs_data_t settings, obs_source_t source)
{
    UNUSED_PARAMETER(settings);
    
    struct pulse_data *data = bmalloc(sizeof(struct pulse_data));
    memset(data, 0, sizeof(struct pulse_data));

    data->source = source;
    data->frames = 480;
    data->samples_per_sec = 48000;
    data->speakers = SPEAKERS_STEREO;
    data->channels = (data->speakers == SPEAKERS_STEREO) ? 2 : 1;

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
