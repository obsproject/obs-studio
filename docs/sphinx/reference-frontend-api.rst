OBS Studio Frontend API
=======================

The OBS Studio frontend API is the API specific to OBS Studio itself.

.. code:: cpp

   #include <obs-frontend-api.h>


Structures/Enumerations
-----------------------

.. enum:: obs_frontend_event

   Specifies a front-end event.  Can be one of the following values:

   - **OBS_FRONTEND_EVENT_STREAMING_STARTING**

     Triggered when streaming is starting.

   - **OBS_FRONTEND_EVENT_STREAMING_STARTED**

     Triggered when streaming has successfully started.

   - **OBS_FRONTEND_EVENT_STREAMING_STOPPING**

     Triggered when streaming is stopping.

   - **OBS_FRONTEND_EVENT_STREAMING_STOPPED**

     Triggered when streaming has fully stopped.

   - **OBS_FRONTEND_EVENT_RECORDING_STARTING**

     Triggered when recording is starting.

   - **OBS_FRONTEND_EVENT_RECORDING_STARTED**

     Triggered when recording has successfully started.

   - **OBS_FRONTEND_EVENT_RECORDING_STOPPING**

     Triggered when recording is stopping.

   - **OBS_FRONTEND_EVENT_RECORDING_STOPPED**

     Triggered when recording has fully stopped.

   - **OBS_FRONTEND_EVENT_RECORDING_PAUSED**

     Triggered when the recording has been paused.

   - **OBS_FRONTEND_EVENT_RECORDING_UNPAUSED**

     Triggered when the recording has been unpaused.

   - **OBS_FRONTEND_EVENT_SCENE_CHANGED**

     Triggered when the current scene has changed.

   - **OBS_FRONTEND_EVENT_SCENE_LIST_CHANGED**

     Triggered when a scenes has been added/removed/reordered by the
     user.

   - **OBS_FRONTEND_EVENT_TRANSITION_CHANGED**

     Triggered when the current transition has changed by the user.

   - **OBS_FRONTEND_EVENT_TRANSITION_STOPPED**

     Triggered when a transition has completed.

   - **OBS_FRONTEND_EVENT_TRANSITION_LIST_CHANGED**

     Triggered when the user adds/removes transitions.

   - **OBS_FRONTEND_EVENT_TRANSITION_DURATION_CHANGED**

     Triggered when the transition duration has been changed by the
     user.

   - **OBS_FRONTEND_EVENT_TBAR_VALUE_CHANGED**

     Triggered when the transition bar is moved.

   - **OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGING**

     Triggered when the current scene collection is about to change.

   - **OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED**

     Triggered when the current scene collection has changed.

   - **OBS_FRONTEND_EVENT_SCENE_COLLECTION_LIST_CHANGED**

     Triggered when a scene collection has been added or removed.

   - **OBS_FRONTEND_EVENT_SCENE_COLLECTION_RENAMED**

     Triggered when a scene collection has been renamed.

   - **OBS_FRONTEND_EVENT_PROFILE_CHANGING**

     Triggered when the current profile is about to change.

   - **OBS_FRONTEND_EVENT_PROFILE_CHANGED**

     Triggered when the current profile has changed.

   - **OBS_FRONTEND_EVENT_PROFILE_LIST_CHANGED**

     Triggered when a profile has been added or removed.

   - **OBS_FRONTEND_EVENT_PROFILE_RENAMED**

     Triggered when a profile has been renamed.

   - **OBS_FRONTEND_EVENT_FINISHED_LOADING**

     Triggered when the program has finished loading.

   - **OBS_FRONTEND_EVENT_SCRIPTING_SHUTDOWN**

     Triggered when scripts need to know that OBS is exiting.  The
     **OBS_FRONTEND_EVENT_EXIT** event is normally called after scripts
     have been destroyed.

   - **OBS_FRONTEND_EVENT_EXIT**

     Triggered when the program is about to exit. This is the last chance
     to call any frontend API functions for any saving / cleanup / etc.
     After returning from this event callback, it is not permitted to make
     any further frontend API calls.

   - **OBS_FRONTEND_EVENT_REPLAY_BUFFER_STARTING**

     Triggered when the replay buffer is starting.

   - **OBS_FRONTEND_EVENT_REPLAY_BUFFER_STARTED**

     Triggered when the replay buffer has successfully started.

   - **OBS_FRONTEND_EVENT_REPLAY_BUFFER_STOPPING**

     Triggered when the replay buffer is stopping.

   - **OBS_FRONTEND_EVENT_REPLAY_BUFFER_STOPPED**

     Triggered when the replay buffer has fully stopped.

   - **OBS_FRONTEND_EVENT_REPLAY_BUFFER_SAVED**

     Triggered when the replay buffer has been saved.

   - **OBS_FRONTEND_EVENT_STUDIO_MODE_ENABLED**

     Triggered when the user has turned on studio mode.

   - **OBS_FRONTEND_EVENT_STUDIO_MODE_DISABLED**

     Triggered when the user has turned off studio mode.

   - **OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED**

     Triggered when the current preview scene has changed in studio
     mode.

   - **OBS_FRONTEND_EVENT_SCENE_COLLECTION_CLEANUP**

     Triggered when a scene collection has been completely unloaded, and
     the program is either about to load a new scene collection, or the
     program is about to exit.

   - **OBS_FRONTEND_EVENT_VIRTUALCAM_STARTED**

     Triggered when the virtual camera is started.

   - **OBS_FRONTEND_EVENT_VIRTUALCAM_STOPPED**

     Triggered when the virtual camera is stopped.

   - **OBS_FRONTEND_EVENT_THEME_CHANGED**

     Triggered when the theme is changed.

     .. versionadded:: 29.0.0

   - **OBS_FRONTEND_EVENT_SCREENSHOT_TAKEN**

     Triggered when a screenshot is taken.

     .. versionadded:: 29.0.0

