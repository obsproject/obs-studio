#include <util/dstr.h>
#include <obs-module.h>
#include <pulse/error.h>
#include "pulse-wrapper.h"
#include "pulse-utils.h"

#define PULSE_DATA(voidptr) struct pulse_data *data = voidptr;
#define blog(level, msg, ...) blog(level, "pulse-output: " msg, ##__VA_ARGS__)

struct pulse_data {
	obs_output_t *obs_output;
	pa_stream *pulse_stream;
	bool device_is_virtual;
	const char *device_name;
	const char *device_description;
	pa_sample_spec pulse_sample_spec;
	uint_fast32_t bytes_per_frame;
	int_fast32_t module_idx1;
	int_fast32_t module_idx2;
};

static void pulse_output_stop(void *data, uint64_t);

static const char *pulse_output_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "Pulseaudio Output";
}

static void *pulse_output_create(obs_data_t *settings, obs_output_t *obs_output)
{
	UNUSED_PARAMETER(settings);
	struct pulse_data *data =
		(struct pulse_data *)bzalloc(sizeof(struct pulse_data));

	data->device_name = obs_data_get_string(settings, "device");
	if (!data->device_name || strlen(data->device_name) == 0)
		data->device_name = "default";

	if ((data->device_is_virtual =
		     strncmp(data->device_name, "obs-", strlen("obs-")) == 0)) {
		data->device_description =
			obs_data_get_string(settings, "description");
		if (!data->device_description ||
		    strlen(data->device_description) == 0)
			data->device_description = data->device_name;
	}

	blog(LOG_INFO, "Creating output on %s", data->device_name);
	data->obs_output = obs_output;
	pulse_init();
	return data;
}

static void pulse_output_destroy(void *userdata)
{
	PULSE_DATA(userdata);
	blog(LOG_INFO, "Destroying output on %s", data->device_name);
	bfree(data);
	pulse_unref();
}

/* Response to pulse_get_server_info() */
static void pulse_server_info_cb(pa_context *c, const pa_server_info *i,
				 void *userdata)
{
	UNUSED_PARAMETER(c);
	PULSE_DATA(userdata);
	blog(LOG_INFO, "Using default sink: %s", i->default_sink_name);
	memcpy(&data->pulse_sample_spec, &i->sample_spec,
	       sizeof(pa_sample_spec));
	pulse_signal(0);
}

/* Response to pulse_get_sink_info() */
static void pulse_sink_info_cb(pa_context *c, const pa_sink_info *i, int eol,
			       void *userdata)
{
	UNUSED_PARAMETER(c);
	PULSE_DATA(userdata);
	if (eol == 0)
		memcpy(&data->pulse_sample_spec, &i->sample_spec,
		       sizeof(pa_sample_spec));
	else
		pulse_signal(0);
}

