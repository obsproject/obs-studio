Serializer
==========

General programmable serialization functions.  (A shared interface to
various reading/writing to/from different inputs/outputs)

.. code:: cpp

   #include <serializer.h>

Serializer Structure (struct serializer)
----------------------------------------

.. struct:: serializer
.. member:: void     *serializer.data
.. member:: size_t   (*serializer.read)(void *, void *, size_t)
.. member:: size_t   (*serializer.write)(void *, const void *, size_t)
.. member:: int64_t  (*serializer.seek)(void *, int64_t, enum serialize_seek_type)
.. member:: int64_t  (*serializer.get_pos)(void *)

Serializer Inline Functions
---------------------------

.. function:: size_t s_read(struct serializer *s, void *data, size_t size)

---------------------

.. function:: size_t s_write(struct serializer *s, const void *data, size_t size)

---------------------

.. function:: size_t serialize(struct serializer *s, void *data, size_t len)

---------------------

.. function:: int64_t serializer_seek(struct serializer *s, int64_t offset, enum serialize_seek_type seek_type)

---------------------

.. function:: int64_t serializer_get_pos(struct serializer *s)

---------------------

.. function:: void s_w8(struct serializer *s, uint8_t u8)

---------------------

.. function:: void s_wl16(struct serializer *s, uint16_t u16)

---------------------

.. function:: void s_wl32(struct serializer *s, uint32_t u32)

---------------------

.. function:: void s_wl64(struct serializer *s, uint64_t u64)

---------------------

.. function:: void s_wlf(struct serializer *s, float f)

---------------------

.. function:: void s_wld(struct serializer *s, double d)

---------------------

.. function:: void s_wb16(struct serializer *s, uint16_t u16)

---------------------

.. function:: void s_wb24(struct serializer *s, uint32_t u24)

---------------------

.. function:: void s_wb32(struct serializer *s, uint32_t u32)

---------------------

.. function:: void s_wb64(struct serializer *s, uint64_t u64)

---------------------

.. function:: void s_wbf(struct serializer *s, float f)

---------------------

.. function:: void s_wbd(struct serializer *s, double d)

---------------------


Array Output Serializer
=======================

Provides an output serializer used with dynamic arrays.

.. versionchanged:: 30.2
   Array output serializer now supports seeking.

.. code:: cpp

   #include <util/array-serializer.h>

Array Output Serializer Structure (struct array_output_data)
------------------------------------------------------------

.. struct:: array_output_data

.. code:: cpp

   DARRAY(uint8_t) array_output_data.bytes

Array Output Serializer Functions
---------------------------------

.. function:: void array_output_serializer_init(struct serializer *s, struct array_output_data *data)

---------------------

.. function:: void array_output_serializer_free(struct array_output_data *data)

---------------------

.. function:: void array_output_serializer_reset(struct array_output_data *data)

   Resets serializer without freeing data.

   .. versionadded:: 30.2

---------------------

File Input/Output Serializers
=============================

Provides file reading/writing serializers.

.. code:: cpp

   #include <util/file-serializer.h>


File Input Serializer Functions
-------------------------------

.. function:: bool file_input_serializer_init(struct serializer *s, const char *path)

   Initializes a file input serializer.

   :return:     *true* if file opened successfully, *false* otherwise

---------------------

.. function:: void file_input_serializer_free(struct serializer *s)

   Frees a file input serializer.

---------------------


File Output Serializer Functions
--------------------------------

.. function:: bool file_output_serializer_init(struct serializer *s, const char *path)

   Initializes a file output serializer.

   :return:     *true* if file created successfully, *false* otherwise

---------------------

.. function:: bool file_output_serializer_init_safe(struct serializer *s, const char *path, const char *temp_ext)

   Initializes and safely writes to a temporary file (determined by the
   temporary extension) until freed.

   :return:     *true* if file created successfully, *false* otherwise

---------------------

.. function:: void file_output_serializer_free(struct serializer *s)

   Frees the file output serializer and saves the file.

---------------------

Buffered File Output Serializer
===============================

Provides a buffered file serializer that writes data asynchronously to prevent stalls due to slow disk I/O.

Writes will only block when the buffer is full.

The buffer and chunk size are configurable with the defaults being 256 MiB and 1 MiB respectively.

.. versionadded:: 30.2

.. code:: cpp

   #include <util/buffered-file-serializer.h>


Buffered File Output Serializer Functions
-----------------------------------------

.. function:: bool buffered_file_serializer_init_defaults(struct serializer *s, const char *path)

   Initializes a buffered file output serializer with default buffer and chunk sizes.

   :return:     *true* if file created successfully, *false* otherwise

---------------------

.. function:: bool buffered_file_serializer_init(struct serializer *s, const char *path, size_t max_bufsize, size_t chunk_size)

   Initialize buffered writer with specified buffer and chunk sizes. Setting either to `0` will use the default value.

   :return:     *true* if file created successfully, *false* otherwise

---------------------

.. function:: void buffered_file_serializer_free(struct serializer *s)

   Frees the file output serializer and saves the file. Will block until I/O thread completes outstanding writes.
