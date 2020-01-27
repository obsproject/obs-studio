/* Permission is hereby granted, free of charge, to any person
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
 *
 * Copyright:
 *   2017      Evan Nemerson <evan@nemerson.com>
 *   2015-2017 John W. Ratcliff <jratcliffscarab@gmail.com>
 *   2015      Brandon Rowlett <browlett@nvidia.com>
 *   2015      Ken Fast <kfast@gdeb.com>
 *   2017      Hasindu Gamaarachchi <hasindu@unsw.edu.au>
 *   2018      Jeff Daily <jeff.daily@amd.com>
 */

#if !defined(SIMDE__SSE2_H)
#if !defined(SIMDE__SSE2_H)
#define SIMDE__SSE2_H
#endif
#include "sse.h"

#if defined(SIMDE_SSE2_NATIVE)
#undef SIMDE_SSE2_NATIVE
#endif
#if defined(SIMDE_SSE2_FORCE_NATIVE)
#define SIMDE_SSE2_NATIVE
#elif defined(__SSE2__) && !defined(SIMDE_SSE2_NO_NATIVE) && \
	!defined(SIMDE_NO_NATIVE)
#define SIMDE_SSE2_NATIVE
#elif defined(__ARM_NEON) && !defined(SIMDE_SSE2_NO_NEON) && \
	!defined(SIMDE_NO_NEON)
#define SIMDE_SSE2_NEON
#endif

#if defined(SIMDE_SSE2_NATIVE) && !defined(SIMDE_SSE_NATIVE)
#if defined(SIMDE_SSE2_FORCE_NATIVE)
#error Native SSE2 support requires native SSE support
#else
#warning Native SSE2 support requires native SSE support, disabling
#undef SIMDE_SSE2_NATIVE
#endif
#elif defined(SIMDE_SSE2_NEON) && !defined(SIMDE_SSE_NEON)
#warning SSE2 NEON support requires SSE NEON support, disabling
#undef SIMDE_SSE_NEON
#endif

#if defined(SIMDE_SSE2_NATIVE)
#include <emmintrin.h>
#else
#if defined(SIMDE_SSE2_NEON)
#include <arm_neon.h>
#endif
#endif

#include <stdint.h>
#include <limits.h>
#include <string.h>

#define vreinterpretq_m128i_s32(v) \
	(simde__m128i) { .neon_i32 = v }
#define vreinterpretq_m128i_u64(v) \
	(simde__m128i) { .neon_u64 = v }

#define vreinterpretq_s32_m128i(a) a.neon_i32
#define vreinterpretq_u64_m128i(a) a.neon_u64

SIMDE__BEGIN_DECLS

typedef SIMDE_ALIGN(16) union {
#if defined(SIMDE__ENABLE_GCC_VEC_EXT)
	int8_t i8 __attribute__((__vector_size__(16), __may_alias__));
	int16_t i16 __attribute__((__vector_size__(16), __may_alias__));
	int32_t i32 __attribute__((__vector_size__(16), __may_alias__));
	int64_t i64 __attribute__((__vector_size__(16), __may_alias__));
	uint8_t u8 __attribute__((__vector_size__(16), __may_alias__));
	uint16_t u16 __attribute__((__vector_size__(16), __may_alias__));
	uint32_t u32 __attribute__((__vector_size__(16), __may_alias__));
	uint64_t u64 __attribute__((__vector_size__(16), __may_alias__));
#if defined(SIMDE__HAVE_INT128)
	simde_int128 i128 __attribute__((__vector_size__(16), __may_alias__));
	simde_uint128 u128 __attribute__((__vector_size__(16), __may_alias__));
#endif
	simde_float32 f32 __attribute__((__vector_size__(16), __may_alias__));
	simde_float64 f64 __attribute__((__vector_size__(16), __may_alias__));
#else
	int8_t i8[16];
	int16_t i16[8];
	int32_t i32[4];
	int64_t i64[2];
	uint8_t u8[16];
	uint16_t u16[8];
	uint32_t u32[4];
	uint64_t u64[2];
#if defined(SIMDE__HAVE_INT128)
	simde_int128 i128[1];
	simde_uint128 u128[1];
#endif
	simde_float32 f32[4];
	simde_float64 f64[2];
#endif

#if defined(SIMDE_SSE2_NATIVE)
	__m128i n;
#elif defined(SIMDE_SSE2_NEON)
	int8x16_t neon_i8;
	int16x8_t neon_i16;
	int32x4_t neon_i32;
	int64x2_t neon_i64;
	uint8x16_t neon_u8;
	uint16x8_t neon_u16;
	uint32x4_t neon_u32;
	uint64x2_t neon_u64;
	float32x4_t neon_f32;
#if defined(SIMDE_ARCH_AMD64)
	float64x2_t neon_f64;
#endif
#endif
} simde__m128i;

typedef SIMDE_ALIGN(16) union {
#if defined(SIMDE__ENABLE_GCC_VEC_EXT)
	int8_t i8 __attribute__((__vector_size__(16), __may_alias__));
	int16_t i16 __attribute__((__vector_size__(16), __may_alias__));
	int32_t i32 __attribute__((__vector_size__(16), __may_alias__));
	int64_t i64 __attribute__((__vector_size__(16), __may_alias__));
	uint8_t u8 __attribute__((__vector_size__(16), __may_alias__));
	uint16_t u16 __attribute__((__vector_size__(16), __may_alias__));
	uint32_t u32 __attribute__((__vector_size__(16), __may_alias__));
	uint64_t u64 __attribute__((__vector_size__(16), __may_alias__));
	simde_float32 f32 __attribute__((__vector_size__(16), __may_alias__));
	simde_float64 f64 __attribute__((__vector_size__(16), __may_alias__));
#else
	int8_t i8[16];
	int16_t i16[8];
	int32_t i32[4];
	int64_t i64[2];
	uint8_t u8[16];
	uint16_t u16[8];
	uint32_t u32[4];
	uint64_t u64[2];
	simde_float32 f32[4];
	simde_float64 f64[2];
#endif

#if defined(SIMDE_SSE2_NATIVE)
	__m128d n;
#elif defined(SIMDE_SSE2_NEON)
	int8x16_t neon_i8;
	int16x8_t neon_i16;
	int32x4_t neon_i32;
	int64x2_t neon_i64;
	uint8x16_t neon_u8;
	uint16x8_t neon_u16;
	uint32x4_t neon_u32;
	uint64x2_t neon_u64;
	float32x4_t neon_f32;
#if defined(SIMDE_ARCH_AMD64)
	float64x2_t neon_f64;
#endif
#endif
} simde__m128d;

#if defined(SIMDE_SSE2_NATIVE)
HEDLEY_STATIC_ASSERT(sizeof(__m128i) == sizeof(simde__m128i),
		     "__m128i size doesn't match simde__m128i size");
HEDLEY_STATIC_ASSERT(sizeof(__m128d) == sizeof(simde__m128d),
		     "__m128d size doesn't match simde__m128d size");
