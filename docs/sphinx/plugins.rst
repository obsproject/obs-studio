Plugins
=======
Almost all custom functionality is added through plugin modules, which
are typically dynamic libraries or scripts.  The ability to capture
and/or output audio/video, make a recording, output to an RTMP stream,
encode in x264 are all examples of things that are accomplished via
plugin modules.

Plugins can implement sources, outputs, encoders, and services.


Plugin Module Headers
---------------------
These are some notable headers commonly used by plugins:

- `libobs/obs-module.h`_ -- The primary header used for creating plugin
  modules.  This file automatically includes the following files: 

  - `libobs/obs.h`_ -- The main libobs header.  This file automatically
    includes the following files:

    - `libobs/obs-source.h`_ -- Used for implementing sources in plugin
      modules

    - `libobs/obs-output.h`_ -- Used for implementing outputs in plugin
      modules

    - `libobs/obs-encoder.h`_ -- Used for implementing encoders in
      plugin modules

    - `libobs/obs-service.h`_ -- Used for implementing services in
      plugin modules

    - `libobs/obs-data.h`_ -- Used for managing settings for libobs
      objects

    - `libobs/obs-properties.h`_ -- Used for generating properties for
      libobs objects

    - `libobs/graphics/graphics.h`_ -- Used for graphics rendering


Common Directory Structure and CMakeLists.txt
---------------------------------------------
The common way source files are organized is to have one file for plugin
initialization, and then specific files for each individual object
you're implementing.  For example, if you were to create a plugin called
'my-plugin', you'd have something like my-plugin.c where plugin
initialization is done, my-source.c for the definition of a custom
source, my-output.c for the definition of a custom output, etc.  (This
is not a rule of course)

This is an example of a common directory structure for a native plugin
module::

  my-plugin/data/locale/en-US.ini
  my-plugin/CMakeLists.txt
  my-plugin/my-plugin.c
  my-plugin/my-source.c
  my-plugin/my-output.c
  my-plugin/my-encoder.c
  my-plugin/my-service.c

This would be an example of a common CMakeLists.txt file associated with
these files::

  # my-plugin/CMakeLists.txt

  project(my-plugin)

  set(my-plugin_SOURCES
        my-plugin.c
        my-source.c
        my-output.c
        my-encoder.c
        my-service.c)

  add_library(my-plugin MODULE
        ${my-plugin_SOURCES})
  target_link_libraries(my-plugin
        libobs)

  install_obs_plugin_with_data(my-plugin data)


Native Plugin Initialization
----------------------------
To create a native plugin module, you will need to include the
`libobs/obs-module.h`_ header, use :c:func:`OBS_DECLARE_MODULE()` macro,
then create a definition of the function :c:func:`obs_module_load()`.
In your :c:func:`obs_module_load()` function, you then register any of
your custom sources, outputs, encoders, or services.  See the
:doc:`reference-modules` for more information.

The following is an example of my-plugin.c, which would register one
object of each type:

.. code:: cpp

  /* my-plugin.c */
  #include <obs-module.h>

  /* Defines common functions (required) */
  OBS_DECLARE_MODULE()

  /* Implements common ini-based locale (optional) */
  OBS_MODULE_USE_DEFAULT_LOCALE("my-plugin", "en-US")

  extern struct obs_source_info  my_source;  /* Defined in my-source.c  */
  extern struct obs_output_info  my_output;  /* Defined in my-output.c  */
  extern struct obs_encoder_info my_encoder; /* Defined in my-encoder.c */
  extern struct obs_service_info my_service; /* Defined in my-service.c */

  bool obs_module_load(void)
  {
          obs_register_source(&my_source);
          obs_register_output(&my_output);
          obs_register_encoder(&my_encoder);
          obs_register_service(&my_service);
          return true;
  }


.. _plugins_sources:

Sources
-------
Sources are used to render video and/or audio on stream.  Things such as
capturing displays/games/audio, playing a video, showing an image, or
playing audio.  Sources can also be used to implement audio and video
filters as well as transitions.  The `libobs/obs-source.h`_ file is the
dedicated header for implementing sources.  See the
:doc:`reference-sources` for more information.

