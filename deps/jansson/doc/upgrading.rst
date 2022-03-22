.. highlight:: c

******************
Upgrading from 1.x
******************

This chapter lists the backwards incompatible changes introduced in
Jansson 2.0, and the steps that are needed for upgrading your code.

**The incompatibilities are not dramatic.** The biggest change is that
all decoding functions now require and extra parameter. Most programs
can be modified to work with 2.0 by adding a ``0`` as the second
parameter to all calls of :func:`json_loads()`, :func:`json_loadf()`
and :func:`json_load_file()`.


Compatibility
=============

Jansson 2.0 is backwards incompatible with the Jansson 1.x releases.
It is ABI incompatible, i.e. all programs dynamically linking to the
Jansson library need to be recompiled. It's also API incompatible,
i.e. the source code of programs using Jansson 1.x may need
modifications to make them compile against Jansson 2.0.

All the 2.x releases are guaranteed to be backwards compatible for
both ABI and API, so no recompilation or source changes are needed
when upgrading from 2.x to 2.y.


List of Incompatible Changes
============================

**Decoding flags**
    For future needs, a ``flags`` parameter was added as the second
    parameter to all decoding functions, i.e. :func:`json_loads()`,
    :func:`json_loadf()` and :func:`json_load_file()`. All calls to
    these functions need to be changed by adding a ``0`` as the second
    argument. For example::

        /* old code */
        json_loads(input, &error);

        /* new code */
        json_loads(input, 0, &error);


**Underlying type of JSON integers**
    The underlying C type of JSON integers has been changed from
    :type:`int` to the widest available signed integer type, i.e.
    :type:`long long` or :type:`long`, depending on whether
    :type:`long long` is supported on your system or not. This makes
    the whole 64-bit integer range available on most modern systems.

    ``jansson.h`` has a typedef :type:`json_int_t` to the underlying
    integer type. :type:`int` should still be used in most cases when
    dealing with smallish JSON integers, as the compiler handles
    implicit type coercion. Only when the full 64-bit range is needed,
    :type:`json_int_t` should be explicitly used.


**Maximum encoder indentation depth**
    The maximum argument of the ``JSON_INDENT()`` macro has been
    changed from 255 to 31, to free up bits from the ``flags``
    parameter of :func:`json_dumps()`, :func:`json_dumpf()` and
    :func:`json_dump_file()`. If your code uses a bigger indentation
    than 31, it needs to be changed.


**Unsigned integers in API functions**
    Version 2.0 unifies unsigned integer usage in the API. All uses of
    :type:`unsigned int` and :type:`unsigned long` have been replaced
    with :type:`size_t`. This includes flags, container sizes, etc.
    This should not require source code changes, as both
    :type:`unsigned int` and :type:`unsigned long` are usually
    compatible with :type:`size_t`.
