OBS Core API Reference
======================

.. code:: cpp

   #include <obs.h>


.. _obs_init_shutdown_reference:

Initialization, Shutdown, and Information
-----------------------------------------

.. function:: bool obs_startup(const char *locale, const char *module_config_path, profiler_name_store_t *store)

   Initializes the OBS core context.
  
   :param  locale:             The locale to use for modules
                               (E.G. "en-US")
   :param  module_config_path: Path to module config storage directory
                               (or *NULL* if none)
   :param  store:              The profiler name store for OBS to use or NULL
   :return:                    *false* if already initialized or failed
                               to initialize

---------------------

.. function:: void obs_shutdown(void)

   Releases all data associated with OBS and terminates the OBS context.

---------------------

.. function:: bool obs_initialized(void)

   :return: true if the main OBS context has been initialized

---------------------

.. function:: uint32_t obs_get_version(void)

   :return: The current core version

---------------------

.. function:: const char *obs_get_version_string(void)

   :return: The current core version string

---------------------

.. function:: void obs_set_locale(const char *locale)

   Sets a new locale to use for modules.  This will call
   :c:func:`obs_module_set_locale()` for each module with the new locale.
  
   :param  locale: The locale to use for modules

---------------------

.. function:: const char *obs_get_locale(void)

   :return: The current locale

---------------------

.. function:: profiler_name_store_t *obs_get_profiler_name_store(void)

   :return: The profiler name store (see util/profiler.h) used by OBS,
            which is either a name store passed to :c:func:`obs_startup()`, an
            internal name store, or NULL in case :c:func:`obs_initialized()`
            returns false.

---------------------

.. function:: int obs_reset_video(struct obs_video_info *ovi)

   Sets base video output base resolution/fps/format.
  
   Note: This data cannot be changed if an output is currently active.

   Note: The graphics module cannot be changed without fully destroying
   the OBS context.

   :param   ovi: Pointer to an obs_video_info structure containing the
                 specification of the graphics subsystem,
   :return:      | OBS_VIDEO_SUCCESS          - Success
                 | OBS_VIDEO_NOT_SUPPORTED    - The adapter lacks capabilities
                 | OBS_VIDEO_INVALID_PARAM    - A parameter is invalid
                 | OBS_VIDEO_CURRENTLY_ACTIVE - Video is currently active
                 | OBS_VIDEO_MODULE_NOT_FOUND - The graphics module is not found
                 | OBS_VIDEO_FAIL             - Generic failure

   Relevant data types used with this function:

.. code:: cpp

   struct obs_video_info {
           /**
            * Graphics module to use (usually "libobs-opengl" or "libobs-d3d11")
            */
           const char          *graphics_module;
   
           uint32_t            fps_num;       /**< Output FPS numerator */
           uint32_t            fps_den;       /**< Output FPS denominator */
   
           uint32_t            base_width;    /**< Base compositing width */
           uint32_t            base_height;   /**< Base compositing height */
   
           uint32_t            output_width;  /**< Output width */
           uint32_t            output_height; /**< Output height */
           enum video_format   output_format; /**< Output format */
   
           /** Video adapter index to use (NOTE: avoid for optimus laptops) */
           uint32_t            adapter;
   
           /** Use shaders to convert to different color formats */
           bool                gpu_conversion;
   
           enum video_colorspace colorspace;  /**< YUV type (if YUV) */
           enum video_range_type range;       /**< YUV range (if YUV) */
   
           enum obs_scale_type scale_type;    /**< How to scale if scaling */
   };

---------------------

.. function:: bool obs_reset_audio(const struct obs_audio_info *oai)

   Sets base audio output format/channels/samples/etc.

   Note: Cannot reset base audio if an output is currently active.

   :return: *true* if successful, *false* otherwise

   Relevant data types used with this function:

.. code:: cpp

   struct obs_audio_info {
           uint32_t            samples_per_sec;
           enum speaker_layout speakers;
   };

