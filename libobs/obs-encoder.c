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

struct obs_encoder_info *find_encoder(const char *id)
{
	for (size_t i = 0; i < obs->encoder_types.num; i++) {
		struct obs_encoder_info *info = obs->encoder_types.array+i;

		if (strcmp(info->id, id) == 0)
			return info;
	}

	return NULL;
}

const char *obs_encoder_get_display_name(const char *id)
{
	struct obs_encoder_info *ei = find_encoder(id);
	return ei ? ei->get_name() : NULL;
}

static bool init_encoder(struct obs_encoder *encoder, const char *name,
		obs_data_t *settings, obs_data_t *hotkey_data)
{
	pthread_mutexattr_t attr;

	pthread_mutex_init_value(&encoder->callbacks_mutex);
	pthread_mutex_init_value(&encoder->outputs_mutex);

	if (pthread_mutexattr_init(&attr) != 0)
		return false;
	if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) != 0)
		return false;
	if (!obs_context_data_init(&encoder->context, settings, name,
				hotkey_data))
		return false;
	if (pthread_mutex_init(&encoder->callbacks_mutex, &attr) != 0)
		return false;
	if (pthread_mutex_init(&encoder->outputs_mutex, NULL) != 0)
		return false;

	if (encoder->info.get_defaults)
		encoder->info.get_defaults(encoder->context.settings);

	return true;
}

static struct obs_encoder *create_encoder(const char *id,
		enum obs_encoder_type type, const char *name,
		obs_data_t *settings, size_t mixer_idx, obs_data_t *hotkey_data)
{
	struct obs_encoder *encoder;
	struct obs_encoder_info *ei = find_encoder(id);
	bool success;

	if (!ei || ei->type != type)
		return NULL;

	encoder = bzalloc(sizeof(struct obs_encoder));
	encoder->info = *ei;
	encoder->mixer_idx = mixer_idx;

	success = init_encoder(encoder, name, settings, hotkey_data);
	if (!success) {
		blog(LOG_ERROR, "creating encoder '%s' (%s) failed", name, id);
		obs_encoder_destroy(encoder);
		return NULL;
	}

	encoder->control = bzalloc(sizeof(obs_weak_encoder_t));
	encoder->control->encoder = encoder;

	obs_context_data_insert(&encoder->context,
			&obs->data.encoders_mutex,
			&obs->data.first_encoder);

	blog(LOG_INFO, "encoder '%s' (%s) created", name, id);
	return encoder;
}

obs_encoder_t *obs_video_encoder_create(const char *id, const char *name,
		obs_data_t *settings, obs_data_t *hotkey_data)
{
	if (!name || !id) return NULL;
	return create_encoder(id, OBS_ENCODER_VIDEO, name, settings, 0,
			hotkey_data);
}

obs_encoder_t *obs_audio_encoder_create(const char *id, const char *name,
		obs_data_t *settings, size_t mixer_idx, obs_data_t *hotkey_data)
{
	if (!name || !id) return NULL;
	return create_encoder(id, OBS_ENCODER_AUDIO, name, settings, mixer_idx,
			hotkey_data);
}

static void receive_video(void *param, struct video_data *frame);
static void receive_audio(void *param, size_t mix_idx, struct audio_data *data);

static inline void get_audio_info(const struct obs_encoder *encoder,
		struct audio_convert_info *info)
{
	const struct audio_output_info *aoi;
	aoi = audio_output_get_info(encoder->media);

	if (info->format == AUDIO_FORMAT_UNKNOWN)
		info->format = aoi->format;
	if (!info->samples_per_sec)
		info->samples_per_sec = aoi->samples_per_sec;
	if (info->speakers == SPEAKERS_UNKNOWN)
		info->speakers = aoi->speakers;

	if (encoder->info.get_audio_info)
		encoder->info.get_audio_info(encoder->context.data, info);
}

static inline void get_video_info(struct obs_encoder *encoder,
		struct video_scale_info *info)
{
	const struct video_output_info *voi;
	voi = video_output_get_info(encoder->media);

	info->format     = voi->format;
	info->colorspace = voi->colorspace;
	info->range      = voi->range;
	info->width      = obs_encoder_get_width(encoder);
	info->height     = obs_encoder_get_height(encoder);

	if (encoder->info.get_video_info)
		encoder->info.get_video_info(encoder->context.data, info);