.. struct:: obs_frontend_source_list

   - DARRAY(obs_source_t*) **sources**

   Example usage:

   .. code:: cpp

      struct obs_frontend_source_list scenes = {0};

      obs_frontend_get_scenes(&scenes);

      for (size_t i = 0; i < scenes.sources.num; i++) {
            /* Do NOT call `obs_source_release` or `obs_scene_release`
             * on these sources
             */
            obs_source_t *source = scenes.sources.array[i];

            /* Convert to obs_scene_t if needed */
            obs_scene_t *scene = obs_scene_from_source(source);

            [...]
      }

      obs_frontend_source_list_free(&scenes);

.. type:: void (*obs_frontend_cb)(void *private_data)

   Frontend tool menu callback

.. type:: void (*obs_frontend_event_cb)(enum obs_frontend_event event, void *private_data)

   Frontend event callback

.. type:: void (*obs_frontend_save_cb)(obs_data_t *save_data, bool saving, void *private_data)

   Frontend save/load callback

.. type:: bool (*obs_frontend_translate_ui_cb)(const char *text, const char **out)

   Translation callback

.. type::  void (*undo_redo_cb)(const char *data)

   Undo redo callback


Functions
---------

.. function:: void obs_frontend_source_list_free(struct obs_frontend_source_list *source_list)

   Releases sources within a source list and frees the list.

   :param source_list: Source list to free

---------------------------------------

.. function:: void *obs_frontend_get_main_window(void)

   :return: The QMainWindow pointer to the OBS Studio window

---------------------------------------

.. function:: void *obs_frontend_get_main_window_handle(void)

   :return: The native window handle of the OBS Studio window

---------------------------------------

.. function:: char **obs_frontend_get_scene_names(void)

   :return: The scene name list, ending with NULL.  The list is stored
            within one contiguous segment of memory, so freeing the
            returned pointer with :c:func:`bfree()` will free the entire
            list. The order is same as the way the frontend displays it in
            the Scenes dock.

---------------------------------------

.. function:: void obs_frontend_get_scenes(struct obs_frontend_source_list *sources)

   Populates ``sources`` with reference-incremented scenes in the same order as
   the frontend displays it in the Scenes dock. Release with
   :c:func:`obs_frontend_source_list_free`, which will automatically release all
   scenes with :c:func:`obs_source_release`. Do not release a scene manually to
   prevent double releasing, which may cause scenes to be deleted.

   Use :c:func:`obs_scene_from_source` to access a source from the list as an
   :c:type:`obs_scene_t` object.

   If you wish to keep a reference to a certain scene, use
   :c:func:`obs_source_get_ref` or :c:func:`obs_scene_get_ref` on that scene and
   release it with either :c:func:`obs_source_release` or
   :c:func:`obs_scene_release`. Use only one release function, as both releases
   the same object.

   :param sources: Pointer to a :c:type:`obs_frontend_source_list`
                   structure to receive the list of reference-incremented
                   scenes.

