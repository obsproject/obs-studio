/*
 * Copyright (c) 2009-2016 Petri Lehtinen <petri@digip.org>
 *
 * Jansson is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#ifndef JANSSON_PRIVATE_H
#define JANSSON_PRIVATE_H

#include <stddef.h>
#include "jansson.h"
#include "hashtable.h"
#include "strbuffer.h"

#define container_of(ptr_, type_, member_)  \
    ((type_ *)((char *)ptr_ - offsetof(type_, member_)))

/* On some platforms, max() may already be defined */
#ifndef max
#define max(a, b)  ((a) > (b) ? (a) : (b))
#endif

/* va_copy is a C99 feature. In C89 implementations, it's sometimes
   available as __va_copy. If not, memcpy() should do the trick. */
#ifndef va_copy
#ifdef __va_copy
#define va_copy __va_copy
#else
#define va_copy(a, b)  memcpy(&(a), &(b), sizeof(va_list))
#endif
#endif

typedef struct {
    json_t json;
    hashtable_t hashtable;
    int visited;
} json_object_t;

typedef struct {
    json_t json;
    size_t size;
    size_t entries;
    json_t **table;
    int visited;
} json_array_t;

typedef struct {
    json_t json;
    char *value;
    size_t length;
} json_string_t;

typedef struct {
    json_t json;
    double value;
} json_real_t;

typedef struct {
    json_t json;
    json_int_t value;
} json_integer_t;

#define json_to_object(json_)  container_of(json_, json_object_t, json)
#define json_to_array(json_)   container_of(json_, json_array_t, json)
#define json_to_string(json_)  container_of(json_, json_string_t, json)
#define json_to_real(json_)    container_of(json_, json_real_t, json)
#define json_to_integer(json_) container_of(json_, json_integer_t, json)

/* Create a string by taking ownership of an existing buffer */
json_t *jsonp_stringn_nocheck_own(const char *value, size_t len);

/* Error message formatting */
void jsonp_error_init(json_error_t *error, const char *source);
void jsonp_error_set_source(json_error_t *error, const char *source);
void jsonp_error_set(json_error_t *error, int line, int column,
                     size_t position, const char *msg, ...);
void jsonp_error_vset(json_error_t *error, int line, int column,
                      size_t position, const char *msg, va_list ap);

/* Locale independent string<->double conversions */
int jsonp_strtod(strbuffer_t *strbuffer, double *out);
int jsonp_dtostr(char *buffer, size_t size, double value, int prec);

/* Wrappers for custom memory functions */
void* jsonp_malloc(size_t size);
void jsonp_free(void *ptr);
char *jsonp_strndup(const char *str, size_t length);
char *jsonp_strdup(const char *str);
char *jsonp_strndup(const char *str, size_t len);


/* Windows compatibility */
#if defined(_WIN32) || defined(WIN32)
#  if defined(_MSC_VER)  /* MS compiller */
#    if (_MSC_VER < 1900) && !defined(snprintf)  /* snprintf not defined yet & not introduced */
#      define snprintf _snprintf
#    endif
#    if (_MSC_VER < 1500) && !defined(vsnprintf)  /* vsnprintf not defined yet & not introduced */
#      define vsnprintf(b,c,f,a) _vsnprintf(b,c,f,a)
#    endif
#  else  /* Other Windows compiller, old definition */
#    define snprintf _snprintf
#    define vsnprintf _vsnprintf
#  endif
#endif

#endif
