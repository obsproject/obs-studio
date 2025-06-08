Properties API Reference (obs_properties_t)
===========================================

Properties are used to enumerate available settings for a libobs object.
Typically this is used to automatically generate user interface widgets,
though can be used to enumerate available and/or valid values for
specific settings as well.

.. type:: obs_properties_t

   Properties object (not reference counted).

.. type:: obs_property_t

   A property object (not reference counted).

.. code:: cpp

   #include <obs.h>


General Functions
-----------------

.. function:: obs_properties_t *obs_properties_create(void)

   :return: A new properties object.

---------------------

.. function:: obs_properties_t *obs_properties_create_param(void *param, void (*destroy)(void *param))

   Creates a new properties object with specific private data *param*
   associated with the object, and is automatically freed with the
   object when the properties are destroyed via the *destroy* function.

   :return: A new properties object.

---------------------

.. function:: void obs_properties_destroy(obs_properties_t *props)

---------------------

.. function:: void obs_properties_set_flags(obs_properties_t *props, uint32_t flags)
              uint32_t obs_properties_get_flags(obs_properties_t *props)

   :param flags: 0 or a bitwise OR combination of one of the following
                 values:
                 
                 - OBS_PROPERTIES_DEFER_UPDATE - A hint that tells the
                   front-end to defers updating the settings until the
                   user has finished editing all properties rather than
                   immediately updating any settings

---------------------

.. function:: void obs_properties_set_param(obs_properties_t *props, void *param, void (*destroy)(void *param))
              void *obs_properties_get_param(obs_properties_t *props)

   Sets custom data associated with this properties object.  If private
   data is already associated with the object, that private data will be
   destroyed before assigning new private data to it.

---------------------

.. function:: void obs_properties_apply_settings(obs_properties_t *props, obs_data_t *settings)

   Applies settings to the properties by calling all the necessary
   modification callbacks

---------------------

.. function:: obs_properties_t *obs_properties_get_parent(obs_properties_t *props)

---------------------

.. function:: void obs_properties_remove_by_name(obs_properties_t *props, const char *property)

   Removes a property from a properties list. Only valid in ``get_properties``,
   ``script_properties`` for scripts, ``modified_callback``, and ``modified_callback2``.
   ``modified_callback`` and ``modified_callback2`` *must* return true so that
   all UI properties are rebuilt. Returning false is undefined behavior.

   :param props:    Properties to remove from.
   :param property: Name of the property to remove.

---------------------


Property Object Functions
-------------------------

.. function:: obs_property_t *obs_properties_add_bool(obs_properties_t *props, const char *name, const char *description)

   Adds a boolean property.

   :param    name:        Setting identifier string
   :param    description: Localized name shown to user
   :return:               The property

---------------------

.. function:: obs_property_t *obs_properties_add_int(obs_properties_t *props, const char *name, const char *description, int min, int max, int step)

   Adds an integer property.

   :param    name:        Setting identifier string
   :param    description: Localized name shown to user
   :param    min:         Minimum value
   :param    max:         Maximum value
   :param    step:        Step value
   :return:               The property

---------------------

.. function:: obs_property_t *obs_properties_add_float(obs_properties_t *props, const char *name, const char *description, double min, double max, double step)

   :param    name:        Setting identifier string
   :param    description: Localized name shown to user
   :param    min:         Minimum value
   :param    max:         Maximum value
   :param    step:        Step value
   :return:               The property

---------------------

.. function:: obs_property_t *obs_properties_add_int_slider(obs_properties_t *props, const char *name, const char *description, int min, int max, int step)

   :param    name:        Setting identifier string
   :param    description: Localized name shown to user
   :param    min:         Minimum value
   :param    max:         Maximum value
   :param    step:        Step value
   :return:               The property

---------------------

.. function:: obs_property_t *obs_properties_add_float_slider(obs_properties_t *props, const char *name, const char *description, double min, double max, double step)

   :param    name:        Setting identifier string
   :param    description: Localized name shown to user
   :param    min:         Minimum value
   :param    max:         Maximum value
   :param    step:        Step value
   :return:               The property

---------------------

