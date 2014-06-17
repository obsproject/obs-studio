/******************************************************************************
    Copyright (C) 2013-2014 by Hugh Bailey <obs.jim@gmail.com>

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
******************************************************************************/

#include "obs.h"
#include "obs-internal.h"

static inline void signal_stop(struct obs_output *output, int code);

static inline const struct obs_output_info *find_output(const char *id)
{
	size_t i;
	for (i = 0; i < obs->output_types.num; i++)
		if (strcmp(obs->output_types.array[i].id, id) == 0)
			return obs->output_types.array+i;

	return NULL;
}

const char *obs_output_getdisplayname(const char *id, const char *locale)
{
	const struct obs_output_info *info = find_output(id);
	return (info != NULL) ? info->getname(locale) : NULL;
}

static const char *output_signals[] = {
	"void start(ptr output)",
	"void stop(ptr output, int code)",
	"void reconnect(ptr output)",
	"void reconnect_success(ptr output)",
	NULL
};

static bool init_output_handlers(struct obs_output *output, const char *name,
		obs_data_t settings)
{
	if (!obs_context_data_init(&output->context, settings, name))
		return false;

	signal_handler_add_array(output->context.signals, output_signals);
	return true;
}

obs_output_t obs_output_create(const char *id, const char *name,
		obs_data_t settings)
{
	const struct obs_output_info *info = find_output(id);
	struct obs_output *output;

	if (!info) {
		blog(LOG_ERROR, "Output '%s' not found", id);
		return NULL;
	}

	output = bzalloc(sizeof(struct obs_output));
	pthread_mutex_init_value(&output->interleaved_mutex);

	if (pthread_mutex_init(&output->interleaved_mutex, NULL) != 0)
		goto fail;
	if (!init_output_handlers(output, name, settings))
		goto fail;

	output->info     = *info;
	output->video    = obs_video();
	output->audio    = obs_audio();
	if (output->info.defaults)
		output->info.defaults(output->context.settings);

	output->context.data = info->create(output->context.settings, output);
	if (!output->context.data)
		goto fail;

	output->valid = true;

	obs_context_data_insert(&output->context,
			&obs->data.outputs_mutex,
			&obs->data.first_output);

	return output;

fail:
	obs_output_destroy(output);
	return NULL;
}

static inline void free_packets(struct obs_output *output)
{
	for (size_t i = 0; i < output->interleaved_packets.num; i++)
		obs_free_encoder_packet(output->interleaved_packets.array+i);
	da_free(output->interleaved_packets);
}

void obs_output_destroy(obs_output_t output)
{
	if (output) {
		obs_context_data_remove(&output->context);

		if (output->valid && output->active)
			output->info.stop(output->context.data);
		if (output->service)
			output->service->output = NULL;

		free_packets(output);

		if (output->context.data)
			output->info.destroy(output->context.data);

		if (output->video_encoder) {
			obs_encoder_remove_output(output->video_encoder,
					output);
		}
		if (output->audio_encoder) {
			obs_encoder_remove_output(output->audio_encoder,
					output);
		}

		pthread_mutex_destroy(&output->interleaved_mutex);
		obs_context_data_free(&output->context);
		bfree(output);
	}
}

bool obs_output_start(obs_output_t output)
{
	return (output != NULL) ?
		output->info.start(output->context.data) : false;
}

void obs_output_stop(obs_output_t output)
{
	if (output) {
		output->info.stop(output->context.data);
		signal_stop(output, OBS_OUTPUT_SUCCESS);
	}
}

bool obs_output_active(obs_output_t output)
{
	return (output != NULL) ? output->active : false;
}

static inline obs_data_t get_defaults(const struct obs_output_info *info)
{
	obs_data_t settings = obs_data_create();
	if (info->defaults)
		info->defaults(settings);
	return settings;
}

obs_data_t obs_output_defaults(const char *id)
{
	const struct obs_output_info *info = find_output(id);
	return (info) ? get_defaults(info) : NULL;
}

obs_properties_t obs_get_output_properties(const char *id, const char *locale)
{
	const struct obs_output_info *info = find_output(id);
	if (info && info->properties) {
		obs_data_t       defaults = get_defaults(info);
		obs_properties_t properties;

		properties = info->properties(locale);
		obs_properties_apply_settings(properties, defaults);
		obs_data_release(defaults);
		return properties;
	}
	return NULL;
}

