/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
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
#include "obs-module.h"

struct obs_subsystem *obs = NULL;

extern char *find_libobs_data_file(const char *file);

static inline void make_gs_init_data(struct gs_init_data *gid,
		struct obs_video_info *ovi)
{
	memcpy(&gid->window, &ovi->window, sizeof(struct gs_window));
	gid->cx              = ovi->base_width;
	gid->cy              = ovi->base_height;
	gid->num_backbuffers = 2;
	gid->format          = GS_RGBA;
	gid->zsformat        = GS_ZS_NONE;
	gid->adapter         = ovi->adapter;
}

static inline void make_video_info(struct video_info *vi,
		struct obs_video_info *ovi)
{
	vi->name    = "video";
	vi->type    = ovi->output_format;
	vi->fps_num = ovi->fps_num;
	vi->fps_den = ovi->fps_den;
	vi->width   = ovi->output_width;
	vi->height  = ovi->output_height;
}

static bool obs_init_graphics(struct obs_video_info *ovi)
{
	struct obs_video *video = &obs->video;
	struct gs_init_data graphics_data;
	bool success = true;
	int errorcode;
	size_t i;

	make_gs_init_data(&graphics_data, ovi);
	video->output_width  = ovi->output_width;
	video->output_height = ovi->output_height;

	errorcode = gs_create(&video->graphics, ovi->graphics_module,
			&graphics_data);
	if (errorcode != GS_SUCCESS) {
		if (errorcode == GS_ERROR_MODULENOTFOUND)
			blog(LOG_ERROR, "Could not find graphics module '%s'",
					ovi->graphics_module);
		return false;
	}

	gs_entercontext(video->graphics);

	for (i = 0; i < NUM_TEXTURES; i++) {
		video->copy_surfaces[i] = gs_create_stagesurface(
				ovi->output_width, ovi->output_height,
				graphics_data.format);

		if (!video->copy_surfaces[i])
			success = false;
	}

	if (success) {
		char *filename = find_libobs_data_file("default.effect");
		video->default_effect = gs_create_effect_from_file(filename,
				NULL);
		bfree(filename);
		if (!video->default_effect)
			success = false;
	}

	gs_leavecontext();
	return success;
}

static bool obs_init_video(struct obs_video_info *ovi)
{
	struct obs_video *video = &obs->video;
	struct video_info vi;
	int errorcode;

	memset(video, 0, sizeof(struct obs_video));

	if (!obs_init_graphics(ovi))
		return false;

	make_video_info(&vi, ovi);
	errorcode = video_output_open(&video->video, obs->media, &vi);

	if (errorcode != VIDEO_OUTPUT_SUCCESS) {
		if (errorcode == VIDEO_OUTPUT_INVALIDPARAM)
			blog(LOG_ERROR, "Invalid video parameters specified");
		else
			blog(LOG_ERROR, "Could not open video output");

		return false;
	}

	errorcode = pthread_create(&video->video_thread, NULL,
			obs_video_thread, obs);
	if (errorcode != 0)
		return false;

	video->thread_initialized = true;
	return true;
}

static void obs_free_video(void)
{
	struct obs_video *video = &obs->video;
	size_t i;

	if (video->video) {
		void *thread_retval;

		video_output_stop(video->video);
		if (video->thread_initialized)
			pthread_join(video->video_thread, &thread_retval);

		video_output_close(video->video);
	}

	if (video->graphics) {
		int cur_texture = video->cur_texture;
		gs_entercontext(video->graphics);

		if (video->copy_mapped)
			stagesurface_unmap(video->copy_surfaces[cur_texture]);

		for (i = 0; i < NUM_TEXTURES; i++)
			stagesurface_destroy(video->copy_surfaces[i]);

		effect_destroy(video->default_effect);

		gs_leavecontext();

		gs_destroy(video->graphics);
	}

	memset(video, 0, sizeof(struct obs_video));
}

static bool obs_init_audio(struct audio_info *ai)
{
	struct obs_audio *audio = &obs->audio;
	int errorcode;

	/* TODO: sound subsystem */

	errorcode = audio_output_open(&audio->audio, obs->media, ai);
	if (errorcode == AUDIO_OUTPUT_SUCCESS)
		return true;
	else if (errorcode == AUDIO_OUTPUT_INVALIDPARAM)
		blog(LOG_ERROR, "Invalid audio parameters specified");
	else
		blog(LOG_ERROR, "Could not open audio output");

	return false;
}

