/******************************************************************************
    Copyright (C) 2014 by Hugh Bailey <obs.jim@gmail.com>

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

bool obs_viewport_init(struct obs_viewport *viewport)
{
	pthread_mutex_init_value(&viewport->channels_mutex);

	if (pthread_mutex_init(&viewport->channels_mutex, NULL) != 0) {
		blog(LOG_ERROR, "obs_viewport_init: Failed to create mutex");
		return false;
	}

	return true;
}

obs_viewport_t obs_viewport_create(void)
{
	struct obs_viewport *viewport = bzalloc(sizeof(struct obs_viewport));

	if (!obs_viewport_init(viewport)) {
		bfree(viewport);
		viewport = NULL;
	}

	return viewport;
}

void obs_viewport_free(struct obs_viewport *viewport)
{
	for (size_t i = 0; i < MAX_CHANNELS; i++)
		obs_source_release(viewport->channels[i]);

	memset(viewport->channels, 0, sizeof(viewport->channels));
	pthread_mutex_destroy(&viewport->channels_mutex);
}

void obs_viewport_destroy(obs_viewport_t viewport)
{
	if (viewport) {
		obs_viewport_free(viewport);
		bfree(viewport);
	}
}

obs_source_t obs_viewport_getsource(obs_viewport_t viewport, uint32_t channel)
{
	obs_source_t source;
	assert(channel < MAX_CHANNELS);

	if (!viewport) return NULL;
	if (channel >= MAX_CHANNELS) return NULL;

	pthread_mutex_lock(&viewport->channels_mutex);

	source = viewport->channels[channel];
	if (source)
		obs_source_addref(source);

	pthread_mutex_unlock(&viewport->channels_mutex);

	return source;
}

void obs_viewport_setsource(obs_viewport_t viewport, uint32_t channel,
		obs_source_t source)
{
	struct obs_source *prev_source;

	assert(channel < MAX_CHANNELS);

	if (!viewport) return;
	if (channel >= MAX_CHANNELS) return;

	pthread_mutex_lock(&viewport->channels_mutex);

	prev_source = viewport->channels[channel];
	viewport->channels[channel] = source;

	if (source)
		obs_source_addref(source);
	if (prev_source)
		obs_source_release(prev_source);

	pthread_mutex_unlock(&viewport->channels_mutex);
}

void obs_viewport_render(obs_viewport_t viewport)
{
	if (!viewport) return;

	pthread_mutex_lock(&viewport->channels_mutex);

	for (size_t i = 0; i < MAX_CHANNELS; i++) {
		struct obs_source *source;

		source = viewport->channels[i];

		if (source) {
			if (source->removed) {
				obs_source_release(source);
				viewport->channels[i] = NULL;
			} else {
				obs_source_video_render(source);
			}
		}
	}

	pthread_mutex_unlock(&viewport->channels_mutex);
}