---------------------

.. function:: bool obs_reset_audio2(const struct obs_audio_info2 *oai)

   Sets base audio output format/channels/samples/etc. Also allows the
   ability to set the maximum audio latency of OBS, and set whether the
   audio buffering is fixed or dynamically increasing.

   When using fixed audio buffering, OBS will automatically buffer to
   the maximum audio latency on startup.

   Maximum audio latency will clamp to the closest multiple of the audio
   output frames (which is typically 1024 audio frames).

   Note: Cannot reset base audio if an output is currently active.

   :return: *true* if successful, *false* otherwise

   Relevant data types used with this function:

.. code:: cpp

   struct obs_audio_info2 {
           uint32_t            samples_per_sec;
           enum speaker_layout speakers;

           uint32_t max_buffering_ms;
           bool fixed_buffering;
   };

---------------------

.. function:: bool obs_get_video_info(struct obs_video_info *ovi)

   Gets the current video settings.

   :return: *false* if no video

---------------------

.. function:: float obs_get_video_sdr_white_level(void)

   Gets the current SDR white level.

   :return: SDR white level, 300.f if no video

---------------------

.. function:: float obs_get_video_hdr_nominal_peak_level(void)

   Gets the current HDR nominal peak level.

   :return: HDR nominal peak level, 1000.f if no video

---------------------

.. function:: void obs_set_video_sdr_white_level(float sdr_white_level, float hdr_nominal_peak_level)

   Sets the current video levels.

---------------------

.. function:: bool obs_get_audio_info(struct obs_audio_info *oai)

   Gets the current audio settings.
   
   :return: *false* if no audio

---------------------


Libobs Objects
--------------

.. function:: bool obs_enum_source_types(size_t idx, const char **id)

   Enumerates all source types (inputs, filters, transitions, etc).

---------------------

.. function:: bool obs_enum_input_types(size_t idx, const char **id)

   Enumerates all available inputs source types.
  
   Inputs are general source inputs (such as capture sources, device sources,
   etc).

---------------------

.. function:: bool obs_enum_filter_types(size_t idx, const char **id)

   Enumerates all available filter source types.
  
   Filters are sources that are used to modify the video/audio output of
   other sources.

---------------------

.. function:: bool obs_enum_transition_types(size_t idx, const char **id)

   Enumerates all available transition source types.
  
   Transitions are sources used to transition between two or more other
   sources.

---------------------

.. function:: bool obs_enum_output_types(size_t idx, const char **id)

   Enumerates all available output types.

---------------------

.. function:: bool obs_enum_encoder_types(size_t idx, const char **id)

   Enumerates all available encoder types.

---------------------

.. function:: bool obs_enum_service_types(size_t idx, const char **id)

   Enumerates all available service types.

---------------------

.. function:: void obs_enum_sources(bool (*enum_proc)(void*, obs_source_t*), void *param)

   Enumerates all input sources.
  
   Callback function returns true to continue enumeration, or false to end
   enumeration.
  
   Use :c:func:`obs_source_get_ref()` or
   :c:func:`obs_source_get_weak_source()` if you want to retain a
   reference after obs_enum_sources finishes.

   For scripting, use :py:func:`obs_enum_sources`.

---------------------

.. function:: void obs_enum_scenes(bool (*enum_proc)(void*, obs_source_t*), void *param)

   Enumerates all scenes. Use :c:func:`obs_scene_from_source()` if the scene is
   needed as an :c:type:`obs_scene_t`. The order that they are enumerated should
   not be relied on. If one intends to enumerate the scenes in the order
   presented by the OBS Studio Frontend, use :c:func:`obs_frontend_get_scenes()`.
  
   Callback function returns true to continue enumeration, or false to end
   enumeration.
  
   Use :c:func:`obs_source_get_ref()` or
   :c:func:`obs_source_get_weak_source()` if you want to retain a
   reference after obs_enum_scenes finishes.
 