---------------------------------------

.. function:: obs_source_t *obs_frontend_get_current_scene(void)

   :return: A new reference to the currently active scene. Release with
            :c:func:`obs_source_release()`.

---------------------------------------

.. function:: void obs_frontend_set_current_scene(obs_source_t *scene)

   :param scene: The scene to set as the current scene

---------------------------------------

.. function:: void obs_frontend_get_transitions(struct obs_frontend_source_list *sources)

   :param sources: Pointer to a :c:type:`obs_frontend_source_list`
                   structure to receive the list of
                   reference-incremented transitions.  Release with
                   :c:func:`obs_frontend_source_list_free`

---------------------------------------

.. function:: obs_source_t *obs_frontend_get_current_transition(void)

   :return: A new reference to the currently active transition.
            Release with :c:func:`obs_source_release()`.

---------------------------------------

.. function:: void obs_frontend_set_current_transition(obs_source_t *transition)

   :param transition: The transition to set as the current transition

---------------------------------------

.. function:: int obs_frontend_get_transition_duration(void)

   :return: The transition duration (in milliseconds) currently set in the UI

---------------------------------------

.. function:: void obs_frontend_set_transition_duration(int duration)

   :param duration: Desired transition duration, in milliseconds

---------------------------------------

.. function:: void obs_frontend_release_tbar(void)

   Emulate a mouse button release on the transition bar and determine transition status.

---------------------------------------

.. function:: void obs_frontend_set_tbar_position(int position)

   Set the value of the transition bar.

   :param position: The position to set the T-bar to, with a range of 0-1023

---------------------------------------

.. function:: int obs_frontend_get_tbar_position(void)

   Get the value of the transition bar.

   :return: The value of the position of the T-bar to, with a range of 0-1023

---------------------------------------

.. function:: char **obs_frontend_get_scene_collections(void)

   :return: The list of scene collection names, ending with NULL.  The list is
            stored within one contiguous segment of memory, so freeing
            the returned pointer with :c:func:`bfree()` will free the
            entire list.

---------------------------------------

.. function:: char *obs_frontend_get_current_scene_collection(void)

   :return: A new pointer to the current scene collection name.  Free
            with :c:func:`bfree()`

---------------------------------------

.. function:: void obs_frontend_set_current_scene_collection(const char *collection)

   :param collection: Name of the scene collection to activate

---------------------------------------

.. function:: bool obs_frontend_add_scene_collection(const char *name)

   Add a new scene collection, then switch to it.

   :param name: Name of the scene collection to add/create

---------------------------------------

.. function:: char **obs_frontend_get_profiles(void)

   :return: The list of profile names, ending with NULL.  The list is
            stored within one contiguous segment of memory, so freeing
            the returned pointer with :c:func:`bfree()` will free the
            entire list.

---------------------------------------

.. function:: char *obs_frontend_get_current_profile(void)

   :return: A new pointer to the current profile name.  Free with
            :c:func:`bfree()`

---------------------------------------

.. function:: char *obs_frontend_get_current_profile_path(void)

   :return: A new pointer to the current profile's path on the
            filesystem.  Free with :c:func:`bfree()`

---------------------------------------

.. function:: void obs_frontend_set_current_profile(const char *profile)

   :param profile: Name of the profile to activate

---------------------------------------

.. function:: bool obs_frontend_create_profile(const char *name)

   :param name: Name of the new profile to create (must be unique)

---------------------------------------

.. function:: bool obs_frontend_duplicate_profile(const char *name)

   :param name: Name of the duplicate profile to create (must be unique)

---------------------------------------

.. function:: void obs_frontend_delete_profile(const char *profile)

   :param profile: Name of the profile to delete

---------------------------------------

.. function:: void *obs_frontend_add_tools_menu_qaction(const char *name)

   Adds a QAction to the tools menu then returns it.

   :param name: Name for the new menu item
   :return: A pointer to the added QAction

---------------------------------------

.. function:: void obs_frontend_add_tools_menu_item(const char *name, obs_frontend_cb callback, void *private_data)

   Adds a tools menu item and links the ::clicked signal to the
   callback.

   :param name: The name for the new menu item
   :param callback: Callback to use when the menu item is clicked
   :param private_data: Private data associated with the callback

