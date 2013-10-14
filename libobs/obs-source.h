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

#pragma once

#include "util/c99defs.h"
#include "util/darray.h"
#include "util/dstr.h"
#include "media-io/media-io.h"

/*
 * ===========================================
 *   Sources
 * ===========================================
 *
 *   A source is literally a "source" of audio and/or video.
 *
 *   A module with sources needs to export these functions:
 *       + enum_[type]
 *
 *   Each individual source is then exported by it's name.  For example, a
 * source named "mysource" would have the following exports:
 *       + mysource_create
 *       + mysource_destroy
 *       + mysource_getflags
 *
 *       [and optionally]
 *       + mysource_update
 *       + mysource_config
 *       + mysource_video_tick
 *       + mysource_video_render
 *       + mysource_getparam
 *       + mysource_setparam
 *
 * ===========================================
 *   Primary Exports
 * ===========================================
 *   const bool enum_[type](size_t idx, const char **name);
 *       idx: index of the source.
 *       type: pointer to variable that receives the type of the source
 *       Return value: false when no more available.
 *
 * ===========================================
 *   Source Exports
 * ===========================================
 *   void *[name]_create(const char *settings, obs_source_t source);
 *       Creates a source.
 *
 *       settings: Settings of the source.
 *       source: pointer to main source
 *       Return value: Internal source pointer, or NULL if failed.
 *
 * ---------------------------------------------------------
 *   void [name]_destroy(void *data);
 *       Destroys the source.
 *
 * ---------------------------------------------------------
 *   uint32_t [name]_getflags(void *data);
 *       Returns a combination of one of the following values:
 *           + SOURCE_VIDEO: source has video
 *           + SOURCE_AUDIO: source has audio
 *           + SOURCE_ASYNC: video is sent asynchronously via RAM
 *
 * ---------------------------------------------------------
 *   int [name]_getwidth(void *data);
 *       Returns the width of a source, or -1 for maximum width.  If you render
 *       video, this is required.
 *
 * ---------------------------------------------------------
 *   int [name]_getheight(void *data);
 *       Returns the height of a source, or -1 for maximum height.  If you
 *       render video, this is required.
 *
 * ===========================================
 *   Optional Source Exports
 * ===========================================
 *   void [name]_config(void *data, void *parent);
 *       Called to configure the source.
 *
 *       parent: Parent window pointer
 *
 * ---------------------------------------------------------
 *   void [name]_video_activate(void *data);
 *       Called when the source is being displayed.
 *
 * ---------------------------------------------------------
 *   void [name]_video_deactivate(void *data);
 *       Called when the source is no longer being displayed.
 *
 * ---------------------------------------------------------
 *   void [name]_video_tick(void *data, float seconds);
 *       Called each video frame with the time taken between the last and
 *       current frame, in seconds.
 *
 * ---------------------------------------------------------
 *   void [name]_video_render(void *data);
 *       Called to render the source.
 *
 * ---------------------------------------------------------
 *   void [name]_getparam(void *data, const char *param, void *buf,
 *                        size_t buf_size);
 *       Gets a source parameter value.  
 *
 *       param: Name of parameter.
 *       Return value: Value of parameter.
 *
 * ---------------------------------------------------------
 *   void [name]_setparam(void *data, const char *param, const void *buf,
 *                        size_t size);
 *       Sets a source parameter value.
 *
 *       param: Name of parameter.
 *       val: Value of parameter to set.
 *
 * ---------------------------------------------------------
 *   bool [name]_enum_children(void *data, size_t idx, obs_source_t *child);
 *       Enumerates child sources, if any.
 *
 *       idx: Child source index.
 *       child: Pointer to variable that receives child source. 
 *       Return value: true if sources remaining, otherwise false.
 *
 * ---------------------------------------------------------
 *   void [name]_filter_video(void *data, struct video_frame *frame);
 *       Filters audio data.  Used with audio filters.
 *
 *       frame: Video frame data.
 *
 * ---------------------------------------------------------
 *   void [name]_filter_audio(void *data, struct audio_data *audio);
 *       Filters video data.  Used with async video data.
 *
 *       audio: Audio data.
 */

struct obs_source;

struct source_info {
	const char *name;

	/* ----------------------------------------------------------------- */
	/* required implementations */

	void *(*create)(const char *settings, obs_source_t source);
	void (*destroy)(void *data);

	uint32_t (*get_output_flags)(void *data);

	/* ----------------------------------------------------------------- */
	/* optional implementations */

	bool (*config)(void *data, void *parent);

	void (*activate)(void *data);
	void (*deactivate)(void *data);

	void (*video_tick)(void *data, float seconds);
	void (*video_render)(void *data);
	int (*getwidth)(void *data);
	int (*getheight)(void *data);

	size_t (*getparam)(void *data, const char *param, void *data_out,
			size_t buf_size);
	void (*setparam)(void *data, const char *param, const void *data_in,
			size_t size);

	bool (*enum_children)(void *data, size_t idx, obs_source_t *child);

	void (*filter_video)(void *data, struct video_frame *frame);
	void (*filter_audio)(void *data, struct audio_data *audio);
};

struct obs_source {
	void                       *data;
	struct source_info         callbacks;
	struct dstr                settings;
	bool                       rendering_filter;

	struct obs_source          *filter_target;
	DARRAY(struct obs_source*) filters;
};

extern bool get_source_info(void *module, const char *module_name,
		const char *source_name, struct source_info *info);

extern void obs_source_init(struct obs_source *source);

extern void obs_source_activate(obs_source_t source);
extern void obs_source_deactivate(obs_source_t source);
extern void obs_source_video_tick(obs_source_t source, float seconds);