.. function:: obs_property_t *obs_properties_add_text(obs_properties_t *props, const char *name, const char *description, enum obs_text_type type)

   :param    name:        Setting identifier string
   :param    description: Localized name shown to user
   :param    type:        Can be one of the following values:

                          - **OBS_TEXT_DEFAULT** - Single line of text
                          - **OBS_TEXT_PASSWORD** - Single line of text (passworded)
                          - **OBS_TEXT_MULTILINE** - Multi-line text
                          - **OBS_TEXT_INFO** - Read-only informative text, behaves differently
                            depending on wether description, string value and long description
                            are set

   :return:               The property

   Important Related Functions:

   - :c:func:`obs_property_text_set_info_type`
   - :c:func:`obs_property_text_set_info_word_wrap`

---------------------

.. function:: obs_property_t *obs_properties_add_path(obs_properties_t *props, const char *name, const char *description, enum obs_path_type type, const char *filter, const char *default_path)

   Adds a 'path' property.  Can be a directory or a file.

   If target is a file path, the filters should be this format, separated by
   double semicolons, and extensions separated by space::

     "Example types 1 and 2 (*.ex1 *.ex2);;Example type 3 (*.ex3)"

   :param    name:         Setting identifier string
   :param    description:  Localized name shown to user
   :param    type:         Can be one of the following values:

                           - **OBS_PATH_FILE** - File (for reading)
                           - **OBS_PATH_FILE_SAVE** - File (for writing)
                           - **OBS_PATH_DIRECTORY** - Directory

   :param    filter:       If type is a file path, then describes the file filter
                           that the user can browse.  Items are separated via
                           double semicolons.  If multiple file types in a
                           filter, separate with space.
   :param    default_path: The default path to start in, or *NULL*
   :return:                The property

---------------------

.. function:: obs_property_t *obs_properties_add_list(obs_properties_t *props, const char *name, const char *description, enum obs_combo_type type, enum obs_combo_format format)

   Adds an integer/string/floating point item list.  This would be
   implemented as a combo box in user interface.

   :param    name:        Setting identifier string
   :param    description: Localized name shown to user
   :param    type:        Can be one of the following values:

                          - **OBS_COMBO_TYPE_EDITABLE** - Can be edited.
                            Only used with string lists.
                          - **OBS_COMBO_TYPE_LIST** - Not editable. Displayed as combo box.
                          - **OBS_COMBO_TYPE_RADIO** - Not editable. Displayed as radio buttons.

                              .. versionadded:: 30.0

   :param    format:      Can be one of the following values:

                          - **OBS_COMBO_FORMAT_INT** - Integer list
                          - **OBS_COMBO_FORMAT_FLOAT** - Floating point
                            list
                          - **OBS_COMBO_FORMAT_STRING** - String list
                          - **OBS_COMBO_FORMAT_BOOL** - Boolean list

                              .. versionadded:: 30.0

   :return:               The property

   Important Related Functions:

   - :c:func:`obs_property_list_add_string`
   - :c:func:`obs_property_list_add_int`
   - :c:func:`obs_property_list_add_float`
   - :c:func:`obs_property_list_insert_string`
   - :c:func:`obs_property_list_insert_int`
   - :c:func:`obs_property_list_insert_float`
   - :c:func:`obs_property_list_item_remove`
   - :c:func:`obs_property_list_clear`

---------------------

.. function:: obs_property_t *obs_properties_add_color(obs_properties_t *props, const char *name, const char *description)

   Adds a color property without alpha (stored internally with an alpha value of 255).
   The color can be retrieved from an :c:type:`obs_data_t` object by using :c:func:`obs_data_get_int()`.

   :param    name:        Setting identifier string
   :param    description: Localized name shown to user
   :return:               The property

---------------------

.. function:: obs_property_t *obs_properties_add_color_alpha(obs_properties_t *props, const char *name, const char *description)

   Adds a color property with alpha. The color can be retrieved from an
   :c:type:`obs_data_t` object by using :c:func:`obs_data_get_int()`.

   :param    name:        Setting identifier string
   :param    description: Localized name shown to user
   :return:               The property

---------------------

