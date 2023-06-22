Service API Reference (obs_service_t)
=====================================

Services are custom implementations of streaming services, which are
used with outputs that stream.  For example, you could have a custom
implementation for streaming to Twitch, and another for YouTube to allow
the ability to log in and use their APIs to do things such as get the
RTMP servers or control the channel.  The `libobs/obs-service.h`_ file
is the dedicated header for implementing services.

*(Author's note: the service API is incomplete as of this writing)*

.. type:: obs_service_t

   A reference-counted service object.

.. type:: obs_weak_service_t

   A weak reference to a service object.

.. code:: cpp

   #include <obs.h>


Service Definition Structure
----------------------------

.. struct:: obs_service_info

   Service definition structure.

.. member:: const char *obs_service_info.id

   Unique string identifier for the service (required).

.. member:: const char *(*obs_service_info.get_name)(void *type_data)

   Get the translated name of the service type.

   :param  type_data:  The type_data variable of this structure
   :return:            The translated name of the service type

.. member:: void *(*obs_service_info.create)(obs_data_t *settings, obs_service_t *service)

   Creates the implementation data for the service.

   :param  settings: Settings to initialize the service with
   :param  service:   Source that this data is associated with
   :return:          The implementation data associated with this service

.. member:: void (*obs_service_info.destroy)(void *data)

   Destroys the implementation data for the service.

.. member:: void (*obs_service_info.get_defaults)(obs_data_t *settings)
            void (*obs_service_info.get_defaults2)(void *type_data, obs_data_t *settings)

   Sets the default settings for this service.

   :param  settings:  Default settings.  Call obs_data_set_default*
                      functions on this object to set default setting
                      values

.. member:: obs_properties_t *(*obs_service_info.get_properties)(void *data)
            obs_properties_t *(*obs_service_info.get_properties2)(void *data, void *type_data)

   Gets the property information of this service.

   (Optional)

   :return: The properties of the service

.. member:: void (*obs_service_info.update)(void *data, obs_data_t *settings)

   Updates the settings for this service.

   (Optional)

   :param settings: New settings for this service

.. member:: bool (*obs_service_info.initialize)(void *data, obs_output_t *output)

   Called when getting ready to start up an output, before the encoders
   and output are initialized.

   (Optional)

   :param  output: Output context to use this service with
   :return:        *true* to allow the output to start up,
                   *false* to prevent output from starting up

.. member:: const char *(*obs_service_info.get_url)(void *data)

   :return: The stream URL

.. member:: const char *(*obs_service_info.get_key)(void *data)

   :return: The stream key

.. member:: const char *(*obs_service_info.get_username)(void *data)

   (Optional)

   :return: The username

.. member:: const char *(*obs_service_info.get_password)(void *data)

   (Optional)

   :return: The password

.. member:: void (*obs_service_info.apply_encoder_settings)(void *data, obs_data_t *video_encoder_settings, obs_data_t *audio_encoder_settings)

   This function is called to apply custom encoder settings specific to
   this service.  For example, if a service requires a specific keyframe
   interval, or has a bitrate limit, the settings for the video and
   audio encoders can be optionally modified if the front-end optionally
   calls :c:func:`obs_service_apply_encoder_settings()`.

   (Optional)

   :param video_encoder_settings: The audio encoder settings to change
   :param audio_encoder_settings: The video encoder settings to change

.. member:: void *obs_service_info.type_data
            void (*obs_service_info.free_type_data)(void *type_data)

   Private data associated with this entry.  Note that this is not the
   same as the implementation data; this is used to differentiate
   between two different types if the same callbacks are used for more
   than one different type.

   (Optional)

.. member:: const char *(*obs_service_info.get_output_type)(void *data)

   (Optional)

   :return: The output type that should be preferred with this service

.. member:: const char **(*get_supported_video_codecs)(void *data)

   (Optional)

   :return: A string pointer array of the supported video codecs, should
            be stored by the plugin so the caller does not need to free
            the data manually (typically best to use strlist_split to
            generate this)

.. member:: const char **(*get_supported_audio_codecs)(void *data)

   (Optional)

   :return: A string pointer array of the supported audio codecs, should
            be stored by the plugin so the caller does not need to free
            the data manually (typically best to use strlist_split to
            generate this)

   .. versionadded:: 29.1

.. member:: const char *(*obs_service_info.get_protocol)(void *data)

   :return: The protocol used by the service

   .. versionadded:: 29.1

.. member:: const char *(*obs_service_info.get_connect_info)(void *data, uint32_t type)

   Output a service connection info related to a given type value:

   - **OBS_SERVICE_CONNECT_INFO_SERVER_URL** - Server URL (0)

   - **OBS_SERVICE_CONNECT_INFO_STREAM_ID** - Stream ID (2)

   - **OBS_SERVICE_CONNECT_INFO_STREAM_KEY** - Stream key (alias of **OBS_SERVICE_CONNECT_INFO_STREAM_ID**)

   - **OBS_SERVICE_CONNECT_INFO_USERNAME** - Username (4)

   - **OBS_SERVICE_CONNECT_INFO_PASSWORD** - Password (6)

   - **OBS_SERVICE_CONNECT_INFO_ENCRYPT_PASSPHRASE** - Encryption passphrase (8)

   - Odd values as types are reserved for third-party protocols

   Depending on the protocol, the service will have to provide information
   to the output to be able to connect.

   Irrelevant or unused types can return `NULL`.

   .. versionadded:: 29.1

.. member:: bool (*obs_service_info.can_try_to_connect)(void *data)

   (Optional)

   :return: If the service has all the needed connection info to be
            able to connect.

   NOTE: If not set, :c:func:`obs_service_can_try_to_connect()`
   returns *true* by default.

   .. versionadded:: 29.1

.. member:: enum obs_service_audio_track_cap (*get_audio_track_cap)(void *data)

   Return the audio track capability of the service:

   - **OBS_SERVICE_AUDIO_SINGLE_TRACK** - Only a single audio track is used by the service

   - **OBS_SERVICE_AUDIO_ARCHIVE_TRACK** - A second audio track is accepted and
     is meant to become the archive/VOD audio

   - **OBS_SERVICE_AUDIO_MULTI_TRACK** - Supports multiple audio tracks

.. member:: uint32_t obs_service_info.flags

   Service feature flags (Optional).

   A bitwise OR combination of one or more of the following values:

   - **OBS_SERVICE_DEPRECATED** - Service is deprecrated.

   - **OBS_SERVICE_INTERNAL** - Service is not user-facing in a UI context.

   - **OBS_SERVICE_UNCOMMON** - Service can be hidden behind an option in a UI
     context.

.. member:: const char *obs_service_info.supported_protocols

   This variable specifies which protocol are supported by the service,
   separated by semicolon.

.. member:: bool (*obs_service_info.can_bandwidth_test)(void *data)

   (Optional)

   :return: If the service can do bandwith test or not

.. member:: void (*obs_service_info.enable_bandwidth_test)(void *data, bool enabled)

   Enable/disable the bandwith test of the service.
   (Optional)

.. member:: bool (*obs_service_info.bandwidth_test_enabled)(void *data)

   (Optional)

   :return: If the bandwith test is enabled or not

General Service Functions
-------------------------

.. function:: void obs_register_service(struct obs_service_info *info)

   Registers a service type.  Typically used in
   :c:func:`obs_module_load()` or in the program's initialization phase.

---------------------

.. function:: const char *obs_service_get_display_name(const char *id)

   Calls the :c:member:`obs_service_info.get_name` callback to get the
   translated display name of a service type.

   :param    id:            The service type string identifier
   :return:                 The translated display name of a service type

---------------------

.. function:: obs_service_t *obs_service_create(const char *id, const char *name, obs_data_t *settings, obs_data_t *hotkey_data)

   Creates a service with the specified settings.

   The "service" context is used for encoding video/audio data.  Use
   obs_service_release to release it.

   :param   id:             The service type string identifier
   :param   name:           The desired name of the service.  If this is
                            not unique, it will be made to be unique
   :param   settings:       The settings for the service, or *NULL* if
                            none
   :param   hotkey_data:    Saved hotkey data for the service, or *NULL*
                            if none
   :return:                 A reference to the newly created service, or
                            *NULL* if failed

---------------------

.. function:: void obs_service_addref(obs_service_t *service)

   Adds a reference to a service.

.. deprecated:: 27.2.0
   Use :c:func:`obs_service_get_ref()` instead.

---------------------

.. function:: obs_service_t *obs_service_get_ref(obs_service_t *service)

   Returns an incremented reference if still valid, otherwise returns
   *NULL*. Release with :c:func:`obs_service_release()`.

---------------------

.. function:: void obs_service_release(obs_service_t *service)

   Releases a reference to a service.  When the last reference is
   released, the service is destroyed.

---------------------

.. function:: obs_weak_service_t *obs_service_get_weak_service(obs_service_t *service)
              obs_service_t *obs_weak_service_get_service(obs_weak_service_t *weak)

   These functions are used to get a weak reference from a strong service
   reference, or a strong service reference from a weak reference.  If
   the service is destroyed, *obs_weak_service_get_service* will return
   *NULL*.

---------------------

.. function:: void obs_weak_service_addref(obs_weak_service_t *weak)
              void obs_weak_service_release(obs_weak_service_t *weak)

   Adds/releases a weak reference to a service.

---------------------

.. function:: const char *obs_service_get_name(const obs_service_t *service)

   :return: The name of the service

---------------------

.. function:: obs_data_t *obs_service_defaults(const char *id)

   :return: An incremented reference to the service's default settings.
            Release with :c:func:`obs_data_release()`.

---------------------

.. function:: obs_properties_t *obs_service_properties(const obs_service_t *service)
              obs_properties_t *obs_get_service_properties(const char *id)

   Use these functions to get the properties of a service or service
   type.  Properties are optionally used (if desired) to automatically
   generate user interface widgets to allow users to update settings.

   :return: The properties list for a specific existing service.  Free
            with :c:func:`obs_properties_destroy()`

---------------------

.. function:: obs_data_t *obs_service_get_settings(const obs_service_t *service)

   :return: An incremented reference to the service's settings. Release with
            :c:func:`obs_data_release()`.

---------------------

.. function:: void obs_service_update(obs_service_t *service, obs_data_t *settings)

   Updates the settings for this service context.

---------------------

.. function:: const char *obs_service_get_url(const obs_service_t *service)

  :return: The URL currently used for this service

.. deprecated:: 29.1.0
   Use :c:func:`obs_service_get_connect_info()` instead.

---------------------

.. function:: const char *obs_service_get_key(const obs_service_t *service)

  :return: Stream key (if any) currently used for this service

.. deprecated:: 29.1.0
   Use :c:func:`obs_service_get_connect_info()` instead.

---------------------

.. function:: const char *obs_service_get_username(const obs_service_t *service)

   :return: User name (if any) currently used for this service

.. deprecated:: 29.1.0
   Use :c:func:`obs_service_get_connect_info()` instead.

---------------------

.. function:: const char *obs_service_get_password(const obs_service_t *service)

   :return: Password (if any) currently used for this service

.. deprecated:: 29.1.0
   Use :c:func:`obs_service_get_connect_info()` instead.

---------------------

.. function:: void obs_service_apply_encoder_settings(obs_service_t *service, obs_data_t *video_encoder_settings, obs_data_t *audio_encoder_settings)

   Applies service-specific video encoder settings.

   :param  video_encoder_settings: Video encoder settings.  Can be *NULL*
   :param  audio_encoder_settings: Audio encoder settings.  Can be *NULL*

---------------------

.. function:: const char **obs_service_get_supported_video_codecs(const obs_service_t *service)

   :return: An array of string pointers containing the supported video
            codecs for the service, terminated with a *NULL* pointer.
            Does not need to be freed

---------------------

.. function:: const char **obs_service_get_supported_audio_codecs(const obs_service_t *service)

   :return: An array of string pointers containing the supported audio
            codecs for the service, terminated with a *NULL* pointer.
            Does not need to be freed

   .. versionadded:: 29.1

---------------------

.. function:: const char *obs_service_get_protocol(const obs_service_t *service)

   :return: Protocol currently used for this service

   .. versionadded:: 29.1

---------------------

.. function:: const char *obs_service_get_preferred_output_type(const obs_service_t *service)

   :return: The output type that should be preferred with this service

   .. versionadded:: 29.1

---------------------

.. function:: const char *obs_service_get_connect_info(const obs_service_t *service, uint32_t type)

   :param type: Check :c:member:`obs_service_info.get_connect_info` for
                type values.
   :return: Connection info related to the type value.

   .. versionadded:: 29.1

---------------------

.. function:: bool obs_service_can_try_to_connect(const obs_service_t *service)

   :return: If the service has all the needed connection info to be
            able to connect. Returns `true` if
            :c:member:`obs_service_info.can_try_to_connect` is not set.

   .. versionadded:: 29.1

---------------------

.. function:: enum obs_service_audio_track_cap obs_service_get_audio_track_cap(const obs_service_t *service)

   :return: The audio track capability of the service

---------------------

.. function:: uint32_t obs_get_service_flags(const char *id)
              uint32_t obs_service_get_flags(const obs_service_t *service)

   :return: The service feature flags

---------------------

.. function:: const char *obs_get_service_supported_protocols(const char *id)

   :return: Supported protocol of the service, separated by semicolon

---------------------

.. function:: bool obs_service_can_bandwidth_test(const obs_service_t *service)

   :return: If the service can do bandwidth test

---------------------

.. function:: void obs_service_enable_bandwidth_test(const obs_service_t *service, bool enabled)

   Enable the bandwidth test has this capability

---------------------

.. function:: bool obs_service_bandwidth_test_enabled(const obs_service_t *service)

   :return: If the service has bandwidth test enabled

.. ---------------------------------------------------------------------------

.. _libobs/obs-service.h: https://github.com/obsproject/obs-studio/blob/master/libobs/obs-service.h
