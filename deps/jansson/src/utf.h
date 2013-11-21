/*
 * Copyright (c) 2009-2013 Petri Lehtinen <petri@digip.org>
 *
 * Jansson is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#ifndef UTF_H
#define UTF_H

#ifdef HAVE_CONFIG_H
#include <config.h>

#ifdef HAVE_INTTYPES_H
/* inttypes.h includes stdint.h in a standard environment, so there's
no need to include stdint.h separately. If inttypes.h doesn't define
int32_t, it's defined in config.h. */
#include <inttypes.h>
#endif /* HAVE_INTTYPES_H */

#else /* !HAVE_CONFIG_H */
#ifdef _WIN32
typedef int int32_t;
#else /* !_WIN32 */
/* Assume a standard environment */
#include <inttypes.h>
#endif /* _WIN32 */

#endif /* HAVE_CONFIG_H */

int utf8_encode(int32_t codepoint, char *buffer, size_t *size);

size_t utf8_check_first(char byte);
size_t utf8_check_full(const char *buffer, size_t size, int32_t *codepoint);
const char *utf8_iterate(const char *buffer, size_t size, int32_t *codepoint);

int utf8_check_string(const char *string, size_t length);

#endif
