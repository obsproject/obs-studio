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
#include "util/circlebuf.h"
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

	long long                       unnamed_index;

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
	volatile long                   refs;
	struct obs_source_info          info;

	/* source-specific data */
	char                            *name; /* user-defined name */
	obs_data_t                      settings;
	void                            *data;

	signal_handler_t                signals;
	proc_handler_t                  procs;

	bool                            defer_update;

	/* ensures show/hide are only called once */
	volatile long                   show_refs;

	/* ensures activate/deactivate are only called once */
	volatile long                   activate_refs;

	/* prevents infinite recursion when enumerating sources */
	volatile long                   enum_refs;

	/* used to indicate that the source has been removed and all
	 * references to it should be released (not exactly how I would prefer
	 * to handle things but it's the best option) */
	bool                            removed;

	/* timing (if video is present, is based upon video) */
	volatile bool                   timing_set;
	volatile uint64_t               timing_adjust;
	volatile long                   audio_reset_ref;
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

	/* transition volume is meant to store the sum of transitioning volumes
	 * of a source, i.e. if a source is within both the "to" and "from"
	 * targets of a transition, it would add both volumes to this variable,
	 * and then when the transition frame is complete, is applies the value
	 * to the presentation volume. */
	float                           transition_volume;

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

enum view_type {
	MAIN_VIEW,
	AUX_VIEW
};

extern void obs_source_activate(obs_source_t source, enum view_type type);
extern void obs_source_deactivate(obs_source_t source, enum view_type type);
extern void obs_source_video_tick(obs_source_t source, float seconds);


/* ------------------------------------------------------------------------- */
/* outputs  */

struct il_packet {
	int64_t                         input_ts_us;
	int64_t                         output_ts_us;
	struct encoder_packet           packet;
};

struct obs_output {
	char                            *name;
	void                            *data;
	struct obs_output_info          info;
	obs_data_t                      settings;

	signal_handler_t                signals;
	proc_handler_t                  procs;

	bool                            received_video;
	bool                            received_audio;
	int64_t                         first_video_ts;
	int64_t                         video_offset;
	int64_t                         audio_offset;
	int                             interleaved_wait;
	pthread_mutex_t                 interleaved_mutex;
	DARRAY(struct il_packet)        interleaved_packets;

	bool                            active;
	video_t                         video;
	audio_t                         audio;
	obs_encoder_t                   video_encoder;
	obs_encoder_t                   audio_encoder;

	bool                            video_conversion_set;
	bool                            audio_conversion_set;
	struct video_scale_info         video_conversion;
	struct audio_convert_info       audio_conversion;

	bool                            valid;
};

extern void obs_output_remove_encoder(struct obs_output *output,
		struct obs_encoder *encoder);


/* ------------------------------------------------------------------------- */
/* encoders  */

struct encoder_callback {
	bool sent_first_packet;
	void (*new_packet)(void *param, struct encoder_packet *packet);
	void *param;
};

struct obs_encoder {
	char                            *name;
	void                            *data;
	struct obs_encoder_info         info;
	obs_data_t                      settings;

	uint32_t                        samplerate;
	size_t                          planes;
	size_t                          blocksize;
	size_t                          framesize;
	size_t                          framesize_bytes;

	bool                            active;

	uint32_t                        timebase_num;
	uint32_t                        timebase_den;

	int64_t                         cur_pts;

	struct circlebuf                audio_input_buffer[MAX_AV_PLANES];
	uint8_t                         *audio_output_buffer[MAX_AV_PLANES];

	/* if a video encoder is paired with an audio encoder, make it start
	 * up at the specific timestamp.  if this is the audio encoder,
	 * wait_for_video makes it wait until it's ready to sync up with
	 * video */
	bool                            wait_for_video;
	struct obs_encoder              *paired_encoder;
	uint64_t                        start_ts;

	pthread_mutex_t                 outputs_mutex;
	DARRAY(obs_output_t)            outputs;

	bool                            destroy_on_stop;

	/* stores the video/audio media output pointer.  video_t or audio_t */
	void                            *media;

	pthread_mutex_t                 callbacks_mutex;
	DARRAY(struct encoder_callback) callbacks;
};

extern bool obs_encoder_initialize(obs_encoder_t encoder);

extern void obs_encoder_start(obs_encoder_t encoder,
		void (*new_packet)(void *param, struct encoder_packet *packet),
		void *param);
extern void obs_encoder_stop(obs_encoder_t encoder,
		void (*new_packet)(void *param, struct encoder_packet *packet),
		void *param);

extern void obs_encoder_add_output(struct obs_encoder *encoder,
		struct obs_output *output);
extern void obs_encoder_remove_output(struct obs_encoder *encoder,
		struct obs_output *output);