---------------------------------------

.. function:: bool obs_frontend_add_dock_by_id(const char *id, const char *title, void *widget)

   Adds a dock with the widget to the UI with a toggle in the Docks
   menu.

   When the dock is closed, a custom QEvent of type `QEvent::User + QEvent::Close`
   is sent to the widget to enable it to react to the event (e.g., unload elements
   to save resources).
   A generic QShowEvent is already sent by default when the widget is being
   shown (e.g., dock opened).

   Note: Use :c:func:`obs_frontend_remove_dock` to remove the dock
         and the id from the UI.

   :param id: Unique identifier of the dock
   :param title: Window title of the dock
   :param widget: QWidget to insert in the dock
   :return: *true* if the dock was added, *false* if the id was already
            used

   .. versionadded:: 30.0

---------------------------------------

.. function:: void obs_frontend_remove_dock(const char *id)

   Removes the dock with this id from the UI.

   :param id: Unique identifier of the dock to remove.

   .. versionadded:: 30.0

---------------------------------------

.. function:: bool obs_frontend_add_custom_qdock(const char *id, void *dock)

   Adds a custom dock to the UI with no toggle.

   Note: Use :c:func:`obs_frontend_remove_dock` to remove the dock
         reference and id from the UI.

   :param id: Unique identifier of the dock
   :param dock: QDockWidget to add
   :return: *true* if the dock was added, *false* if the id was already
            used

   .. versionadded:: 30.0

---------------------------------------

.. function:: void obs_frontend_add_event_callback(obs_frontend_event_cb callback, void *private_data)

   Adds a callback that will be called when a frontend event occurs.
   See :c:type:`obs_frontend_event` on what sort of events can be
   triggered.

   :param callback:     Callback to use when a frontend event occurs
   :param private_data: Private data associated with the callback

---------------------------------------

.. function:: void obs_frontend_remove_event_callback(obs_frontend_event_cb callback, void *private_data)

   Removes an event callback.

   :param callback:     Callback to remove
   :param private_data: Private data associated with the callback

---------------------------------------

.. function:: void obs_frontend_add_save_callback(obs_frontend_save_cb callback, void *private_data)

   Adds a callback that will be called when the current scene collection
   is being saved/loaded.

   :param callback:     Callback to use when saving/loading a scene
                        collection
   :param private_data: Private data associated with the callback

---------------------------------------

.. function:: void obs_frontend_remove_save_callback(obs_frontend_save_cb callback, void *private_data)

   Removes a save/load callback.

   :param callback:     Callback to remove
   :param private_data: Private data associated with the callback

---------------------------------------

.. function:: void obs_frontend_add_preload_callback(obs_frontend_save_cb callback, void *private_data)

   Adds a callback that will be called right before a scene collection
   is loaded.

   :param callback:     Callback to use when pre-loading
   :param private_data: Private data associated with the callback

---------------------------------------

.. function:: void obs_frontend_remove_preload_callback(obs_frontend_save_cb callback, void *private_data)

   Removes a pre-load callback.

   :param callback:     Callback to remove
   :param private_data: Private data associated with the callback

---------------------------------------

.. function:: void obs_frontend_push_ui_translation(obs_frontend_translate_ui_cb translate)

   Pushes a UI translation callback.  This allows a front-end plugin to
   intercept when Qt is automatically generating translating data.
   Typically this is just called with obs_module_get_string.

   :param translate: The translation callback to use

---------------------------------------

.. function:: void obs_frontend_pop_ui_translation(void)

   Pops the current UI translation callback.

---------------------------------------

.. function:: void obs_frontend_streaming_start(void)

   Starts streaming.

---------------------------------------

.. function:: void obs_frontend_streaming_stop(void)

   Stops streaming.

---------------------------------------

.. function:: bool obs_frontend_streaming_active(void)

   :return: *true* if streaming active, *false* otherwise

---------------------------------------

.. function:: void obs_frontend_recording_start(void)

   Starts recording.

---------------------------------------

.. function:: void obs_frontend_recording_stop(void)

   Stops recording.

---------------------------------------

.. function:: bool obs_frontend_recording_active(void)

   :return: *true* if recording active, *false* otherwise

---------------------------------------

