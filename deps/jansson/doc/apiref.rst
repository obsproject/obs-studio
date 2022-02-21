.. _apiref:

*************
API Reference
*************

.. highlight:: c

Preliminaries
=============

All declarations are in :file:`jansson.h`, so it's enough to

::

   #include <jansson.h>

in each source file.

All constants are prefixed with ``JSON_`` (except for those describing
the library version, prefixed with ``JANSSON_``). Other identifiers
are prefixed with ``json_``. Type names are suffixed with ``_t`` and
``typedef``\ 'd so that the ``struct`` keyword need not be used.


Library Version
===============

The Jansson version is of the form *A.B.C*, where *A* is the major
version, *B* is the minor version and *C* is the micro version. If the
micro version is zero, it's omitted from the version string, i.e. the
version string is just *A.B*.

When a new release only fixes bugs and doesn't add new features or
functionality, the micro version is incremented. When new features are
added in a backwards compatible way, the minor version is incremented
and the micro version is set to zero. When there are backwards
incompatible changes, the major version is incremented and others are
set to zero.

The following preprocessor constants specify the current version of
the library:

``JANSSON_MAJOR_VERSION``, ``JANSSON_MINOR_VERSION``, ``JANSSON_MICRO_VERSION``
  Integers specifying the major, minor and micro versions,
  respectively.

``JANSSON_VERSION``
  A string representation of the current version, e.g. ``"1.2.1"`` or
  ``"1.3"``.

``JANSSON_VERSION_HEX``
  A 3-byte hexadecimal representation of the version, e.g.
  ``0x010201`` for version 1.2.1 and ``0x010300`` for version 1.3.
  This is useful in numeric comparisons, e.g.::

      #if JANSSON_VERSION_HEX >= 0x010300
      /* Code specific to version 1.3 and above */
      #endif


Value Representation
====================

The JSON specification (:rfc:`4627`) defines the following data types:
*object*, *array*, *string*, *number*, *boolean*, and *null*. JSON
types are used dynamically; arrays and objects can hold any other data
type, including themselves. For this reason, Jansson's type system is
also dynamic in nature. There's one C type to represent all JSON
values, and this structure knows the type of the JSON value it holds.

.. type:: json_t

  This data structure is used throughout the library to represent all
  JSON values. It always contains the type of the JSON value it holds
  and the value's reference count. The rest depends on the type of the
  value.

Objects of :type:`json_t` are always used through a pointer. There
are APIs for querying the type, manipulating the reference count, and
for constructing and manipulating values of different types.

Unless noted otherwise, all API functions return an error value if an
error occurs. Depending on the function's signature, the error value
is either *NULL* or -1. Invalid arguments or invalid input are
apparent sources for errors. Memory allocation and I/O operations may
also cause errors.


Type
----

.. type:: enum json_type

   The type of a JSON value. The following members are defined:

   +--------------------+
   | ``JSON_OBJECT``    |
   +--------------------+
   | ``JSON_ARRAY``     |
   +--------------------+
   | ``JSON_STRING``    |
   +--------------------+
   | ``JSON_INTEGER``   |
   +--------------------+
   | ``JSON_REAL``      |
   +--------------------+
   | ``JSON_TRUE``      |
   +--------------------+
   | ``JSON_FALSE``     |
   +--------------------+
   | ``JSON_NULL``      |
   +--------------------+

   These correspond to JSON object, array, string, number, boolean and
   null. A number is represented by either a value of the type
   ``JSON_INTEGER`` or of the type ``JSON_REAL``. A true boolean value
   is represented by a value of the type ``JSON_TRUE`` and false by a
   value of the type ``JSON_FALSE``.

.. function:: int json_typeof(const json_t *json)

   Return the type of the JSON value (a :type:`json_type` cast to
   :type:`int`). *json* MUST NOT be *NULL*. This function is actually
   implemented as a macro for speed.

.. function:: json_is_object(const json_t *json)
               json_is_array(const json_t *json)
               json_is_string(const json_t *json)
               json_is_integer(const json_t *json)
               json_is_real(const json_t *json)
               json_is_true(const json_t *json)
               json_is_false(const json_t *json)
               json_is_null(const json_t *json)

   These functions (actually macros) return true (non-zero) for values
   of the given type, and false (zero) for values of other types and
   for *NULL*.

.. function:: json_is_number(const json_t *json)

   Returns true for values of types ``JSON_INTEGER`` and
   ``JSON_REAL``, and false for other types and for *NULL*.

.. function:: json_is_boolean(const json_t *json)

   Returns true for types ``JSON_TRUE`` and ``JSON_FALSE``, and false
   for values of other types and for *NULL*.

.. function:: json_boolean_value(const json_t *json)

   Alias of :func:`json_is_true()`, i.e. returns 1 for ``JSON_TRUE``
   and 0 otherwise.

   .. versionadded:: 2.7


.. _apiref-reference-count:

Reference Count
---------------

The reference count is used to track whether a value is still in use
or not. When a value is created, it's reference count is set to 1. If
a reference to a value is kept (e.g. a value is stored somewhere for
later use), its reference count is incremented, and when the value is
no longer needed, the reference count is decremented. When the
reference count drops to zero, there are no references left, and the
value can be destroyed.

.. function:: json_t *json_incref(json_t *json)

   Increment the reference count of *json* if it's not *NULL*.
   Returns *json*.

.. function:: void json_decref(json_t *json)

   Decrement the reference count of *json*. As soon as a call to
   :func:`json_decref()` drops the reference count to zero, the value
   is destroyed and it can no longer be used.

Functions creating new JSON values set the reference count to 1. These
functions are said to return a **new reference**. Other functions
returning (existing) JSON values do not normally increase the
reference count. These functions are said to return a **borrowed
reference**. So, if the user will hold a reference to a value returned
as a borrowed reference, he must call :func:`json_incref`. As soon as
the value is no longer needed, :func:`json_decref` should be called
to release the reference.

Normally, all functions accepting a JSON value as an argument will
manage the reference, i.e. increase and decrease the reference count
as needed. However, some functions **steal** the reference, i.e. they
have the same result as if the user called :func:`json_decref()` on
the argument right after calling the function. These functions are
suffixed with ``_new`` or have ``_new_`` somewhere in their name.

For example, the following code creates a new JSON array and appends
an integer to it::

  json_t *array, *integer;

  array = json_array();
  integer = json_integer(42);

  json_array_append(array, integer);
  json_decref(integer);

