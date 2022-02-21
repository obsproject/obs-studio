/*
 * Copyright (c) 2010-2016 Petri Lehtinen <petri@digip.org>
 *
 * Jansson is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 *
 *
 * This file specifies a part of the site-specific configuration for
 * Jansson, namely those things that affect the public API in
 * jansson.h.
 *
 * The CMake system will generate the jansson_config.h file and
 * copy it to the build and install directories.
 */

#ifndef JANSSON_CONFIG_H
#define JANSSON_CONFIG_H

/* Define this so that we can disable scattered automake configuration in source files */
#ifndef JANSSON_USING_CMAKE
#define JANSSON_USING_CMAKE
#endif

/* Note: when using cmake, JSON_INTEGER_IS_LONG_LONG is not defined nor used,
 * as we will also check for __int64 etc types.
 * (the definition was used in the automake system) */

/* Bring in the cmake-detected defines */
#cmakedefine HAVE_STDINT_H 1
#cmakedefine HAVE_INTTYPES_H 1
#cmakedefine HAVE_SYS_TYPES_H 1

/* Include our standard type header for the integer typedef */

#if defined(HAVE_STDINT_H)
#  include <stdint.h>
#elif defined(HAVE_INTTYPES_H)
#  include <inttypes.h>
#elif defined(HAVE_SYS_TYPES_H)
#  include <sys/types.h>
#endif


/* If your compiler supports the inline keyword in C, JSON_INLINE is
   defined to `inline', otherwise empty. In C++, the inline is always
   supported. */
#ifdef __cplusplus
#define JSON_INLINE inline
#else
#define JSON_INLINE @JSON_INLINE@
#endif


#define json_int_t @JSON_INT_T@
#define json_strtoint @JSON_STRTOINT@
#define JSON_INTEGER_FORMAT @JSON_INTEGER_FORMAT@


/* If locale.h and localeconv() are available, define to 1, otherwise to 0. */
#define JSON_HAVE_LOCALECONV @JSON_HAVE_LOCALECONV@


/* Maximum recursion depth for parsing JSON input.
   This limits the depth of e.g. array-within-array constructions. */
#define JSON_PARSER_MAX_DEPTH 2048


#endif