SIMDE__FUNCTION_ATTRIBUTES simde__m128i SIMDE__M128I_C(__m128i v)
{
	simde__m128i r;
	r.n = v;
	return r;
}
SIMDE__FUNCTION_ATTRIBUTES simde__m128d SIMDE__M128D_C(__m128d v)
{
	simde__m128d r;
	r.n = v;
	return r;
}
#elif defined(SIMDE_SSE_NEON)
#define SIMDE__M128I_NEON_C(T, expr) \
	(simde__m128i) { .neon_##T = expr }
#define SIMDE__M128D_NEON_C(T, expr) \
	(simde__m128d) { .neon_##T = expr }
#endif
HEDLEY_STATIC_ASSERT(16 == sizeof(simde__m128i), "simde__m128i size incorrect");
HEDLEY_STATIC_ASSERT(16 == sizeof(simde__m128d), "simde__m128d size incorrect");

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_add_epi8(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_add_epi8(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	return SIMDE__M128I_NEON_C(i8, vaddq_s8(a.neon_i8, b.neon_i8));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i8) / sizeof(r.i8[0])); i++) {
		r.i8[i] = a.i8[i] + b.i8[i];
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_add_epi16(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_add_epi16(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	return SIMDE__M128I_NEON_C(i16, vaddq_s16(a.neon_i16, b.neon_i16));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i16) / sizeof(r.i16[0])); i++) {
		r.i16[i] = a.i16[i] + b.i16[i];
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_add_epi32(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_add_epi32(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	return SIMDE__M128I_NEON_C(i32, vaddq_s32(a.neon_i32, b.neon_i32));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i32) / sizeof(r.i32[0])); i++) {
		r.i32[i] = a.i32[i] + b.i32[i];
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_add_epi64(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_add_epi64(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	return SIMDE__M128I_NEON_C(i64, vaddq_s64(a.neon_i64, b.neon_i64));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i64) / sizeof(r.i64[0])); i++) {
		r.i64[i] = a.i64[i] + b.i64[i];
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_add_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128D_C(_mm_add_pd(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON) && defined(SIMDE_ARCH_AMD64)
	return SIMDE__M128I_NEON_C(f64, vaddq_f64(a.neon_f64, b.neon_f64));
#else
	simde__m128d r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f64) / sizeof(r.f64[0])); i++) {
		r.f64[i] = a.f64[i] + b.f64[i];
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_add_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128D_C(_mm_add_sd(a.n, b.n));
#else
	simde__m128d r;
	r.f64[0] = a.f64[0] + b.f64[0];
	r.f64[1] = a.f64[1];
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_add_si64(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M64_C(_mm_add_si64(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	return SIMDE__M64_NEON_C(i64, vadd_s64(a.neon_i64, b.neon_i64));
#else
	simde__m64 r;
	r.i64[0] = a.i64[0] + b.i64[0];
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_adds_epi8(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_adds_epi8(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	return SIMDE__M128I_NEON_C(i8, vqaddq_s8(a.neon_i8, b.neon_i8));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i8) / sizeof(r.i8[0])); i++) {
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

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_adds_epi16(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_adds_epi16(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	return SIMDE__M128I_NEON_C(i16, vqaddq_s16(a.neon_i16, b.neon_i16));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i16) / sizeof(r.i16[0])); i++) {
		if ((((b.i16[i]) > 0) &&
		     ((a.i16[i]) > (INT16_MAX - (b.i16[i]))))) {
			r.i16[i] = INT16_MAX;
		} else if ((((b.i16[i]) < 0) &&
			    ((a.i16[i]) < (INT16_MIN - (b.i16[i]))))) {
			r.i16[i] = INT16_MIN;
		} else {
			r.i16[i] = (a.i16[i]) + (b.i16[i]);
		}
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_adds_epu8(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_adds_epu8(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	return SIMDE__M128I_NEON_C(u8, vqaddq_u8(a.neon_u8, b.neon_u8));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.u8) / sizeof(r.u8[0])); i++) {
		r.u8[i] = ((UINT8_MAX - a.u8[i]) > b.u8[i])
				  ? (a.u8[i] + b.u8[i])
				  : UINT8_MAX;
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_adds_epu16(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_adds_epu16(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	return SIMDE__M128I_NEON_C(u16, vqaddq_u16(a.neon_u16, b.neon_u16));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.u16) / sizeof(r.u16[0])); i++) {
		r.u16[i] = ((UINT16_MAX - a.u16[i]) > b.u16[i])
				   ? (a.u16[i] + b.u16[i])
				   : UINT16_MAX;
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_and_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128D_C(_mm_and_pd(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	return SIMDE__M128D_NEON_C(i32, vandq_s32(a.neon_i32, b.neon_i32));
#else
	simde__m128d r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.u64) / sizeof(r.u64[0])); i++) {
		r.u64[i] = a.u64[i] & b.u64[i];
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_and_si128(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_and_si128(a.n, b.n));
#elif defined(SIMDE_SSE_NEON)
	return SIMDE__M128I_NEON_C(i32, vandq_s32(b.neon_i32, a.neon_i32));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i64) / sizeof(r.i64[0])); i++) {
		r.i64[i] = a.i64[i] & b.i64[i];
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_andnot_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128D_C(_mm_andnot_pd(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	return SIMDE__M128D_NEON_C(i32, vbicq_s32(a.neon_i32, b.neon_i32));
#else
	simde__m128d r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.u64) / sizeof(r.u64[0])); i++) {
		r.u64[i] = ~a.u64[i] & b.u64[i];
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_andnot_si128(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_andnot_si128(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	return SIMDE__M128I_NEON_C(i32, vbicq_s32(b.neon_i32, a.neon_i32));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i64) / sizeof(r.i64[0])); i++) {
		r.i64[i] = ~(a.i64[i]) & b.i64[i];
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_avg_epu8(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_avg_epu8(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	return SIMDE__M128I_NEON_C(u8, vrhaddq_u8(b.neon_u8, a.neon_u8));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.u8) / sizeof(r.u8[0])); i++) {
		r.u8[i] = (a.u8[i] + b.u8[i] + 1) >> 1;
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_avg_epu16(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_avg_epu16(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	return SIMDE__M128I_NEON_C(u16, vrhaddq_u16(b.neon_u16, a.neon_u16));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.u16) / sizeof(r.u16[0])); i++) {
		r.u16[i] = (a.u16[i] + b.u16[i] + 1) >> 1;
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_bslli_si128(simde__m128i a, const int imm8)
{
	simde__m128i r;

	if (HEDLEY_UNLIKELY(imm8 > 15)) {
		r.u64[0] = 0;
		r.u64[1] = 0;
		return r;
	}

	const int s = imm8 * 8;

#if defined(SIMDE__HAVE_INT128)
	r.u128[0] = a.u128[0] << s;
#else
	if (s < 64) {
		r.u64[0] = (a.u64[0] << s);
		r.u64[1] = (a.u64[1] << s) | (a.u64[0] >> (64 - s));
	} else {
		r.u64[0] = 0;
		r.u64[1] = a.u64[0] << (s - 64);
	}
#endif

	return r;
}
#if defined(SIMDE_SSE2_NATIVE) && !defined(__PGI)
#define simde_mm_bslli_si128(a, imm8) SIMDE__M128I_C(_mm_slli_si128(a.n, imm8))
#elif defined(SIMDE_SSE2_NEON)
#define simde_mm_bslli_si128(a, imm8)                                      \
	SIMDE__M128I_NEON_C(                                               \
		i8,                                                        \
		(((imm8) <= 0) ? ((a).neon_i8)                             \
			       : (((imm8) > 15) ? (vdupq_n_s8(0))          \
						: (vextq_s8(vdupq_n_s8(0), \
							    (a).neon_i8,   \
							    16 - (imm8))))))
#endif
#define simde_mm_slli_si128(a, imm8) simde_mm_bslli_si128(a, imm8)

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_bsrli_si128(simde__m128i a, const int imm8)
{
	simde__m128i r;

	if (HEDLEY_UNLIKELY(imm8 > 15)) {
		r.u64[0] = 0;
		r.u64[1] = 0;
		return r;
	}

	const int s = imm8 * 8;

#if defined(SIMDE__HAVE_INT128)
	r.u128[0] = a.u128[0] >> s;
#else
	if (s < 64) {
		r.u64[0] = (a.u64[0] >> s) | (a.u64[1] << (64 - s));
		r.u64[1] = (a.u64[1] >> s);
	} else {
		r.u64[0] = a.u64[1] >> (s - 64);
		r.u64[1] = 0;
	}
#endif

	return r;
}
#if defined(SIMDE_SSE2_NATIVE) && !defined(__PGI)
#define simde_mm_bsrli_si128(a, imm8) SIMDE__M128I_C(_mm_srli_si128(a.n, imm8))
#elif defined(SIMDE_SSE2_NEON)
#define simde_mm_bsrli_si128(a, imm8)                             \
	SIMDE__M128I_NEON_C(                                      \
		i8,                                               \
		((imm8) <= 0)                                     \
			? ((a).neon_i8)                           \
			: (((imm8) > 15) ? (vdupq_n_s8(0))        \
					 : (vextq_s8((a).neon_i8, \
						     vdupq_n_s8(0), (imm8)))))
#endif
#define simde_mm_srli_si128(a, imm8) simde_mm_bsrli_si128(a, imm8)

SIMDE__FUNCTION_ATTRIBUTES
void simde_mm_clflush(void const *p)
{
#if defined(SIMDE_SSE2_NATIVE)
	_mm_clflush(p);
#else
	(void)p;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
int simde_mm_comieq_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return _mm_comieq_sd(a.n, b.n);
#else
	return a.f64[0] == b.f64[0];
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
int simde_mm_comige_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return _mm_comige_sd(a.n, b.n);
#else
	return a.f64[0] >= b.f64[0];
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
int simde_mm_comigt_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return _mm_comigt_sd(a.n, b.n);
#else
	return a.f64[0] > b.f64[0];
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
int simde_mm_comile_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return _mm_comile_sd(a.n, b.n);
#else
	return a.f64[0] <= b.f64[0];
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
int simde_mm_comilt_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return _mm_comilt_sd(a.n, b.n);
#else
	return a.f64[0] < b.f64[0];
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
int simde_mm_comineq_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return _mm_comineq_sd(a.n, b.n);
#else
	return a.f64[0] != b.f64[0];
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_castpd_ps(simde__m128d a)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128_C(_mm_castpd_ps(a.n));
#else
	union {
		simde__m128d pd;
		simde__m128 ps;
	} r;
	r.pd = a;
	return r.ps;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_castpd_si128(simde__m128d a)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_castpd_si128(a.n));
#else
	union {
		simde__m128d pd;
		simde__m128i si128;
	} r;
	r.pd = a;
	return r.si128;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_castps_pd(simde__m128 a)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128D_C(_mm_castps_pd(a.n));
#else
	union {
		simde__m128 ps;
		simde__m128d pd;
	} r;
	r.ps = a;
	return r.pd;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_castps_si128(simde__m128 a)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_castps_si128(a.n));
#elif defined(SIMDE_SSE2_NEON)
	return SIMDE__M128I_NEON_C(i32, a.neon_i32);
#else
	union {
		simde__m128 ps;
		simde__m128i si128;
	} r;
	r.ps = a;
	return r.si128;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_castsi128_pd(simde__m128i a)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128D_C(_mm_castsi128_pd(a.n));
#else
	union {
		simde__m128i si128;
		simde__m128d pd;
	} r;
	r.si128 = a;
	return r.pd;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_castsi128_ps(simde__m128i a)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128_C(_mm_castsi128_ps(a.n));
#elif defined(SIMDE_SSE2_NEON)
	return SIMDE__M128_NEON_C(f32, a.neon_f32);
#else
	union {
		simde__m128i si128;
		simde__m128 ps;
	} r;
	r.si128 = a;
	return r.ps;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_cmpeq_epi8(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_cmpeq_epi8(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	return SIMDE__M128I_NEON_C(
		i8, vreinterpretq_s8_u8(vceqq_s8(b.neon_i8, a.neon_i8)));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i8) / sizeof(r.i8[0])); i++) {
		r.i8[i] = (a.i8[i] == b.i8[i]) ? 0xff : 0x00;
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_cmpeq_epi16(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_cmpeq_epi16(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	return SIMDE__M128I_NEON_C(
		i16, vreinterpretq_s16_u16(vceqq_s16(b.neon_i16, a.neon_i16)));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i16) / sizeof(r.i16[0])); i++) {
		r.i16[i] = (a.i16[i] == b.i16[i]) ? 0xffff : 0x0000;
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_cmpeq_epi32(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_cmpeq_epi32(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	return SIMDE__M128I_NEON_C(
		i32, vreinterpretq_s32_u32(vceqq_s32(b.neon_i32, a.neon_i32)));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i32) / sizeof(r.i32[0])); i++) {
		r.i32[i] = (a.i32[i] == b.i32[i]) ? 0xffffffff : 0x00000000;
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmpeq_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128D_C(_mm_cmpeq_pd(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	return SIMDE__M128D_NEON_C(
		i32, vreinterpretq_s32_u32(
			     vceqq_s32(vreinterpretq_s32_f32(b.neon_f32),
				       vreinterpretq_s32_f32(a.neon_f32))));
#else
	simde__m128d r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f64) / sizeof(r.f64[0])); i++) {
		r.u64[i] = (a.f64[i] == b.f64[i]) ? ~UINT64_C(0) : UINT64_C(0);
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmpeq_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128D_C(_mm_cmpeq_sd(a.n, b.n));
#else
	simde__m128d r;
	r.u64[0] = (a.f64[0] == b.f64[0]) ? ~UINT64_C(0) : 0;
	r.u64[1] = a.u64[1];
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmpneq_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128D_C(_mm_cmpneq_pd(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	return SIMDE__M128D_NEON_C(f32,
				   vreinterpretq_f32_u16(vmvnq_u16(
					   vceqq_s16(b.neon_i16, a.neon_i16))));
#else
	simde__m128d r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f64) / sizeof(r.f64[0])); i++) {
		r.u64[i] = (a.f64[i] != b.f64[i]) ? ~UINT64_C(0) : UINT64_C(0);
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmpneq_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128D_C(_mm_cmpneq_sd(a.n, b.n));
#else
	simde__m128d r;
	r.u64[0] = (a.f64[0] != b.f64[0]) ? ~UINT64_C(0) : UINT64_C(0);
	r.u64[1] = a.u64[1];
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_cmplt_epi8(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_cmplt_epi8(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	return SIMDE__M128I_NEON_C(
		i8, vreinterpretq_s8_u8(vcltq_s8(a.neon_i8, b.neon_i8)));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i8) / sizeof(r.i8[0])); i++) {
		r.i8[i] = (a.i8[i] < b.i8[i]) ? 0xff : 0x00;
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_cmplt_epi16(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_cmplt_epi16(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	return SIMDE__M128I_NEON_C(
		i16, vreinterpretq_s16_u16(vcltq_s16(a.neon_i16, b.neon_i16)));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i16) / sizeof(r.i16[0])); i++) {
		r.i16[i] = (a.i16[i] < b.i16[i]) ? 0xffff : 0x0000;
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_cmplt_epi32(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_cmplt_epi32(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	return SIMDE__M128I_NEON_C(
		i32, vreinterpretq_s32_u32(vcltq_s32(a.neon_i32, b.neon_i32)));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i32) / sizeof(r.i32[0])); i++) {
		r.i32[i] = (a.i32[i] < b.i32[i]) ? 0xffffffff : 0x00000000;
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmplt_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128D_C(_mm_cmplt_pd(a.n, b.n));
#else
	simde__m128d r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f64) / sizeof(r.f64[0])); i++) {
		r.u64[i] = (a.f64[i] < b.f64[i]) ? ~UINT64_C(0) : UINT64_C(0);
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmplt_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128D_C(_mm_cmplt_sd(a.n, b.n));
#else
	simde__m128d r;
	r.u64[0] = (a.f64[0] < b.f64[0]) ? ~UINT64_C(0) : UINT64_C(0);
	r.u64[1] = a.u64[1];
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmple_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128D_C(_mm_cmple_pd(a.n, b.n));
#else
	simde__m128d r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f64) / sizeof(r.f64[0])); i++) {
		r.u64[i] = (a.f64[i] <= b.f64[i]) ? ~UINT64_C(0) : UINT64_C(0);
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmple_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128D_C(_mm_cmple_sd(a.n, b.n));
#else
	simde__m128d r;
	r.u64[0] = (a.f64[0] <= b.f64[0]) ? ~UINT64_C(0) : UINT64_C(0);
	r.u64[1] = a.u64[1];
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_cmpgt_epi8(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_cmpgt_epi8(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	return SIMDE__M128I_NEON_C(
		i8, vreinterpretq_s8_u8(vcgtq_s8(a.neon_i8, b.neon_i8)));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i8) / sizeof(r.i8[0])); i++) {
		r.i8[i] = (a.i8[i] > b.i8[i]) ? 0xff : 0x00;
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_cmpgt_epi16(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_cmpgt_epi16(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	return SIMDE__M128I_NEON_C(
		i16, vreinterpretq_s16_u16(vcgtq_s16(a.neon_i16, b.neon_i16)));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i16) / sizeof(r.i16[0])); i++) {
		r.i16[i] = (a.i16[i] > b.i16[i]) ? 0xffff : 0x0000;
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_cmpgt_epi32(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_cmpgt_epi32(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	return SIMDE__M128I_NEON_C(
		i32, vreinterpretq_s32_u32(vcgtq_s32(a.neon_i32, b.neon_i32)));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i32) / sizeof(r.i32[0])); i++) {
		r.i32[i] = (a.i32[i] > b.i32[i]) ? 0xffffffff : 0x00000000;
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmpgt_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128D_C(_mm_cmpgt_pd(a.n, b.n));
#else
	simde__m128d r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f64) / sizeof(r.f64[0])); i++) {
		r.u64[i] = (a.f64[i] > b.f64[i]) ? ~UINT64_C(0) : UINT64_C(0);
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmpgt_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE) && !defined(__PGI)
	return SIMDE__M128D_C(_mm_cmpgt_sd(a.n, b.n));