Note how the caller has to release the reference to the integer value
by calling :func:`json_decref()`. By using a reference stealing
function :func:`json_array_append_new()` instead of
:func:`json_array_append()`, the code becomes much simpler::

  json_t *array = json_array();
  json_array_append_new(array, json_integer(42));

In this case, the user doesn't have to explicitly release the
reference to the integer value, as :func:`json_array_append_new()`
steals the reference when appending the value to the array.

In the following sections it is clearly documented whether a function
will return a new or borrowed reference or steal a reference to its
argument.


Circular References
-------------------

A circular reference is created when an object or an array is,
directly or indirectly, inserted inside itself. The direct case is
simple::

  json_t *obj = json_object();
  json_object_set(obj, "foo", obj);

Jansson will refuse to do this, and :func:`json_object_set()` (and
all the other such functions for objects and arrays) will return with
an error status. The indirect case is the dangerous one::

  json_t *arr1 = json_array(), *arr2 = json_array();
  json_array_append(arr1, arr2);
  json_array_append(arr2, arr1);

In this example, the array ``arr2`` is contained in the array
``arr1``, and vice versa. Jansson cannot check for this kind of
indirect circular references without a performance hit, so it's up to
the user to avoid them.

If a circular reference is created, the memory consumed by the values
cannot be freed by :func:`json_decref()`. The reference counts never
drops to zero because the values are keeping the references to each
other. Moreover, trying to encode the values with any of the encoding
functions will fail. The encoder detects circular references and
returns an error status.

Scope Dereferencing
-------------------

.. versionadded:: 2.9

It is possible to use the ``json_auto_t`` type to automatically
dereference a value at the end of a scope. For example::

  void function(void) {
    json_auto_t *value = NULL;
    value = json_string("foo");
    /* json_decref(value) is automatically called. */
  }

This feature is only available on GCC and Clang. So if your project
has a portability requirement for other compilers, you should avoid
this feature.

Additionally, as always, care should be taken when passing values to
functions that steal references.

True, False and Null
====================

These three values are implemented as singletons, so the returned
pointers won't change between invocations of these functions.

.. function:: json_t *json_true(void)

   .. refcounting:: new

   Returns the JSON true value.

.. function:: json_t *json_false(void)

   .. refcounting:: new

   Returns the JSON false value.

.. function:: json_t *json_boolean(val)

   .. refcounting:: new

   Returns JSON false if ``val`` is zero, and JSON true otherwise.
   This is a macro, and equivalent to ``val ? json_true() :
   json_false()``.

   .. versionadded:: 2.4


.. function:: json_t *json_null(void)

   .. refcounting:: new

   Returns the JSON null value.


String
======

