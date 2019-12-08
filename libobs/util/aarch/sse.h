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
 */

#if !defined(SIMDE__SSE_H)
#if !defined(SIMDE__SSE_H)
#define SIMDE__SSE_H
#endif
#include "mmx.h"

#if defined(SIMDE_SSE_NATIVE)
#undef SIMDE_SSE_NATIVE
#endif
#if defined(SIMDE_SSE_FORCE_NATIVE)
#define SIMDE_SSE_NATIVE
#elif defined(__SSE__) && !defined(SIMDE_SSE_NO_NATIVE) && \
	!defined(SIMDE_NO_NATIVE)
#define SIMDE_SSE_NATIVE
#elif defined(__ARM_NEON) && !defined(SIMDE_SSE_NO_NEON) && \
	!defined(SIMDE_NO_NEON)
#define SIMDE_SSE_NEON
#endif

#if defined(SIMDE_SSE_NATIVE) && !defined(SIMDE_MMX_NATIVE)
#if defined(SIMDE_SSE_FORCE_NATIVE)
#error Native SSE support requires native MMX support
#else
#warning Native SSE support requires native MMX support, disabling
#undef SIMDE_SSE_NATIVE
#endif
#elif defined(SIMDE_SSE_NEON) && !defined(SIMDE_MMX_NEON)
#warning SSE3 NEON support requires MMX NEON support, disabling
#undef SIMDE_SSE3_NEON
#endif

#if defined(SIMDE_SSE_NATIVE)
#include <xmmintrin.h>
#else
#if defined(SIMDE_SSE_NEON)
#include <arm_neon.h>
#endif

#if !defined(__INTEL_COMPILER) && defined(__STDC_VERSION__) && \
	(__STDC_VERSION__ >= 201112L) && !defined(__STDC_NO_ATOMICS__)
#include <stdatomic.h>
#elif defined(_WIN32)
#include <Windows.h>
#endif
#endif

#include <math.h>
#include <fenv.h>

#define SIMDE_ALIGN(alignment) __attribute__((aligned(alignment)))
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
#endif

#if defined(SIMDE_SSE_NATIVE)
	__m128 n;
#elif defined(SIMDE_SSE_NEON)
	int8x16_t neon_i8;
	int16x8_t neon_i16;
	int32x4_t neon_i32;
	int64x2_t neon_i64;
	uint8x16_t neon_u8;
	uint16x8_t neon_u16;
	uint32x4_t neon_u32;
	uint64x2_t neon_u64;
	float32x4_t neon_f32;
#endif
} simde__m128;

#if defined(SIMDE_SSE_NATIVE)
HEDLEY_STATIC_ASSERT(sizeof(__m128) == sizeof(simde__m128),
		     "__m128 size doesn't match simde__m128 size");
