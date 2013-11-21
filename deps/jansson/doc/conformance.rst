.. _rfc-conformance:

***************
RFC Conformance
***************

JSON is specified in :rfc:`4627`, *"The application/json Media Type
for JavaScript Object Notation (JSON)"*.

Character Encoding
==================

Jansson only supports UTF-8 encoded JSON texts. It does not support or
auto-detect any of the other encodings mentioned in the RFC, namely
UTF-16LE, UTF-16BE, UTF-32LE or UTF-32BE. Pure ASCII is supported, as
it's a subset of UTF-8.

Strings
=======

JSON strings are mapped to C-style null-terminated character arrays,
and UTF-8 encoding is used internally.

All Unicode codepoints U+0000 through U+10FFFF are allowed in string
values. However, U+0000 is not allowed in object keys because of API
restrictions.

Unicode normalization or any other transformation is never performed
on any strings (string values or object keys). When checking for
equivalence of strings or object keys, the comparison is performed
byte by byte between the original UTF-8 representations of the
strings.

Numbers
=======

.. _real-vs-integer:

Real vs. Integer
----------------

JSON makes no distinction between real and integer numbers; Jansson
does. Real numbers are mapped to the ``double`` type and integers to
the ``json_int_t`` type, which is a typedef of ``long long`` or
``long``, depending on whether ``long long`` is supported by your
compiler or not.

A JSON number is considered to be a real number if its lexical
representation includes one of ``e``, ``E``, or ``.``; regardless if
its actual numeric value is a true integer (e.g., all of ``1E6``,
``3.0``, ``400E-2``, and ``3.14E3`` are mathematical integers, but
will be treated as real values). With the ``JSON_DECODE_INT_AS_REAL``
decoder flag set all numbers are interpreted as real.

All other JSON numbers are considered integers.

When encoding to JSON, real values are always represented
with a fractional part; e.g., the ``double`` value 3.0 will be
represented in JSON as ``3.0``, not ``3``.

Overflow, Underflow & Precision
-------------------------------

Real numbers whose absolute values are too small to be represented in
a C ``double`` will be silently estimated with 0.0. Thus, depending on
platform, JSON numbers very close to zero such as 1E-999 may result in
0.0.

Real numbers whose absolute values are too large to be represented in
a C ``double`` will result in an overflow error (a JSON decoding
error). Thus, depending on platform, JSON numbers like 1E+999 or
-1E+999 may result in a parsing error.

Likewise, integer numbers whose absolute values are too large to be
represented in the ``json_int_t`` type (see above) will result in an
overflow error (a JSON decoding error). Thus, depending on platform,
JSON numbers like 1000000000000000 may result in parsing error.

Parsing JSON real numbers may result in a loss of precision. As long
as overflow does not occur (i.e. a total loss of precision), the
rounded approximate value is silently used. Thus the JSON number
1.000000000000000005 may, depending on platform, result in the
``double`` value 1.0.

Signed zeros
------------

JSON makes no statement about what a number means; however Javascript
(ECMAscript) does state that +0.0 and -0.0 must be treated as being
distinct values, i.e. -0.0 |not-equal| 0.0. Jansson relies on the
underlying floating point library in the C environment in which it is
compiled. Therefore it is platform-dependent whether 0.0 and -0.0 will
be distinct values. Most platforms that use the IEEE 754
floating-point standard will support signed zeros.

Note that this only applies to floating-point; neither JSON, C, or
IEEE support the concept of signed integer zeros.

.. |not-equal| unicode:: U+2260

Types
-----

No support is provided in Jansson for any C numeric types other than
``json_int_t`` and ``double``. This excludes things such as unsigned
types, ``long double``, etc. Obviously, shorter types like ``short``,
``int``, ``long`` (if ``json_int_t`` is ``long long``) and ``float``
are implicitly handled via the ordinary C type coercion rules (subject
to overflow semantics). Also, no support or hooks are provided for any
supplemental "bignum" type add-on packages.