static bool pulse_output_start(void *userdata)
{
	PULSE_DATA(userdata);

	if (data->device_is_virtual) {
		blog(LOG_INFO, "Creating virtual cable %s (%s)",
		     data->device_name, data->device_description);
		struct dstr argument;
		dstr_init(&argument);

		dstr_printf(
			&argument,
			"sink_name=%s sink_properties=\"device.description='%s'\"",
			data->device_name, data->device_description);
		if ((data->module_idx1 = pulse_load_module(
			     "module-null-sink", argument.array)) < 0) {
			blog(LOG_ERROR, "Failed to load module-null-sink: %s",
			     pa_strerror(pulse_errno()));
			return false;
		}

		dstr_printf(
			&argument,
			"master=%s.monitor source_name=%s source_properties=\"device.description='%s'\"",
			data->device_name, data->device_name,
			data->device_description);
		if ((data->module_idx2 = pulse_load_module(
			     "module-remap-source", argument.array)) < 0) {
			blog(LOG_ERROR,
			     "Failed to load module-remap-source: %s",
			     pa_strerror(pulse_errno()));
			pulse_unload_module(data->module_idx1);
			return false;
		}

		dstr_free(&argument);
	}

	blog(LOG_INFO, "Starting output on %s", data->device_name);
	data->pulse_sample_spec.format = PA_SAMPLE_INVALID;
	if (strcmp(data->device_name, "default") == 0) {
		if (pulse_get_server_info(pulse_server_info_cb, (void *)data) <
		    0) {
			blog(LOG_ERROR, "Unable to get server info: %s",
			     pa_strerror(pulse_errno()));
			return -1;
		}
	} else {
		if (pulse_get_sink_info(pulse_sink_info_cb, data->device_name,
					(void *)data) < 0) {
			blog(LOG_ERROR, "Failed to get sink info: %s",
			     pa_strerror(pulse_errno()));
			return false;
		}
	}
	if (data->pulse_sample_spec.format == PA_SAMPLE_INVALID) {
		blog(LOG_ERROR, "Failed to get sink info: %s",
		     pa_strerror(pulse_errno()));
		return false;
	}

	blog(LOG_INFO,
	     "Sink's audio format: %s, %" PRIu32 " Hz"
	     ", %" PRIu8 " channels",
	     pa_sample_format_to_string(data->pulse_sample_spec.format),
	     data->pulse_sample_spec.rate, data->pulse_sample_spec.channels);

	struct audio_convert_info conversion = {};
	conversion.samples_per_sec = (uint32_t)data->pulse_sample_spec.rate;
	if ((conversion.format = pulse_to_obs_audio_format(
		     data->pulse_sample_spec.format)) == AUDIO_FORMAT_UNKNOWN) {
		conversion.format = AUDIO_FORMAT_FLOAT;
		data->pulse_sample_spec.format = PA_SAMPLE_FLOAT32LE;
		blog(LOG_INFO,
		     "Sink's preferred sample format %s not supported by OBS, "
		     "using %s instead",
		     pa_sample_format_to_string(data->pulse_sample_spec.format),
		     pa_sample_format_to_string(PA_SAMPLE_FLOAT32LE));
	}
	if ((conversion.speakers = pulse_channels_to_obs_speakers(
		     data->pulse_sample_spec.channels)) == SPEAKERS_UNKNOWN) {
		conversion.speakers = SPEAKERS_STEREO;
		data->pulse_sample_spec.channels = 2;
		blog(LOG_INFO,
		     "Sink's %c channels not supported by OBS, "
		     "using 2 instead",
		     data->pulse_sample_spec.channels);
	}
	obs_output_set_audio_conversion(data->obs_output, &conversion);
	pa_channel_map channel_map = pulse_channel_map(conversion.speakers);
	data->bytes_per_frame = pa_frame_size(&data->pulse_sample_spec);

	if (!(data->pulse_stream = pulse_stream_new(
		      "OBS Output", &data->pulse_sample_spec, &channel_map))) {
		blog(LOG_ERROR, "Unable to create stream: %s",
		     pa_strerror(pulse_errno()));
		return false;
	}

	pa_buffer_attr attr;
	attr.fragsize = (uint32_t)-1;
	attr.maxlength = (uint32_t)-1;
	attr.minreq = (uint32_t)-1;
	attr.prebuf = (uint32_t)-1;
	attr.tlength = pa_usec_to_bytes(25000, &data->pulse_sample_spec);

	pulse_lock();
	int_fast32_t ret = pa_stream_connect_playback(
		data->pulse_stream,
		strcmp(data->device_name, "default") == 0 ? NULL
							  : data->device_name,
		&attr, PA_STREAM_NOFLAGS, NULL, NULL);
	pulse_unlock();
	if (ret < 0) {
		blog(LOG_ERROR, "Unable to connect stream: %s",
		     pa_strerror(pulse_errno()));
		pulse_output_stop(data, 0);
		return false;
	}

	while (true) {
		int ready = pa_stream_get_state(data->pulse_stream);
		if (ready == PA_STREAM_READY)
			break;
		else if (ready == PA_STREAM_FAILED) {
			blog(LOG_ERROR, "pa_stream_get_state() failed: %s",
			     pa_strerror(pulse_errno()));
			pulse_output_stop(data, 0);
			return false;
		}
		pulse_wait();
	}

	obs_output_begin_data_capture(data->obs_output, 0 /* flags */);

	return true;
}

static void pulse_output_stop(void *userdata, uint64_t)
{
	PULSE_DATA(userdata);
	blog(LOG_INFO, "Stopping output on %s", data->device_name);

	obs_output_end_data_capture(data->obs_output);

	if (data->pulse_stream) {
		pulse_lock();
		pa_stream_disconnect(data->pulse_stream);
		pa_stream_unref(data->pulse_stream);
		pulse_unlock();
		data->pulse_stream = NULL;
	}

	if (data->module_idx2 > 0) {
		blog(LOG_INFO, "Destroying virtual cable %s (%s)",
		     data->device_name, data->device_description);
		pulse_unload_module(data->module_idx2);
		data->module_idx2 = 0;
	}
	if (data->module_idx1 > 0) {
		pulse_unload_module(data->module_idx1);
		data->module_idx1 = 0;
	}
}

static void pulse_output_raw_audio(void *userdata, struct audio_data *frames)
{
	PULSE_DATA(userdata);
	pulse_lock();

	uint8_t *buffer;
	size_t bytes = frames->frames * data->bytes_per_frame;
	size_t bytesToFill = bytes;
	if (pa_stream_begin_write(data->pulse_stream, (void **)&buffer,
				  &bytesToFill)) {
		blog(LOG_ERROR, "pa_stream_begin_write() failed: %s",
		     pa_strerror(pulse_errno()));
	} else if (bytesToFill < bytes) {
		blog(LOG_ERROR, "Pulse buffer overrun");
		pa_stream_cancel_write(data->pulse_stream);
	} else {
		memcpy(buffer, frames->data[0], bytes);
		pa_stream_write(data->pulse_stream, buffer, bytesToFill, NULL,
				0LL, PA_SEEK_RELATIVE);
	}

	pulse_unlock();
}

const struct obs_output_info pulse_output = {
	.id = "pulse_output",
	.flags = OBS_OUTPUT_AUDIO,
	.get_name = pulse_output_get_name,
	.create = pulse_output_create,
	.destroy = pulse_output_destroy,
	.start = pulse_output_start,
	.stop = pulse_output_stop,
	.raw_audio = pulse_output_raw_audio,
};