.. function:: void obs_frontend_recording_pause(bool pause)

   :pause: *true* to pause recording, *false* to unpause

---------------------------------------

.. function:: bool obs_frontend_recording_paused(void)

   :return: *true* if recording paused, *false* otherwise

---------------------------------------

.. function:: bool obs_frontend_recording_split_file(void)

   Asks OBS to split the current recording file.

   :return: *true* if splitting was successfully requested (this
            does not mean that splitting has finished or guarantee that it
            split successfully), *false* if recording is inactive or paused
            or if file splitting is disabled.

---------------------------------------

.. function:: bool obs_frontend_recording_add_chapter(const char *name)

   Asks OBS to insert a chapter marker at the current output time into the recording.

   :param name: The name for the chapter, may be *NULL* to use an automatically generated name ("Unnamed <Chapter number>" or localized equivalent).
   :return: *true* if insertion was successful, *false* if recording is inactive, paused, or if chapter insertion is not supported by the current output.

   .. versionadded:: 30.2

---------------------------------------

.. function:: void obs_frontend_replay_buffer_start(void)

   Starts the replay buffer.

---------------------------------------

.. function:: void obs_frontend_replay_buffer_stop(void)

   Stops the replay buffer.

---------------------------------------

.. function:: void obs_frontend_replay_buffer_save(void)

   Saves a replay if the replay buffer is active.

---------------------------------------

.. function:: bool obs_frontend_replay_buffer_active(void)

   :return: *true* if replay buffer active, *false* otherwise

---------------------------------------

.. function:: void obs_frontend_open_projector(const char *type, int monitor, const char *geometry, const char *name)

   :param type:     "Preview", "Source", "Scene", "StudioProgram", or "Multiview" (case insensitive)
   :param monitor:  Monitor to open the projector on.  If -1, this opens a window.
   :param geometry: If *monitor* is -1, size and position of the projector window.  Encoded in Base64 using Qt's geometry encoding.
   :param name:     If *type* is "Source" or "Scene", name of the source or scene to be displayed

---------------------------------------

.. function:: void obs_frontend_save(void)

   Saves the current scene collection.

---------------------------------------

.. function:: obs_output_t *obs_frontend_get_streaming_output(void)

   :return: A new reference to the current streaming output.
            Release with :c:func:`obs_output_release()`.

---------------------------------------

.. function:: obs_output_t *obs_frontend_get_recording_output(void)

   :return: A new reference to the current recording output.
            Release with :c:func:`obs_output_release()`.

---------------------------------------

.. function:: obs_output_t *obs_frontend_get_replay_buffer_output(void)

   :return: A new reference to the current replay buffer output.
            Release with :c:func:`obs_output_release()`.

---------------------------------------

.. function:: config_t *obs_frontend_get_profile_config(void)

   :return: The config_t* associated with the current profile

---------------------------------------

.. deprecated:: 31.0
.. function:: config_t *obs_frontend_get_global_config(void)

   :return: The config_t* associated with the global config (global.ini)

---------------------------------------

.. function:: config_t *obs_frontend_get_app_config(void)

   :return: The config_t* associated with system-wide settings (global.ini)

   .. versionadded:: 31.0

---------------------------------------

.. function:: config_t *obs_frontend_get_user_config(void)

   :return: The config_t* associated with user settings (user.ini)

   .. versionadded:: 31.0

---------------------------------------

.. function:: void obs_frontend_set_streaming_service(obs_service_t *service)

   Sets the current streaming service to stream with.

   :param service: The streaming service to set

---------------------------------------

.. function:: obs_service_t *obs_frontend_get_streaming_service(void)

   :return: The current streaming service object. Does not increment the
            reference.

---------------------------------------

.. function:: void obs_frontend_save_streaming_service(void)

   Saves the current streaming service data.

---------------------------------------

.. function:: bool obs_frontend_preview_program_mode_active(void)

   :return: *true* if studio mode is active, *false* otherwise

---------------------------------------

.. function:: void obs_frontend_set_preview_program_mode(bool enable)

   Activates/deactivates studio mode.

   :param enable: *true* to activate studio mode, *false* to deactivate
                  studio mode

---------------------------------------

.. function:: void obs_frontend_preview_program_trigger_transition(void)

   Triggers a preview-to-program transition if studio mode is active.

---------------------------------------

