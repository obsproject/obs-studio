/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

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

static inline struct obs_encoder_info *get_encoder_info(const char *id)
{
	for (size_t i = 0; i < obs->encoder_types.num; i++) {
		struct obs_encoder_info *info = obs->encoder_types.array+i;

		if (strcmp(info->id, id) == 0)
			return info;
	}

	return NULL;
}

const char *obs_encoder_getdisplayname(const char *id, const char *locale)
{
	struct obs_encoder_info *ei = get_encoder_info(id);
	return ei ? ei->getname(locale) : NULL;
}

static bool init_encoder(struct obs_encoder *encoder, const char *name,
		obs_data_t settings)
{
	pthread_mutex_init_value(&encoder->callbacks_mutex);
	pthread_mutex_init_value(&encoder->outputs_mutex);

	if (pthread_mutex_init(&encoder->callbacks_mutex, NULL) != 0)
		return false;
	if (pthread_mutex_init(&encoder->outputs_mutex, NULL) != 0)
		return false;

	encoder->settings = obs_data_newref(settings);
	if (encoder->info.defaults)
		encoder->info.defaults(encoder->settings);

	pthread_mutex_lock(&obs->data.encoders_mutex);
	da_push_back(obs->data.encoders, &encoder);
	pthread_mutex_unlock(&obs->data.encoders_mutex);

	encoder->name = bstrdup(name);
	return true;
}

static struct obs_encoder *create_encoder(const char *id,
		enum obs_encoder_type type, const char *name,
		obs_data_t settings, void *media,
		uint32_t timebase_num, uint32_t timebase_den)
{
	struct obs_encoder *encoder;
	struct obs_encoder_info *ei = get_encoder_info(id);
	bool success;

	if (!ei || ei->type != type)
		return NULL;

	encoder = bzalloc(sizeof(struct obs_encoder));
	encoder->info         = *ei;
	encoder->media        = media;
	encoder->timebase_num = timebase_num;
	encoder->timebase_den = timebase_den;

	success = init_encoder(encoder, name, settings);
	if (!success) {
		bfree(encoder);
		encoder = NULL;
	}

	return encoder;
}

obs_encoder_t obs_encoder_create_video(const char *id, const char *name,
		obs_data_t settings, video_t video)
{
	const struct video_output_info *voi;

	if (!name || !id || !video)
		return NULL;

	voi = video_output_getinfo(video);
	return create_encoder(id, OBS_ENCODER_VIDEO, name, settings, video,
			voi->fps_den, voi->fps_num);
}

obs_encoder_t obs_encoder_create_audio(const char *id, const char *name,
		obs_data_t settings, audio_t audio)
{
	const struct audio_output_info *aoi;

	if (!name || !id || !audio)
		return NULL;

	aoi = audio_output_getinfo(audio);
	return create_encoder(id, OBS_ENCODER_AUDIO, name, settings, audio,
			1, aoi->samples_per_sec);
}

static void receive_video(void *param, struct video_data *frame);
static void receive_audio(void *param, struct audio_data *data);

static inline struct audio_convert_info *get_audio_info(
		struct obs_encoder *encoder, struct audio_convert_info *info)
{
	if (encoder->info.audio_info)
		if (encoder->info.audio_info(encoder->data, info))
			return info;

	return false;
}

static inline struct video_scale_info *get_video_info(
		struct obs_encoder *encoder, struct video_scale_info *info)
{
	if (encoder->info.video_info)
		if (encoder->info.video_info(encoder->data, info))
			return info;

	return NULL;
}

static void add_connection(struct obs_encoder *encoder)
{
	struct audio_convert_info audio_info = {0};
	struct video_scale_info   video_info = {0};

	if (encoder->info.type == OBS_ENCODER_AUDIO) {
		struct audio_convert_info *info = NULL;

		info = get_audio_info(encoder, &audio_info);
		audio_output_connect(encoder->media, info, receive_audio,
				encoder);
	} else {
		struct video_scale_info *info = NULL;

		info = get_video_info(encoder, &video_info);
		video_output_connect(encoder->media, info, receive_video,
				encoder);
	}

	encoder->active = true;
}