SIMDE__FUNCTION_ATTRIBUTES simde__m128 SIMDE__M128_C(__m128 v)
{
	simde__m128 r;
	r.n = v;
	return r;
}
#elif defined(SIMDE_SSE_NEON)
#define SIMDE__M128_NEON_C(T, expr) \
	(simde__m128) { .neon_##T = expr }
#endif
HEDLEY_STATIC_ASSERT(16 == sizeof(simde__m128), "simde__m128 size incorrect");

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_add_ps(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_add_ps(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	r.neon_f32 = vaddq_f32(a.neon_f32, b.neon_f32);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f32) / sizeof(r.f32[0])); i++) {
		r.f32[i] = a.f32[i] + b.f32[i];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_add_ss(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_add_ss(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	float32_t b0 = vgetq_lane_f32(b.neon_f32, 0);
	float32x4_t value = vsetq_lane_f32(b0, vdupq_n_f32(0), 0);
	/* the upper values in the result must be the remnants of <a>. */
	r.neon_f32 = vaddq_f32(a.neon_f32, value);
#elif defined(SIMDE__SHUFFLE_VECTOR) && defined(SIMDE_ASSUME_VECTORIZATION)
	r.f32 = SIMDE__SHUFFLE_VECTOR(32, 16, a.f32, simde_mm_add_ps(a, b).f32,
				      4, 1, 2, 3);
#else
	r.f32[0] = a.f32[0] + b.f32[0];
	r.f32[1] = a.f32[1];
	r.f32[2] = a.f32[2];
	r.f32[3] = a.f32[3];
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_and_ps(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_and_ps(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	r.neon_i32 = vandq_s32(a.neon_i32, b.neon_i32);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i32) / sizeof(r.i32[0])); i++) {
		r.i32[i] = a.i32[i] & b.i32[i];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_andnot_ps(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_andnot_ps(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	r.neon_i32 = vbicq_s32(b.neon_i32, a.neon_i32);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i32) / sizeof(r.i32[0])); i++) {
		r.i32[i] = ~(a.i32[i]) & b.i32[i];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_avg_pu16(simde__m64 a, simde__m64 b)
{
	simde__m64 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_avg_pu16(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	r.neon_u16 = vrhadd_u16(b.neon_u16, a.neon_u16);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < 4; i++) {
		r.u16[i] = (a.u16[i] + b.u16[i] + 1) >> 1;
	}
#endif

	return r;
}
#define simde_m_pavgw(a, b) simde_mm_avg_pu16(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_avg_pu8(simde__m64 a, simde__m64 b)
{
	simde__m64 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_avg_pu8(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	r.neon_u8 = vrhadd_u8(b.neon_u8, a.neon_u8);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < 8; i++) {
		r.u8[i] = (a.u8[i] + b.u8[i] + 1) >> 1;
	}
#endif

	return r;
}
#define simde_m_pavgb(a, b) simde_mm_avg_pu8(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cmpeq_ps(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_cmpeq_ps(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	r.neon_u32 = vceqq_f32(a.neon_f32, b.neon_f32);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f32) / sizeof(r.f32[0])); i++) {
		r.u32[i] = (a.f32[i] == b.f32[i]) ? 0xffffffff : 0;
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cmpeq_ss(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_cmpeq_ss(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	float32x4_t s =
		vreinterpretq_f32_u32(vceqq_f32(a.neon_f32, b.neon_f32));
	float32x4_t t = vextq_f32(a.neon_f32, s, 1);
	r.neon_f32 = vextq_f32(t, t, 3);
#elif defined(SIMDE__SHUFFLE_VECTOR) && defined(SIMDE_ASSUME_VECTORIZATION)
	r.f32 = SIMDE__SHUFFLE_VECTOR(32, 16, a.f32,
				      simde_mm_cmpeq_ps(a, b).f32, 4, 1, 2, 3);
#else
	r.u32[0] = (a.f32[0] == b.f32[0]) ? 0xffffffff : 0;
	SIMDE__VECTORIZE
	for (size_t i = 1; i < (sizeof(r.f32) / sizeof(r.f32[0])); i++) {
		r.u32[i] = a.u32[i];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cmpge_ps(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_cmpge_ps(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	r.neon_u32 = vcgeq_f32(a.neon_f32, b.neon_f32);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f32) / sizeof(r.f32[0])); i++) {
		r.u32[i] = (a.f32[i] >= b.f32[i]) ? 0xffffffff : 0;
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cmpge_ss(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE) && !defined(__PGI)
	r.n = _mm_cmpge_ss(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	float32x4_t s =
		vreinterpretq_f32_u32(vcgeq_f32(a.neon_f32, b.neon_f32));
	float32x4_t t = vextq_f32(a.neon_f32, s, 1);
	r.neon_f32 = vextq_f32(t, t, 3);
#elif defined(SIMDE__SHUFFLE_VECTOR) && defined(SIMDE_ASSUME_VECTORIZATION)
	r.f32 = SIMDE__SHUFFLE_VECTOR(32, 16, a.f32,
				      simde_mm_cmpge_ps(a, b).f32, 4, 1, 2, 3);
#else
	r.u32[0] = (a.f32[0] >= b.f32[0]) ? 0xffffffff : 0;
	SIMDE__VECTORIZE
	for (size_t i = 1; i < (sizeof(r.f32) / sizeof(r.f32[0])); i++) {
		r.u32[i] = a.u32[i];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cmpgt_ps(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_cmpgt_ps(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	r.neon_u32 = vcgtq_f32(a.neon_f32, b.neon_f32);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f32) / sizeof(r.f32[0])); i++) {
		r.u32[i] = (a.f32[i] > b.f32[i]) ? 0xffffffff : 0;
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cmpgt_ss(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE) && !defined(__PGI)
	r.n = _mm_cmpgt_ss(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	float32x4_t s =
		vreinterpretq_f32_u32(vcgtq_f32(a.neon_f32, b.neon_f32));
	float32x4_t t = vextq_f32(a.neon_f32, s, 1);
	r.neon_f32 = vextq_f32(t, t, 3);
#elif defined(SIMDE__SHUFFLE_VECTOR) && defined(SIMDE_ASSUME_VECTORIZATION)
	r.f32 = SIMDE__SHUFFLE_VECTOR(32, 16, a.f32,
				      simde_mm_cmpgt_ps(a, b).f32, 4, 1, 2, 3);
#else
	r.u32[0] = (a.f32[0] > b.f32[0]) ? 0xffffffff : 0;
	SIMDE__VECTORIZE
	for (size_t i = 1; i < (sizeof(r.f32) / sizeof(r.f32[0])); i++) {
		r.u32[i] = a.u32[i];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cmple_ps(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_cmple_ps(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	r.neon_u32 = vcleq_f32(a.neon_f32, b.neon_f32);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f32) / sizeof(r.f32[0])); i++) {
		r.u32[i] = (a.f32[i] <= b.f32[i]) ? 0xffffffff : 0;
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cmple_ss(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_cmple_ss(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	float32x4_t s =
		vreinterpretq_f32_u32(vcleq_f32(a.neon_f32, b.neon_f32));
	float32x4_t t = vextq_f32(a.neon_f32, s, 1);
	r.neon_f32 = vextq_f32(t, t, 3);
#elif defined(SIMDE__SHUFFLE_VECTOR) && defined(SIMDE_ASSUME_VECTORIZATION)
	r.f32 = SIMDE__SHUFFLE_VECTOR(32, 16, a.f32,
				      simde_mm_cmple_ps(a, b).f32, 4, 1, 2, 3);
#else
	r.u32[0] = (a.f32[0] <= b.f32[0]) ? 0xffffffff : 0;
	SIMDE__VECTORIZE
	for (size_t i = 1; i < (sizeof(r.f32) / sizeof(r.f32[0])); i++) {
		r.u32[i] = a.u32[i];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cmplt_ps(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_cmplt_ps(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	r.neon_u32 = vcltq_f32(a.neon_f32, b.neon_f32);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f32) / sizeof(r.f32[0])); i++) {
		r.u32[i] = (a.f32[i] < b.f32[i]) ? 0xffffffff : 0;
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cmplt_ss(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_cmplt_ss(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	float32x4_t s =
		vreinterpretq_f32_u32(vcltq_f32(a.neon_f32, b.neon_f32));
	float32x4_t t = vextq_f32(a.neon_f32, s, 1);
	r.neon_f32 = vextq_f32(t, t, 3);
#elif defined(SIMDE__SHUFFLE_VECTOR) && defined(SIMDE_ASSUME_VECTORIZATION)
	r.f32 = SIMDE__SHUFFLE_VECTOR(32, 16, a.f32,
				      simde_mm_cmplt_ps(a, b).f32, 4, 1, 2, 3);
#else
	r.u32[0] = (a.f32[0] < b.f32[0]) ? 0xffffffff : 0;
	SIMDE__VECTORIZE
	for (size_t i = 1; i < (sizeof(r.f32) / sizeof(r.f32[0])); i++) {
		r.u32[i] = a.u32[i];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cmpneq_ps(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_cmpneq_ps(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	r.neon_u32 = vmvnq_u32(vceqq_f32(a.neon_f32, b.neon_f32));
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f32) / sizeof(r.f32[0])); i++) {
		r.u32[i] = (a.f32[i] != b.f32[i]) ? 0xffffffff : 0;
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cmpneq_ss(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_cmpneq_ss(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	float32x4_t e =
		vreinterpretq_f32_u32(vceqq_f32(a.neon_f32, b.neon_f32));
	float32x4_t s =
		vreinterpretq_f32_u32(vmvnq_u32(vreinterpretq_u32_f32(e)));
	float32x4_t t = vextq_f32(a.neon_f32, s, 1);
	r.neon_f32 = vextq_f32(t, t, 3);
#elif defined(SIMDE__SHUFFLE_VECTOR) && defined(SIMDE_ASSUME_VECTORIZATION)
	r.f32 = SIMDE__SHUFFLE_VECTOR(32, 16, a.f32,
				      simde_mm_cmpneq_ps(a, b).f32, 4, 1, 2, 3);
#else
	r.u32[0] = (a.f32[0] != b.f32[0]) ? 0xffffffff : 0;
	SIMDE__VECTORIZE
	for (size_t i = 1; i < (sizeof(r.f32) / sizeof(r.f32[0])); i++) {
		r.u32[i] = a.u32[i];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cmpnge_ps(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_cmpnge_ps(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	r.neon_u32 = vcltq_f32(a.neon_f32, b.neon_f32);
#else
	r = simde_mm_cmplt_ps(a, b);
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cmpnge_ss(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE) && !defined(__PGI)
	r.n = _mm_cmpnge_ss(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	float32x4_t s =
		vreinterpretq_f32_u32(vcltq_f32(a.neon_f32, b.neon_f32));
	float32x4_t t = vextq_f32(a.neon_f32, s, 1);
	r.neon_f32 = vextq_f32(t, t, 3);
#else
	r = simde_mm_cmplt_ss(a, b);
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cmpngt_ps(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_cmpngt_ps(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	r.neon_u32 = vcleq_f32(a.neon_f32, b.neon_f32);
#else
	r = simde_mm_cmple_ps(a, b);
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cmpngt_ss(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE) && !defined(__PGI)
	r.n = _mm_cmpngt_ss(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	float32x4_t s =
		vreinterpretq_f32_u32(vcleq_f32(a.neon_f32, b.neon_f32));
	float32x4_t t = vextq_f32(a.neon_f32, s, 1);
	r.neon_f32 = vextq_f32(t, t, 3);
#else
	r = simde_mm_cmple_ss(a, b);
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cmpnle_ps(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_cmpnle_ps(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	r.neon_u32 = vcgtq_f32(a.neon_f32, b.neon_f32);
#else
	r = simde_mm_cmpgt_ps(a, b);
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cmpnle_ss(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_cmpnle_ss(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	float32x4_t s =
		vreinterpretq_f32_u32(vcgtq_f32(a.neon_f32, b.neon_f32));
	float32x4_t t = vextq_f32(a.neon_f32, s, 1);
	r.neon_f32 = vextq_f32(t, t, 3);
#else
	r = simde_mm_cmpgt_ss(a, b);
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cmpnlt_ps(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_cmpnlt_ps(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	r.neon_u32 = vcgeq_f32(a.neon_f32, b.neon_f32);
#else
	r = simde_mm_cmpge_ps(a, b);
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cmpnlt_ss(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_cmpnlt_ss(a.n, b.n);
#else
	r = simde_mm_cmpge_ss(a, b);
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cmpord_ps(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_cmpord_ps(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	/* Note: NEON does not have ordered compare builtin
     Need to compare a eq a and b eq b to check for NaN
     Do AND of results to get final */
	uint32x4_t ceqaa = vceqq_f32(a.neon_f32, a.neon_f32);
	uint32x4_t ceqbb = vceqq_f32(b.neon_f32, b.neon_f32);
	r.neon_u32 = vandq_u32(ceqaa, ceqbb);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f32) / sizeof(r.f32[0])); i++) {
		r.u32[i] = (isnan(a.f32[i]) || isnan(b.f32[i])) ? 0
								: 0xffffffff;
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cmpord_ss(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_cmpord_ss(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	uint32x4_t ceqaa = vceqq_f32(a.neon_f32, a.neon_f32);
	uint32x4_t ceqbb = vceqq_f32(b.neon_f32, b.neon_f32);
	float32x4_t s = vreinterpretq_f32_u32(vandq_u32(ceqaa, ceqbb));
	float32x4_t t = vextq_f32(a.neon_f32, s, 1);
	r.neon_f32 = vextq_f32(t, t, 3);
#elif defined(SIMDE__SHUFFLE_VECTOR) && defined(SIMDE_ASSUME_VECTORIZATION)
	r.f32 = SIMDE__SHUFFLE_VECTOR(32, 16, a.f32,
				      simde_mm_cmpord_ps(a, b).f32, 4, 1, 2, 3);
#else
	r.u32[0] = (isnan(a.f32[0]) || isnan(b.f32[0])) ? 0 : 0xffffffff;
	SIMDE__VECTORIZE
	for (size_t i = 1; i < (sizeof(r.f32) / sizeof(r.f32[0])); i++) {
		r.f32[i] = a.f32[i];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cmpunord_ps(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_cmpunord_ps(a.n, b.n);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f32) / sizeof(r.f32[0])); i++) {
		r.u32[i] = (isnan(a.f32[i]) || isnan(b.f32[i])) ? 0xffffffff
								: 0;
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cmpunord_ss(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE) && !defined(__PGI)
	r.n = _mm_cmpunord_ss(a.n, b.n);
#elif defined(SIMDE__SHUFFLE_VECTOR) && defined(SIMDE_ASSUME_VECTORIZATION)
	r.f32 = SIMDE__SHUFFLE_VECTOR(
		32, 16, a.f32, simde_mm_cmpunord_ps(a, b).f32, 4, 1, 2, 3);
#else
	r.u32[0] = (isnan(a.f32[0]) || isnan(b.f32[0])) ? 0xffffffff : 0;
	SIMDE__VECTORIZE
	for (size_t i = 1; i < (sizeof(r.f32) / sizeof(r.f32[0])); i++) {
		r.f32[i] = a.f32[i];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
int simde_mm_comieq_ss(simde__m128 a, simde__m128 b)
{
#if defined(SIMDE_SSE_NATIVE)
	return _mm_comieq_ss(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	uint32x4_t a_not_nan = vceqq_f32(a.neon_f32, a.neon_f32);
	uint32x4_t b_not_nan = vceqq_f32(b.neon_f32, b.neon_f32);
	uint32x4_t a_or_b_nan = vmvnq_u32(vandq_u32(a_not_nan, b_not_nan));
	uint32x4_t a_eq_b = vceqq_f32(a.neon_f32, b.neon_f32);
	return (vgetq_lane_u32(vorrq_u32(a_or_b_nan, a_eq_b), 0) != 0) ? 1 : 0;
#else
	return a.f32[0] == b.f32[0];
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
int simde_mm_comige_ss(simde__m128 a, simde__m128 b)
{
#if defined(SIMDE_SSE_NATIVE)
	return _mm_comige_ss(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	uint32x4_t a_not_nan = vceqq_f32(a.neon_f32, a.neon_f32);
	uint32x4_t b_not_nan = vceqq_f32(b.neon_f32, b.neon_f32);
	uint32x4_t a_and_b_not_nan = vandq_u32(a_not_nan, b_not_nan);
	uint32x4_t a_ge_b = vcgeq_f32(a.neon_f32, b.neon_f32);
	return (vgetq_lane_u32(vandq_u32(a_and_b_not_nan, a_ge_b), 0) != 0) ? 1
									    : 0;
#else
	return a.f32[0] >= b.f32[0];
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
int simde_mm_comigt_ss(simde__m128 a, simde__m128 b)
{
#if defined(SIMDE_SSE_NATIVE)
	return _mm_comigt_ss(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	uint32x4_t a_not_nan = vceqq_f32(a.neon_f32, a.neon_f32);
	uint32x4_t b_not_nan = vceqq_f32(b.neon_f32, b.neon_f32);
	uint32x4_t a_and_b_not_nan = vandq_u32(a_not_nan, b_not_nan);
	uint32x4_t a_gt_b = vcgtq_f32(a.neon_f32, b.neon_f32);
	return (vgetq_lane_u32(vandq_u32(a_and_b_not_nan, a_gt_b), 0) != 0) ? 1
									    : 0;
#else
	return a.f32[0] > b.f32[0];
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
int simde_mm_comile_ss(simde__m128 a, simde__m128 b)
{
#if defined(SIMDE_SSE_NATIVE)
	return _mm_comile_ss(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	uint32x4_t a_not_nan = vceqq_f32(a.neon_f32, a.neon_f32);
	uint32x4_t b_not_nan = vceqq_f32(b.neon_f32, b.neon_f32);
	uint32x4_t a_or_b_nan = vmvnq_u32(vandq_u32(a_not_nan, b_not_nan));
	uint32x4_t a_le_b = vcleq_f32(a.neon_f32, b.neon_f32);
	return (vgetq_lane_u32(vorrq_u32(a_or_b_nan, a_le_b), 0) != 0) ? 1 : 0;
#else
	return a.f32[0] <= b.f32[0];
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
int simde_mm_comilt_ss(simde__m128 a, simde__m128 b)
{
#if defined(SIMDE_SSE_NATIVE)
	return _mm_comilt_ss(a.n, b.n);
#elif defined(SIMDE_SSE_NATIVE)
	uint32x4_t a_not_nan = vceqq_f32(a.neon_f32, a.neon_f32);
	uint32x4_t b_not_nan = vceqq_f32(b.neon_f32, b.neon_f32);
	uint32x4_t a_or_b_nan = vmvnq_u32(vandq_u32(a_not_nan, b_not_nan));
	uint32x4_t a_lt_b = vcltq_f32(a.neon_f32, b.neon_f32);
	return (vgetq_lane_u32(vorrq_u32(a_or_b_nan, a_lt_b), 0) != 0) ? 1 : 0;
#else
	return a.f32[0] < b.f32[0];
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
int simde_mm_comineq_ss(simde__m128 a, simde__m128 b)
{
#if defined(SIMDE_SSE_NATIVE)
	return _mm_comineq_ss(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	uint32x4_t a_not_nan = vceqq_f32(a.neon_f32, a.neon_f32);
	uint32x4_t b_not_nan = vceqq_f32(b.neon_f32, b.neon_f32);
	uint32x4_t a_and_b_not_nan = vandq_u32(a_not_nan, b_not_nan);
	uint32x4_t a_neq_b = vmvnq_u32(vceqq_f32(a.neon_f32, b.neon_f32));
	return (vgetq_lane_u32(vandq_u32(a_and_b_not_nan, a_neq_b), 0) != 0)
		       ? 1
		       : 0;
#else
	return a.f32[0] != b.f32[0];
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cvt_pi2ps(simde__m128 a, simde__m64 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_cvt_pi2ps(a.n, b.n);
#else
	r.f32[0] = (simde_float32)b.i32[0];
	r.f32[1] = (simde_float32)b.i32[1];
	r.i32[2] = a.i32[2];
	r.i32[3] = a.i32[3];
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_cvt_ps2pi(simde__m128 a)
{
	simde__m64 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_cvt_ps2pi(a.n);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i32) / sizeof(r.i32[0])); i++) {
		r.i32[i] = (int32_t)a.f32[i];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cvt_si2ss(simde__m128 a, int32_t b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_cvt_si2ss(a.n, b);
#else
	r.f32[0] = (simde_float32)b;
	r.i32[1] = a.i32[1];
	r.i32[2] = a.i32[2];
	r.i32[3] = a.i32[3];
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
int32_t simde_mm_cvt_ss2si(simde__m128 a)
{
#if defined(SIMDE_SSE_NATIVE)
	return _mm_cvt_ss2si(a.n);
#else
	return (int32_t)a.f32[0];
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cvtpi16_ps(simde__m64 a)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_cvtpi16_ps(a.n);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f32) / sizeof(r.f32[0])); i++) {
		r.f32[i] = (simde_float32)a.i16[i];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cvtpi32_ps(simde__m128 a, simde__m64 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_cvtpi32_ps(a.n, b.n);
#else
	r.f32[0] = (simde_float32)b.i32[0];
	r.f32[1] = (simde_float32)b.i32[1];
	r.i32[2] = a.i32[2];
	r.i32[3] = a.i32[3];
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cvtpi32x2_ps(simde__m64 a, simde__m64 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_cvtpi32x2_ps(a.n, b.n);
#else
	r.f32[0] = (simde_float32)a.i32[0];
	r.f32[1] = (simde_float32)a.i32[1];
	r.f32[2] = (simde_float32)b.i32[0];
	r.f32[3] = (simde_float32)b.i32[1];
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cvtpi8_ps(simde__m64 a)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_cvtpi8_ps(a.n);
#else
	r.f32[0] = (simde_float32)a.i8[0];
	r.f32[1] = (simde_float32)a.i8[1];
	r.f32[2] = (simde_float32)a.i8[2];
	r.f32[3] = (simde_float32)a.i8[3];
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_cvtps_pi16(simde__m128 a)
{
	simde__m64 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_cvtps_pi16(a.n);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i16) / sizeof(r.i16[0])); i++) {
		r.i16[i] = (int16_t)a.f32[i];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_cvtps_pi32(simde__m128 a)
{
	simde__m64 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_cvtps_pi32(a.n);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i32) / sizeof(r.i32[0])); i++) {
		r.i32[i] = (int32_t)a.f32[i];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_cvtps_pi8(simde__m128 a)
{
	simde__m64 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_cvtps_pi8(a.n);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(a.f32) / sizeof(a.f32[0])); i++) {
		r.i8[i] = (int8_t)a.f32[i];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cvtpu16_ps(simde__m64 a)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_cvtpu16_ps(a.n);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f32) / sizeof(r.f32[0])); i++) {
		r.f32[i] = (simde_float32)a.u16[i];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cvtpu8_ps(simde__m64 a)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_cvtpu8_ps(a.n);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < 4; i++) {
		r.f32[i] = (simde_float32)a.u8[i];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cvtsi32_ss(simde__m128 a, int32_t b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_cvtsi32_ss(a.n, b);
#else
	r.f32[0] = (simde_float32)b;
	SIMDE__VECTORIZE
	for (size_t i = 1; i < 4; i++) {
		r.i32[i] = a.i32[i];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cvtsi64_ss(simde__m128 a, int64_t b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE) && defined(SIMDE_ARCH_AMD64)
#if !defined(__PGI)
	r.n = _mm_cvtsi64_ss(a.n, b);
#else
	r.n = _mm_cvtsi64x_ss(a.n, b);
#endif
#else
	r.f32[0] = (simde_float32)b;
	SIMDE__VECTORIZE
	for (size_t i = 1; i < 4; i++) {
		r.i32[i] = a.i32[i];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde_float32 simde_mm_cvtss_f32(simde__m128 a)
{
#if defined(SIMDE_SSE_NATIVE)
	return _mm_cvtss_f32(a.n);
#elif defined(SIMDE_SSE_NEON)
	return vgetq_lane_f32(a.neon_f32, 0);
#else
	return a.f32[0];
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
int32_t simde_mm_cvtss_si32(simde__m128 a)
{
#if defined(SIMDE_SSE_NATIVE)
	return _mm_cvtss_si32(a.n);
#else
	return (int32_t)a.f32[0];
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
int64_t simde_mm_cvtss_si64(simde__m128 a)
{
#if defined(SIMDE_SSE_NATIVE) && defined(SIMDE_ARCH_AMD64)
#if !defined(__PGI)
	return _mm_cvtss_si64(a.n);
#else
	return _mm_cvtss_si64x(a.n);
#endif
#else
	return (int64_t)a.f32[0];
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_cvtt_ps2pi(simde__m128 a)
{
	simde__m64 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_cvtt_ps2pi(a.n);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f32) / sizeof(r.f32[0])); i++) {
		r.i32[i] = (int32_t)truncf(a.f32[i]);
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
int32_t simde_mm_cvtt_ss2si(simde__m128 a)
{
#if defined(SIMDE_SSE_NATIVE)
	return _mm_cvtt_ss2si(a.n);
#else
	return (int32_t)truncf(a.f32[0]);
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_cvttps_pi32(simde__m128 a)
{
	simde__m64 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_cvttps_pi32(a.n);
#else
	r = simde_mm_cvtt_ps2pi(a);
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
int32_t simde_mm_cvttss_si32(simde__m128 a)
{
#if defined(SIMDE_SSE_NATIVE)
	return _mm_cvttss_si32(a.n);
#else
	return (int32_t)truncf(a.f32[0]);
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
int64_t simde_mm_cvttss_si64(simde__m128 a)
{
#if defined(SIMDE_SSE_NATIVE) && defined(SIMDE_ARCH_AMD64)
#if defined(__PGI)
	return _mm_cvttss_si64x(a.n);
#else
	return _mm_cvttss_si64(a.n);
#endif
#else
	return (int64_t)truncf(a.f32[0]);
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_div_ps(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_div_ps(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	float32x4_t recip0 = vrecpeq_f32(b.neon_f32);
	float32x4_t recip1 = vmulq_f32(recip0, vrecpsq_f32(recip0, b.neon_f32));
	r.neon_f32 = vmulq_f32(a.neon_f32, recip1);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f32) / sizeof(r.f32[0])); i++) {
		r.f32[i] = a.f32[i] / b.f32[i];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_div_ss(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_div_ss(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	float32_t value = vgetq_lane_f32(simde_mm_div_ps(a, b).neon_f32, 0);
	r.neon_f32 = vsetq_lane_f32(value, a.neon_f32, 0);
#else
	r.f32[0] = a.f32[0] / b.f32[0];
	SIMDE__VECTORIZE
	for (size_t i = 1; i < (sizeof(r.f32) / sizeof(r.f32[0])); i++) {
		r.f32[i] = a.f32[i];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
int32_t simde_mm_extract_pi16(simde__m64 a, const int imm8)
{
	return a.u16[imm8];
}
#if defined(SIMDE_SSE_NATIVE)
#define simde_mm_extract_pi16(a, imm8) _mm_extract_pi16(a.n, imm8)
#endif
#define simde_m_pextrw(a, imm8) simde_mm_extract_pi16(a.n, imm8)

enum {
#if defined(SIMDE_SSE_NATIVE)
	simde_MM_ROUND_NEAREST = _MM_ROUND_NEAREST,
	simde_MM_ROUND_DOWN = _MM_ROUND_DOWN,
	simde_MM_ROUND_UP = _MM_ROUND_UP,
	simde_MM_ROUND_TOWARD_ZERO = _MM_ROUND_TOWARD_ZERO
#else
	simde_MM_ROUND_NEAREST
#if defined(FE_TONEAREST)
	= FE_TONEAREST
#endif
	,

	simde_MM_ROUND_DOWN
#if defined(FE_DOWNWARD)
	= FE_DOWNWARD
#endif
	,

	simde_MM_ROUND_UP
#if defined(FE_UPWARD)
	= FE_UPWARD
#endif
	,

	simde_MM_ROUND_TOWARD_ZERO
#if defined(FE_TOWARDZERO)
	= FE_TOWARDZERO
#endif
#endif
};

SIMDE__FUNCTION_ATTRIBUTES
unsigned int simde_MM_GET_ROUNDING_MODE(void)
{
#if defined(SIMDE_SSE_NATIVE)
	return _MM_GET_ROUNDING_MODE();
#else
	return fegetround();
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
void simde_MM_SET_ROUNDING_MODE(unsigned int a)
{
#if defined(SIMDE_SSE_NATIVE)
	_MM_SET_ROUNDING_MODE(a);
#else
	fesetround((int)a);
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_insert_pi16(simde__m64 a, int16_t i, const int imm8)
{
	simde__m64 r;
	r.i64[0] = a.i64[0];
	r.i16[imm8] = i;
	return r;
}
#if defined(SIMDE_SSE_NATIVE) && !defined(__PGI)
#define simde_mm_insert_pi16(a, i, imm8) \
	SIMDE__M64_C(_mm_insert_pi16((a).n, i, imm8));
#endif
#define simde_m_pinsrw(a, i, imm8) \
	SIMDE__M64_C(simde_mm_insert_pi16((a).n, i, imm8));

SIMDE__FUNCTION_ATTRIBUTES
simde__m128
simde_mm_load_ps(simde_float32 const mem_addr[HEDLEY_ARRAY_PARAM(4)])
{
	simde__m128 r;

	simde_assert_aligned(16, mem_addr);

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_load_ps(mem_addr);
#elif defined(SIMDE_SSE_NEON)
	r.neon_f32 = vld1q_f32(mem_addr);
#else
	memcpy(&r, mem_addr, sizeof(r.f32));
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_load_ps1(simde_float32 const *mem_addr)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_load_ps1(mem_addr);
#else
	const simde_float32 v = *mem_addr;
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i32) / sizeof(r.i32[0])); i++) {
		r.f32[i] = v;
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_load_ss(simde_float32 const *mem_addr)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_load_ss(mem_addr);
#elif defined(SIMDE_SSE_NEON)
	r.neon_f32 = vsetq_lane_f32(*mem_addr, vdupq_n_f32(0), 0);
#else
	r.f32[0] = *mem_addr;
	r.i32[1] = 0;
	r.i32[2] = 0;
	r.i32[3] = 0;
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_load1_ps(simde_float32 const *mem_addr)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_load1_ps(mem_addr);
#elif defined(SIMDE_SSE_NEON)
	r.neon_f32 = vld1q_dup_f32(mem_addr);
#else
	r = simde_mm_load_ps1(mem_addr);
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_loadh_pi(simde__m128 a, simde__m64 const *mem_addr)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_loadh_pi(a.n, (__m64 *)mem_addr);
#else
	r.f32[0] = a.f32[0];
	r.f32[1] = a.f32[1];
	r.f32[2] = mem_addr->f32[0];
	r.f32[3] = mem_addr->f32[1];
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_loadl_pi(simde__m128 a, simde__m64 const *mem_addr)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_loadl_pi(a.n, (__m64 *)mem_addr);
#else
	r.f32[0] = mem_addr->f32[0];
	r.f32[1] = mem_addr->f32[1];
	r.f32[2] = a.f32[2];
	r.f32[3] = a.f32[3];
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128
simde_mm_loadr_ps(simde_float32 const mem_addr[HEDLEY_ARRAY_PARAM(4)])
{
	simde__m128 r;

	simde_assert_aligned(16, mem_addr);

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_loadr_ps(mem_addr);
#else
	r.f32[0] = mem_addr[3];
	r.f32[1] = mem_addr[2];
	r.f32[2] = mem_addr[1];
	r.f32[3] = mem_addr[0];
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128
simde_mm_loadu_ps(simde_float32 const mem_addr[HEDLEY_ARRAY_PARAM(4)])
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_loadu_ps(mem_addr);
#elif defined(SIMDE_SSE_NEON)
	r.neon_f32 = vld1q_f32(mem_addr);
#else
	r.f32[0] = mem_addr[0];
	r.f32[1] = mem_addr[1];
	r.f32[2] = mem_addr[2];
	r.f32[3] = mem_addr[3];
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
void simde_mm_maskmove_si64(simde__m64 a, simde__m64 mask, char *mem_addr)
{
#if defined(SIMDE_SSE_NATIVE)
	_mm_maskmove_si64(a.n, mask.n, mem_addr);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(a.i8) / sizeof(a.i8[0])); i++)
		if (mask.i8[i] < 0)
			mem_addr[i] = a.i8[i];
#endif
}
#define simde_m_maskmovq(a, mask, mem_addr) \
	simde_mm_maskmove_si64(a, mask, mem_addr)

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_max_pi16(simde__m64 a, simde__m64 b)
{
	simde__m64 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_max_pi16(a.n, b.n);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i16) / sizeof(r.i16[0])); i++) {
		r.i16[i] = (a.i16[i] > b.i16[i]) ? a.i16[i] : b.i16[i];
	}
#endif

	return r;
}
#define simde_m_pmaxsw(a, b) simde_mm_max_pi16(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_max_ps(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_max_ps(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	r.neon_f32 = vmaxq_f32(a.neon_f32, b.neon_f32);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f32) / sizeof(r.f32[0])); i++) {
		r.f32[i] = (a.f32[i] > b.f32[i]) ? a.f32[i] : b.f32[i];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_max_pu8(simde__m64 a, simde__m64 b)
{
	simde__m64 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_max_pu8(a.n, b.n);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.u8) / sizeof(r.u8[0])); i++) {
		r.u8[i] = (a.u8[i] > b.u8[i]) ? a.u8[i] : b.u8[i];
	}
#endif

	return r;
}
#define simde_m_pmaxub(a, b) simde_mm_max_pu8(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_max_ss(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_max_ss(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	float32_t value = vgetq_lane_f32(vmaxq_f32(a.neon_f32, b.neon_f32), 0);
	r.neon_f32 = vsetq_lane_f32(value, a.neon_f32, 0);
#else
	r.f32[0] = (a.f32[0] > b.f32[0]) ? a.f32[0] : b.f32[0];
	r.f32[1] = a.f32[1];
	r.f32[2] = a.f32[2];
	r.f32[3] = a.f32[3];
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_min_pi16(simde__m64 a, simde__m64 b)
{
	simde__m64 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_min_pi16(a.n, b.n);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.i16) / sizeof(r.i16[0])); i++) {
		r.i16[i] = (a.i16[i] < b.i16[i]) ? a.i16[i] : b.i16[i];
	}
#endif

	return r;
}
#define simde_m_pminsw(a, b) simde_mm_min_pi16(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_min_ps(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_min_ps(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	r.neon_f32 = vminq_f32(a.neon_f32, b.neon_f32);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f32) / sizeof(r.f32[0])); i++) {
		r.f32[i] = (a.f32[i] < b.f32[i]) ? a.f32[i] : b.f32[i];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_min_pu8(simde__m64 a, simde__m64 b)
{
	simde__m64 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_min_pu8(a.n, b.n);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.u8) / sizeof(r.u8[0])); i++) {
		r.u8[i] = (a.u8[i] < b.u8[i]) ? a.u8[i] : b.u8[i];
	}
#endif

	return r;
}
#define simde_m_pminub(a, b) simde_mm_min_pu8(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_min_ss(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_min_ss(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	float32_t value = vgetq_lane_f32(vminq_f32(a.neon_f32, b.neon_f32), 0);
	r.neon_f32 = vsetq_lane_f32(value, a.neon_f32, 0);
#else
	r.f32[0] = (a.f32[0] < b.f32[0]) ? a.f32[0] : b.f32[0];
	r.f32[1] = a.f32[1];
	r.f32[2] = a.f32[2];
	r.f32[3] = a.f32[3];
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_move_ss(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_move_ss(a.n, b.n);
#else
	r.f32[0] = b.f32[0];
	r.f32[1] = a.f32[1];
	r.f32[2] = a.f32[2];
	r.f32[3] = a.f32[3];
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_movehl_ps(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_movehl_ps(a.n, b.n);
#else
	r.f32[0] = b.f32[2];
	r.f32[1] = b.f32[3];
	r.f32[2] = a.f32[2];
	r.f32[3] = a.f32[3];
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_movelh_ps(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_movelh_ps(a.n, b.n);
#else
	r.f32[0] = a.f32[0];
	r.f32[1] = a.f32[1];
	r.f32[2] = b.f32[0];
	r.f32[3] = b.f32[1];
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
int simde_mm_movemask_pi8(simde__m64 a)
{
#if defined(SIMDE_SSE_NATIVE)
	return _mm_movemask_pi8(a.n);
#else
	int r = 0;
	const size_t nmemb = sizeof(a.i8) / sizeof(a.i8[0]);

	SIMDE__VECTORIZE_REDUCTION(| : r)
	for (size_t i = 0; i < nmemb; i++) {
		r |= (a.u8[nmemb - 1 - i] >> 7) << (nmemb - 1 - i);
	}

	return r;
#endif
}
#define simde_m_pmovmskb(a, b) simde_mm_movemask_pi8(a, b)

SIMDE__FUNCTION_ATTRIBUTES
int simde_mm_movemask_ps(simde__m128 a)
{
#if defined(SIMDE_SSE_NATIVE)
	return _mm_movemask_ps(a.n);
#elif defined(SIMDE_SSE_NEON)
	/* TODO: check to see if NEON version is faster than the portable version */
	static const uint32x4_t movemask = {1, 2, 4, 8};
	static const uint32x4_t highbit = {0x80000000, 0x80000000, 0x80000000,
					   0x80000000};
	uint32x4_t t0 = a.neon_u32;
	uint32x4_t t1 = vtstq_u32(t0, highbit);
	uint32x4_t t2 = vandq_u32(t1, movemask);
	uint32x2_t t3 = vorr_u32(vget_low_u32(t2), vget_high_u32(t2));
	return vget_lane_u32(t3, 0) | vget_lane_u32(t3, 1);
#else
	int r = 0;

	SIMDE__VECTORIZE_REDUCTION(| : r)
	for (size_t i = 0; i < sizeof(a.u32) / sizeof(a.u32[0]); i++) {
		r |= (a.u32[i] >> ((sizeof(a.u32[i]) * CHAR_BIT) - 1)) << i;
	}

	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_mul_ps(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_mul_ps(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	r.neon_f32 = vmulq_f32(a.neon_f32, b.neon_f32);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f32) / sizeof(r.f32[0])); i++) {
		r.f32[i] = a.f32[i] * b.f32[i];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_mul_ss(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_mul_ss(a.n, b.n);
#else
	r.f32[0] = a.f32[0] * b.f32[0];
	r.f32[1] = a.f32[1];
	r.f32[2] = a.f32[2];
	r.f32[3] = a.f32[3];
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_mulhi_pu16(simde__m64 a, simde__m64 b)
{
	simde__m64 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_mulhi_pu16(a.n, b.n);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.u16) / sizeof(r.u16[0])); i++) {
		r.u16[i] = (a.u16[i] * b.u16[i]) >> 16;
	}
#endif

	return r;
}
#define simde_m_pmulhuw(a, b) simde_mm_mulhi_pu16(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_or_ps(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_or_ps(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	r.neon_i32 = vorrq_s32(a.neon_i32, b.neon_i32);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.u32) / sizeof(r.u32[0])); i++) {
		r.u32[i] = a.u32[i] | b.u32[i];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
void simde_mm_prefetch(char const *p, int i)
{
	(void)p;
	(void)i;
}
#if defined(SIMDE_SSE_NATIVE)
#define simde_mm_prefetch(p, i) _mm_prefetch(p, i)
#endif

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_rcp_ps(simde__m128 a)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_rcp_ps(a.n);
#elif defined(SIMDE_SSE_NEON)
	float32x4_t recip = vrecpeq_f32(a.neon_f32);

#if !defined(SIMDE_MM_RCP_PS_ITERS)
#define SIMDE_MM_RCP_PS_ITERS SIMDE_ACCURACY_ITERS
#endif

	for (int i = 0; i < SIMDE_MM_RCP_PS_ITERS; ++i) {
		recip = vmulq_f32(recip, vrecpsq_f32(recip, a.neon_f32));
	}

	r.neon_f32 = recip;
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f32) / sizeof(r.f32[0])); i++) {
		r.f32[i] = 1.0f / a.f32[i];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_rcp_ss(simde__m128 a)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_rcp_ss(a.n);
#else
	r.f32[0] = 1.0f / a.f32[0];
	r.f32[1] = a.f32[1];
	r.f32[2] = a.f32[2];
	r.f32[3] = a.f32[3];
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_rsqrt_ps(simde__m128 a)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_rsqrt_ps(a.n);
#elif defined(SIMDE_SSE_NEON)
	r.neon_f32 = vrsqrteq_f32(a.neon_f32);
#elif defined(__STDC_IEC_559__)
	/* http://h14s.p5r.org/2012/09/0x5f3759df.html?mwh=1 */
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f32) / sizeof(r.f32[0])); i++) {
		r.i32[i] = INT32_C(0x5f3759df) - (a.i32[i] >> 1);

#if SIMDE_ACCURACY_ITERS > 2
		const float half = SIMDE_FLOAT32_C(0.5) * a.f32[i];
		for (int ai = 2; ai < SIMDE_ACCURACY_ITERS; ai++)
			r.f32[i] *= SIMDE_FLOAT32_C(1.5) -
				    (half * r.f32[i] * r.f32[i]);
#endif
	}
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f32) / sizeof(r.f32[0])); i++) {
		r.f32[i] = 1.0f / sqrtf(a.f32[i]);
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_rsqrt_ss(simde__m128 a)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_rsqrt_ss(a.n);
#elif defined(__STDC_IEC_559__)
	{
		r.i32[0] = INT32_C(0x5f3759df) - (a.i32[0] >> 1);

#if SIMDE_ACCURACY_ITERS > 2
		float half = SIMDE_FLOAT32_C(0.5) * a.f32[0];
		for (int ai = 2; ai < SIMDE_ACCURACY_ITERS; ai++)
			r.f32[0] *= SIMDE_FLOAT32_C(1.5) -
				    (half * r.f32[0] * r.f32[0]);
#endif
	}
	r.f32[0] = 1.0f / sqrtf(a.f32[0]);
	r.f32[1] = a.f32[1];
	r.f32[2] = a.f32[2];
	r.f32[3] = a.f32[3];
#else
	r.f32[0] = 1.0f / sqrtf(a.f32[0]);
	r.f32[1] = a.f32[1];
	r.f32[2] = a.f32[2];
	r.f32[3] = a.f32[3];
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_sad_pu8(simde__m64 a, simde__m64 b)
{
	simde__m64 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_sad_pu8(a.n, b.n);
#else
	uint16_t sum = 0;

	SIMDE__VECTORIZE_REDUCTION(+ : sum)
	for (size_t i = 0; i < (sizeof(r.u8) / sizeof(r.u8[0])); i++) {
		sum += (uint8_t)abs(a.u8[i] - b.u8[i]);
	}

	r.i16[0] = sum;
	r.i16[1] = 0;
	r.i16[2] = 0;
	r.i16[3] = 0;
#endif

	return r;
}
#define simde_m_psadbw(a, b) simde_mm_sad_pu8(a, b)

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_set_ps(simde_float32 e3, simde_float32 e2,
			    simde_float32 e1, simde_float32 e0)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_set_ps(e3, e2, e1, e0);
#elif defined(SIMDE_SSE_NEON)
	SIMDE_ALIGN(16) simde_float32 data[4] = {e0, e1, e2, e3};
	r.neon_f32 = vld1q_f32(data);
#else
	r.f32[0] = e0;
	r.f32[1] = e1;
	r.f32[2] = e2;
	r.f32[3] = e3;
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_set_ps1(simde_float32 a)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_set1_ps(a);
#elif defined(SIMDE_SSE_NEON)
	r.neon_f32 = vdupq_n_f32(a);
#else
	r = simde_mm_set_ps(a, a, a, a);
#endif

	return r;
}
#define simde_mm_set1_ps(a) simde_mm_set_ps1(a)

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_set_ss(simde_float32 a)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_set_ss(a);
#else
	r = simde_mm_set_ps(0, 0, 0, a);
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_setr_ps(simde_float32 e3, simde_float32 e2,
			     simde_float32 e1, simde_float32 e0)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_setr_ps(e3, e2, e1, e0);
#elif defined(SIMDE_SSE_NEON)
	SIMDE_ALIGN(16) simde_float32 data[4] = {e3, e2, e1, e0};
	r.neon_f32 = vld1q_f32(data);
#else
	r = simde_mm_set_ps(e0, e1, e2, e3);
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_setzero_ps(void)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_setzero_ps();
#elif defined(SIMDE_SSE_NEON)
	r.neon_f32 = vdupq_n_f32(0.0f);
#else
	r = simde_mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f);
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
void simde_mm_sfence(void)
{
	/* TODO: Use Hedley. */
#if defined(SIMDE_SSE_NATIVE)
	_mm_sfence();
#elif defined(__GNUC__) && \
	((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7))
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
#elif !defined(__INTEL_COMPILER) && defined(__STDC_VERSION__) && \
	(__STDC_VERSION__ >= 201112L) && !defined(__STDC_NO_ATOMICS__)
#if defined(__GNUC__) && (__GNUC__ == 4) && (__GNUC_MINOR__ < 9)
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
#else
	atomic_thread_fence(memory_order_seq_cst);
#endif
#elif defined(_MSC_VER)
	MemoryBarrier();
#elif defined(__GNUC__) && \
	((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7))
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
#elif HEDLEY_CLANG_HAS_FEATURE(c_atomic)
	__c11_atomic_thread_fence(__ATOMIC_SEQ_CST)
#elif defined(__GNUC__) && \
	((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 1))
	__sync_synchronize();
#elif (defined(__SUNPRO_C) && (__SUNPRO_C >= 0x5140)) || \
	(defined(__SUNPRO_CC) && (__SUNPRO_CC >= 0x5140))
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
#elif defined(_OPENMP)
#pragma omp critical(simde_mm_sfence_)
	{
	}
#endif
}

#define SIMDE_MM_SHUFFLE(z, y, x, w) \
	(((z) << 6) | ((y) << 4) | ((x) << 2) | (w))

SIMDE__FUNCTION_ATTRIBUTES
simde__m64 simde_mm_shuffle_pi16(simde__m64 a, const int imm8)
{
	simde__m64 r;
	for (size_t i = 0; i < sizeof(r.u16) / sizeof(r.u16[0]); i++) {
		r.i16[i] = a.i16[(imm8 >> (i * 2)) & 3];
	}
	return r;
}
#if defined(SIMDE_SSE_NATIVE) && !defined(__PGI)
#define simde_mm_shuffle_pi16(a, imm8) SIMDE__M64_C(_mm_shuffle_pi16(a.n, imm8))
#elif defined(SIMDE__SHUFFLE_VECTOR)
#define simde_mm_shuffle_pi16(a, imm8)                                         \
	({                                                                     \
		const simde__m64 simde__tmp_a_ = a;                            \
		(simde__m64){.i16 = SIMDE__SHUFFLE_VECTOR(                     \
				     16, 8, (simde__tmp_a_).i16,               \
				     (simde__tmp_a_).i16, (((imm8)) & 3),      \
				     (((imm8) >> 2) & 3), (((imm8) >> 4) & 3), \
				     (((imm8) >> 6) & 3))};                    \
	})
#endif

#if defined(SIMDE_SSE_NATIVE) && !defined(__PGI)
#define simde_m_pshufw(a, imm8) SIMDE__M64_C(_m_pshufw(a.n, imm8))
#else
#define simde_m_pshufw(a, imm8) simde_mm_shuffle_pi16(a, imm8)
#endif

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_shuffle_ps(simde__m128 a, simde__m128 b, const int imm8)
{
	simde__m128 r;
	r.f32[0] = a.f32[(imm8 >> 0) & 3];
	r.f32[1] = a.f32[(imm8 >> 2) & 3];
	r.f32[2] = b.f32[(imm8 >> 4) & 3];
	r.f32[3] = b.f32[(imm8 >> 6) & 3];
	return r;
}
#if defined(SIMDE_SSE_NATIVE) && !defined(__PGI)
#define simde_mm_shuffle_ps(a, b, imm8) \
	SIMDE__M128_C(_mm_shuffle_ps(a.n, b.n, imm8))
#elif defined(SIMDE__SHUFFLE_VECTOR)
#define simde_mm_shuffle_ps(a, b, imm8)                                    \
	({                                                                 \
		(simde__m128){.f32 = SIMDE__SHUFFLE_VECTOR(                \
				      32, 16, (a).f32, (b).f32,            \
				      (((imm8)) & 3), (((imm8) >> 2) & 3), \
				      (((imm8) >> 4) & 3) + 4,             \
				      (((imm8) >> 6) & 3) + 4)};           \
	})
#endif

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_sqrt_ps(simde__m128 a)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_sqrt_ps(a.n);
#elif defined(SIMDE_SSE_NEON)
	float32x4_t recipsq = vrsqrteq_f32(a.neon_f32);
	float32x4_t sq = vrecpeq_f32(recipsq);
	/* ??? use step versions of both sqrt and recip for better accuracy? */
	r.neon_f32 = sq;
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < sizeof(r.f32) / sizeof(r.f32[0]); i++) {
		r.f32[i] = sqrtf(a.f32[i]);
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_sqrt_ss(simde__m128 a)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_sqrt_ss(a.n);
#elif defined(SIMDE_SSE_NEON)
	float32_t value = vgetq_lane_f32(simde_mm_sqrt_ps(a).neon_f32, 0);
	r.neon_f32 = vsetq_lane_f32(value, a.neon_f32, 0);
#else
	r.f32[0] = sqrtf(a.f32[0]);
	r.f32[1] = a.f32[1];
	r.f32[2] = a.f32[2];
	r.f32[3] = a.f32[3];
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
void simde_mm_store_ps(simde_float32 mem_addr[4], simde__m128 a)
{
	simde_assert_aligned(16, mem_addr);

#if defined(SIMDE_SSE_NATIVE)
	_mm_store_ps(mem_addr, a.n);
#elif defined(SIMDE_SSE_NEON)
	vst1q_f32(mem_addr, a.neon_f32);
#else
	SIMDE__VECTORIZE_ALIGNED(mem_addr : 16)
	for (size_t i = 0; i < sizeof(a.f32) / sizeof(a.f32[0]); i++) {
		mem_addr[i] = a.f32[i];
	}
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
void simde_mm_store_ps1(simde_float32 mem_addr[4], simde__m128 a)
{
	simde_assert_aligned(16, mem_addr);

#if defined(SIMDE_SSE_NATIVE)
	_mm_store_ps1(mem_addr, a.n);
#else
	SIMDE__VECTORIZE_ALIGNED(mem_addr : 16)
	for (size_t i = 0; i < sizeof(a.f32) / sizeof(a.f32[0]); i++) {
		mem_addr[i] = a.f32[0];
	}
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
void simde_mm_store_ss(simde_float32 *mem_addr, simde__m128 a)
{
#if defined(SIMDE_SSE_NATIVE)
	_mm_store_ss(mem_addr, a.n);
#elif defined(SIMDE_SSE_NEON)
	vst1q_lane_f32(mem_addr, a.neon_f32, 0);
#else
	*mem_addr = a.f32[0];
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
void simde_mm_store1_ps(simde_float32 mem_addr[4], simde__m128 a)
{
	simde_assert_aligned(16, mem_addr);

#if defined(SIMDE_SSE_NATIVE)
	_mm_store1_ps(mem_addr, a.n);
#else
	simde_mm_store_ps1(mem_addr, a);
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
void simde_mm_storeh_pi(simde__m64 *mem_addr, simde__m128 a)
{
#if defined(SIMDE_SSE_NATIVE)
	_mm_storeh_pi(&(mem_addr->n), a.n);
#else
	mem_addr->f32[0] = a.f32[2];
	mem_addr->f32[1] = a.f32[3];
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
void simde_mm_storel_pi(simde__m64 *mem_addr, simde__m128 a)
{
#if defined(SIMDE_SSE_NATIVE)
	_mm_storel_pi(&(mem_addr->n), a.n);
#else
	mem_addr->f32[0] = a.f32[0];
	mem_addr->f32[1] = a.f32[1];
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
void simde_mm_storer_ps(simde_float32 mem_addr[4], simde__m128 a)
{
	simde_assert_aligned(16, mem_addr);

#if defined(SIMDE_SSE_NATIVE)
	_mm_storer_ps(mem_addr, a.n);
#else
	SIMDE__VECTORIZE_ALIGNED(mem_addr : 16)
	for (size_t i = 0; i < sizeof(a.f32) / sizeof(a.f32[0]); i++) {
		mem_addr[i] =
			a.f32[((sizeof(a.f32) / sizeof(a.f32[0])) - 1) - i];
	}
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
void simde_mm_storeu_ps(simde_float32 mem_addr[4], simde__m128 a)
{
#if defined(SIMDE_SSE_NATIVE)
	_mm_storeu_ps(mem_addr, a.n);
#elif defined(SIMDE_SSE_NEON)
	vst1q_f32(mem_addr, a.neon_f32);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < sizeof(a.f32) / sizeof(a.f32[0]); i++) {
		mem_addr[i] = a.f32[i];
	}
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_sub_ps(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_sub_ps(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	r.neon_f32 = vsubq_f32(a.neon_f32, b.neon_f32);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.f32) / sizeof(r.f32[0])); i++) {
		r.f32[i] = a.f32[i] - b.f32[i];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_sub_ss(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_sub_ss(a.n, b.n);
#else
	r.f32[0] = a.f32[0] - b.f32[0];
	r.f32[1] = a.f32[1];
	r.f32[2] = a.f32[2];
	r.f32[3] = a.f32[3];
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
int simde_mm_ucomieq_ss(simde__m128 a, simde__m128 b)
{
#if defined(SIMDE_SSE_NATIVE)
	return _mm_ucomieq_ss(a.n, b.n);
#else
	fenv_t envp;
	int x = feholdexcept(&envp);
	int r = a.f32[0] == b.f32[0];
	if (HEDLEY_LIKELY(x == 0))
		fesetenv(&envp);
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
int simde_mm_ucomige_ss(simde__m128 a, simde__m128 b)
{
#if defined(SIMDE_SSE_NATIVE)
	return _mm_ucomige_ss(a.n, b.n);
#else
	fenv_t envp;
	int x = feholdexcept(&envp);
	int r = a.f32[0] >= b.f32[0];
	if (HEDLEY_LIKELY(x == 0))
		fesetenv(&envp);
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
int simde_mm_ucomigt_ss(simde__m128 a, simde__m128 b)
{
#if defined(SIMDE_SSE_NATIVE)
	return _mm_ucomigt_ss(a.n, b.n);
#else
	fenv_t envp;
	int x = feholdexcept(&envp);
	int r = a.f32[0] > b.f32[0];
	if (HEDLEY_LIKELY(x == 0))
		fesetenv(&envp);
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
int simde_mm_ucomile_ss(simde__m128 a, simde__m128 b)
{
#if defined(SIMDE_SSE_NATIVE)
	return _mm_ucomile_ss(a.n, b.n);
#else
	fenv_t envp;
	int x = feholdexcept(&envp);
	int r = a.f32[0] <= b.f32[0];
	if (HEDLEY_LIKELY(x == 0))
		fesetenv(&envp);
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
int simde_mm_ucomilt_ss(simde__m128 a, simde__m128 b)
{
#if defined(SIMDE_SSE_NATIVE)
	return _mm_ucomilt_ss(a.n, b.n);
#else
	fenv_t envp;
	int x = feholdexcept(&envp);
	int r = a.f32[0] < b.f32[0];
	if (HEDLEY_LIKELY(x == 0))
		fesetenv(&envp);
	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
int simde_mm_ucomineq_ss(simde__m128 a, simde__m128 b)
{
#if defined(SIMDE_SSE_NATIVE)
	return _mm_ucomineq_ss(a.n, b.n);
#else
	fenv_t envp;
	int x = feholdexcept(&envp);
	int r = a.f32[0] != b.f32[0];
	if (HEDLEY_LIKELY(x == 0))
		fesetenv(&envp);
	return r;
#endif
}

#if defined(SIMDE_SSE_NATIVE)
#if defined(__has_builtin)
#if __has_builtin(__builtin_ia32_undef128)
#define SIMDE__HAVE_UNDEFINED128
#endif
#elif !defined(__PGI) && !defined(SIMDE_BUG_GCC_REV_208793)
#define SIMDE__HAVE_UNDEFINED128
#endif
#endif

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_undefined_ps(void)
{
	simde__m128 r;

#if defined(SIMDE__HAVE_UNDEFINED128)
	r.n = _mm_undefined_ps();
#else
	r = simde_mm_setzero_ps();
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_unpackhi_ps(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_unpackhi_ps(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	float32x2_t a1 = vget_high_f32(a.neon_f32);
	float32x2_t b1 = vget_high_f32(b.neon_f32);
	float32x2x2_t result = vzip_f32(a1, b1);
	r.neon_f32 = vcombine_f32(result.val[0], result.val[1]);
#else
	r.f32[0] = a.f32[2];
	r.f32[1] = b.f32[2];
	r.f32[2] = a.f32[3];
	r.f32[3] = b.f32[3];
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_unpacklo_ps(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_unpacklo_ps(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	float32x2_t a1 = vget_low_f32(a.neon_f32);
	float32x2_t b1 = vget_low_f32(b.neon_f32);
	float32x2x2_t result = vzip_f32(a1, b1);
	r.neon_f32 = vcombine_f32(result.val[0], result.val[1]);
#else
	r.f32[0] = a.f32[0];
	r.f32[1] = b.f32[0];
	r.f32[2] = a.f32[1];
	r.f32[3] = b.f32[1];
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
simde__m128 simde_mm_xor_ps(simde__m128 a, simde__m128 b)
{
	simde__m128 r;

#if defined(SIMDE_SSE_NATIVE)
	r.n = _mm_xor_ps(a.n, b.n);
#elif defined(SIMDE_SSE_NEON)
	r.neon_i32 = veorq_s32(a.neon_i32, b.neon_i32);
#else
	SIMDE__VECTORIZE
	for (size_t i = 0; i < (sizeof(r.u32) / sizeof(r.u32[0])); i++) {
		r.u32[i] = a.u32[i] ^ b.u32[i];
	}
#endif

	return r;
}

SIMDE__FUNCTION_ATTRIBUTES
void simde_mm_stream_pi(simde__m64 *mem_addr, simde__m64 a)
{
#if defined(SIMDE_SSE_NATIVE)
	_mm_stream_pi(&(mem_addr->n), a.n);
#else
	mem_addr->i64[0] = a.i64[0];
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
void simde_mm_stream_ps(simde_float32 mem_addr[4], simde__m128 a)
{
	simde_assert_aligned(16, mem_addr);

#if defined(SIMDE_SSE_NATIVE)
	_mm_stream_ps(mem_addr, a.n);
#else
	SIMDE__ASSUME_ALIGNED(mem_addr, 16);
	memcpy(mem_addr, &a, sizeof(a));
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
uint32_t simde_mm_getcsr(void)
{
#if defined(SIMDE_SSE_NATIVE)
	return _mm_getcsr();
#else
	uint32_t r = 0;
	int rounding_mode = fegetround();

	switch (rounding_mode) {
	case FE_TONEAREST:
		break;
	case FE_UPWARD:
		r |= 2 << 13;
		break;
	case FE_DOWNWARD:
		r |= 1 << 13;
		break;
	case FE_TOWARDZERO:
		r = 3 << 13;
		break;
	}

	return r;
#endif
}

SIMDE__FUNCTION_ATTRIBUTES
void simde_mm_setcsr(uint32_t a)
{
#if defined(SIMDE_SSE_NATIVE)
	_mm_setcsr(a);
#else
	switch ((a >> 13) & 3) {
	case 0:
		fesetround(FE_TONEAREST);
		break;
	case 1:
		fesetround(FE_DOWNWARD);
		break;
	case 2:
		fesetround(FE_UPWARD);
		break;
	case 3:
		fesetround(FE_TOWARDZERO);
		break;
	}
#endif
}

#define SIMDE_MM_TRANSPOSE4_PS(row0, row1, row2, row3)       \
	do {                                                 \
		simde__m128 tmp3, tmp2, tmp1, tmp0;          \
		tmp0 = simde_mm_unpacklo_ps((row0), (row1)); \
		tmp2 = simde_mm_unpacklo_ps((row2), (row3)); \
		tmp1 = simde_mm_unpackhi_ps((row0), (row1)); \
		tmp3 = simde_mm_unpackhi_ps((row2), (row3)); \
		row0 = simde_mm_movelh_ps(tmp0, tmp2);       \
		row1 = simde_mm_movehl_ps(tmp2, tmp0);       \
		row2 = simde_mm_movelh_ps(tmp1, tmp3);       \
		row3 = simde_mm_movehl_ps(tmp3, tmp1);       \
	} while (0)

SIMDE__END_DECLS

#endif /* !defined(SIMDE__SSE_H) */
