Config Files
============

The configuration file functions are a simple implementation of the INI
file format, with the addition of default values.

.. code:: cpp

   #include <util/config-file.h>

.. type:: config_t


Config File Functions
---------------------

.. function:: config_t *config_create(const char *file)

   Creates a new configuration object and associates it with the
   specified file name.

   :param file: Path to the new configuration file
   :return:     A new configuration file object

----------------------

.. function:: int config_open(config_t **config, const char *file, enum config_open_type open_type)

   Opens a configuration file.

   :param config:    Pointer that receives a pointer to a new configuration
                     file object (if successful)
   :param file:      Path to the configuration file
   :param open_type: Can be one of the following values:

                     - CONFIG_OPEN_EXISTING - Fail if the file doesn't
                       exist.
                     - CONFIG_OPEN_ALWAYS - Try to open the file.  If
                       the file doesn't exist, create it.

   :return:          Can return the following values:

                     - CONFIG_SUCCESS - Successful
                     - CONFIG_FILENOTFOUND - File not found
                     - CONFIG_ERROR - Generic error

----------------------

.. function:: int config_open_string(config_t **config, const char *str)

   Opens configuration data via a string rather than a file.

   :param config:    Pointer that receives a pointer to a new configuration
                     file object (if successful)
   :param str:       Configuration string

   :return:          Can return the following values:

                     - CONFIG_SUCCESS - Successful
                     - CONFIG_FILENOTFOUND - File not found
                     - CONFIG_ERROR - Generic error

----------------------

.. function:: int config_save(config_t *config)

   Saves configuration data to a file (if associated with a file).

   :param config:    Configuration object

   :return:          Can return the following values:

                     - CONFIG_SUCCESS - Successful
                     - CONFIG_FILENOTFOUND - File not found
                     - CONFIG_ERROR - Generic error

----------------------

.. function:: int config_save_safe(config_t *config, const char *temp_ext, const char *backup_ext)

   Saves configuration data and minimizes overwrite corruption risk.
   Saves the file with the file name

   :param config:     Configuration object
   :param temp_ext:   Temporary extension for the new file
   :param backup_ext: Backup extension for the old file.  Can be *NULL*
                      if no backup is desired.

   :return:           Can return the following values:

                      - CONFIG_SUCCESS - Successful
                      - CONFIG_FILENOTFOUND - File not found
                      - CONFIG_ERROR - Generic error

----------------------

.. function:: void config_close(config_t *config)

   Closes the configuration object.

   :param config:     Configuration object

----------------------

.. function:: size_t config_num_sections(config_t *config)

   Returns the number of sections.

   :param config:     Configuration object
   :return:           Number of configuration sections

----------------------

.. function:: const char *config_get_section(config_t *config, size_t idx)

   Returns a section name based upon its index.

   :param config:     Configuration object
   :param idx:        Index of the section
   :return:           The section's name

Set/Get Functions
-----------------

.. function:: void config_set_string(config_t *config, const char *section, const char *name, const char *value)

   Sets a string value.

   :param config:     Configuration object
   :param section:    The section of the value
   :param name:       The value name
   :param value:      The string value

----------------------

.. function:: void config_set_int(config_t *config, const char *section, const char *name, int64_t value)

   Sets an integer value.

   :param config:     Configuration object
   :param section:    The section of the value
   :param name:       The value name
   :param value:      The integer value

----------------------

.. function:: void config_set_uint(config_t *config, const char *section, const char *name, uint64_t value)

   Sets an unsigned integer value.

   :param config:     Configuration object
   :param section:    The section of the value
   :param name:       The value name
   :param value:      The unsigned integer value

----------------------

.. function:: void config_set_bool(config_t *config, const char *section, const char *name, bool value)

   Sets a boolean value.

   :param config:     Configuration object
   :param section:    The section of the value
   :param name:       The value name
   :param value:      The boolean value

----------------------

.. function:: void config_set_double(config_t *config, const char *section, const char *name, double value)

   Sets a floating point value.

   :param config:     Configuration object
   :param section:    The section of the value
   :param name:       The value name
   :param value:      The floating point value

----------------------

.. function:: const char *config_get_string(config_t *config, const char *section, const char *name)

   Gets a string value.  If the value is not set, it will use the
   default value.  If there is no default value, it will return *NULL*.

   :param config:     Configuration object
   :param section:    The section of the value
   :param name:       The value name
   :return:           The string value

----------------------

.. function:: int64_t config_get_int(config_t *config, const char *section, const char *name)

   Gets an integer value.  If the value is not set, it will use the
   default value.  If there is no default value, it will return 0.

   :param config:     Configuration object
   :param section:    The section of the value
   :param name:       The value name
   :return:           The integer value

----------------------

.. function:: uint64_t config_get_uint(config_t *config, const char *section, const char *name)

   Gets an unsigned integer value.  If the value is not set, it will use
   the default value.  If there is no default value, it will return 0.

   :param config:     Configuration object
   :param section:    The section of the value
   :param name:       The value name
   :return:           The unsigned integer value

----------------------

.. function:: bool config_get_bool(config_t *config, const char *section, const char *name)

   Gets a boolean value.  If the value is not set, it will use the
   default value.  If there is no default value, it will return false.

   :param config:     Configuration object
   :param section:    The section of the value
   :param name:       The value name
   :return:           The boolean value

----------------------

.. function:: double config_get_double(config_t *config, const char *section, const char *name)

   Gets a floating point value.  If the value is not set, it will use
   the default value.  If there is no default value, it will return 0.0.

   :param config:     Configuration object
   :param section:    The section of the value
   :param name:       The value name
   :return:           The floating point value

----------------------

.. function:: bool config_remove_value(config_t *config, const char *section, const char *name)

   Removes a value.  Does not remove the default value if any.

   :param config:     Configuration object
   :param section:    The section of the value
   :param name:       The value name


Default Value Functions
-----------------------

.. function:: int config_open_defaults(config_t *config, const char *file)

   Opens a file and uses it for default values.

   :param config:     Configuration object
   :param file:       The file to open for default values

----------------------

.. function:: void config_set_default_string(config_t *config, const char *section, const char *name, const char *value)

   Sets a default string value.

   :param config:     Configuration object
   :param section:    The section of the value
   :param name:       The value name
   :param value:      The string value

----------------------

.. function:: void config_set_default_int(config_t *config, const char *section, const char *name, int64_t value)

   Sets a default integer value.

   :param config:     Configuration object
   :param section:    The section of the value
   :param name:       The value name
   :param value:      The integer value

----------------------

.. function:: void config_set_default_uint(config_t *config, const char *section, const char *name, uint64_t value)

   Sets a default unsigned integer value.

   :param config:     Configuration object
   :param section:    The section of the value
   :param name:       The value name
   :param value:      The unsigned integer value

----------------------

.. function:: void config_set_default_bool(config_t *config, const char *section, const char *name, bool value)

   Sets a default boolean value.

   :param config:     Configuration object
   :param section:    The section of the value
   :param name:       The value name
   :param value:      The boolean value

----------------------

.. function:: void config_set_default_double(config_t *config, const char *section, const char *name, double value)

   Sets a default floating point value.

   :param config:     Configuration object
   :param section:    The section of the value
   :param name:       The value name
   :param value:      The floating point value