obs_properties_t obs_output_properties(obs_output_t output, const char *locale)
{
	if (output && output->info.properties) {
		obs_properties_t props;
		props = output->info.properties(locale);
		obs_properties_apply_settings(props, output->context.settings);
		return props;
	}

	return NULL;
}

void obs_output_update(obs_output_t output, obs_data_t settings)
{
	if (!output) return;

	obs_data_apply(output->context.settings, settings);

	if (output->info.update)
		output->info.update(output->context.data,
				output->context.settings);
}

obs_data_t obs_output_get_settings(obs_output_t output)
{
	if (!output)
		return NULL;

	obs_data_addref(output->context.settings);
	return output->context.settings;
}

bool obs_output_canpause(obs_output_t output)
{
	return output ? (output->info.pause != NULL) : false;
}

void obs_output_pause(obs_output_t output)
{
	if (output && output->info.pause)
		output->info.pause(output->context.data);
}

signal_handler_t obs_output_signalhandler(obs_output_t output)
{
	return output ? output->context.signals : NULL;
}

proc_handler_t obs_output_prochandler(obs_output_t output)
{
	return output ? output->context.procs : NULL;
}

void obs_output_set_media(obs_output_t output, video_t video, audio_t audio)
{
	if (!output)
		return;

	output->video = video;
	output->audio = audio;
}

video_t obs_output_video(obs_output_t output)
{
	return output ? output->video : NULL;
}

audio_t obs_output_audio(obs_output_t output)
{
	return output ? output->audio : NULL;
}

void obs_output_remove_encoder(struct obs_output *output,
		struct obs_encoder *encoder)
{
	if (!output) return;

	if (output->video_encoder == encoder)
		output->video_encoder = NULL;
	else if (output->audio_encoder == encoder)
		output->audio_encoder = NULL;
}

void obs_output_set_video_encoder(obs_output_t output, obs_encoder_t encoder)
{
	if (!output) return;
	if (output->video_encoder == encoder) return;
	if (encoder && encoder->info.type != OBS_ENCODER_VIDEO) return;

	obs_encoder_remove_output(encoder, output);
	obs_encoder_add_output(encoder, output);
	output->video_encoder = encoder;
}

void obs_output_set_audio_encoder(obs_output_t output, obs_encoder_t encoder)
{
	if (!output) return;
	if (output->audio_encoder == encoder) return;
	if (encoder && encoder->info.type != OBS_ENCODER_AUDIO) return;

	obs_encoder_remove_output(encoder, output);
	obs_encoder_add_output(encoder, output);
	output->audio_encoder = encoder;
}

obs_encoder_t obs_output_get_video_encoder(obs_output_t output)
{
	return output ? output->video_encoder : NULL;
}

obs_encoder_t obs_output_get_audio_encoder(obs_output_t output)
{
	return output ? output->audio_encoder : NULL;
}

void obs_output_set_service(obs_output_t output, obs_service_t service)
{
	if (!output || output->active || !service || service->active) return;

	if (service->output)
		service->output->service = NULL;

	output->service = service;
	service->output = output;
}

obs_service_t obs_output_get_service(obs_output_t output)
{
	return output ? output->service : NULL;
}

void obs_output_set_video_conversion(obs_output_t output,
		const struct video_scale_info *conversion)
{
	if (!output || !conversion) return;

	output->video_conversion = *conversion;
	output->video_conversion_set = true;
}

void obs_output_set_audio_conversion(obs_output_t output,
		const struct audio_convert_info *conversion)
{
	if (!output || !conversion) return;

	output->audio_conversion = *conversion;
	output->audio_conversion_set = true;
}

static bool can_begin_data_capture(struct obs_output *output, bool encoded,
		bool has_video, bool has_audio, bool has_service)
{
	if (has_video) {
		if (encoded) {
			if (!output->video_encoder)
				return false;
		} else {
			if (!output->video)
				return false;
		}
	}

	if (has_audio) {
		if (encoded) {
			if (!output->audio_encoder)
				return false;
		} else {
			if (!output->audio)
				return false;
		}
	}

	if (has_service && !output->service)
		return false;

	return true;
}

static inline struct video_scale_info *get_video_conversion(
		struct obs_output *output)
{
	return output->video_conversion_set ? &output->video_conversion : NULL;
}

static inline struct audio_convert_info *get_audio_conversion(
		struct obs_output *output)
{
	return output->audio_conversion_set ? &output->audio_conversion : NULL;
}

