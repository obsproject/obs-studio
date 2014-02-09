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

#include "callback/calldata.h"

#include "obs.h"
#include "obs-internal.h"
#include "obs-module.h"

struct obs_core *obs = NULL;

extern char *find_libobs_data_file(const char *file);

static inline void make_gs_init_data(struct gs_init_data *gid,
		struct obs_video_info *ovi)
{
	memcpy(&gid->window, &ovi->window, sizeof(struct gs_window));
	gid->cx              = ovi->window_width;
	gid->cy              = ovi->window_height;
	gid->num_backbuffers = 2;
	gid->format          = GS_RGBA;
	gid->zsformat        = GS_ZS_NONE;
	gid->adapter         = ovi->adapter;
}

static inline void make_video_info(struct video_output_info *vi,
		struct obs_video_info *ovi)
{
	vi->name    = "video";
	vi->format  = ovi->output_format;
	vi->fps_num = ovi->fps_num;
	vi->fps_den = ovi->fps_den;
	vi->width   = ovi->output_width;
	vi->height  = ovi->output_height;
}

static bool obs_init_textures(struct obs_video_info *ovi)
{
	struct obs_core_video *video = &obs->video;
	bool yuv = format_is_yuv(ovi->output_format);
	size_t i;

	for (i = 0; i < NUM_TEXTURES; i++) {
		video->copy_surfaces[i] = gs_create_stagesurface(
				ovi->output_width, ovi->output_height,
				GS_RGBA);

		if (!video->copy_surfaces[i])
			return false;

		video->render_textures[i] = gs_create_texture(
				ovi->base_width, ovi->base_height,
				GS_RGBA, 1, NULL, GS_RENDERTARGET);

		if (!video->render_textures[i])
			return false;

		video->output_textures[i] = gs_create_texture(
				ovi->output_width, ovi->output_height,
				GS_RGBA, 1, NULL, GS_RENDERTARGET);

		if (!video->output_textures[i])
			return false;

		if (yuv)
			source_frame_init(&video->convert_frames[i],
					ovi->output_format,
					ovi->output_width, ovi->output_height);
	}

	return true;
}

static bool obs_init_graphics(struct obs_video_info *ovi)
{
	struct obs_core_video *video = &obs->video;
	struct gs_init_data graphics_data;
	bool success = true;
	int errorcode;

	make_gs_init_data(&graphics_data, ovi);
	video->base_width  = ovi->base_width;
	video->base_height = ovi->base_height;

	errorcode = gs_create(&video->graphics, ovi->graphics_module,
			&graphics_data);
	if (errorcode != GS_SUCCESS) {
		if (errorcode == GS_ERROR_MODULENOTFOUND)
			blog(LOG_ERROR, "Could not find graphics module '%s'",
					ovi->graphics_module);
		return false;
	}

	gs_entercontext(video->graphics);

	if (!obs_init_textures(ovi))
		success = false;

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
	struct obs_core_video *video = &obs->video;
	struct video_output_info vi;
	int errorcode;

	make_video_info(&vi, ovi);
	errorcode = video_output_open(&video->video, &vi);

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

static void obs_free_video()
{
	struct obs_core_video *video = &obs->video;

	if (video->video) {
		void *thread_retval;

		video_output_stop(video->video);
		if (video->thread_initialized) {
			pthread_join(video->video_thread, &thread_retval);
			video->thread_initialized = false;
		}

		video_output_close(video->video);
		video->video = NULL;
	}
}

static void obs_free_graphics()
{
	struct obs_core_video *video = &obs->video;
	size_t i;

	if (video->graphics) {
		int cur_texture = video->cur_texture;
		gs_entercontext(video->graphics);

		if (video->mapped_surface)
			stagesurface_unmap(video->mapped_surface);

		for (i = 0; i < NUM_TEXTURES; i++) {
			stagesurface_destroy(video->copy_surfaces[i]);
			texture_destroy(video->render_textures[i]);
			texture_destroy(video->output_textures[i]);
			source_frame_free(&video->convert_frames[i]);

			video->copy_surfaces[i]   = NULL;
			video->render_textures[i] = NULL;
			video->output_textures[i] = NULL;
		}

		effect_destroy(video->default_effect);
		video->default_effect = NULL;

		gs_leavecontext();

		gs_destroy(video->graphics);
		video->graphics = NULL;
		video->cur_texture = 0;
	}
}

static bool obs_init_audio(struct audio_output_info *ai)
{
	struct obs_core_audio *audio = &obs->audio;
	int errorcode;

	/* TODO: sound subsystem */

	errorcode = audio_output_open(&audio->audio, ai);
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
	struct obs_core_audio *audio = &obs->audio;
	if (audio->audio)
		audio_output_close(audio->audio);

	memset(audio, 0, sizeof(struct obs_core_audio));
}

static bool obs_init_data(void)
{
	struct obs_core_data *data = &obs->data;
	pthread_mutexattr_t attr;
	bool success = false;

	pthread_mutex_init_value(&obs->data.displays_mutex);

	if (pthread_mutexattr_init(&attr) != 0)
		return false;
	if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) != 0)
		goto fail;
	if (pthread_mutex_init(&data->sources_mutex, &attr) != 0)
		goto fail;
	if (pthread_mutex_init(&data->displays_mutex, &attr) != 0)
		goto fail;
	if (pthread_mutex_init(&data->outputs_mutex, &attr) != 0)
		goto fail;
	if (pthread_mutex_init(&data->encoders_mutex, &attr) != 0)
		goto fail;

	data->valid = true;

