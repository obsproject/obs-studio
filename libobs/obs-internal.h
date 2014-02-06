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

#include "media-io/video-io.h"
#include "media-io/audio-io.h"

#include "obs.h"
#include "obs-module.h"
#include "obs-source.h"
#include "obs-output.h"
#include "obs-service.h"
#include "obs-encoder.h"

#define LOAD_MODULE_SUBFUNC(name, required) \
	do { \
		info->name = load_module_subfunc(module, module_name, \
				id, #name, required); \
		if (required && !info->name) \
			return false; \
	} while (false)

#define NUM_TEXTURES 2

struct obs_display {
	swapchain_t                 swap; /* can be NULL if just sound */
	obs_source_t                channels[MAX_CHANNELS];

	/* TODO: sound output target */
};

/* ------------------------------------------------------------------------- */

struct obs_core_video {
	graphics_t                  graphics;
	stagesurf_t                 copy_surfaces[NUM_TEXTURES];
	texture_t                   render_textures[NUM_TEXTURES];
	texture_t                   output_textures[NUM_TEXTURES];
	bool                        textures_rendered[NUM_TEXTURES];
	bool                        textures_output[NUM_TEXTURES];
	bool                        textures_copied[NUM_TEXTURES];
	effect_t                    default_effect;
	stagesurf_t                 mapped_surface;
	int                         cur_texture;

	video_t                     video;
	pthread_t                   video_thread;
	bool                        thread_initialized;

	uint32_t                    base_width;
	uint32_t                    base_height;
};

struct obs_core_audio {
	/* TODO: sound output subsystem */
	audio_t                     audio;
};

/* user sources, output channels, and displays */
struct obs_core_data {
	/* arrays of pointers jim?  you should really stop being lazy and use
	 * linked lists. */
	DARRAY(struct obs_display*) displays;
	DARRAY(struct obs_source*)  sources;
	DARRAY(struct obs_output*)  outputs;
	DARRAY(struct obs_encoder*) encoders;

	obs_source_t                channels[MAX_CHANNELS];
	pthread_mutex_t             sources_mutex;
	pthread_mutex_t             displays_mutex;
	pthread_mutex_t             outputs_mutex;
	pthread_mutex_t             encoders_mutex;

	volatile bool               valid;
};

struct obs_core {
	DARRAY(struct obs_module)   modules;
	DARRAY(struct source_info)  input_types;
	DARRAY(struct source_info)  filter_types;
	DARRAY(struct source_info)  transition_types;
	DARRAY(struct output_info)  output_types;
	DARRAY(struct encoder_info) encoder_types;
	DARRAY(struct service_info) service_types;
	DARRAY(struct obs_modal_ui) ui_modal_callbacks;
	DARRAY(struct obs_modeless_ui) ui_modeless_callbacks;

	signal_handler_t            signals;
	proc_handler_t              procs;

	/* segmented into multiple sub-structures to keep things a bit more
	 * clean and organized */
	struct obs_core_video       video;
	struct obs_core_audio       audio;
	struct obs_core_data        data;
};

extern struct obs_core *obs;

extern void *obs_video_thread(void *param);
