Memory Management
=================

Various functions and helpers used for memory management.

.. code:: cpp

   #include <util/bmem.h>


Memory Functions
----------------

.. function:: void *bmalloc(size_t size)

   Allocates memory and increases the memory leak counter.

---------------------

.. function:: void *brealloc(void *ptr, size_t size)

   Reallocates memory.  Use only with memory that's been allocated by
   :c:func:`bmalloc()`.

---------------------

.. function:: void bfree(void *ptr)

   Frees memory allocated with :c:func:`bmalloc()` or :c:func:`bfree()`.

---------------------

.. function:: long bnum_allocs(void)

   Returns current number of active allocations.

---------------------

.. function:: void *bmemdup(const void *ptr, size_t size)

   Duplicates memory.

---------------------

.. function:: void *bzalloc(size_t size)

   Inline function that allocates zeroed memory.

---------------------

.. function:: char *bstrdup_n(const char *str, size_t n)
              wchar_t *bwstrdup_n(const wchar_t *str, size_t n)

   Duplicates a string of *n* bytes and automatically zero-terminates
   it.

---------------------

.. function:: char *bstrdup(const char *str)
              wchar_t *bwstrdup(const wchar_t *str)

   Duplicates a string.
