Dynamic Arrays
==============

Dynamically resizing arrays (a C equivalent to std::vector).

.. code:: cpp

   #include <util/darray.h>

.. struct:: darray

   The base dynamic array structure.

.. macro:: DARRAY(type)

   Macro for a dynamic array based upon an actual type.  Use this with
   da_* macros.

.. member:: void *darray.array

   The array pointer.

.. member:: size_t darray.num

   The number of items within the array.

.. member:: size_t darray.capacity

   The capacity of the array.


Dynamic Array Macros
--------------------

These macro functions are used with variables created with the
**DARRAY** type macro.  When using these functions, do not use the
dynamic array value with a reference (&) operator.  For example:

.. code:: cpp

   /* creates an array of integers: 0..9 */
   DARRAY(int) array_of_integers;
   da_init(array_of_integers);

   for (size_t i = 0; i < 10; i++)
           da_push_back(array_of_integers, &i);

   [...]

   /* free when complete */
   da_free(array_of_integers);

To pass dynamic arrays to functions as parameters, create a typedef with the
:c:macro:`DARRAY` macro and type so you can pass the dynamic array with a
reference (&) operator. An alternative is to define a struct that will contain
the dynamic array, and that struct will be passed to the functions, instead of
the dynamic array directly. An example with the typedef method:

.. code:: cpp

   typedef DARRAY(int) int_array_t;

   void generate_integers(int_array_t *integers, int start, int end)
   {
           for (int i = start; i < end; i++)
                   da_push_back(*integers, &i);
   }

   [...]

   int_array_t array_of_integers;
   da_init(array_of_integers);

   generate_integers(&array_of_integers, 0, 10);

   /* free when complete */
   da_free(array_of_integers);

**IMPORTANT NOTE:** While it is also possible to accept the internal
:c:struct:`darray` struct as a function parameter (via the ``da`` member
variable of dynamic arrays) and redefine a variable with :c:macro:`DARRAY`
inside the function, doing so is not safe and not recommended. One potential
issue with it is having a type declaration in the function that is different
than the type of the actual dynamic array that will be passed to the function,
which will cause memory access issues that will not be caught by the compiler.
As mentioned above, the recommended way is to create a typedef or a container
struct, which will be safer in usage.

.. function:: void da_init(da)

   Initializes a dynamic array.

   :param da: The dynamic array

---------------------

.. function:: void da_free(da)

   Frees a dynamic array.

   :param da: The dynamic array

---------------------

.. function:: size_t da_alloc_size(v)

   Gets a size of allocated array in bytes.

   :param da: The dynamic array
   :return:   The allocated size of the dynamic array.

---------------------

.. function:: void *da_end(da)

   Gets a pointer to the last value.

   :param da: The dynamic array
   :return:   The last value of a dynamic array, or *NULL* if empty.

---------------------

.. function:: void da_reserve(da, size_t capacity)

   Reserves a specific amount of buffer space for the dynamic array.

   :param da:       The dynamic array
   :param capacity: New capacity of the dynamic array

---------------------

.. function:: void da_resize(da, size_t new_size)

   Resizes the dynamic array with zeroed values.

   :param da:   The dynamic array
   :param size: New size of the dynamic array

---------------------

.. function:: void da_copy(da_dst, da_src)

   Makes a copy of a dynamic array.

   :param da_dst: The dynamic array to copy to
   :param da_src: The dynamic array to copy from

---------------------

.. function:: void da_copy_array(da, const void *src_array, size_t size)

   Makes a copy of an array pointer.

   :param da:        The dynamic array
   :param src_array: The array pointer to make a copy from
   :param size:      New size of the dynamic array

---------------------

.. function:: void da_move(da_dst, da_src)

   Moves one dynamic array variable to another without allocating new
   data.  *da_dst* is freed before moving, *da_dst* is set to *da_src*,
   then *da_src* is then zeroed.

   :param da_dst: Destination variable
   :param da_src: Source variable

---------------------