#else
	simde__m128d r;
	r.u64[0] = (a.f64[0] > b.f64[0]) ? ~UINT64_C(0) : UINT64_C(0);
	r.u64[1] = a.u64[1];
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmpge_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128D_C(_mm_cmpge_pd(a.n, b.n));
#else
	simde__m128d r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f64) / sizeof(r.f64[0])); i++) {
		r.u64[i] = (a.f64[i] >= b.f64[i]) ? ~UINT64_C(0) : UINT64_C(0);
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmpge_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE) && !defined(__PGI)
	return SIMDE__M128D_C(_mm_cmpge_sd(a.n, b.n));
#else
	simde__m128d r;
	r.u64[0] = (a.f64[0] >= b.f64[0]) ? ~UINT64_C(0) : UINT64_C(0);
	r.u64[1] = a.u64[1];
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmpnge_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128D_C(_mm_cmpnge_pd(a.n, b.n));
#else
	return simde_mm_cmplt_pd(a, b);
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmpnge_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE) && !defined(__PGI)
	return SIMDE__M128D_C(_mm_cmpnge_sd(a.n, b.n));
#else
	return simde_mm_cmplt_sd(a, b);
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmpnlt_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128D_C(_mm_cmpnlt_pd(a.n, b.n));
#else
	return simde_mm_cmpge_pd(a, b);
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmpnlt_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128D_C(_mm_cmpnlt_sd(a.n, b.n));
#else
	return simde_mm_cmpge_sd(a, b);
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmpnle_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128D_C(_mm_cmpnle_pd(a.n, b.n));
#else
	return simde_mm_cmpgt_pd(a, b);
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmpnle_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128D_C(_mm_cmpnle_sd(a.n, b.n));
#else
	return simde_mm_cmpgt_sd(a, b);
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmpord_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128D_C(_mm_cmpord_pd(a.n, b.n));
#else
	simde__m128d r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f64) / sizeof(r.f64[0])); i++) {
		r.u64[i] = (!isnan(a.f64[i]) && !isnan(b.f64[i])) ? ~UINT64_C(0)
								  : UINT64_C(0);
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmpord_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128D_C(_mm_cmpord_sd(a.n, b.n));
#else
	simde__m128d r;
	r.u64[0] = (!isnan(a.f64[0]) && !isnan(b.f64[0])) ? ~UINT64_C(0)
							  : UINT64_C(0);
	r.u64[1] = a.u64[1];
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmpunord_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128D_C(_mm_cmpunord_pd(a.n, b.n));
#else
	simde__m128d r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f64) / sizeof(r.f64[0])); i++) {
		r.u64[i] = (isnan(a.f64[i]) || isnan(b.f64[i])) ? ~UINT64_C(0)
								: UINT64_C(0);
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmpunord_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128D_C(_mm_cmpunord_sd(a.n, b.n));
#else
	simde__m128d r;
	r.u64[0] = (isnan(a.f64[0]) || isnan(b.f64[0])) ? ~UINT64_C(0)
							: UINT64_C(0);
	r.u64[1] = a.u64[1];
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cvtepi32_pd(simde__m128i a)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128D_C(_mm_cvtepi32_pd(a.n));
#else
	simde__m128d r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f64) / sizeof(r.f64[0])); i++) {
		r.f64[i] = (simde_float64)a.i32[i];
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cvtepi32_ps(simde__m128i a)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128_C(_mm_cvtepi32_ps(a.n));
#elif defined(SIMDE_SSE2_NEON)
	return SIMDE__M128_NEON_C(f32, vcvtq_f32_s32(a.neon_i32));
#else
	simde__m128 r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f32) / sizeof(r.f32[0])); i++) {
		r.f32[i] = (simde_float32)a.i32[i];
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_cvtpd_epi32(simde__m128d a)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_cvtpd_epi32(a.n));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f64) / sizeof(r.f64[0])); i++) {
		r.i32[i] = (int32_t)a.f64[i];
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_cvtpd_pi32(simde__m128d a)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M64_C(_mm_cvtpd_pi32(a.n));
#else
	simde__m64 r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i32) / sizeof(r.i32[0])); i++) {
		r.i32[i] = (int32_t)a.f64[i];
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cvtpd_ps(simde__m128d a)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128_C(_mm_cvtpd_ps(a.n));
#else
	simde__m128 r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(a.f64) / sizeof(a.f64[0])); i++) {
		r.f32[i] = (simde_float32)a.f64[i];
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cvtpi32_pd(simde__m64 a)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128D_C(_mm_cvtpi32_pd(a.n));
#else
	simde__m128d r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f64) / sizeof(r.f64[0])); i++) {
		r.f64[i] = (simde_float64)a.i32[i];
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_cvtps_epi32(simde__m128 a)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_cvtps_epi32(a.n));
#elif defined(SIMDE_SSE2_NEON)
/* The default rounding mode on SSE is 'round to even', which ArmV7
     does not support!  It is supported on ARMv8 however. */
#if defined(SIMDE_ARCH_AARCH64)
	return SIMDE__M128I_NEON_C(i32, vcvtnq_s32_f32(a.neon_f32));
#else
	uint32x4_t signmask = vdupq_n_u32(0x80000000);
	float32x4_t half = vbslq_f32(signmask, a.neon_f32,
				     vdupq_n_f32(0.5f)); /* +/- 0.5 */
	int32x4_t r_normal = vcvtq_s32_f32(
		vaddq_f32(a.neon_f32, half)); /* round to integer: [a + 0.5]*/
	int32x4_t r_trunc =
		vcvtq_s32_f32(a.neon_f32); /* truncate to integer: [a] */
	int32x4_t plusone = vshrq_n_s32(vnegq_s32(r_trunc), 31); /* 1 or 0 */
	int32x4_t r_even = vbicq_s32(vaddq_s32(r_trunc, plusone),
				     vdupq_n_s32(1)); /* ([a] + {0,1}) & ~1 */
	float32x4_t delta = vsubq_f32(
		a.neon_f32,
		vcvtq_f32_s32(r_trunc)); /* compute delta: delta = (a - [a]) */
	uint32x4_t is_delta_half =
		vceqq_f32(delta, half); /* delta == +/- 0.5 */
	return SIMDE__M128I_NEON_C(i32,
				   vbslq_s32(is_delta_half, r_even, r_normal));