.. function:: obs_property_t *obs_properties_add_button(obs_properties_t *props, const char *name, const char *text, obs_property_clicked_t callback)
              obs_property_t *obs_properties_add_button2(obs_properties_t *props, const char *name, const char *text, obs_property_clicked_t callback, void *priv)

   Adds a button property.  This property does not actually store any
   settings; it's used to implement a button in user interface if the
   properties are used to generate user interface.

   If the properties need to be refreshed due to changes to the property layout,
   the callback should return true, otherwise return false.

   :param    name:        Setting identifier string
   :param    text:        Localized name shown to user
   :param    callback:    Callback to be executed when the button is pressed
   :param    priv:        Pointer passed back as the `data` argument of the callback
   :return:               The property

   Important Related Functions:

      - :c:func:`obs_property_button_set_type`
      - :c:func:`obs_property_button_set_url`

   For scripting, use :py:func:`obs_properties_add_button`.

   Relevant data types used with this function:

.. code:: cpp

   typedef bool (*obs_property_clicked_t)(obs_properties_t *props,
                   obs_property_t *property, void *data);

---------------------

.. function:: obs_property_t *obs_properties_add_font(obs_properties_t *props, const char *name, const char *description)

   Adds a font property. The font can be retrieved from an :c:type:`obs_data_t`
   object by using :c:func:`obs_data_get_obj()`.

   :param    name:        Setting identifier string
   :param    description: Localized name shown to user
   :return:               The property

---------------------

.. function:: obs_property_t *obs_properties_add_editable_list(obs_properties_t *props, const char *name, const char *description, enum obs_editable_list_type type, const char *filter, const char *default_path)

   Adds a list in which the user can add/insert/remove items. The items can be
   retrieved from an :c:type:`obs_data_t` object by using :c:func:`obs_data_get_array()`.

   :param    name:         Setting identifier string
   :param    description:  Localized name shown to user
   :param    type:         Can be one of the following values:
                          
                           - **OBS_EDITABLE_LIST_TYPE_STRINGS** - An
                             editable list of strings.
                           - **OBS_EDITABLE_LIST_TYPE_FILES** - An
                             editable list of files.
                           - **OBS_EDITABLE_LIST_TYPE_FILES_AND_URLS** -
                             An editable list of files and URLs.

   :param    filter:       File filter to use if a file list
   :param    default_path: Default path if a file list
   :return:                The property

---------------------

.. function:: obs_property_t *obs_properties_add_frame_rate(obs_properties_t *props, const char *name, const char *description)

   Adds a frame rate property.

   :param    name:        Setting identifier string
   :param    description: Localized name shown to user
   :return:               The property

   Important Related Functions:

   - :c:func:`obs_property_frame_rate_option_add`
   - :c:func:`obs_property_frame_rate_fps_range_add`
   - :c:func:`obs_property_frame_rate_option_insert`
   - :c:func:`obs_property_frame_rate_fps_range_insert`

---------------------

.. function:: obs_property_t *obs_properties_add_group(obs_properties_t *props, const char *name, const char *description, enum obs_group_type type, obs_properties_t *group)

   Adds a property group.

   :param    name:        Setting identifier string
   :param    description: Localized name shown to user
   :param    type:        Can be one of the following values:

                          - **OBS_GROUP_NORMAL** - A normal group with just a name and content.
                          - **OBS_GROUP_CHECKABLE** - A checkable group with a checkbox, name and content.

   :param    group:       Group to add

   :return:               The property

   Important Related Functions:

   - :c:func:`obs_property_group_type`
   - :c:func:`obs_property_group_content`
   - :c:func:`obs_properties_get_parent`

---------------------


Property Enumeration Functions
------------------------------

.. function:: obs_property_t *obs_properties_first(obs_properties_t *props)

   :return: The first property in the properties object.

---------------------

.. function:: obs_property_t *obs_properties_get(obs_properties_t *props, const char *property)

   :param property: The name of the property to get
   :return:         A specific property or *NULL* if not found

---------------------

.. function:: bool                   obs_property_next(obs_property_t **p)

   :param p: Pointer to the pointer of the next property
   :return: *true* if successful, *false* if no more properties

---------------------