static void remove_connection(struct obs_encoder *encoder)
{
	if (encoder->info.type == OBS_ENCODER_AUDIO)
		audio_output_disconnect(encoder->media, receive_audio,
				encoder);
	else
		video_output_disconnect(encoder->media, receive_video,
				encoder);

	encoder->active = false;
}

static void obs_encoder_actually_destroy(obs_encoder_t encoder)
{
	if (encoder) {
		pthread_mutex_lock(&encoder->outputs_mutex);
		for (size_t i = 0; i < encoder->outputs.num; i++) {
			struct obs_output *output = encoder->outputs.array[i];
			obs_output_remove_encoder(output, encoder);
		}
		da_free(encoder->outputs);
		pthread_mutex_unlock(&encoder->outputs_mutex);

		if (encoder->data)
			encoder->info.destroy(encoder->data);
		obs_data_release(encoder->settings);
		pthread_mutex_destroy(&encoder->callbacks_mutex);
		pthread_mutex_destroy(&encoder->outputs_mutex);
		bfree(encoder->name);
		bfree(encoder);
	}
}

/* does not actually destroy the encoder until all connections to it have been
 * removed. (full reference counting really would have been superfluous) */
void obs_encoder_destroy(obs_encoder_t encoder)
{
	if (encoder) {
		bool destroy;

		pthread_mutex_lock(&obs->data.encoders_mutex);
		da_erase_item(obs->data.encoders, &encoder);
		pthread_mutex_unlock(&obs->data.encoders_mutex);

		pthread_mutex_lock(&encoder->callbacks_mutex);
		destroy = encoder->callbacks.num == 0;
		if (!destroy)
			encoder->destroy_on_stop = true;
		pthread_mutex_unlock(&encoder->callbacks_mutex);

		if (destroy)
			obs_encoder_actually_destroy(encoder);
	}
}

static inline obs_data_t get_defaults(const struct obs_encoder_info *info)
{
	obs_data_t settings = obs_data_create();
	if (info->defaults)
		info->defaults(settings);
	return settings;
}

obs_data_t obs_encoder_defaults(const char *id)
{
	const struct obs_encoder_info *info = get_encoder_info(id);
	return (info) ? get_defaults(info) : NULL;
}

obs_properties_t obs_get_encoder_properties(const char *id, const char *locale)
{
	const struct obs_encoder_info *ei = get_encoder_info(id);
	if (ei && ei->properties) {
		obs_data_t       defaults = get_defaults(ei);
		obs_properties_t properties;

		properties = ei->properties(locale);
		obs_properties_apply_settings(properties, defaults);
		obs_data_release(defaults);
		return properties;
	}
	return NULL;
}

obs_properties_t obs_encoder_properties(obs_encoder_t encoder,
		const char *locale)
{
	if (encoder && encoder->info.properties) {
		obs_properties_t props;
		props = encoder->info.properties(locale);
		obs_properties_apply_settings(props, encoder->settings);
		return props;
	}
	return NULL;
}

void obs_encoder_update(obs_encoder_t encoder, obs_data_t settings)
{
	if (!encoder) return;

	obs_data_apply(encoder->settings, settings);
	if (encoder->info.update && encoder->data)
		encoder->info.update(encoder->data, encoder->settings);
}

bool obs_encoder_get_extra_data(obs_encoder_t encoder, uint8_t **extra_data,
		size_t *size)
{
	if (encoder && encoder->info.extra_data && encoder->data)
		return encoder->info.extra_data(encoder->data, extra_data,
				size);

	return false;
}

obs_data_t obs_encoder_get_settings(obs_encoder_t encoder)
{
	if (!encoder) return NULL;

	obs_data_addref(encoder->settings);
	return encoder->settings;
}

bool obs_encoder_initialize(obs_encoder_t encoder)
{
	if (!encoder) return false;

	if (encoder->active)
		return true;

	if (encoder->data)
		encoder->info.destroy(encoder->data);

	encoder->data = encoder->info.create(encoder->settings, encoder);
	return encoder->data != NULL;
}