Jansson uses UTF-8 as the character encoding. All JSON strings must be
valid UTF-8 (or ASCII, as it's a subset of UTF-8). All Unicode
codepoints U+0000 through U+10FFFF are allowed, but you must use
length-aware functions if you wish to embed null bytes in strings.

.. function:: json_t *json_string(const char *value)

   .. refcounting:: new

   Returns a new JSON string, or *NULL* on error. *value* must be a
   valid null terminated UTF-8 encoded Unicode string.

.. function:: json_t *json_stringn(const char *value, size_t len)

   .. refcounting:: new

   Like :func:`json_string`, but with explicit length, so *value* may
   contain null characters or not be null terminated.

.. function:: json_t *json_string_nocheck(const char *value)

   .. refcounting:: new

   Like :func:`json_string`, but doesn't check that *value* is valid
   UTF-8. Use this function only if you are certain that this really
   is the case (e.g. you have already checked it by other means).

.. function:: json_t *json_stringn_nocheck(const char *value, size_t len)

   .. refcounting:: new

   Like :func:`json_string_nocheck`, but with explicit length, so
   *value* may contain null characters or not be null terminated.

.. function:: const char *json_string_value(const json_t *string)

   Returns the associated value of *string* as a null terminated UTF-8
   encoded string, or *NULL* if *string* is not a JSON string.

   The returned value is read-only and must not be modified or freed by
   the user. It is valid as long as *string* exists, i.e. as long as
   its reference count has not dropped to zero.

.. function:: size_t json_string_length(const json_t *string)

   Returns the length of *string* in its UTF-8 presentation, or zero
   if *string* is not a JSON string.

.. function:: int json_string_set(json_t *string, const char *value)

   Sets the associated value of *string* to *value*. *value* must be a
   valid UTF-8 encoded Unicode string. Returns 0 on success and -1 on
   error.

.. function:: int json_string_setn(json_t *string, const char *value, size_t len)

   Like :func:`json_string_set`, but with explicit length, so *value*
   may contain null characters or not be null terminated.

.. function:: int json_string_set_nocheck(json_t *string, const char *value)

   Like :func:`json_string_set`, but doesn't check that *value* is
   valid UTF-8. Use this function only if you are certain that this
   really is the case (e.g. you have already checked it by other
   means).

.. function:: int json_string_setn_nocheck(json_t *string, const char *value, size_t len)

   Like :func:`json_string_set_nocheck`, but with explicit length,
   so *value* may contain null characters or not be null terminated.


Number
======

The JSON specification only contains one numeric type, "number". The C
programming language has distinct types for integer and floating-point
numbers, so for practical reasons Jansson also has distinct types for
the two. They are called "integer" and "real", respectively. For more
information, see :ref:`rfc-conformance`.

.. type:: json_int_t

   This is the C type that is used to store JSON integer values. It
   represents the widest integer type available on your system. In
   practice it's just a typedef of ``long long`` if your compiler
   supports it, otherwise ``long``.

   Usually, you can safely use plain ``int`` in place of
   ``json_int_t``, and the implicit C integer conversion handles the
   rest. Only when you know that you need the full 64-bit range, you
   should use ``json_int_t`` explicitly.

``JSON_INTEGER_IS_LONG_LONG``
   This is a preprocessor variable that holds the value 1 if
   :type:`json_int_t` is ``long long``, and 0 if it's ``long``. It
   can be used as follows::

       #if JSON_INTEGER_IS_LONG_LONG
       /* Code specific for long long */
       #else
       /* Code specific for long */
       #endif

``JSON_INTEGER_FORMAT``
   This is a macro that expands to a :func:`printf()` conversion
   specifier that corresponds to :type:`json_int_t`, without the
   leading ``%`` sign, i.e. either ``"lld"`` or ``"ld"``. This macro
   is required because the actual type of :type:`json_int_t` can be
   either ``long`` or ``long long``, and :func:`printf()` requires
   different length modifiers for the two.

   Example::

       json_int_t x = 123123123;
       printf("x is %" JSON_INTEGER_FORMAT "\n", x);


.. function:: json_t *json_integer(json_int_t value)

   .. refcounting:: new

   Returns a new JSON integer, or *NULL* on error.

.. function:: json_int_t json_integer_value(const json_t *integer)

   Returns the associated value of *integer*, or 0 if *json* is not a
   JSON integer.

.. function:: int json_integer_set(const json_t *integer, json_int_t value)

   Sets the associated value of *integer* to *value*. Returns 0 on
   success and -1 if *integer* is not a JSON integer.

.. function:: json_t *json_real(double value)

   .. refcounting:: new

   Returns a new JSON real, or *NULL* on error.

.. function:: double json_real_value(const json_t *real)

   Returns the associated value of *real*, or 0.0 if *real* is not a
   JSON real.

.. function:: int json_real_set(const json_t *real, double value)

   Sets the associated value of *real* to *value*. Returns 0 on
   success and -1 if *real* is not a JSON real.

.. function:: double json_number_value(const json_t *json)

   Returns the associated value of the JSON integer or JSON real
   *json*, cast to double regardless of the actual type. If *json* is
   neither JSON real nor JSON integer, 0.0 is returned.


Array
=====

A JSON array is an ordered collection of other JSON values.

.. function:: json_t *json_array(void)

   .. refcounting:: new

   Returns a new JSON array, or *NULL* on error. Initially, the array
   is empty.

.. function:: size_t json_array_size(const json_t *array)

   Returns the number of elements in *array*, or 0 if *array* is NULL
   or not a JSON array.

.. function:: json_t *json_array_get(const json_t *array, size_t index)

   .. refcounting:: borrow

   Returns the element in *array* at position *index*. The valid range
   for *index* is from 0 to the return value of
   :func:`json_array_size()` minus 1. If *array* is not a JSON array,
   if *array* is *NULL*, or if *index* is out of range, *NULL* is
   returned.

.. function:: int json_array_set(json_t *array, size_t index, json_t *value)

   Replaces the element in *array* at position *index* with *value*.
   The valid range for *index* is from 0 to the return value of
   :func:`json_array_size()` minus 1. Returns 0 on success and -1 on
   error.

.. function:: int json_array_set_new(json_t *array, size_t index, json_t *value)

   Like :func:`json_array_set()` but steals the reference to *value*.
   This is useful when *value* is newly created and not used after
   the call.

.. function:: int json_array_append(json_t *array, json_t *value)

   Appends *value* to the end of *array*, growing the size of *array*
   by 1. Returns 0 on success and -1 on error.

.. function:: int json_array_append_new(json_t *array, json_t *value)

   Like :func:`json_array_append()` but steals the reference to
   *value*. This is useful when *value* is newly created and not used
   after the call.

.. function:: int json_array_insert(json_t *array, size_t index, json_t *value)

   Inserts *value* to *array* at position *index*, shifting the
   elements at *index* and after it one position towards the end of
   the array. Returns 0 on success and -1 on error.

.. function:: int json_array_insert_new(json_t *array, size_t index, json_t *value)

   Like :func:`json_array_insert()` but steals the reference to
   *value*. This is useful when *value* is newly created and not used
   after the call.

.. function:: int json_array_remove(json_t *array, size_t index)

   Removes the element in *array* at position *index*, shifting the
   elements after *index* one position towards the start of the array.
   Returns 0 on success and -1 on error. The reference count of the
   removed value is decremented.

.. function:: int json_array_clear(json_t *array)

   Removes all elements from *array*. Returns 0 on success and -1 on
   error. The reference count of all removed values are decremented.

.. function:: int json_array_extend(json_t *array, json_t *other_array)

   Appends all elements in *other_array* to the end of *array*.
   Returns 0 on success and -1 on error.

.. function:: json_array_foreach(array, index, value)

   Iterate over every element of ``array``, running the block
   of code that follows each time with the proper values set to
   variables ``index`` and ``value``, of types :type:`size_t` and
   :type:`json_t *` respectively. Example::

       /* array is a JSON array */
       size_t index;
       json_t *value;

       json_array_foreach(array, index, value) {
           /* block of code that uses index and value */
       }

   The items are returned in increasing index order.

   This macro expands to an ordinary ``for`` statement upon
   preprocessing, so its performance is equivalent to that of
   hand-written code using the array access functions.
   The main advantage of this macro is that it abstracts
   away the complexity, and makes for more concise and readable code.

   .. versionadded:: 2.5


Object
======

A JSON object is a dictionary of key-value pairs, where the key is a
Unicode string and the value is any JSON value.

Even though null bytes are allowed in string values, they are not
allowed in object keys.

.. function:: json_t *json_object(void)

   .. refcounting:: new

   Returns a new JSON object, or *NULL* on error. Initially, the
   object is empty.

.. function:: size_t json_object_size(const json_t *object)

   Returns the number of elements in *object*, or 0 if *object* is not
   a JSON object.

.. function:: json_t *json_object_get(const json_t *object, const char *key)

   .. refcounting:: borrow

   Get a value corresponding to *key* from *object*. Returns *NULL* if
   *key* is not found and on error.

.. function:: int json_object_set(json_t *object, const char *key, json_t *value)

   Set the value of *key* to *value* in *object*. *key* must be a
   valid null terminated UTF-8 encoded Unicode string. If there
   already is a value for *key*, it is replaced by the new value.
   Returns 0 on success and -1 on error.

.. function:: int json_object_set_nocheck(json_t *object, const char *key, json_t *value)

   Like :func:`json_object_set`, but doesn't check that *key* is
   valid UTF-8. Use this function only if you are certain that this
   really is the case (e.g. you have already checked it by other
   means).

.. function:: int json_object_set_new(json_t *object, const char *key, json_t *value)

   Like :func:`json_object_set()` but steals the reference to
   *value*. This is useful when *value* is newly created and not used
   after the call.

.. function:: int json_object_set_new_nocheck(json_t *object, const char *key, json_t *value)

   Like :func:`json_object_set_new`, but doesn't check that *key* is
   valid UTF-8. Use this function only if you are certain that this
   really is the case (e.g. you have already checked it by other
   means).

.. function:: int json_object_del(json_t *object, const char *key)

   Delete *key* from *object* if it exists. Returns 0 on success, or
   -1 if *key* was not found. The reference count of the removed value
   is decremented.

.. function:: int json_object_clear(json_t *object)

   Remove all elements from *object*. Returns 0 on success and -1 if
   *object* is not a JSON object. The reference count of all removed
   values are decremented.

.. function:: int json_object_update(json_t *object, json_t *other)

   Update *object* with the key-value pairs from *other*, overwriting
   existing keys. Returns 0 on success or -1 on error.

.. function:: int json_object_update_existing(json_t *object, json_t *other)

   Like :func:`json_object_update()`, but only the values of existing
   keys are updated. No new keys are created. Returns 0 on success or
   -1 on error.

   .. versionadded:: 2.3

.. function:: int json_object_update_missing(json_t *object, json_t *other)

   Like :func:`json_object_update()`, but only new keys are created.
   The value of any existing key is not changed. Returns 0 on success
   or -1 on error.

   .. versionadded:: 2.3

.. function:: json_object_foreach(object, key, value)

   Iterate over every key-value pair of ``object``, running the block
   of code that follows each time with the proper values set to
   variables ``key`` and ``value``, of types :type:`const char *` and
   :type:`json_t *` respectively. Example::

       /* obj is a JSON object */
       const char *key;
       json_t *value;

       json_object_foreach(obj, key, value) {
           /* block of code that uses key and value */
       }

   The items are returned in the order they were inserted to the
   object.

   **Note:** It's not safe to call ``json_object_del(object, key)``
   during iteration. If you need to, use
   :func:`json_object_foreach_safe` instead.

   This macro expands to an ordinary ``for`` statement upon
   preprocessing, so its performance is equivalent to that of
   hand-written iteration code using the object iteration protocol
   (see below). The main advantage of this macro is that it abstracts
   away the complexity behind iteration, and makes for more concise and
   readable code.

   .. versionadded:: 2.3


.. function:: json_object_foreach_safe(object, tmp, key, value)

   Like :func:`json_object_foreach()`, but it's safe to call
   ``json_object_del(object, key)`` during iteration. You need to pass
   an extra ``void *`` parameter ``tmp`` that is used for temporary storage.

   .. versionadded:: 2.8


The following functions can be used to iterate through all key-value
pairs in an object. The items are returned in the order they were
inserted to the object.

.. function:: void *json_object_iter(json_t *object)

   Returns an opaque iterator which can be used to iterate over all
   key-value pairs in *object*, or *NULL* if *object* is empty.

.. function:: void *json_object_iter_at(json_t *object, const char *key)

   Like :func:`json_object_iter()`, but returns an iterator to the
   key-value pair in *object* whose key is equal to *key*, or NULL if
   *key* is not found in *object*. Iterating forward to the end of
   *object* only yields all key-value pairs of the object if *key*
   happens to be the first key in the underlying hash table.

.. function:: void *json_object_iter_next(json_t *object, void *iter)

   Returns an iterator pointing to the next key-value pair in *object*
   after *iter*, or *NULL* if the whole object has been iterated
   through.

.. function:: const char *json_object_iter_key(void *iter)

   Extract the associated key from *iter*.

.. function:: json_t *json_object_iter_value(void *iter)

   .. refcounting:: borrow

   Extract the associated value from *iter*.

.. function:: int json_object_iter_set(json_t *object, void *iter, json_t *value)

   Set the value of the key-value pair in *object*, that is pointed to
   by *iter*, to *value*.

.. function:: int json_object_iter_set_new(json_t *object, void *iter, json_t *value)

   Like :func:`json_object_iter_set()`, but steals the reference to
   *value*. This is useful when *value* is newly created and not used
   after the call.

.. function:: void *json_object_key_to_iter(const char *key)

   Like :func:`json_object_iter_at()`, but much faster. Only works for
   values returned by :func:`json_object_iter_key()`. Using other keys
   will lead to segfaults. This function is used internally to
   implement :func:`json_object_foreach`. Example::

     /* obj is a JSON object */
     const char *key;
     json_t *value;
  
     void *iter = json_object_iter(obj);
     while(iter)
     {
         key = json_object_iter_key(iter);
         value = json_object_iter_value(iter);
         /* use key and value ... */
         iter = json_object_iter_next(obj, iter);
     }

   .. versionadded:: 2.3

.. function:: void json_object_seed(size_t seed)

    Seed the hash function used in Jansson's hashtable implementation.
    The seed is used to randomize the hash function so that an
    attacker cannot control its output.

    If *seed* is 0, Jansson generates the seed itself by reading
    random data from the operating system's entropy sources. If no
    entropy sources are available, falls back to using a combination
    of the current timestamp (with microsecond precision if possible)
    and the process ID.

    If called at all, this function must be called before any calls to
    :func:`json_object()`, either explicit or implicit. If this
    function is not called by the user, the first call to
    :func:`json_object()` (either explicit or implicit) seeds the hash
    function. See :ref:`portability-thread-safety` for notes on thread
    safety.

    If repeatable results are required, for e.g. unit tests, the hash
    function can be "unrandomized" by calling :func:`json_object_seed`
    with a constant value on program startup, e.g.
    ``json_object_seed(1)``.

    .. versionadded:: 2.6


Error reporting
===============

Jansson uses a single struct type to pass error information to the
user. See sections :ref:`apiref-decoding`, :ref:`apiref-pack` and
:ref:`apiref-unpack` for functions that pass error information using
this struct.

.. type:: json_error_t

   .. member:: char text[]

      The error message (in UTF-8), or an empty string if a message is
      not available.

   .. member:: char source[]

      Source of the error. This can be (a part of) the file name or a
      special identifier in angle brackets (e.g. ``<string>``).

   .. member:: int line

      The line number on which the error occurred.

   .. member:: int column

      The column on which the error occurred. Note that this is the
      *character column*, not the byte column, i.e. a multibyte UTF-8
      character counts as one column.

   .. member:: int position

      The position in bytes from the start of the input. This is
      useful for debugging Unicode encoding problems.

The normal use of :type:`json_error_t` is to allocate it on the stack,
and pass a pointer to a function. Example::

   int main() {
       json_t *json;
       json_error_t error;

       json = json_load_file("/path/to/file.json", 0, &error);
       if(!json) {
           /* the error variable contains error information */
       }
       ...
   }

Also note that if the call succeeded (``json != NULL`` in the above
example), the contents of ``error`` are generally left unspecified.
The decoding functions write to the ``position`` member also on
success. See :ref:`apiref-decoding` for more info.

All functions also accept *NULL* as the :type:`json_error_t` pointer,
in which case no error information is returned to the caller.


Encoding
========

This sections describes the functions that can be used to encode
values to JSON. By default, only objects and arrays can be encoded
directly, since they are the only valid *root* values of a JSON text.
To encode any JSON value, use the ``JSON_ENCODE_ANY`` flag (see
below).

By default, the output has no newlines, and spaces are used between
array and object elements for a readable output. This behavior can be
altered by using the ``JSON_INDENT`` and ``JSON_COMPACT`` flags
described below. A newline is never appended to the end of the encoded
JSON data.

Each function takes a *flags* parameter that controls some aspects of
how the data is encoded. Its default value is 0. The following macros
can be ORed together to obtain *flags*.

``JSON_INDENT(n)``
   Pretty-print the result, using newlines between array and object
   items, and indenting with *n* spaces. The valid range for *n* is
   between 0 and 31 (inclusive), other values result in an undefined
   output. If ``JSON_INDENT`` is not used or *n* is 0, no newlines are
   inserted between array and object items.

   The ``JSON_MAX_INDENT`` constant defines the maximum indentation
   that can be used, and its value is 31.

   .. versionchanged:: 2.7
      Added ``JSON_MAX_INDENT``.

``JSON_COMPACT``
   This flag enables a compact representation, i.e. sets the separator
   between array and object items to ``","`` and between object keys
   and values to ``":"``. Without this flag, the corresponding
   separators are ``", "`` and ``": "`` for more readable output.

``JSON_ENSURE_ASCII``
   If this flag is used, the output is guaranteed to consist only of
   ASCII characters. This is achieved by escaping all Unicode
   characters outside the ASCII range.

``JSON_SORT_KEYS``
   If this flag is used, all the objects in output are sorted by key.
   This is useful e.g. if two JSON texts are diffed or visually
   compared.

``JSON_PRESERVE_ORDER``
   **Deprecated since version 2.8:** Order of object keys
   is always preserved.

   Prior to version 2.8: If this flag is used, object keys in the
   output are sorted into the same order in which they were first
   inserted to the object. For example, decoding a JSON text and then
   encoding with this flag preserves the order of object keys.

``JSON_ENCODE_ANY``
   Specifying this flag makes it possible to encode any JSON value on
   its own. Without it, only objects and arrays can be passed as the
   *json* value to the encoding functions.

   **Note:** Encoding any value may be useful in some scenarios, but
   it's generally discouraged as it violates strict compatibility with
   :rfc:`4627`. If you use this flag, don't expect interoperability
   with other JSON systems.

   .. versionadded:: 2.1

``JSON_ESCAPE_SLASH``
   Escape the ``/`` characters in strings with ``\/``.

   .. versionadded:: 2.4

``JSON_REAL_PRECISION(n)``
   Output all real numbers with at most *n* digits of precision. The
   valid range for *n* is between 0 and 31 (inclusive), and other
   values result in an undefined behavior.

   By default, the precision is 17, to correctly and losslessly encode
   all IEEE 754 double precision floating point numbers.

   .. versionadded:: 2.7

These functions output UTF-8:

.. function:: char *json_dumps(const json_t *json, size_t flags)

   Returns the JSON representation of *json* as a string, or *NULL* on
   error. *flags* is described above. The return value must be freed
   by the caller using :func:`free()`.

.. function:: int json_dumpf(const json_t *json, FILE *output, size_t flags)

   Write the JSON representation of *json* to the stream *output*.
   *flags* is described above. Returns 0 on success and -1 on error.
   If an error occurs, something may have already been written to
   *output*. In this case, the output is undefined and most likely not
   valid JSON.

.. function:: int json_dump_file(const json_t *json, const char *path, size_t flags)

   Write the JSON representation of *json* to the file *path*. If
   *path* already exists, it is overwritten. *flags* is described
   above. Returns 0 on success and -1 on error.

.. type:: json_dump_callback_t

   A typedef for a function that's called by
   :func:`json_dump_callback()`::

       typedef int (*json_dump_callback_t)(const char *buffer, size_t size, void *data);

   *buffer* points to a buffer containing a chunk of output, *size* is
   the length of the buffer, and *data* is the corresponding
   :func:`json_dump_callback()` argument passed through.

   On error, the function should return -1 to stop the encoding
   process. On success, it should return 0.

   .. versionadded:: 2.2

.. function:: int json_dump_callback(const json_t *json, json_dump_callback_t callback, void *data, size_t flags)

   Call *callback* repeatedly, passing a chunk of the JSON
   representation of *json* each time. *flags* is described above.
   Returns 0 on success and -1 on error.

   .. versionadded:: 2.2


.. _apiref-decoding:

Decoding
========

This sections describes the functions that can be used to decode JSON
text to the Jansson representation of JSON data. The JSON
specification requires that a JSON text is either a serialized array
or object, and this requirement is also enforced with the following
functions. In other words, the top level value in the JSON text being
decoded must be either array or object. To decode any JSON value, use
the ``JSON_DECODE_ANY`` flag (see below).

See :ref:`rfc-conformance` for a discussion on Jansson's conformance
to the JSON specification. It explains many design decisions that
affect especially the behavior of the decoder.

Each function takes a *flags* parameter that can be used to control
the behavior of the decoder. Its default value is 0. The following
macros can be ORed together to obtain *flags*.

``JSON_REJECT_DUPLICATES``
   Issue a decoding error if any JSON object in the input text
   contains duplicate keys. Without this flag, the value of the last
   occurrence of each key ends up in the result. Key equivalence is
   checked byte-by-byte, without special Unicode comparison
   algorithms.

   .. versionadded:: 2.1

``JSON_DECODE_ANY``
   By default, the decoder expects an array or object as the input.
   With this flag enabled, the decoder accepts any valid JSON value.

   **Note:** Decoding any value may be useful in some scenarios, but
   it's generally discouraged as it violates strict compatibility with
   :rfc:`4627`. If you use this flag, don't expect interoperability
   with other JSON systems.

   .. versionadded:: 2.3

``JSON_DISABLE_EOF_CHECK``
   By default, the decoder expects that its whole input constitutes a
   valid JSON text, and issues an error if there's extra data after
   the otherwise valid JSON input. With this flag enabled, the decoder
   stops after decoding a valid JSON array or object, and thus allows
   extra data after the JSON text.

   Normally, reading will stop when the last ``]`` or ``}`` in the
   JSON input is encountered. If both ``JSON_DISABLE_EOF_CHECK`` and
   ``JSON_DECODE_ANY`` flags are used, the decoder may read one extra
   UTF-8 code unit (up to 4 bytes of input). For example, decoding
   ``4true`` correctly decodes the integer 4, but also reads the
   ``t``. For this reason, if reading multiple consecutive values that
   are not arrays or objects, they should be separated by at least one
   whitespace character.

   .. versionadded:: 2.1