.. function:: const char *           obs_property_name(obs_property_t *p)

   :return: The setting identifier string of the property

   *(Author's Note: "name" was a bad name to use here.  Should have been
   "setting")*

---------------------

.. function:: const char *           obs_property_description(obs_property_t *p)

   :return: The actual localized display name of the property

   *(Author's note: This one should have been the "name")*

---------------------

.. function:: const char *           obs_property_long_description(obs_property_t *p)

   :return: A detailed description of what the setting is used for.
            Usually used with things like tooltips.

---------------------

.. function:: enum obs_property_type obs_property_get_type(obs_property_t *p)

   :return: One of the following values:

            - OBS_PROPERTY_INVALID
            - OBS_PROPERTY_BOOL
            - OBS_PROPERTY_INT
            - OBS_PROPERTY_FLOAT
            - OBS_PROPERTY_TEXT
            - OBS_PROPERTY_PATH
            - OBS_PROPERTY_LIST
            - OBS_PROPERTY_COLOR
            - OBS_PROPERTY_BUTTON
            - OBS_PROPERTY_FONT
            - OBS_PROPERTY_EDITABLE_LIST
            - OBS_PROPERTY_FRAME_RATE
            - OBS_PROPERTY_GROUP

---------------------

.. function:: bool                   obs_property_enabled(obs_property_t *p)

---------------------

.. function:: bool                   obs_property_visible(obs_property_t *p)

---------------------

.. function:: int                    obs_property_int_min(obs_property_t *p)

---------------------

.. function:: int                    obs_property_int_max(obs_property_t *p)

---------------------

.. function:: int                    obs_property_int_step(obs_property_t *p)

---------------------

.. function:: enum obs_number_type   obs_property_int_type(obs_property_t *p)

---------------------

.. function:: const char *           obs_property_int_suffix(obs_property_t *p)

---------------------

.. function:: double                 obs_property_float_min(obs_property_t *p)

---------------------

.. function:: double                 obs_property_float_max(obs_property_t *p)

---------------------

.. function:: double                 obs_property_float_step(obs_property_t *p)

---------------------

.. function:: enum obs_number_type   obs_property_float_type(obs_property_t *p)

---------------------

.. function:: const char *           obs_property_float_suffix(obs_property_t *p)

---------------------

.. function:: enum obs_text_type     obs_property_text_type(obs_property_t *p)

---------------------

.. function:: bool                   obs_property_text_monospace(obs_property_t *p)

   Returns whether the input of the text property should be rendered
   with a monospace font or not. Only has an effect if the text type
   of the property is ``OBS_TEXT_MULTILINE``, even if this returns *true*.

---------------------

.. function:: enum obs_text_info_type     obs_property_text_info_type(obs_property_t *p)

   :return: One of the following values:

             - OBS_TEXT_INFO_NORMAL
             - OBS_TEXT_INFO_WARNING
             - OBS_TEXT_INFO_ERROR

---------------------

.. function:: bool     obs_property_text_info_word_wrap(obs_property_t *p)

---------------------

.. function:: enum obs_path_type     obs_property_path_type(obs_property_t *p)

---------------------

.. function:: const char *           obs_property_path_filter(obs_property_t *p)

---------------------

.. function:: const char *           obs_property_path_default_path(obs_property_t *p)

---------------------

.. function:: enum obs_combo_type    obs_property_list_type(obs_property_t *p)

---------------------

.. function:: enum obs_combo_format  obs_property_list_format(obs_property_t *p)

---------------------

.. function:: bool obs_property_list_item_disabled(obs_property_t *p, size_t idx)

---------------------

.. function:: size_t      obs_property_list_item_count(obs_property_t *p)

---------------------

.. function:: const char *obs_property_list_item_name(obs_property_t *p, size_t idx)

---------------------

.. function:: const char *obs_property_list_item_string(obs_property_t *p, size_t idx)

---------------------

.. function:: long long   obs_property_list_item_int(obs_property_t *p, size_t idx)

---------------------

.. function:: double      obs_property_list_item_float(obs_property_t *p, size_t idx)

---------------------

.. function:: enum obs_editable_list_type obs_property_editable_list_type(obs_property_t *p)

---------------------

.. function:: const char *obs_property_editable_list_filter(obs_property_t *p)

---------------------

.. function:: const char *obs_property_editable_list_default_path(obs_property_t *p)

---------------------

.. function:: size_t      obs_property_frame_rate_options_count(obs_property_t *p)

---------------------

.. function:: const char *obs_property_frame_rate_option_name(obs_property_t *p, size_t idx)

---------------------

.. function:: const char *obs_property_frame_rate_option_description( obs_property_t *p, size_t idx)

---------------------

.. function:: size_t      obs_property_frame_rate_fps_ranges_count(obs_property_t *p)

---------------------

.. function:: struct media_frames_per_second obs_property_frame_rate_fps_range_min( obs_property_t *p, size_t idx)

---------------------

.. function:: struct media_frames_per_second obs_property_frame_rate_fps_range_max( obs_property_t *p, size_t idx)

---------------------

.. function:: enum obs_button_type obs_property_button_type(obs_property_t *p)

   :return: One of the following values:

             - OBS_BUTTON_DEFAULT
             - OBS_BUTTON_URL

---------------------

.. function:: const char *obs_property_button_url(obs_property_t *p)

---------------------

.. function:: enum obs_group_type obs_property_group_type(obs_property_t *p)

  :return: One of the following values:

            - OBS_COMBO_INVALID
            - OBS_GROUP_NORMAL
            - OBS_GROUP_CHECKABLE

---------------------

.. function:: obs_properties_t *obs_property_group_content(obs_property_t *p)

---------------------


Property Modification Functions
-------------------------------

.. function:: void obs_property_set_modified_callback(obs_property_t *p, obs_property_modified_t modified)
              void obs_property_set_modified_callback2(obs_property_t *p, obs_property_modified2_t modified2, void *priv)

   Allows the ability to change the properties depending on what
   settings are used by the user. The callback should return ``true``
   if the property widgets need to be refreshed due to changes to the
   property layout.

   Relevant data types used with these functions:

.. code:: cpp

   typedef bool (*obs_property_modified_t)(obs_properties_t *props,
                   obs_property_t *property, obs_data_t *settings);
   typedef bool (*obs_property_modified2_t)(void *priv,
                   obs_properties_t *props, obs_property_t *property,
                   obs_data_t *settings);

---------------------

.. function:: bool obs_property_modified(obs_property_t *p, obs_data_t *settings)

---------------------

.. function:: bool obs_property_button_clicked(obs_property_t *p, void *obj)

---------------------

.. function:: void obs_property_set_visible(obs_property_t *p, bool visible)

---------------------

.. function:: void obs_property_set_enabled(obs_property_t *p, bool enabled)

---------------------

.. function:: void obs_property_set_description(obs_property_t *p, const char *description)

   Sets the displayed localized name of the property, shown to the user.

---------------------

.. function:: void obs_property_set_long_description(obs_property_t *p, const char *long_description)

   Sets the localized long description of the property, usually shown to
   a user via tooltip.

---------------------

.. function:: void obs_property_int_set_limits(obs_property_t *p, int min, int max, int step)

---------------------

.. function:: void obs_property_float_set_limits(obs_property_t *p, double min, double max, double step)

---------------------

.. function:: void obs_property_int_set_suffix(obs_property_t *p, const char *suffix)

   Adds a suffix to the int property, such that 100 will show up
   as "100ms" if the suffix is "ms". The user will only be able
   to edit the number, not the suffix.

---------------------

.. function:: void obs_property_float_set_suffix(obs_property_t *p, const char *suffix)

   Adds a suffix to the float property, such that 1.5 will show up
   as "1.5s" if the suffix is "s". The user will only be able
   to edit the number, not the suffix.

---------------------

.. function:: void obs_property_text_set_monospace(obs_property_t *p, bool monospace)

   Sets whether the input of text property should be rendered with
   a monospace font or not. Only has an effect if the text type of
   the property is ``OBS_TEXT_MULTILINE``.

---------------------

.. function:: void     obs_property_text_set_info_type(obs_property_t *p, enum obs_text_info_type type)

   :param   type: Can be one of the following values:

                  - **OBS_TEXT_INFO_NORMAL** - To show a basic message
                  - **OBS_TEXT_INFO_WARNING** - To show a warning
                  - **OBS_TEXT_INFO_ERROR** - To show an error

---------------------

.. function:: void     obs_property_text_set_info_word_wrap(obs_property_t *p, bool word_wrap)

---------------------

.. function:: void obs_property_list_clear(obs_property_t *p)

---------------------

.. function:: size_t obs_property_list_add_string(obs_property_t *p, const char *name, const char *val)

   Adds a string to a string list.

   :param    name: Localized name shown to user
   :param    val:  The actual string value stored and will be returned by :c:func:`obs_data_get_string`
   :returns: The index of the list item.

   `Note` : If ``p`` is of type OBS_COMBO_TYPE_EDITABLE, :c:func:`obs_data_get_string` will return ``name`` instead of ``val``.

---------------------

.. function:: size_t obs_property_list_add_int(obs_property_t *p, const char *name, long long val)

   Adds an integer to a integer list.

   :param    name: Localized name shown to user
   :param    val:  The actual int value stored and will be returned by :c:func:`obs_data_get_int`
   :returns: The index of the list item.

---------------------

.. function:: size_t obs_property_list_add_float(obs_property_t *p, const char *name, double val)

   Adds a floating point to a floating point list.

   :param    name: Localized name shown to user
   :param    val:  The actual float value stored and will be returned by :c:func:`obs_data_get_double`
   :returns: The index of the list item.

---------------------

.. function:: void obs_property_list_insert_string(obs_property_t *p, size_t idx, const char *name, const char *val)

   Inserts a string in to a string list.

   :param    idx:  The index of the list item
   :param    name: Localized name shown to user
   :param    val:  The actual string value stored and will be returned by :c:func:`obs_data_get_string`

   `Note` : If ``p`` is of type OBS_COMBO_TYPE_EDITABLE, :c:func:`obs_data_get_string` will return ``name`` instead of ``val``.

---------------------

.. function:: void obs_property_list_insert_int(obs_property_t *p, size_t idx, const char *name, long long val)

   Inserts an integer in to an integer list.

   :param    idx:  The index of the list item
   :param    name: Localized name shown to user
   :param    val:  The actual int value stored and will be returned by :c:func:`obs_data_get_int`

---------------------

.. function:: void obs_property_list_insert_float(obs_property_t *p, size_t idx, const char *name, double val)

   Inserts a floating point in to a floating point list.

   :param    idx:  The index of the list item.
   :param    name: Localized name shown to user
   :param    val:  The actual float value stored and will be returned by :c:func:`obs_data_get_double`

---------------------

.. function:: void obs_property_list_item_disable(obs_property_t *p, size_t idx, bool disabled)

---------------------

.. function:: void obs_property_list_item_remove(obs_property_t *p, size_t idx)

---------------------

.. function:: size_t obs_property_frame_rate_option_add(obs_property_t *p, const char *name, const char *description)

---------------------

.. function:: size_t obs_property_frame_rate_fps_range_add(obs_property_t *p, struct media_frames_per_second min, struct media_frames_per_second max)

---------------------

.. function:: void obs_property_frame_rate_clear(obs_property_t *p)

---------------------

.. function:: void obs_property_frame_rate_options_clear(obs_property_t *p)

---------------------

.. function:: void obs_property_frame_rate_fps_ranges_clear(obs_property_t *p)

---------------------

.. function:: void obs_property_frame_rate_option_insert(obs_property_t *p, size_t idx, const char *name, const char *description)

---------------------

.. function:: void obs_property_frame_rate_fps_range_insert(obs_property_t *p, size_t idx, struct media_frames_per_second min, struct media_frames_per_second max)

---------------------

.. function:: void obs_property_button_set_type(obs_property_t *p, enum obs_button_type type)

   :param   type: Can be one of the following values:

                  - **OBS_BUTTON_DEFAULT** - Standard button
                  - **OBS_BUTTON_URL** - Button that opens a URL
   :return:       The property

---------------------

.. function:: void obs_property_button_set_url(obs_property_t *p, char *url)