.. function:: size_t da_find(da, const void *item_data, size_t starting_idx)

   Finds a value based upon its data.  If the value cannot be found, the
   return value will be DARRAY_INVALID (-1).

   :param da:           The dynamic array
   :param item_data:    The item data to find
   :param starting_idx: The index to start from or 0 to search the
                        entire array

---------------------

.. function:: size_t da_push_back(da, const void *data)

   Pushes data to the back of the array.

   :param da:   The dynamic array
   :param data: Pointer to the new data to push
   :return:     Index of the new value

---------------------

.. function:: void *da_push_back_new(da)

   Pushes a zeroed value to the back of the array, and returns a pointer
   to it.

   :param da: The dynamic array
   :return:   Pointer to the new value

---------------------

.. function:: size_t da_push_back_array(da, const void *src_array, size_t item_count)

   Pushes an array of values to the back of the array.

   :param da:         The dynamic array
   :param src_array:  Pointer of the array of values
   :param item_count: Number of items to push back
   :return:           Index of the first new value

---------------------

.. function:: void da_insert(da, size_t idx, const void *data)

   Inserts a value at a given index.

   :param da:   The dynamic array:
   :param idx:  Index where the new item will be inserted
   :param data: Pointer to the item data to insert

---------------------

.. function:: void *da_insert_new(da, size_t idx)

   Inserts a new zeroed value at a specific index, and returns a pointer
   to it.

   :param da:  The dynamic array
   :param idx: Index to insert at
   :return:    Pointer to the new value

---------------------

.. function:: void da_insert_array(dst, size_t idx, src, size_t n)

   Inserts one or more items at a given index.

   :param dst: The dynamic array:
   :param idx: Index where the new item will be inserted
   :param src: Pointer to the first item to insert
   :param n:   Number of items to insert

---------------------

.. function:: void da_insert_da(da_dst, size_t idx, da_src)

   Inserts a dynamic array in to another dynamic array at a specific
   index.

   :param da_dst: Destination dynamic array being inserted in to
   :param idx:    Index to insert the data at
   :param da_src: The dynamic array to insert

---------------------

.. function:: void da_erase(da, size_t idx)

   Erases an item at a specific index.

   :param da:  The dynamic array
   :param idx: The index of the value to remove

---------------------

.. function:: void da_erase_item(da, const void *item_data)

   Erases an item that matches the value specified

   :param da:        The dynamic array
   :param item_data: Pointer to the data to remove

---------------------

.. function:: void da_erase_range(da, size_t start_idx, size_t end_idx)

   Erases a range of values, including the element at ``start_idx``, but
   not the one at ``end_idx``.

   :param da:        The dynamic array
   :param start_idx: The starting index
   :param end_idx:   The ending index

---------------------

.. function:: void da_pop_back(da)

   Removes one item from the end of a dynamic array.

   :param da: The dynamic array

---------------------

.. function:: void da_join(da_dst, da_src)

   Pushes *da_src* to the end of *da_dst* and frees *da_src*.

   :param da_dst: The destination dynamic array
   :param da_src: The source dynamic array

---------------------

.. function:: void da_split(da_dst1, da_dst2, da_src, size_t split_idx)

   Creates two dynamic arrays by splitting another dynamic array at a
   specific index.  If the destination arrays are not freed, they will
   be freed before getting their new values.  The array being split will
   not be freed.

   :param da_dst1:   Dynamic array that will get the lower half
   :param da_dst2:   Dynamic array that will get the upper half
   :param da_src:    Dynamic array to split
   :param split_idx: Index to split *da_src* at

---------------------

.. function:: void da_move_item(da, size_t src_idx, size_t dst_idx)

   Moves an item from one index to another, moving data between if
   necessary.

   :param da:      The dynamic array
   :param src_idx: The index of the item to move
   :param dst_idx: The new index of where the item will be moved to

---------------------

.. function:: void da_swap(da, size_t idx1, size_t idx2)

   Swaps two values at the given indices.

   :param da: The dynamic array
   :param idx1: Index of the first item to swap
   :param idx2: Index of the second item to swap
