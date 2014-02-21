/******************************************************************************
    Copyright (C) 2013-2014 by Hugh Bailey <obs.jim@gmail.com>

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

#include "util/c99defs.h"
#include "util/darray.h"
#include "util/dstr.h"
#include "util/threading.h"
#include "callback/signal.h"
#include "callback/proc.h"

#include "graphics/graphics.h"

#include "media-io/audio-resampler.h"
#include "media-io/video-io.h"
#include "media-io/audio-io.h"

#include "obs.h"

#define NUM_TEXTURES 2


struct draw_callback {
	void (*draw)(void *param, uint32_t cx, uint32_t cy);
	void *param;
};

/* ------------------------------------------------------------------------- */
/* modules */

struct obs_module {
	char *name;
	void *module;
};

extern void free_module(struct obs_module *mod);


/* ------------------------------------------------------------------------- */
/* views */

struct obs_view {
	pthread_mutex_t                 channels_mutex;
	obs_source_t                    channels[MAX_CHANNELS];
};

extern bool obs_view_init(struct obs_view *view);
extern void obs_view_free(struct obs_view *view);


/* ------------------------------------------------------------------------- */
/* displays */

struct obs_display {
	bool                            size_changed;
	uint32_t                        cx, cy;
	swapchain_t                     swap;
	pthread_mutex_t                 draw_callbacks_mutex;
	DARRAY(struct draw_callback)    draw_callbacks;
};

extern bool obs_display_init(struct obs_display *display,
		struct gs_init_data *graphics_data);
extern void obs_display_free(struct obs_display *display);


/* ------------------------------------------------------------------------- */
/* core */

struct obs_core_video {
	graphics_t                      graphics;
	stagesurf_t                     copy_surfaces[NUM_TEXTURES];
	texture_t                       render_textures[NUM_TEXTURES];
	texture_t                       output_textures[NUM_TEXTURES];
	texture_t                       convert_textures[NUM_TEXTURES];
	bool                            textures_rendered[NUM_TEXTURES];
	bool                            textures_output[NUM_TEXTURES];
	bool                            textures_copied[NUM_TEXTURES];
	bool                            textures_converted[NUM_TEXTURES];
	struct source_frame             convert_frames[NUM_TEXTURES];
	effect_t                        default_effect;
	effect_t                        conversion_effect;
	stagesurf_t                     mapped_surface;
	int                             cur_texture;

	video_t                         video;
	pthread_t                       video_thread;
	bool                            thread_initialized;

	bool                            gpu_conversion;
	const char                      *conversion_tech;
	uint32_t                        conversion_height;
	uint32_t                        plane_offsets[3];
	uint32_t                        plane_sizes[3];
	uint32_t                        plane_linewidth[3];

	uint32_t                        output_width;
	uint32_t                        output_height;
	uint32_t                        base_width;
	uint32_t                        base_height;

	struct obs_display              main_display;
};

struct obs_core_audio {
	/* TODO: sound output subsystem */
	audio_t                         audio;

	float                           user_volume;
	float                           present_volume;
};

/* user sources, output channels, and displays */
struct obs_core_data {
	/* arrays of pointers jim?  you should really stop being lazy and use
	 * linked lists. */
	DARRAY(struct obs_display*)     displays;
	DARRAY(struct obs_source*)      sources;
	DARRAY(struct obs_output*)      outputs;
	DARRAY(struct obs_encoder*)     encoders;

	pthread_mutex_t                 sources_mutex;
	pthread_mutex_t                 displays_mutex;
	pthread_mutex_t                 outputs_mutex;
	pthread_mutex_t                 encoders_mutex;

	struct obs_view                 main_view;

	volatile bool                   valid;
};

struct obs_core {
	DARRAY(struct obs_module)       modules;
	DARRAY(struct obs_source_info)  input_types;
	DARRAY(struct obs_source_info)  filter_types;
	DARRAY(struct obs_source_info)  transition_types;
	DARRAY(struct obs_output_info)  output_types;
	DARRAY(struct obs_encoder_info) encoder_types;
	DARRAY(struct obs_service_info) service_types;
	DARRAY(struct obs_modal_ui)     modal_ui_callbacks;
	DARRAY(struct obs_modeless_ui)  modeless_ui_callbacks;

	signal_handler_t                signals;
	proc_handler_t                  procs;

	/* segmented into multiple sub-structures to keep things a bit more
	 * clean and organized */
	struct obs_core_video           video;
	struct obs_core_audio           audio;
	struct obs_core_data            data;
};

extern struct obs_core *obs;

extern void *obs_video_thread(void *param);


/* ------------------------------------------------------------------------- */
/* sources  */

struct obs_source {
	volatile int                    refs;
	struct obs_source_info          info;

	/* source-specific data */
	char                            *name; /* user-defined name */
	enum obs_source_type            type;
	obs_data_t                      settings;
	void                            *data;

	signal_handler_t                signals;
	proc_handler_t                  procs;

	/* prevents infinite recursion when enumerating sources */
	int                             enum_refs;

	/* used to indicate that the source has been removed and all
	 * references to it should be released (not exactly how I would prefer
	 * to handle things but it's the best option) */
	bool                            removed;

	/* timing (if video is present, is based upon video) */
	volatile bool                   timing_set;
	volatile uint64_t               timing_adjust;
	volatile int                    audio_reset_ref;
	uint64_t                        next_audio_ts_min;
	uint64_t                        last_frame_ts;
	uint64_t                        last_sys_timestamp;

	/* audio */
	bool                            audio_failed;
	struct resample_info            sample_info;
	audio_resampler_t               resampler;
	audio_line_t                    audio_line;
	pthread_mutex_t                 audio_mutex;
	struct filtered_audio           audio_data;
	size_t                          audio_storage_size;
	float                           user_volume;
	float                           present_volume;
	int64_t                         sync_offset;

	/* async video data */
	texture_t                       output_texture;
	DARRAY(struct source_frame*)    video_frames;
	pthread_mutex_t                 video_mutex;

	/* filters */
	struct obs_source               *filter_parent;
	struct obs_source               *filter_target;
	DARRAY(struct obs_source*)      filters;
	pthread_mutex_t                 filter_mutex;
	texrender_t                     filter_texrender;
	bool                            rendering_filter;
};

bool obs_source_init_handlers(struct obs_source *source);
extern bool obs_source_init(struct obs_source *source,
		const struct obs_source_info *info);

extern void obs_source_activate(obs_source_t source);
extern void obs_source_deactivate(obs_source_t source);
extern void obs_source_video_tick(obs_source_t source, float seconds);


/* ------------------------------------------------------------------------- */
/* outputs  */

struct obs_output {
	char                            *name;
	void                            *data;
	struct obs_output_info          info;
	obs_data_t                      settings;
};


/* ------------------------------------------------------------------------- */
/* encoders  */

struct encoder_callback {
	void (*new_packet)(void *param, struct encoder_packet *packet);
	void *param;
};

struct obs_encoder {
	char                            *name;
	void                            *data;
	struct obs_encoder_info         info;
	obs_data_t                      settings;

	pthread_mutex_t                 data_callbacks_mutex;
	DARRAY(struct encoder_callback) data_callbacks;
};