For example, to implement a source object, you need to define an
:c:type:`obs_source_info` structure and fill it out with information and
callbacks related to your source:

.. code:: cpp

  /* my-source.c */

  [...]

  struct obs_source_info my_source {
          .id           = "my_source",
          .type         = OBS_SOURCE_TYPE_INPUT,
          .output_flags = OBS_SOURCE_VIDEO,
          .get_name     = my_source_name,
          .create       = my_source_create,
          .destroy      = my_source_destroy,
          .update       = my_source_update,
          .video_render = my_source_render,
          .get_width    = my_source_width,
          .get_height   = my_source_height
  };

Then, in my-plugin.c, you would call :c:func:`obs_register_source()` in
:c:func:`obs_module_load()` to register the source with libobs.

.. code:: cpp

  /* my-plugin.c */

  [...]
  
  extern struct obs_source_info  my_source;  /* Defined in my-source.c  */

  bool obs_module_load(void)
  {
          obs_register_source(&my_source);

          [...]

          return true;
  }

Some simple examples of sources:

- Synchronous video source: The `image source <https://github.com/jp9000/obs-studio/blob/master/plugins/image-source/image-source.c>`_
- Asynchronous video source: The `random texture test source <https://github.com/jp9000/obs-studio/blob/master/test/test-input/test-random.c>`_
- Audio source: The `sine wave test source <https://github.com/jp9000/obs-studio/blob/master/test/test-input/test-sinewave.c>`_
- Video filter: The `test video filter <https://github.com/jp9000/obs-studio/blob/master/test/test-input/test-filter.c>`_
- Audio filter: The `gain audio filter <https://github.com/jp9000/obs-studio/blob/master/plugins/obs-filters/gain-filter.c>`_


.. _plugins_outputs:

Outputs
-------
Outputs allow the ability to output the currently rendering audio/video.
Streaming and recording are two common examples of outputs, but not the
only types of outputs.  Outputs can receive the raw data or receive
encoded data.  The `libobs/obs-output.h`_ file is the dedicated header
for implementing outputs.  See the :doc:`reference-outputs` for more
information.

For example, to implement an output object, you need to define an
:c:type:`obs_output_info` structure and fill it out with information and
callbacks related to your output:

.. code:: cpp

  /* my-output.c */

  [...]

  struct obs_output_info my_output {
          .id                   = "my_output",
          .flags                = OBS_OUTPUT_AV | OBS_OUTPUT_ENCODED,
          .get_name             = my_output_name,
          .create               = my_output_create,
          .destroy              = my_output_destroy,
          .start                = my_output_start,
          .stop                 = my_output_stop,
          .encoded_packet       = my_output_data,
          .get_total_bytes      = my_output_total_bytes,
          .encoded_video_codecs = "h264",
          .encoded_audio_codecs = "aac"
  };

Then, in my-plugin.c, you would call :c:func:`obs_register_output()` in
:c:func:`obs_module_load()` to register the output with libobs.

.. code:: cpp

  /* my-plugin.c */

  [...]
  
  extern struct obs_output_info  my_output;  /* Defined in my-output.c  */

  bool obs_module_load(void)
  {
          obs_register_output(&my_output);

          [...]

          return true;
  }

Some examples of outputs:

- Encoded video/audio outputs:

  - The `FLV output <https://github.com/jp9000/obs-studio/blob/master/plugins/obs-outputs/flv-output.c>`_
  - The `FFmpeg muxer output <https://github.com/jp9000/obs-studio/blob/master/plugins/obs-ffmpeg/obs-ffmpeg-mux.c>`_
  - The `RTMP stream output <https://github.com/jp9000/obs-studio/blob/master/plugins/obs-outputs/rtmp-stream.c>`_

- Raw video/audio outputs:

  - The `FFmpeg output <https://github.com/jp9000/obs-studio/blob/master/plugins/obs-ffmpeg/obs-ffmpeg-output.c>`_