fail:
	pthread_mutexattr_destroy(&attr);
	return data->valid;
}

static void obs_free_data(void)
{
	struct obs_core_data *data = &obs->data;
	uint32_t i;

	data->valid = false;

	for (i = 0; i < MAX_CHANNELS; i++)
		obs_set_output_source(i, NULL);

	while (data->outputs.num)
		obs_output_destroy(data->outputs.array[0]);
	while (data->encoders.num)
		obs_encoder_destroy(data->encoders.array[0]);
	while (data->displays.num)
		obs_display_destroy(data->displays.array[0]);

	pthread_mutex_lock(&obs->data.sources_mutex);
	for (i = 0; i < data->sources.num; i++)
		obs_source_release(data->sources.array[i]);
	da_free(data->sources);
	pthread_mutex_unlock(&obs->data.sources_mutex);

	pthread_mutex_destroy(&data->sources_mutex);
	pthread_mutex_destroy(&data->displays_mutex);
	pthread_mutex_destroy(&data->outputs_mutex);
	pthread_mutex_destroy(&data->encoders_mutex);
}

static inline bool obs_init_handlers(void)
{
	obs->signals = signal_handler_create();
	if (!obs->signals)
		return false;

	obs->procs   = proc_handler_create();
	return (obs->procs != NULL);
}

static bool obs_init(void)
{
	obs = bmalloc(sizeof(struct obs_core));
	memset(obs, 0, sizeof(struct obs_core));

	obs_init_data();
	return obs_init_handlers();
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
	da_free(obs->ui_modal_callbacks);
	da_free(obs->ui_modeless_callbacks);

	obs_free_data();
	obs_free_video();
	obs_free_graphics();
	obs_free_audio();
	proc_handler_destroy(obs->procs);
	signal_handler_destroy(obs->signals);

	for (i = 0; i < obs->modules.num; i++)
		free_module(obs->modules.array+i);
	da_free(obs->modules);

	bfree(obs);
	obs = NULL;
}

bool obs_reset_video(struct obs_video_info *ovi)
{
	struct obs_core_video *video = &obs->video;

	obs_free_video();

	if (!ovi) {
		obs_free_graphics();
		return true;
	}

	if (!video->graphics && !obs_init_graphics(ovi))
		return false;

	return obs_init_video(ovi);
}

bool obs_reset_audio(struct audio_output_info *ai)
{
	obs_free_audio();
	if(!ai)
		return true;

	return obs_init_audio(ai);
}

bool obs_get_video_info(struct obs_video_info *ovi)
{
	struct obs_core_video *video = &obs->video;
	const struct video_output_info *info;

	if (!obs || !video->graphics)
		return false;

	info = video_output_getinfo(video->video);

	memset(ovi, 0, sizeof(struct obs_video_info));
	ovi->base_width    = video->base_width;
	ovi->base_height   = video->base_height;
	ovi->output_width  = info->width;
	ovi->output_height = info->height;
	ovi->output_format = info->format;
	ovi->fps_num       = info->fps_num;
	ovi->fps_den       = info->fps_den;

	return true;
}

bool obs_get_audio_info(struct audio_output_info *aoi)
{
	struct obs_core_audio *audio = &obs->audio;
	const struct audio_output_info *info;

	if (!obs || !audio->audio)
		return false;

	info = audio_output_getinfo(audio->audio);
	memcpy(aoi, info, sizeof(struct audio_output_info));

	return true;
}

bool obs_enum_input_types(size_t idx, const char **id)
{
	if (idx >= obs->input_types.num)
		return false;
	*id = obs->input_types.array[idx].id;
	return true;
}

bool obs_enum_filter_types(size_t idx, const char **id)
{
	if (idx >= obs->filter_types.num)
		return false;
	*id = obs->filter_types.array[idx].id;
	return true;
}

bool obs_enum_transition_types(size_t idx, const char **id)
{
	if (idx >= obs->transition_types.num)
		return false;
	*id = obs->transition_types.array[idx].id;
	return true;
}

bool obs_enum_output_types(size_t idx, const char **id)
{
	if (idx >= obs->output_types.num)
		return false;
	*id = obs->output_types.array[idx].id;
	return true;
}

graphics_t obs_graphics(void)
{
	return (obs != NULL) ? obs->video.graphics : NULL;
}

audio_t obs_audio(void)
{
	return (obs != NULL) ? obs->audio.audio : NULL;
}

