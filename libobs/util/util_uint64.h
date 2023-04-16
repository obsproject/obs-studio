/*
 * Copyright (c) 2020 Hans Petter Selasky <hps@selasky.org>
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

#include <assert.h>
#if defined(_MSC_VER)
#include <intrin.h>
#endif

static inline uint64_t util_mul64(uint64_t num, uint64_t mul, uint64_t *u1)
{
#if defined(_MSC_VER)
#if defined(_M_X64)
	return _umul128(num, mul, u1);
#elif defined(_M_ARM64)
	*u1 = __umulh(num, mul);
	return num * mul;
#else
	/* Don't call this */
	assert(false);
	*u1 = 0;
	return num * mul;
#endif
#else
	const __uint128_t u = (__uint128_t)num * (__uint128_t)mul;
	*u1 = (uint64_t)(u >> 64);
	return (uint64_t)u;
#endif
}

static uint64_t util_div64(uint64_t u1, uint64_t u0, uint64_t v, uint64_t *r)
{
	/* Adapted from __udivmodti4 in LLVM */
	// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
	// See https://llvm.org/LICENSE.txt for license information.
	// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#if !defined(_MSC_VER) && defined(__x86_64__)
	uint64_t result;
	__asm__("divq %2" : "=a"(result), "=d"(*r) : "r"(v), "a"(u0), "d"(u1));
	return result;
#elif defined(_MSC_VER) && defined(_M_X64) && (_MSC_VER >= 1920)
	return _udiv128(u1, u0, v, r);
#elif defined(_MSC_VER) && !defined(_M_X64) && !defined(_M_ARM64)
	/* Don't call this */
	assert(false);
	*r = u0 % v;
	return u0 / v;
#else
	uint64_t quotient;
	if (u1 == 0) {
		quotient = u0 / v;
		*r = u0 % v;
	} else {
		const uint64_t b = (UINT64_C(1) << 32);
#if defined(_MSC_VER)
		unsigned long index;
		const unsigned long s =
			_BitScanReverse64(&index, v) ? (63 - index) : 0;
#else
		const unsigned long s = __builtin_clzll(v);
#endif
		uint64_t un64, un10;
		if (s > 0) {
			v = v << s;
			un64 = (u1 << s) | (u0 >> (64 - s));
			un10 = u0 << s;
		} else {
			un64 = u1;
			un10 = u0;
		}
		const uint64_t vn1 = v >> 32;
		const uint64_t vn0 = v & 0xFFFFFFFF;
		const uint64_t un1 = un10 >> 32;
		const uint64_t un0 = un10 & 0xFFFFFFFF;
		uint64_t q1 = un64 / vn1;
		uint64_t rhat = un64 - q1 * vn1;
		while (q1 >= b || q1 * vn0 > b * rhat + un1) {
			q1 = q1 - 1;
			rhat = rhat + vn1;
			if (rhat >= b)
				break;
		}
		const uint64_t un21 = un64 * b + un1 - q1 * v;
		uint64_t q0 = un21 / vn1;
		rhat = un21 - q0 * vn1;
		while (q0 >= b || q0 * vn0 > b * rhat + un0) {
			q0 = q0 - 1;
			rhat = rhat + vn1;
			if (rhat >= b)
				break;
		}
		quotient = q1 * b + q0;
		*r = (un21 * b + un0 - q0 * v) >> s;
	}
	return quotient;
#endif
}

static inline uint64_t util_mul_div64(uint64_t num, uint64_t mul, uint64_t div)
{
#if defined(_MSC_VER) && !defined(_M_X64) && !defined(_M_ARM64)
	const uint64_t rem = num % div;
	return (num / div) * mul + (rem * mul) / div;
#else
	uint64_t u1, r;
	const uint64_t u0 = util_mul64(num, mul, &u1);
	return util_div64(u1, u0, div, &r);
#endif
}