``JSON_DECODE_INT_AS_REAL``
   JSON defines only one number type. Jansson distinguishes between
   ints and reals. For more information see :ref:`real-vs-integer`.
   With this flag enabled the decoder interprets all numbers as real
   values. Integers that do not have an exact double representation
   will silently result in a loss of precision. Integers that cause
   a double overflow will cause an error.

   .. versionadded:: 2.5

``JSON_ALLOW_NUL``
   Allow ``\u0000`` escape inside string values. This is a safety
   measure; If you know your input can contain null bytes, use this
   flag. If you don't use this flag, you don't have to worry about null
   bytes inside strings unless you explicitly create themselves by
   using e.g. :func:`json_stringn()` or ``s#`` format specifier for
   :func:`json_pack()`.

   Object keys cannot have embedded null bytes even if this flag is
   used.

   .. versionadded:: 2.6

Each function also takes an optional :type:`json_error_t` parameter
that is filled with error information if decoding fails. It's also
updated on success; the number of bytes of input read is written to
its ``position`` field. This is especially useful when using
``JSON_DISABLE_EOF_CHECK`` to read multiple consecutive JSON texts.

.. versionadded:: 2.3
   Number of bytes of input read is written to the ``position`` field
   of the :type:`json_error_t` structure.

If no error or position information is needed, you can pass *NULL*.

