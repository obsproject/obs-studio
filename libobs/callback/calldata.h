/*
 * Copyright (c) 2013 Hugh Bailey <obs.jim@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#pragma once

#include <string.h>
#include "../util/c99defs.h"
#include "../util/bmem.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Procedure call data structure
 *
 *   This is used to store parameters (and return value) sent to/from signals,
 * procedures, and callbacks.
 */

enum call_param_type {
	CALL_PARAM_TYPE_VOID,
	CALL_PARAM_TYPE_INT,
	CALL_PARAM_TYPE_FLOAT,
	CALL_PARAM_TYPE_BOOL,
	CALL_PARAM_TYPE_PTR,
	CALL_PARAM_TYPE_STRING
};

#define CALL_PARAM_IN  (1<<0)
#define CALL_PARAM_OUT (1<<1)

struct calldata {
	size_t  size;     /* size of the stack, in bytes */
	size_t  capacity; /* capacity of the stack, in bytes */
	uint8_t *stack;
};

typedef struct calldata *calldata_t;

static inline void calldata_init(struct calldata *data)
{
	memset(data, 0, sizeof(struct calldata));
}

static inline void calldata_free(struct calldata *data)
{
	bfree(data->stack);
}

EXPORT bool calldata_getdata(calldata_t data, const char *name, void *out,
		size_t size);
EXPORT void calldata_setdata(calldata_t data, const char *name, const void *in,
		size_t new_size);

static inline void calldata_clear(struct calldata *data)
{
	if (data->stack) {
		data->size = sizeof(size_t);
		*(size_t*)data->stack = 0;
	}
}

/* ------------------------------------------------------------------------- */
/* NOTE: 'get' functions return true only if paramter exists, and is the
 *       same type.  They return false otherwise. */

static inline bool calldata_getint(calldata_t data, const char *name,
		long long *val)
{
	return calldata_getdata(data, name, val, sizeof(*val));
}

static inline bool calldata_getfloat (calldata_t data, const char *name,
		double *val)
{
	return calldata_getdata(data, name, val, sizeof(*val));
}

static inline bool calldata_getbool  (calldata_t data, const char *name,
		bool *val)
{
	return calldata_getdata(data, name, val, sizeof(*val));
}

static inline bool calldata_getptr   (calldata_t data, const char *name,
		void *p_ptr)
{
	return calldata_getdata(data, name, p_ptr, sizeof(p_ptr));
}

EXPORT bool calldata_getstring(calldata_t data, const char *name,
		const char **str);

/* ------------------------------------------------------------------------- */
/* call if you know your data is valid */

static inline long long calldata_int(calldata_t data, const char *name)
{
	long long val = 0;
	calldata_getint(data, name, &val);
	return val;
}

static inline double calldata_float(calldata_t data, const char *name)
{
	double val = 0.0;
	calldata_getfloat(data, name, &val);
	return val;
}

static inline bool calldata_bool(calldata_t data, const char *name)
{
	bool val = false;
	calldata_getbool(data, name, &val);
	return val;
}

static inline void *calldata_ptr(calldata_t data, const char *name)
{
	void *val;
	calldata_getptr(data, name, &val);
	return val;
}

static inline const char *calldata_string(calldata_t data, const char *name)
{
	const char *val;
	calldata_getstring(data, name, &val);
	return val;
}

/* ------------------------------------------------------------------------- */

static inline void calldata_setint   (calldata_t data, const char *name,
		long long val)
{
	calldata_setdata(data, name, &val, sizeof(val));
}

static inline void calldata_setfloat (calldata_t data, const char *name,
		double val)
{
	calldata_setdata(data, name, &val, sizeof(val));
}

static inline void calldata_setbool  (calldata_t data, const char *name,
		bool val)
{
	calldata_setdata(data, name, &val, sizeof(val));
}

static inline void calldata_setptr   (calldata_t data, const char *name,
		void *ptr)
{
	calldata_setdata(data, name, &ptr, sizeof(ptr));
}

static inline void calldata_setstring(calldata_t data, const char *name,
		const char *str)
{
	if (str)
		calldata_setdata(data, name, str, strlen(str)+1);
	else
		calldata_setdata(data, name, NULL, 0);
}

#ifdef __cplusplus
}
#endif
