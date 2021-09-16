Random Number Generation
========================

Helper functions to turn raw random bytes into random numbers.

.. code:: cpp

   #include <util/rng.h>

RNG Functions
-------------

.. function:: uint64_t random_uint64_bounded(uint64_t min, uint64_t max)

   Generates an unbiased 64 bit unsigned integer in the range min - max
   (inclusive).

   :param min: Minimum bound
   :param max: Maximum bound, must be >= min
   :return:           The generated number

---------------------

.. function:: uint64_t random_uint64(void)

   Generates a 64 bit unsigned integer in the range 0 - UINT64_MAX.
   
   :return:           The generated number
