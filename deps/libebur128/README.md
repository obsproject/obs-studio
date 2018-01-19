libebur128
==========

libebur128 is a library that implements the EBU R 128 standard for loudness
normalisation.

All source code is licensed under the MIT license. See COPYING file for
details.

See also [loudness-scanner tool](https://github.com/jiixyj/loudness-scanner).

News
----

v1.2.3 released:
 * Fix uninitialized memory access during true peak scanning (bug #72)

v1.2.2 released (v1.2.1 was mistagged):
 * Fix a null pointer dereference when doing true peak scanning of 192kHz data

v1.2.0 released:

 * New functions for real time loudness/peak monitoring:
   * `ebur128_loudness_window()`
   * `ebur128_set_max_window()`
   * `ebur128_set_max_history()`
   * `ebur128_prev_sample_peak()`
   * `ebur128_prev_true_peak()`
 * New FIR resampler for true peak calculation, removing Speex dependency
 * Add true peak conformance tests
 * Bug fixes

v1.1.0 released:

 * Add `ebur128_relative_threshold()`
 * Add channel definitions from ITU R-REC-BS 1770-4 to channel enum
 * Fix some minor build issues

v1.0.3 released:

 * Fix build with recent speexdsp
 * Correct license file name
 * CMake option to disable static library
 * minimal-example.c: do not hard code program name in usage

Features
--------

* Portable ANSI C code
* Implements M, S and I modes
* Implements loudness range measurement (EBU - TECH 3342)
* True peak scanning
* Supports all samplerates by recalculation of the filter coefficients

Installation
------------

In the root folder, type:

    mkdir build
    cd build
    cmake ..
    make

If you want the git version, run simply:

    git clone git://github.com/jiixyj/libebur128.git

Usage
-----

Library usage should be pretty straightforward. All exported symbols are
documented in the ebur128.h header file. For a usage example, see
minimal-example.c in the tests folder.

On some operating systems, static libraries should be compiled as position
independent code. You can enable that by turning on `WITH_STATIC_PIC`.
