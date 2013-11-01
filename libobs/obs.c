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

struct obs_data *obs = NULL;

extern char *find_libobs_data_file(const char *file);

static bool obs_init_graphics(const char *graphics_module,
		struct gs_init_data *graphics_data, struct video_info *vi)
{
	int errorcode;
	bool success = true;
	size_t i;

	errorcode = gs_create(&obs->graphics, graphics_module, graphics_data);
	if (errorcode != GS_SUCCESS) {
		if (errorcode == GS_ERROR_MODULENOTFOUND)
			blog(LOG_ERROR, "Could not find graphics module '%s'",
					graphics_module);
		return false;
	}

	gs_entercontext(obs->graphics);

	for (i = 0; i < NUM_TEXTURES; i++) {
		obs->copy_surfaces[i] = gs_create_stagesurface(vi->width,
				vi->height, graphics_data->format);
		if (!obs->copy_surfaces[i])
			success = false;
	}

	if (success) {
		char *filename = find_libobs_data_file("default.effect");
		obs->default_effect = gs_create_effect_from_file( filename,
				NULL);
		bfree(filename);
		if (!obs->default_effect)
			success = false;
	}

	gs_leavecontext();
	return success;
}

static bool obs_init_media(struct video_info *vi, struct audio_info *ai)
{
	obs->media = media_open();
	if (!obs->media)
		return false;

	if (!obs_reset_video(vi))
		return false;
	if (!obs_reset_audio(ai))
		return false;

	return true;
}

static bool obs_init_threading(void)
{
	if (pthread_mutex_init(&obs->source_list_mutex, NULL) != 0)
		return false;
	if (pthread_mutex_init(&obs->display_list_mutex, NULL) != 0)
		return false;
	if (pthread_create(&obs->video_thread, NULL, obs_video_thread,
				obs) != 0)
		return false;

	obs->thread_initialized = true;
	return true;
}

static bool obs_init(const char *graphics_module,
		struct gs_init_data *graphics_data,
		struct video_info *vi, struct audio_info *ai)
{
	obs = bmalloc(sizeof(struct obs_data));

	memset(obs, 0, sizeof(struct obs_data));
	pthread_mutex_init_value(&obs->source_list_mutex);
	pthread_mutex_init_value(&obs->display_list_mutex);

	if (!obs_init_graphics(graphics_module, graphics_data, vi))
		return false;
	if (!obs_init_media(vi, ai))
		return false;
	if (!obs_init_threading())
		return false;

	return true;
}

static inline void obs_free_graphics(void)
{
	size_t i;
	if (!obs->graphics)
		return;

	gs_entercontext(obs->graphics);

	if (obs->copy_mapped)
		stagesurface_unmap(obs->copy_surfaces[obs->cur_texture]);

	for (i = 0; i < NUM_TEXTURES; i++)
		stagesurface_destroy(obs->copy_surfaces[i]);

	effect_destroy(obs->default_effect);

	gs_leavecontext();

	gs_destroy(obs->graphics);
}

static inline void obs_free_media(void)
{
	video_output_close(obs->video);
	audio_output_close(obs->audio);
	media_close(obs->media);
}

static inline void obs_free_threading(void)
{
	void *thread_ret;
	video_output_stop(obs->video);
	if (obs->thread_initialized)
		pthread_join(obs->video_thread, &thread_ret);
	pthread_mutex_destroy(&obs->source_list_mutex);
	pthread_mutex_destroy(&obs->display_list_mutex);
}

static void obs_destroy(void)
{
	size_t i;

	if (!obs)
		return;

	for (i = 0; i < obs->displays.num; i++)
		obs_display_destroy(obs->displays.array[i]);

	da_free(obs->input_types);
	da_free(obs->filter_types);
	da_free(obs->transition_types);
	da_free(obs->output_types);
	/*da_free(obs->services);*/

	da_free(obs->displays);
	da_free(obs->sources);

	obs_free_threading();
	obs_free_media();
	obs_free_graphics();

	for (i = 0; i < obs->modules.num; i++)
		free_module(obs->modules.array+i);
	da_free(obs->modules);

	bfree(obs);
	obs = NULL;
}

bool obs_startup(const char *graphics_module,
		struct gs_init_data *graphics_data,
		struct video_info *vi, struct audio_info *ai)
{
	if (!obs_init(graphics_module, graphics_data, vi, ai)) {
		obs_destroy();
		return false;
	}

	return true;
}

void obs_shutdown(void)
{
	obs_destroy();
}

bool obs_reset_video(struct video_info *vi)
{
	int errorcode;

	if (obs->video) {
		video_output_close(obs->video);
		obs->video = NULL;
	}

	obs->output_width  = vi->width;
	obs->output_height = vi->height;

	errorcode = video_output_open(&obs->video, obs->media, vi);
	if (errorcode == VIDEO_OUTPUT_SUCCESS)
		return true;
	else if (errorcode == VIDEO_OUTPUT_INVALIDPARAM)
		blog(LOG_ERROR, "Invalid video parameters specified");
	else
		blog(LOG_ERROR, "Could not open video output");

	return false;
}

bool obs_reset_audio(struct audio_info *ai)
{
	/* TODO */
	return true;
}


bool obs_enum_inputs(size_t idx, const char **name)
{
	if (idx >= obs->input_types.num)
		return false;
	*name = obs->input_types.array[idx].name;
	return true;
}

bool obs_enum_filters(size_t idx, const char **name)
{
	if (idx >= obs->filter_types.num)
		return false;
	*name = obs->filter_types.array[idx].name;
	return true;
}

bool obs_enum_transitions(size_t idx, const char **name)
{
	if (idx >= obs->transition_types.num)
		return false;
	*name = obs->transition_types.array[idx].name;
	return true;
}

bool obs_enum_outputs(size_t idx, const char **name)
{
	if (idx >= obs->output_types.num)
		return false;
	*name = obs->output_types.array[idx].name;
	return true;
}


graphics_t obs_graphics(void)
{
	return obs->graphics;
}

media_t obs_media(void)
{
	return obs->media;
}

obs_source_t obs_get_primary_source(void)
{
	return obs->primary_source;
}

void obs_set_primary_source(obs_source_t source)
{
	obs->primary_source = source;
}
