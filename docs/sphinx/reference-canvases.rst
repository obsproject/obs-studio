Canvas API Reference (obs_canvas_t)
===================================

Canvases are reference-counted objects that contain scenes and define how those are rendered.
They provide a video object which can be used with encoders or raw outputs.

libobs maintains a main canvas that exists at all times and is used for the default video outputs.

.. type:: obs_canvas_t

   A reference-counted canvas.

.. type:: obs_weak_canvas_t

   A weak reference to a canvas.

.. code:: cpp

   #include <obs.h>

.. _canvas_signal_handler_reference:

Canvas Signals
--------------

The following signals are defined for canvases:

**remove** (ptr canvas)

   Called when the :c:func:`obs_canvas_remove()` function is called on the canvas.

**destroy** (ptr canvas)

   Called when a canvas is about to be destroyed.

**video_reset** (ptr canvas)

   Called when the canvas's video mix has been reset after a call to
   :c:func:`obs_reset_video()` or :c:func:`obs_canvas_reset_video()`.

**source_add** (ptr canvas, ptr source)

   Called when a source has been added to the canvas.

**source_remove** (ptr canvas, ptr source)

   Called when a source has been removed from the canvas.

**rename** (ptr canvas, string new_name, string prev_name)

   Called when the canvas has been renamed.

**channel_change** (ptr canvas, int channel, in out ptr source, ptr prev_source)

   Called when a channel source has been changed.

Canvas Flags
------------

Canvases can have different behaviors, these can be controlled via the **flags** parameter when creating a canvas.

Flags may be `0` or a bitwise `OR` combination of the following values:

- **MAIN** - Main canvas, cannot be renamed or reset, cannot be set by user
- **ACTIVATE** - Canvas's sources will become active when they are visible
- **MIX_AUDIO** - Audio from channels in this canvas will be mixed into the audio output
- **SCENE_REF** - Canvas will hold references for scene sources
- **EPHEMERAL** - Indicates this canvas is not supposed to be saved

Additionally, the following preset combinations of flags are defined:

- **PROGRAM** which equals `ACTIVATE | MIX_AUDIO | SCENE_REF`
- **PREVIEW** which equals `EPHEMERAL`
- **DEVICE** which equals `ACTIVATE | EPHEMERAL`

General Canvas Functions
------------------------

.. function:: obs_canvas_t *obs_get_main_canvas()

   Get a strong reference to the main OBS canvas.

---------------------

.. function:: obs_canvas_t *obs_canvas_create(const char *name, struct obs_video_info *ovi, uint32_t flags)

   Creates a new canvas.

   :param name: Name, will be deduplicated if necessary
   :param ovi: Video configuration to use for this canvas's video output
   :param flags: Canvas flags
   :return: Canvas object

---------------------

.. function:: obs_canvas_t *obs_canvas_create_private(const char *name, struct obs_video_info *ovi, uint32_t flags)

   Creates a new private canvas.

   :param name: Name, will **not** be deduplicated
   :param ovi: Video configuration to use for this canvas's video output
   :param flags: Canvas flags
   :return: Canvas object

---------------------

.. function:: void obs_canvas_remove(obs_canvas_t *canvas)

   Signal that references to canvas should be released and mark the canvas as removed.

---------------------

.. function:: bool obs_canvas_removed(obs_canvas_t *canvas)

   Returns if a canvas is marked as removed (i.e., should no longer be used).

---------------------

.. function:: void obs_canvas_set_name(obs_canvas_t *canvas, const char *name)

   Set canvas name

---------------------

.. function:: const char *obs_canvas_get_name(const obs_canvas_t *canvas)

   Get canvas name

---------------------

.. function:: const char *obs_canvas_get_uuid(const obs_canvas_t *canvas)

   Get canvas UUID

---------------------

.. function:: uint32_t obs_canvas_get_flags(const obs_canvas_t *canvas)

   Gets flags set on a canvas

---------------------

Saving/Loading Functions
------------------------

.. function:: obs_data_t *obs_save_canvas(obs_canvas_t *source)

   Saves a canvas to settings data

---------------------

.. function:: obs_canvas_t *obs_load_canvas(obs_data_t *data)

   Loads a canvas from settings data

---------------------

Reference Counting Functions
----------------------------

.. function:: obs_canvas_t *obs_canvas_get_ref(obs_canvas_t *canvas)

   Add strong reference to a canvas

---------------------

.. function:: void obs_canvas_release(obs_canvas_t *canvas)

   Release strong reference

---------------------

.. function:: void obs_weak_canvas_addref(obs_weak_canvas_t *weak)

   Add weak reference

---------------------

.. function:: void obs_weak_canvas_release(obs_weak_canvas_t *weak)

   Release weak reference

---------------------

.. function:: obs_weak_canvas_t *obs_canvas_get_weak_canvas(obs_canvas_t *canvas)

   Get weak reference from strong reference

---------------------

.. function:: obs_canvas_t *obs_weak_canvas_get_canvas(obs_weak_canvas_t *weak)

   Get strong reference from weak reference

---------------------

Canvas Channel Functions
------------------------

.. function:: void obs_canvas_set_channel(obs_canvas_t *canvas, uint32_t channel, obs_source_t *source)

   Sets the source to be used for a canvas channel.

---------------------

.. function:: obs_source_t *obs_canvas_get_channel(obs_canvas_t *canvas, uint32_t channel)

   Gets the source currently in use for a canvas channel.

---------------------

Canvas Source List Functions
----------------------------

.. function:: obs_scene_t *obs_canvas_scene_create(obs_canvas_t *canvas, const char *name)

   Create scene attached to a canvas.

---------------------

.. function:: void obs_canvas_scene_remove(obs_scene_t *scene)

   Remove a scene from a canvas.

---------------------

.. function:: void obs_canvas_move_scene(obs_scene_t *scene, obs_canvas_t *dst)

   Move scene to another canvas, detaching it from the previous one and deduplicating the name if needed.

---------------------

.. function:: void obs_canvas_enum_scenes(obs_canvas_t *canvas, bool (*enum_proc)(void *, obs_source_t *), void *param)

   Enumerates scenes belonging to a canvas.

   Callback function returns true to continue enumeration, or false to end enumeration.

---------------------

.. function:: obs_source_t *obs_canvas_get_source_by_name(const char *name)

   Gets a canvas source by its name.
  
   Increments the source reference counter, use
   :c:func:`obs_source_release()` to release it when complete.

---------------------

.. function:: obs_scene_t *obs_canvas_get_scene_by_name(const char *name)

   Gets a canvas scene by its name.
  
   Increments the source reference counter, use
   :c:func:`obs_scene_release()` to release it when complete.

---------------------

Canvas Video Functions
----------------------

.. function:: bool obs_canvas_reset_video(obs_canvas_t *canvas, struct obs_video_info *ovi)

   Reset a canvas's video configuration.
   
   Note that the frame rate property is ignored and the global rendering frame rate is used instead.

---------------------

.. function:: bool obs_canvas_has_video(obs_canvas_t *canvas)

   Returns true if the canvas video is configured.

---------------------

.. function:: video_t *obs_canvas_get_video(const obs_canvas_t *canvas)

   Get canvas video output

---------------------

.. function:: bool obs_canvas_get_video_info(const obs_canvas_t *canvas, struct obs_video_info *ovi)

   Get canvas video info (if any)

---------------------

.. function:: void obs_canvas_render(obs_canvas_t *canvas)

   Render the canvas's view. Must be called on the graphics thread.

---------------------