	if (info->width != voi->width || info->height != voi->height)
		obs_encoder_set_scaled_size(encoder, info->width, info->height);
}

static inline bool has_scaling(const struct obs_encoder *encoder)
{
	uint32_t video_width  = video_output_get_width(encoder->media);
	uint32_t video_height = video_output_get_height(encoder->media);

	return encoder->scaled_width && encoder->scaled_height &&
		(video_width  != encoder->scaled_width ||
		 video_height != encoder->scaled_height);
}

static void add_connection(struct obs_encoder *encoder)
{
	if (encoder->info.type == OBS_ENCODER_AUDIO) {
		struct audio_convert_info audio_info = {0};
		get_audio_info(encoder, &audio_info);

		audio_output_connect(encoder->media, encoder->mixer_idx,
				&audio_info, receive_audio, encoder);
	} else {
		struct video_scale_info info = {0};
		get_video_info(encoder, &info);

		video_output_connect(encoder->media, &info, receive_video,
			encoder);
	}

	encoder->active = true;
}

static void remove_connection(struct obs_encoder *encoder)
{
	if (encoder->info.type == OBS_ENCODER_AUDIO)
		audio_output_disconnect(encoder->media, encoder->mixer_idx,
				receive_audio, encoder);
	else
		video_output_disconnect(encoder->media, receive_video,
				encoder);

	encoder->active = false;
}

static inline void free_audio_buffers(struct obs_encoder *encoder)
{
	for (size_t i = 0; i < MAX_AV_PLANES; i++) {
		circlebuf_free(&encoder->audio_input_buffer[i]);
		bfree(encoder->audio_output_buffer[i]);
		encoder->audio_output_buffer[i] = NULL;
	}
}

static void obs_encoder_actually_destroy(obs_encoder_t *encoder)
{
	if (encoder) {
		pthread_mutex_lock(&encoder->outputs_mutex);
		for (size_t i = 0; i < encoder->outputs.num; i++) {
			struct obs_output *output = encoder->outputs.array[i];
			obs_output_remove_encoder(output, encoder);
		}
		da_free(encoder->outputs);
		pthread_mutex_unlock(&encoder->outputs_mutex);

		blog(LOG_INFO, "encoder '%s' destroyed", encoder->context.name);

		free_audio_buffers(encoder);

		if (encoder->context.data)
			encoder->info.destroy(encoder->context.data);
		da_free(encoder->callbacks);
		pthread_mutex_destroy(&encoder->callbacks_mutex);
		pthread_mutex_destroy(&encoder->outputs_mutex);
		obs_context_data_free(&encoder->context);
		bfree(encoder);
	}
}

/* does not actually destroy the encoder until all connections to it have been
 * removed. (full reference counting really would have been superfluous) */
void obs_encoder_destroy(obs_encoder_t *encoder)
{
	if (encoder) {
		bool destroy;

		obs_context_data_remove(&encoder->context);

		pthread_mutex_lock(&encoder->callbacks_mutex);
		destroy = encoder->callbacks.num == 0;
		if (!destroy)
			encoder->destroy_on_stop = true;
		pthread_mutex_unlock(&encoder->callbacks_mutex);

		if (destroy)
			obs_encoder_actually_destroy(encoder);
	}
}

const char *obs_encoder_get_name(const obs_encoder_t *encoder)
{
	return encoder ? encoder->context.name : NULL;
}

void obs_encoder_set_name(obs_encoder_t *encoder, const char *name)
{
	if (!encoder) return;

	if (name && *name && strcmp(name, encoder->context.name) != 0)
		obs_context_data_setname(&encoder->context, name);
}

static inline obs_data_t *get_defaults(const struct obs_encoder_info *info)
{
	obs_data_t *settings = obs_data_create();
	if (info->get_defaults)
		info->get_defaults(settings);
	return settings;
}

obs_data_t *obs_encoder_defaults(const char *id)
{
	const struct obs_encoder_info *info = find_encoder(id);
	return (info) ? get_defaults(info) : NULL;
}

obs_properties_t *obs_get_encoder_properties(const char *id)
{
	const struct obs_encoder_info *ei = find_encoder(id);
	if (ei && ei->get_properties) {
		obs_data_t       *defaults = get_defaults(ei);
		obs_properties_t *properties;

		properties = ei->get_properties(NULL);
		obs_properties_apply_settings(properties, defaults);
		obs_data_release(defaults);
		return properties;
	}
	return NULL;
}

