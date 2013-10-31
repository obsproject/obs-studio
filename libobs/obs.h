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
#include "graphics/graphics.h"
#include "graphics/vec2.h"
#include "media-io/media-io.h"

#include "obs-defs.h"

/*
 * Main libobs header used by applications.
 */

#ifdef __cplusplus
extern "C" {
#endif

enum obs_source_type {
	SOURCE_INPUT,
	SOURCE_FILTER,
	SOURCE_TRANSITION,
	SOURCE_SCENE
};

/* used for changing the order of items (for example, filters in a source,
 * or items in a scene) */
enum order_movement {
	ORDER_MOVE_UP,
	ORDER_MOVE_DOWN,
	ORDER_MOVE_TOP,
	ORDER_MOVE_BOTTOM
};

struct source_audio {
	const void          *data;
	uint32_t            frames;

	/* audio will be automatically resampled/upmixed/downmixed */
	enum speaker_layout speakers;
	enum audio_format   type;
	uint32_t            samples_per_sec;

	/* can be 0 if 'immediate' */
	uint64_t            timestamp;
};

struct source_frame {
	void                *data;
	uint32_t            width;
	uint32_t            height;
	uint32_t            row_bytes;
	uint64_t            timestamp;

	enum video_type     type;
	float               yuv_matrix[16];
	bool                flip;
};

static inline void source_frame_destroy(struct source_frame *frame)
{
	if (frame) {
		bfree(frame->data);
		bfree(frame);
	}
}

/* opaque types */
struct obs_display;
struct obs_source;
struct obs_scene;
struct obs_scene_item;
struct obs_output;

typedef struct obs_display    *obs_display_t;
typedef struct obs_source     *obs_source_t;
typedef struct obs_scene      *obs_scene_t;
typedef struct obs_scene_item *obs_sceneitem_t;
typedef struct obs_output     *obs_output_t;

/* ------------------------------------------------------------------------- */
/* OBS context */

/**
 * Starts up and shuts down OBS
 *
 *   Using the graphics module specified, creates an OBS context and sets the
 * primary video/audio output information.
 */
EXPORT bool obs_startup(const char *graphics_module,
		struct gs_init_data *graphics_data,
		struct video_info *vi, struct audio_info *ai);
EXPORT void obs_shutdown(void);

/**
 * Sets base video ouput base resolution/fps/format
 *
 *   NOTE: Cannot reset base video if currently streaming/recording.
 */

EXPORT bool obs_reset_video(struct video_info *vi);

/**
 * Sets base audio output format/channels/samples/etc
 *
 *   NOTE: Cannot reset base audio if currently streaming/recording.
 */
EXPORT bool obs_reset_audio(struct audio_info *ai);

/**
 * Loads a plugin module
 *
 *   A plugin module contains exports for inputs/fitlers/transitions/outputs.
 * See obs-source.h and obs-output.h for more information on the exports to
 * use.
 */
EXPORT int obs_load_module(const char *path);

/**
 * Enumerates all available inputs.
 *
 *   Inputs are general source inputs (such as capture sources, device sources,
 * etc).
 */
EXPORT bool obs_enum_inputs(size_t idx, const char **name);

/**
 * Enumerates all available filters.
 *
 *   Filters are sources that are used to modify the video/audio output of
 * other sources.
 */
EXPORT bool obs_enum_filters(size_t idx, const char **name);

/**
 * Enumerates all available transitions.
 *
 *   Transitions are sources used to transition between two or more other
 * sources.
 */
EXPORT bool obs_enum_transitions(size_t idx, const char **name);

/**
 * Enumerates all available ouputs.
 *
 *   Outputs handle color space conversion, encoding, and output to file or
 * streams.
 */
EXPORT bool obs_enum_outputs(size_t idx, const char **name);

/** Gets the graphics context for this OBS context */
EXPORT graphics_t obs_graphics(void);

/** Get the media context for this OBS context (used for outputs) */
EXPORT media_t obs_media(void);

/**
 * Sets/gets the primary output source.
 *
 *   The primary source is the source that's presented.
 */
EXPORT void         obs_set_primary_source(obs_source_t source);
EXPORT obs_source_t obs_get_primary_source(void);


/* ------------------------------------------------------------------------- */
/* Display context */

/**
 * Creates an extra display context.
 *
 *   An extra display can be used for things like separate previews,
 * viewing sources independently, and other things.  Creates a new swap chain
 * linked to a specific window to display a source.
 */
EXPORT obs_display_t obs_display_create(struct gs_init_data *graphics_data);
EXPORT void obs_display_destroy(obs_display_t display);

/** Sets the source to be used for a display context. */
EXPORT void obs_display_setsource(obs_display_t display, obs_source_t source);
EXPORT obs_source_t obs_display_getsource(obs_display_t display);


/* ------------------------------------------------------------------------- */
/* Sources */

/**
 * Creates a source of the specified type with the specified settings.
 *
 *   The "source" context is used for anything related to presenting
 * or modifying video/audio.
 */
EXPORT obs_source_t obs_source_create(enum obs_source_type type,
                                const char *name, const char *settings);
EXPORT void obs_source_destroy(obs_source_t source);

/**
 * Retrieves flags that specify what type of data the source presents/modifies.
 *
 *   SOURCE_VIDEO if it presents/modifies video_frame
 *   SOURCE_ASYNC if the video is asynchronous.
 *   SOURCE_AUDIO if it presents/modifies audio (always async)
 */
EXPORT uint32_t obs_source_get_output_flags(obs_source_t source);

/** Specifies whether the source can be configured */
EXPORT bool obs_source_hasconfig(obs_source_t source);

/** Opens a configuration panel and attaches it to the parent window */
EXPORT void obs_source_config(obs_source_t source, void *parent);

/** Renders a video source. */
EXPORT void obs_source_video_render(obs_source_t source);

/** Gets the width of a source (if it has video) */
EXPORT uint32_t obs_source_getwidth(obs_source_t source);

/** Gets the height of a source (if it has video) */
EXPORT uint32_t obs_source_getheight(obs_source_t source);

/**
 * Gets a custom parameter from the source.
 *
 *   Used for efficiently modifying source properties in real time.
 */
EXPORT size_t obs_source_getparam(obs_source_t source, const char *param,
		void *buf, size_t buf_size);

/**
 * Sets a custom parameter for the source.
 *
 *   Used for efficiently modifying source properties in real time.
 */
EXPORT void obs_source_setparam(obs_source_t source, const char *param,
		const void *data, size_t size);

/** Enumerates child sources this source has */
EXPORT bool obs_source_enum_children(obs_source_t source, size_t idx,
		obs_source_t *child);

/** If the source is a filter, returns the target source of the filter */
EXPORT obs_source_t obs_filter_gettarget(obs_source_t filter);

/** Adds a filter to the source (which is used whenever the source is used) */
EXPORT void obs_source_filter_add(obs_source_t source,obs_source_t filter);

/** Removes a filter from the source */
EXPORT void obs_source_filter_remove(obs_source_t source, obs_source_t filter);

/** Modifies the order of a specific filter */
EXPORT void obs_source_filter_setorder(obs_source_t source, obs_source_t filter,
		enum order_movement movement);

/** Gets the settings string for a source */
EXPORT const char *obs_source_get_settings(obs_source_t source);

/* ------------------------------------------------------------------------- */
/* Functions used by sources */

/** Saves the settings string for a source */
EXPORT void obs_source_save_settings(obs_source_t source, const char *settings);

/** Outputs asynchronous video data */
EXPORT void obs_source_obs_async_video(obs_source_t source,
		const struct source_frame *frame);

/** Outputs audio data (always asynchronous) */
EXPORT void obs_source_obs_async_audio(obs_source_t source,
		const struct source_audio *audio);

/** Gets the current async video frame */
EXPORT struct source_frame *obs_source_getframe(obs_source_t source);

/** Releases the current async video frame */
EXPORT void obs_source_releaseframe(obs_source_t source,
		struct source_frame *frame);


/* ------------------------------------------------------------------------- */
/* Scenes */

/**
 * Creates a scene.
 *
 *   A scene is a source which is a container of other sources with specific
 * display oriantations.  Scenes can also be used like any other source.
 */
EXPORT obs_scene_t obs_scene_create(void);
EXPORT void        obs_scene_destroy(obs_scene_t scene);

/** Gets the scene's source context */
EXPORT obs_source_t obs_scene_getsource(obs_scene_t scene);

/** Adds/creates a new scene item for a source */
EXPORT obs_sceneitem_t obs_scene_add(obs_scene_t scene, obs_source_t source);

/** Removes/destroys a scene item */
EXPORT void  obs_sceneitem_destroy(obs_sceneitem_t item);

/* Functions for gettings/setting specific oriantation of a scene item */
EXPORT void obs_sceneitem_setpos(obs_sceneitem_t item, const struct vec2 *pos);
EXPORT void obs_sceneitem_setrot(obs_sceneitem_t item, float rot);
EXPORT void obs_sceneitem_setorigin(obs_sceneitem_t item,
		const struct vec2 *origin);
EXPORT void obs_sceneitem_setscale(obs_sceneitem_t item,
		const struct vec2 *scale);
EXPORT void obs_sceneitem_setorder(obs_sceneitem_t item,
		enum order_movement movement);

EXPORT void  obs_sceneitem_getpos(obs_sceneitem_t item, struct vec2 *pos);
EXPORT float obs_sceneitem_getrot(obs_sceneitem_t item);
EXPORT void  obs_sceneitem_getorigin(obs_sceneitem_t item, struct vec2 *center);
EXPORT void  obs_sceneitem_getscale(obs_sceneitem_t item, struct vec2 *scale);


/* ------------------------------------------------------------------------- */
/* Outputs */

/**
 * Creates an output.
 *
 *   Outputs allow outputting to file, outputting to network, outputting to
 * directshow, or other custom outputs.
 */
EXPORT obs_output_t obs_output_create(const char *name, const char *settings);
EXPORT void obs_output_destroy(obs_output_t output);

/** Starts the output. */
EXPORT void obs_output_start(obs_output_t output);

/** Stops the output. */
EXPORT void obs_output_stop(obs_output_t output);

/** Specifies whether the output can be configured */
EXPORT bool obs_output_canconfig(obs_output_t output);

/** Opens a configuration panel with the specified parent window */
EXPORT void obs_output_config(obs_output_t output, void *parent);

/** Specifies whether the output can be paused */
EXPORT bool obs_output_canpause(obs_output_t output);

/** Pauses the output (if the functionality is allowed by the output */
EXPORT void obs_output_pause(obs_output_t output);

/* Gets the current output settings string */
EXPORT const char *obs_output_get_settings(obs_output_t output);

/* Saves the output settings string, typically used only by outputs */
EXPORT void obs_output_save_settings(obs_output_t output,
		const char *settings);

#ifdef __cplusplus
}
#endif
