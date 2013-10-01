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

#ifndef LIBOBS_H
#define LIBOBS_H

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

enum source_type {
	SOURCE_INPUT,
	SOURCE_FILTER,
	SOURCE_TRANSITION,
	SOURCE_SCENE
};

enum order_movement {
	ORDER_MOVE_UP,
	ORDER_MOVE_DOWN,
	ORDER_MOVE_TOP,
	ORDER_MOVE_BOTTOM
};

/* opaque types */
struct obs_data;
struct obs_display;
struct obs_source;
struct obs_scene;
struct obs_scene_item;
struct obs_output;

typedef struct obs_data       *obs_t;
typedef struct obs_display    *display_t;
typedef struct obs_source     *source_t;
typedef struct obs_scene      *scene_t;
typedef struct obs_scene_item *sceneitem_t;
typedef struct obs_output     *output_t;

/* ------------------------------------------------------------------------- */
/* OBS context */

/**
 * Creates the OBS context.
 *
 *   Using the graphics module specified, creates an OBS context and sets the
 * primary video/audio output information.
 */
EXPORT obs_t      obs_create(const char *graphics_module,
                             struct gs_init_data *graphics_data,
                             struct video_info *vi, struct audio_info *ai);
EXPORT void       obs_destroy(obs_t obs);

/**
 * Sets base video ouput base resolution/fps/format
 *
 *   NOTE: Cannot reset base video if currently streaming/recording.
 */
EXPORT bool       obs_reset_video(obs_t obs, struct video_info *vi);
/**
 * Sets base audio output format/channels/samples/etc
 *
 *   NOTE: Cannot reset base audio if currently streaming/recording.
 */
EXPORT bool       obs_reset_audio(obs_t obs, struct audio_info *ai);

/**
 * Loads a plugin module
 *
 *   A plugin module contains exports for inputs/fitlers/transitions/outputs.
 * See obs-source.h and obs-output.h for more information on the exports to
 * use.
 */
EXPORT int        obs_load_module(obs_t obs, const char *path);

/**
 * Enumerates all available inputs.
 *
 *   Inputs are general source inputs (such as capture sources, device sources,
 * etc).
 */
EXPORT bool       obs_enum_inputs(obs_t obs, size_t idx, const char **name);
/**
 * Enumerates all available filters.
 *
 *   Filters are sources that are used to modify the video/audio output of
 * other sources.
 */
EXPORT bool       obs_enum_filters(obs_t obs, size_t idx, const char **name);
/**
 * Enumerates all available transitions.
 *
 *   Transitions are sources used to transition between two or more other
 * sources.
 */
EXPORT bool       obs_enum_transitions(obs_t obs, size_t idx,
                                       const char **name);
/**
 * Enumerates all available ouputs.
 *
 *   Outputs handle color space conversion, encoding, and output to file or
 * streams.
 */
EXPORT bool       obs_enum_outputs(obs_t obs, size_t idx, const char **name);

/** Gets the graphics context for this OBS context */
EXPORT graphics_t obs_graphics(obs_t obs);
/** Get the media context for this OBS context (used for outputs) */
EXPORT media_t    obs_media(obs_t obs);

/**
 * Sets/gets the primary output source.
 *
 *   The primary source is the source that's broadcasted/saved.
 */
EXPORT void       obs_set_primary_source(obs_t obs, source_t source);
EXPORT source_t   obs_get_primary_source(obs_t obs);


/* ------------------------------------------------------------------------- */
/* Display context */

/**
 * Creates an extra display context.
 *
 *   An extra display can be used for things like separate previews,
 * viewing sources independently, and other things.  Creates a new swap chain
 * linked to a specific window to display a source.
 */
EXPORT display_t  display_create(obs_t obs, struct gs_init_data *graphics_data);
EXPORT void       display_destroy(display_t display);
/** Sets the source to be used for a display context. */
EXPORT void       display_setsource(display_t display, source_t source);
EXPORT source_t   display_getsource(display_t display);


/* ------------------------------------------------------------------------- */
/* Sources */

/**
 * Creates a source of the specified type with the specified settings.
 *
 *   The "source" context is used for anything related to presenting
 * or modifying video/audio.
 */
EXPORT source_t   source_create(obs_t obs, enum source_type type,
                                const char *name, const char *settings);
EXPORT void       source_destroy(source_t source);
/**
 * Retrieves flags that specify what type of data the source presents/modifies.
 *
 *   SOURCE_VIDEO if it presents/modifies video_frame
 *   SOURCE_ASYNC if the video is asynchronous.
 *   SOURCE_AUDIO if it presents/modifies audio (always async)
 */