obs_properties_t *obs_encoder_properties(const obs_encoder_t *encoder)
{
	if (encoder && encoder->info.get_properties) {
		obs_properties_t *props;
		props = encoder->info.get_properties(encoder->context.data);
		obs_properties_apply_settings(props, encoder->context.settings);
		return props;
	}
	return NULL;
}

void obs_encoder_update(obs_encoder_t *encoder, obs_data_t *settings)
{
	if (!encoder) return;

	obs_data_apply(encoder->context.settings, settings);

	if (encoder->info.update && encoder->context.data)
		encoder->info.update(encoder->context.data,
				encoder->context.settings);
}

bool obs_encoder_get_extra_data(const obs_encoder_t *encoder,
		uint8_t **extra_data, size_t *size)
{
	if (encoder && encoder->info.get_extra_data && encoder->context.data)
		return encoder->info.get_extra_data(encoder->context.data,
				extra_data, size);

	return false;
}

obs_data_t *obs_encoder_get_settings(const obs_encoder_t *encoder)
{
	if (!encoder) return NULL;

	obs_data_addref(encoder->context.settings);
	return encoder->context.settings;
}

static inline void reset_audio_buffers(struct obs_encoder *encoder)
{
	free_audio_buffers(encoder);

	for (size_t i = 0; i < encoder->planes; i++)
		encoder->audio_output_buffer[i] =
			bmalloc(encoder->framesize_bytes);
}

static void intitialize_audio_encoder(struct obs_encoder *encoder)
{
	struct audio_convert_info info = {0};
	get_audio_info(encoder, &info);

	encoder->samplerate = info.samples_per_sec;
	encoder->planes     = get_audio_planes(info.format, info.speakers);
	encoder->blocksize  = get_audio_size(info.format, info.speakers, 1);
	encoder->framesize  = encoder->info.get_frame_size(
			encoder->context.data);

	encoder->framesize_bytes = encoder->blocksize * encoder->framesize;
	reset_audio_buffers(encoder);
}

bool obs_encoder_initialize(obs_encoder_t *encoder)
{
	if (!encoder) return false;

	if (encoder->active)
		return true;

	if (encoder->context.data)
		encoder->info.destroy(encoder->context.data);

	encoder->context.data = encoder->info.create(encoder->context.settings,
			encoder);
	if (!encoder->context.data)
		return false;

	encoder->paired_encoder  = NULL;
	encoder->start_ts        = 0;

	if (encoder->info.type == OBS_ENCODER_AUDIO)
		intitialize_audio_encoder(encoder);

	return true;
}

static inline size_t get_callback_idx(
		const struct obs_encoder *encoder,
		void (*new_packet)(void *param, struct encoder_packet *packet),
		void *param)
{
	for (size_t i = 0; i < encoder->callbacks.num; i++) {
		struct encoder_callback *cb = encoder->callbacks.array+i;

		if (cb->new_packet == new_packet && cb->param == param)
			return i;
	}

	return DARRAY_INVALID;
}

void obs_encoder_start(obs_encoder_t *encoder,
		void (*new_packet)(void *param, struct encoder_packet *packet),
		void *param)
{
	struct encoder_callback cb = {false, new_packet, param};
	bool first   = false;

	if (!encoder || !new_packet || !encoder->context.data) return;

	pthread_mutex_lock(&encoder->callbacks_mutex);

	first = (encoder->callbacks.num == 0);

	size_t idx = get_callback_idx(encoder, new_packet, param);
	if (idx == DARRAY_INVALID)
		da_push_back(encoder->callbacks, &cb);

	pthread_mutex_unlock(&encoder->callbacks_mutex);

	if (first) {
		encoder->cur_pts = 0;
		add_connection(encoder);
	}
}

void obs_encoder_stop(obs_encoder_t *encoder,
		void (*new_packet)(void *param, struct encoder_packet *packet),
		void *param)
{
	bool   last = false;
	size_t idx;

	if (!encoder) return;

	pthread_mutex_lock(&encoder->callbacks_mutex);

	idx = get_callback_idx(encoder, new_packet, param);
	if (idx != DARRAY_INVALID) {
		da_erase(encoder->callbacks, idx);
		last = (encoder->callbacks.num == 0);
	}

	pthread_mutex_unlock(&encoder->callbacks_mutex);

	if (last) {
		remove_connection(encoder);

		if (encoder->destroy_on_stop)
			obs_encoder_actually_destroy(encoder);
	}
}