.. function:: obs_source_t *obs_frontend_get_current_preview_scene(void)

   :return: A new reference to the current preview scene if studio mode
            is active, or *NULL* if studio mode is not active. Release
            with :c:func:`obs_source_release()`.

---------------------------------------

.. function:: void obs_frontend_set_current_preview_scene(obs_source_t *scene)

   Sets the current preview scene in studio mode. Does nothing if studio
   mode is disabled.

   :param scene: The scene to set as the current preview

---------------------------------------

.. function:: void obs_frontend_set_preview_enabled(bool enable)

   Sets the enable state of the preview display.  Only relevant with
   studio mode disabled.

   :param enable: *true* to enable preview, *false* to disable preview

---------------------------------------

.. function:: bool obs_frontend_preview_enabled(void)

   :return: *true* if the preview display is enabled, *false* otherwise

---------------------------------------

.. function:: void *obs_frontend_take_screenshot(void)

   Takes a screenshot of the main OBS output.

---------------------------------------

.. function:: void *obs_frontend_take_source_screenshot(obs_source_t *source)

   Takes a screenshot of the specified source.

   :param source: The source to take screenshot of

---------------------------------------

.. function:: obs_output_t *obs_frontend_get_virtualcam_output(void)

   :return: A new reference to the current virtual camera output.
            Release with :c:func:`obs_output_release()`.

---------------------------------------

.. function:: void obs_frontend_start_virtualcam(void)

   Starts the virtual camera.

---------------------------------------

.. function:: void obs_frontend_stop_virtualcam(void)

   Stops the virtual camera.

---------------------------------------

.. function:: bool obs_frontend_virtualcam_active(void)

   :return: *true* if virtual camera is active, *false* otherwise

---------------------------------------

.. function:: void obs_frontend_reset_video(void)

   Reloads the UI canvas and resets libobs video with latest data from
   the current profile.

---------------------------------------

.. function:: void *obs_frontend_open_source_properties(obs_source_t *source)

   Opens the properties window of the specified source.

   :param source: The source to open the properties window of

---------------------------------------

.. function:: void *obs_frontend_open_source_filters(obs_source_t *source)

   Opens the filters window of the specified source.

   :param source: The source to open the filters window of

---------------------------------------

.. function:: void *obs_frontend_open_source_interaction(obs_source_t *source)

   Opens the interact window of the specified source. Only call if
   source has the *OBS_SOURCE_INTERACTION* output flag.

   :param source: The source to open the interact window of

---------------------------------------

.. function:: void *obs_frontend_open_sceneitem_edit_transform(obs_sceneitem_t *item)

   Opens the edit transform window of the specified sceneitem.

   :param item: The sceneitem to open the edit transform window of

   .. versionadded:: 29.1

---------------------------------------

.. function:: char *obs_frontend_get_current_record_output_path(void)

   :return: A new pointer to the current record output path.  Free
            with :c:func:`bfree()`

---------------------------------------

.. function:: const char *obs_frontend_get_locale_string(const char *string)

   :return: Gets the frontend translation of a given string.

---------------------------------------

.. function:: bool obs_frontend_is_theme_dark(void)

   :return: Checks if the current theme is dark or light.

   .. versionadded:: 29.0.0

---------------------------------------

.. function:: char *obs_frontend_get_last_recording(void)

   :return: The file path of the last recording. Free with :c:func:`bfree()`

   .. versionadded:: 29.0.0

---------------------------------------

.. function:: char *obs_frontend_get_last_screenshot(void)

   :return: The file path of the last screenshot taken. Free with
            :c:func:`bfree()`

   .. versionadded:: 29.0.0

---------------------------------------

.. function:: char *obs_frontend_get_last_replay(void)

   :return: The file path of the last replay buffer saved. Free with
            :c:func:`bfree()`

   .. versionadded:: 29.0.0

---------------------------------------

.. function:: void obs_frontend_add_undo_redo_action(const char *name, const undo_redo_cb undo, const undo_redo_cb redo, const char *undo_data, const char *redo_data, bool repeatable)

   :param name: The name of the undo redo action
   :param undo: Callback to use for undo
   :param redo: Callback to use for redo
   :param undo_data: String with data for the undo callback
   :param redo_data: String with data for the redo callback
   :param repeatable: Allow multiple actions with the same name to be merged to 1 undo redo action.
                      This uses the undo action from the first and the redo action from the last action.

   .. versionadded:: 29.1