EXPORT uint32_t   source_get_output_flags(source_t source);
/** Specifies whether the source can be configured */
EXPORT bool       source_hasconfig(source_t source);
/** Opens a configuration panel and attaches it to the parent window */
EXPORT void       source_config(source_t source, void *parent);
/** Renders a video source. */
EXPORT void       source_video_render(source_t source);
/** Gets the width of a source (if it has video) */
EXPORT int        source_getwidth(source_t source);
/** Gets the height of a source (if it has video) */
EXPORT int        source_getheight(source_t source);
/**
 * Gets a custom parameter from the source.
 *
 *   Used for efficiently modifying source properties in real time.
 */
EXPORT size_t     source_getparam(source_t source, const char *param, void *buf,
                                  size_t buf_size);
/**
 * Sets a custom parameter for the source.
 *
 *   Used for efficiently modifying source properties in real time.
 */
EXPORT void       source_setparam(source_t source, const char *param,
                                  const void *data, size_t size);
/**
 * Enumerates child sources this source has
 */
EXPORT bool       source_enum_children(source_t source, size_t idx,
                                       source_t *child);

/** If the source is a filter, returns the target source of the filter */
EXPORT source_t   filter_gettarget(source_t filter);
/** Adds a filter to the source (which is used whenever the source is used) */
EXPORT void       source_filter_add(source_t source, source_t filter);
/** Removes a filter from the source */
EXPORT void       source_filter_remove(source_t source, source_t filter);
/** Modifies the order of a specific filter */
EXPORT void       source_filter_setorder(source_t source, source_t filter,
                                         enum order_movement movement);

/** Gets the settings string for a source */
EXPORT const char *source_get_settings(source_t source);

/* ------------------------------------------------------------------------- */
/* Functions used by sources */

/** Saves the settings string for a source */
EXPORT void source_save_settings(source_t source, const char *settings);
/** Outputs asynchronous video data */
EXPORT void source_output_video(source_t source, struct video_frame *frame);
/** Outputs audio data (always asynchronous) */
EXPORT void source_output_audio(source_t source, struct audio_data *audio);


/* ------------------------------------------------------------------------- */
/* Scenes */

/**
 * Creates a scene.
 *
 *   A scene is a source which is a container of other sources with specific
 * display oriantations.  Scenes can also be used like any other source.
 */
EXPORT scene_t     scene_create(obs_t obs);
EXPORT void        scene_destroy(scene_t scene);
/** Gets the scene's source context */
EXPORT source_t    scene_source(scene_t scene);

/** Adds/creates a new scene item for a source */
EXPORT sceneitem_t scene_add(scene_t scene, source_t source);

/** Removes/destroys a scene item */
EXPORT void  sceneitem_destroy(sceneitem_t item);

/* Functions for gettings/setting specific oriantation of a scene item */
EXPORT void  sceneitem_setpos(sceneitem_t item, const struct vec2 *pos);
EXPORT void  sceneitem_setrot(sceneitem_t item, float rot);
EXPORT void  sceneitem_setorigin(sceneitem_t item, const struct vec2 *origin);
EXPORT void  sceneitem_setscale(sceneitem_t item, const struct vec2 *scale);
EXPORT void  sceneitem_setorder(sceneitem_t item, enum order_movement movement);

EXPORT void  sceneitem_getpos(sceneitem_t item, struct vec2 *pos);
EXPORT float sceneitem_getrot(sceneitem_t item);
EXPORT void  sceneitem_getorigin(sceneitem_t item, struct vec2 *center);
EXPORT void  sceneitem_getscale(sceneitem_t item, struct vec2 *scale);


/* ------------------------------------------------------------------------- */
/* Outputs */

/**
 * Creates an output.
 *
 *   Outputs allow outputting to file, outputting to network, outputting to
 * directshow, or other custom outputs.
 */
EXPORT output_t   output_create(obs_t obs, const char *name,
                                const char *settings);
EXPORT void       output_destroy(output_t output);
/** Starts the output. */
EXPORT void       output_start(output_t output);
/** Stops the output. */
EXPORT void       output_stop(output_t output);
/** Specifies whether the output can be configured */
EXPORT bool       output_canconfig(output_t output);
/** Opens a configuration panel with the specified parent window */
EXPORT void       output_config(output_t output, void *parent);
/** Specifies whether the output can be paused */
EXPORT bool       output_canpause(output_t output);
/** Pauses the output (if the functionality is allowed by the output */
EXPORT void       output_pause(output_t output);

/* Gets the current output settings string */
EXPORT const char *output_get_settings(output_t output);

/* Saves the output settings string, typically used only by outputs */
EXPORT void       output_save_settings(output_t output, const char *settings);

#ifdef __cplusplus
}
#endif

#endif
