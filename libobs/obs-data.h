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

#ifndef OBS_DATA_H
#define OBS_DATA_H

#include "util/darray.h"
#include "util/threading.h"

#include "graphics/graphics.h"

#include "media-io/media-io.h"
#include "media-io/video-io.h"
#include "media-io/audio-io.h"

#include "obs.h"
#include "obs-module.h"
#include "obs-source.h"
#include "obs-output.h"
/*#include "obs-service.h"*/

#define NUM_TEXTURES 2

struct obs_display {
	swapchain_t swap; /* can be NULL if just sound */
	source_t    source;
	/* TODO: sound output target */
};

struct obs_data {
	DARRAY(struct obs_module)       modules;

	DARRAY(struct source_info)      input_types;
	DARRAY(struct source_info)      filter_types;
	DARRAY(struct source_info)      transition_types;
	DARRAY(struct output_info)      output_types;
	/*DARRAY(struct service_info)     service_types;*/

	DARRAY(struct obs_display*)     displays;
	DARRAY(struct obs_source*)      sources;

	/* graphics */
	graphics_t  graphics;
	stagesurf_t copy_surfaces[NUM_TEXTURES];
	bool        textures_copied[NUM_TEXTURES];
	bool        copy_mapped;
	int         cur_texture;

	/* TODO: sound output stuff */

	/* media */
	media_t    media;
	video_t    video;
	audio_t    audio;

	uint32_t output_width;
	uint32_t output_height;

	/* threading */
	pthread_t       video_thread;
	pthread_mutex_t source_mutex;
	bool            thread_initialized;

	source_t primary_source;
};

extern void *obs_video_thread(void *param);

#endif
