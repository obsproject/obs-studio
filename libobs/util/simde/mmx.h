/* Copyright (c) 2017-2018 Evan Nemerson <evan@nemerson.com>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#if !defined(SIMDE__MMX_H)
#if !defined(SIMDE__MMX_H)
#define SIMDE__MMX_H
#endif
#include "simde-common.h"

#if defined(SIMDE_MMX_FORCE_NATIVE)
#define SIMDE_MMX_NATIVE
#elif defined(__MMX__) && !defined(SIMDE_MMX_NO_NATIVE) && \
	!defined(SIMDE_NO_NATIVE)
#define SIMDE_MMX_NATIVE
#elif defined(__ARM_NEON) && !defined(SIMDE_MMX_NO_NEON) && \
	!defined(SIMDE_NO_NEON)
#define SIMDE_MMX_NEON
#endif

#if defined(SIMDE_MMX_NATIVE)
#include <mmintrin.h>
#else
#if defined(SIMDE_MMX_NEON)
#include <arm_neon.h>
#endif
#endif
#include <stdint.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

SIMDE__BEGIN_DECLS

typedef union {
#if defined(SIMDE__ENABLE_GCC_VEC_EXT)
	int8_t i8 __attribute__((__vector_size__(8), __may_alias__));
	int16_t i16 __attribute__((__vector_size__(8), __may_alias__));
	int32_t i32 __attribute__((__vector_size__(8), __may_alias__));
	int64_t i64 __attribute__((__vector_size__(8), __may_alias__));
	uint8_t u8 __attribute__((__vector_size__(8), __may_alias__));
	uint16_t u16 __attribute__((__vector_size__(8), __may_alias__));
	uint32_t u32 __attribute__((__vector_size__(8), __may_alias__));
	uint64_t u64 __attribute__((__vector_size__(8), __may_alias__));
	simde_float32 f32 __attribute__((__vector_size__(8), __may_alias__));
#else
	int8_t i8[8];
	int16_t i16[4];
	int32_t i32[2];
	int64_t i64[1];
	uint8_t u8[8];
	uint16_t u16[4];
	uint32_t u32[2];
	uint64_t u64[1];
	simde_float32 f32[2];
#endif

#if defined(SIMDE_MMX_NATIVE)
	__m64 n;
#elif defined(SIMDE_MMX_NEON)
	int8x8_t neon_i8;
	int16x4_t neon_i16;
	int32x2_t neon_i32;
	int64x1_t neon_i64;
	uint8x8_t neon_u8;
	uint16x4_t neon_u16;
	uint32x2_t neon_u32;
	uint64x1_t neon_u64;
	float32x2_t neon_f32;
#endif
} simde__m64;

#if defined(SIMDE_MMX_NATIVE)
HEDLEY_STATIC_ASSERT(sizeof(__m64) == sizeof(simde__m64),
		     "__m64 size doesn't match simde__m64 size");
SIMDE__FUNCTION_ATTRIBUTES simde__m64 SIMDE__M64_C(__m64 v)
{
	simde__m64 r;
	r.n = v;
	return r;
}
#elif defined(SIMDE_MMX_NEON)
#define SIMDE__M64_NEON_C(T, expr) \
	(simde__m64) { .neon_##T = (expr) }
#endif
HEDLEY_STATIC_ASSERT(8 == sizeof(simde__m64), "__m64 size incorrect");

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_add_pi8(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_add_pi8(a.n, b.n));
#else
	simde__m64 r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < 8; i++) {
		r.i8[i] = a.i8[i] + b.i8[i];
	}
	return r;
#endif
}
#define simde_m_paddb(a, b) simde_mm_add_pi8(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_add_pi16(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_add_pi16(a.n, b.n));
#else
	simde__m64 r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (8 / sizeof(int16_t)); i++) {
		r.i16[i] = a.i16[i] + b.i16[i];
	}
	return r;
#endif
}
#define simde_m_paddw(a, b) simde_mm_add_pi16(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_add_pi32(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_add_pi32(a.n, b.n));
#else
	simde__m64 r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (8 / sizeof(int32_t)); i++) {
		r.i32[i] = a.i32[i] + b.i32[i];
	}
	return r;
#endif
}
#define simde_m_paddd(a, b) simde_mm_add_pi32(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_adds_pi8(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_adds_pi8(a.n, b.n));
#else
	simde__m64 r;
	SIMDE__VECTORIZE
	for (int i = 0; i < 8; i++) {
		if ((((b.i8[i]) > 0) && ((a.i8[i]) > (INT8_MAX - (b.i8[i]))))) {
			r.i8[i] = INT8_MAX;
		} else if ((((b.i8[i]) < 0) &&
			    ((a.i8[i]) < (INT8_MIN - (b.i8[i]))))) {
			r.i8[i] = INT8_MIN;
		} else {
			r.i8[i] = (a.i8[i]) + (b.i8[i]);
		}
	}
	return r;
#endif
}
#define simde_m_paddsb(a, b) simde_mm_adds_pi8(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_adds_pu8(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_adds_pu8(a.n, b.n));
#else
	simde__m64 r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < 8; i++) {
		const int32_t x = a.u8[i] + b.u8[i];
		if (x < 0)
			r.u8[i] = 0;
		else if (x > UINT8_MAX)
			r.u8[i] = UINT8_MAX;
		else
			r.u8[i] = (uint8_t)x;
	}
	return r;
#endif
}
#define simde_m_paddusb(a, b) simde_mm_adds_pu8(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_adds_pi16(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_adds_pi16(a.n, b.n));
#else
	simde__m64 r;
	SIMDE__VECTORIZE
	for (int i = 0; i < 4; i++) {
		if ((((b.i16[i]) > 0) &&
		     ((a.i16[i]) > (INT16_MAX - (b.i16[i]))))) {
			r.i16[i] = INT16_MAX;
		} else if ((((b.i16[i]) < 0) &&
			    ((a.i16[i]) < (SHRT_MIN - (b.i16[i]))))) {
			r.i16[i] = SHRT_MIN;
		} else {
			r.i16[i] = (a.i16[i]) + (b.i16[i]);
		}
	}
	return r;
#endif
}
#define simde_m_paddsw(a, b) simde_mm_adds_pi16(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_adds_pu16(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_adds_pu16(a.n, b.n));
#else
	simde__m64 r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (8 / sizeof(int16_t)); i++) {
		const uint32_t x = a.u16[i] + b.u16[i];
		if (x > UINT16_MAX)
			r.u16[i] = UINT16_MAX;
		else
			r.u16[i] = (uint16_t)x;
	}
	return r;
#endif
}
#define simde_m_paddusw(a, b) simde_mm_adds_pu16(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_and_si64(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_and_si64(a.n, b.n));
#else
	simde__m64 r;
	r.i64[0] = a.i64[0] & b.i64[0];
	return r;
#endif
}
#define simde_m_pand(a, b) simde_mm_and_si64(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_andnot_si64(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_andnot_si64(a.n, b.n));
#else
	simde__m64 r;
	r.i64[0] = ~(a.i64[0]) & b.i64[0];
	return r;
#endif
}
#define simde_m_pandn(a, b) simde_mm_andnot_si64(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_cmpeq_pi8(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_cmpeq_pi8(a.n, b.n));
#else
	simde__m64 r;
	SIMDE__VECTORIZE
	for (int i = 0; i < 8; i++) {
		r.i8[i] = (a.i8[i] == b.i8[i]) * 0xff;
	}
	return r;
#endif
}
#define simde_m_pcmpeqb(a, b) simde_mm_cmpeq_pi8(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_cmpeq_pi16(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_cmpeq_pi16(a.n, b.n));
#else
	simde__m64 r;
	SIMDE__VECTORIZE
	for (int i = 0; i < 4; i++) {
		r.i16[i] = (a.i16[i] == b.i16[i]) * 0xffff;
	}
	return r;
#endif
}
#define simde_m_pcmpeqw(a, b) simde_mm_cmpeq_pi16(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_cmpeq_pi32(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_cmpeq_pi32(a.n, b.n));
#else
	simde__m64 r;
	SIMDE__VECTORIZE
	for (int i = 0; i < 2; i++) {
		r.i32[i] = (a.i32[i] == b.i32[i]) * 0xffffffff;
	}
	return r;
#endif
}
#define simde_m_pcmpeqd(a, b) simde_mm_cmpeq_pi32(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_cmpgt_pi8(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_cmpgt_pi8(a.n, b.n));
#else
	simde__m64 r;
	SIMDE__VECTORIZE
	for (int i = 0; i < 8; i++) {
		r.i8[i] = (a.i8[i] > b.i8[i]) * 0xff;
	}
	return r;
#endif
}
#define simde_m_pcmpgtb(a, b) simde_mm_cmpgt_pi8(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_cmpgt_pi16(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_cmpgt_pi16(a.n, b.n));
#else
	simde__m64 r;
	SIMDE__VECTORIZE
	for (int i = 0; i < 4; i++) {
		r.i16[i] = (a.i16[i] > b.i16[i]) * 0xffff;
	}
	return r;
#endif
}
#define simde_m_pcmpgtw(a, b) simde_mm_cmpgt_pi16(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_cmpgt_pi32(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_cmpgt_pi32(a.n, b.n));
#else
	simde__m64 r;
	SIMDE__VECTORIZE
	for (int i = 0; i < 2; i++) {
		r.i32[i] = (a.i32[i] > b.i32[i]) * 0xffffffff;
	}
	return r;
#endif
}
#define simde_m_pcmpgtd(a, b) simde_mm_cmpgt_pi32(a, b)

SIMDE__FUNCTION_ATTRIBUTES
int64_t simde_mm_cvtm64_si64(simde__m64 a)
{
#if defined(SIMDE_MMX_NATIVE) && defined(SIMDE_ARCH_AMD64) && !defined(__PGI)
	return _mm_cvtm64_si64(a.n);
#else
	return a.i64[0];
#endif
}
#define simde_m_to_int64(a) simde_mm_cvtm64_si64(a)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_cvtsi32_si64(int32_t a)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_cvtsi32_si64(a));
#else
	simde__m64 r;
	r.i32[0] = a;
	r.i32[1] = 0;
	return r;
#endif
}
#define simde_m_from_int(a) simde_mm_cvtsi32_si64(a)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_cvtsi64_m64(int64_t a)
{
#if defined(SIMDE_MMX_NATIVE) && defined(SIMDE_ARCH_AMD64) && !defined(__PGI)
	return SIMDE__M64_C(_mm_cvtsi64_m64(a));
#else
	simde__m64 r;
	r.i64[0] = a;
	return r;
#endif
}
#define simde_m_from_int64(a) simde_mm_cvtsi64_m64(a)

SIMDE__FUNCTION_ATTRIBUTES
int32_t simde_mm_cvtsi64_si32(simde__m64 a)
{
#if defined(SIMDE_MMX_NATIVE)
	return _mm_cvtsi64_si32(a.n);
#else
	return a.i32[0];
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
void simde_mm_empty(void)
{
#if defined(SIMDE_MMX_NATIVE)
	_mm_empty();
#else
#endif
}
#define simde_m_empty() simde_mm_empty()

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_madd_pi16(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_madd_pi16(a.n, b.n));
#else
	simde__m64 r;
	SIMDE__VECTORIZE
	for (int i = 0; i < 4; i += 2) {
		r.i32[i / 2] =
			(a.i16[i] * b.i16[i]) + (a.i16[i + 1] * b.i16[i + 1]);
	}
	return r;
#endif
}
#define simde_m_pmaddwd(a, b) simde_mm_madd_pi16(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_mulhi_pi16(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_mulhi_pi16(a.n, b.n));
#else
	simde__m64 r;
	SIMDE__VECTORIZE
	for (int i = 0; i < 4; i++) {
		r.i16[i] = (int16_t)((a.i16[i] * b.i16[i]) >> 16);
	}
	return r;
#endif
}
#define simde_m_pmulhw(a, b) simde_mm_mulhi_pi16(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_mullo_pi16(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_mullo_pi16(a.n, b.n));
#else
	simde__m64 r;
	SIMDE__VECTORIZE
	for (int i = 0; i < 4; i++) {
		r.i16[i] = (int16_t)((a.i16[i] * b.i16[i]) & 0xffff);
	}
	return r;
#endif
}
#define simde_m_pmullw(a, b) simde_mm_mullo_pi16(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_or_si64(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_or_si64(a.n, b.n));
#else
	simde__m64 r;
	r.i64[0] = a.i64[0] | b.i64[0];
	return r;
#endif
}
#define simde_m_por(a, b) simde_mm_or_si64(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_packs_pi16(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_packs_pi16(a.n, b.n));
#else
	simde__m64 r;

	SIMDE__VECTORIZE
	for (size_t i = 0; i < (8 / sizeof(int16_t)); i++) {
		if (a.i16[i] < INT8_MIN) {
			r.i8[i] = INT8_MIN;
		} else if (a.i16[i] > INT8_MAX) {
			r.i8[i] = INT8_MAX;
		} else {
			r.i8[i] = (int8_t)a.i16[i];
		}
	}

	SIMDE__VECTORIZE
	for (size_t i = 0; i < (8 / sizeof(int16_t)); i++) {
		if (b.i16[i] < INT8_MIN) {
			r.i8[i + 4] = INT8_MIN;
		} else if (b.i16[i] > INT8_MAX) {
			r.i8[i + 4] = INT8_MAX;
		} else {
			r.i8[i + 4] = (int8_t)b.i16[i];
		}
	}

	return r;
#endif
}
#define simde_m_packsswb(a, b) simde_mm_packs_pi16(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_packs_pi32(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_packs_pi32(a.n, b.n));
#else
	simde__m64 r;

	SIMDE__VECTORIZE
	for (size_t i = 0; i < (8 / sizeof(a.i32[0])); i++) {
		if (a.i32[i] < SHRT_MIN) {
			r.i16[i] = SHRT_MIN;
		} else if (a.i32[i] > INT16_MAX) {
			r.i16[i] = INT16_MAX;
		} else {
			r.i16[i] = (int16_t)a.i32[i];
		}
	}

	SIMDE__VECTORIZE
	for (size_t i = 0; i < (8 / sizeof(b.i32[0])); i++) {
		if (b.i32[i] < SHRT_MIN) {
			r.i16[i + 2] = SHRT_MIN;
		} else if (b.i32[i] > INT16_MAX) {
			r.i16[i + 2] = INT16_MAX;
		} else {
			r.i16[i + 2] = (int16_t)b.i32[i];
		}
	}

	return r;
#endif
}
#define simde_m_packssdw(a, b) simde_mm_packs_pi32(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_packs_pu16(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_packs_pu16(a.n, b.n));
#else
	simde__m64 r;

	SIMDE__VECTORIZE
	for (size_t i = 0; i < (8 / sizeof(int16_t)); i++) {
		if (a.i16[i] > UINT8_MAX) {
			r.u8[i] = UINT8_MAX;
		} else if (a.i16[i] < 0) {
			r.u8[i] = 0;
		} else {
			r.u8[i] = (int8_t)a.i16[i];
		}
	}

	SIMDE__VECTORIZE
	for (size_t i = 0; i < (8 / sizeof(int16_t)); i++) {
		if (b.i16[i] > UINT8_MAX) {
			r.u8[i + 4] = UINT8_MAX;
		} else if (b.i16[i] < 0) {
			r.u8[i + 4] = 0;
		} else {
			r.u8[i + 4] = (int8_t)b.i16[i];
		}
	}

	return r;
#endif
}
#define simde_m_packuswb(a, b) simde_mm_packs_pu16(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_set_pi8(int8_t e7, int8_t e6, int8_t e5, int8_t e4,
			    int8_t e3, int8_t e2, int8_t e1, int8_t e0)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_set_pi8(e7, e6, e5, e4, e3, e2, e1, e0));
#else
	simde__m64 r;
	r.i8[0] = e0;
	r.i8[1] = e1;
	r.i8[2] = e2;
	r.i8[3] = e3;
	r.i8[4] = e4;
	r.i8[5] = e5;
	r.i8[6] = e6;
	r.i8[7] = e7;
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_x_mm_set_pu8(uint8_t e7, uint8_t e6, uint8_t e5, uint8_t e4,
			      uint8_t e3, uint8_t e2, uint8_t e1, uint8_t e0)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_set_pi8((int8_t)e7, (int8_t)e6, (int8_t)e5,
					(int8_t)e4, (int8_t)e3, (int8_t)e2,
					(int8_t)e1, (int8_t)e0));
#else
	simde__m64 r;
	r.u8[0] = e0;
	r.u8[1] = e1;
	r.u8[2] = e2;
	r.u8[3] = e3;
	r.u8[4] = e4;
	r.u8[5] = e5;
	r.u8[6] = e6;
	r.u8[7] = e7;
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_set_pi16(int16_t e3, int16_t e2, int16_t e1, int16_t e0)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_set_pi16(e3, e2, e1, e0));
#else
	simde__m64 r;
	r.i16[0] = e0;
	r.i16[1] = e1;
	r.i16[2] = e2;
	r.i16[3] = e3;
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_x_mm_set_pu16(uint16_t e3, uint16_t e2, uint16_t e1,
			       uint16_t e0)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_set_pi16((int16_t)e3, (int16_t)e2, (int16_t)e1,
					 (int16_t)e0));
#else
	simde__m64 r;
	r.u16[0] = e0;
	r.u16[1] = e1;
	r.u16[2] = e2;
	r.u16[3] = e3;
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_x_mm_set_pu32(uint32_t e1, uint32_t e0)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_set_pi32((int32_t)e1, (int32_t)e0));
#else
	simde__m64 r;
	r.u32[0] = e0;
	r.u32[1] = e1;
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_set_pi32(int32_t e1, int32_t e0)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_set_pi32(e1, e0));
#else
	simde__m64 r;
	r.i32[0] = e0;
	r.i32[1] = e1;
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_set1_pi8(int8_t a)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_set1_pi8(a));
#else
	return simde_mm_set_pi8(a, a, a, a, a, a, a, a);
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_set1_pi16(int16_t a)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_set1_pi16(a));
#else
	return simde_mm_set_pi16(a, a, a, a);
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_set1_pi32(int32_t a)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_set1_pi32(a));
#else
	return simde_mm_set_pi32(a, a);
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_setr_pi8(int8_t e7, int8_t e6, int8_t e5, int8_t e4,
			     int8_t e3, int8_t e2, int8_t e1, int8_t e0)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_setr_pi8(e7, e6, e5, e4, e3, e2, e1, e0));
#else
	return simde_mm_set_pi8(e0, e1, e2, e3, e4, e5, e6, e7);
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_setr_pi16(int16_t e3, int16_t e2, int16_t e1, int16_t e0)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_setr_pi16(e3, e2, e1, e0));
#else
	return simde_mm_set_pi16(e0, e1, e2, e3);
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_setr_pi32(int32_t e1, int32_t e0)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_setr_pi32(e1, e0));
#else
	return simde_mm_set_pi32(e0, e1);
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_setzero_si64(void)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_setzero_si64());
#else
	return simde_mm_set_pi32(0, 0);
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_sll_pi16(simde__m64 a, simde__m64 count)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_sll_pi16(a.n, count.n));
#else
	simde__m64 r;

	if (HEDLEY_UNLIKELY(count.u64[0] > 15)) {
		memset(&r, 0, sizeof(r));
		return r;
	}

	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.u16) / sizeof(r.u16[0])); i++) {
		r.u16[i] = a.u16[i] << count.u64[0];
	}
	return r;
#endif
}
#define simde_m_psllw(a, count) simde_mm_sll_pi16(a, count)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_sll_pi32(simde__m64 a, simde__m64 count)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_sll_pi32(a.n, count.n));
#else
	simde__m64 r;

	if (HEDLEY_UNLIKELY(count.u64[0] > 31)) {
		memset(&r, 0, sizeof(r));
		return r;
	}

	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.u32) / sizeof(r.u32[0])); i++) {
		r.u32[i] = a.u32[i] << count.u64[0];
	}
	return r;
#endif
}
#define simde_m_pslld(a, count) simde_mm_sll_pi32(a, count)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_slli_pi16(simde__m64 a, int count)
{
#if defined(SIMDE_MMX_NATIVE) && !defined(__PGI)
	return SIMDE__M64_C(_mm_slli_pi16(a.n, count));
#else
	simde__m64 r;

	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.u16) / sizeof(r.u16[0])); i++) {
		r.u16[i] = a.u16[i] << count;
	}

	return r;
#endif
}
#define simde_m_psllwi(a, count) simde_mm_slli_pi16(a, count)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_slli_pi32(simde__m64 a, int count)
{
#if defined(SIMDE_MMX_NATIVE) && !defined(__PGI)
	return SIMDE__M64_C(_mm_slli_pi32(a.n, count));
#else
	simde__m64 r;

	SIMDE__VECTORIZE
	for (size_t i = 0; i < (8 / sizeof(int)); i++) {
		r.u32[i] = a.u32[i] << count;
	}

	return r;
#endif
}
#define simde_m_pslldi(a, b) simde_mm_slli_pi32(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_slli_si64(simde__m64 a, int count)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_slli_si64(a.n, count));
#else
	simde__m64 r;
	r.u64[0] = a.u64[0] << count;
	return r;
#endif
}
#define simde_m_psllqi(a, count) simde_mm_slli_si64(a, count)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_sll_si64(simde__m64 a, simde__m64 count)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_sll_si64(a.n, count.n));
#else
	simde__m64 r;

	if (HEDLEY_UNLIKELY(count.u64[0] > 63)) {
		memset(&r, 0, sizeof(r));
		return r;
	}

	r.u64[0] = a.u64[0] << count.u64[0];

	return r;
#endif
}
#define simde_m_psllq(a, count) simde_mm_sll_si64(a, count)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_srl_pi16(simde__m64 a, simde__m64 count)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_srl_pi16(a.n, count.n));
#else
	simde__m64 r;

	if (HEDLEY_UNLIKELY(count.u64[0] > 15)) {
		memset(&r, 0, sizeof(r));
		return r;
	}

	SIMDE__VECTORIZE
	for (size_t i = 0; i < sizeof(r.u16) / sizeof(r.u16[0]); i++) {
		r.u16[i] = a.u16[i] >> count.u64[0];
	}
	return r;
#endif
}
#define simde_m_psrlw(a, count) simde_mm_srl_pi16(a, count)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_srl_pi32(simde__m64 a, simde__m64 count)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_srl_pi32(a.n, count.n));
#else
	simde__m64 r;

	if (HEDLEY_UNLIKELY(count.u64[0] > 31)) {
		memset(&r, 0, sizeof(r));
		return r;
	}

	SIMDE__VECTORIZE
	for (size_t i = 0; i < sizeof(r.u32) / sizeof(r.u32[0]); i++) {
		r.u32[i] = a.u32[i] >> count.u64[0];
	}
	return r;
#endif
}
#define simde_m_psrld(a, count) simde_mm_srl_pi32(a, count)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_srli_pi16(simde__m64 a, int count)
{
#if defined(SIMDE_MMX_NATIVE) && !defined(__PGI)
	return SIMDE__M64_C(_mm_srli_pi16(a.n, count));
#else
	simde__m64 r;

	SIMDE__VECTORIZE
	for (size_t i = 0; i < (8 / sizeof(uint16_t)); i++) {
		r.u16[i] = a.u16[i] >> count;
	}

	return r;
#endif
}
#define simde_m_psrlwi(a, count) simde_mm_srli_pi16(a, count)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_srli_pi32(simde__m64 a, int count)
{
#if defined(SIMDE_MMX_NATIVE) && !defined(__PGI)
	return SIMDE__M64_C(_mm_srli_pi32(a.n, count));
#else
	simde__m64 r;

	SIMDE__VECTORIZE
	for (size_t i = 0; i < (8 / sizeof(int)); i++) {
		r.u32[i] = a.u32[i] >> count;
	}

	return r;
#endif
}
#define simde_m_psrldi(a, count) simde_mm_srli_pi32(a, count)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_srli_si64(simde__m64 a, int count)
{
#if defined(SIMDE_MMX_NATIVE) && !defined(__PGI)
	return SIMDE__M64_C(_mm_srli_si64(a.n, count));
#else
	simde__m64 r;
	r.u64[0] = a.u64[0] >> count;
	return r;
#endif
}
#define simde_m_psrlqi(a, count) simde_mm_srli_si64(a, count)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_srl_si64(simde__m64 a, simde__m64 count)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_srl_si64(a.n, count.n));
#else
	simde__m64 r;

	if (HEDLEY_UNLIKELY(count.u64[0] > 63)) {
		memset(&r, 0, sizeof(r));
		return r;
	}

	r.u64[0] = a.u64[0] >> count.u64[0];
	return r;
#endif
}
#define simde_m_psrlq(a, count) simde_mm_srl_si64(a, count)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_srai_pi16(simde__m64 a, int count)
{
#if defined(SIMDE_MMX_NATIVE) && !defined(__PGI)
	return SIMDE__M64_C(_mm_srai_pi16(a.n, count));
#else
	simde__m64 r;

	const uint16_t m =
		(uint16_t)((~0U) << ((sizeof(int16_t) * CHAR_BIT) - count));

	SIMDE__VECTORIZE
	for (size_t i = 0; i < (8 / sizeof(int16_t)); i++) {
		const uint16_t is_neg = ((uint16_t)(
			((a.u16[i]) >> ((sizeof(int16_t) * CHAR_BIT) - 1))));
		r.u16[i] = (a.u16[i] >> count) | (m * is_neg);
	}

	return r;
#endif
}
#define simde_m_psrawi(a, count) simde_mm_srai_pi16(a, count)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_srai_pi32(simde__m64 a, int count)
{
#if defined(SIMDE_MMX_NATIVE) && !defined(__PGI)
	return SIMDE__M64_C(_mm_srai_pi32(a.n, count));
#else
	simde__m64 r;

	const uint32_t m =
		(uint32_t)((~0U) << ((sizeof(int) * CHAR_BIT) - count));
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (8 / sizeof(int)); i++) {
		const uint32_t is_neg = ((uint32_t)(
			((a.u32[i]) >> ((sizeof(int) * CHAR_BIT) - 1))));
		r.u32[i] = (a.u32[i] >> count) | (m * is_neg);
	}

	return r;
#endif
}
#define simde_m_srai_pi32(a, count) simde_mm_srai_pi32(a, count)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_sra_pi16(simde__m64 a, simde__m64 count)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_sra_pi16(a.n, count.n));
#else
	simde__m64 r;
	int cnt = (int)count.i64[0];

	if (cnt > 15 || cnt < 0) {
		for (size_t i = 0; i < (sizeof(r.i16) / sizeof(r.i16[0]));
		     i++) {
			r.u16[i] = (a.i16[i] < 0) ? 0xffff : 0x0000;
		}
	} else {
		const uint16_t m = (uint16_t)(
			(~0U) << ((sizeof(int16_t) * CHAR_BIT) - cnt));
		for (size_t i = 0; i < (sizeof(r.i16) / sizeof(r.i16[0]));
		     i++) {
			const uint16_t is_neg = a.i16[i] < 0;
			r.u16[i] = (a.u16[i] >> cnt) | (m * is_neg);
		}
	}

	return r;
#endif
}
#define simde_m_psraw(a, count) simde_mm_sra_pi16(a, count)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_sra_pi32(simde__m64 a, simde__m64 count)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_sra_pi32(a.n, count.n));
#else
	simde__m64 r;
	const uint64_t cnt = count.u64[0];

	if (cnt > 31) {
		for (size_t i = 0; i < (sizeof(r.i32) / sizeof(r.i32[0]));
		     i++) {
			r.u32[i] = (a.i32[i] < 0) ? UINT32_MAX : 0;
		}
	} else if (cnt == 0) {
		memcpy(&r, &a, sizeof(r));
	} else {
		const uint32_t m = (uint32_t)(
			(~0U) << ((sizeof(int32_t) * CHAR_BIT) - cnt));
		for (size_t i = 0; i < (sizeof(r.i32) / sizeof(r.i32[0]));
		     i++) {
			const uint32_t is_neg = a.i32[i] < 0;
			r.u32[i] = (a.u32[i] >> cnt) | (m * is_neg);
		}
	}

	return r;
#endif
}
#define simde_m_psrad(a, b) simde_mm_sra_pi32(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_sub_pi8(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_sub_pi8(a.n, b.n));
#else
	simde__m64 r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < 8; i++) {
		r.i8[i] = a.i8[i] - b.i8[i];
	}
	return r;
#endif
}
#define simde_m_psubb(a, b) simde_mm_sub_pi8(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_sub_pi16(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_sub_pi16(a.n, b.n));
#else
	simde__m64 r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (8 / sizeof(int16_t)); i++) {
		r.i16[i] = a.i16[i] - b.i16[i];
	}
	return r;
#endif
}
#define simde_m_psubw(a, b) simde_mm_sub_pi16(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_sub_pi32(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_sub_pi32(a.n, b.n));
#else
	simde__m64 r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (8 / sizeof(int)); i++) {
		r.i32[i] = a.i32[i] - b.i32[i];
	}
	return r;
#endif
}
#define simde_m_psubd(a, b) simde_mm_sub_pi32(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_subs_pi8(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_subs_pi8(a.n, b.n));
#else
	simde__m64 r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (8); i++) {
		if (((b.i8[i]) > 0 && (a.i8[i]) < INT8_MIN + (b.i8[i]))) {
			r.i8[i] = INT8_MIN;
		} else if ((b.i8[i]) < 0 && (a.i8[i]) > INT8_MAX + (b.i8[i])) {
			r.i8[i] = INT8_MAX;
		} else {
			r.i8[i] = (a.i8[i]) - (b.i8[i]);
		}
	}
	return r;
#endif
}
#define simde_m_psubsb(a, b) simde_mm_subs_pi8(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_subs_pu8(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_subs_pu8(a.n, b.n));
#else
	simde__m64 r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (8); i++) {
		const int32_t x = a.u8[i] - b.u8[i];
		if (x < 0) {
			r.u8[i] = 0;
		} else if (x > UINT8_MAX) {
			r.u8[i] = UINT8_MAX;
		} else {
			r.u8[i] = (uint8_t)x;
		}
	}
	return r;
#endif
}
#define simde_m_psubusb(a, b) simde_mm_subs_pu8(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_subs_pi16(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_subs_pi16(a.n, b.n));
#else
	simde__m64 r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (8 / sizeof(int16_t)); i++) {
		if (((b.i16[i]) > 0 && (a.i16[i]) < SHRT_MIN + (b.i16[i]))) {
			r.i16[i] = SHRT_MIN;
		} else if ((b.i16[i]) < 0 &&
			   (a.i16[i]) > INT16_MAX + (b.i16[i])) {
			r.i16[i] = INT16_MAX;
		} else {
			r.i16[i] = (a.i16[i]) - (b.i16[i]);
		}
	}
	return r;
#endif
}
#define simde_m_psubsw(a, b) simde_mm_subs_pi16(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_subs_pu16(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_subs_pu16(a.n, b.n));
#else
	simde__m64 r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (8 / sizeof(uint16_t)); i++) {
		const int x = a.u16[i] - b.u16[i];
		if (x < 0) {
			r.u16[i] = 0;
		} else if (x > UINT16_MAX) {
			r.u16[i] = UINT16_MAX;
		} else {
			r.u16[i] = (uint16_t)x;
		}
	}
	return r;
#endif
}
#define simde_m_psubusw(a, b) simde_mm_subs_pu16(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_unpackhi_pi8(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_unpackhi_pi8(a.n, b.n));
#else
	simde__m64 r;
	r.i8[0] = a.i8[4];
	r.i8[1] = b.i8[4];
	r.i8[2] = a.i8[5];
	r.i8[3] = b.i8[5];
	r.i8[4] = a.i8[6];
	r.i8[5] = b.i8[6];
	r.i8[6] = a.i8[7];
	r.i8[7] = b.i8[7];
	return r;
#endif
}
#define simde_m_punpckhbw(a, b) simde_mm_unpackhi_pi8(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_unpackhi_pi16(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_unpackhi_pi16(a.n, b.n));
#else
	simde__m64 r;
	r.i16[0] = a.i16[2];
	r.i16[1] = b.i16[2];
	r.i16[2] = a.i16[3];
	r.i16[3] = b.i16[3];
	return r;
#endif
}
#define simde_m_punpckhwd(a, b) simde_mm_unpackhi_pi16(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_unpackhi_pi32(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_unpackhi_pi32(a.n, b.n));
#else
	simde__m64 r;
	r.i32[0] = a.i32[1];
	r.i32[1] = b.i32[1];
	return r;
#endif
}
#define simde_m_punpckhdq(a, b) simde_mm_unpackhi_pi32(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_unpacklo_pi8(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_unpacklo_pi8(a.n, b.n));
#else
	simde__m64 r;
	r.i8[0] = a.i8[0];
	r.i8[1] = b.i8[0];
	r.i8[2] = a.i8[1];
	r.i8[3] = b.i8[1];
	r.i8[4] = a.i8[2];
	r.i8[5] = b.i8[2];
	r.i8[6] = a.i8[3];
	r.i8[7] = b.i8[3];
	return r;
#endif
}
#define simde_m_punpcklbw(a, b) simde_mm_unpacklo_pi8(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_unpacklo_pi16(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_unpacklo_pi16(a.n, b.n));
#else
	simde__m64 r;
	r.i16[0] = a.i16[0];
	r.i16[1] = b.i16[0];
	r.i16[2] = a.i16[1];
	r.i16[3] = b.i16[1];
	return r;
#endif
}
#define simde_m_punpcklwd(a, b) simde_mm_unpacklo_pi16(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_unpacklo_pi32(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_unpacklo_pi32(a.n, b.n));
#else
	simde__m64 r;
	r.i32[0] = a.i32[0];
	r.i32[1] = b.i32[0];
	return r;
#endif
}
#define simde_m_punpckldq(a, b) simde_mm_unpacklo_pi32(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_xor_si64(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_MMX_NATIVE)
	return SIMDE__M64_C(_mm_xor_si64(a.n, b.n));
#else
	simde__m64 r;
	r.i64[0] = a.i64[0] ^ b.i64[0];
	return r;
#endif
}
#define simde_m_pxor(a, b) simde_mm_xor_si64(a, b)

SIMDE__FUNCTION_ATTRIBUTES
int32_t simde_m_to_int(simde__m64 a)
{
#if defined(SIMDE_MMX_NATIVE)
	return _m_to_int(a.n);
#else
	return a.i32[0];
#endif
}

SIMDE__END_DECLS

#endif /* !defined(SIMDE__MMX_H) */