#endif
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i32) / sizeof(r.i32[0])); i++) {
		r.i32[i] = (int32_t)a.f32[i];
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cvtps_pd(simde__m128 a)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128D_C(_mm_cvtps_pd(a.n));
#else
	simde__m128d r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f64) / sizeof(r.f64[0])); i++) {
		r.f64[i] = a.f32[i];
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
double simde_mm_cvtsd_f64(simde__m128d a)
{
#if defined(SIMDE_SSE2_NATIVE) && !defined(__PGI)
	return _mm_cvtsd_f64(a.n);
#else
	return a.f64[0];
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
int32_t simde_mm_cvtsd_si32(simde__m128d a)
{
#if defined(SIMDE_SSE2_NATIVE)
	return _mm_cvtsd_si32(a.n);
#else
	return (int32_t)a.f64[0];
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
int32_t simde_mm_cvtsd_si64(simde__m128d a)
{
#if defined(SIMDE_SSE2_NATIVE) && defined(SIMDE_ARCH_AMD64)
#if defined(__PGI)
	return _mm_cvtsd_si64x(a.n);
#else
	return _mm_cvtsd_si64(a.n);
#endif
#else
	return (int32_t)a.f64[0];
#endif
}
#define simde_mm_cvtsd_si64x(a) simde_mm_cvtsd_si64(a)

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cvtsd_ss(simde__m128 a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128_C(_mm_cvtsd_ss(a.n, b.n));
#else
	simde__m128 r;

	r.f32[0] = (simde_float32)b.f64[0];

	SIMDE__VECTORIZE
	for (size_t i = 1; i < (sizeof(r) / sizeof(r.i32[0])); i++) {
		r.i32[i] = a.i32[i];
	}

	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
int32_t simde_mm_cvtsi128_si32(simde__m128i a)
{
#if defined(SIMDE_SSE2_NATIVE)
	return _mm_cvtsi128_si32(a.n);
#elif defined(SIMDE_SSE2_NEON)
	return vgetq_lane_s32(a.neon_i32, 0);
#else
	return a.i32[0];
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
int64_t simde_mm_cvtsi128_si64(simde__m128i a)
{
#if defined(SIMDE_SSE2_NATIVE) && defined(SIMDE_ARCH_AMD64)
#if defined(__PGI)
	return _mm_cvtsi128_si64x(a.n);
#else
	return _mm_cvtsi128_si64(a.n);
#endif
#else
	return a.i64[0];
#endif
}
#define simde_mm_cvtsi128_si64x(a) simde_mm_cvtsi128_si64(a)

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cvtsi32_sd(simde__m128d a, int32_t b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128D_C(_mm_cvtsi32_sd(a.n, b));
#else
	simde__m128d r;

	r.f64[0] = (simde_float64)b;
	r.i64[1] = a.i64[1];

	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_cvtsi32_si128(int32_t a)
{
	simde__m128i r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_cvtsi32_si128(a);
#elif defined(SIMDE_SSE2_NEON)
	r.neon_i32 = vsetq_lane_s32(a, vdupq_n_s32(0), 0);
#else
	r.i32[0] = a;
	r.i32[1] = 0;
	r.i32[2] = 0;
	r.i32[3] = 0;
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cvtsi64_sd(simde__m128d a, int32_t b)
{
	simde__m128d r;

#if defined(SIMDE_SSE2_NATIVE) && defined(SIMDE_ARCH_AMD64)
#if !defined(__PGI)
	r.n = _mm_cvtsi64_sd(a.n, b);
#else
	r.n = _mm_cvtsi64x_sd(a.n, b);
#endif
#else
	r.f64[0] = (simde_float64)b;
	r.f64[1] = a.f64[1];
#endif

	return r;
}
#define simde_mm_cvtsi64x_sd(a, b) simde_mm_cvtsi64(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_cvtsi64_si128(int64_t a)
{
	simde__m128i r;

#if defined(SIMDE_SSE2_NATIVE) && defined(SIMDE_ARCH_AMD64)
#if !defined(__PGI)
	r.n = _mm_cvtsi64_si128(a);
#else
	r.n = _mm_cvtsi64x_si128(a);
#endif
#else
	r.i64[0] = a;
	r.i64[1] = 0;
#endif

	return r;
}
#define simde_mm_cvtsi64x_si128(a) simde_mm_cvtsi64_si128(a)

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cvtss_sd(simde__m128d a, simde__m128 b)
{
	simde__m128d r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_cvtss_sd(a.n, b.n);
#else
	r.f64[0] = b.f32[0];
	r.i64[1] = a.i64[1];
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_cvttpd_epi32(simde__m128d a)
{
	simde__m128i r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_cvttpd_epi32(a.n);
#else
	for (size_t i = 0; i < (sizeof(a.f64) / sizeof(a.f64[0])); i++) {
		r.i32[i] = (int32_t)trunc(a.f64[i]);
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_cvttpd_pi32(simde__m128d a)
{
	simde__m64 r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_cvttpd_pi32(a.n);
#else
	for (size_t i = 0; i < (sizeof(r.i32) / sizeof(r.i32[0])); i++) {
		r.i32[i] = (int32_t)trunc(a.f64[i]);
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_cvttps_epi32(simde__m128 a)
{
	simde__m128i r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_cvttps_epi32(a.n);
#elif defined(SIMDE_SSE2_NEON)
	r.neon_i32 = vcvtq_s32_f32(a.neon_f32);
#else
	for (size_t i = 0; i < (sizeof(r.i32) / sizeof(r.i32[0])); i++) {
		r.i32[i] = (int32_t)truncf(a.f32[i]);
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
int32_t simde_mm_cvttsd_si32(simde__m128d a)
{
#if defined(SIMDE_SSE2_NATIVE)
	return _mm_cvttsd_si32(a.n);
#else
	return (int32_t)trunc(a.f64[0]);
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
int64_t simde_mm_cvttsd_si64(simde__m128d a)
{
#if defined(SIMDE_SSE2_NATIVE) && defined(SIMDE_ARCH_AMD64)
#if !defined(__PGI)
	return _mm_cvttsd_si64(a.n);
#else
	return _mm_cvttsd_si64x(a.n);
#endif
#else
	return (int64_t)trunc(a.f64[0]);
#endif
}
#define simde_mm_cvttsd_si64x(a) simde_mm_cvttsd_si64(a)

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_div_pd(simde__m128d a, simde__m128d b)
{
	simde__m128d r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_div_pd(a.n, b.n);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f64) / sizeof(r.f64[0])); i++) {
		r.f64[i] = a.f64[i] / b.f64[i];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_div_sd(simde__m128d a, simde__m128d b)
{
	simde__m128d r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_div_sd(a.n, b.n);
#else
	r.f64[0] = a.f64[0] / b.f64[0];
	r.f64[1] = a.f64[1];
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
int32_t simde_mm_extract_epi16(simde__m128i a, const int imm8)
{
	return a.u16[imm8 & 7];
}
#if defined(SIMDE_SSE2_NATIVE) && \
	(!defined(SIMDE__REALLY_GCC) || HEDLEY_GCC_VERSION_CHECK(4, 6, 0))
#define simde_mm_extract_epi16(a, imm8) _mm_extract_epi16(a.n, imm8)
#elif defined(SIMDE_SSE2_NEON)
#define simde_mm_extract_epi16(a, imm8) \
	(vgetq_lane_s16((a).neon_i16, (imm8)) & ((int32_t)UINT32_C(0x0000ffff)))
#endif

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_insert_epi16(simde__m128i a, int32_t i, const int imm8)
{
	a.u16[imm8 & 7] = (int16_t)i;
	return a;
}
#if defined(SIMDE_SSE2_NATIVE) && !defined(__PGI)
#define simde_mm_insert_epi16(a, i, imm8) \
	SIMDE__M128I_C(_mm_insert_epi16((a).n, (i), (imm8)))
#elif defined(SIMDE_SSE2_NEON)
#define simde_mm_insert_epi16(a, i, imm8) \
	SIMDE__M128I_NEON_C(i16, vsetq_lane_s16((i), a.neon_i16, (imm8)))
#endif

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d
simde_mm_load_pd(simde_float64 const mem_addr[HEDLEY_ARRAY_PARAM(2)])
{
	simde__m128d r;

	simde_assert_aligned(16, mem_addr);

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_load_pd(mem_addr);
#elif defined(SIMDE_SSE2_NEON)
	r.neon_u32 = vld1q_u32((uint32_t const *)mem_addr);
#else
	SIMDE__ASSUME_ALIGNED(mem_addr, 16);
	memcpy(&r, mem_addr, sizeof(r));
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_load_pd1(simde_float64 const *mem_addr)
{
	simde__m128d r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_load_pd1(mem_addr);
#else
	r.f64[0] = *mem_addr;
	r.f64[1] = *mem_addr;
#endif

	return r;
}
#define simde_mm_load1_pd(mem_addr) simde_mm_load_pd1(mem_addr)

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_load_sd(simde_float64 const *mem_addr)
{
	simde__m128d r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_load_sd(mem_addr);
#else
	memcpy(&r, mem_addr, sizeof(simde_float64));
	r.u64[1] = 0;
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_load_si128(simde__m128i const *mem_addr)
{
	simde__m128i r;

	simde_assert_aligned(16, mem_addr);

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_load_si128(&(mem_addr->n));
#elif defined(SIMDE_SSE2_NEON)
	r.neon_i32 = vld1q_s32((int32_t const *)mem_addr);
#else
	SIMDE__ASSUME_ALIGNED(mem_addr, 16);
	memcpy(&r, mem_addr, sizeof(r));
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_loadh_pd(simde__m128d a, simde_float64 const *mem_addr)
{
	simde__m128d r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_loadh_pd(a.n, mem_addr);
#else
	simde_float64 t;
	memcpy(&t, mem_addr, sizeof(t));
	r.f64[0] = a.f64[0];
	r.f64[1] = t;
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_loadl_epi64(simde__m128i const *mem_addr)
{
	simde__m128i r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_loadl_epi64(&mem_addr->n);
#elif defined(SIMDE_SSE2_NEON)
	r.neon_i32 = vcombine_s32(vld1_s32((int32_t const *)mem_addr),
				  vcreate_s32(0));
#else
	r.u64[0] = mem_addr->u64[0];
	r.u64[1] = 0;
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_loadl_pd(simde__m128d a, simde_float64 const *mem_addr)
{
	simde__m128d r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_loadl_pd(a.n, mem_addr);
#else
	memcpy(&r, mem_addr, sizeof(simde_float64));
	r.u64[1] = a.u64[1];
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d
simde_mm_loadr_pd(simde_float64 const mem_addr[HEDLEY_ARRAY_PARAM(2)])
{
	simde__m128d r;

	simde_assert_aligned(16, mem_addr);

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_loadr_pd(mem_addr);
#else
	SIMDE__ASSUME_ALIGNED(mem_addr, 16);
	r.f64[0] = mem_addr[1];
	r.f64[1] = mem_addr[0];
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d
simde_mm_loadu_pd(simde_float64 const mem_addr[HEDLEY_ARRAY_PARAM(2)])
{
	simde__m128d r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_loadu_pd(mem_addr);
#else
	simde_float64 l, h;
	memcpy(&l, &mem_addr[0], sizeof(l));
	memcpy(&h, &mem_addr[1], sizeof(h));
	r.f64[0] = l;
	r.f64[1] = h;
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_loadu_si128(simde__m128i const *mem_addr)
{
	simde__m128i r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_loadu_si128(&((*mem_addr).n));
#elif defined(SIMDE_SSE2_NEON)
	r.neon_i32 = vld1q_s32((int32_t const *)mem_addr);
#else
	memcpy(&r, mem_addr, sizeof(r));
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_madd_epi16(simde__m128i a, simde__m128i b)
{
	simde__m128i r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_madd_epi16(a.n, b.n);
#elif defined(SIMDE_SSE2_NEON)
	int32x4_t pl =
		vmull_s16(vget_low_s16(a.neon_i16), vget_low_s16(b.neon_i16));
	int32x4_t ph =
		vmull_s16(vget_high_s16(a.neon_i16), vget_high_s16(b.neon_i16));
	int32x2_t rl = vpadd_s32(vget_low_s32(pl), vget_high_s32(pl));
	int32x2_t rh = vpadd_s32(vget_low_s32(ph), vget_high_s32(ph));
	r.neon_i32 = vcombine_s32(rl, rh);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r) / sizeof(r.i16[0])); i += 2) {
		r.i32[i / 2] =
			(a.i16[i] * b.i16[i]) + (a.i16[i + 1] * b.i16[i + 1]);
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
void simde_mm_maskmoveu_si128(simde__m128i a, simde__m128i mask,
			      int8_t mem_addr[HEDLEY_ARRAY_PARAM(16)])
{
#if defined(SIMDE_SSE2_NATIVE)
	_mm_maskmoveu_si128(a.n, mask.n, (char *)mem_addr);
#else
	for (size_t i = 0; i < 16; i++) {
		if (mask.u8[i] & 0x80) {
			mem_addr[i] = a.i8[i];
		}
	}
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
int32_t simde_mm_movemask_epi8(simde__m128i a)
{
#if defined(SIMDE_SSE2_NATIVE)
	return _mm_movemask_epi8(a.n);
#elif defined(SIMDE_SSE2_NEON)
	uint8x16_t input = a.neon_u8;
	SIMDE_ALIGN(16)
	static const int8_t xr[8] = {-7, -6, -5, -4, -3, -2, -1, 0};
	uint8x8_t mask_and = vdup_n_u8(0x80);
	int8x8_t mask_shift = vld1_s8(xr);

	uint8x8_t lo = vget_low_u8(input);
	uint8x8_t hi = vget_high_u8(input);

	lo = vand_u8(lo, mask_and);
	lo = vshl_u8(lo, mask_shift);

	hi = vand_u8(hi, mask_and);
	hi = vshl_u8(hi, mask_shift);

	lo = vpadd_u8(lo, lo);
	lo = vpadd_u8(lo, lo);
	lo = vpadd_u8(lo, lo);

	hi = vpadd_u8(hi, hi);
	hi = vpadd_u8(hi, hi);
	hi = vpadd_u8(hi, hi);

	return ((hi[0] << 8) | (lo[0] & 0xFF));
#else
	int32_t r = 0;
	SIMDE__VECTORIZE_REDUCTION(| : r)
	for (size_t i = 0; i < 16; i++) {
		r |= (a.u8[15 - i] >> 7) << (15 - i);
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
int32_t simde_mm_movemask_pd(simde__m128d a)
{
#if defined(SIMDE_SSE2_NATIVE)
	return _mm_movemask_pd(a.n);
#else
	int32_t r = 0;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(a.u64) / sizeof(a.u64[0])); i++) {
		r |= (a.u64[i] >> 63) << i;
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_movepi64_pi64(simde__m128i a)
{
	simde__m64 r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_movepi64_pi64(a.n);
#else
	r.i64[0] = a.i64[0];
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_movpi64_epi64(simde__m64 a)
{
	simde__m128i r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_movpi64_epi64(a.n);
#else
	r.i64[0] = a.i64[0];
	r.i64[1] = 0;
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_min_epi16(simde__m128i a, simde__m128i b)
{
	simde__m128i r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_min_epi16(a.n, b.n);
#elif defined(SIMDE_SSE2_NEON)
	r.neon_i16 = vminq_s16(a.neon_i16, b.neon_i16);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i16) / sizeof(r.i16[0])); i++) {
		r.i16[i] = (a.i16[i] < b.i16[i]) ? a.i16[i] : b.i16[i];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_min_epu8(simde__m128i a, simde__m128i b)
{
	simde__m128i r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_min_epu8(a.n, b.n);
#elif defined(SIMDE_SSE2_NEON)
	r.neon_u8 = vminq_u8(a.neon_u8, b.neon_u8);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.u8) / sizeof(r.u8[0])); i++) {
		r.u8[i] = (a.u8[i] < b.u8[i]) ? a.u8[i] : b.u8[i];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_min_pd(simde__m128d a, simde__m128d b)
{
	simde__m128d r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_min_pd(a.n, b.n);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f64) / sizeof(r.f64[0])); i++) {
		r.f64[i] = (a.f64[i] < b.f64[i]) ? a.f64[i] : b.f64[i];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_min_sd(simde__m128d a, simde__m128d b)
{
	simde__m128d r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_min_sd(a.n, b.n);
#else
	r.f64[0] = (a.f64[0] < b.f64[0]) ? a.f64[0] : b.f64[0];
	r.f64[1] = a.f64[1];
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_max_epi16(simde__m128i a, simde__m128i b)
{
	simde__m128i r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_max_epi16(a.n, b.n);
#elif defined(SIMDE_SSE2_NEON)
	r.neon_i16 = vmaxq_s16(a.neon_i16, b.neon_i16);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i16) / sizeof(r.i16[0])); i++) {
		r.i16[i] = (a.i16[i] > b.i16[i]) ? a.i16[i] : b.i16[i];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_max_epu8(simde__m128i a, simde__m128i b)
{
	simde__m128i r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_max_epu8(a.n, b.n);
#elif defined(SIMDE_SSE2_NEON)
	r.neon_u8 = vmaxq_u8(a.neon_u8, b.neon_u8);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.u8) / sizeof(r.u8[0])); i++) {
		r.u8[i] = (a.u8[i] > b.u8[i]) ? a.u8[i] : b.u8[i];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_max_pd(simde__m128d a, simde__m128d b)
{
	simde__m128d r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_max_pd(a.n, b.n);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f64) / sizeof(r.f64[0])); i++) {
		r.f64[i] = (a.f64[i] > b.f64[i]) ? a.f64[i] : b.f64[i];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_max_sd(simde__m128d a, simde__m128d b)
{
	simde__m128d r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_max_sd(a.n, b.n);
#else
	r.f64[0] = (a.f64[0] > b.f64[0]) ? a.f64[0] : b.f64[0];
	r.f64[1] = a.f64[1];
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_move_epi64(simde__m128i a)
{
	simde__m128i r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_move_epi64(a.n);
#elif defined(SIMDE_SSE2_NEON)
	r.neon_i64 = vsetq_lane_s64(0, a.neon_i64, 1);
#else
	r.i64[0] = a.i64[0];
	r.i64[1] = 0;
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_move_sd(simde__m128d a, simde__m128d b)
{
	simde__m128d r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_move_sd(a.n, b.n);
#else
	r.f64[0] = b.f64[0];
	r.f64[1] = a.f64[1];
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_mul_epu32(simde__m128i a, simde__m128i b)
{
	simde__m128i r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_mul_epu32(a.n, b.n);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.u64) / sizeof(r.u64[0])); i++) {
		r.u64[i] = ((uint64_t)a.u32[i * 2]) * ((uint64_t)b.u32[i * 2]);
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_x_mm_mul_epi64(simde__m128i a, simde__m128i b)
{
	simde__m128i r;

	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i64) / sizeof(r.i64[0])); i++) {
		r.i64[i] = a.i64[i] * b.i64[i];
	}

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_x_mm_mod_epi64(simde__m128i a, simde__m128i b)
{
	simde__m128i r;

	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i64) / sizeof(r.i64[0])); i++) {
		r.i64[i] = a.i64[i] % b.i64[i];
	}

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_mul_pd(simde__m128d a, simde__m128d b)
{
	simde__m128d r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_mul_pd(a.n, b.n);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f64) / sizeof(r.f64[0])); i++) {
		r.f64[i] = a.f64[i] * b.f64[i];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_mul_sd(simde__m128d a, simde__m128d b)
{
	simde__m128d r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_mul_sd(a.n, b.n);
#else
	r.f64[0] = a.f64[0] * b.f64[0];
	r.f64[1] = a.f64[1];
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_mul_su32(simde__m64 a, simde__m64 b)
{
	simde__m64 r;

#if defined(SIMDE_SSE2_NATIVE) && !defined(__PGI)
	r.n = _mm_mul_su32(a.n, b.n);
#else
	r.u64[0] = ((uint64_t)a.u32[0]) * ((uint64_t)b.u32[0]);
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_mulhi_epi16(simde__m128i a, simde__m128i b)
{
	simde__m128i r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_mulhi_epi16(a.n, b.n);
#elif defined(SIMDE_SSE2_NEON)
	int16x4_t a3210 = vget_low_s16(a.neon_i16);
	int16x4_t b3210 = vget_low_s16(b.neon_i16);
	int32x4_t ab3210 = vmull_s16(a3210, b3210); /* 3333222211110000 */
	int16x4_t a7654 = vget_high_s16(a.neon_i16);
	int16x4_t b7654 = vget_high_s16(b.neon_i16);
	int32x4_t ab7654 = vmull_s16(a7654, b7654); /* 7777666655554444 */
	uint16x8x2_t rv = vuzpq_u16(vreinterpretq_u16_s32(ab3210),
				    vreinterpretq_u16_s32(ab7654));
	r.neon_u16 = rv.val[1];
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i16) / sizeof(r.i16[0])); i++) {
		r.u16[i] = (uint16_t)(((uint32_t)(((int32_t)a.i16[i]) *
						  ((int32_t)b.i16[i]))) >>
				      16);
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_mulhi_epu16(simde__m128i a, simde__m128i b)
{
	simde__m128i r;

#if defined(SIMDE_SSE2_NATIVE) && !defined(__PGI)
	r.n = _mm_mulhi_epu16(a.n, b.n);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.u16) / sizeof(r.u16[0])); i++) {
		r.u16[i] = (uint16_t)(
			(((uint32_t)a.u16[i]) * ((uint32_t)b.u16[i])) >> 16);
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_mullo_epi16(simde__m128i a, simde__m128i b)
{
	simde__m128i r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_mullo_epi16(a.n, b.n);
#elif defined(SIMDE_SSE2_NEON)
	r.neon_i16 = vmulq_s16(a.neon_i16, b.neon_i16);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i16) / sizeof(r.i16[0])); i++) {
		r.u16[i] = (uint16_t)(((uint32_t)(((int32_t)a.i16[i]) *
						  ((int32_t)b.i16[i]))) &
				      0xffff);
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_or_pd(simde__m128d a, simde__m128d b)
{
	simde__m128d r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_or_pd(a.n, b.n);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i64) / sizeof(r.i64[0])); i++) {
		r.i64[i] = a.i64[i] | b.i64[i];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_or_si128(simde__m128i a, simde__m128i b)
{
	simde__m128i r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_or_si128(a.n, b.n);
#elif defined(SIMDE_SSE2_NEON)
	r.neon_i32 = vorrq_s32(a.neon_i32, b.neon_i32);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i64) / sizeof(r.i64[0])); i++) {
		r.i64[i] = a.i64[i] | b.i64[i];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_packs_epi16(simde__m128i a, simde__m128i b)
{
	simde__m128i r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_packs_epi16(a.n, b.n);
#elif defined(SIMDE_SSE2_NEON)
	r.neon_i8 = vcombine_s8(vqmovn_s16(a.neon_i16), vqmovn_s16(b.neon_i16));
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i16) / sizeof(r.i16[0])); i++) {
		r.i8[i] = (a.i16[i] > INT8_MAX)
				  ? INT8_MAX
				  : ((a.i16[i] < INT8_MIN)
					     ? INT8_MIN
					     : ((int8_t)a.i16[i]));
		r.i8[i + 8] = (b.i16[i] > INT8_MAX)
				      ? INT8_MAX
				      : ((b.i16[i] < INT8_MIN)
						 ? INT8_MIN
						 : ((int8_t)b.i16[i]));
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_packs_epi32(simde__m128i a, simde__m128i b)
{
	simde__m128i r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_packs_epi32(a.n, b.n);
#elif defined(SIMDE_SSE2_NEON)
	r.neon_i16 =
		vcombine_s16(vqmovn_s32(a.neon_i32), vqmovn_s32(b.neon_i32));
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i32) / sizeof(r.i32[0])); i++) {
		r.i16[i] = (a.i32[i] > INT16_MAX)
				   ? INT16_MAX
				   : ((a.i32[i] < INT16_MIN)
					      ? INT16_MIN
					      : ((int16_t)a.i32[i]));
		r.i16[i + 4] = (b.i32[i] > INT16_MAX)
				       ? INT16_MAX
				       : ((b.i32[i] < INT16_MIN)
						  ? INT16_MIN
						  : ((int16_t)b.i32[i]));
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_packus_epi16(simde__m128i a, simde__m128i b)
{
	simde__m128i r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_packus_epi16(a.n, b.n);
#elif defined(SIMDE_SSE2_NEON)
	r.neon_u8 =
		vcombine_u8(vqmovun_s16(a.neon_i16), vqmovun_s16(b.neon_i16));
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i16) / sizeof(r.i16[0])); i++) {
		r.u8[i] = (a.i16[i] > UINT8_MAX)
				  ? UINT8_MAX
				  : ((a.i16[i] < 0) ? 0 : ((int8_t)a.i16[i]));
		r.u8[i + 8] =
			(b.i16[i] > UINT8_MAX)
				? UINT8_MAX
				: ((b.i16[i] < 0) ? 0 : ((int8_t)b.i16[i]));
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
void simde_mm_pause(void)
{
#if defined(SIMDE_SSE2_NATIVE)
	_mm_pause();
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_sad_epu8(simde__m128i a, simde__m128i b)
{
	simde__m128i r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_sad_epu8(a.n, b.n);
#else
	for (size_t i = 0; i < (sizeof(r.i64) / sizeof(r.i64[0])); i++) {
		uint16_t tmp = 0;
		SIMDE__VECTORIZE_REDUCTION(+ : tmp)
		for (size_t j = 0; j < ((sizeof(r.u8) / sizeof(r.u8[0])) / 2);
		     j++) {
			const size_t e = j + (i * 8);
			tmp += (a.u8[e] > b.u8[e]) ? (a.u8[e] - b.u8[e])
						   : (b.u8[e] - a.u8[e]);
		}
		r.i64[i] = tmp;
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_set_epi8(int8_t e15, int8_t e14, int8_t e13, int8_t e12,
			       int8_t e11, int8_t e10, int8_t e9, int8_t e8,
			       int8_t e7, int8_t e6, int8_t e5, int8_t e4,
			       int8_t e3, int8_t e2, int8_t e1, int8_t e0)
{
	simde__m128i r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_set_epi8(e15, e14, e13, e12, e11, e10, e9, e8, e7, e6, e5, e4,
			   e3, e2, e1, e0);
#else
	r.i8[0] = e0;
	r.i8[1] = e1;
	r.i8[2] = e2;
	r.i8[3] = e3;
	r.i8[4] = e4;
	r.i8[5] = e5;
	r.i8[6] = e6;
	r.i8[7] = e7;
	r.i8[8] = e8;
	r.i8[9] = e9;
	r.i8[10] = e10;
	r.i8[11] = e11;
	r.i8[12] = e12;
	r.i8[13] = e13;
	r.i8[14] = e14;
	r.i8[15] = e15;
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_set_epi16(int16_t e7, int16_t e6, int16_t e5, int16_t e4,
				int16_t e3, int16_t e2, int16_t e1, int16_t e0)
{
	simde__m128i r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_set_epi16(e7, e6, e5, e4, e3, e2, e1, e0);
#elif defined(SIMDE_SSE2_NEON)
	SIMDE_ALIGN(16) int16_t data[8] = {e0, e1, e2, e3, e4, e5, e6, e7};
	r.neon_i16 = vld1q_s16(data);
#else
	r.i16[0] = e0;
	r.i16[1] = e1;
	r.i16[2] = e2;
	r.i16[3] = e3;
	r.i16[4] = e4;
	r.i16[5] = e5;
	r.i16[6] = e6;
	r.i16[7] = e7;
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_set_epi32(int32_t e3, int32_t e2, int32_t e1, int32_t e0)
{
	simde__m128i r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_set_epi32(e3, e2, e1, e0);
#elif defined(SIMDE_SSE2_NEON)
	SIMDE_ALIGN(16) int32_t data[4] = {e0, e1, e2, e3};
	r.neon_i32 = vld1q_s32(data);
#else
	r.i32[0] = e0;
	r.i32[1] = e1;
	r.i32[2] = e2;
	r.i32[3] = e3;
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_set_epi64(simde__m64 e1, simde__m64 e0)
{
	simde__m128i r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_set_epi64(e1.n, e0.n);
#else
	r.i64[0] = e0.i64[0];
	r.i64[1] = e1.i64[0];
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_set_epi64x(int64_t e1, int64_t e0)
{
	simde__m128i r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_set_epi64x(e1, e0);
#elif defined(SIMDE_SSE2_NEON)
	r = SIMDE__M128I_NEON_C(i64,
				vcombine_s64(vdup_n_s64(e0), vdup_n_s64(e1)));
#else
	r.i64[0] = e0;
	r.i64[1] = e1;
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_x_mm_set_epu8(uint8_t e15, uint8_t e14, uint8_t e13,
				 uint8_t e12, uint8_t e11, uint8_t e10,
				 uint8_t e9, uint8_t e8, uint8_t e7, uint8_t e6,
				 uint8_t e5, uint8_t e4, uint8_t e3, uint8_t e2,
				 uint8_t e1, uint8_t e0)
{
	simde__m128i r;

	r.u8[0] = e0;
	r.u8[1] = e1;
	r.u8[2] = e2;
	r.u8[3] = e3;
	r.u8[4] = e4;
	r.u8[5] = e5;
	r.u8[6] = e6;
	r.u8[7] = e7;
	r.u8[8] = e8;
	r.u8[9] = e9;
	r.u8[10] = e10;
	r.u8[11] = e11;
	r.u8[12] = e12;
	r.u8[13] = e13;
	r.u8[14] = e14;
	r.u8[15] = e15;

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_x_mm_set_epu16(uint16_t e7, uint16_t e6, uint16_t e5,
				  uint16_t e4, uint16_t e3, uint16_t e2,
				  uint16_t e1, uint16_t e0)
{
	simde__m128i r;

	r.u16[0] = e0;
	r.u16[1] = e1;
	r.u16[2] = e2;
	r.u16[3] = e3;
	r.u16[4] = e4;
	r.u16[5] = e5;
	r.u16[6] = e6;
	r.u16[7] = e7;

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_x_mm_set_epu32(uint32_t e3, uint32_t e2, uint32_t e1,
				  uint32_t e0)
{
	simde__m128i r;

	r.u32[0] = e0;
	r.u32[1] = e1;
	r.u32[2] = e2;
	r.u32[3] = e3;

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_x_mm_set_epu64x(uint64_t e1, uint64_t e0)
{
	simde__m128i r;

	r.u64[0] = e0;
	r.u64[1] = e1;

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_set_pd(simde_float64 e1, simde_float64 e0)
{
	simde__m128d r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_set_pd(e1, e0);
#else
	r.f64[0] = e0;
	r.f64[1] = e1;
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_set_pd1(simde_float64 a)
{
	simde__m128d r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_set1_pd(a);
#else
	r.f64[0] = a;
	r.f64[1] = a;
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_set_sd(simde_float64 a)
{
	simde__m128d r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_set_sd(a);
#else
	r.f64[0] = a;
	r.u64[1] = 0;
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_set1_epi8(int8_t a)
{
	simde__m128i r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_set1_epi8(a);
#elif defined(SIMDE_SSE2_NEON)
	r.neon_i8 = vdupq_n_s8(a);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i8) / sizeof(r.i8[0])); i++) {
		r.i8[i] = a;
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_set1_epi16(int16_t a)
{
	simde__m128i r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_set1_epi16(a);
#elif defined(SIMDE_SSE2_NEON)
	r.neon_i16 = vdupq_n_s16(a);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i16) / sizeof(r.i16[0])); i++) {
		r.i16[i] = a;
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_set1_epi32(int32_t a)
{
	simde__m128i r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_set1_epi32(a);
#elif defined(SIMDE_SSE2_NEON)
	r.neon_i32 = vdupq_n_s32(a);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i32) / sizeof(r.i32[0])); i++) {
		r.i32[i] = a;
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_set1_epi64x(int64_t a)
{
	simde__m128i r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_set1_epi64x(a);
#elif defined(SIMDE_SSE2_NEON)
	r.neon_i64 = vmovq_n_s64(a);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i64) / sizeof(r.i64[0])); i++) {
		r.i64[i] = a;
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_set1_epi64(simde__m64 a)
{
	simde__m128i r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_set1_epi64(a.n);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i64) / sizeof(r.i64[0])); i++) {
		r.i64[i] = a.i64[0];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_set1_pd(simde_float64 a)
{
	simde__m128d r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_set1_pd(a);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i64) / sizeof(r.i64[0])); i++) {
		r.f64[i] = a;
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_setr_epi8(int8_t e15, int8_t e14, int8_t e13, int8_t e12,
				int8_t e11, int8_t e10, int8_t e9, int8_t e8,
				int8_t e7, int8_t e6, int8_t e5, int8_t e4,
				int8_t e3, int8_t e2, int8_t e1, int8_t e0)
{
	simde__m128i r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_setr_epi8(e15, e14, e13, e12, e11, e10, e9, e8, e7, e6, e5,
			    e4, e3, e2, e1, e0);
#elif defined(SIMDE_SSE2_NEON)
	int8_t t[] = {e15, e14, e13, e12, e11, e10, e9, e8,
		      e7,  e6,  e5,  e4,  e3,  e2,  e1, e0};
	r.neon_i8 = vld1q_s8(t);
#else
	r.i8[0] = e15;
	r.i8[1] = e14;
	r.i8[2] = e13;
	r.i8[3] = e12;
	r.i8[4] = e11;
	r.i8[5] = e10;
	r.i8[6] = e9;
	r.i8[7] = e8;
	r.i8[8] = e7;
	r.i8[9] = e6;
	r.i8[10] = e5;
	r.i8[11] = e4;
	r.i8[12] = e3;
	r.i8[13] = e2;
	r.i8[14] = e1;
	r.i8[15] = e0;
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_setr_epi16(int16_t e7, int16_t e6, int16_t e5, int16_t e4,
				 int16_t e3, int16_t e2, int16_t e1, int16_t e0)
{
	simde__m128i r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_setr_epi16(e7, e6, e5, e4, e3, e2, e1, e0);
#elif defined(SIMDE_SSE2_NEON)
	int16_t t[] = {e7, e6, e5, e4, e3, e2, e1, e0};
	r.neon_i16 = vld1q_s16(t);
#else
	r.i16[0] = e7;
	r.i16[1] = e6;
	r.i16[2] = e5;
	r.i16[3] = e4;
	r.i16[4] = e3;
	r.i16[5] = e2;
	r.i16[6] = e1;
	r.i16[7] = e0;
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_setr_epi32(int32_t e3, int32_t e2, int32_t e1, int32_t e0)
{
	simde__m128i r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_setr_epi32(e3, e2, e1, e0);
#elif defined(SIMDE_SSE2_NEON)
	int32_t t[] = {e3, e2, e1, e0};
	r.neon_i32 = vld1q_s32(t);
#else
	r.i32[0] = e3;
	r.i32[1] = e2;
	r.i32[2] = e1;
	r.i32[3] = e0;
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_setr_epi64(simde__m64 e1, simde__m64 e0)
{
	simde__m128i r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_setr_epi64(e1.n, e0.n);
#elif defined(SIMDE_SSE2_NEON)
	r.neon_i64 = vcombine_s64(e1.neon_i64, e0.neon_i64);
#else
	r.i64[0] = e1.i64[0];
	r.i64[1] = e0.i64[0];
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_setr_pd(simde_float64 e1, simde_float64 e0)
{
	simde__m128d r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_setr_pd(e1, e0);
#else
	r.f64[0] = e1;
	r.f64[1] = e0;
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_setzero_pd(void)
{
	simde__m128d r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_setzero_pd();
#else
	r.u64[0] = 0;
	r.u64[1] = 0;
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_setzero_si128(void)
{
	simde__m128i r;

#if defined(SIMDE_SSE2_NATIVE)
	r.n = _mm_setzero_si128();
#elif defined(SIMDE_SSE2_NEON)
	r.neon_i32 = vdupq_n_s32(0);
#else
	r.u64[0] = 0;
	r.u64[1] = 0;
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_shuffle_epi32(simde__m128i a, const int imm8)
{
	simde__m128i r;

	for (size_t i = 0; i < (sizeof(r.i32) / sizeof(r.i32[0])); i++) {
		r.i32[i] = a.i32[(imm8 >> (i * 2)) & 3];
	}

	return r;
}
#if defined(SIMDE_SSE2_NATIVE)
#define simde_mm_shuffle_epi32(a, imm8) \
	SIMDE__M128I_C(_mm_shuffle_epi32((a).n, (imm8)))
#elif defined(SIMDE__SHUFFLE_VECTOR)
#define simde_mm_shuffle_epi32(a, imm8)                                      \
	({                                                                   \
		const simde__m128i simde__tmp_a_ = a;                        \
		(simde__m128i){.i32 = SIMDE__SHUFFLE_VECTOR(                 \
				       32, 16, (simde__tmp_a_).i32,          \
				       (simde__tmp_a_).i32, ((imm8)) & 3,    \
				       ((imm8) >> 2) & 3, ((imm8) >> 4) & 3, \
				       ((imm8) >> 6) & 3)};                  \
	})
#endif

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_shuffle_pd(simde__m128d a, simde__m128d b, const int imm8)
{
	simde__m128d r;

	r.f64[0] = ((imm8 & 1) == 0) ? a.f64[0] : a.f64[1];
	r.f64[1] = ((imm8 & 2) == 0) ? b.f64[0] : b.f64[1];

	return r;
}
#if defined(SIMDE_SSE2_NATIVE) && !defined(__PGI)
#define simde_mm_shuffle_pd(a, b, imm8) \
	SIMDE__M128D_C(_mm_shuffle_pd((a).n, (b).n, (imm8)))
#elif defined(SIMDE__SHUFFLE_VECTOR)
#define simde_mm_shuffle_pd(a, b, imm8)                           \
	({                                                        \
		(simde__m128d){.f64 = SIMDE__SHUFFLE_VECTOR(      \
				       64, 16, (a).f64, (b).f64,  \
				       (((imm8)) & 1),            \
				       (((imm8) >> 1) & 1) + 2)}; \
	})
#endif

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_shufflehi_epi16(simde__m128i a, const int imm8)
{
	simde__m128i r;

	r.i64[0] = a.i64[0];
	for (size_t i = 4; i < (sizeof(r.i16) / sizeof(r.i16[0])); i++) {
		r.i16[i] = a.i16[((imm8 >> ((i - 4) * 2)) & 3) + 4];
	}

	return r;
}
#if defined(SIMDE_SSE2_NATIVE)
#define simde_mm_shufflehi_epi16(a, imm8) \
	SIMDE__M128I_C(_mm_shufflehi_epi16((a).n, (imm8)))
#elif defined(SIMDE__SHUFFLE_VECTOR)
#define simde_mm_shufflehi_epi16(a, imm8)                               \
	({                                                              \
		const simde__m128i simde__tmp_a_ = a;                   \
		(simde__m128i){.i16 = SIMDE__SHUFFLE_VECTOR(            \
				       16, 16, (simde__tmp_a_).i16,     \
				       (simde__tmp_a_).i16, 0, 1, 2, 3, \
				       (((imm8)) & 3) + 4,              \
				       (((imm8) >> 2) & 3) + 4,         \
				       (((imm8) >> 4) & 3) + 4,         \
				       (((imm8) >> 6) & 3) + 4)};       \
	})
#endif

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_shufflelo_epi16(simde__m128i a, const int imm8)
{
	simde__m128i r;

	for (size_t i = 0; i < ((sizeof(r.i16) / sizeof(r.i16[0])) / 2); i++) {
		r.i16[i] = a.i16[((imm8 >> (i * 2)) & 3)];
	}
	r.i64[1] = a.i64[1];

	return r;
}
#if defined(SIMDE_SSE2_NATIVE)
#define simde_mm_shufflelo_epi16(a, imm8) \
	SIMDE__M128I_C(_mm_shufflelo_epi16((a).n, (imm8)))
#elif defined(SIMDE__SHUFFLE_VECTOR)
#define simde_mm_shufflelo_epi16(a, imm8)                                   \
	({                                                                  \
		const simde__m128i simde__tmp_a_ = a;                       \
		(simde__m128i){.i16 = SIMDE__SHUFFLE_VECTOR(                \
				       16, 16, (simde__tmp_a_).i16,         \
				       (simde__tmp_a_).i16, (((imm8)) & 3), \
				       (((imm8) >> 2) & 3),                 \
				       (((imm8) >> 4) & 3),                 \
				       (((imm8) >> 6) & 3), 4, 5, 6, 7)};   \
	})
#endif

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_sll_epi16(simde__m128i a, simde__m128i count)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_sll_epi16(a.n, count.n));
#else
	simde__m128i r;

	if (count.u64[0] > 15)
		return simde_mm_setzero_si128();
	const int s = (int)(count.u64[0]);

	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.u16) / sizeof(r.u16[0])); i++) {
		r.u16[i] = a.u16[i] << s;
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_sll_epi32(simde__m128i a, simde__m128i count)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_sll_epi32(a.n, count.n));
#else
	simde__m128i r;

	if (count.u64[0] > 31)
		return simde_mm_setzero_si128();
	const int s = (int)(count.u64[0]);

	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i32) / sizeof(r.i32[0])); i++) {
		r.i32[i] = a.i32[i] << s;
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_sll_epi64(simde__m128i a, simde__m128i count)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_sll_epi64(a.n, count.n));
#else
	simde__m128i r;

	if (count.u64[0] > 63)
		return simde_mm_setzero_si128();
	const int s = (int)(count.u64[0]);

	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i64) / sizeof(r.i64[0])); i++) {
		r.i64[i] = a.i64[i] << s;
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_sqrt_pd(simde__m128d a)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128D_C(_mm_sqrt_pd(a.n));
#else
	simde__m128d r;

	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f64) / sizeof(r.f64[0])); i++) {
		r.f64[i] = sqrt(a.f64[i]);
	}

	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_sqrt_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128D_C(_mm_sqrt_sd(a.n, b.n));
#else
	simde__m128d r;
	r.f64[0] = sqrt(b.f64[0]);
	r.f64[1] = a.f64[1];
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_srl_epi16(simde__m128i a, simde__m128i count)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_srl_epi16(a.n, count.n));
#else
	simde__m128i r;

	if (count.u64[0] > 15)
		return simde_mm_setzero_si128();
	const int s = (int)(count.u64[0]);

	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.u16) / sizeof(r.u16[0])); i++) {
		r.u16[i] = a.u16[i] >> s;
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_srl_epi32(simde__m128i a, simde__m128i count)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_srl_epi32(a.n, count.n));
#else
	simde__m128i r;

	if (count.u64[0] > 31)
		return simde_mm_setzero_si128();
	const int s = (int)(count.u64[0]);

	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.u32) / sizeof(r.u32[0])); i++) {
		r.u32[i] = a.u32[i] >> s;
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_srl_epi64(simde__m128i a, simde__m128i count)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_srl_epi64(a.n, count.n));
#else
	simde__m128i r;

	if (count.u64[0] > 31)
		return simde_mm_setzero_si128();
	const int s = (int)(count.u64[0]);

	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.u64) / sizeof(r.u64[0])); i++) {
		r.u64[i] = a.u64[i] >> s;
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_srai_epi16(simde__m128i a, int imm8)
{
	simde__m128i r;

	const uint16_t m =
		(uint16_t)((~0U) << ((sizeof(int16_t) * CHAR_BIT) - imm8));

	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r) / sizeof(r.u16[0])); i++) {
		const uint16_t is_neg = ((uint16_t)(
			((a.u16[i]) >> ((sizeof(int16_t) * CHAR_BIT) - 1))));
		r.u16[i] = (a.u16[i] >> imm8) | (m * is_neg);
	}

	return r;
}
#if defined(SIMDE_SSE2_NATIVE)
#define simde_mm_srai_epi16(a, imm8) \
	SIMDE__M128I_C(_mm_srai_epi16((a).n, (imm8)));
#endif

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_srai_epi32(simde__m128i a, int imm8)
{
	simde__m128i r;

	const uint32_t m =
		(uint32_t)((~0U) << ((sizeof(int) * CHAR_BIT) - imm8));
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r) / sizeof(r.u32[0])); i++) {
		uint32_t is_neg = ((uint32_t)(
			((a.u32[i]) >> ((sizeof(int32_t) * CHAR_BIT) - 1))));
		r.u32[i] = (a.u32[i] >> imm8) | (m * is_neg);
	}

	return r;
}
#if defined(SIMDE_SSE2_NATIVE)
#define simde_mm_srai_epi32(a, imm8) \
	SIMDE__M128I_C(_mm_srai_epi32((a).n, (imm8)))
#elif defined(SIMDE_SSE2_NEON)
#define simde_mm_srai_epi32(a, imm8)                                           \
	SIMDE__M128I_NEON_C(                                                   \
		i32,                                                           \
		((imm8) <= 0)                                                  \
			? (a.neon_i32)                                         \
			: (((imm8) > 31)                                       \
				   ? (vshrq_n_s32(vshrq_n_s32(a.neon_i32, 16), \
						  16))                         \
				   : (vshrq_n_s32(a.neon_i32, (imm8)))))
#endif

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_sra_epi16(simde__m128i a, simde__m128i count)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_sra_epi16(a.n, count.n));
#else
	simde__m128i r;
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

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_sra_epi32(simde__m128i a, simde__m128i count)
{
#if defined(SIMDE_SSE2_NATIVE) && !defined(SIMDE_BUG_GCC_BAD_MM_SRA_EPI32)
	return SIMDE__M128I_C(_mm_sra_epi32(a.n, count.n));
#else
	simde__m128i r;
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

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_slli_epi16(simde__m128i a, const int imm8)
{
	simde__m128i r;
	const int s = (imm8 > ((int)sizeof(r.i16[0]) * CHAR_BIT) - 1) ? 0
								      : imm8;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i16) / sizeof(r.i16[0])); i++) {
		r.i16[i] = a.i16[i] << s;
	}
	return r;
}
#if defined(SIMDE_SSE2_NATIVE)
#define simde_mm_slli_epi16(a, imm8) SIMDE__M128I_C(_mm_slli_epi16(a.n, imm8));
#elif defined(SIMDE_SSE2_NEON)
#define simde_mm_slli_epi16(a, imm8)                                       \
	SIMDE__M128I_NEON_C(                                               \
		i16, ((imm8) <= 0)                                         \
			     ? ((a).neon_i16)                              \
			     : (((imm8) > 31) ? (vdupq_n_s16(0))           \
					      : (vshlq_n_s16((a).neon_i16, \
							     (imm8)))))
