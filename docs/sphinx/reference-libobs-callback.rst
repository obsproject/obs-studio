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

   Frees a calldata structure. Should only be used if :c:func:`calldata_init()`
   was used. If the object is received as a callback parameter, this function
   should not be used.

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

   Sets a pointer parameter. This does not have to be a pointer to a pointer.

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

.. function:: void calldata_set_data(calldata_t *data, const char *name, const void *in, size_t new_size)

   Sets a parameter with any type. For common parameter types, use the functions
   above.

   :param data:     Calldata structure
   :param name:     Parameter name
   :param in:       Pointer to the value to be stored
   :param new_size: Size of the value to be stored

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

   Gets a pointer parameter. For example, :ref:`core_signal_handler_reference`
   that have ``ptr source`` as a parameter requires this function to get the
   pointer, which can be casted to :c:type:`obs_source_t`. Does not have to be freed.

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

.. function:: bool calldata_get_int(const calldata_t *data, const char *name, long long *val)

   Gets an integer parameter. See :c:func:`calldata_int` for a simpler call.

   :param data: Calldata structure
   :param name: Parameter name
   :param val:  Pointer to an integer receiving the parameter
   :return:     ``true`` if the stored value is successfully cast

---------------------

.. function:: bool calldata_get_float(const calldata_t *data, const char *name, double *val)

   Gets a floating point parameter. See :c:func:`calldata_float` for a simpler
   call.

   :param data: Calldata structure
   :param name: Parameter name
   :param val:  Pointer to a floating point receiving the parameter
   :return:     ``true`` if the stored value is successfully cast

---------------------

.. function:: bool calldata_get_bool(const calldata_t *data, const char *name, bool *val)

   Gets a boolean parameter. See :c:func:`calldata_bool` for a simpler call.

   :param data: Calldata structure
   :param name: Parameter name
   :param val:  Pointer to a boolean receiving the parameter
   :return:     ``true`` if the stored value is successfully cast

---------------------

.. function:: bool calldata_get_ptr(const calldata_t *data, const char *name, void *p_ptr)

   Gets a pointer parameter. See :c:func:`calldata_ptr` for a simpler call.

   :param data:   Calldata structure
   :param name:   Parameter name
   :param p_ptr:  Pointer to a pointer receiving the parameter
   :return:       ``true`` if the stored value is successfully cast

---------------------

.. function:: bool calldata_get_string(const calldata_t *data, const char *name, const char **str)

   Gets a string parameter. See :c:func:`calldata_string` for a simpler call.

   :param data: Calldata structure
   :param name: Parameter name
   :param str:  Pointer to a string receiving the parameter
   :return:     ``true`` if the stored value is successfully cast

---------------------

.. function:: bool calldata_get_data(const calldata_t *data, const char *name, void *out, size_t size);

   Gets a parameter of any type. For common parameter types, use the functions
   above.

   :param data: Calldata structure
   :param name: Parameter name
   :param out:  Pointer receiving the parameter
   :param size: Size of the parameter type
   :return:     ``true`` if the stored value has the correct size

---------------------


Signals
-------

Signals are used for all event-based callbacks.

.. code:: cpp

   #include <callback/signal.h>

.. type:: signal_handler_t

---------------------

.. type:: void (*signal_callback_t)(void *data, calldata_t *cd)

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

   Adds a signal to a signal handler. Will fail if the declaration string has
   invalid syntax or a signal with the same name was already added. Other than
   the function identifier, the ``decl_string`` is mostly for documentation, as
   libobs does not strictly enforce how the parameters are handled.

   :param handler:     Signal handler object
   :param signal_decl: C-style function declaration string of the signal. The
                       function identifier dictates the signal name. Parameter
                       types can be ``int``, ``float``, ``bool``, ``ptr``,
                       ``string``, which indicates the appropriate calldata
                       function to get the parameters. May also be preceded by
                       ``in`` or ``out`` indicating how the parameters are used.
   :return:            ``true`` if the signal is successfully added.

   Example declaration strings:

   - ``void restart()``
   - ``void slide_changed(int index, string path)``
   - ``void current_index(out int current_index)``
   - ``int current_index()``
   - ``void toggled()``

   Example code:

   .. code:: cpp

      signal_handler_t *sh = obs_source_get_signal_handler(source);
      signal_handler_add(sh, "void file_changed(string next_file)");

---------------------

.. function:: bool signal_handler_add_array(signal_handler_t *handler, const char **signal_decls)

   Adds multiple signals to a signal handler. This will process all signals even
   if some can not be successfully added.

   :param handler:      Signal handler object
   :param signal_decls: An array of signal declaration strings,
                        terminated by *NULL*
   :return:             ``true`` if all signals are successfully added.
                        ``false`` if any of the signals can not be added

   Example:

   .. code:: cpp

      static const char *source_signals[] = {
              "void destroy(ptr source)",
              "void remove(ptr source)",
              "void update(ptr source)",
              NULL,
      };

      signal_handler_t *sh = obs_source_get_signal_handler(source);
      signal_handler_add_array(sh, source_signals);

---------------------

