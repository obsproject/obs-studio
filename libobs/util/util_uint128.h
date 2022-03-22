/*
 * Copyright (c) 2015 Hugh Bailey <obs.jim@gmail.com>
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

struct util_uint128 {
	union {
		uint32_t i32[4];
		struct {
			uint64_t low;
			uint64_t high;
		};
	};
};

typedef struct util_uint128 util_uint128_t;

static inline util_uint128_t util_add128(util_uint128_t a, util_uint128_t b)
{
	util_uint128_t out;
	uint64_t val;

	val = (a.low & 0xFFFFFFFFULL) + (b.low & 0xFFFFFFFFULL);
	out.i32[0] = (uint32_t)(val & 0xFFFFFFFFULL);
	val >>= 32;

	val += (a.low >> 32) + (b.low >> 32);
	out.i32[1] = (uint32_t)val;
	val >>= 32;

	val += (a.high & 0xFFFFFFFFULL) + (b.high & 0xFFFFFFFFULL);
	out.i32[2] = (uint32_t)(val & 0xFFFFFFFFULL);
	val >>= 32;

	val += (a.high >> 32) + (b.high >> 32);
	out.i32[3] = (uint32_t)val;

	return out;
}

static inline util_uint128_t util_lshift64_internal_32(uint64_t a)
{
	util_uint128_t val;
	val.low = a << 32;
	val.high = a >> 32;
	return val;
}

static inline util_uint128_t util_lshift64_internal_64(uint64_t a)
{
	util_uint128_t val;
	val.low = 0;
	val.high = a;
	return val;
}

static inline util_uint128_t util_mul64_64(uint64_t a, uint64_t b)
{
	util_uint128_t out;
	uint64_t m;

	m = (a & 0xFFFFFFFFULL) * (b & 0xFFFFFFFFULL);
	out.low = m;
	out.high = 0;

	m = (a >> 32) * (b & 0xFFFFFFFFULL);
	out = util_add128(out, util_lshift64_internal_32(m));

	m = (a & 0xFFFFFFFFULL) * (b >> 32);
	out = util_add128(out, util_lshift64_internal_32(m));

	m = (a >> 32) * (b >> 32);
	out = util_add128(out, util_lshift64_internal_64(m));

	return out;
}

static inline util_uint128_t util_div128_32(util_uint128_t a, uint32_t b)
{
	util_uint128_t out;
	uint64_t val = 0;

	for (int i = 3; i >= 0; i--) {
		val = (val << 32) | a.i32[i];
		if (val < b) {
			out.i32[i] = 0;
			continue;
		}

		out.i32[i] = (uint32_t)(val / b);
		val = val % b;
	}

	return out;
}
