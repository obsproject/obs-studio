Circular Buffers
================

A circular buffer that will automatically increase in size as necessary
as data is pushed to the front or back.

.. code:: cpp

   #include <util/circlebuf.h>


Circular Buffer Structure (struct circlebuf)
--------------------------------------------

.. type:: struct circlebuf
.. member:: void   *circlebuf.data
.. member:: size_t circlebuf.size
.. member:: size_t circlebuf.start_pos
.. member:: size_t circlebuf.end_pos
.. member:: size_t circlebuf.capacity


Circular Buffer Inline Functions
--------------------------------

.. function:: void circlebuf_init(struct circlebuf *cb)

   Initializes a circular buffer (just zeroes out the entire structure).

   :param cb: The circular buffer

---------------------

.. function:: void circlebuf_free(struct circlebuf *cb)

   Frees a circular buffer.

   :param cb: The circular buffer

---------------------

.. function:: void circlebuf_reserve(struct circlebuf *cb, size_t capacity)

   Reserves a specific amount of buffer space to ensure minimum
   upsizing.

   :param cb:       The circular buffer
   :param capacity: The new capacity, in bytes

---------------------

.. function:: void circlebuf_upsize(struct circlebuf *cb, size_t size)

   Sets the current active (not just reserved) size.  Any new data is
   zeroed.

   :param cb:       The circular buffer
   :param size:     The new size, in bytes

---------------------

.. function:: void circlebuf_place(struct circlebuf *cb, size_t position, const void *data, size_t size)

   Places data at a specific positional index (relative to the starting
   point) within the circular buffer.

   :param cb:       The circular buffer
   :param position: Positional index relative to starting point
   :param data:     Data to insert
   :param size:     Size of data to insert

---------------------

.. function:: void circlebuf_push_back(struct circlebuf *cb, const void *data, size_t size)

   Pushes data to the end of the circular buffer.

   :param cb:       The circular buffer
   :param data:     Data
   :param size:     Size of data

---------------------

.. function:: void circlebuf_push_front(struct circlebuf *cb, const void *data, size_t size)

   Pushes data to the front of the circular buffer.

   :param cb:       The circular buffer
   :param data:     Data
   :param size:     Size of data

---------------------

.. function:: void circlebuf_push_back_zero(struct circlebuf *cb, size_t size)

   Pushes zeroed data to the end of the circular buffer.

   :param cb:       The circular buffer
   :param size:     Size

---------------------

.. function:: void circlebuf_push_front_zero(struct circlebuf *cb, size_t size)

   Pushes zeroed data to the front of the circular buffer.

   :param cb:       The circular buffer
   :param size:     Size

---------------------

.. function:: void circlebuf_peek_front(struct circlebuf *cb, void *data, size_t size)

   Peeks data at the front of the circular buffer.

   :param cb:       The circular buffer
   :param data:     Buffer to store data in
   :param size:     Size of data to retrieve

---------------------

.. function:: void circlebuf_peek_back(struct circlebuf *cb, void *data, size_t size)

   Peeks data at the back of the circular buffer.

   :param cb:       The circular buffer
   :param data:     Buffer to store data in
   :param size:     Size of data to retrieve

---------------------

.. function:: void circlebuf_pop_front(struct circlebuf *cb, void *data, size_t size)

   Pops data from the front of the circular buffer.

   :param cb:       The circular buffer
   :param data:     Buffer to store data in, or *NULL*
   :param size:     Size of data to retrieve

---------------------

.. function:: void circlebuf_pop_back(struct circlebuf *cb, void *data, size_t size)

   Pops data from the back of the circular buffer.

   :param cb:       The circular buffer
   :param data:     Buffer to store data in, or *NULL*
   :param size:     Size of data to retrieve

---------------------

.. function:: void *circlebuf_data(struct circlebuf *cb, size_t idx)

   Gets a direct pointer to data at a specific positional index within
   the circular buffer, relative to the starting point.

   :param cb:       The circular buffer
   :param idx:      Byte index relative to the starting point
