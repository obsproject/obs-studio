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

/* NOTE: 'get' functions return true only if paramter exists, and is the
 *       same size.  They return false otherwise. */

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

static inline bool calldata_getchar  (calldata_t data, const char *name,
		char *val)
{
	return calldata_getdata(data, name, val, sizeof(*val));
}

static inline bool calldata_getuchar (calldata_t data, const char *name,
		unsigned char *val)
{
	return calldata_getdata(data, name, val, sizeof(*val));
}

static inline bool calldata_getshort (calldata_t data, const char *name,
		short *val)
{
	return calldata_getdata(data, name, val, sizeof(*val));
}

static inline bool calldata_getushort(calldata_t data, const char *name,
		unsigned short *val)
{
	return calldata_getdata(data, name, val, sizeof(*val));
}

static inline bool calldata_getint   (calldata_t data, const char *name,
		int *val)
{
	return calldata_getdata(data, name, val, sizeof(*val));
}

static inline bool calldata_getuint  (calldata_t data, const char *name,
		unsigned int *val)
{
	return calldata_getdata(data, name, val, sizeof(*val));
}

static inline bool calldata_getlong  (calldata_t data, const char *name,
		long *val)
{
	return calldata_getdata(data, name, val, sizeof(*val));
}

static inline bool calldata_getulong (calldata_t data, const char *name,
		unsigned long *val)
{
	return calldata_getdata(data, name, val, sizeof(*val));
}

static inline bool calldata_getint8  (calldata_t data, const char *name,
		int8_t *val)
{
	return calldata_getdata(data, name, val, sizeof(*val));
}

static inline bool calldata_getuint8 (calldata_t data, const char *name,
		uint8_t *val)
{
	return calldata_getdata(data, name, val, sizeof(*val));
}

static inline bool calldata_getint16 (calldata_t data, const char *name,
		int8_t *val)
{
	return calldata_getdata(data, name, val, sizeof(*val));
}

static inline bool calldata_getuint16(calldata_t data, const char *name,
		uint8_t *val)
{
	return calldata_getdata(data, name, val, sizeof(*val));
}

static inline bool calldata_getint32 (calldata_t data, const char *name,
		int32_t *val)
{
	return calldata_getdata(data, name, val, sizeof(*val));
}

static inline bool calldata_getuint32(calldata_t data, const char *name,
		uint32_t *val)
{
	return calldata_getdata(data, name, val, sizeof(*val));
}

static inline bool calldata_getint64 (calldata_t data, const char *name,
		int64_t *val)
{
	return calldata_getdata(data, name, val, sizeof(*val));
}

static inline bool calldata_getuint64(calldata_t data, const char *name,
		uint64_t *val)
{
	return calldata_getdata(data, name, val, sizeof(*val));
}

static inline bool calldata_getfloat (calldata_t data, const char *name,
		float *val)
{
	return calldata_getdata(data, name, val, sizeof(*val));
}

static inline bool calldata_getdouble(calldata_t data, const char *name,
		double *val)
{
	return calldata_getdata(data, name, val, sizeof(*val));
}

static inline bool calldata_getbool  (calldata_t data, const char *name,
		bool *val)
{
	return calldata_getdata(data, name, val, sizeof(*val));
}

static inline bool calldata_getsize  (calldata_t data, const char *name,
		size_t *val)
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

static inline char calldata_char(calldata_t data, const char *name)
{
	char val;
	calldata_getchar(data, name, &val);
	return val;
}

static inline unsigned char calldata_uchar(calldata_t data, const char *name)
{
	unsigned char val;
	calldata_getuchar(data, name, &val);
	return val;
}

static inline short calldata_short(calldata_t data, const char *name)
{
	short val;
	calldata_getshort(data, name, &val);
	return val;
}

static inline unsigned short calldata_ushort(calldata_t data, const char *name)
{
	unsigned short val;
	calldata_getushort(data, name, &val);
	return val;
}

static inline int calldata_int(calldata_t data, const char *name)
{
	int val;
	calldata_getint(data, name, &val);
	return val;
}

static inline unsigned int calldata_uint(calldata_t data, const char *name)
{
	unsigned int val;
	calldata_getuint(data, name, &val);
	return val;
}

static inline long calldata_long(calldata_t data, const char *name)
{
	long val;
	calldata_getlong(data, name, &val);
	return val;
}

static inline unsigned long calldata_ulong(calldata_t data, const char *name)
{
	unsigned long val;
	calldata_getulong(data, name, &val);
	return val;
}

static inline int8_t calldata_int8(calldata_t data, const char *name)
{
	int8_t val;
	calldata_getint8(data, name, &val);
	return val;
}

