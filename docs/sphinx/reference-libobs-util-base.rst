Logging
=======

Functions for logging and getting log data.

.. code:: cpp

   #include <util/base.h>


Logging Levels
--------------

**LOG_ERROR** = 100

   Use if there's a problem that can potentially affect the program,
   but isn't enough to require termination of the program.

   Use in creation functions and core subsystem functions.  Places that
   should definitely not fail.

**LOG_WARNING** = 200

   Use if a problem occurs that doesn't affect the program and is
   recoverable.

   Use in places where failure isn't entirely unexpected, and can
   be handled safely.

**LOG_INFO** = 300

   Informative message to be displayed in the log.

**LOG_DEBUG** = 400

   Debug message to be used mostly by and for developers.


Logging Functions
-----------------

.. type:: typedef void (*log_handler_t)(int lvl, const char *msg, va_list args, void *p)

   Logging callback.

---------------------

.. function:: void base_set_log_handler(log_handler_t handler, void *param)
              void base_get_log_handler(log_handler_t *handler, void **param)

   Sets/gets the current log handler.

---------------------

.. function:: void base_set_crash_handler(void (*handler)(const char *, va_list, void *), void *param)

   Sets the current crash handler.

---------------------

.. function:: void blogva(int log_level, const char *format, va_list args)

   Logging function (using a va_list).

---------------------

.. function:: void blog(int log_level, const char *format, ...)

   Logging function.

---------------------

.. function:: void bcrash(const char *format, ...)

   Crash function.