.. _plugins_encoders:

Encoders
--------
Encoders are OBS-specific implementations of video/audio encoders, which
are used with outputs that use encoders.  x264, NVENC, Quicksync are
examples of encoder implementations.  The `libobs/obs-encoder.h`_ file
is the dedicated header for implementing encoders.  See the
:doc:`reference-encoders` for more information.

For example, to implement an encoder object, you need to define an
:c:type:`obs_encoder_info` structure and fill it out with information
and callbacks related to your encoder:

.. code:: cpp

  /* my-encoder.c */

  [...]

  struct obs_encoder_info my_encoder_encoder = {
          .id             = "my_encoder",
          .type           = OBS_ENCODER_VIDEO,
          .codec          = "h264",
          .get_name       = my_encoder_name,
          .create         = my_encoder_create,
          .destroy        = my_encoder_destroy,
          .encode         = my_encoder_encode,
          .update         = my_encoder_update,
          .get_extra_data = my_encoder_extra_data,
          .get_sei_data   = my_encoder_sei,
          .get_video_info = my_encoder_video_info
  };

Then, in my-plugin.c, you would call :c:func:`obs_register_encoder()`
in :c:func:`obs_module_load()` to register the encoder with libobs.

.. code:: cpp

  /* my-plugin.c */

  [...]
  
  extern struct obs_encoder_info my_encoder; /* Defined in my-encoder.c */

  bool obs_module_load(void)
  {
          obs_register_encoder(&my_encoder);

          [...]

          return true;
  }

**IMPORTANT NOTE:** Encoder settings currently have a few expected
common setting values that should have a specific naming convention:

  - **"bitrate"** - This value should be used for both video and audio
    encoders: bitrate, in kilobits.
  - **"rate_control"** - This is a setting used for video encoders.
    It's generally expected to have at least a "CBR" rate control.
    Other common rate controls are "VBR", "CQP".
  - **"keyint_sec"** - For video encoders, sets the keyframe interval
    value, in seconds, or closest possible approximation.  *(Author's
    note: This should have have been "keyint", in frames.)*

Examples of encoders:

- Video encoders:

  - The `x264 encoder <https://github.com/jp9000/obs-studio/tree/master/plugins/obs-x264>`_
  - The `FFmpeg NVENC encoder <https://github.com/jp9000/obs-studio/blob/master/plugins/obs-ffmpeg/obs-ffmpeg-nvenc.c>`_
  - The `Quicksync encoder <https://github.com/jp9000/obs-studio/tree/master/plugins/obs-qsv11>`_

- Audio encoders:

  - The `FFmpeg AAC/Opus encoder <https://github.com/jp9000/obs-studio/blob/master/plugins/obs-ffmpeg/obs-ffmpeg-audio-encoders.c>`_


.. _plugins_services:

Services
--------
Services are custom implementations of streaming services, which are
used with outputs that stream.  For example, you could have a custom
implementation for streaming to Twitch, and another for YouTube to allow
the ability to log in and use their APIs to do things such as get the
RTMP servers or control the channel.  The `libobs/obs-service.h`_ file
is the dedicated header for implementing services.  See the
:doc:`reference-services` for more information.

