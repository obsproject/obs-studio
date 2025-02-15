/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

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

bool obs_view_init(struct obs_view *view)
{
	if (!view)
		return false;

	pthread_mutex_init_value(&view->channels_mutex);

	if (pthread_mutex_init(&view->channels_mutex, NULL) != 0) {
		blog(LOG_ERROR, "obs_view_init: Failed to create mutex");
		return false;
	}

	return true;
}

obs_view_t *obs_view_create(void)
{
	struct obs_view *view = bzalloc(sizeof(struct obs_view));

	if (!obs_view_init(view)) {
		bfree(view);
		view = NULL;
	}

	return view;
}

void obs_view_free(struct obs_view *view)
{
	if (!view)
		return;

	for (size_t i = 0; i < MAX_CHANNELS; i++) {
		struct obs_source *source = view->channels[i];
		if (source) {
			obs_source_deactivate(source, AUX_VIEW);
			obs_source_release(source);
		}
	}

	memset(view->channels, 0, sizeof(view->channels));
	pthread_mutex_destroy(&view->channels_mutex);
}

void obs_view_destroy(obs_view_t *view)
{
	if (view) {
		obs_view_free(view);
		bfree(view);
	}
}

obs_source_t *obs_view_get_source(obs_view_t *view, uint32_t channel)
{
	obs_source_t *source;
	assert(channel < MAX_CHANNELS);

	if (!view)
		return NULL;
	if (channel >= MAX_CHANNELS)
		return NULL;

	pthread_mutex_lock(&view->channels_mutex);
	source = obs_source_get_ref(view->channels[channel]);
	pthread_mutex_unlock(&view->channels_mutex);

	return source;
}

void obs_view_set_source(obs_view_t *view, uint32_t channel, obs_source_t *source)
{
	struct obs_source *prev_source;

	assert(channel < MAX_CHANNELS);

	if (!view)
		return;
	if (channel >= MAX_CHANNELS)
		return;

	pthread_mutex_lock(&view->channels_mutex);
	source = obs_source_get_ref(source);
	prev_source = view->channels[channel];
	view->channels[channel] = source;

	pthread_mutex_unlock(&view->channels_mutex);

	if (source)
		obs_source_activate(source, AUX_VIEW);

	if (prev_source) {
		obs_source_deactivate(prev_source, AUX_VIEW);
		obs_source_release(prev_source);
	}
}

void obs_view_render(obs_view_t *view)
{
	if (!view)
		return;

	pthread_mutex_lock(&view->channels_mutex);

	for (size_t i = 0; i < MAX_CHANNELS; i++) {
		struct obs_source *source;

		source = view->channels[i];

		if (source) {
			if (source->removed) {
				obs_source_release(source);
				view->channels[i] = NULL;
			} else {
				obs_source_video_render(source);
			}
		}
	}

	pthread_mutex_unlock(&view->channels_mutex);
}

static inline size_t find_mix_for_view(obs_view_t *view)
{
	for (size_t i = 0, num = obs->video.mixes.num; i < num; i++) {
		if (obs->video.mixes.array[i]->view == view)
			return i;
	}

	return DARRAY_INVALID;
}

static inline void set_main_mix()
{
	size_t idx = find_mix_for_view(&obs->data.main_view);

	struct obs_core_video_mix *mix = NULL;
	if (idx != DARRAY_INVALID)
		mix = obs->video.mixes.array[idx];
	obs->video.main_mix = mix;
}

video_t *obs_view_add(obs_view_t *view)
{
	if (!obs->video.main_mix)
		return NULL;
	return obs_view_add2(view, &obs->video.main_mix->ovi);
}

video_t *obs_view_add2(obs_view_t *view, struct obs_video_info *ovi)
{
	if (!view || !ovi)
		return NULL;

	struct obs_core_video_mix *mix = obs_create_video_mix(ovi);
	if (!mix) {
		return NULL;
	}
	mix->view = view;

	pthread_mutex_lock(&obs->video.mixes_mutex);
	da_push_back(obs->video.mixes, &mix);
	set_main_mix();
	pthread_mutex_unlock(&obs->video.mixes_mutex);

	return mix->video;
}

void obs_view_remove(obs_view_t *view)
{
	if (!view)
		return;

	pthread_mutex_lock(&obs->video.mixes_mutex);
	for (size_t i = 0, num = obs->video.mixes.num; i < num; i++) {
		if (obs->video.mixes.array[i]->view == view)
			obs->video.mixes.array[i]->view = NULL;
	}
	set_main_mix();
	pthread_mutex_unlock(&obs->video.mixes_mutex);
}

bool obs_view_get_video_info(obs_view_t *view, struct obs_video_info *ovi)
{
	if (!view)
		return false;

	pthread_mutex_lock(&obs->video.mixes_mutex);

	size_t idx = find_mix_for_view(view);
	if (idx != DARRAY_INVALID) {
		*ovi = obs->video.mixes.array[idx]->ovi;
		pthread_mutex_unlock(&obs->video.mixes_mutex);
		return true;
	}

	pthread_mutex_unlock(&obs->video.mixes_mutex);

	return false;
}

void obs_view_enum_video_info(obs_view_t *view, bool (*enum_proc)(void *, struct obs_video_info *), void *param)
{
	pthread_mutex_lock(&obs->video.mixes_mutex);

	for (size_t i = 0, num = obs->video.mixes.num; i < num; i++) {
		struct obs_core_video_mix *mix = obs->video.mixes.array[i];
		if (mix->view != view)
			continue;
		if (!enum_proc(param, &mix->ovi))
			break;
	}

	pthread_mutex_unlock(&obs->video.mixes_mutex);
}
