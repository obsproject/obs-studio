Callback API Reference (libobs/callback)
========================================


Calldata
--------

The :c:type:`calldata_t` object is used to pass parameters from signal
handlers or to procedure handlers.

.. type:: calldata_t

---------------------

.. function:: void calldata_init(calldata_t *data)

   Initializes a calldata structure (zeroes it).

   :param data: Calldata structure

---------------------

.. function:: void calldata_free(calldata_t *data)

   Frees a calldata structure.

   :param data: Calldata structure

---------------------

.. function:: void calldata_set_int(calldata_t *data, const char *name, long long val)

   Sets an integer parameter.

   :param data: Calldata structure
   :param name: Parameter name
   :param val:  Integer value

---------------------

.. function:: void calldata_set_float(calldata_t *data, const char *name, double val)

   Sets a floating point parameter.

   :param data: Calldata structure
   :param name: Parameter name
   :param val:  Floating point value

---------------------

.. function:: void calldata_set_bool(calldata_t *data, const char *name, bool val)

   Sets a boolean parameter.

   :param data: Calldata structure
   :param name: Parameter name
   :param val:  Boolean value

---------------------

.. function:: void calldata_set_ptr(calldata_t *data, const char *name, void *ptr)

   Sets a pointer parameter.

   :param data: Calldata structure
   :param name: Parameter name
   :param val:  Pointer value

---------------------

.. function:: void calldata_set_string(calldata_t *data, const char *name, const char *str)

   Sets a string parameter.

   :param data: Calldata structure
   :param name: Parameter name
   :param val:  String

---------------------

.. function:: long long calldata_int(const calldata_t *data, const char *name)

   Gets an integer parameter.

   :param data: Calldata structure
   :param name: Parameter name
   :return:     Integer value

---------------------

.. function:: double calldata_float(const calldata_t *data, const char *name)

   Gets a floating point parameter.

   :param data: Calldata structure
   :param name: Parameter name
   :return:     Floating point value

---------------------

.. function:: bool calldata_bool(const calldata_t *data, const char *name)

   Gets a boolean parameter.

   :param data: Calldata structure
   :param name: Parameter name
   :return:     Boolean value

---------------------

.. function:: void *calldata_ptr(const calldata_t *data, const char *name)

   Gets a pointer parameter.

   :param data: Calldata structure
   :param name: Parameter name
   :return:     Pointer value

---------------------

.. function:: const char *calldata_string(const calldata_t *data, const char *name)

   Gets a string parameter.

   :param data: Calldata structure
   :param name: Parameter name
   :return:     String value

---------------------


Signals
-------

Signals are used for all event-based callbacks.

.. code:: cpp

   #include <callback/signal.h>

.. type:: signal_handler_t

---------------------

.. type:: typedef void (*signal_callback_t)(void *data, calldata_t *cd)

   Signal callback.

   :param data: Private data passed to this callback
   :param cd:   Calldata object

---------------------

.. function:: signal_handler_t *signal_handler_create(void)

   Creates a new signal handler object.

   :return: A new signal handler object

---------------------

.. function:: void signal_handler_destroy(signal_handler_t *handler)

   Destroys a signal handler.

   :param handler: Signal handler object

---------------------

.. function:: bool signal_handler_add(signal_handler_t *handler, const char *signal_decl)

   Adds a signal to a signal handler.

   :param handler:     Signal handler object
   :param signal_decl: Signal declaration string

---------------------

.. function:: bool signal_handler_add_array(signal_handler_t *handler, const char **signal_decls)

   Adds multiple signals to a signal handler.

   :param handler:      Signal handler object
   :param signal_decls: An array of signal declaration strings,
                        terminated by *NULL*

---------------------

.. function:: void signal_handler_connect(signal_handler_t *handler, const char *signal, signal_callback_t callback, void *data)

   Connect a callback to a signal on a signal handler.

   :param handler:  Signal handler object
   :param callback: Signal callback
   :param data:     Private data passed the callback

---------------------

.. function:: void signal_handler_connect_ref(signal_handler_t *handler, const char *signal, signal_callback_t callback, void *data)

   Connect a callback to a signal on a signal handler, and increments
   the handler's internal reference counter, preventing it from being
   destroyed until the signal has been disconnected.

   :param handler:  Signal handler object
   :param callback: Signal callback
   :param data:     Private data passed the callback

---------------------

.. function:: void signal_handler_disconnect(signal_handler_t *handler, const char *signal, signal_callback_t callback, void *data)

   Disconnects a callback from a signal on a signal handler.

   :param handler:  Signal handler object
   :param callback: Signal callback
   :param data:     Private data passed the callback

---------------------

.. function:: void signal_handler_signal(signal_handler_t *handler, const char *signal, calldata_t *params)

   Triggers a signal, calling all connected callbacks.

   :param handler: Signal handler object
   :param signal:  Name of signal to trigger
   :param params:  Parameters to pass to the signal

---------------------


Procedure Handlers
------------------

Procedure handlers are used to call functions without having to have
direct access to declarations or callback pointers.

.. code:: cpp

   #include <callback/proc.h>

.. type:: proc_handler_t

---------------------

.. type:: typedef void (*proc_handler_proc_t)(void *data, calldata_t *cd)

   Procedure handler callback.

   :param data: Private data passed to this callback
   :param cd:   Calldata object

---------------------

.. function:: proc_handler_t *proc_handler_create(void)

   Creates a new procedure handler.

   :return: A new procedure handler object

---------------------

.. function:: void proc_handler_destroy(proc_handler_t *handler)

   Destroys a procedure handler object.

   :param handler: Procedure handler object

---------------------

.. function:: void proc_handler_add(proc_handler_t *handler, const char *decl_string, proc_handler_proc_t proc, void *data)

   Adds a procedure to a procedure handler.

   :param handler:     Procedure handler object
   :param decl_string: Procedure declaration string
   :param proc:        Procedure callback
   :param data:        Private data to pass to the callback

---------------------

.. function:: bool proc_handler_call(proc_handler_t *handler, const char *name, calldata_t *params)

   Calls a procedure within the procedure handler.

   :param handler: Procedure handler object
   :param name:    Name of procedure to call
   :param params:  Calldata structure to pass to the procedure