---------------------

.. function:: void obs_enum_outputs(bool (*enum_proc)(void*, obs_output_t*), void *param)

   Enumerates outputs.

   Callback function returns true to continue enumeration, or false to end
   enumeration.

   Use :c:func:`obs_output_get_ref()` or
   :c:func:`obs_output_get_weak_output()` if you want to retain a
   reference after obs_enum_outputs finishes.

---------------------

.. function:: void obs_enum_encoders(bool (*enum_proc)(void*, obs_encoder_t*), void *param)

   Enumerates encoders.

   Callback function returns true to continue enumeration, or false to end
   enumeration.

   Use :c:func:`obs_encoder_get_ref()` or
   :c:func:`obs_encoder_get_weak_encoder()` if you want to retain a
   reference after obs_enum_encoders finishes.

---------------------

.. function:: obs_source_t *obs_get_source_by_name(const char *name)

   Gets a source by its name.
  
   Increments the source reference counter, use
   :c:func:`obs_source_release()` to release it when complete.

---------------------

.. function:: obs_source_t *obs_get_source_by_uuid(const char *uuid)

   Gets a source by its UUID.
  
   Increments the source reference counter, use
   :c:func:`obs_source_release()` to release it when complete.

   .. versionadded:: 29.1

---------------------

.. function:: obs_source_t *obs_get_transition_by_name(const char *name)

   Gets a transition by its name.
  
   Increments the source reference counter, use
   :c:func:`obs_source_release()` to release it when complete.

   .. deprecated:: 31.0
      Use :c:func:`obs_frontend_get_transitions` from the Frontend API or :c:func:`obs_get_source_by_uuid` instead.

---------------------

.. function:: obs_source_t *obs_get_transition_by_uuid(const char *uuid)

   Gets a transition by its UUID.

   Increments the source reference counter, use
   :c:func:`obs_source_release()` to release it when complete.

   .. versionadded:: 29.1

   .. deprecated:: 31.0
      Use :c:func:`obs_get_source_by_uuid` instead.

---------------------

.. function:: obs_scene_t *obs_get_scene_by_name(const char *name)

   Gets a scene by its name.
  
   Increments the scene reference counter, use
   :c:func:`obs_scene_release()` to release it when complete.

---------------------

.. function:: obs_output_t *obs_get_output_by_name(const char *name)

   Gets an output by its name.

   Increments the output reference counter, use
   :c:func:`obs_output_release()` to release it when complete.

---------------------

.. function:: obs_encoder_t *obs_get_encoder_by_name(const char *name)

   Gets an encoder by its name.

   Increments the encoder reference counter, use
   :c:func:`obs_encoder_release()` to release it when complete.

---------------------

.. function:: obs_service_t *obs_get_service_by_name(const char *name)

   Gets an service by its name.

   Increments the service reference counter, use
   :c:func:`obs_service_release()` to release it when complete.

---------------------

.. function:: obs_data_t *obs_save_source(obs_source_t *source)

   :return: A new reference to a source's saved data. Use
            :c:func:`obs_data_release()` to release it when complete.

---------------------

.. function:: obs_source_t *obs_load_source(obs_data_t *data)

   :return: A source created from saved data

---------------------

.. function:: void obs_load_sources(obs_data_array_t *array, obs_load_source_cb cb, void *private_data)

   Helper function to load active sources from a data array.

   Relevant data types used with this function:

.. code:: cpp

   typedef void (*obs_load_source_cb)(void *private_data, obs_source_t *source);

---------------------

.. function:: obs_data_array_t *obs_save_sources(void)

   :return: A data array with the saved data of all active sources

---------------------

.. function:: obs_data_array_t *obs_save_sources_filtered(obs_save_source_filter_cb cb, void *data)

   :return: A data array with the saved data of all active sources,
            filtered by the *cb* function

   Relevant data types used with this function:

.. code:: cpp

   typedef bool (*obs_save_source_filter_cb)(void *data, obs_source_t *source);