#endif

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_slli_epi32(simde__m128i a, const int imm8)
{
	simde__m128i r;
	const int s = (imm8 > ((int)sizeof(r.i32[0]) * CHAR_BIT) - 1) ? 0
								      : imm8;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i32) / sizeof(r.i32[0])); i++) {
		r.i32[i] = a.i32[i] << s;
	}
	return r;
}
#if defined(SIMDE_SSE2_NATIVE)
#define simde_mm_slli_epi32(a, imm8) SIMDE__M128I_C(_mm_slli_epi32(a.n, imm8));
#elif defined(SIMDE_SSE2_NEON)
#define simde_mm_slli_epi32(a, imm8)                                       \
	SIMDE__M128I_NEON_C(                                               \
		i32, ((imm8) <= 0)                                         \
			     ? ((a).neon_i32)                              \
			     : (((imm8) > 31) ? (vdupq_n_s32(0))           \
					      : (vshlq_n_s32((a).neon_i32, \
							     (imm8)))))
#endif

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_slli_epi64(simde__m128i a, const int imm8)
{
	simde__m128i r;
	const int s = (imm8 > ((int)sizeof(r.i64[0]) * CHAR_BIT) - 1) ? 0
								      : imm8;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i64) / sizeof(r.i64[0])); i++) {
		r.i64[i] = a.i64[i] << s;
	}
	return r;
}
#if defined(SIMDE_SSE2_NATIVE)
#define simde_mm_slli_epi64(a, imm8) SIMDE__M128I_C(_mm_slli_epi64(a.n, imm8));
#endif

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_srli_epi16(simde__m128i a, const int imm8)
{
	simde__m128i r;
	const int s = (imm8 > ((int)sizeof(r.i16[0]) * CHAR_BIT) - 1) ? 0
								      : imm8;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i16) / sizeof(r.i16[0])); i++) {
		r.u16[i] = a.u16[i] >> s;
	}
	return r;
}
#if defined(SIMDE_SSE2_NATIVE)
#define simde_mm_srli_epi16(a, imm8) SIMDE__M128I_C(_mm_srli_epi16(a.n, imm8));
#elif defined(SIMDE_SSE2_NEON)
#define simde_mm_srli_epi16(a, imm8)                                       \
	SIMDE__M128I_NEON_C(                                               \
		u16, ((imm8) <= 0)                                         \
			     ? ((a).neon_u16)                              \
			     : (((imm8) > 31) ? (vdupq_n_u16(0))           \
					      : (vshrq_n_u16((a).neon_u16, \
							     (imm8)))))