static void obs_free_audio(void)
{
	struct obs_audio *audio = &obs->audio;
	if (audio->audio)
		audio_output_close(audio->audio);

	memset(audio, 0, sizeof(struct obs_audio));
}

static bool obs_init_data(void)
{
	struct obs_data *data = &obs->data;

	pthread_mutex_init_value(&obs->data.displays_mutex);

	if (pthread_mutex_init(&data->sources_mutex, NULL) != 0)
		return false;
	if (pthread_mutex_init(&data->displays_mutex, NULL) != 0)
		return false;

	return true;
}

static void obs_free_data(void)
{
	struct obs_data *data = &obs->data;
	uint32_t i;

	for (i = 0; i < MAX_CHANNELS; i++)
		obs_set_output_source(i, NULL);

	while (data->displays.num)
		obs_display_destroy(data->displays.array[0]);

	pthread_mutex_lock(&obs->data.sources_mutex);
	for (i = 0; i < data->sources.num; i++)
		obs_source_release(data->sources.array[i]);
	da_free(data->sources);
	pthread_mutex_unlock(&obs->data.sources_mutex);
}

static bool obs_init(void)
{
	obs = bmalloc(sizeof(struct obs_subsystem));

	memset(obs, 0, sizeof(struct obs_subsystem));
	obs_init_data();

	obs->media = media_open();
	if (!obs->media)
		return false;

	return true;
}

bool obs_startup()
{
	bool success;

	if (obs) {
		blog(LOG_ERROR, "Tried to call obs_startup more than once");
		return false;
	}

	success = obs_init();
	if (!success)
		obs_shutdown();

	return success;
}

void obs_shutdown(void)
{
	size_t i;

	if (!obs)
		return;

	da_free(obs->input_types);
	da_free(obs->filter_types);
	da_free(obs->transition_types);
	da_free(obs->output_types);
	da_free(obs->service_types);

	obs_free_data();
	obs_free_video();
	obs_free_audio();
	media_close(obs->media);

	for (i = 0; i < obs->modules.num; i++)
		free_module(obs->modules.array+i);
	da_free(obs->modules);

	bfree(obs);
	obs = NULL;
}

bool obs_reset_video(struct obs_video_info *ovi)
{
	obs_free_video();
	if (ovi)
		return obs_init_video(ovi);

	return true;
}

bool obs_reset_audio(struct audio_info *ai)
{
	obs_free_audio();
	if(ai)
		return obs_init_audio(ai);

	return true;
}

bool obs_enum_input_types(size_t idx, const char **name)
{
	if (idx >= obs->input_types.num)
		return false;
	*name = obs->input_types.array[idx].name;
	return true;
}

bool obs_enum_filter_types(size_t idx, const char **name)
{
	if (idx >= obs->filter_types.num)
		return false;
	*name = obs->filter_types.array[idx].name;
	return true;
}

bool obs_enum_transition_types(size_t idx, const char **name)
{
	if (idx >= obs->transition_types.num)
		return false;
	*name = obs->transition_types.array[idx].name;
	return true;
}

bool obs_enum_output_types(size_t idx, const char **name)
{
	if (idx >= obs->output_types.num)
		return false;
	*name = obs->output_types.array[idx].name;
	return true;
}

graphics_t obs_graphics(void)
{
	return obs->video.graphics;
}

media_t obs_media(void)
{
	return obs->media;
}

bool obs_add_source(obs_source_t source)
{
	pthread_mutex_lock(&obs->data.sources_mutex);
	da_push_back(obs->data.sources, &source);
	obs_source_addref(source);
	pthread_mutex_unlock(&obs->data.sources_mutex);

	return true;
}

obs_source_t obs_get_output_source(uint32_t channel)
{
	struct obs_source *source;
	assert(channel < MAX_CHANNELS);
	source = obs->data.channels[channel];

	obs_source_addref(source);
	return source;
}

void obs_set_output_source(uint32_t channel, obs_source_t source)
{
	struct obs_source *prev_source;
	assert(channel < MAX_CHANNELS);

	prev_source = obs->data.channels[channel];
	obs->data.channels[channel] = source;

	if (source)
		obs_source_addref(source);
	if (prev_source)
		obs_source_release(prev_source);
}

void obs_enum_sources(ENUM_SOURCES_PROC enum_proc, void *param)
{
	struct obs_data *data = &obs->data;
	size_t i;

	pthread_mutex_lock(&data->sources_mutex);

	for (i = 0; i < data->sources.num; i++)
		enum_proc(data->sources.array[i], param);

	pthread_mutex_unlock(&data->sources_mutex);
}