static inline uint8_t calldata_uint8(calldata_t data, const char *name)
{
	uint8_t val;
	calldata_getuint8(data, name, &val);
	return val;
}

static inline int8_t calldata_int16(calldata_t data, const char *name)
{
	int8_t val;
	calldata_getint16(data, name, &val);
	return val;
}

static inline uint8_t calldata_uint16(calldata_t data, const char *name)
{
	uint8_t val;
	calldata_getuint16(data, name, &val);
	return val;
}

static inline int32_t calldata_int32(calldata_t data, const char *name)
{
	int32_t val;
	calldata_getint32(data, name, &val);
	return val;
}

static inline uint32_t calldata_uint32(calldata_t data, const char *name)
{
	uint32_t val;
	calldata_getuint32(data, name, &val);
	return val;
}

static inline int64_t calldata_int64(calldata_t data, const char *name)
{
	int64_t val;
	calldata_getint64(data, name, &val);
	return val;
}

static inline uint64_t calldata_uint64(calldata_t data, const char *name)
{
	uint64_t val;
	calldata_getuint64(data, name, &val);
	return val;
}

static inline float calldata_float(calldata_t data, const char *name)
{
	float val;
	calldata_getfloat(data, name, &val);
	return val;
}

static inline double calldata_double(calldata_t data, const char *name)
{
	double val;
	calldata_getdouble(data, name, &val);
	return val;
}

static inline bool calldata_bool(calldata_t data, const char *name)
{
	bool val;
	calldata_getbool(data, name, &val);
	return val;
}

static inline size_t calldata_size(calldata_t data, const char *name)
{
	size_t val;
	calldata_getsize(data, name, &val);
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

static void calldata_setchar  (calldata_t data, const char *name, char val)
{
	calldata_setdata(data, name, &val, sizeof(val));
}

static inline void calldata_setuchar (calldata_t data, const char *name,
		unsigned char val)
{
	calldata_setdata(data, name, &val, sizeof(val));
}

static inline void calldata_setshort (calldata_t data, const char *name,
		short val)
{
	calldata_setdata(data, name, &val, sizeof(val));
}

static inline void calldata_setushort(calldata_t data, const char *name,
		unsigned short val)
{
	calldata_setdata(data, name, &val, sizeof(val));
}

static inline void calldata_setint   (calldata_t data, const char *name,
		int val)
{
	calldata_setdata(data, name, &val, sizeof(val));
}

static inline void calldata_setuint  (calldata_t data, const char *name,
		unsigned int val)
{
	calldata_setdata(data, name, &val, sizeof(val));
}

static inline void calldata_setlong  (calldata_t data, const char *name,
		long val)
{
	calldata_setdata(data, name, &val, sizeof(val));
}

static inline void calldata_setulong (calldata_t data, const char *name,
		unsigned long val)
{
	calldata_setdata(data, name, &val, sizeof(val));
}

static inline void calldata_setint8  (calldata_t data, const char *name,
		int8_t val)
{
	calldata_setdata(data, name, &val, sizeof(val));
}

static inline void calldata_setuint8 (calldata_t data, const char *name,
		uint8_t val)
{
	calldata_setdata(data, name, &val, sizeof(val));
}

static inline void calldata_setint16 (calldata_t data, const char *name,
		int8_t val)
{
	calldata_setdata(data, name, &val, sizeof(val));
}

static inline void calldata_setuint16(calldata_t data, const char *name,
		uint8_t val)
{
	calldata_setdata(data, name, &val, sizeof(val));
}

static inline void calldata_setint32 (calldata_t data, const char *name,
		int32_t val)
{
	calldata_setdata(data, name, &val, sizeof(val));
}

static inline void calldata_setuint32(calldata_t data, const char *name,
		uint32_t val)
{
	calldata_setdata(data, name, &val, sizeof(val));
}

static inline void calldata_setint64 (calldata_t data, const char *name,
		int64_t val)
{
	calldata_setdata(data, name, &val, sizeof(val));
}

static inline void calldata_setuint64(calldata_t data, const char *name,
		uint64_t val)
{
	calldata_setdata(data, name, &val, sizeof(val));
}

static inline void calldata_setfloat (calldata_t data, const char *name,
		float val)
{
	calldata_setdata(data, name, &val, sizeof(val));
}

static inline void calldata_setdouble(calldata_t data, const char *name,
		double val)
{
	calldata_setdata(data, name, &val, sizeof(val));
}

static inline void calldata_setbool  (calldata_t data, const char *name,
		bool val)
{
	calldata_setdata(data, name, &val, sizeof(val));
}

static inline void calldata_setsize  (calldata_t data, const char *name,
		size_t val)
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