.. function:: json_t *json_loads(const char *input, size_t flags, json_error_t *error)

   .. refcounting:: new

   Decodes the JSON string *input* and returns the array or object it
   contains, or *NULL* on error, in which case *error* is filled with
   information about the error. *flags* is described above.

.. function:: json_t *json_loadb(const char *buffer, size_t buflen, size_t flags, json_error_t *error)

   .. refcounting:: new

   Decodes the JSON string *buffer*, whose length is *buflen*, and
   returns the array or object it contains, or *NULL* on error, in
   which case *error* is filled with information about the error. This
   is similar to :func:`json_loads()` except that the string doesn't
   need to be null-terminated. *flags* is described above.

   .. versionadded:: 2.1

.. function:: json_t *json_loadf(FILE *input, size_t flags, json_error_t *error)

   .. refcounting:: new

   Decodes the JSON text in stream *input* and returns the array or
   object it contains, or *NULL* on error, in which case *error* is
   filled with information about the error. *flags* is described
   above.

   This function will start reading the input from whatever position
   the input file was, without attempting to seek first. If an error
   occurs, the file position will be left indeterminate. On success,
   the file position will be at EOF, unless ``JSON_DISABLE_EOF_CHECK``
   flag was used. In this case, the file position will be at the first
   character after the last ``]`` or ``}`` in the JSON input. This
   allows calling :func:`json_loadf()` on the same ``FILE`` object
   multiple times, if the input consists of consecutive JSON texts,
   possibly separated by whitespace.