const char *obs_encoder_get_codec(const obs_encoder_t *encoder)
{
	return encoder ? encoder->info.codec : NULL;
}

const char *obs_get_encoder_codec(const char *id)
{
	struct obs_encoder_info *info = find_encoder(id);
	return info ? info->codec : NULL;
}

enum obs_encoder_type obs_encoder_get_type(const obs_encoder_t *encoder)
{
	return encoder ? encoder->info.type : OBS_ENCODER_AUDIO;
}

enum obs_encoder_type obs_get_encoder_type(const char *id)
{
	struct obs_encoder_info *info = find_encoder(id);
	return info ? info->type : OBS_ENCODER_AUDIO;
}

void obs_encoder_set_scaled_size(obs_encoder_t *encoder, uint32_t width,
		uint32_t height)
{
	if (!encoder || encoder->info.type != OBS_ENCODER_VIDEO)
		return;

	if (encoder->active) {
		blog(LOG_WARNING, "encoder '%s': Cannot set the scaled "
		                  "resolution while the encoder is active",
		                  obs_encoder_get_name(encoder));
		return;
	}

	encoder->scaled_width  = width;
	encoder->scaled_height = height;
}

uint32_t obs_encoder_get_width(const obs_encoder_t *encoder)
{
	if (!encoder || !encoder->media ||
	    encoder->info.type != OBS_ENCODER_VIDEO)
		return 0;

	return encoder->scaled_width != 0 ?
		encoder->scaled_width :
		video_output_get_width(encoder->media);
}

uint32_t obs_encoder_get_height(const obs_encoder_t *encoder)
{
	if (!encoder || !encoder->media ||
	    encoder->info.type != OBS_ENCODER_VIDEO)
		return 0;

	return encoder->scaled_width != 0 ?
		encoder->scaled_height :
		video_output_get_height(encoder->media);
}

void obs_encoder_set_video(obs_encoder_t *encoder, video_t *video)
{
	const struct video_output_info *voi;

	if (!video || !encoder || encoder->info.type != OBS_ENCODER_VIDEO)
		return;

	voi = video_output_get_info(video);

	encoder->media        = video;
	encoder->timebase_num = voi->fps_den;
	encoder->timebase_den = voi->fps_num;
}

void obs_encoder_set_audio(obs_encoder_t *encoder, audio_t *audio)
{
	if (!audio || !encoder || encoder->info.type != OBS_ENCODER_AUDIO)
		return;

	encoder->media        = audio;
	encoder->timebase_num = 1;
	encoder->timebase_den = audio_output_get_sample_rate(audio);
}

video_t *obs_encoder_video(const obs_encoder_t *encoder)
{
	return (encoder && encoder->info.type == OBS_ENCODER_VIDEO) ?
		encoder->media : NULL;
}

audio_t *obs_encoder_audio(const obs_encoder_t *encoder)
{
	return (encoder && encoder->info.type == OBS_ENCODER_AUDIO) ?
		encoder->media : NULL;
}

bool obs_encoder_active(const obs_encoder_t *encoder)
{
	return encoder ? encoder->active : false;
}

static inline bool get_sei(const struct obs_encoder *encoder,
		uint8_t **sei, size_t *size)
{
	if (encoder->info.get_sei_data)
		return encoder->info.get_sei_data(encoder->context.data, sei,
				size);
	return false;
}

static void send_first_video_packet(struct obs_encoder *encoder,
		struct encoder_callback *cb, struct encoder_packet *packet)
{
	struct encoder_packet first_packet;
	DARRAY(uint8_t)       data;
	uint8_t               *sei;
	size_t                size;

	/* always wait for first keyframe */
	if (!packet->keyframe)
		return;

	da_init(data);

	if (!get_sei(encoder, &sei, &size)) {
		cb->new_packet(cb->param, packet);
		cb->sent_first_packet = true;
		return;
	}

	da_push_back_array(data, sei, size);
	da_push_back_array(data, packet->data, packet->size);

