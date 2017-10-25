Text Lookup Interface
=====================

Used for storing and looking up localized strings.  Uses an ini-file
like file format for localization lookup.

.. type:: struct text_lookup lookup_t

.. code:: cpp

   #include <util/text-lookup.h>


Text Lookup Functions
---------------------

.. function:: lookup_t *text_lookup_create(const char *path)

   Creates a text lookup object from a text lookup file.

   :param path: Path to the localization file
   :return:     New lookup object, or *NULL* if an error occurred

---------------------

.. function:: bool text_lookup_add(lookup_t *lookup, const char *path)

   Adds text lookup from a text lookup file and replaces any values.
   For example, you would load a default fallback language such as
   english with :c:func:`text_lookup_create()`, and then call this
   function to load the actual desired language in case the desired
   language isn't fully translated.

   :param lookup: Lookup object
   :param path:   Path to the localization file
   :return:       *true* if successful, *false* otherwise

---------------------

.. function:: void text_lookup_destroy(lookup_t *lookup)

   Destroys a text lookup object.

   :param lookup: Lookup object

---------------------

.. function:: bool text_lookup_getstr(lookup_t *lookup, const char *lookup_val, const char **out)

   Gets a localized text string.

   :param lookup:     Lookup object
   :param lookup_val: Value to look up
   :param out:        Pointer that receives the translated string
                      pointer
   :return:           *true* if the value exists, *false* otherwise
