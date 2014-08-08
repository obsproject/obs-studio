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

#include "obs.h"

#ifdef __cplusplus
extern "C" {
#endif


enum obs_source_type {
	OBS_SOURCE_TYPE_INPUT,
	OBS_SOURCE_TYPE_FILTER,
	OBS_SOURCE_TYPE_TRANSITION,
};


/**
 * @name Source output flags
 *
 * These flags determine what type of data the source outputs and expects.
 * @{
 */

/**
 * Source has video.
 *
 * Unless SOURCE_ASYNC_VIDEO is specified, the source must include the
 * video_render callback in the source definition structure.
 */
#define OBS_SOURCE_VIDEO        (1<<0)

/**
 * Source has audio.
 *
 * Use the obs_source_output_audio function to pass raw audio data, which will
 * be automatically converted and uploaded.  If used with SOURCE_ASYNC_VIDEO,
 * audio will automatically be synced up to the video output.
 */
#define OBS_SOURCE_AUDIO        (1<<1)

/** Async video flag (use OBS_SOURCE_ASYNC_VIDEO) */
#define OBS_SOURCE_ASYNC        (1<<2)

/**
 * Source passes raw video data via RAM.
 *
 * Use the obs_source_output_video function to pass raw video data, which will
 * be automatically uploaded at the specified timestamp.
 *
 * If this flag is specified, it is not necessary to include the video_render
 * callback.  However, if you wish to use that function as well, you must call
 * obs_source_getframe to get the current frame data, and
 * obs_source_releaseframe to release the data when complete.
 */
#define OBS_SOURCE_ASYNC_VIDEO  (OBS_SOURCE_ASYNC | OBS_SOURCE_VIDEO)

/**
 * Source uses custom drawing, rather than a default effect.
 *
 * If this flag is specified, the video_render callback will pass a NULL
 * effect, and effect-based filters will not use direct rendering.
 */
#define OBS_SOURCE_CUSTOM_DRAW  (1<<3)

/**
 * Source uses a color matrix (usually YUV sources).
 *
 * When this is used, the video_render callback will automatically assign a
 * 4x4 YUV->RGB matrix to the "color_matrix" parameter of the effect, or it can
 * be changed to a custom value.
 */
#define OBS_SOURCE_COLOR_MATRIX (1<<4)

/** @} */

typedef void (*obs_source_enum_proc_t)(obs_source_t parent, obs_source_t child,
		void *param);

/**
 * Source definition structure
 */
struct obs_source_info {
	/* ----------------------------------------------------------------- */
	/* Required implementation*/

	/** Unique string identifier for the source */
	const char *id;

	/**
	 * Type of source.
	 *
	 * OBS_SOURCE_TYPE_INPUT for input sources,
	 * OBS_SOURCE_TYPE_FILTER for filter sources, and
	 * OBS_SOURCE_TYPE_TRANSITION for transition sources.
	 */
	enum obs_source_type type;

	/** Source output flags */
	uint32_t output_flags;

	/**
	 * Get the translated name of the source type
	 *
	 * @return         The translated name of the source type
	 */
	const char *(*get_name)(void);

	/**
	 * Creates the source data for the source
	 *
	 * @param  settings  Settings to initialize the source with
	 * @param  source    Source that this data is assoicated with
	 * @return           The data associated with this source
	 */
	void *(*create)(obs_data_t settings, obs_source_t source);

	/**
	 * Destroys the private data for the source
	 *
	 * Async sources must not call obs_source_output_video after returning
	 * from destroy
	 */
	void (*destroy)(void *data);

	/** Returns the width of the source.  Required if this is an input
	 * source and has non-async video */
	uint32_t (*get_width)(void *data);

	/** Returns the height of the source.  Required if this is an input
	 * source and has non-async video */
	uint32_t (*get_height)(void *data);

	/* ----------------------------------------------------------------- */
	/* Optional implementation */

	/**
	 * Gets the default settings for this source
	 *
	 * @param[out]  settings  Data to assign default settings to
	 */
	void (*get_defaults)(obs_data_t settings);

	/** 
	 * Gets the property information of this source
	 *
	 * @return         The properties data
	 */
	obs_properties_t (*get_properties)(void);

	/**
	 * Updates the settings for this source
	 *
	 * @param data      Source data
	 * @param settings  New settings for this source
	 */
	void (*update)(void *data, obs_data_t settings);

	/** Called when the source has been activated in the main view */
	void (*activate)(void *data);

	/**
	 * Called when the source has been deactivated from the main view
	 * (no longer being played/displayed)
	 */
	void (*deactivate)(void *data);

	/** Called when the source is visible */
	void (*show)(void *data);

	/** Called when the source is no longer visible */
	void (*hide)(void *data);

	/**
	 * Called each video frame with the time elapsed
	 *
	 * @param  data     Source data
	 * @param  seconds  Seconds elapsed since the last frame
	 */
	void (*video_tick)(void *data, float seconds);

	/**
	 * Called when rendering the source with the graphics subsystem.
	 *
	 * If this is an input/transition source, this is called to draw the
	 * source texture with the graphics subsystem using the specified
	 * effect.
	 *
	 * If this is a filter source, it wraps source draw calls (for
	 * example applying a custom effect with custom parameters to a
	 * source).  In this case, it's highly recommended to use the
	 * obs_source_process_filter function to automatically handle
	 * effect-based filter processing.  However, you can implement custom
	 * draw handling as desired as well.
	 *
	 * If the source output flags do not include SOURCE_CUSTOM_DRAW, all
	 * a source needs to do is set the "image" parameter of the effect to
	 * the desired texture, and then draw.  If the output flags include
	 * SOURCE_COLOR_MATRIX, you may optionally set the the "color_matrix"
	 * parameter of the effect to a custom 4x4 conversion matrix (by
	 * default it will be set to an YUV->RGB conversion matrix)
	 *
	 * @param data    Source data
	 * @param effect  Effect to be used with this source.  If the source
	 *                output flags include SOURCE_CUSTOM_DRAW, this will
	 *                be NULL, and the source is expected to process with
	 *                an effect manually.
	 */
	void (*video_render)(void *data, gs_effect_t effect);

	/**
	 * Called to filter raw async video data.
	 *
	 * @note          This function is only used with filter sources.
	 *
	 * @param  data   Source data
	 * @param  frame  Video frame to filter
	 * @return        New video frame data.  This can defer video data to
	 *                be drawn later if time is needed for processing
	 */
	struct obs_source_frame *(*filter_video)(void *data,
			const struct obs_source_frame *frame);

	/**
	 * Called to filter raw audio data.
	 *
	 * @note          This function is only used with filter sources.
	 *
	 * @param  data   Source data
	 * @param  audio  Audio data to filter.
	 * @return        Modified or new audio data.  You can directly modify
	 *                the data passed and return it, or you can defer audio
	 *                data for later if time is needed for processing.
	 */
	struct obs_audio_data *(*filter_audio)(void *data,
			struct obs_audio_data *audio);

	/**
	 * Called to enumerate all sources being used within this source.
	 * If the source has children it must implement this callback.
	 *
	 * @param  data           Source data
	 * @param  enum_callback  Enumeration callback
	 * @param  param          User data to pass to callback
	 */
	void (*enum_sources)(void *data,
			obs_source_enum_proc_t enum_callback,
			void *param);

	/**
	 * Called when saving a source.  This is a separate function because
	 * sometimes a source needs to know when it is being saved so it
	 * doesn't always have to update the current settings until a certain
	 * point.
	 *
	 * @param  data      Source data
	 * @param  settings  Settings
	 */
	void (*save)(void *data, obs_data_t settings);

	/**
	 * Called when loading a source from saved data.  This should be called
	 * after all the loading sources have actually been created because
	 * sometimes there are sources that depend on each other.
	 *
	 * @param  data      Source data
	 * @param  settings  Settings
	 */
	void (*load)(void *data, obs_data_t settings);
};

EXPORT void obs_register_source_s(const struct obs_source_info *info,
		size_t size);

/**
 * Regsiters a source definition to the current obs context.  This should be
 * used in obs_module_load.
 *
 * @param  info  Pointer to the source definition structure
 */
#define obs_register_source(info) \
	obs_register_source_s(info, sizeof(struct obs_source_info))

#ifdef __cplusplus
}
#endif
