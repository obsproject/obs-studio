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
#include "obs-data.h"

bool load_encoder_info(void *module, const char *module_name,
		const char *encoder_id, struct encoder_info *info)
{
	info->getname = load_module_subfunc(module, module_name,
			encoder_id, "getname", true);
	info->create = load_module_subfunc(module, module_name,
			encoder_id, "create", true);
	info->destroy = load_module_subfunc(module, module_name,
			encoder_id, "destroy", true);
	info->update = load_module_subfunc(module, module_name,
			encoder_id, "update", true);
	info->reset = load_module_subfunc(module, module_name,
			encoder_id, "reset", true);
	info->encode = load_module_subfunc(module, module_name,
			encoder_id, "encode", true);
	info->getheader = load_module_subfunc(module, module_name,
			encoder_id, "getheader", true);

	if (!info->getname || !info->create || !info->destroy ||
	    !info->reset || !info->encode || !info->getheader)
		return false;

	info->setbitrate = load_module_subfunc(module, module_name,
			encoder_id, "setbitrate", false);
	info->request_keyframe = load_module_subfunc(module, module_name,
			encoder_id, "request_keyframe", false);

	info->id = encoder_id;
	return true;
}

static inline struct encoder_info *get_encoder_info(const char *id)
{
	for (size_t i = 0; i < obs->encoder_types.num; i++) {
		struct encoder_info *info = obs->encoder_types.array+i;

		if (strcmp(info->id, id) == 0)
			return info;
	}

	return NULL;
}

const char *obs_encoder_getdisplayname(const char *id, const char *locale)
{
	struct encoder_info *ei = get_encoder_info(id);
	if (!ei)
		return NULL;

	return ei->getname(locale);
}

obs_encoder_t obs_encoder_create(const char *id, const char *name,
		const char *settings)
{
	struct obs_encoder *encoder;
	struct encoder_info *ei = get_encoder_info(id);

	if (!ei)
		return NULL;

	encoder = bmalloc(sizeof(struct obs_encoder));
	memset(encoder, 0, sizeof(struct obs_encoder));
	encoder->callbacks = *ei;

	if (pthread_mutex_init(&encoder->data_callbacks_mutex, NULL) != 0) {
		bfree(encoder);
		return NULL;
	}

	encoder->data = ei->create(settings, encoder);
	if (!encoder->data) {
		pthread_mutex_destroy(&encoder->data_callbacks_mutex);
		bfree(encoder);
		return NULL;
	}

	dstr_copy(&encoder->settings, settings);

	pthread_mutex_lock(&obs->data.encoders_mutex);
	da_push_back(obs->data.encoders, &encoder);
	pthread_mutex_unlock(&obs->data.encoders_mutex);
	return encoder;
}

void obs_encoder_destroy(obs_encoder_t encoder)
{
	if (encoder) {
		pthread_mutex_lock(&obs->data.encoders_mutex);
		da_erase_item(obs->data.encoders, &encoder);
		pthread_mutex_unlock(&obs->data.encoders_mutex);

		encoder->callbacks.destroy(encoder->data);
		dstr_free(&encoder->settings);
		bfree(encoder);
	}
}

void obs_encoder_update(obs_encoder_t encoder, const char *settings)
{
	encoder->callbacks.update(encoder->data, settings);
}

bool obs_encoder_reset(obs_encoder_t encoder)
{
	return encoder->callbacks.reset(encoder->data);
}

bool obs_encoder_encode(obs_encoder_t encoder, void *frames, size_t size)
{
	/* TODO */
	//encoder->callbacks.encode(encoder->data, frames, size, packets);
	return false;
}

int obs_encoder_getheader(obs_encoder_t encoder,
		struct encoder_packet **packets)
{
	return encoder->callbacks.getheader(encoder, packets);
}

void obs_encoder_setbitrate(obs_encoder_t encoder, uint32_t bitrate,
		uint32_t buffersize)
{
	if (encoder->callbacks.setbitrate)
		encoder->callbacks.setbitrate(encoder->data, bitrate,
				buffersize);
}

void obs_encoder_request_keyframe(obs_encoder_t encoder)
{
	if (encoder->callbacks.request_keyframe)
		encoder->callbacks.request_keyframe(encoder->data);
}

const char *obs_encoder_get_settings(obs_encoder_t encoder)
{
	return encoder->settings.array;
}

void obs_encoder_save_settings(obs_encoder_t encoder, const char *settings)
{
	dstr_copy(&encoder->settings, settings);
}