.. function:: json_t *json_load_file(const char *path, size_t flags, json_error_t *error)

   .. refcounting:: new

   Decodes the JSON text in file *path* and returns the array or
   object it contains, or *NULL* on error, in which case *error* is
   filled with information about the error. *flags* is described
   above.

.. type:: json_load_callback_t

   A typedef for a function that's called by
   :func:`json_load_callback()` to read a chunk of input data::

       typedef size_t (*json_load_callback_t)(void *buffer, size_t buflen, void *data);

   *buffer* points to a buffer of *buflen* bytes, and *data* is the
   corresponding :func:`json_load_callback()` argument passed through.

   On success, the function should return the number of bytes read; a
   returned value of 0 indicates that no data was read and that the
   end of file has been reached. On error, the function should return
   ``(size_t)-1`` to abort the decoding process.

   .. versionadded:: 2.4

.. function:: json_t *json_load_callback(json_load_callback_t callback, void *data, size_t flags, json_error_t *error)

   .. refcounting:: new

   Decodes the JSON text produced by repeated calls to *callback*, and
   returns the array or object it contains, or *NULL* on error, in
   which case *error* is filled with information about the error.
   *data* is passed through to *callback* on each call. *flags* is
   described above.

   .. versionadded:: 2.4


.. _apiref-pack:

Building Values
===============

This section describes functions that help to create, or *pack*,
complex JSON values, especially nested objects and arrays. Value
building is based on a *format string* that is used to tell the
functions about the expected arguments.

For example, the format string ``"i"`` specifies a single integer
value, while the format string ``"[ssb]"`` or the equivalent ``"[s, s,
b]"`` specifies an array value with two strings and a boolean as its
items::

    /* Create the JSON integer 42 */
    json_pack("i", 42);

    /* Create the JSON array ["foo", "bar", true] */
    json_pack("[ssb]", "foo", "bar", 1);

Here's the full list of format specifiers. The type in parentheses
denotes the resulting JSON type, and the type in brackets (if any)
denotes the C type that is expected as the corresponding argument or
arguments.

``s`` (string) [const char \*]
    Convert a null terminated UTF-8 string to a JSON string.

``s?`` (string) [const char \*]
    Like ``s``, but if the argument is *NULL*, output a JSON null
    value.

    .. versionadded:: 2.8

``s#`` (string) [const char \*, int]
    Convert a UTF-8 buffer of a given length to a JSON string.

    .. versionadded:: 2.5

``s%`` (string) [const char \*, size_t]
    Like ``s#`` but the length argument is of type :type:`size_t`.

    .. versionadded:: 2.6

``+`` [const char \*]
    Like ``s``, but concatenate to the previous string. Only valid
    after ``s``, ``s#``, ``+`` or ``+#``.

    .. versionadded:: 2.5

``+#`` [const char \*, int]
    Like ``s#``, but concatenate to the previous string. Only valid
    after ``s``, ``s#``, ``+`` or ``+#``.

    .. versionadded:: 2.5

``+%`` (string) [const char \*, size_t]
    Like ``+#`` but the length argument is of type :type:`size_t`.

    .. versionadded:: 2.6

``n`` (null)
    Output a JSON null value. No argument is consumed.

``b`` (boolean) [int]
    Convert a C :type:`int` to JSON boolean value. Zero is converted
    to ``false`` and non-zero to ``true``.

``i`` (integer) [int]
    Convert a C :type:`int` to JSON integer.

``I`` (integer) [json_int_t]
    Convert a C :type:`json_int_t` to JSON integer.

``f`` (real) [double]
    Convert a C :type:`double` to JSON real.

