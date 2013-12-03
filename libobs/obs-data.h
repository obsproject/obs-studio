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

#pragma once

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
#include "obs-service.h"

#define NUM_TEXTURES 2

struct obs_display {
	swapchain_t                 swap; /* can be NULL if just sound */
	obs_source_t                channels[MAX_CHANNELS];

	/* TODO: sound output target */
};

/* ------------------------------------------------------------------------- */

struct obs_video {
	graphics_t                  graphics;
	stagesurf_t                 copy_surfaces[NUM_TEXTURES];
	texture_t                   render_textures[NUM_TEXTURES];
	texture_t                   output_textures[NUM_TEXTURES];
	effect_t                    default_effect;
	bool                        textures_copied[NUM_TEXTURES];
	bool                        copy_mapped;
	int                         cur_texture;

	video_t                     video;
	pthread_t                   video_thread;
	bool                        thread_initialized;

	uint32_t                    output_width;
	uint32_t                    output_height;
};

struct obs_audio {
	/* TODO: audio subsystem */
	audio_t                     audio;
};

/* user sources, output channels, and displays */
struct obs_data {
	DARRAY(struct obs_display*) displays;
	DARRAY(struct obs_source*)  sources;

	obs_source_t                channels[MAX_CHANNELS];
	pthread_mutex_t             sources_mutex;
	pthread_mutex_t             displays_mutex;
};

struct obs_subsystem {
	DARRAY(struct obs_module)   modules;
	DARRAY(struct source_info)  input_types;
	DARRAY(struct source_info)  filter_types;
	DARRAY(struct source_info)  transition_types;
	DARRAY(struct output_info)  output_types;
	DARRAY(struct service_info) service_types;

	media_t                     media;

	/* segmented into multiple sub-structures to keep things a bit more
	 * clean and organized */
	struct obs_video            video;
	struct obs_audio            audio;
	struct obs_data             data;
};

extern struct obs_subsystem *obs;

extern void *obs_video_thread(void *param);