#endif

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_srli_epi32(simde__m128i a, const int imm8)
{
	simde__m128i r;
	const int s = (imm8 > ((int)sizeof(r.i32[0]) * CHAR_BIT) - 1) ? 0
								      : imm8;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i32) / sizeof(r.i32[0])); i++) {
		r.u32[i] = a.u32[i] >> s;
	}
	return r;
}
#if defined(SIMDE_SSE2_NATIVE)
#define simde_mm_srli_epi32(a, imm8) SIMDE__M128I_C(_mm_srli_epi32(a.n, imm8))
#elif defined(SIMDE_SSE2_NEON)
#define simde_mm_srli_epi32(a, imm8)                                       \
	SIMDE__M128I_NEON_C(                                               \
		u32, ((imm8) <= 0)                                         \
			     ? ((a).neon_u32)                              \
			     : (((imm8) > 31) ? (vdupq_n_u32(0))           \
					      : (vshrq_n_u32((a).neon_u32, \
							     (imm8)))))
#endif

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_srli_epi64(simde__m128i a, const int imm8)
{
	simde__m128i r;
	const unsigned char s = imm8 & 255;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i64) / sizeof(r.i64[0])); i++) {
		if (s > 63) {
			r.u64[i] = 0;
		} else {
			r.u64[i] = a.u64[i] >> s;
		}
	}
	return r;
}
#if defined(SIMDE_SSE2_NATIVE)
#define simde_mm_srli_epi64(a, imm8) SIMDE__M128I_C(_mm_srli_epi64(a.n, imm8))
#elif defined(SIMDE_SSE2_NEON)
#define simde_mm_srli_epi64(a, imm8)                    \
	SIMDE__M128I_NEON_C(                            \
		u64,                                    \
		(((imm8)&255) < 0 || ((imm8)&255) > 63) \
			? (vdupq_n_u64(0))              \
			: ((((imm8)&255) == 0)          \
				   ? (a.neon_u64)       \
				   : (vshrq_n_u64((a).neon_u64, (imm8)&255))))