---------------------


Video, Audio, and Graphics
--------------------------

.. function:: void obs_enter_graphics(void)

   Helper function for entering the OBS graphics context.

---------------------

.. function:: void obs_leave_graphics(void)

   Helper function for leaving the OBS graphics context.

---------------------

.. function:: audio_t *obs_get_audio(void)

   :return: The main audio output handler for this OBS context

---------------------

.. function:: video_t *obs_get_video(void)

   :return: The main video output handler for this OBS context

---------------------

.. function:: void obs_set_output_source(uint32_t channel, obs_source_t *source)

   Sets the primary output source for a channel.

---------------------

.. function:: obs_source_t *obs_get_output_source(uint32_t channel)

   Gets the primary output source for a channel and increments the reference
   counter for that source.  Use :c:func:`obs_source_release()` to release.

---------------------

.. function:: gs_effect_t *obs_get_base_effect(enum obs_base_effect effect)

   Returns a commonly used base effect.

   :param effect: | Can be one of the following values:
                  | OBS_EFFECT_DEFAULT             - RGB/YUV
                  | OBS_EFFECT_DEFAULT_RECT        - RGB/YUV (using texture_rect)
                  | OBS_EFFECT_OPAQUE              - RGB/YUV (alpha set to 1.0)
                  | OBS_EFFECT_SOLID               - RGB/YUV (solid color only)
                  | OBS_EFFECT_BICUBIC             - Bicubic downscale
                  | OBS_EFFECT_LANCZOS             - Lanczos downscale
                  | OBS_EFFECT_BILINEAR_LOWRES     - Bilinear low resolution downscale
                  | OBS_EFFECT_PREMULTIPLIED_ALPHA - Premultiplied alpha

---------------------

.. function:: void obs_render_main_texture(void)

   Renders the main output texture.  Useful for rendering a preview pane
   of the main output.

---------------------

.. function:: bool obs_audio_monitoring_available(void)

   :return: Whether audio monitoring is supported and available on the current platform

---------------------

.. function:: void obs_reset_audio_monitoring(void)

   Resets all audio monitoring devices.

   .. versionadded:: 30.1

---------------------

.. function:: void obs_enum_audio_monitoring_devices(obs_enum_audio_device_cb cb, void *data)

   Enumerates audio devices which can be used for audio monitoring.

   Callback function returns true to continue enumeration, or false to end
   enumeration.

   Relevant data types used with this function:

.. code:: cpp

   typedef bool (*obs_enum_audio_device_cb)(void *data, const char *name, const char *id);

---------------------

.. function:: bool obs_set_audio_monitoring_device(const char *name, const char *id)

   Sets the current audio device for audio monitoring.

---------------------

.. function:: void obs_get_audio_monitoring_device(const char **name, const char **id)

   Gets the current audio device for audio monitoring.

---------------------

.. function:: void obs_add_main_render_callback(void (*draw)(void *param, uint32_t cx, uint32_t cy), void *param)
              void obs_remove_main_render_callback(void (*draw)(void *param, uint32_t cx, uint32_t cy), void *param)

   Adds/removes a main rendering callback.  Allows custom rendering to
   the main stream/recording output.

   For scripting (**Lua only**), use :py:func:`obs_add_main_render_callback`
   and :py:func:`obs_remove_main_render_callback`.

---------------------

.. function:: void obs_add_main_rendered_callback(void (*rendered)(void *param), void *param)
              void obs_remove_main_rendered_callback(void (*rendered)(void *param), void *param)

   Adds/removes a main rendered callback.  Allows using the result of
   the main stream/recording output.

   .. versionadded:: 29.1

---------------------