static inline size_t get_callback_idx(
		struct obs_encoder *encoder,
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

void obs_encoder_start(obs_encoder_t encoder,
		void (*new_packet)(void *param, struct encoder_packet *packet),
		void *param)
{
	struct encoder_callback cb = {false, new_packet, param};
	bool success = true;
	bool first   = false;

	if (!encoder || !new_packet || !encoder->data) return;

	pthread_mutex_lock(&encoder->callbacks_mutex);

	first = (encoder->callbacks.num == 0);

	if (success) {
		size_t idx = get_callback_idx(encoder, new_packet, param);
		if (idx == DARRAY_INVALID)
			da_push_back(encoder->callbacks, &cb);
		else
			success = false;
	}

	pthread_mutex_unlock(&encoder->callbacks_mutex);

	if (first) {
		encoder->cur_pts = 0;
		add_connection(encoder);
	}
}

void obs_encoder_stop(obs_encoder_t encoder,
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

const char *obs_encoder_get_codec(obs_encoder_t encoder)
{
	return encoder ? encoder->info.codec : NULL;
}

video_t obs_encoder_video(obs_encoder_t encoder)
{
	return (encoder && encoder->info.type == OBS_ENCODER_VIDEO) ?
		encoder->media : NULL;
}

audio_t obs_encoder_audio(obs_encoder_t encoder)
{
	return (encoder && encoder->info.type == OBS_ENCODER_AUDIO) ?
		encoder->media : NULL;
}

static inline bool get_sei(struct obs_encoder *encoder,
		uint8_t **sei, size_t *size)
{
	if (encoder->info.sei_data)
		return encoder->info.sei_data(encoder->data, sei, size);
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
		struct encoder_frame *frame, struct encoder_packet *packet)
{
	bool received = false;
	bool success;

	packet->timebase_num = encoder->timebase_num;
	packet->timebase_den = encoder->timebase_den;

	success = encoder->info.encode(encoder->data, frame, packet, &received);
	if (!success) {
		full_stop(encoder);
		blog(LOG_ERROR, "Error encoding with encoder '%s'",
				encoder->name);
		return;
	}

	if (received) {
		pthread_mutex_lock(&encoder->callbacks_mutex);

		for (size_t i = 0; i < encoder->callbacks.num; i++) {
			struct encoder_callback *cb;
			cb = encoder->callbacks.array+i;
			send_packet(encoder, cb, packet);
		}

		pthread_mutex_unlock(&encoder->callbacks_mutex);
	}
}

static void receive_video(void *param, struct video_data *frame)
{
	struct obs_encoder    *encoder  = param;
	struct encoder_packet packet    = {0};
	struct encoder_frame  enc_frame;

	memset(&enc_frame, 0, sizeof(struct encoder_frame));

	for (size_t i = 0; i < MAX_AV_PLANES; i++) {
		enc_frame.data[i]     = frame->data[i];
		enc_frame.linesize[i] = frame->linesize[i];
	}

	enc_frame.frames = 1;
	enc_frame.pts    = encoder->cur_pts;

	do_encode(encoder, &enc_frame, &packet);

	encoder->cur_pts += encoder->timebase_num;
}

static void receive_audio(void *param, struct audio_data *data)
{
	struct obs_encoder    *encoder  = param;
	struct encoder_packet packet    = {0};
	struct encoder_frame  enc_frame;
	size_t                data_size;

	memset(&enc_frame, 0, sizeof(struct encoder_frame));

	data_size = audio_output_blocksize(encoder->media) * data->frames;

	for (size_t i = 0; i < MAX_AV_PLANES; i++) {
		if (data->data[i]) {
			enc_frame.data[i]     = data->data[i];
			enc_frame.linesize[i] = (uint32_t)data_size;
		}
	}

	enc_frame.frames = data->frames;
	enc_frame.pts    = encoder->cur_pts;

	do_encode(encoder, &enc_frame, &packet);

	encoder->cur_pts += data->frames;
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