``o`` (any value) [json_t \*]
    Output any given JSON value as-is. If the value is added to an
    array or object, the reference to the value passed to ``o`` is
    stolen by the container.

``O`` (any value) [json_t \*]
    Like ``o``, but the argument's reference count is incremented.
    This is useful if you pack into an array or object and want to
    keep the reference for the JSON value consumed by ``O`` to
    yourself.

``o?``, ``O?`` (any value) [json_t \*]
    Like ``o`` and ``O?``, respectively, but if the argument is
    *NULL*, output a JSON null value.

    .. versionadded:: 2.8

``[fmt]`` (array)
    Build an array with contents from the inner format string. ``fmt``
    may contain objects and arrays, i.e. recursive value building is
    supported.

``{fmt}`` (object)
    Build an object with contents from the inner format string
    ``fmt``. The first, third, etc. format specifier represent a key,
    and must be a string (see ``s``, ``s#``, ``+`` and ``+#`` above),
    as object keys are always strings. The second, fourth, etc. format
    specifier represent a value. Any value may be an object or array,
    i.e. recursive value building is supported.

Whitespace, ``:`` and ``,`` are ignored.

.. function:: json_t *json_pack(const char *fmt, ...)

   .. refcounting:: new

   Build a new JSON value according to the format string *fmt*. For
   each format specifier (except for ``{}[]n``), one or more arguments
   are consumed and used to build the corresponding value. Returns
   *NULL* on error.

.. function:: json_t *json_pack_ex(json_error_t *error, size_t flags, const char *fmt, ...)
              json_t *json_vpack_ex(json_error_t *error, size_t flags, const char *fmt, va_list ap)

   .. refcounting:: new

   Like :func:`json_pack()`, but an in the case of an error, an error
   message is written to *error*, if it's not *NULL*. The *flags*
   parameter is currently unused and should be set to 0.

   As only the errors in format string (and out-of-memory errors) can
   be caught by the packer, these two functions are most likely only
   useful for debugging format strings.

More examples::

  /* Build an empty JSON object */
  json_pack("{}");

  /* Build the JSON object {"foo": 42, "bar": 7} */
  json_pack("{sisi}", "foo", 42, "bar", 7);

  /* Like above, ':', ',' and whitespace are ignored */
  json_pack("{s:i, s:i}", "foo", 42, "bar", 7);

  /* Build the JSON array [[1, 2], {"cool": true}] */
  json_pack("[[i,i],{s:b}]", 1, 2, "cool", 1);

  /* Build a string from a non-null terminated buffer */
  char buffer[4] = {'t', 'e', 's', 't'};
  json_pack("s#", buffer, 4);

  /* Concatenate strings together to build the JSON string "foobarbaz" */
  json_pack("s++", "foo", "bar", "baz");


.. _apiref-unpack:

Parsing and Validating Values
=============================

This section describes functions that help to validate complex values
and extract, or *unpack*, data from them. Like :ref:`building values
<apiref-pack>`, this is also based on format strings.

While a JSON value is unpacked, the type specified in the format
string is checked to match that of the JSON value. This is the
validation part of the process. In addition to this, the unpacking
functions can also check that all items of arrays and objects are
unpacked. This check be enabled with the format specifier ``!`` or by
using the flag ``JSON_STRICT``. See below for details.

Here's the full list of format specifiers. The type in parentheses
denotes the JSON type, and the type in brackets (if any) denotes the C
type whose address should be passed.

``s`` (string) [const char \*]
    Convert a JSON string to a pointer to a null terminated UTF-8
    string. The resulting string is extracted by using
    :func:`json_string_value()` internally, so it exists as long as
    there are still references to the corresponding JSON string.

``s%`` (string) [const char \*, size_t \*]
    Convert a JSON string to a pointer to a null terminated UTF-8
    string and its length.

    .. versionadded:: 2.6

``n`` (null)
    Expect a JSON null value. Nothing is extracted.

``b`` (boolean) [int]
    Convert a JSON boolean value to a C :type:`int`, so that ``true``
    is converted to 1 and ``false`` to 0.

``i`` (integer) [int]
    Convert a JSON integer to C :type:`int`.

``I`` (integer) [json_int_t]
    Convert a JSON integer to C :type:`json_int_t`.

``f`` (real) [double]
    Convert a JSON real to C :type:`double`.

``F`` (integer or real) [double]
    Convert a JSON number (integer or real) to C :type:`double`.

``o`` (any value) [json_t \*]
    Store a JSON value with no conversion to a :type:`json_t` pointer.

``O`` (any value) [json_t \*]
    Like ``O``, but the JSON value's reference count is incremented.

``[fmt]`` (array)
    Convert each item in the JSON array according to the inner format
    string. ``fmt`` may contain objects and arrays, i.e. recursive
    value extraction is supported.

``{fmt}`` (object)
    Convert each item in the JSON object according to the inner format
    string ``fmt``. The first, third, etc. format specifier represent
    a key, and must be ``s``. The corresponding argument to unpack
    functions is read as the object key. The second fourth, etc.
    format specifier represent a value and is written to the address
    given as the corresponding argument. **Note** that every other
    argument is read from and every other is written to.

    ``fmt`` may contain objects and arrays as values, i.e. recursive
    value extraction is supported.

    .. versionadded:: 2.3
       Any ``s`` representing a key may be suffixed with a ``?`` to
       make the key optional. If the key is not found, nothing is
       extracted. See below for an example.

``!``
    This special format specifier is used to enable the check that
    all object and array items are accessed, on a per-value basis. It
    must appear inside an array or object as the last format specifier
    before the closing bracket or brace. To enable the check globally,
    use the ``JSON_STRICT`` unpacking flag.

``*``
    This special format specifier is the opposite of ``!``. If the
    ``JSON_STRICT`` flag is used, ``*`` can be used to disable the
    strict check on a per-value basis. It must appear inside an array
    or object as the last format specifier before the closing bracket
    or brace.

Whitespace, ``:`` and ``,`` are ignored.

.. function:: int json_unpack(json_t *root, const char *fmt, ...)

   Validate and unpack the JSON value *root* according to the format
   string *fmt*. Returns 0 on success and -1 on failure.

.. function:: int json_unpack_ex(json_t *root, json_error_t *error, size_t flags, const char *fmt, ...)
              int json_vunpack_ex(json_t *root, json_error_t *error, size_t flags, const char *fmt, va_list ap)

   Validate and unpack the JSON value *root* according to the format
   string *fmt*. If an error occurs and *error* is not *NULL*, write
   error information to *error*. *flags* can be used to control the
   behaviour of the unpacker, see below for the flags. Returns 0 on
   success and -1 on failure.

.. note::

   The first argument of all unpack functions is ``json_t *root``
   instead of ``const json_t *root``, because the use of ``O`` format
   specifier causes the reference count of ``root``, or some value
   reachable from ``root``, to be increased. Furthermore, the ``o``
   format specifier may be used to extract a value as-is, which allows
   modifying the structure or contents of a value reachable from
   ``root``.

   If the ``O`` and ``o`` format specifiers are not used, it's
   perfectly safe to cast a ``const json_t *`` variable to plain
   ``json_t *`` when used with these functions.

The following unpacking flags are available:

``JSON_STRICT``
    Enable the extra validation step checking that all object and
    array items are unpacked. This is equivalent to appending the
    format specifier ``!`` to the end of every array and object in the
    format string.

``JSON_VALIDATE_ONLY``
    Don't extract any data, just validate the JSON value against the
    given format string. Note that object keys must still be specified
    after the format string.

Examples::

    /* root is the JSON integer 42 */
    int myint;
    json_unpack(root, "i", &myint);
    assert(myint == 42);

    /* root is the JSON object {"foo": "bar", "quux": true} */
    const char *str;
    int boolean;
    json_unpack(root, "{s:s, s:b}", "foo", &str, "quux", &boolean);
    assert(strcmp(str, "bar") == 0 && boolean == 1);

    /* root is the JSON array [[1, 2], {"baz": null} */
    json_error_t error;
    json_unpack_ex(root, &error, JSON_VALIDATE_ONLY, "[[i,i], {s:n}]", "baz");
    /* returns 0 for validation success, nothing is extracted */

    /* root is the JSON array [1, 2, 3, 4, 5] */
    int myint1, myint2;
    json_unpack(root, "[ii!]", &myint1, &myint2);
    /* returns -1 for failed validation */

    /* root is an empty JSON object */
    int myint = 0, myint2 = 0, myint3 = 0;
    json_unpack(root, "{s?i, s?[ii]}",
                "foo", &myint1,
                "bar", &myint2, &myint3);
    /* myint1, myint2 or myint3 is no touched as "foo" and "bar" don't exist */


Equality
========

Testing for equality of two JSON values cannot, in general, be
achieved using the ``==`` operator. Equality in the terms of the
``==`` operator states that the two :type:`json_t` pointers point to
exactly the same JSON value. However, two JSON values can be equal not
only if they are exactly the same value, but also if they have equal
"contents":

* Two integer or real values are equal if their contained numeric
  values are equal. An integer value is never equal to a real value,
  though.

* Two strings are equal if their contained UTF-8 strings are equal,
  byte by byte. Unicode comparison algorithms are not implemented.

* Two arrays are equal if they have the same number of elements and
  each element in the first array is equal to the corresponding
  element in the second array.

* Two objects are equal if they have exactly the same keys and the
  value for each key in the first object is equal to the value of the
  corresponding key in the second object.

* Two true, false or null values have no "contents", so they are equal
  if their types are equal. (Because these values are singletons,
  their equality can actually be tested with ``==``.)

.. function:: int json_equal(json_t *value1, json_t *value2)

   Returns 1 if *value1* and *value2* are equal, as defined above.
   Returns 0 if they are unequal or one or both of the pointers are
   *NULL*.


Copying
=======

Because of reference counting, passing JSON values around doesn't
require copying them. But sometimes a fresh copy of a JSON value is
needed. For example, if you need to modify an array, but still want to
use the original afterwards, you should take a copy of it first.

Jansson supports two kinds of copying: shallow and deep. There is a
difference between these methods only for arrays and objects. Shallow
copying only copies the first level value (array or object) and uses
the same child values in the copied value. Deep copying makes a fresh
copy of the child values, too. Moreover, all the child values are deep
copied in a recursive fashion.

Copying objects preserves the insertion order of keys.

.. function:: json_t *json_copy(json_t *value)

   .. refcounting:: new

   Returns a shallow copy of *value*, or *NULL* on error.

.. function:: json_t *json_deep_copy(const json_t *value)

   .. refcounting:: new

   Returns a deep copy of *value*, or *NULL* on error.


.. _apiref-custom-memory-allocation:

Custom Memory Allocation
========================

By default, Jansson uses :func:`malloc()` and :func:`free()` for
memory allocation. These functions can be overridden if custom
behavior is needed.

.. type:: json_malloc_t

   A typedef for a function pointer with :func:`malloc()`'s
   signature::

       typedef void *(*json_malloc_t)(size_t);

.. type:: json_free_t

   A typedef for a function pointer with :func:`free()`'s
   signature::

       typedef void (*json_free_t)(void *);

.. function:: void json_set_alloc_funcs(json_malloc_t malloc_fn, json_free_t free_fn)

   Use *malloc_fn* instead of :func:`malloc()` and *free_fn* instead
   of :func:`free()`. This function has to be called before any other
   Jansson's API functions to ensure that all memory operations use
   the same functions.

.. function:: void json_get_alloc_funcs(json_malloc_t *malloc_fn, json_free_t *free_fn)

   Fetch the current malloc_fn and free_fn used. Either parameter
   may be NULL.

   .. versionadded:: 2.8

**Examples:**

Circumvent problems with different CRT heaps on Windows by using
application's :func:`malloc()` and :func:`free()`::

    json_set_alloc_funcs(malloc, free);

Use the `Boehm's conservative garbage collector`_ for memory
operations::

    json_set_alloc_funcs(GC_malloc, GC_free);

.. _Boehm's conservative garbage collector: http://www.hboehm.info/gc/

Allow storing sensitive data (e.g. passwords or encryption keys) in
JSON structures by zeroing all memory when freed::

    static void *secure_malloc(size_t size)
    {
        /* Store the memory area size in the beginning of the block */
        void *ptr = malloc(size + 8);
        *((size_t *)ptr) = size;
        return ptr + 8;
    }

    static void secure_free(void *ptr)
    {
        size_t size;

        ptr -= 8;
        size = *((size_t *)ptr);

        guaranteed_memset(ptr, 0, size + 8);
        free(ptr);
    }

    int main()
    {
        json_set_alloc_funcs(secure_malloc, secure_free);
        /* ... */
    }

For more information about the issues of storing sensitive data in
memory, see
http://www.dwheeler.com/secure-programs/Secure-Programs-HOWTO/protect-secrets.html.
The page also explains the :func:`guaranteed_memset()` function used
in the example and gives a sample implementation for it.