.. function:: void obs_add_raw_video_callback(const struct video_scale_info *conversion, void (*callback)(void *param, struct video_data *frame), void *param)
              void obs_remove_raw_video_callback(void (*callback)(void *param, struct video_data *frame), void *param)

   Adds/removes a raw video callback.  Allows the ability to obtain raw
   video frames without necessarily using an output.

   :param conversion: Specifies conversion requirements.  Can be NULL.
   :param callback:   The callback that receives raw video frames.
   :param param:      The private data associated with the callback.

---------------------

.. function:: void obs_add_raw_audio_callback(size_t mix_idx, const struct audio_convert_info *conversion, audio_output_callback_t callback, void *param)
              void obs_remove_raw_raw_callback(size_t track, audio_output_callback_t callback, void *param)

   Adds/removes a raw audio callback.  Allows the ability to obtain raw
   audio data without necessarily using an output.

   :param mix_idx:    Specifies audio track to get data from.
   :param conversion: Specifies conversion requirements.  Can be NULL.
   :param callback:   The callback that receives raw audio data.
   :param param:      The private data associated with the callback.

Primary signal/procedure handlers
---------------------------------

.. function:: signal_handler_t *obs_get_signal_handler(void)

   :return: The primary obs signal handler. Should not be manually freed,
            as its lifecycle is managed by libobs.

   See :ref:`core_signal_handler_reference` for more information on
   core signals.

---------------------

.. function:: proc_handler_t *obs_get_proc_handler(void)

   :return: The primary obs procedure handler. Should not be manually freed,
            as its lifecycle is managed by libobs.


.. _core_signal_handler_reference:

Core OBS Signals
----------------

**source_create** (ptr source)

   Called when a source has been created.

**source_destroy** (ptr source)

   Called when a source has been destroyed.

**source_remove** (ptr source)

   Called when a source has been removed (:c:func:`obs_source_remove()`
   has been called on the source).

**source_update** (ptr source)

   Called when a source's settings have been updated.

**source_save** (ptr source)

   Called when a source is being saved.

**source_load** (ptr source)

   Called when a source is being loaded.

**source_activate** (ptr source)

   Called when a source has been activated in the main view (visible on
   stream/recording).

**source_deactivate** (ptr source)

   Called when a source has been deactivated from the main view (no
   longer visible on stream/recording).

**source_show** (ptr source)

   Called when a source is visible on any display and/or on the main
   view.

**source_hide** (ptr source)

   Called when a source is no longer visible on any display and/or on
   the main view.

**source_rename** (ptr source, string new_name, string prev_name)

   Called when a source has been renamed.

**source_volume** (ptr source, in out float volume)

   Called when a source's volume has changed.

**source_audio_activate** (ptr source)

   Called when a source's audio becomes active.

**source_audio_deactivate** (ptr source)

   Called when a source's audio becomes inactive.

**source_filter_add** (ptr source, ptr filter)

   Called when a filter is added to a source.

**source_filter_remove** (ptr source, ptr filter)

   Called when a filter is removed from a source.

**source_transition_start** (ptr source)

   Called when a transition has started its transition.

**source_transition_video_stop** (ptr source)

   Called when a transition has stopped its video transitioning.

**source_transition_stop** (ptr source)

   Called when a transition has stopped its transition.

**channel_change** (int channel, in out ptr source, ptr prev_source)

   Called when :c:func:`obs_set_output_source()` has been called.

**hotkey_layout_change** ()

   Called when the hotkey layout has changed.

**hotkey_register** (ptr hotkey)

   Called when a hotkey has been registered.

**hotkey_unregister** (ptr hotkey)

   Called when a hotkey has been unregistered.

**hotkey_bindings_changed** (ptr hotkey)

   Called when a hotkey's bindings has changed.

---------------------


.. _display_reference:

Displays
--------

.. function:: obs_display_t *obs_display_create(const struct gs_init_data *graphics_data)

   Adds a new window display linked to the main render pipeline.  This creates
   a new swap chain which updates every frame.
  
   *(Important note: do not use more than one display widget within the
   hierarchy of the same base window; this will cause presentation
   stalls on macOS.)*

   :param  graphics_data: The swap chain initialization data
   :return:               The new display context, or NULL if failed

   Relevant data types used with this function:

