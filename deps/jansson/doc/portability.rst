***********
Portability
***********

Thread safety
-------------

Jansson is thread safe and has no mutable global state. The only
exception are the memory allocation functions, that should be set at
most once, and only on program startup. See
:ref:`apiref-custom-memory-allocation`.

There's no locking performed inside Jansson's code, so a multithreaded
program must perform its own locking if JSON values are shared by
multiple threads. Jansson's reference counting semantics may make this
a bit harder than it seems, as it's possible to have a reference to a
value that's also stored inside a list or object. Modifying the
container (adding or removing values) may trigger concurrent access to
such values, as containers manage the reference count of their
contained values. Bugs involving concurrent incrementing or
decrementing of deference counts may be hard to track.

The encoding functions (:func:`json_dumps()` and friends) track
reference loops by modifying the internal state of objects and arrays.
For this reason, encoding functions must not be run on the same JSON
values in two separate threads at the same time. As already noted
above, be especially careful if two arrays or objects share their
contained values with another array or object.

If you want to make sure that two JSON value hierarchies do not
contain shared values, use :func:`json_deep_copy()` to make copies.

Locale
------

Jansson works fine under any locale.

However, if the host program is multithreaded and uses ``setlocale()``
to switch the locale in one thread while Jansson is currently encoding
or decoding JSON data in another thread, the result may be wrong or
the program may even crash.

Jansson uses locale specific functions for certain string conversions
in the encoder and decoder, and then converts the locale specific
values to/from the JSON representation. This fails if the locale
changes between the string conversion and the locale-to-JSON
conversion. This can only happen in multithreaded programs that use
``setlocale()``, because ``setlocale()`` switches the locale for all
running threads, not only the thread that calls ``setlocale()``.

If your program uses ``setlocale()`` as described above, consider
using the thread-safe ``uselocale()`` instead.