static bool prepare_interleaved_packet(struct obs_output *output,
		struct encoder_packet *out, struct encoder_packet *in)
{
	int64_t offset;

	/* audio and video need to start at timestamp 0, and the encoders
	 * may not currently be at 0 when we get data.  so, we store the
	 * current dts as offset and subtract that value from the dts/pts
	 * of the output packet. */
	if (in->type == OBS_ENCODER_VIDEO) {
		if (!output->received_video) {
			output->first_video_ts = in->dts_usec;
			output->video_offset   = in->dts;
			output->received_video = true;
		}

		offset = output->video_offset;
	} else{
		/* don't accept audio that's before the first video timestamp */
		if (!output->received_video ||
		    in->dts_usec < output->first_video_ts)
			return false;

		if (!output->received_audio) {
			output->audio_offset   = in->dts;
			output->received_audio = true;
		}

		offset = output->audio_offset;
	}

	obs_duplicate_encoder_packet(out, in);
	out->dts -= offset;
	out->pts -= offset;

	/* convert the newly adjusted dts to relative dts time to ensure proper
	 * interleaving.  if we're using an audio encoder that's already been
	 * started on another output, then the first audio packet may not be
	 * quite perfectly synced up in terms of system time (and there's
	 * nothing we can really do about that), but it will always at least be
	 * within a 23ish millisecond threshold (at least for AAC) */
	out->dts_usec = packet_dts_usec(out);
	return true;
}

static inline bool has_higher_opposing_ts(struct obs_output *output,
		struct encoder_packet *packet)
{
	if (packet->type == OBS_ENCODER_VIDEO)
		return output->highest_audio_ts > packet->dts_usec;
	else
		return output->highest_video_ts > packet->dts_usec;
}

static inline void send_interleaved(struct obs_output *output)
{
	struct encoder_packet out = output->interleaved_packets.array[0];

	/* do not send an interleaved packet if there's no packet of the
	 * opposing type of a higher timstamp in the interleave buffer.
	 * this ensures that the timestamps are monotonic */
	if (!has_higher_opposing_ts(output, &out))
		return;

	da_erase(output->interleaved_packets, 0);
	output->info.encoded_packet(output->context.data, &out);
	obs_free_encoder_packet(&out);
}

static inline void set_higher_ts(struct obs_output *output,
		struct encoder_packet *packet)
{
	if (packet->type == OBS_ENCODER_VIDEO) {
		if (output->highest_video_ts < packet->dts_usec)
			output->highest_video_ts = packet->dts_usec;
	} else {
		if (output->highest_audio_ts < packet->dts_usec)
			output->highest_audio_ts = packet->dts_usec;
	}
}

static void interleave_packets(void *data, struct encoder_packet *packet)
{
	struct obs_output     *output = data;
	struct encoder_packet out;
	size_t                idx;

	pthread_mutex_lock(&output->interleaved_mutex);

	if (prepare_interleaved_packet(output, &out, packet)) {
		for (idx = 0; idx < output->interleaved_packets.num; idx++) {
			struct encoder_packet *cur_packet;
			cur_packet = output->interleaved_packets.array + idx;

			if (out.dts_usec < cur_packet->dts_usec)
				break;
		}

		da_insert(output->interleaved_packets, idx, &out);
		set_higher_ts(output, &out);

		/* when both video and audio have been received, we're ready
		 * to start sending out packets (one at a time) */
		if (output->received_audio && output->received_video)
			send_interleaved(output);
	}

	pthread_mutex_unlock(&output->interleaved_mutex);
}

static void hook_data_capture(struct obs_output *output, bool encoded,
		bool has_video, bool has_audio)
{
	void (*encoded_callback)(void *data, struct encoder_packet *packet);
	void *param;

	if (encoded) {
		output->received_video   = false;
		output->received_video   = false;
		output->highest_audio_ts = 0;
		output->highest_video_ts = 0;
		free_packets(output);

		encoded_callback = (has_video && has_audio) ?
			interleave_packets : output->info.encoded_packet;
		param = (has_video && has_audio) ?
			output : output->context.data;

		if (has_video)
			obs_encoder_start(output->video_encoder,
					encoded_callback, param);
		if (has_audio)
			obs_encoder_start(output->audio_encoder,
					encoded_callback, param);
	} else {
		if (has_video)
			video_output_connect(output->video,
					get_video_conversion(output),
					output->info.raw_video,
					output->context.data);
		if (has_audio)
			audio_output_connect(output->audio,
					get_audio_conversion(output),
					output->info.raw_audio,
					output->context.data);
	}
}