.. function:: void signal_handler_connect(signal_handler_t *handler, const char *signal, signal_callback_t callback, void *data)

   Connects a callback to a signal on a signal handler. Does nothing
   if the combination of ``signal``, ``callback``, and ``data``
   is already connected to the handler.

   :param handler:  Signal handler object
   :param signal:   Name of signal to handle
   :param callback: Signal callback
   :param data:     Private data passed to the callback

   For scripting, use :py:func:`signal_handler_connect`.

   Example connecting to ``rename`` signal from :ref:`source_signal_handler_reference`,
   declared as ``void rename(ptr source, string new_name, string prev_name)``:

   .. code:: cpp

      /* Sample source data */
      struct my_source {
              obs_source_t *source;
              char *prev_name;
              ...
      };

      void rename_cb(void *data, calldata_t *cd) {
              struct my_source *ms = data;
              obs_source_t *source = calldata_ptr(cd, "source");
              /* We could also just access the source from our source data */
              source = ms->source;
              const char *new_name = calldata_string(cd, "new_name");
              const char *prev_name = calldata_string(cd, "prev_name");

              /* Do processing here */

              /* `prev_name` will be freed after the callback, so if we want a
               * reference to it after the callback, we must duplicate it.
               */
              bfree(ms->prev_name);
              ms->prev_name = bstrdup(prev_name);
      }

      /* Assuming `ms` already contains our source data */
      struct my_source *ms;
      signal_handler_t *sh = obs_source_get_signal_handler(ms->source);
      signal_handler_connect(sh, "rename", rename_cb, ms);

---------------------

.. function:: void signal_handler_connect_ref(signal_handler_t *handler, const char *signal, signal_callback_t callback, void *data)

   Connects a callback to a signal on a signal handler, and increments
   the handler's internal reference counter, preventing it from being
   destroyed until the signal has been disconnected. Even if the combination of
   ``signal``, ``callback``, and ``data`` is already connected to the handler,
   the reference counter is still incremented.

   :param handler:  Signal handler object
   :param signal:   Name of signal to handle
   :param callback: Signal callback
   :param data:     Private data passed to the callback

---------------------

.. function:: void signal_handler_disconnect(signal_handler_t *handler, const char *signal, signal_callback_t callback, void *data)

   Disconnects a callback from a signal on a signal handler. Does nothing
   if the combination of ``signal``, ``callback``, and ``data``
   is not yet connected to the handler.

   :param handler:  Signal handler object
   :param signal:   Name of signal that was handled
   :param callback: Signal callback
   :param data:     Private data passed to the callback

   For scripting, use :py:func:`signal_handler_disconnect`.

---------------------

.. function:: void signal_handler_signal(signal_handler_t *handler, const char *signal, calldata_t *params)

   Emits a signal, calling all connected callbacks.

   :param handler: Signal handler object
   :param signal:  Name of signal to emit
   :param params:  Parameters to pass to the signal

   Example:

   .. code:: cpp

      obs_source_t *source = obs_get_source_by_name("Image Slideshow Source");

      /* Declaration string: void slide_changed(int index, string path) */
      calldata_t cd = {0};
      calldata_set_int(&cd, "index", 1);
      calldata_set_string(&cd, "path", "path/to/image.png");

      signal_handler_t *sh = obs_source_get_signal_handler(source);
      signal_handler_signal(sh, "slide_changed", &cd);
      calldata_free(&cd);
      obs_source_release(source);

---------------------


Procedure Handlers
------------------

Procedure handlers are used to call functions without having to have
direct access to declarations or callback pointers.

.. code:: cpp

   #include <callback/proc.h>

.. type:: proc_handler_t

---------------------

.. type:: void (*proc_handler_proc_t)(void *data, calldata_t *cd)

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

   Adds a procedure to a procedure handler. Will fail if the declaration string
   has invalid syntax or a procedure with the same name was already added. Other
   than the function identifier, the ``decl_string`` is mostly for
   documentation, as libobs does not strictly enforce how the parameters are
   handled.

   :param handler:     Procedure handler object
   :param decl_string: C-style function declaration string of the procedure. The
                       function identifier dictates the procedure name.
                       Parameter types can be ``int``, ``float``, ``bool``,
                       ``ptr``, ``string``, which indicates the appropriate
                       calldata function to get the parameters. May also be
                       preceded by ``in`` or ``out`` indicating how the
                       parameters are used.
   :param proc:        Procedure callback
   :param data:        Private data to pass to the callback

   Example code:

   .. code:: cpp

      /* Sample source data */
      struct my_source {
              obs_source_t *source;
              bool active;
              ...
      };

      static void proc_activate(void *data, calldata_t *cd)
      {
              struct my_source *ms = data;
              ms->active = calldata_bool(cd, "active");
      }

      /* Assuming `ms` already contains our source data */
      struct my_source *ms;
      proc_handler_t *ph = obs_source_get_proc_handler(ms->source);
      proc_handler_add(ph, "void activate(bool active)",
              proc_activate, ms);

---------------------

.. function:: bool proc_handler_call(proc_handler_t *handler, const char *name, calldata_t *params)

   Calls a procedure within the procedure handler.

   :param handler: Procedure handler object
   :param name:    Name of procedure to call
   :param params:  Calldata structure to pass to the procedure
   :return:        ``true`` if the procedure exists

   Example:

   .. code:: cpp

      /* Assuming we already have a `source` */
      proc_handler_t *ph = obs_source_get_proc_handler(source);
      calldata_t cd = {0};
      calldata_set_bool(&cd, "active", false);
      proc_handler_call(ph, "activate", &cd);
      calldata_free(&cd);