	first_packet      = *packet;
	first_packet.data = data.array;
	first_packet.size = data.num;

	cb->new_packet(cb->param, &first_packet);
	cb->sent_first_packet = true;

	da_free(data);
}

static inline void send_packet(struct obs_encoder *encoder,
		struct encoder_callback *cb, struct encoder_packet *packet)
{
	/* include SEI in first video packet */
	if (encoder->info.type == OBS_ENCODER_VIDEO && !cb->sent_first_packet)
		send_first_video_packet(encoder, cb, packet);
	else
		cb->new_packet(cb->param, packet);
}

static void full_stop(struct obs_encoder *encoder)
{
	if (encoder) {
		pthread_mutex_lock(&encoder->callbacks_mutex);
		da_free(encoder->callbacks);
		remove_connection(encoder);
		pthread_mutex_unlock(&encoder->callbacks_mutex);
	}
}

static inline void do_encode(struct obs_encoder *encoder,
		struct encoder_frame *frame)
{
	struct encoder_packet pkt = {0};
	bool received = false;
	bool success;

	pkt.timebase_num = encoder->timebase_num;
	pkt.timebase_den = encoder->timebase_den;
	pkt.encoder = encoder;

	success = encoder->info.encode(encoder->context.data, frame, &pkt,
			&received);
	if (!success) {
		full_stop(encoder);
		blog(LOG_ERROR, "Error encoding with encoder '%s'",
				encoder->context.name);
		return;
	}

	if (received) {
		/* we use system time here to ensure sync with other encoders,
		 * you do not want to use relative timestamps here */
		pkt.dts_usec = encoder->start_ts / 1000 + packet_dts_usec(&pkt);

		pthread_mutex_lock(&encoder->callbacks_mutex);

		for (size_t i = encoder->callbacks.num; i > 0; i--) {
			struct encoder_callback *cb;
			cb = encoder->callbacks.array+(i-1);
			send_packet(encoder, cb, &pkt);
		}

		pthread_mutex_unlock(&encoder->callbacks_mutex);
	}
}

static void receive_video(void *param, struct video_data *frame)
{
	struct obs_encoder    *encoder  = param;
	struct encoder_frame  enc_frame;

	memset(&enc_frame, 0, sizeof(struct encoder_frame));

	for (size_t i = 0; i < MAX_AV_PLANES; i++) {
		enc_frame.data[i]     = frame->data[i];
		enc_frame.linesize[i] = frame->linesize[i];
	}

	if (!encoder->start_ts)
		encoder->start_ts = frame->timestamp;

	enc_frame.frames = 1;
	enc_frame.pts    = encoder->cur_pts;

	do_encode(encoder, &enc_frame);

	encoder->cur_pts += encoder->timebase_num;
}

static bool buffer_audio(struct obs_encoder *encoder, struct audio_data *data)
{
	size_t samplerate = encoder->samplerate;
	size_t size = data->frames * encoder->blocksize;
	size_t offset_size = 0;

	if (!encoder->start_ts && encoder->paired_encoder) {
		uint64_t end_ts     = data->timestamp;
		uint64_t v_start_ts = encoder->paired_encoder->start_ts;

		/* no video yet, so don't start audio */
		if (!v_start_ts)
			return false;

		/* audio starting point still not synced with video starting
		 * point, so don't start audio */
		end_ts += (uint64_t)data->frames * 1000000000ULL / samplerate;
		if (end_ts <= v_start_ts)
			return false;

		/* ready to start audio, truncate if necessary */
		if (data->timestamp < v_start_ts) {
			uint64_t offset = v_start_ts - data->timestamp;
			offset = (int)(offset * samplerate / 1000000000);
			offset_size = (size_t)offset * encoder->blocksize;
		}

		encoder->start_ts = v_start_ts;

	} else if (!encoder->start_ts && !encoder->paired_encoder) {
		encoder->start_ts = data->timestamp;
	}

	size -= offset_size;

	/* push in to the circular buffer */
	if (size)
		for (size_t i = 0; i < encoder->planes; i++)
			circlebuf_push_back(&encoder->audio_input_buffer[i],
					data->data[i] + offset_size, size);

	return true;
}

