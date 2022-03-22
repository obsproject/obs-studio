Extra Math Functions/Macros
===========================

.. code:: 

   #include <graphics/math-extra.h>

Helper functions/macros for graphics math.

.. function:: RAD(val)

   Macro that converts a floating point degrees value to radians.

.. function:: DEG(val)

   Macro that converts a floating point radians value to degrees.

**LARGE_EPSILON**   1e-2f

   Large epsilon value.

**EPSILON**         1e-4f

   Epsilon value.

**TINY_EPSILON**    1e-5f

   Tiny Epsilon value.

**M_INFINITE**      3.4e38f

   Infinite value

---------------------

.. function:: float rand_float(int positive_only)

   Generates a random floating point value (from -1.0f..1.0f, or
   0.0f..1.0f if *positive_only* is set).
