Data Settings API Reference (obs_data_t)
========================================

Data settings objects are reference-counted objects that store values in
a string-table or array.  They're similar to Json objects, but
additionally allow additional functionality such as default or
auto-selection values.  Data is saved/loaded to/from Json text and Json
text files.

.. type:: obs_data_t

   A reference-counted data object.

.. type:: obs_data_array_t

   A reference-counted data array object.

.. code:: cpp

   #include <obs.h>


General Functions
-----------------

.. function:: obs_data_t *obs_data_create()

   :return: A new reference to a data object.

---------------------

.. function:: obs_data_t *obs_data_create_from_json(const char *json_string)

   Creates a data object from a Json string.

   :param json_string: Json string
   :return:            A new reference to a data object

---------------------

.. function:: obs_data_t *obs_data_create_from_json_file(const char *json_file)

   Creates a data object from a Json file.

   :param json_file: Json file path
   :return:          A new reference to a data object

---------------------

.. function:: obs_data_t *obs_data_create_from_json_file_safe(const char *json_file, const char *backup_ext)

   Creates a data object from a Json file, with a backup file in case
   the original is corrupted or fails to load.

   :param json_file:  Json file path
   :param backup_ext: Backup file extension
   :return:           A new reference to a data object

---------------------

.. function:: void obs_data_addref(obs_data_t *data)
              void obs_data_release(obs_data_t *data)

   Adds/releases a reference to a data object.

---------------------

.. function:: const char *obs_data_get_json(obs_data_t *data)

   :return: Json string for this object

---------------------

.. function:: bool obs_data_save_json(obs_data_t *data, const char *file)

   Saves the data to a file as Json text.

   :param file: The file to save to
   :return:     *true* if successful, *false* otherwise

---------------------

.. function:: bool obs_data_save_json_safe(obs_data_t *data, const char *file, const char *temp_ext, const char *backup_ext)

   Saves the data to a file as Json text, and if overwriting an old
   file, backs up that old file to help prevent potential file
   corruption.

   :param file:       The file to save to
   :param backup_ext: The backup extension to use for the overwritten
                      file if it exists
   :return:           *true* if successful, *false* otherwise

---------------------

.. function:: void obs_data_apply(obs_data_t *target, obs_data_t *apply_data)

   Merges the data of *apply_data* in to *target*.

---------------------

.. function:: void obs_data_erase(obs_data_t *data, const char *name)

   Erases the user data for item *name* within the data object.

---------------------

.. function:: void obs_data_clear(obs_data_t *data)

   Clears all user data in the data object.

---------------------


Set Functions
-------------

.. function:: void obs_data_set_string(obs_data_t *data, const char *name, const char *val)

---------------------

.. function:: void obs_data_set_int(obs_data_t *data, const char *name, long long val)

---------------------

.. function:: void obs_data_set_double(obs_data_t *data, const char *name, double val)

---------------------

.. function:: void obs_data_set_bool(obs_data_t *data, const char *name, bool val)

---------------------

.. function:: void obs_data_set_obj(obs_data_t *data, const char *name, obs_data_t *obj)

---------------------

.. function:: void obs_data_set_array(obs_data_t *data, const char *name, obs_data_array_t *array)

---------------------


.. _obs_data_get_funcs:

Get Functions
-------------

.. function:: const char *obs_data_get_string(obs_data_t *data, const char *name)

---------------------

.. function:: long long obs_data_get_int(obs_data_t *data, const char *name)

---------------------

.. function:: double obs_data_get_double(obs_data_t *data, const char *name)

---------------------

.. function:: bool obs_data_get_bool(obs_data_t *data, const char *name)

---------------------

.. function:: obs_data_t *obs_data_get_obj(obs_data_t *data, const char *name)

   :return: An incremented reference to a data object.

---------------------

.. function:: obs_data_array_t *obs_data_get_array(obs_data_t *data, const char *name)

   :return: An incremented reference to a data array object.

---------------------


.. _obs_data_default_funcs:

Default Value Functions
-----------------------

Default values are used to determine what value will be given if a value
is not set.

.. function:: void obs_data_set_default_string(obs_data_t *data, const char *name, const char *val)
              const char *obs_data_get_default_string(obs_data_t *data, const char *name)

---------------------

.. function:: void obs_data_set_default_int(obs_data_t *data, const char *name, long long val)
              long long obs_data_get_default_int(obs_data_t *data, const char *name)

---------------------

.. function:: void obs_data_set_default_double(obs_data_t *data, const char *name, double val)
              double obs_data_get_default_double(obs_data_t *data, const char *name)

---------------------

.. function:: void obs_data_set_default_bool(obs_data_t *data, const char *name, bool val)
              bool obs_data_get_default_bool(obs_data_t *data, const char *name)

---------------------

.. function:: void obs_data_set_default_obj(obs_data_t *data, const char *name, obs_data_t *obj)
              obs_data_t *obs_data_get_default_obj(obs_data_t *data, const char *name)

   :return: An incremented reference to a data object.

---------------------


Autoselect Functions
--------------------

Autoselect values are optionally used to determine what values should be
used to ensure functionality if the currently set values are
inappropriate or invalid.

.. function:: void obs_data_set_autoselect_string(obs_data_t *data, const char *name, const char *val)
              const char *obs_data_get_autoselect_string(obs_data_t *data, const char *name)

---------------------

.. function:: void obs_data_set_autoselect_int(obs_data_t *data, const char *name, long long val)
              long long obs_data_get_autoselect_int(obs_data_t *data, const char *name)

---------------------

.. function:: void obs_data_set_autoselect_double(obs_data_t *data, const char *name, double val)
              double obs_data_get_autoselect_double(obs_data_t *data, const char *name)

---------------------

.. function:: void obs_data_set_autoselect_bool(obs_data_t *data, const char *name, bool val)
              bool obs_data_get_autoselect_bool(obs_data_t *data, const char *name)

---------------------

.. function:: void obs_data_set_autoselect_obj(obs_data_t *data, const char *name, obs_data_t *obj)
              obs_data_t *obs_data_get_autoselect_obj(obs_data_t *data, const char *name)

   :return: An incremented reference to a data object.

---------------------


Array Functions
---------------

.. function:: obs_data_array_t *obs_data_array_create()

   :return: A new reference to a data array object.

---------------------

.. function:: void obs_data_array_addref(obs_data_array_t *array)

---------------------

.. function:: void obs_data_array_release(obs_data_array_t *array)

---------------------

.. function:: size_t obs_data_array_count(obs_data_array_t *array)

---------------------

.. function:: obs_data_t *obs_data_array_item(obs_data_array_t *array, size_t idx)

   :return: An incremented reference to the data object associated with
            this array entry.

---------------------

.. function:: size_t obs_data_array_push_back(obs_data_array_t *array, obs_data_t *obj)

---------------------

.. function:: void obs_data_array_insert(obs_data_array_t *array, size_t idx, obs_data_t *obj)

---------------------

.. function:: void obs_data_array_erase(obs_data_array_t *array, size_t idx)