video_t obs_video(void)
{
	return (obs != NULL) ? obs->video.video : NULL;
}

/* TODO: optimize this later so it's not just O(N) string lookups */
static inline struct obs_modal_ui *get_modal_ui_callback(const char *name,
		const char *task, const char *target)
{
	for (size_t i = 0; i < obs->ui_modal_callbacks.num; i++) {
		struct obs_modal_ui *callback = obs->ui_modal_callbacks.array+i;

		if (strcmp(callback->name,   name)   == 0 &&
		    strcmp(callback->task,   task)   == 0 &&
		    strcmp(callback->target, target) == 0)
			return callback;
	}

	return NULL;
}

static inline struct obs_modeless_ui *get_modeless_ui_callback(const char *name,
		const char *task, const char *target)
{
	for (size_t i = 0; i < obs->ui_modeless_callbacks.num; i++) {
		struct obs_modeless_ui *callback;
		callback = obs->ui_modeless_callbacks.array+i;

		if (strcmp(callback->name,   name)   == 0 &&
		    strcmp(callback->task,   task)   == 0 &&
		    strcmp(callback->target, target) == 0)
			return callback;
	}

	return NULL;
}

int obs_exec_ui(const char *name, const char *task, const char *target,
		void *data, void *ui_data)
{
	struct obs_modal_ui *callback;
	int errorcode = OBS_UI_NOTFOUND;

	callback = get_modal_ui_callback(name, task, target);
	if (callback) {
		bool success = callback->callback(data, ui_data);
		errorcode = success ? OBS_UI_SUCCESS : OBS_UI_CANCEL;
	}

	return errorcode;
}

void *obs_create_ui(const char *name, const char *task, const char *target,
		void *data, void *ui_data)
{
	struct obs_modeless_ui *callback;

	callback = get_modeless_ui_callback(name, task, target);
	return callback ? callback->callback(data, ui_data) : NULL;
}

bool obs_add_source(obs_source_t source)
{
	struct calldata params = {0};

	pthread_mutex_lock(&obs->data.sources_mutex);
	da_push_back(obs->data.sources, &source);
	obs_source_addref(source);
	pthread_mutex_unlock(&obs->data.sources_mutex);

	calldata_setptr(&params, "source", source);
	signal_handler_signal(obs->signals, "source-add", &params);
	calldata_free(&params);

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
	struct calldata params = {0};
	assert(channel < MAX_CHANNELS);

	prev_source = obs->data.channels[channel];

	calldata_setuint32(&params, "channel", channel);
	calldata_setptr(&params, "prev_source", prev_source);
	calldata_setptr(&params, "source", source);
	signal_handler_signal(obs->signals, "channel-change", &params);
	calldata_getptr(&params, "source", &source);
	calldata_free(&params);

	obs->data.channels[channel] = source;

	if (source != prev_source) {
		if (source)
			obs_source_addref(source);
		if (prev_source)
			obs_source_release(prev_source);
	}
}

void obs_enum_outputs(bool (*enum_proc)(void*, obs_output_t), void *param)
{
	struct obs_core_data *data = &obs->data;

	pthread_mutex_lock(&data->outputs_mutex);

	for (size_t i = 0; i < data->outputs.num; i++)
		if (!enum_proc(param, data->outputs.array[i]))
			break;

	pthread_mutex_unlock(&data->outputs_mutex);
}

void obs_enum_encoders(bool (*enum_proc)(void*, obs_encoder_t), void *param)
{
	struct obs_core_data *data = &obs->data;

	pthread_mutex_lock(&data->encoders_mutex);

	for (size_t i = 0; i < data->encoders.num; i++)
		if (!enum_proc(param, data->encoders.array[i]))
			break;

	pthread_mutex_unlock(&data->encoders_mutex);
}

void obs_enum_sources(bool (*enum_proc)(void*, obs_source_t), void *param)
{
	struct obs_core_data *data = &obs->data;

	pthread_mutex_lock(&data->sources_mutex);

	for (size_t i = 0; i < data->sources.num; i++)
		if (!enum_proc(param, data->sources.array[i]))
			break;

	pthread_mutex_unlock(&data->sources_mutex);
}

obs_source_t obs_get_source_by_name(const char *name)
{
	struct obs_core_data *data = &obs->data;
	struct obs_source *source = NULL;
	size_t i;

	pthread_mutex_lock(&data->sources_mutex);

	for (i = 0; i < data->sources.num; i++) {
		struct obs_source *cur_source = data->sources.array[i];
		if (strcmp(cur_source->name, name) == 0) {
			source = cur_source;
			obs_source_addref(source);
			break;
		}
	}

	pthread_mutex_unlock(&data->sources_mutex);
	return source;
}

effect_t obs_get_default_effect(void)
{
	return obs->video.default_effect;
}

signal_handler_t obs_signalhandler(void)
{
	return obs->signals;
}

proc_handler_t obs_prochandler(void)
{
	return obs->procs;
}
