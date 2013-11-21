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
#include "util/threading.h"
#include "media-io/media-io.h"
#include "media-io/audio-resampler.h"

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
 *       + mysource_getname
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
 *   const char *[name]_getname(const char *locale);
 *       Returns the full name of the source type (seen by the user).
 *
 * ---------------------------------------------------------
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
 *   uint32_t [name]_getwidth(void *data);
 *       Returns the width of a source, or -1 for maximum width.  If you render
 *       video, this is required.
 *
 * ---------------------------------------------------------
 *   uint32_t [name]_getheight(void *data);
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
 *   struct source_frame *[name]_filter_video(void *data,
 *                                     const struct source_frame *frame);
 *       Filters audio data.  Used with audio filters.
 *
 *       frame: Video frame data.
 *       returns: New video frame data (or NULL if pending)
 *
 * ---------------------------------------------------------
 *   struct filter_audio [name]_filter_audio(void *data,
 *                                     struct filter_audio *audio);
 *       Filters video data.  Used with async video data.
 *
 *       audio: Audio data.
 *       reutrns New audio data (or NULL if pending)
 */

struct obs_source;

struct source_info {
	const char *name;

	/* ----------------------------------------------------------------- */
	/* required implementations */

	const char *(*getname)(const char *locale);

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
	uint32_t (*getwidth)(void *data);
	uint32_t (*getheight)(void *data);

	size_t (*getparam)(void *data, const char *param, void *data_out,
			size_t buf_size);
	void (*setparam)(void *data, const char *param, const void *data_in,
			size_t size);

	struct source_frame *(*filter_video)(void *data,
			const struct source_frame *frame);
	struct filtered_audio *(*filter_audio)(void *data,
			struct filtered_audio *audio);
};

struct audiobuf {
	void     *data;
	uint32_t frames;
	uint64_t timestamp;
};

static inline void audiobuf_free(struct audiobuf *buf)
{
	bfree(buf->data);
}

struct obs_source {
	volatile int                 refs;

	/* source-specific data */
	void                         *data;
	struct source_info           callbacks;
	struct dstr                  settings;

	/* used to indicate that the source has been removed and all
	 * references to it should be released (not exactly how I would prefer
	 * to handle things but it's the best option) */
	bool                         removed;

	/* async video and audio */
	bool                         timing_set;
	uint64_t                     timing_adjust;
	uint64_t                     last_frame_timestamp;
	uint64_t                     last_sys_timestamp;
	texture_t                    output_texture;

	bool                         audio_failed;
	struct resample_info         sample_info;
	audio_resampler_t            resampler;
	audio_line_t                 audio_line;
	DARRAY(struct audiobuf)      audio_wait_buffer;
	DARRAY(struct source_frame*) video_frames;
	pthread_mutex_t              audio_mutex;
	pthread_mutex_t              video_mutex;
	struct filtered_audio        audio_data;
	size_t                       audio_storage_size;

	/* filters */
	struct obs_source            *filter_parent;
	struct obs_source            *filter_target;
	DARRAY(struct obs_source*)   filters;
	pthread_mutex_t              filter_mutex;
	bool                         rendering_filter;
};

extern bool get_source_info(void *module, const char *module_name,
		const char *source_name, struct source_info *info);

extern bool obs_source_init(struct obs_source *source, const char *settings,
		const struct source_info *info);

extern void obs_source_activate(obs_source_t source);
extern void obs_source_deactivate(obs_source_t source);
extern void obs_source_video_tick(obs_source_t source, float seconds);