*(Author's note: the service API is incomplete as of this writing)*

For example, to implement a service object, you need to define an
:c:type:`obs_service_info` structure and fill it out with information
and callbacks related to your service:

.. code:: cpp

  /* my-service.c */

  [...]

  struct obs_service_info my_service_service = {
          .id       = "my_service",
          .get_name = my_service_name,
          .create   = my_service_create,
          .destroy  = my_service_destroy,
          .encode   = my_service_encode,
          .update   = my_service_update,
          .get_url  = my_service_url,
          .get_key  = my_service_key
  };

Then, in my-plugin.c, you would call :c:func:`obs_register_service()` in
:c:func:`obs_module_load()` to register the service with libobs.

.. code:: cpp

  /* my-plugin.c */

  [...]
  
  extern struct obs_service_info my_service; /* Defined in my-service.c */

  bool obs_module_load(void)
  {
          obs_register_service(&my_service);

          [...]

          return true;
  }

The only two existing services objects are the "common RTMP services"
and "custom RTMP service" objects in `plugins/rtmp-services
<https://github.com/jp9000/obs-studio/tree/master/plugins/rtmp-services>`_


Settings
--------
Settings (see `libobs/obs-data.h`_) are used to get or set settings data
typically associated with libobs objects, and can then be saved and
loaded via Json text.  See the :doc:`reference-settings` for more
information.

The *obs_data_t* is the equivalent of a Json object, where it's a string
table of sub-objects, and the *obs_data_array_t* is similarly used to
store an array of *obs_data_t* objects, similar to Json arrays (though
not quite identical).

To create an *obs_data_t* or *obs_data_array_t* object, you'd call the
:c:func:`obs_data_create()` or :c:func:`obs_data_array_create()`
functions.  *obs_data_t* and *obs_data_array_t* objects are reference
counted, so when you are finished with the object, call
:c:func:`obs_data_release()` or :c:func:`obs_data_array_release()` to
release those references.  Any time an *obs_data_t* or
*obs_data_array_t* object is returned by a function, their references
are incremented, so you must release those references each time.

To set values for an *obs_data_t* object, you'd use one of the following
functions:

.. code:: cpp

  /* Set functions */
  EXPORT void obs_data_set_string(obs_data_t *data, const char *name, const char *val);
  EXPORT void obs_data_set_int(obs_data_t *data, const char *name, long long val);
  EXPORT void obs_data_set_double(obs_data_t *data, const char *name, double val);
  EXPORT void obs_data_set_bool(obs_data_t *data, const char *name, bool val);
  EXPORT void obs_data_set_obj(obs_data_t *data, const char *name, obs_data_t *obj);
  EXPORT void obs_data_set_array(obs_data_t *data, const char *name, obs_data_array_t *array);

Similarly, to get a value from an *obs_data_t* object, you'd use one of
the following functions:

.. code:: cpp

  /* Get functions */
  EXPORT const char *obs_data_get_string(obs_data_t *data, const char *name);
  EXPORT long long obs_data_get_int(obs_data_t *data, const char *name);
  EXPORT double obs_data_get_double(obs_data_t *data, const char *name);
  EXPORT bool obs_data_get_bool(obs_data_t *data, const char *name);
  EXPORT obs_data_t *obs_data_get_obj(obs_data_t *data, const char *name);
  EXPORT obs_data_array_t *obs_data_get_array(obs_data_t *data, const char *name);

Unlike typical Json data objects, the *obs_data_t* object can also set
default values.  This allows the ability to control what is returned if
there is no value assigned to a specific string in an *obs_data_t*
object when that data is loaded from a Json string or Json file.  Each
libobs object also has a *get_defaults* callback which allows setting
the default settings for the object on creation.

These functions control the default values are as follows:

.. code:: cpp

  /* Default value functions. */
  EXPORT void obs_data_set_default_string(obs_data_t *data, const char *name, const char *val);
  EXPORT void obs_data_set_default_int(obs_data_t *data, const char *name, long long val);
  EXPORT void obs_data_set_default_double(obs_data_t *data, const char *name, double val);
  EXPORT void obs_data_set_default_bool(obs_data_t *data, const char *name, bool val);
  EXPORT void obs_data_set_default_obj(obs_data_t *data, const char *name, obs_data_t *obj);


Properties
----------
Properties (see `libobs/obs-properties.h`_) are used to automatically
generate user interface to modify settings for a libobs object (if
desired).  Each libobs object has a *get_properties* callback which is
used to generate properties.  The properties API defines specific
properties that are linked to the object's settings, and the front-end
uses those properties to generate widgets in order to allow the user to
modify the settings.  For example, if you had a boolean setting, you
would use :c:func:`obs_properties_add_bool()` to allow the user to be
able to change that setting.  See the :doc:`reference-properties` for
more information.

An example of this:

.. code:: cpp

   static obs_properties_t *my_source_properties(void *data)
   {
           obs_properties_t *ppts = obs_properties_create();
           obs_properties_add_bool(ppts, "my_bool",
                           obs_module_text("MyBool"));
           UNUSED_PARAMETER(data);
           return ppts;
   }

   [...]

   struct obs_source_info my_source {
           .get_properties = my_source_properties,
           [...]
   };

The *data* parameter is the object's data if the object is present.
Typically this is unused and probably shouldn't be used if possible.  It
can be null if the properties are retrieved without an object associated
with it.

Properties can also be modified depending on what settings are shown.
For example, you can mark certain properties as disabled or invisible
depending on what a particular setting is set to using the
:c:func:`obs_property_set_modified_callback()` function.

For example, if you wanted boolean property A to hide text property B:

.. code:: cpp

   static bool setting_a_modified(obs_properties_t *ppts,
                   obs_property_t *p, obs_data_t *settings)
   {
           bool enabled = obs_data_get_bool(settings, "setting_a");
           p = obs_properties_get(ppts, "setting_b");
           obs_property_set_enabled(p, enabled);

           /* return true to update property widgets, false
              otherwise */
           return true;
   }

   [...]

   static obs_properties_t *my_source_properties(void *data)
   {
           obs_properties_t *ppts = obs_properties_create();
           obs_property_t *p;

           p = obs_properties_add_bool(ppts, "setting_a",
                           obs_module_text("SettingA"));
           obs_property_set_modified_callback(p, setting_a_modified);

           obs_properties_add_text(ppts, "setting_b",
                           obs_module_text("SettingB"),
                           OBS_TEXT_DEFAULT);
           return ppts;
   }


Localization
------------
Typically, most plugins bundled with OBS Studio will use a simple
ini-file localization method, where each file is a different language.
When using this method, the :c:func:`OBS_MODULE_USE_DEFAULT_LOCALE()`
macro is used which will automatically load/destroy the locale data with
no extra effort on part of the plugin.  Then the
:c:func:`obs_module_text()` function (which is automatically declared as
an extern by `libobs/obs-module.h`_) is used when text lookup is needed.

There are two exports the module used to load/destroy locale: the
:c:func:`obs_module_set_locale()` export, and the
:c:func:`obs_module_free_locale()` export.  The
:c:func:`obs_module_set_locale()` export is called by libobs to set the
current language, and then the :c:func:`obs_module_free_locale()` export
is called by libobs on destruction of the module.  If you wish to
implement a custom locale implementation for your plugin, you'd want to
define these exports along with the :c:func:`obs_module_text()` extern
yourself instead of relying on the
:c:func:`OBS_MODULE_USE_DEFAULT_LOCALE()` macro.


.. ---------------------------------------------------------------------------

.. _libobs/obs-module.h: https://github.com/jp9000/obs-studio/blob/master/libobs/obs-module.h
.. _libobs/obs.h: https://github.com/jp9000/obs-studio/blob/master/libobs/obs.h
.. _libobs/obs-source.h: https://github.com/jp9000/obs-studio/blob/master/libobs/obs-source.h
.. _libobs/obs-output.h: https://github.com/jp9000/obs-studio/blob/master/libobs/obs-output.h
.. _libobs/obs-encoder.h: https://github.com/jp9000/obs-studio/blob/master/libobs/obs-encoder.h
.. _libobs/obs-service.h: https://github.com/jp9000/obs-studio/blob/master/libobs/obs-service.h
.. _libobs/obs-data.h: https://github.com/jp9000/obs-studio/blob/master/libobs/obs-data.h
.. _libobs/obs-properties.h: https://github.com/jp9000/obs-studio/blob/master/libobs/obs-properties.h
.. _libobs/graphics/graphics.h: https://github.com/jp9000/obs-studio/blob/master/libobs/graphics/graphics.h