.. code:: cpp

   enum gs_color_format {
           [...]
           GS_RGBA,
           GS_BGRX,
           GS_BGRA,
           GS_RGBA16F,
           GS_RGBA32F,
           [...]
   };
   
   enum gs_zstencil_format {
           GS_ZS_NONE,
           GS_Z16,
           GS_Z24_S8,
           GS_Z32F,
           GS_Z32F_S8X24
   };
   
   struct gs_window {
   #if defined(_WIN32)
           void                    *hwnd;
   #elif defined(__APPLE__)
           __unsafe_unretained id  view;
   #elif defined(__linux__) || defined(__FreeBSD__)
           uint32_t                id;
           void                    *display;
   #endif
   };
   
   struct gs_init_data {
           struct gs_window        window;
           uint32_t                cx, cy;
           uint32_t                num_backbuffers;
           enum gs_color_format    format;
           enum gs_zstencil_format zsformat;
           uint32_t                adapter;
   };

---------------------

.. function:: void obs_display_destroy(obs_display_t *display)

   Destroys a display context.

---------------------

.. function:: void obs_display_resize(obs_display_t *display, uint32_t cx, uint32_t cy)

   Changes the size of a display context.

---------------------

.. function:: void obs_display_add_draw_callback(obs_display_t *display, void (*draw)(void *param, uint32_t cx, uint32_t cy), void *param)

   Adds a draw callback for a display context, which will be called
   whenever the display is rendered.
  
   :param  display: The display context
   :param  draw:    The draw callback which is called each time a frame
                    updates
   :param  param:   The user data to be associated with this draw callback

---------------------

.. function:: void obs_display_remove_draw_callback(obs_display_t *display, void (*draw)(void *param, uint32_t cx, uint32_t cy), void *param)

   Removes a draw callback for a display context.

---------------------

.. function:: void obs_display_set_enabled(obs_display_t *display, bool enable)

   Enables/disables a display context.

---------------------

.. function:: bool obs_display_enabled(obs_display_t *display)

   :return: *true* if the display is enabled, *false* otherwise

---------------------

.. function:: void obs_display_set_background_color(obs_display_t *display, uint32_t color)

   Sets the background (clear) color for the display context.

.. _view_reference:

Views
----------------

.. function:: obs_view_t *obs_view_create(void)

   :return: A view context

---------------------

.. function:: void obs_view_destroy(obs_view_t *view)

   Destroys a view context.

---------------------

.. function:: void obs_view_render(obs_view_t *view)

   Renders the sources of this view context.

---------------------

.. function:: video_t *obs_view_add(obs_view_t *view)

   Renders the sources of this view context.

   :return: The main video output handler for the view context

---------------------

.. function:: video_t *obs_view_add2(obs_view_t *view, struct obs_video_info *ovi)

   Adds a view to the main render loop, with custom video settings.

   :return: The main video output handler for the view context

---------------------

.. function:: void obs_view_remove(obs_view_t *view)

   Removes a view from the main render loop.

---------------------

.. function:: void obs_view_set_source(obs_view_t *view, uint32_t channel, obs_source_t *source)

   Sets the source to be used for this view context.

---------------------

.. function:: obs_source_t *obs_view_get_source(obs_view_t *view, uint32_t channel)

   :return: The source currently in use for this view context

---------------------

.. function:: bool obs_view_get_video_info(obs_view_t *view, struct obs_video_info *ovi)

   Gets the video settings of the first matching mix currently in use for this view context.

   :return: *false* if no video

   .. deprecated:: 3X.X

---------------------

.. function:: void obs_view_enum_video_info(obs_view_t *view, bool (*enum_proc)(void *, struct obs_video_info *), void *param)

   Enumerates all the video info of all mixes that use the specified mix.

   .. versionadded:: 30.1
