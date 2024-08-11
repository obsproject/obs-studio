Double-Ended Queue
==================

A double-ended queue (deque) that will automatically increase in size as necessary
as data is pushed to the front or back.

.. code:: cpp

   #include <util/deque.h>

.. versionadded:: 30.1


Deque Structure (struct deque)
--------------------------------------------

.. struct:: deque
.. member:: void   *deque.data
.. member:: size_t deque.size
.. member:: size_t deque.start_pos
.. member:: size_t deque.end_pos
.. member:: size_t deque.capacity


Deque Inline Functions
--------------------------------

.. function:: void deque_init(struct deque *dq)

   Initializes a deque (just zeroes out the entire structure).

   :param dq: The deque

---------------------

.. function:: void deque_free(struct deque *dq)

   Frees a deque.

   :param dq: The deque

---------------------

.. function:: void deque_reserve(struct deque *dq, size_t capacity)

   Reserves a specific amount of buffer space to ensure minimum
   upsizing.

   :param dq:       The deque
   :param capacity: The new capacity, in bytes

---------------------

.. function:: void deque_upsize(struct deque *dq, size_t size)

   Sets the current active (not just reserved) size.  Any new data is
   zeroed.

   :param dq:       The deque
   :param size:     The new size, in bytes

---------------------

.. function:: void deque_place(struct deque *dq, size_t position, const void *data, size_t size)

   Places data at a specific positional index (relative to the starting
   point) within the deque.

   :param dq:       The deque
   :param position: Positional index relative to starting point
   :param data:     Data to insert
   :param size:     Size of data to insert

---------------------

.. function:: void deque_push_back(struct deque *dq, const void *data, size_t size)

   Pushes data to the end of the deque.

   :param dq:       The deque
   :param data:     Data
   :param size:     Size of data

---------------------

.. function:: void deque_push_front(struct deque *dq, const void *data, size_t size)

   Pushes data to the front of the deque.

   :param dq:       The deque
   :param data:     Data
   :param size:     Size of data

---------------------

.. function:: void deque_push_back_zero(struct deque *dq, size_t size)

   Pushes zeroed data to the end of the deque.

   :param dq:       The deque
   :param size:     Size

---------------------

.. function:: void deque_push_front_zero(struct deque *dq, size_t size)

   Pushes zeroed data to the front of the deque.

   :param dq:       The deque
   :param size:     Size

---------------------

.. function:: void deque_peek_front(struct deque *dq, void *data, size_t size)

   Peeks data at the front of the deque.

   :param dq:       The deque
   :param data:     Buffer to store data in
   :param size:     Size of data to retrieve

---------------------

.. function:: void deque_peek_back(struct deque *dq, void *data, size_t size)

   Peeks data at the back of the deque.

   :param dq:       The deque
   :param data:     Buffer to store data in
   :param size:     Size of data to retrieve

---------------------

.. function:: void deque_pop_front(struct deque *dq, void *data, size_t size)

   Pops data from the front of the deque.

   :param dq:       The deque
   :param data:     Buffer to store data in, or *NULL*
   :param size:     Size of data to retrieve

---------------------

.. function:: void deque_pop_back(struct deque *dq, void *data, size_t size)

   Pops data from the back of the deque.

   :param dq:       The deque
   :param data:     Buffer to store data in, or *NULL*
   :param size:     Size of data to retrieve

---------------------

.. function:: void *deque_data(struct deque *dq, size_t idx)

   Gets a direct pointer to data at a specific positional index within
   the deque, relative to the starting point.

   :param dq:       The deque
   :param idx:      Byte index relative to the starting point
