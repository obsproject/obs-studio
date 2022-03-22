/*
 * Copyright (c) 2009-2016 Petri Lehtinen <petri@digip.org>
 *
 * Jansson is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#ifndef UTF_H
#define UTF_H

#ifdef HAVE_CONFIG_H
#include <jansson_private_config.h>
#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

int utf8_encode(int32_t codepoint, char *buffer, size_t *size);

size_t utf8_check_first(char byte);
size_t utf8_check_full(const char *buffer, size_t size, int32_t *codepoint);
const char *utf8_iterate(const char *buffer, size_t size, int32_t *codepoint);

int utf8_check_string(const char *string, size_t length);

#endif