static void send_audio_data(struct obs_encoder *encoder)
{
	struct encoder_frame  enc_frame;

	memset(&enc_frame, 0, sizeof(struct encoder_frame));

	for (size_t i = 0; i < encoder->planes; i++) {
		circlebuf_pop_front(&encoder->audio_input_buffer[i],
				encoder->audio_output_buffer[i],
				encoder->framesize_bytes);

		enc_frame.data[i]     = encoder->audio_output_buffer[i];
		enc_frame.linesize[i] = (uint32_t)encoder->framesize_bytes;
	}

	enc_frame.frames = (uint32_t)encoder->framesize;
	enc_frame.pts    = encoder->cur_pts;

	do_encode(encoder, &enc_frame);

	encoder->cur_pts += encoder->framesize;
}

static void receive_audio(void *param, size_t mix_idx, struct audio_data *data)
{
	struct obs_encoder *encoder = param;

	if (!buffer_audio(encoder, data))
		return;

	while (encoder->audio_input_buffer[0].size >= encoder->framesize_bytes)
		send_audio_data(encoder);

	UNUSED_PARAMETER(mix_idx);
}

void obs_encoder_add_output(struct obs_encoder *encoder,
		struct obs_output *output)
{
	if (!encoder) return;

	pthread_mutex_lock(&encoder->outputs_mutex);
	da_push_back(encoder->outputs, &output);
	pthread_mutex_unlock(&encoder->outputs_mutex);
}

void obs_encoder_remove_output(struct obs_encoder *encoder,
		struct obs_output *output)
{
	if (!encoder) return;

	pthread_mutex_lock(&encoder->outputs_mutex);
	da_erase_item(encoder->outputs, &output);
	pthread_mutex_unlock(&encoder->outputs_mutex);
}

void obs_duplicate_encoder_packet(struct encoder_packet *dst,
		const struct encoder_packet *src)
{
	*dst = *src;
	dst->data = bmemdup(src->data, src->size);
}

void obs_free_encoder_packet(struct encoder_packet *packet)
{
	bfree(packet->data);
	memset(packet, 0, sizeof(struct encoder_packet));
}

void obs_encoder_set_preferred_video_format(obs_encoder_t *encoder,
		enum video_format format)
{
	if (!encoder || encoder->info.type != OBS_ENCODER_VIDEO)
		return;

	encoder->preferred_format = format;
}

enum video_format obs_encoder_get_preferred_video_format(
		const obs_encoder_t *encoder)
{
	if (!encoder || encoder->info.type != OBS_ENCODER_VIDEO)
		return VIDEO_FORMAT_NONE;

	return encoder->preferred_format;
}

void obs_encoder_addref(obs_encoder_t *encoder)
{
	if (!encoder)
		return;

	obs_ref_addref(&encoder->control->ref);
}

void obs_encoder_release(obs_encoder_t *encoder)
{
	if (!encoder)
		return;

	obs_weak_encoder_t *control = encoder->control;
	if (obs_ref_release(&control->ref)) {
		// The order of operations is important here since
		// get_context_by_name in obs.c relies on weak refs
		// being alive while the context is listed
		obs_encoder_destroy(encoder);
		obs_weak_encoder_release(control);
	}
}

void obs_weak_encoder_addref(obs_weak_encoder_t *weak)
{
	if (!weak)
		return;

	obs_weak_ref_addref(&weak->ref);
}

void obs_weak_encoder_release(obs_weak_encoder_t *weak)
{
	if (!weak)
		return;

	if (obs_weak_ref_release(&weak->ref))
		bfree(weak);
}

obs_encoder_t *obs_encoder_get_ref(obs_encoder_t *encoder)
{
	if (!encoder)
		return NULL;

	return obs_weak_encoder_get_encoder(encoder->control);
}

obs_weak_encoder_t *obs_encoder_get_weak_encoder(obs_encoder_t *encoder)
{
	if (!encoder)
		return NULL;

	obs_weak_encoder_t *weak = encoder->control;
	obs_weak_encoder_addref(weak);
	return weak;
}

obs_encoder_t *obs_weak_encoder_get_encoder(obs_weak_encoder_t *weak)
{
	if (!weak)
		return NULL;

	if (obs_weak_ref_get_ref(&weak->ref))
		return weak->encoder;

	return NULL;
}

bool obs_weak_encoder_references_encoder(obs_weak_encoder_t *weak,
		obs_encoder_t *encoder)
{
	return weak && encoder && weak->encoder == encoder;
}