static inline void signal_start(struct obs_output *output)
{
	struct calldata params = {0};
	calldata_setptr(&params, "output", output);
	signal_handler_signal(output->context.signals, "start", &params);
	calldata_free(&params);
}

static inline void signal_stop(struct obs_output *output, int code)
{
	struct calldata params = {0};
	calldata_setint(&params, "code", code);
	calldata_setptr(&params, "output", output);
	signal_handler_signal(output->context.signals, "stop", &params);
	calldata_free(&params);
}

static inline void convert_flags(struct obs_output *output, uint32_t flags,
		bool *encoded, bool *has_video, bool *has_audio,
		bool *has_service)
{
	*encoded = (output->info.flags & OBS_OUTPUT_ENCODED) != 0;
	if (!flags)
		flags = output->info.flags;
	else
		flags &= output->info.flags;

	*has_video   = (flags & OBS_OUTPUT_VIDEO)   != 0;
	*has_audio   = (flags & OBS_OUTPUT_AUDIO)   != 0;
	*has_service = (flags & OBS_OUTPUT_SERVICE) != 0;
}

bool obs_output_can_begin_data_capture(obs_output_t output, uint32_t flags)
{
	bool encoded, has_video, has_audio, has_service;

	if (!output) return false;
	if (output->active) return false;

	convert_flags(output, flags, &encoded, &has_video, &has_audio,
			&has_service);

	return can_begin_data_capture(output, encoded, has_video, has_audio,
			has_service);
}

bool obs_output_initialize_encoders(obs_output_t output, uint32_t flags)
{
	bool encoded, has_video, has_audio, has_service;

	if (!output) return false;
	if (output->active) return false;

	convert_flags(output, flags, &encoded, &has_video, &has_audio,
			&has_service);

	if (!encoded)
		return false;
	if (has_service && !obs_service_initialize(output->service, output))
		return false;
	if (has_video && !obs_encoder_initialize(output->video_encoder))
		return false;
	if (has_audio && !obs_encoder_initialize(output->audio_encoder))
		return false;

	if (has_video && has_audio && !output->audio_encoder->active &&
	    !output->video_encoder->active) {
		output->audio_encoder->wait_for_video = true;
		output->video_encoder->paired_encoder = output->audio_encoder;
		output->audio_encoder->paired_encoder = output->video_encoder;
	}

	return true;
}

bool obs_output_begin_data_capture(obs_output_t output, uint32_t flags)
{
	bool encoded, has_video, has_audio, has_service;

	if (!output) return false;
	if (output->active) return false;

	convert_flags(output, flags, &encoded, &has_video, &has_audio,
			&has_service);

	if (!can_begin_data_capture(output, encoded, has_video, has_audio,
				has_service))
		return false;

	hook_data_capture(output, encoded, has_video, has_audio);

	if (has_service)
		obs_service_activate(output->service);

	output->active = true;
	signal_start(output);
	return true;
}

void obs_output_end_data_capture(obs_output_t output)
{
	bool encoded, has_video, has_audio, has_service;
	void (*encoded_callback)(void *data, struct encoder_packet *packet);
	void *param;

	if (!output) return;
	if (!output->active) return;

	convert_flags(output, 0, &encoded, &has_video, &has_audio,
			&has_service);

	if (encoded) {
		encoded_callback = (has_video && has_audio) ?
			interleave_packets : output->info.encoded_packet;
		param = (has_video && has_audio) ?
			output : output->context.data;

		if (has_video)
			obs_encoder_stop(output->video_encoder,
					encoded_callback, param);
		if (has_audio)
			obs_encoder_stop(output->audio_encoder,
					encoded_callback, param);
	} else {
		if (has_video)
			video_output_disconnect(output->video,
					output->info.raw_video,
					output->context.data);
		if (has_audio)
			audio_output_disconnect(output->audio,
					output->info.raw_audio,
					output->context.data);
	}

	if (has_service)
		obs_service_deactivate(output->service, false);

	output->active = false;
}

void obs_output_signal_stop(obs_output_t output, int code)
{
	obs_output_end_data_capture(output);
	signal_stop(output, code);
}