#endif

SIMDE__FUNCTION_ATTRIBUTES
void simde_mm_store_pd(simde_float64 mem_addr[HEDLEY_ARRAY_PARAM(2)],
		       simde__m128d a)
{
	simde_assert_aligned(16, mem_addr);

#if defined(SIMDE_SSE2_NATIVE)
	_mm_store_pd(mem_addr, a.n);
#else
	SIMDE__ASSUME_ALIGNED(mem_addr, 16);
	memcpy(mem_addr, &a, sizeof(a));
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
void simde_mm_store1_pd(simde_float64 mem_addr[HEDLEY_ARRAY_PARAM(2)],
			simde__m128d a)
{
	simde_assert_aligned(16, mem_addr);

#if defined(SIMDE_SSE2_NATIVE)
	_mm_store1_pd(mem_addr, a.n);
#else
	SIMDE__ASSUME_ALIGNED(mem_addr, 16);
	mem_addr[0] = a.f64[0];
	mem_addr[1] = a.f64[0];
#endif
}
#define simde_mm_store_pd1(mem_addr, a) simde_mm_store1_pd(mem_addr, a)

SIMDE__FUNCTION_ATTRIBUTES
void simde_mm_store_sd(simde_float64 *mem_addr, simde__m128d a)
{
#if defined(SIMDE_SSE2_NATIVE)
	_mm_store_sd(mem_addr, a.n);
#else
	memcpy(mem_addr, &a, sizeof(a.f64[0]));
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
void simde_mm_store_si128(simde__m128i *mem_addr, simde__m128i a)
{
#if defined(SIMDE_SSE2_NATIVE)
	_mm_store_si128(&mem_addr->n, a.n);
#elif defined(SIMDE_SSE2_NEON)
	vst1q_s32((int32_t *)mem_addr, a.neon_i32);
#else
	SIMDE__ASSUME_ALIGNED(mem_addr, 16);
	memcpy(mem_addr, &a, sizeof(a));
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
void simde_mm_storeh_pd(simde_float64 *mem_addr, simde__m128d a)
{
#if defined(SIMDE_SSE2_NATIVE)
	_mm_storeh_pd(mem_addr, a.n);
#else
	*mem_addr = a.f64[1];
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
void simde_mm_storel_epi64(simde__m128i *mem_addr, simde__m128i a)
{
#if defined(SIMDE_SSE2_NATIVE)
	_mm_storel_epi64(&(mem_addr->n), a.n);
#elif defined(SIMDE_SSE2_NEON)
	mem_addr->i64[0] = vgetq_lane_s64(a.neon_i64, 0);
#else
	mem_addr->i64[0] = a.i64[0];
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
void simde_mm_storel_pd(simde_float64 *mem_addr, simde__m128d a)
{
#if defined(SIMDE_SSE2_NATIVE)
	_mm_storel_pd(mem_addr, a.n);
#else
	*mem_addr = a.f64[0];
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
void simde_mm_storer_pd(simde_float64 mem_addr[2], simde__m128d a)
{
	simde_assert_aligned(16, mem_addr);

#if defined(SIMDE_SSE2_NATIVE)
	_mm_storer_pd(mem_addr, a.n);
#else
	SIMDE__ASSUME_ALIGNED(mem_addr, 16);
	mem_addr[0] = a.f64[1];
	mem_addr[1] = a.f64[0];
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
void simde_mm_storeu_pd(simde_float64 *mem_addr, simde__m128d a)
{
#if defined(SIMDE_SSE2_NATIVE)
	_mm_storeu_pd(mem_addr, a.n);
#else
	memcpy(mem_addr, &a, sizeof(a));
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
void simde_mm_storeu_si128(simde__m128i *mem_addr, simde__m128i a)
{
#if defined(SIMDE_SSE2_NATIVE)
	_mm_storeu_si128(&mem_addr->n, a.n);
#elif defined(SIMDE_SSE2_NEON)
	int32_t v[4];
	vst1q_s32(v, a.neon_i32);
	memcpy(mem_addr, v, sizeof(v));
#else
	memcpy(mem_addr, &a, sizeof(a));
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
void simde_mm_stream_pd(simde_float64 mem_addr[HEDLEY_ARRAY_PARAM(2)],
			simde__m128d a)
{
#if defined(SIMDE_SSE2_NATIVE)
	_mm_stream_pd(mem_addr, a.n);
#else
	SIMDE__ASSUME_ALIGNED(mem_addr, 16);
	memcpy(mem_addr, &a, sizeof(a));
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
void simde_mm_stream_si128(simde__m128i *mem_addr, simde__m128i a)
{
#if defined(SIMDE_SSE2_NATIVE)
	_mm_stream_si128(&mem_addr->n, a.n);
#else
	SIMDE__ASSUME_ALIGNED(mem_addr, 16);
	memcpy(mem_addr, &a, sizeof(a));
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
void simde_mm_stream_si32(int32_t *mem_addr, int32_t a)
{
#if defined(SIMDE_SSE2_NATIVE)
	_mm_stream_si32(mem_addr, a);
#else
	*mem_addr = a;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
void simde_mm_stream_si64(int64_t *mem_addr, int64_t a)
{
#if defined(SIMDE_SSE2_NATIVE) && defined(SIMDE_ARCH_AMD64)
#if defined(SIMDE__REALLY_GCC) && !HEDLEY_GCC_VERSION_CHECK(4, 8, 0)
	*mem_addr = a;
#elif defined(__GNUC__)
	_mm_stream_si64((long long *)mem_addr, a);
#else
	_mm_stream_si64(mem_addr, a);
#endif
#else
	*mem_addr = a;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_sub_epi8(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_sub_epi8(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	return SIMDE__M128I_NEON_C(i8, vsubq_s8(a.neon_i8, b.neon_i8));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i8) / sizeof(r.i8[0])); i++) {
		r.i8[i] = a.i8[i] - b.i8[i];
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_sub_epi16(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_sub_epi16(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	return SIMDE__M128I_NEON_C(i16, vsubq_s16(a.neon_i16, b.neon_i16));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i16) / sizeof(r.i16[0])); i++) {
		r.i16[i] = a.i16[i] - b.i16[i];
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_sub_epi32(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_sub_epi32(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	return SIMDE__M128I_NEON_C(i32, vsubq_s32(a.neon_i32, b.neon_i32));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i32) / sizeof(r.i32[0])); i++) {
		r.i32[i] = a.i32[i] - b.i32[i];
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_sub_epi64(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_sub_epi64(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	return SIMDE__M128I_NEON_C(i64, vsubq_s64(a.neon_i64, b.neon_i64));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i64) / sizeof(r.i64[0])); i++) {
		r.i64[i] = a.i64[i] - b.i64[i];
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_sub_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128D_C(_mm_sub_pd(a.n, b.n));
#else
	simde__m128d r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f64) / sizeof(r.f64[0])); i++) {
		r.f64[i] = a.f64[i] - b.f64[i];
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_sub_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128D_C(_mm_sub_sd(a.n, b.n));
#else
	simde__m128d r;
	r.f64[0] = a.f64[0] - b.f64[0];
	r.f64[1] = a.f64[1];
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_sub_si64(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M64_C(_mm_sub_si64(a.n, b.n));
#else
	simde__m64 r;
	r.i64[0] = a.i64[0] - b.i64[0];
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_subs_epi8(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_subs_epi8(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	return SIMDE__M128I_NEON_C(i8, vqsubq_s8(a.neon_i8, b.neon_i8));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r) / sizeof(r.i8[0])); i++) {
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

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_subs_epi16(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_subs_epi16(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	return SIMDE__M128I_NEON_C(i16, vqsubq_s16(a.neon_i16, b.neon_i16));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r) / sizeof(r.i16[0])); i++) {
		if (((b.i16[i]) > 0 && (a.i16[i]) < INT16_MIN + (b.i16[i]))) {
			r.i16[i] = INT16_MIN;
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

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_subs_epu8(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_subs_epu8(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	return SIMDE__M128I_NEON_C(u8, vqsubq_u8(a.neon_u8, b.neon_u8));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r) / sizeof(r.i8[0])); i++) {
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

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_subs_epu16(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_subs_epu16(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	return SIMDE__M128I_NEON_C(u16, vqsubq_u16(a.neon_u16, b.neon_u16));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r) / sizeof(r.i16[0])); i++) {
		const int32_t x = a.u16[i] - b.u16[i];
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

SIMDE__FUNCTION_ATTRIBUTES
int simde_mm_ucomieq_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return _mm_ucomieq_sd(a.n, b.n);
#else
	fenv_t envp;
	int x = feholdexcept(&envp);
	int r = a.f64[0] == b.f64[0];
	if (HEDLEY_LIKELY(x == 0))
		fesetenv(&envp);
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
int simde_mm_ucomige_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return _mm_ucomige_sd(a.n, b.n);
#else
	fenv_t envp;
	int x = feholdexcept(&envp);
	int r = a.f64[0] >= b.f64[0];
	if (HEDLEY_LIKELY(x == 0))
		fesetenv(&envp);
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
int simde_mm_ucomigt_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return _mm_ucomigt_sd(a.n, b.n);
#else
	fenv_t envp;
	int x = feholdexcept(&envp);
	int r = a.f64[0] > b.f64[0];
	if (HEDLEY_LIKELY(x == 0))
		fesetenv(&envp);
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
int simde_mm_ucomile_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return _mm_ucomile_sd(a.n, b.n);
#else
	fenv_t envp;
	int x = feholdexcept(&envp);
	int r = a.f64[0] <= b.f64[0];
	if (HEDLEY_LIKELY(x == 0))
		fesetenv(&envp);
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
int simde_mm_ucomilt_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return _mm_ucomilt_sd(a.n, b.n);
#else
	fenv_t envp;
	int x = feholdexcept(&envp);
	int r = a.f64[0] < b.f64[0];
	if (HEDLEY_LIKELY(x == 0))
		fesetenv(&envp);
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
int simde_mm_ucomineq_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return _mm_ucomineq_sd(a.n, b.n);
#else
	fenv_t envp;
	int x = feholdexcept(&envp);
	int r = a.f64[0] != b.f64[0];
	if (HEDLEY_LIKELY(x == 0))
		fesetenv(&envp);
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_undefined_pd(void)
{
	simde__m128d r;

#if defined(SIMDE_SSE2_NATIVE) && defined(SIMDE__HAVE_UNDEFINED128)
	r.n = _mm_undefined_pd();
#else
	r = simde_mm_setzero_pd();
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_undefined_si128(void)
{
	simde__m128i r;

#if defined(SIMDE_SSE2_NATIVE) && defined(SIMDE__HAVE_UNDEFINED128)
	r.n = _mm_undefined_si128();
#else
	r = simde_mm_setzero_si128();
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
void simde_mm_lfence(void)
{
#if defined(SIMDE_SSE2_NATIVE)
	_mm_lfence();
#else
	simde_mm_sfence();
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
void simde_mm_mfence(void)
{
#if defined(SIMDE_SSE2_NATIVE)
	_mm_mfence();
#else
	simde_mm_sfence();
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_unpackhi_epi8(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_unpackhi_epi8(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	int8x8_t a1 = vreinterpret_s8_s16(vget_high_s16(a.neon_i16));
	int8x8_t b1 = vreinterpret_s8_s16(vget_high_s16(b.neon_i16));
	int8x8x2_t result = vzip_s8(a1, b1);
	return SIMDE__M128I_NEON_C(i8,
				   vcombine_s8(result.val[0], result.val[1]));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < ((sizeof(r) / sizeof(r.i8[0])) / 2); i++) {
		r.i8[(i * 2)] = a.i8[i + ((sizeof(r) / sizeof(r.i8[0])) / 2)];
		r.i8[(i * 2) + 1] =
			b.i8[i + ((sizeof(r) / sizeof(r.i8[0])) / 2)];
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_unpackhi_epi16(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_unpackhi_epi16(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	int16x4_t a1 = vget_high_s16(a.neon_i16);
	int16x4_t b1 = vget_high_s16(b.neon_i16);
	int16x4x2_t result = vzip_s16(a1, b1);
	return SIMDE__M128I_NEON_C(i16,
				   vcombine_s16(result.val[0], result.val[1]));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < ((sizeof(r) / sizeof(r.i16[0])) / 2); i++) {
		r.i16[(i * 2)] =
			a.i16[i + ((sizeof(r) / sizeof(r.i16[0])) / 2)];
		r.i16[(i * 2) + 1] =
			b.i16[i + ((sizeof(r) / sizeof(r.i16[0])) / 2)];
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_unpackhi_epi32(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_unpackhi_epi32(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	int32x2_t a1 = vget_high_s32(a.neon_i32);
	int32x2_t b1 = vget_high_s32(b.neon_i32);
	int32x2x2_t result = vzip_s32(a1, b1);
	return SIMDE__M128I_NEON_C(i32,
				   vcombine_s32(result.val[0], result.val[1]));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < ((sizeof(r) / sizeof(r.i32[0])) / 2); i++) {
		r.i32[(i * 2)] =
			a.i32[i + ((sizeof(r) / sizeof(r.i32[0])) / 2)];
		r.i32[(i * 2) + 1] =
			b.i32[i + ((sizeof(r) / sizeof(r.i32[0])) / 2)];
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_unpackhi_epi64(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_unpackhi_epi64(a.n, b.n));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < ((sizeof(r) / sizeof(r.i64[0])) / 2); i++) {
		r.i64[(i * 2)] =
			a.i64[i + ((sizeof(r) / sizeof(r.i64[0])) / 2)];
		r.i64[(i * 2) + 1] =
			b.i64[i + ((sizeof(r) / sizeof(r.i64[0])) / 2)];
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_unpackhi_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128D_C(_mm_unpackhi_pd(a.n, b.n));
#else
	simde__m128d r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < ((sizeof(r) / sizeof(r.f64[0])) / 2); i++) {
		r.f64[(i * 2)] =
			a.f64[i + ((sizeof(r) / sizeof(r.f64[0])) / 2)];
		r.f64[(i * 2) + 1] =
			b.f64[i + ((sizeof(r) / sizeof(r.f64[0])) / 2)];
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_unpacklo_epi8(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_unpacklo_epi8(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	int8x8_t a1 = vreinterpret_s8_s16(vget_low_s16(a.neon_i16));
	int8x8_t b1 = vreinterpret_s8_s16(vget_low_s16(b.neon_i16));
	int8x8x2_t result = vzip_s8(a1, b1);
	return SIMDE__M128I_NEON_C(i8,
				   vcombine_s8(result.val[0], result.val[1]));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < ((sizeof(r) / sizeof(r.i8[0])) / 2); i++) {
		r.i8[(i * 2)] = a.i8[i];
		r.i8[(i * 2) + 1] = b.i8[i];
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_unpacklo_epi16(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_unpacklo_epi16(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	int16x4_t a1 = vget_low_s16(a.neon_i16);
	int16x4_t b1 = vget_low_s16(b.neon_i16);
	int16x4x2_t result = vzip_s16(a1, b1);
	return SIMDE__M128I_NEON_C(i16,
				   vcombine_s16(result.val[0], result.val[1]));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < ((sizeof(r) / sizeof(r.i16[0])) / 2); i++) {
		r.i16[(i * 2)] = a.i16[i];
		r.i16[(i * 2) + 1] = b.i16[i];
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_unpacklo_epi32(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_unpacklo_epi32(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	int32x2_t a1 = vget_low_s32(a.neon_i32);
	int32x2_t b1 = vget_low_s32(b.neon_i32);
	int32x2x2_t result = vzip_s32(a1, b1);
	return SIMDE__M128I_NEON_C(i32,
				   vcombine_s32(result.val[0], result.val[1]));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < ((sizeof(r) / sizeof(r.i32[0])) / 2); i++) {
		r.i32[(i * 2)] = a.i32[i];
		r.i32[(i * 2) + 1] = b.i32[i];
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_unpacklo_epi64(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_unpacklo_epi64(a.n, b.n));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < ((sizeof(r) / sizeof(r.i64[0])) / 2); i++) {
		r.i64[(i * 2)] = a.i64[i];
		r.i64[(i * 2) + 1] = b.i64[i];
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_unpacklo_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128D_C(_mm_unpacklo_pd(a.n, b.n));
#else
	simde__m128d r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < ((sizeof(r) / sizeof(r.f64[0])) / 2); i++) {
		r.f64[(i * 2)] = a.f64[i];
		r.f64[(i * 2) + 1] = b.f64[i];
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128d simde_mm_xor_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128D_C(_mm_xor_pd(a.n, b.n));
#else
	simde__m128d r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i64) / sizeof(r.i64[0])); i++) {
		r.i64[i] = a.i64[i] ^ b.i64[i];
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_mm_xor_si128(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_SSE2_NATIVE)
	return SIMDE__M128I_C(_mm_xor_si128(a.n, b.n));
#elif defined(SIMDE_SSE2_NEON)
	return SIMDE__M128I_NEON_C(i32, veorq_s32(a.neon_i32, b.neon_i32));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i32) / sizeof(r.i32[0])); i++) {
		r.i32[i] = a.i32[i] ^ b.i32[i];
	}
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128i simde_x_mm_not_si128(simde__m128i a)
{
#if defined(SIMDE_SSE2_NEON)
	return SIMDE__M128I_NEON_C(i32, vmvnq_s32(a.neon_i32));
#else
	simde__m128i r;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i32) / sizeof(r.i32[0])); i++) {
		r.i32[i] = ~(a.i32[i]);
	}
	return r;
#endif
}

SIMDE__END_DECLS

#endif /* !defined(SIMDE__SSE2_H) */
