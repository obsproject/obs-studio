/* SPDX-License-Identifier: MIT
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
 *
 * Copyright:
 *   2017-2020 Evan Nemerson <evan@nemerson.com>
 *   2015-2017 John W. Ratcliff <jratcliffscarab@gmail.com>
 *   2015      Brandon Rowlett <browlett@nvidia.com>
 *   2015      Ken Fast <kfast@gdeb.com>
 *   2017      Hasindu Gamaarachchi <hasindu@unsw.edu.au>
 *   2018      Jeff Daily <jeff.daily@amd.com>
 */

#if !defined(SIMDE_X86_SSE2_H)
#define SIMDE_X86_SSE2_H

#include "sse.h"

#if !defined(SIMDE_X86_SSE2_NATIVE) && defined(SIMDE_ENABLE_NATIVE_ALIASES)
#define SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES
#endif

HEDLEY_DIAGNOSTIC_PUSH
SIMDE_DISABLE_UNWANTED_DIAGNOSTICS
SIMDE_BEGIN_DECLS_

typedef union {
#if defined(SIMDE_VECTOR_SUBSCRIPT)
	SIMDE_ALIGN(16) int8_t i8 SIMDE_VECTOR(16) SIMDE_MAY_ALIAS;
	SIMDE_ALIGN(16) int16_t i16 SIMDE_VECTOR(16) SIMDE_MAY_ALIAS;
	SIMDE_ALIGN(16) int32_t i32 SIMDE_VECTOR(16) SIMDE_MAY_ALIAS;
	SIMDE_ALIGN(16) int64_t i64 SIMDE_VECTOR(16) SIMDE_MAY_ALIAS;
	SIMDE_ALIGN(16) uint8_t u8 SIMDE_VECTOR(16) SIMDE_MAY_ALIAS;
	SIMDE_ALIGN(16) uint16_t u16 SIMDE_VECTOR(16) SIMDE_MAY_ALIAS;
	SIMDE_ALIGN(16) uint32_t u32 SIMDE_VECTOR(16) SIMDE_MAY_ALIAS;
	SIMDE_ALIGN(16) uint64_t u64 SIMDE_VECTOR(16) SIMDE_MAY_ALIAS;
#if defined(SIMDE_HAVE_INT128_)
	SIMDE_ALIGN(16) simde_int128 i128 SIMDE_VECTOR(16) SIMDE_MAY_ALIAS;
	SIMDE_ALIGN(16) simde_uint128 u128 SIMDE_VECTOR(16) SIMDE_MAY_ALIAS;
#endif
	SIMDE_ALIGN(16) simde_float32 f32 SIMDE_VECTOR(16) SIMDE_MAY_ALIAS;
	SIMDE_ALIGN(16) simde_float64 f64 SIMDE_VECTOR(16) SIMDE_MAY_ALIAS;

	SIMDE_ALIGN(16) int_fast32_t i32f SIMDE_VECTOR(16) SIMDE_MAY_ALIAS;
	SIMDE_ALIGN(16) uint_fast32_t u32f SIMDE_VECTOR(16) SIMDE_MAY_ALIAS;
#else
	SIMDE_ALIGN(16) int8_t i8[16];
	SIMDE_ALIGN(16) int16_t i16[8];
	SIMDE_ALIGN(16) int32_t i32[4];
	SIMDE_ALIGN(16) int64_t i64[2];
	SIMDE_ALIGN(16) uint8_t u8[16];
	SIMDE_ALIGN(16) uint16_t u16[8];
	SIMDE_ALIGN(16) uint32_t u32[4];
	SIMDE_ALIGN(16) uint64_t u64[2];
#if defined(SIMDE_HAVE_INT128_)
	SIMDE_ALIGN(16) simde_int128 i128[1];
	SIMDE_ALIGN(16) simde_uint128 u128[1];
#endif
	SIMDE_ALIGN(16) simde_float32 f32[4];
	SIMDE_ALIGN(16) simde_float64 f64[2];

	SIMDE_ALIGN(16) int_fast32_t i32f[16 / sizeof(int_fast32_t)];
	SIMDE_ALIGN(16) uint_fast32_t u32f[16 / sizeof(uint_fast32_t)];
#endif

	SIMDE_ALIGN(16) simde__m64_private m64_private[2];
	SIMDE_ALIGN(16) simde__m64 m64[2];

#if defined(SIMDE_X86_SSE2_NATIVE)
	SIMDE_ALIGN(16) __m128i n;
#elif defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	SIMDE_ALIGN(16) int8x16_t neon_i8;
	SIMDE_ALIGN(16) int16x8_t neon_i16;
	SIMDE_ALIGN(16) int32x4_t neon_i32;
	SIMDE_ALIGN(16) int64x2_t neon_i64;
	SIMDE_ALIGN(16) uint8x16_t neon_u8;
	SIMDE_ALIGN(16) uint16x8_t neon_u16;
	SIMDE_ALIGN(16) uint32x4_t neon_u32;
	SIMDE_ALIGN(16) uint64x2_t neon_u64;
	SIMDE_ALIGN(16) float32x4_t neon_f32;
#if defined(SIMDE_ARCH_AARCH64)
	SIMDE_ALIGN(16) float64x2_t neon_f64;
#endif
#elif defined(SIMDE_WASM_SIMD128_NATIVE)
	SIMDE_ALIGN(16) v128_t wasm_v128;
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	SIMDE_ALIGN(16) SIMDE_POWER_ALTIVEC_VECTOR(signed char) altivec_i8;
	SIMDE_ALIGN(16) SIMDE_POWER_ALTIVEC_VECTOR(signed short) altivec_i16;
	SIMDE_ALIGN(16) SIMDE_POWER_ALTIVEC_VECTOR(signed int) altivec_i32;
#if defined(__UINT_FAST32_TYPE__)
	SIMDE_ALIGN(16)
	SIMDE_POWER_ALTIVEC_VECTOR(__INT_FAST32_TYPE__) altivec_i32f;
#else
	SIMDE_ALIGN(16) SIMDE_POWER_ALTIVEC_VECTOR(signed int) altivec_i32f;
#endif
	SIMDE_ALIGN(16)
	SIMDE_POWER_ALTIVEC_VECTOR(signed long long) altivec_i64;
	SIMDE_ALIGN(16) SIMDE_POWER_ALTIVEC_VECTOR(unsigned char) altivec_u8;
	SIMDE_ALIGN(16) SIMDE_POWER_ALTIVEC_VECTOR(unsigned short) altivec_u16;
	SIMDE_ALIGN(16) SIMDE_POWER_ALTIVEC_VECTOR(unsigned int) altivec_u32;
#if defined(__UINT_FAST32_TYPE__)
	SIMDE_ALIGN(16) vector __UINT_FAST32_TYPE__ altivec_u32f;
#else
	SIMDE_ALIGN(16) SIMDE_POWER_ALTIVEC_VECTOR(unsigned int) altivec_u32f;
#endif
	SIMDE_ALIGN(16)
	SIMDE_POWER_ALTIVEC_VECTOR(unsigned long long) altivec_u64;
	SIMDE_ALIGN(16) SIMDE_POWER_ALTIVEC_VECTOR(float) altivec_f32;
#if defined(SIMDE_POWER_ALTIVEC_P7_NATIVE)
	SIMDE_ALIGN(16) SIMDE_POWER_ALTIVEC_VECTOR(double) altivec_f64;
#endif
#endif
} simde__m128i_private;

typedef union {
#if defined(SIMDE_VECTOR_SUBSCRIPT)
	SIMDE_ALIGN(16) int8_t i8 SIMDE_VECTOR(16) SIMDE_MAY_ALIAS;
	SIMDE_ALIGN(16) int16_t i16 SIMDE_VECTOR(16) SIMDE_MAY_ALIAS;
	SIMDE_ALIGN(16) int32_t i32 SIMDE_VECTOR(16) SIMDE_MAY_ALIAS;
	SIMDE_ALIGN(16) int64_t i64 SIMDE_VECTOR(16) SIMDE_MAY_ALIAS;
	SIMDE_ALIGN(16) uint8_t u8 SIMDE_VECTOR(16) SIMDE_MAY_ALIAS;
	SIMDE_ALIGN(16) uint16_t u16 SIMDE_VECTOR(16) SIMDE_MAY_ALIAS;
	SIMDE_ALIGN(16) uint32_t u32 SIMDE_VECTOR(16) SIMDE_MAY_ALIAS;
	SIMDE_ALIGN(16) uint64_t u64 SIMDE_VECTOR(16) SIMDE_MAY_ALIAS;
	SIMDE_ALIGN(16) simde_float32 f32 SIMDE_VECTOR(16) SIMDE_MAY_ALIAS;
	SIMDE_ALIGN(16) simde_float64 f64 SIMDE_VECTOR(16) SIMDE_MAY_ALIAS;
	SIMDE_ALIGN(16) int_fast32_t i32f SIMDE_VECTOR(16) SIMDE_MAY_ALIAS;
	SIMDE_ALIGN(16) uint_fast32_t u32f SIMDE_VECTOR(16) SIMDE_MAY_ALIAS;
#else
	SIMDE_ALIGN(16) int8_t i8[16];
	SIMDE_ALIGN(16) int16_t i16[8];
	SIMDE_ALIGN(16) int32_t i32[4];
	SIMDE_ALIGN(16) int64_t i64[2];
	SIMDE_ALIGN(16) uint8_t u8[16];
	SIMDE_ALIGN(16) uint16_t u16[8];
	SIMDE_ALIGN(16) uint32_t u32[4];
	SIMDE_ALIGN(16) uint64_t u64[2];
	SIMDE_ALIGN(16) simde_float32 f32[4];
	SIMDE_ALIGN(16) simde_float64 f64[2];
	SIMDE_ALIGN(16) int_fast32_t i32f[16 / sizeof(int_fast32_t)];
	SIMDE_ALIGN(16) uint_fast32_t u32f[16 / sizeof(uint_fast32_t)];
#endif

	SIMDE_ALIGN(16) simde__m64_private m64_private[2];
	SIMDE_ALIGN(16) simde__m64 m64[2];

#if defined(SIMDE_X86_SSE2_NATIVE)
	SIMDE_ALIGN(16) __m128d n;
#elif defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	SIMDE_ALIGN(16) int8x16_t neon_i8;
	SIMDE_ALIGN(16) int16x8_t neon_i16;
	SIMDE_ALIGN(16) int32x4_t neon_i32;
	SIMDE_ALIGN(16) int64x2_t neon_i64;
	SIMDE_ALIGN(16) uint8x16_t neon_u8;
	SIMDE_ALIGN(16) uint16x8_t neon_u16;
	SIMDE_ALIGN(16) uint32x4_t neon_u32;
	SIMDE_ALIGN(16) uint64x2_t neon_u64;
	SIMDE_ALIGN(16) float32x4_t neon_f32;
#if defined(SIMDE_ARCH_AARCH64)
	SIMDE_ALIGN(16) float64x2_t neon_f64;
#endif
#elif defined(SIMDE_WASM_SIMD128_NATIVE)
	SIMDE_ALIGN(16) v128_t wasm_v128;
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	SIMDE_ALIGN(16) SIMDE_POWER_ALTIVEC_VECTOR(signed char) altivec_i8;
	SIMDE_ALIGN(16) SIMDE_POWER_ALTIVEC_VECTOR(signed short) altivec_i16;
	SIMDE_ALIGN(16) SIMDE_POWER_ALTIVEC_VECTOR(signed int) altivec_i32;
#if defined(__INT_FAST32_TYPE__)
	SIMDE_ALIGN(16)
	SIMDE_POWER_ALTIVEC_VECTOR(__INT_FAST32_TYPE__) altivec_i32f;
#else
	SIMDE_ALIGN(16) SIMDE_POWER_ALTIVEC_VECTOR(signed int) altivec_i32f;
#endif
	SIMDE_ALIGN(16)
	SIMDE_POWER_ALTIVEC_VECTOR(signed long long) altivec_i64;
	SIMDE_ALIGN(16) SIMDE_POWER_ALTIVEC_VECTOR(unsigned char) altivec_u8;
	SIMDE_ALIGN(16) SIMDE_POWER_ALTIVEC_VECTOR(unsigned short) altivec_u16;
	SIMDE_ALIGN(16) SIMDE_POWER_ALTIVEC_VECTOR(unsigned int) altivec_u32;
#if defined(__UINT_FAST32_TYPE__)
	SIMDE_ALIGN(16) vector __UINT_FAST32_TYPE__ altivec_u32f;
#else
	SIMDE_ALIGN(16) SIMDE_POWER_ALTIVEC_VECTOR(unsigned int) altivec_u32f;
#endif
	SIMDE_ALIGN(16)
	SIMDE_POWER_ALTIVEC_VECTOR(unsigned long long) altivec_u64;
	SIMDE_ALIGN(16) SIMDE_POWER_ALTIVEC_VECTOR(float) altivec_f32;
#if defined(SIMDE_POWER_ALTIVEC_P7_NATIVE)
	SIMDE_ALIGN(16) SIMDE_POWER_ALTIVEC_VECTOR(double) altivec_f64;
#endif
#endif
} simde__m128d_private;

#if defined(SIMDE_X86_SSE2_NATIVE)
typedef __m128i simde__m128i;
typedef __m128d simde__m128d;
#elif defined(SIMDE_ARM_NEON_A32V7_NATIVE)
typedef int64x2_t simde__m128i;
#if defined(SIMDE_ARCH_AARCH64)
typedef float64x2_t simde__m128d;
#elif defined(SIMDE_VECTOR_SUBSCRIPT)
typedef simde_float64 simde__m128d SIMDE_VECTOR(16) SIMDE_MAY_ALIAS;
#else
typedef simde__m128d_private simde__m128d;
#endif
#elif defined(SIMDE_WASM_SIMD128_NATIVE)
typedef v128_t simde__m128i;
typedef v128_t simde__m128d;
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
typedef SIMDE_POWER_ALTIVEC_VECTOR(float) simde__m128i;
typedef SIMDE_POWER_ALTIVEC_VECTOR(double) simde__m128d;
#elif defined(SIMDE_VECTOR_SUBSCRIPT)
typedef int_fast32_t simde__m128i SIMDE_ALIGN(16)
	SIMDE_VECTOR(16) SIMDE_MAY_ALIAS;
typedef simde_float64 simde__m128d SIMDE_ALIGN(16)
	SIMDE_VECTOR(16) SIMDE_MAY_ALIAS;
#else
typedef simde__m128i_private simde__m128i;
typedef simde__m128d_private simde__m128d;
#endif

#if !defined(SIMDE_X86_SSE2_NATIVE) && defined(SIMDE_ENABLE_NATIVE_ALIASES)
#define SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES
typedef simde__m128i __m128i;
typedef simde__m128d __m128d;
#endif

HEDLEY_STATIC_ASSERT(16 == sizeof(simde__m128i), "simde__m128i size incorrect");
HEDLEY_STATIC_ASSERT(16 == sizeof(simde__m128i_private),
		     "simde__m128i_private size incorrect");
HEDLEY_STATIC_ASSERT(16 == sizeof(simde__m128d), "simde__m128d size incorrect");
HEDLEY_STATIC_ASSERT(16 == sizeof(simde__m128d_private),
		     "simde__m128d_private size incorrect");
#if defined(SIMDE_CHECK_ALIGNMENT) && defined(SIMDE_ALIGN_OF)
HEDLEY_STATIC_ASSERT(SIMDE_ALIGN_OF(simde__m128i) == 16,
		     "simde__m128i is not 16-byte aligned");
HEDLEY_STATIC_ASSERT(SIMDE_ALIGN_OF(simde__m128i_private) == 16,
		     "simde__m128i_private is not 16-byte aligned");
HEDLEY_STATIC_ASSERT(SIMDE_ALIGN_OF(simde__m128d) == 16,
		     "simde__m128d is not 16-byte aligned");
HEDLEY_STATIC_ASSERT(SIMDE_ALIGN_OF(simde__m128d_private) == 16,
		     "simde__m128d_private is not 16-byte aligned");
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde__m128i_from_private(simde__m128i_private v)
{
	simde__m128i r;
	simde_memcpy(&r, &v, sizeof(r));
	return r;
}

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i_private simde__m128i_to_private(simde__m128i v)
{
	simde__m128i_private r;
	simde_memcpy(&r, &v, sizeof(r));
	return r;
}

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde__m128d_from_private(simde__m128d_private v)
{
	simde__m128d r;
	simde_memcpy(&r, &v, sizeof(r));
	return r;
}

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d_private simde__m128d_to_private(simde__m128d v)
{
	simde__m128d_private r;
	simde_memcpy(&r, &v, sizeof(r));
	return r;
}

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
SIMDE_X86_GENERATE_CONVERSION_FUNCTION(m128i, int8x16_t, neon, i8)
SIMDE_X86_GENERATE_CONVERSION_FUNCTION(m128i, int16x8_t, neon, i16)
SIMDE_X86_GENERATE_CONVERSION_FUNCTION(m128i, int32x4_t, neon, i32)
SIMDE_X86_GENERATE_CONVERSION_FUNCTION(m128i, int64x2_t, neon, i64)
SIMDE_X86_GENERATE_CONVERSION_FUNCTION(m128i, uint8x16_t, neon, u8)
SIMDE_X86_GENERATE_CONVERSION_FUNCTION(m128i, uint16x8_t, neon, u16)
SIMDE_X86_GENERATE_CONVERSION_FUNCTION(m128i, uint32x4_t, neon, u32)
SIMDE_X86_GENERATE_CONVERSION_FUNCTION(m128i, uint64x2_t, neon, u64)
SIMDE_X86_GENERATE_CONVERSION_FUNCTION(m128i, float32x4_t, neon, f32)
#if defined(SIMDE_ARM_NEON_A64V8_NATIVE)
SIMDE_X86_GENERATE_CONVERSION_FUNCTION(m128i, float64x2_t, neon, f64)
#endif
#endif /* defined(SIMDE_ARM_NEON_A32V7_NATIVE) */

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
SIMDE_X86_GENERATE_CONVERSION_FUNCTION(m128d, int8x16_t, neon, i8)
SIMDE_X86_GENERATE_CONVERSION_FUNCTION(m128d, int16x8_t, neon, i16)
SIMDE_X86_GENERATE_CONVERSION_FUNCTION(m128d, int32x4_t, neon, i32)
SIMDE_X86_GENERATE_CONVERSION_FUNCTION(m128d, int64x2_t, neon, i64)
SIMDE_X86_GENERATE_CONVERSION_FUNCTION(m128d, uint8x16_t, neon, u8)
SIMDE_X86_GENERATE_CONVERSION_FUNCTION(m128d, uint16x8_t, neon, u16)
SIMDE_X86_GENERATE_CONVERSION_FUNCTION(m128d, uint32x4_t, neon, u32)
SIMDE_X86_GENERATE_CONVERSION_FUNCTION(m128d, uint64x2_t, neon, u64)
SIMDE_X86_GENERATE_CONVERSION_FUNCTION(m128d, float32x4_t, neon, f32)
#if defined(SIMDE_ARM_NEON_A64V8_NATIVE)
SIMDE_X86_GENERATE_CONVERSION_FUNCTION(m128d, float64x2_t, neon, f64)
#endif
#endif /* defined(SIMDE_ARM_NEON_A32V7_NATIVE) */

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_add_epi8(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_add_epi8(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i8 = vaddq_s8(a_.neon_i8, b_.neon_i8);
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	r_.altivec_i8 = vec_add(a_.altivec_i8, b_.altivec_i8);
#elif defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.i8 = a_.i8 + b_.i8;
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i8) / sizeof(r_.i8[0])); i++) {
		r_.i8[i] = a_.i8[i] + b_.i8[i];
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_add_epi8(a, b) simde_mm_add_epi8(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_add_epi16(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_add_epi16(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i16 = vaddq_s16(a_.neon_i16, b_.neon_i16);
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	r_.altivec_i16 = vec_add(a_.altivec_i16, b_.altivec_i16);
#elif defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.i16 = a_.i16 + b_.i16;
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i16) / sizeof(r_.i16[0])); i++) {
		r_.i16[i] = a_.i16[i] + b_.i16[i];
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_add_epi16(a, b) simde_mm_add_epi16(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_add_epi32(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_add_epi32(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i32 = vaddq_s32(a_.neon_i32, b_.neon_i32);
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	r_.altivec_i32 = vec_add(a_.altivec_i32, b_.altivec_i32);
#elif defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.i32 = a_.i32 + b_.i32;
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i32) / sizeof(r_.i32[0])); i++) {
		r_.i32[i] = a_.i32[i] + b_.i32[i];
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_add_epi32(a, b) simde_mm_add_epi32(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_add_epi64(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_add_epi64(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i64 = vaddq_s64(a_.neon_i64, b_.neon_i64);
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	r_.altivec_i64 = vec_add(a_.altivec_i64, b_.altivec_i64);
#elif defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.i64 = a_.i64 + b_.i64;
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i64) / sizeof(r_.i64[0])); i++) {
		r_.i64[i] = a_.i64[i] + b_.i64[i];
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_add_epi64(a, b) simde_mm_add_epi64(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_add_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_add_pd(a, b);
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a),
				 b_ = simde__m128d_to_private(b);

#if defined(SIMDE_ARM_NEON_A64V8_NATIVE)
	r_.neon_f64 = vaddq_f64(a_.neon_f64, b_.neon_f64);
#elif defined(SIMDE_WASM_SIMD128_NATIVE)
	r_.wasm_v128 = wasm_f64x2_add(a_.wasm_v128, b_.wasm_v128);
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	r_.altivec_f64 = vec_add(a_.altivec_f64, b_.altivec_f64);
#elif defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.f64 = a_.f64 + b_.f64;
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.f64) / sizeof(r_.f64[0])); i++) {
		r_.f64[i] = a_.f64[i] + b_.f64[i];
	}
#endif

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_add_pd(a, b) simde_mm_add_pd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_move_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_move_sd(a, b);
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a),
				 b_ = simde__m128d_to_private(b);

#if defined(SIMDE_ARM_NEON_A64V8_NATIVE)
	r_.neon_f64 =
		vsetq_lane_f64(vgetq_lane_f64(b_.neon_f64, 0), a_.neon_f64, 0);
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	SIMDE_POWER_ALTIVEC_VECTOR(unsigned char)
	m = {16, 17, 18, 19, 20, 21, 22, 23, 8, 9, 10, 11, 12, 13, 14, 15};
	r_.altivec_f64 = vec_perm(a_.altivec_f64, b_.altivec_f64, m);
#elif defined(SIMDE_SHUFFLE_VECTOR_)
	r_.f64 = SIMDE_SHUFFLE_VECTOR_(64, 16, a_.f64, b_.f64, 2, 1);
#else
	r_.f64[0] = b_.f64[0];
	r_.f64[1] = a_.f64[1];
#endif

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_move_sd(a, b) simde_mm_move_sd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_add_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_add_sd(a, b);
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a),
				 b_ = simde__m128d_to_private(b);

	r_.f64[0] = a_.f64[0] + b_.f64[0];
	r_.f64[1] = a_.f64[1];

#if defined(SIMDE_ASSUME_VECTORIZATION)
	return simde_mm_move_sd(a, simde_mm_add_pd(a, b));
#else
	r_.f64[0] = a_.f64[0] + b_.f64[0];
	r_.f64[1] = a_.f64[1];
#endif

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_add_sd(a, b) simde_mm_add_sd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m64 simde_mm_add_si64(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_X86_SSE2_NATIVE) && defined(SIMDE_X86_MMX_NATIVE)
	return _mm_add_si64(a, b);
#else
	simde__m64_private r_, a_ = simde__m64_to_private(a),
			       b_ = simde__m64_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i64 = vadd_s64(a_.neon_i64, b_.neon_i64);
#else
	r_.i64[0] = a_.i64[0] + b_.i64[0];
#endif

	return simde__m64_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_add_si64(a, b) simde_mm_add_si64(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_adds_epi8(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_adds_epi8(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i8 = vqaddq_s8(a_.neon_i8, b_.neon_i8);
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	r_.altivec_i8 = vec_adds(a_.altivec_i8, b_.altivec_i8);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i8) / sizeof(r_.i8[0])); i++) {
		const int32_t tmp = HEDLEY_STATIC_CAST(int16_t, a_.i8[i]) +
				    HEDLEY_STATIC_CAST(int16_t, b_.i8[i]);
		r_.i8[i] = HEDLEY_STATIC_CAST(
			int8_t,
			((tmp < INT8_MAX) ? ((tmp > INT8_MIN) ? tmp : INT8_MIN)
					  : INT8_MAX));
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_adds_epi8(a, b) simde_mm_adds_epi8(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_adds_epi16(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_adds_epi16(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i16 = vqaddq_s16(a_.neon_i16, b_.neon_i16);
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	r_.altivec_i16 = vec_adds(a_.altivec_i16, b_.altivec_i16);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i16) / sizeof(r_.i16[0])); i++) {
		const int32_t tmp = HEDLEY_STATIC_CAST(int32_t, a_.i16[i]) +
				    HEDLEY_STATIC_CAST(int32_t, b_.i16[i]);
		r_.i16[i] = HEDLEY_STATIC_CAST(
			int16_t,
			((tmp < INT16_MAX)
				 ? ((tmp > INT16_MIN) ? tmp : INT16_MIN)
				 : INT16_MAX));
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_adds_epi16(a, b) simde_mm_adds_epi16(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_adds_epu8(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_adds_epu8(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_u8 = vqaddq_u8(a_.neon_u8, b_.neon_u8);
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	r_.altivec_u8 = vec_adds(a_.altivec_u8, b_.altivec_u8);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.u8) / sizeof(r_.u8[0])); i++) {
		r_.u8[i] = ((UINT8_MAX - a_.u8[i]) > b_.u8[i])
				   ? (a_.u8[i] + b_.u8[i])
				   : UINT8_MAX;
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_adds_epu8(a, b) simde_mm_adds_epu8(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_adds_epu16(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_adds_epu16(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_u16 = vqaddq_u16(a_.neon_u16, b_.neon_u16);
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	r_.altivec_u16 = vec_adds(a_.altivec_u16, b_.altivec_u16);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.u16) / sizeof(r_.u16[0])); i++) {
		r_.u16[i] = ((UINT16_MAX - a_.u16[i]) > b_.u16[i])
				    ? (a_.u16[i] + b_.u16[i])
				    : UINT16_MAX;
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_adds_epu16(a, b) simde_mm_adds_epu16(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_and_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_and_pd(a, b);
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a),
				 b_ = simde__m128d_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i32 = vandq_s32(a_.neon_i32, b_.neon_i32);
#elif defined(SIMDE_WASM_SIMD128_NATIVE)
	r_.wasm_v128 = wasm_v128_and(a_.wasm_v128, b_.wasm_v128);
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	r_.altivec_f64 = vec_and(a_.altivec_f64, b_.altivec_f64);
#elif defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.i32f = a_.i32f & b_.i32f;
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i32f) / sizeof(r_.i32f[0])); i++) {
		r_.i32f[i] = a_.i32f[i] & b_.i32f[i];
	}
#endif

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_and_pd(a, b) simde_mm_and_pd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_and_si128(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_and_si128(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i32 = vandq_s32(b_.neon_i32, a_.neon_i32);
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	r_.altivec_u32f = vec_and(a_.altivec_u32f, b_.altivec_u32f);
#elif defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.i32f = a_.i32f & b_.i32f;
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i32f) / sizeof(r_.i32f[0])); i++) {
		r_.i32f[i] = a_.i32f[i] & b_.i32f[i];
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_and_si128(a, b) simde_mm_and_si128(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_andnot_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_andnot_pd(a, b);
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a),
				 b_ = simde__m128d_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i32 = vbicq_s32(a_.neon_i32, b_.neon_i32);
#elif defined(SIMDE_WASM_SIMD128_NATIVE)
	r_.wasm_v128 = wasm_v128_andnot(b_.wasm_v128, a_.wasm_v128);
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	r_.altivec_i32f = vec_andc(a_.altivec_i32f, b_.altivec_i32f);
#elif defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.i32f = ~a_.i32f & b_.i32f;
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.u64) / sizeof(r_.u64[0])); i++) {
		r_.u64[i] = ~a_.u64[i] & b_.u64[i];
	}
#endif

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_andnot_pd(a, b) simde_mm_andnot_pd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_andnot_si128(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_andnot_si128(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i32 = vbicq_s32(b_.neon_i32, a_.neon_i32);
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	r_.altivec_i32 = vec_andc(b_.altivec_i32, a_.altivec_i32);
#elif defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.i32f = ~a_.i32f & b_.i32f;
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i32f) / sizeof(r_.i32f[0])); i++) {
		r_.i32f[i] = ~(a_.i32f[i]) & b_.i32f[i];
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_andnot_si128(a, b) simde_mm_andnot_si128(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_avg_epu8(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_avg_epu8(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_u8 = vrhaddq_u8(b_.neon_u8, a_.neon_u8);
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	r_.altivec_u8 = vec_avg(a_.altivec_u8, b_.altivec_u8);
#elif defined(SIMDE_VECTOR_SUBSCRIPT_OPS) &&      \
	defined(SIMDE_VECTOR_SUBSCRIPT_SCALAR) && \
	defined(SIMDE_CONVERT_VECTOR_)
	uint16_t wa SIMDE_VECTOR(32);
	uint16_t wb SIMDE_VECTOR(32);
	uint16_t wr SIMDE_VECTOR(32);
	SIMDE_CONVERT_VECTOR_(wa, a_.u8);
	SIMDE_CONVERT_VECTOR_(wb, b_.u8);
	wr = (wa + wb + 1) >> 1;
	SIMDE_CONVERT_VECTOR_(r_.u8, wr);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.u8) / sizeof(r_.u8[0])); i++) {
		r_.u8[i] = (a_.u8[i] + b_.u8[i] + 1) >> 1;
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_avg_epu8(a, b) simde_mm_avg_epu8(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_avg_epu16(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_avg_epu16(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_u16 = vrhaddq_u16(b_.neon_u16, a_.neon_u16);
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	r_.altivec_u16 = vec_avg(a_.altivec_u16, b_.altivec_u16);
#elif defined(SIMDE_VECTOR_SUBSCRIPT_OPS) &&      \
	defined(SIMDE_VECTOR_SUBSCRIPT_SCALAR) && \
	defined(SIMDE_CONVERT_VECTOR_)
	uint32_t wa SIMDE_VECTOR(32);
	uint32_t wb SIMDE_VECTOR(32);
	uint32_t wr SIMDE_VECTOR(32);
	SIMDE_CONVERT_VECTOR_(wa, a_.u16);
	SIMDE_CONVERT_VECTOR_(wb, b_.u16);
	wr = (wa + wb + 1) >> 1;
	SIMDE_CONVERT_VECTOR_(r_.u16, wr);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.u16) / sizeof(r_.u16[0])); i++) {
		r_.u16[i] = (a_.u16[i] + b_.u16[i] + 1) >> 1;
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_avg_epu16(a, b) simde_mm_avg_epu16(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_setzero_si128(void)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_setzero_si128();
#else
	simde__m128i_private r_;

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i32 = vdupq_n_s32(0);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i32f) / sizeof(r_.i32f[0])); i++) {
		r_.i32f[i] = 0;
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_setzero_si128() (simde_mm_setzero_si128())
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_bslli_si128(simde__m128i a, const int imm8)
	SIMDE_REQUIRE_RANGE(imm8, 0, 255)
{
	simde__m128i_private r_, a_ = simde__m128i_to_private(a);

	if (HEDLEY_UNLIKELY((imm8 & ~15))) {
		return simde_mm_setzero_si128();
	}

#if defined(SIMDE_HAVE_INT128_) && defined(__BYTE_ORDER__) && \
	(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) && 0
	r_.u128[0] = a_.u128[0] << s;
#else
	r_ = simde__m128i_to_private(simde_mm_setzero_si128());
	for (int i = imm8;
	     i < HEDLEY_STATIC_CAST(int, sizeof(r_.i8) / sizeof(r_.i8[0]));
	     i++) {
		r_.i8[i] = a_.i8[i - imm8];
	}
#endif

	return simde__m128i_from_private(r_);
}
#if defined(SIMDE_X86_SSE2_NATIVE) && !defined(__PGI)
#define simde_mm_bslli_si128(a, imm8) _mm_slli_si128(a, imm8)
#elif defined(SIMDE_ARM_NEON_A32V7_NATIVE) && !defined(__clang__)
#define simde_mm_bslli_si128(a, imm8)                                      \
	simde__m128i_from_neon_i8(                                         \
		((imm8) <= 0)                                              \
			? simde__m128i_to_neon_i8(a)                       \
			: (((imm8) > 15)                                   \
				   ? (vdupq_n_s8(0))                       \
				   : (vextq_s8(vdupq_n_s8(0),              \
					       simde__m128i_to_neon_i8(a), \
					       16 - (imm8)))))
#elif defined(SIMDE_SHUFFLE_VECTOR_)
#define simde_mm_bslli_si128(a, imm8)                                          \
	(__extension__({                                                       \
		const simde__m128i_private simde__tmp_a_ =                     \
			simde__m128i_to_private(a);                            \
		const simde__m128i_private simde__tmp_z_ =                     \
			simde__m128i_to_private(simde_mm_setzero_si128());     \
		simde__m128i_private simde__tmp_r_;                            \
		if (HEDLEY_UNLIKELY(imm8 > 15)) {                              \
			simde__tmp_r_ = simde__m128i_to_private(               \
				simde_mm_setzero_si128());                     \
		} else {                                                       \
			simde__tmp_r_.i8 = SIMDE_SHUFFLE_VECTOR_(              \
				8, 16, simde__tmp_z_.i8, (simde__tmp_a_).i8,   \
				HEDLEY_STATIC_CAST(int8_t, (16 - imm8) & 31),  \
				HEDLEY_STATIC_CAST(int8_t, (17 - imm8) & 31),  \
				HEDLEY_STATIC_CAST(int8_t, (18 - imm8) & 31),  \
				HEDLEY_STATIC_CAST(int8_t, (19 - imm8) & 31),  \
				HEDLEY_STATIC_CAST(int8_t, (20 - imm8) & 31),  \
				HEDLEY_STATIC_CAST(int8_t, (21 - imm8) & 31),  \
				HEDLEY_STATIC_CAST(int8_t, (22 - imm8) & 31),  \
				HEDLEY_STATIC_CAST(int8_t, (23 - imm8) & 31),  \
				HEDLEY_STATIC_CAST(int8_t, (24 - imm8) & 31),  \
				HEDLEY_STATIC_CAST(int8_t, (25 - imm8) & 31),  \
				HEDLEY_STATIC_CAST(int8_t, (26 - imm8) & 31),  \
				HEDLEY_STATIC_CAST(int8_t, (27 - imm8) & 31),  \
				HEDLEY_STATIC_CAST(int8_t, (28 - imm8) & 31),  \
				HEDLEY_STATIC_CAST(int8_t, (29 - imm8) & 31),  \
				HEDLEY_STATIC_CAST(int8_t, (30 - imm8) & 31),  \
				HEDLEY_STATIC_CAST(int8_t, (31 - imm8) & 31)); \
		}                                                              \
		simde__m128i_from_private(simde__tmp_r_);                      \
	}))
#endif
#define simde_mm_slli_si128(a, imm8) simde_mm_bslli_si128(a, imm8)
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_bslli_si128(a, b) simde_mm_bslli_si128(a, b)
#define _mm_slli_si128(a, b) simde_mm_bslli_si128(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_bsrli_si128(simde__m128i a, const int imm8)
	SIMDE_REQUIRE_RANGE(imm8, 0, 255)
{
	simde__m128i_private r_, a_ = simde__m128i_to_private(a);

	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i8) / sizeof(r_.i8[0])); i++) {
		const int e = HEDLEY_STATIC_CAST(int, i) + imm8;
		r_.i8[i] = (e < 16) ? a_.i8[e] : 0;
	}

	return simde__m128i_from_private(r_);
}
#if defined(SIMDE_X86_SSE2_NATIVE) && !defined(__PGI)
#define simde_mm_bsrli_si128(a, imm8) _mm_srli_si128(a, imm8)
#elif defined(SIMDE_ARM_NEON_A32V7_NATIVE) && !defined(__clang__)
#define simde_mm_bsrli_si128(a, imm8)                                   \
	simde__m128i_from_neon_i8(                                      \
		((imm8 < 0) || (imm8 > 15))                             \
			? vdupq_n_s8(0)                                 \
			: (vextq_s8(simde__m128i_to_private(a).neon_i8, \
				    vdupq_n_s8(0),                      \
				    ((imm8 & 15) != 0) ? imm8 : (imm8 & 15))))
#elif defined(SIMDE_SHUFFLE_VECTOR_)
#define simde_mm_bsrli_si128(a, imm8)                                          \
	(__extension__({                                                       \
		const simde__m128i_private simde__tmp_a_ =                     \
			simde__m128i_to_private(a);                            \
		const simde__m128i_private simde__tmp_z_ =                     \
			simde__m128i_to_private(simde_mm_setzero_si128());     \
		simde__m128i_private simde__tmp_r_ =                           \
			simde__m128i_to_private(a);                            \
		if (HEDLEY_UNLIKELY(imm8 > 15)) {                              \
			simde__tmp_r_ = simde__m128i_to_private(               \
				simde_mm_setzero_si128());                     \
		} else {                                                       \
			simde__tmp_r_.i8 = SIMDE_SHUFFLE_VECTOR_(              \
				8, 16, simde__tmp_z_.i8, (simde__tmp_a_).i8,   \
				HEDLEY_STATIC_CAST(int8_t, (imm8 + 16) & 31),  \
				HEDLEY_STATIC_CAST(int8_t, (imm8 + 17) & 31),  \
				HEDLEY_STATIC_CAST(int8_t, (imm8 + 18) & 31),  \
				HEDLEY_STATIC_CAST(int8_t, (imm8 + 19) & 31),  \
				HEDLEY_STATIC_CAST(int8_t, (imm8 + 20) & 31),  \
				HEDLEY_STATIC_CAST(int8_t, (imm8 + 21) & 31),  \
				HEDLEY_STATIC_CAST(int8_t, (imm8 + 22) & 31),  \
				HEDLEY_STATIC_CAST(int8_t, (imm8 + 23) & 31),  \
				HEDLEY_STATIC_CAST(int8_t, (imm8 + 24) & 31),  \
				HEDLEY_STATIC_CAST(int8_t, (imm8 + 25) & 31),  \
				HEDLEY_STATIC_CAST(int8_t, (imm8 + 26) & 31),  \
				HEDLEY_STATIC_CAST(int8_t, (imm8 + 27) & 31),  \
				HEDLEY_STATIC_CAST(int8_t, (imm8 + 28) & 31),  \
				HEDLEY_STATIC_CAST(int8_t, (imm8 + 29) & 31),  \
				HEDLEY_STATIC_CAST(int8_t, (imm8 + 30) & 31),  \
				HEDLEY_STATIC_CAST(int8_t, (imm8 + 31) & 31)); \
		}                                                              \
		simde__m128i_from_private(simde__tmp_r_);                      \
	}))
#endif
#define simde_mm_srli_si128(a, imm8) simde_mm_bsrli_si128((a), (imm8))
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_bsrli_si128(a, imm8) simde_mm_bsrli_si128((a), (imm8))
#define _mm_srli_si128(a, imm8) simde_mm_bsrli_si128((a), (imm8))
#endif

SIMDE_FUNCTION_ATTRIBUTES
void simde_mm_clflush(void const *p)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	_mm_clflush(p);
#else
	(void)p;
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_clflush(a, b) simde_mm_clflush()
#endif

SIMDE_FUNCTION_ATTRIBUTES
int simde_mm_comieq_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_comieq_sd(a, b);
#else
	simde__m128d_private a_ = simde__m128d_to_private(a),
			     b_ = simde__m128d_to_private(b);
#if defined(SIMDE_ARM_NEON_A64V8_NATIVE)
	return !!vgetq_lane_u64(vceqq_f64(a_.neon_f64, b_.neon_f64), 0);
#else
	return a_.f64[0] == b_.f64[0];
#endif
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_comieq_sd(a, b) simde_mm_comieq_sd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
int simde_mm_comige_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_comige_sd(a, b);
#else
	simde__m128d_private a_ = simde__m128d_to_private(a),
			     b_ = simde__m128d_to_private(b);
#if defined(SIMDE_ARM_NEON_A64V8_NATIVE)
	return !!vgetq_lane_u64(vcgeq_f64(a_.neon_f64, b_.neon_f64), 0);
#else
	return a_.f64[0] >= b_.f64[0];
#endif
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_comige_sd(a, b) simde_mm_comige_sd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
int simde_mm_comigt_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_comigt_sd(a, b);
#else
	simde__m128d_private a_ = simde__m128d_to_private(a),
			     b_ = simde__m128d_to_private(b);
#if defined(SIMDE_ARM_NEON_A64V8_NATIVE)
	return !!vgetq_lane_u64(vcgtq_f64(a_.neon_f64, b_.neon_f64), 0);
#else
	return a_.f64[0] > b_.f64[0];
#endif
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_comigt_sd(a, b) simde_mm_comigt_sd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
int simde_mm_comile_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_comile_sd(a, b);
#else
	simde__m128d_private a_ = simde__m128d_to_private(a),
			     b_ = simde__m128d_to_private(b);
#if defined(SIMDE_ARM_NEON_A64V8_NATIVE)
	return !!vgetq_lane_u64(vcleq_f64(a_.neon_f64, b_.neon_f64), 0);
#else
	return a_.f64[0] <= b_.f64[0];
#endif
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_comile_sd(a, b) simde_mm_comile_sd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
int simde_mm_comilt_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_comilt_sd(a, b);
#else
	simde__m128d_private a_ = simde__m128d_to_private(a),
			     b_ = simde__m128d_to_private(b);
#if defined(SIMDE_ARM_NEON_A64V8_NATIVE)
	return !!vgetq_lane_u64(vcltq_f64(a_.neon_f64, b_.neon_f64), 0);
#else
	return a_.f64[0] < b_.f64[0];
#endif
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_comilt_sd(a, b) simde_mm_comilt_sd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
int simde_mm_comineq_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_comineq_sd(a, b);
#else
	simde__m128d_private a_ = simde__m128d_to_private(a),
			     b_ = simde__m128d_to_private(b);
#if defined(SIMDE_ARM_NEON_A64V8_NATIVE)
	return !vgetq_lane_u64(vceqq_f64(a_.neon_f64, b_.neon_f64), 0);
#else
	return a_.f64[0] != b_.f64[0];
#endif
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_comineq_sd(a, b) simde_mm_comineq_sd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128 simde_mm_castpd_ps(simde__m128d a)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_castpd_ps(a);
#else
	simde__m128 r;
	simde_memcpy(&r, &a, sizeof(a));
	return r;
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_castpd_ps(a) simde_mm_castpd_ps(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_castpd_si128(simde__m128d a)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_castpd_si128(a);
#else
	simde__m128i r;
	simde_memcpy(&r, &a, sizeof(a));
	return r;
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_castpd_si128(a) simde_mm_castpd_si128(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_castps_pd(simde__m128 a)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_castps_pd(a);
#else
	simde__m128d r;
	simde_memcpy(&r, &a, sizeof(a));
	return r;
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_castps_pd(a) simde_mm_castps_pd(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_castps_si128(simde__m128 a)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_castps_si128(a);
#else
	simde__m128i r;
	simde_memcpy(&r, &a, sizeof(a));
	return r;
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_castps_si128(a) simde_mm_castps_si128(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_castsi128_pd(simde__m128i a)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_castsi128_pd(a);
#else
	simde__m128d r;
	simde_memcpy(&r, &a, sizeof(a));
	return r;
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_castsi128_pd(a) simde_mm_castsi128_pd(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128 simde_mm_castsi128_ps(simde__m128i a)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_castsi128_ps(a);
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	return a;
#else
	simde__m128 r;
	simde_memcpy(&r, &a, sizeof(a));
	return r;
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_castsi128_ps(a) simde_mm_castsi128_ps(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_cmpeq_epi8(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cmpeq_epi8(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i8 = vreinterpretq_s8_u8(vceqq_s8(b_.neon_i8, a_.neon_i8));
#elif defined(SIMDE_WASM_SIMD128_NATIVE)
	r_.wasm_v128 = wasm_i8x16_eq(a_.wasm_v128, b_.wasm_v128);
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	r_.altivec_i8 = (SIMDE_POWER_ALTIVEC_VECTOR(signed char))vec_cmpeq(
		a_.altivec_i8, b_.altivec_i8);
#elif defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.i8 = HEDLEY_STATIC_CAST(__typeof__(r_.i8), (a_.i8 == b_.i8));
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i8) / sizeof(r_.i8[0])); i++) {
		r_.i8[i] = (a_.i8[i] == b_.i8[i]) ? ~INT8_C(0) : INT8_C(0);
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cmpeq_epi8(a, b) simde_mm_cmpeq_epi8(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_cmpeq_epi16(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cmpeq_epi16(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i16 =
		vreinterpretq_s16_u16(vceqq_s16(b_.neon_i16, a_.neon_i16));
#elif defined(SIMDE_WASM_SIMD128_NATIVE)
	r_.wasm_v128 = wasm_i16x8_eq(a_.wasm_v128, b_.wasm_v128);
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	r_.altivec_i16 = (SIMDE_POWER_ALTIVEC_VECTOR(signed short))vec_cmpeq(
		a_.altivec_i16, b_.altivec_i16);
#elif defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.i16 = (a_.i16 == b_.i16);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i16) / sizeof(r_.i16[0])); i++) {
		r_.i16[i] = (a_.i16[i] == b_.i16[i]) ? ~INT16_C(0) : INT16_C(0);
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cmpeq_epi16(a, b) simde_mm_cmpeq_epi16(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_cmpeq_epi32(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cmpeq_epi32(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i32 =
		vreinterpretq_s32_u32(vceqq_s32(b_.neon_i32, a_.neon_i32));
#elif defined(SIMDE_WASM_SIMD128_NATIVE)
	r_.wasm_v128 = wasm_i32x4_eq(a_.wasm_v128, b_.wasm_v128);
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	r_.altivec_i32 = (SIMDE_POWER_ALTIVEC_VECTOR(signed int))vec_cmpeq(
		a_.altivec_i32, b_.altivec_i32);
#elif defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.i32 = HEDLEY_STATIC_CAST(__typeof__(r_.i32), a_.i32 == b_.i32);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i32) / sizeof(r_.i32[0])); i++) {
		r_.i32[i] = (a_.i32[i] == b_.i32[i]) ? ~INT32_C(0) : INT32_C(0);
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cmpeq_epi32(a, b) simde_mm_cmpeq_epi32(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmpeq_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cmpeq_pd(a, b);
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a),
				 b_ = simde__m128d_to_private(b);

#if defined(SIMDE_ARM_NEON_A64V8_NATIVE)
	r_.neon_i64 = vreinterpretq_s64_u64(
		vceqq_s64(vreinterpretq_s64_f64(b_.neon_f64),
			  vreinterpretq_s64_f64(a_.neon_f64)));
#elif defined(SIMDE_WASM_SIMD128_NATIVE)
	r_.wasm_v128 = wasm_f64x2_eq(a_.wasm_v128, b_.wasm_v128);
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	r_.altivec_f64 = (SIMDE_POWER_ALTIVEC_VECTOR(double))vec_cmpeq(
		a_.altivec_f64, b_.altivec_f64);
#elif defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.i64 = HEDLEY_STATIC_CAST(__typeof__(r_.i64), (a_.f64 == b_.f64));
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.f64) / sizeof(r_.f64[0])); i++) {
		r_.u64[i] = (a_.f64[i] == b_.f64[i]) ? ~UINT64_C(0)
						     : UINT64_C(0);
	}
#endif

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cmpeq_pd(a, b) simde_mm_cmpeq_pd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmpeq_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cmpeq_sd(a, b);
#elif defined(SIMDE_ASSUME_VECTORIZATION)
	return simde_mm_move_sd(a, simde_mm_cmpeq_pd(a, b));
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a),
				 b_ = simde__m128d_to_private(b);

	r_.u64[0] = (a_.u64[0] == b_.u64[0]) ? ~UINT64_C(0) : 0;
	r_.u64[1] = a_.u64[1];

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cmpeq_sd(a, b) simde_mm_cmpeq_sd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmpneq_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cmpneq_pd(a, b);
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a),
				 b_ = simde__m128d_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_f32 = vreinterpretq_f32_u16(
		vmvnq_u16(vceqq_s16(b_.neon_i16, a_.neon_i16)));
#elif defined(SIMDE_WASM_SIMD128_NATIVE)
	r_.wasm_v128 = wasm_f64x2_ne(a_.wasm_v128, b_.wasm_v128);
#elif defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.i64 = HEDLEY_STATIC_CAST(__typeof__(r_.i64), (a_.f64 != b_.f64));
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.f64) / sizeof(r_.f64[0])); i++) {
		r_.u64[i] = (a_.f64[i] != b_.f64[i]) ? ~UINT64_C(0)
						     : UINT64_C(0);
	}
#endif

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cmpneq_pd(a, b) simde_mm_cmpneq_pd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmpneq_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cmpneq_sd(a, b);
#elif defined(SIMDE_ASSUME_VECTORIZATION)
	return simde_mm_move_sd(a, simde_mm_cmpneq_pd(a, b));
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a),
				 b_ = simde__m128d_to_private(b);

	r_.u64[0] = (a_.f64[0] != b_.f64[0]) ? ~UINT64_C(0) : UINT64_C(0);
	r_.u64[1] = a_.u64[1];

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cmpneq_sd(a, b) simde_mm_cmpneq_sd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_cmplt_epi8(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cmplt_epi8(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i8 = vreinterpretq_s8_u8(vcltq_s8(a_.neon_i8, b_.neon_i8));
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	r_.altivec_i8 = HEDLEY_REINTERPRET_CAST(
		SIMDE_POWER_ALTIVEC_VECTOR(signed char),
		vec_cmplt(a_.altivec_i8, b_.altivec_i8));
#elif defined(SIMDE_WASM_SIMD128_NATIVE)
	r_.wasm_v128 = wasm_i8x16_lt(a_.wasm_v128, b_.wasm_v128);
#elif defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.i8 = HEDLEY_STATIC_CAST(__typeof__(r_.i8), (a_.i8 < b_.i8));
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i8) / sizeof(r_.i8[0])); i++) {
		r_.i8[i] = (a_.i8[i] < b_.i8[i]) ? ~INT8_C(0) : INT8_C(0);
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cmplt_epi8(a, b) simde_mm_cmplt_epi8(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_cmplt_epi16(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cmplt_epi16(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i16 =
		vreinterpretq_s16_u16(vcltq_s16(a_.neon_i16, b_.neon_i16));
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	r_.altivec_i16 = HEDLEY_REINTERPRET_CAST(
		SIMDE_POWER_ALTIVEC_VECTOR(signed short),
		vec_cmplt(a_.altivec_i16, b_.altivec_i16));
#elif defined(SIMDE_WASM_SIMD128_NATIVE)
	r_.wasm_v128 = wasm_i16x8_lt(a_.wasm_v128, b_.wasm_v128);
#elif defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.i16 = HEDLEY_STATIC_CAST(__typeof__(r_.i16), (a_.i16 < b_.i16));
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i16) / sizeof(r_.i16[0])); i++) {
		r_.i16[i] = (a_.i16[i] < b_.i16[i]) ? ~INT16_C(0) : INT16_C(0);
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cmplt_epi16(a, b) simde_mm_cmplt_epi16(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_cmplt_epi32(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cmplt_epi32(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i32 =
		vreinterpretq_s32_u32(vcltq_s32(a_.neon_i32, b_.neon_i32));
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	r_.altivec_i32 = HEDLEY_REINTERPRET_CAST(
		SIMDE_POWER_ALTIVEC_VECTOR(signed int),
		vec_cmplt(a_.altivec_i32, b_.altivec_i32));
#elif defined(SIMDE_WASM_SIMD128_NATIVE)
	r_.wasm_v128 = wasm_i32x4_lt(a_.wasm_v128, b_.wasm_v128);
#elif defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.i32 = HEDLEY_STATIC_CAST(__typeof__(r_.i32), (a_.i32 < b_.i32));
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i32) / sizeof(r_.i32[0])); i++) {
		r_.i32[i] = (a_.i32[i] < b_.i32[i]) ? ~INT32_C(0) : INT32_C(0);
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cmplt_epi32(a, b) simde_mm_cmplt_epi32(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmplt_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cmplt_pd(a, b);
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a),
				 b_ = simde__m128d_to_private(b);

#if defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.i64 = HEDLEY_STATIC_CAST(__typeof__(r_.i64), (a_.f64 < b_.f64));
#elif defined(SIMDE_WASM_SIMD128_NATIVE)
	r_.wasm_v128 = wasm_f64x2_lt(a_.wasm_v128, b_.wasm_v128);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.f64) / sizeof(r_.f64[0])); i++) {
		r_.u64[i] = (a_.f64[i] < b_.f64[i]) ? ~UINT64_C(0)
						    : UINT64_C(0);
	}
#endif

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cmplt_pd(a, b) simde_mm_cmplt_pd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmplt_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cmplt_sd(a, b);
#elif defined(SIMDE_ASSUME_VECTORIZATION)
	return simde_mm_move_sd(a, simde_mm_cmplt_pd(a, b));
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a),
				 b_ = simde__m128d_to_private(b);

	r_.u64[0] = (a_.f64[0] < b_.f64[0]) ? ~UINT64_C(0) : UINT64_C(0);
	r_.u64[1] = a_.u64[1];

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cmplt_sd(a, b) simde_mm_cmplt_sd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmple_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cmple_pd(a, b);
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a),
				 b_ = simde__m128d_to_private(b);

#if defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.i64 = HEDLEY_STATIC_CAST(__typeof__(r_.i64), (a_.f64 <= b_.f64));
#elif defined(SIMDE_WASM_SIMD128_NATIVE)
	r_.wasm_v128 = wasm_f64x2_le(a_.wasm_v128, b_.wasm_v128);
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	r_.altivec_f64 = (SIMDE_POWER_ALTIVEC_VECTOR(double))vec_cmple(
		a_.altivec_f64, b_.altivec_f64);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.f64) / sizeof(r_.f64[0])); i++) {
		r_.u64[i] = (a_.f64[i] <= b_.f64[i]) ? ~UINT64_C(0)
						     : UINT64_C(0);
	}
#endif

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cmple_pd(a, b) simde_mm_cmple_pd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmple_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cmple_sd(a, b);
#elif defined(SIMDE_ASSUME_VECTORIZATION)
	return simde_mm_move_sd(a, simde_mm_cmple_pd(a, b));
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a),
				 b_ = simde__m128d_to_private(b);

	r_.u64[0] = (a_.f64[0] <= b_.f64[0]) ? ~UINT64_C(0) : UINT64_C(0);
	r_.u64[1] = a_.u64[1];

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cmple_sd(a, b) simde_mm_cmple_sd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_cmpgt_epi8(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cmpgt_epi8(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i8 = vreinterpretq_s8_u8(vcgtq_s8(a_.neon_i8, b_.neon_i8));
#elif defined(SIMDE_WASM_SIMD128_NATIVE)
	r_.wasm_v128 = wasm_i8x16_gt(a_.wasm_v128, b_.wasm_v128);
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	r_.altivec_i8 = (SIMDE_POWER_ALTIVEC_VECTOR(signed char))vec_cmpgt(
		a_.altivec_i8, b_.altivec_i8);
#elif defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.i8 = HEDLEY_STATIC_CAST(__typeof__(r_.i8), (a_.i8 > b_.i8));
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i8) / sizeof(r_.i8[0])); i++) {
		r_.i8[i] = (a_.i8[i] > b_.i8[i]) ? ~INT8_C(0) : INT8_C(0);
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cmpgt_epi8(a, b) simde_mm_cmpgt_epi8(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_cmpgt_epi16(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cmpgt_epi16(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i16 =
		vreinterpretq_s16_u16(vcgtq_s16(a_.neon_i16, b_.neon_i16));
#elif defined(SIMDE_WASM_SIMD128_NATIVE)
	r_.wasm_v128 = wasm_i16x8_gt(a_.wasm_v128, b_.wasm_v128);
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	r_.altivec_i16 = HEDLEY_REINTERPRET_CAST(
		SIMDE_POWER_ALTIVEC_VECTOR(signed short),
		vec_cmpgt(a_.altivec_i16, b_.altivec_i16));
#elif defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.i16 = HEDLEY_STATIC_CAST(__typeof__(r_.i16), (a_.i16 > b_.i16));
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i16) / sizeof(r_.i16[0])); i++) {
		r_.i16[i] = (a_.i16[i] > b_.i16[i]) ? ~INT16_C(0) : INT16_C(0);
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cmpgt_epi16(a, b) simde_mm_cmpgt_epi16(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_cmpgt_epi32(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cmpgt_epi32(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i32 =
		vreinterpretq_s32_u32(vcgtq_s32(a_.neon_i32, b_.neon_i32));
#elif defined(SIMDE_WASM_SIMD128_NATIVE)
	r_.wasm_v128 = wasm_i32x4_gt(a_.wasm_v128, b_.wasm_v128);
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	r_.altivec_i32 = (SIMDE_POWER_ALTIVEC_VECTOR(signed int))vec_cmpgt(
		a_.altivec_i32, b_.altivec_i32);
#elif defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.i32 = HEDLEY_STATIC_CAST(__typeof__(r_.i32), (a_.i32 > b_.i32));
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i32) / sizeof(r_.i32[0])); i++) {
		r_.i32[i] = (a_.i32[i] > b_.i32[i]) ? ~INT32_C(0) : INT32_C(0);
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cmpgt_epi32(a, b) simde_mm_cmpgt_epi32(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmpgt_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cmpgt_pd(a, b);
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a),
				 b_ = simde__m128d_to_private(b);

#if defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.i64 = HEDLEY_STATIC_CAST(__typeof__(r_.i64), (a_.f64 > b_.f64));
#elif defined(SIMDE_WASM_SIMD128_NATIVE)
	r_.wasm_v128 = wasm_f64x2_gt(a_.wasm_v128, b_.wasm_v128);
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	r_.altivec_f64 =
		HEDLEY_STATIC_CAST(SIMDE_POWER_ALTIVEC_VECTOR(double),
				   vec_cmpgt(a_.altivec_f64, b_.altivec_f64));
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.f64) / sizeof(r_.f64[0])); i++) {
		r_.u64[i] = (a_.f64[i] > b_.f64[i]) ? ~UINT64_C(0)
						    : UINT64_C(0);
	}
#endif

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cmpgt_pd(a, b) simde_mm_cmpgt_pd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmpgt_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE) && !defined(__PGI)
	return _mm_cmpgt_sd(a, b);
#elif defined(SIMDE_ASSUME_VECTORIZATION)
	return simde_mm_move_sd(a, simde_mm_cmpgt_pd(a, b));
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a),
				 b_ = simde__m128d_to_private(b);

	r_.u64[0] = (a_.f64[0] > b_.f64[0]) ? ~UINT64_C(0) : UINT64_C(0);
	r_.u64[1] = a_.u64[1];

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cmpgt_sd(a, b) simde_mm_cmpgt_sd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmpge_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cmpge_pd(a, b);
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a),
				 b_ = simde__m128d_to_private(b);

#if defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.i64 = HEDLEY_STATIC_CAST(__typeof__(r_.i64), (a_.f64 >= b_.f64));
#elif defined(SIMDE_WASM_SIMD128_NATIVE)
	r_.wasm_v128 = wasm_f64x2_ge(a_.wasm_v128, b_.wasm_v128);
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	r_.altivec_f64 =
		HEDLEY_STATIC_CAST(SIMDE_POWER_ALTIVEC_VECTOR(double),
				   vec_cmpge(a_.altivec_f64, b_.altivec_f64));
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.f64) / sizeof(r_.f64[0])); i++) {
		r_.u64[i] = (a_.f64[i] >= b_.f64[i]) ? ~UINT64_C(0)
						     : UINT64_C(0);
	}
#endif

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cmpge_pd(a, b) simde_mm_cmpge_pd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmpge_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE) && !defined(__PGI)
	return _mm_cmpge_sd(a, b);
#elif defined(SIMDE_ASSUME_VECTORIZATION)
	return simde_mm_move_sd(a, simde_mm_cmpge_pd(a, b));
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a),
				 b_ = simde__m128d_to_private(b);

	r_.u64[0] = (a_.f64[0] >= b_.f64[0]) ? ~UINT64_C(0) : UINT64_C(0);
	r_.u64[1] = a_.u64[1];

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cmpge_sd(a, b) simde_mm_cmpge_sd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmpnge_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cmpnge_pd(a, b);
#else
	return simde_mm_cmplt_pd(a, b);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cmpnge_pd(a, b) simde_mm_cmpnge_pd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmpnge_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE) && !defined(__PGI)
	return _mm_cmpnge_sd(a, b);
#else
	return simde_mm_cmplt_sd(a, b);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cmpnge_sd(a, b) simde_mm_cmpnge_sd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmpnlt_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cmpnlt_pd(a, b);
#else
	return simde_mm_cmpge_pd(a, b);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cmpnlt_pd(a, b) simde_mm_cmpnlt_pd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmpnlt_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cmpnlt_sd(a, b);
#else
	return simde_mm_cmpge_sd(a, b);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cmpnlt_sd(a, b) simde_mm_cmpnlt_sd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmpnle_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cmpnle_pd(a, b);
#else
	return simde_mm_cmpgt_pd(a, b);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cmpnle_pd(a, b) simde_mm_cmpnle_pd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmpnle_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cmpnle_sd(a, b);
#else
	return simde_mm_cmpgt_sd(a, b);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cmpnle_sd(a, b) simde_mm_cmpnle_sd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmpord_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cmpord_pd(a, b);
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a),
				 b_ = simde__m128d_to_private(b);

#if defined(simde_math_isnan)
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.f64) / sizeof(r_.f64[0])); i++) {
		r_.u64[i] = (!simde_math_isnan(a_.f64[i]) &&
			     !simde_math_isnan(b_.f64[i]))
				    ? ~UINT64_C(0)
				    : UINT64_C(0);
	}
#else
	HEDLEY_UNREACHABLE();
#endif

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cmpord_pd(a, b) simde_mm_cmpord_pd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde_float64 simde_mm_cvtsd_f64(simde__m128d a)
{
#if defined(SIMDE_X86_SSE2_NATIVE) && !defined(__PGI)
	return _mm_cvtsd_f64(a);
#else
	simde__m128d_private a_ = simde__m128d_to_private(a);
	return a_.f64[0];
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cvtsd_f64(a) simde_mm_cvtsd_f64(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmpord_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cmpord_sd(a, b);
#elif defined(SIMDE_ASSUME_VECTORIZATION)
	return simde_mm_move_sd(a, simde_mm_cmpord_pd(a, b));
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a),
				 b_ = simde__m128d_to_private(b);

#if defined(simde_math_isnan)
	r_.u64[0] =
		(!simde_math_isnan(a_.f64[0]) && !simde_math_isnan(b_.f64[0]))
			? ~UINT64_C(0)
			: UINT64_C(0);
	r_.u64[1] = a_.u64[1];
#else
	HEDLEY_UNREACHABLE();
#endif

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cmpord_sd(a, b) simde_mm_cmpord_sd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmpunord_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cmpunord_pd(a, b);
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a),
				 b_ = simde__m128d_to_private(b);

#if defined(simde_math_isnan)
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.f64) / sizeof(r_.f64[0])); i++) {
		r_.u64[i] = (simde_math_isnan(a_.f64[i]) ||
			     simde_math_isnan(b_.f64[i]))
				    ? ~UINT64_C(0)
				    : UINT64_C(0);
	}
#else
	HEDLEY_UNREACHABLE();
#endif

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cmpunord_pd(a, b) simde_mm_cmpunord_pd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cmpunord_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cmpunord_sd(a, b);
#elif defined(SIMDE_ASSUME_VECTORIZATION)
	return simde_mm_move_sd(a, simde_mm_cmpunord_pd(a, b));
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a),
				 b_ = simde__m128d_to_private(b);

#if defined(simde_math_isnan)
	r_.u64[0] = (simde_math_isnan(a_.f64[0]) || simde_math_isnan(b_.f64[0]))
			    ? ~UINT64_C(0)
			    : UINT64_C(0);
	r_.u64[1] = a_.u64[1];

#else
	HEDLEY_UNREACHABLE();
#endif

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cmpunord_sd(a, b) simde_mm_cmpunord_sd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cvtepi32_pd(simde__m128i a)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cvtepi32_pd(a);
#else
	simde__m128d_private r_;
	simde__m128i_private a_ = simde__m128i_to_private(a);

#if defined(SIMDE_CONVERT_VECTOR_)
	SIMDE_CONVERT_VECTOR_(r_.f64, a_.m64_private[0].i32);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.f64) / sizeof(r_.f64[0])); i++) {
		r_.f64[i] = (simde_float64)a_.i32[i];
	}
#endif

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cvtepi32_pd(a) simde_mm_cvtepi32_pd(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cvtepi32_ps(simde__m128i a)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cvtepi32_ps(a);
#else
	simde__m128_private r_;
	simde__m128i_private a_ = simde__m128i_to_private(a);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_f32 = vcvtq_f32_s32(a_.neon_i32);
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	r_.altivec_f32 = vec_ctf(a_.altivec_i32, 0);
#elif defined(SIMDE_CONVERT_VECTOR_)
	SIMDE_CONVERT_VECTOR_(r_.f32, a_.i32);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.f32) / sizeof(r_.f32[0])); i++) {
		r_.f32[i] = (simde_float32)a_.i32[i];
	}
#endif

	return simde__m128_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cvtepi32_ps(a) simde_mm_cvtepi32_ps(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_cvtpd_epi32(simde__m128d a)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cvtpd_epi32(a);
#else
	simde__m128i_private r_;
	simde__m128d_private a_ = simde__m128d_to_private(a);

#if defined(SIMDE_CONVERT_VECTOR_)
	SIMDE_CONVERT_VECTOR_(r_.m64_private[0].i32, a_.f64);
	r_.m64_private[1] = simde__m64_to_private(simde_mm_setzero_si64());
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(a_.f64) / sizeof(a_.f64[0])); i++) {
		r_.i32[i] = HEDLEY_STATIC_CAST(int32_t, a_.f64[i]);
	}
	simde_memset(&(r_.m64_private[1]), 0, sizeof(r_.m64_private[1]));
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cvtpd_epi32(a) simde_mm_cvtpd_epi32(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m64 simde_mm_cvtpd_pi32(simde__m128d a)
{
#if defined(SIMDE_X86_SSE2_NATIVE) && defined(SIMDE_X86_MMX_NATIVE)
	return _mm_cvtpd_pi32(a);
#else
	simde__m64_private r_;
	simde__m128d_private a_ = simde__m128d_to_private(a);

#if defined(SIMDE_CONVERT_VECTOR_)
	SIMDE_CONVERT_VECTOR_(r_.i32, a_.f64);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i32) / sizeof(r_.i32[0])); i++) {
		r_.i32[i] = HEDLEY_STATIC_CAST(int32_t, a_.f64[i]);
	}
#endif

	return simde__m64_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cvtpd_pi32(a) simde_mm_cvtpd_pi32(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cvtpd_ps(simde__m128d a)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cvtpd_ps(a);
#else
	simde__m128_private r_;
	simde__m128d_private a_ = simde__m128d_to_private(a);

#if defined(SIMDE_CONVERT_VECTOR_)
	SIMDE_CONVERT_VECTOR_(r_.m64_private[0].f32, a_.f64);
	r_.m64_private[1] = simde__m64_to_private(simde_mm_setzero_si64());
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(a_.f64) / sizeof(a_.f64[0])); i++) {
		r_.f32[i] = (simde_float32)a_.f64[i];
	}
	simde_memset(&(r_.m64_private[1]), 0, sizeof(r_.m64_private[1]));
#endif

	return simde__m128_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cvtpd_ps(a) simde_mm_cvtpd_ps(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cvtpi32_pd(simde__m64 a)
{
#if defined(SIMDE_X86_SSE2_NATIVE) && defined(SIMDE_X86_MMX_NATIVE)
	return _mm_cvtpi32_pd(a);
#else
	simde__m128d_private r_;
	simde__m64_private a_ = simde__m64_to_private(a);

#if defined(SIMDE_CONVERT_VECTOR_)
	SIMDE_CONVERT_VECTOR_(r_.f64, a_.i32);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.f64) / sizeof(r_.f64[0])); i++) {
		r_.f64[i] = (simde_float64)a_.i32[i];
	}
#endif

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cvtpi32_pd(a) simde_mm_cvtpi32_pd(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_cvtps_epi32(simde__m128 a)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cvtps_epi32(a);
#else
	simde__m128i_private r_;
	simde__m128_private a_ = simde__m128_to_private(a);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
/* The default rounding mode on SSE is 'round to even', which ArmV7
     does not support!  It is supported on ARMv8 however. */
#if defined(SIMDE_ARCH_AARCH64)
	r_.neon_i32 = vcvtnq_s32_f32(a_.neon_f32);
#else
	uint32x4_t signmask = vdupq_n_u32(0x80000000);
	float32x4_t half = vbslq_f32(signmask, a_.neon_f32,
				     vdupq_n_f32(0.5f)); /* +/- 0.5 */
	int32x4_t r_normal = vcvtq_s32_f32(
		vaddq_f32(a_.neon_f32, half)); /* round to integer: [a + 0.5]*/
	int32x4_t r_trunc =
		vcvtq_s32_f32(a_.neon_f32); /* truncate to integer: [a] */
	int32x4_t plusone = vshrq_n_s32(vnegq_s32(r_trunc), 31); /* 1 or 0 */
	int32x4_t r_even = vbicq_s32(vaddq_s32(r_trunc, plusone),
				     vdupq_n_s32(1)); /* ([a] + {0,1}) & ~1 */
	float32x4_t delta = vsubq_f32(
		a_.neon_f32,
		vcvtq_f32_s32(r_trunc)); /* compute delta: delta = (a - [a]) */
	uint32x4_t is_delta_half =
		vceqq_f32(delta, half); /* delta == +/- 0.5 */
	r_.neon_i32 = vbslq_s32(is_delta_half, r_even, r_normal);
#endif
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	r_.altivec_i32 = vec_cts(a_.altivec_f32, 0);
#elif defined(SIMDE_CONVERT_VECTOR_)
	SIMDE_CONVERT_VECTOR_(r_.i32, a_.f32);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i32) / sizeof(r_.i32[0])); i++) {
		r_.i32[i] = HEDLEY_STATIC_CAST(int32_t, a_.f32[i]);
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cvtps_epi32(a) simde_mm_cvtps_epi32(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cvtps_pd(simde__m128 a)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cvtps_pd(a);
#else
	simde__m128d_private r_;
	simde__m128_private a_ = simde__m128_to_private(a);

#if defined(SIMDE_CONVERT_VECTOR_)
	SIMDE_CONVERT_VECTOR_(r_.f64, a_.m64_private[0].f32);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.f64) / sizeof(r_.f64[0])); i++) {
		r_.f64[i] = a_.f32[i];
	}
#endif

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cvtps_pd(a) simde_mm_cvtps_pd(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
int32_t simde_mm_cvtsd_si32(simde__m128d a)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cvtsd_si32(a);
#else
	simde__m128d_private a_ = simde__m128d_to_private(a);
	return SIMDE_CONVERT_FTOI(int32_t, a_.f64[0]);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cvtsd_si32(a) simde_mm_cvtsd_si32(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
int64_t simde_mm_cvtsd_si64(simde__m128d a)
{
#if defined(SIMDE_X86_SSE2_NATIVE) && defined(SIMDE_ARCH_AMD64)
#if defined(__PGI)
	return _mm_cvtsd_si64x(a);
#else
	return _mm_cvtsd_si64(a);
#endif
#else
	simde__m128d_private a_ = simde__m128d_to_private(a);
	return SIMDE_CONVERT_FTOI(int64_t, a_.f64[0]);
#endif
}
#define simde_mm_cvtsd_si64x(a) simde_mm_cvtsd_si64(a)
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cvtsd_si64(a) simde_mm_cvtsd_si64(a)
#define _mm_cvtsd_si64x(a) simde_mm_cvtsd_si64x(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128 simde_mm_cvtsd_ss(simde__m128 a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cvtsd_ss(a, b);
#else
	simde__m128_private r_, a_ = simde__m128_to_private(a);
	simde__m128d_private b_ = simde__m128d_to_private(b);

	r_.f32[0] = HEDLEY_STATIC_CAST(simde_float32, b_.f64[0]);

	SIMDE_VECTORIZE
	for (size_t i = 1; i < (sizeof(r_) / sizeof(r_.i32[0])); i++) {
		r_.i32[i] = a_.i32[i];
	}

	return simde__m128_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cvtsd_ss(a, b) simde_mm_cvtsd_ss(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
int32_t simde_mm_cvtsi128_si32(simde__m128i a)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cvtsi128_si32(a);
#else
	simde__m128i_private a_ = simde__m128i_to_private(a);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	return vgetq_lane_s32(a_.neon_i32, 0);
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
#if defined(SIMDE_BUG_GCC_95227)
	(void)a_;
#endif
	return vec_extract(a_.altivec_i32, 0);
#else
	return a_.i32[0];
#endif
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cvtsi128_si32(a) simde_mm_cvtsi128_si32(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
int64_t simde_mm_cvtsi128_si64(simde__m128i a)
{
#if defined(SIMDE_X86_SSE2_NATIVE) && defined(SIMDE_ARCH_AMD64)
#if defined(__PGI)
	return _mm_cvtsi128_si64x(a);
#else
	return _mm_cvtsi128_si64(a);
#endif
#else
	simde__m128i_private a_ = simde__m128i_to_private(a);
#if defined(SIMDE_POWER_ALTIVEC_P5_NATIVE) && !defined(HEDLEY_IBM_VERSION)
	return vec_extract(a_.i64, 0);
#endif
	return a_.i64[0];
#endif
}
#define simde_mm_cvtsi128_si64x(a) simde_mm_cvtsi128_si64(a)
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cvtsi128_si64(a) simde_mm_cvtsi128_si64(a)
#define _mm_cvtsi128_si64x(a) simde_mm_cvtsi128_si64x(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cvtsi32_sd(simde__m128d a, int32_t b)
{

#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cvtsi32_sd(a, b);
#else
	simde__m128d_private r_;
	simde__m128d_private a_ = simde__m128d_to_private(a);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE) && defined(SIMDE_ARCH_AMD64)
	r_.neon_f64 = vsetq_lane_f64((simde_float64)b, a_.neon_f64, 0);
#else
	r_.f64[0] = HEDLEY_STATIC_CAST(simde_float64, b);
	r_.i64[1] = a_.i64[1];
#endif

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cvtsi32_sd(a, b) simde_mm_cvtsi32_sd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_cvtsi32_si128(int32_t a)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cvtsi32_si128(a);
#else
	simde__m128i_private r_;

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i32 = vsetq_lane_s32(a, vdupq_n_s32(0), 0);
#else
	r_.i32[0] = a;
	r_.i32[1] = 0;
	r_.i32[2] = 0;
	r_.i32[3] = 0;
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cvtsi32_si128(a) simde_mm_cvtsi32_si128(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cvtsi64_sd(simde__m128d a, int64_t b)
{
#if defined(SIMDE_X86_SSE2_NATIVE) && defined(SIMDE_ARCH_AMD64)
#if !defined(__PGI)
	return _mm_cvtsi64_sd(a, b);
#else
	return _mm_cvtsi64x_sd(a, b);
#endif
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a);

#if defined(SIMDE_ARM_NEON_A64V8_NATIVE)
	r_.neon_f64 = vsetq_lane_f64((simde_float64)b, a_.neon_f64, 0);
#else
	r_.f64[0] = HEDLEY_STATIC_CAST(simde_float64, b);
	r_.f64[1] = a_.f64[1];
#endif

	return simde__m128d_from_private(r_);
#endif
}
#define simde_mm_cvtsi64x_sd(a, b) simde_mm_cvtsi64_sd(a, b)
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cvtsi64_sd(a, b) simde_mm_cvtsi64_sd(a, b)
#define _mm_cvtsi64x_sd(a, b) simde_mm_cvtsi64x_sd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_cvtsi64_si128(int64_t a)
{
#if defined(SIMDE_X86_SSE2_NATIVE) && defined(SIMDE_ARCH_AMD64)
#if !defined(__PGI)
	return _mm_cvtsi64_si128(a);
#else
	return _mm_cvtsi64x_si128(a);
#endif
#else
	simde__m128i_private r_;

	r_.i64[0] = a;
	r_.i64[1] = 0;

	return simde__m128i_from_private(r_);
#endif
}
#define simde_mm_cvtsi64x_si128(a) simde_mm_cvtsi64_si128(a)
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cvtsi64_si128(a) simde_mm_cvtsi64_si128(a)
#define _mm_cvtsi64x_si128(a) simde_mm_cvtsi64x_si128(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_cvtss_sd(simde__m128d a, simde__m128 b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cvtss_sd(a, b);
#else
	simde__m128d_private a_ = simde__m128d_to_private(a);
	simde__m128_private b_ = simde__m128_to_private(b);

	a_.f64[0] = HEDLEY_STATIC_CAST(simde_float64, b_.f32[0]);

	return simde__m128d_from_private(a_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cvtss_sd(a, b) simde_mm_cvtss_sd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_cvttpd_epi32(simde__m128d a)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cvttpd_epi32(a);
#else
	simde__m128i_private r_;
	simde__m128d_private a_ = simde__m128d_to_private(a);

	for (size_t i = 0; i < (sizeof(a_.f64) / sizeof(a_.f64[0])); i++) {
		r_.i32[i] = SIMDE_CONVERT_FTOI(int32_t, a_.f64[i]);
	}
	simde_memset(&(r_.m64_private[1]), 0, sizeof(r_.m64_private[1]));

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cvttpd_epi32(a) simde_mm_cvttpd_epi32(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m64 simde_mm_cvttpd_pi32(simde__m128d a)
{
#if defined(SIMDE_X86_SSE2_NATIVE) && defined(SIMDE_X86_MMX_NATIVE)
	return _mm_cvttpd_pi32(a);
#else
	simde__m64_private r_;
	simde__m128d_private a_ = simde__m128d_to_private(a);

#if defined(SIMDE_CONVERT_VECTOR_)
	SIMDE_CONVERT_VECTOR_(r_.i32, a_.f64);
#else
	for (size_t i = 0; i < (sizeof(r_.i32) / sizeof(r_.i32[0])); i++) {
		r_.i32[i] = SIMDE_CONVERT_FTOI(int32_t, a_.f64[i]);
	}
#endif

	return simde__m64_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cvttpd_pi32(a) simde_mm_cvttpd_pi32(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_cvttps_epi32(simde__m128 a)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cvttps_epi32(a);
#else
	simde__m128i_private r_;
	simde__m128_private a_ = simde__m128_to_private(a);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i32 = vcvtq_s32_f32(a_.neon_f32);
#elif defined(SIMDE_CONVERT_VECTOR_)
	SIMDE_CONVERT_VECTOR_(r_.i32, a_.f32);
#else
	for (size_t i = 0; i < (sizeof(r_.i32) / sizeof(r_.i32[0])); i++) {
		r_.i32[i] = SIMDE_CONVERT_FTOI(int32_t, a_.f32[i]);
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cvttps_epi32(a) simde_mm_cvttps_epi32(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
int32_t simde_mm_cvttsd_si32(simde__m128d a)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_cvttsd_si32(a);
#else
	simde__m128d_private a_ = simde__m128d_to_private(a);
	return SIMDE_CONVERT_FTOI(int32_t, a_.f64[0]);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cvttsd_si32(a) simde_mm_cvttsd_si32(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
int64_t simde_mm_cvttsd_si64(simde__m128d a)
{
#if defined(SIMDE_X86_SSE2_NATIVE) && defined(SIMDE_ARCH_AMD64)
#if !defined(__PGI)
	return _mm_cvttsd_si64(a);
#else
	return _mm_cvttsd_si64x(a);
#endif
#else
	simde__m128d_private a_ = simde__m128d_to_private(a);
	return SIMDE_CONVERT_FTOI(int64_t, a_.f64[0]);
#endif
}
#define simde_mm_cvttsd_si64x(a) simde_mm_cvttsd_si64(a)
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_cvttsd_si64(a) simde_mm_cvttsd_si64(a)
#define _mm_cvttsd_si64x(a) simde_mm_cvttsd_si64x(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_div_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_div_pd(a, b);
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a),
				 b_ = simde__m128d_to_private(b);

#if defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.f64 = a_.f64 / b_.f64;
#elif defined(SIMDE_WASM_SIMD128_NATIVE)
	r_.wasm_v128 = wasm_f64x2_div(a_.wasm_v128, b_.wasm_v128);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.f64) / sizeof(r_.f64[0])); i++) {
		r_.f64[i] = a_.f64[i] / b_.f64[i];
	}
#endif

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_div_pd(a, b) simde_mm_div_pd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_div_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_div_sd(a, b);
#elif defined(SIMDE_ASSUME_VECTORIZATION)
	return simde_mm_move_sd(a, simde_mm_div_pd(a, b));
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a),
				 b_ = simde__m128d_to_private(b);

	r_.f64[0] = a_.f64[0] / b_.f64[0];
	r_.f64[1] = a_.f64[1];

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_div_sd(a, b) simde_mm_div_sd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
int32_t simde_mm_extract_epi16(simde__m128i a, const int imm8)
	SIMDE_REQUIRE_RANGE(imm8, 0, 7)
{
	uint16_t r;
	simde__m128i_private a_ = simde__m128i_to_private(a);

#if defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
#if defined(SIMDE_BUG_GCC_95227)
	(void)a_;
	(void)imm8;
#endif
	r = vec_extract(a_.altivec_i16, imm8);
#else
	r = a_.u16[imm8 & 7];
#endif

	return HEDLEY_STATIC_CAST(int32_t, r);
}
#if defined(SIMDE_X86_SSE2_NATIVE) && \
	(!defined(HEDLEY_GCC_VERSION) || HEDLEY_GCC_VERSION_CHECK(4, 6, 0))
#define simde_mm_extract_epi16(a, imm8) _mm_extract_epi16(a, imm8)
#elif defined(SIMDE_ARM_NEON_A32V7_NATIVE)
#define simde_mm_extract_epi16(a, imm8)                                        \
	HEDLEY_STATIC_CAST(int32_t,                                            \
			   vgetq_lane_s16(simde__m128i_to_private(a).neon_i16, \
					  (imm8)) &                            \
				   (UINT32_C(0x0000ffff)))
#endif
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_extract_epi16(a, imm8) simde_mm_extract_epi16(a, imm8)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_insert_epi16(simde__m128i a, int16_t i, const int imm8)
	SIMDE_REQUIRE_RANGE(imm8, 0, 7)
{
	simde__m128i_private a_ = simde__m128i_to_private(a);
	a_.i16[imm8 & 7] = i;
	return simde__m128i_from_private(a_);
}
#if defined(SIMDE_X86_SSE2_NATIVE) && !defined(__PGI)
#define simde_mm_insert_epi16(a, i, imm8) _mm_insert_epi16((a), (i), (imm8))
#elif defined(SIMDE_ARM_NEON_A32V7_NATIVE)
#define simde_mm_insert_epi16(a, i, imm8) \
	simde__m128i_from_neon_i16(       \
		vsetq_lane_s16((i), simde__m128i_to_neon_i16(a), (imm8)))
#endif
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_insert_epi16(a, i, imm8) simde_mm_insert_epi16(a, i, imm8)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d
simde_mm_load_pd(simde_float64 const mem_addr[HEDLEY_ARRAY_PARAM(2)])
{
	simde_assert_aligned(16, mem_addr);

#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_load_pd(mem_addr);
#else
	simde__m128d_private r_;

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_u32 =
		vld1q_u32(HEDLEY_REINTERPRET_CAST(uint32_t const *, mem_addr));
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE) && !defined(HEDLEY_IBM_VERSION)
	r_.altivec_f64 = vec_ld(
		0, HEDLEY_REINTERPRET_CAST(SIMDE_POWER_ALTIVEC_VECTOR(double)
						   const *,
					   mem_addr));
#else
	r_ = *SIMDE_ALIGN_CAST(simde__m128d_private const *, mem_addr);
#endif

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_load_pd(mem_addr) simde_mm_load_pd(mem_addr)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_load_pd1(simde_float64 const *mem_addr)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_load1_pd(mem_addr);
#else
	simde__m128d_private r_;

	r_.f64[0] = *mem_addr;
	r_.f64[1] = *mem_addr;

	return simde__m128d_from_private(r_);
#endif
}
#define simde_mm_load1_pd(mem_addr) simde_mm_load_pd1(mem_addr)
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_load_pd1(mem_addr) simde_mm_load_pd1(mem_addr)
#define _mm_load1_pd(mem_addr) simde_mm_load1_pd(mem_addr)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_load_sd(simde_float64 const *mem_addr)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_load_sd(mem_addr);
#else
	simde__m128d_private r_;

#if defined(SIMDE_ARM_NEON_A64V8_NATIVE)
	r_.neon_f64 = vsetq_lane_f64(*mem_addr, vdupq_n_f64(0), 0);
#else
	r_.f64[0] = *mem_addr;
	r_.u64[1] = UINT64_C(0);
#endif

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_load_sd(mem_addr) simde_mm_load_sd(mem_addr)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_load_si128(simde__m128i const *mem_addr)
{
	simde_assert_aligned(16, mem_addr);

#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_load_si128(
		HEDLEY_REINTERPRET_CAST(__m128i const *, mem_addr));
#elif defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	simde__m128i_private r_;

#if defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	r_.altivec_i32 = vec_ld(
		0, HEDLEY_REINTERPRET_CAST(
			   SIMDE_POWER_ALTIVEC_VECTOR(int) const *, mem_addr));
#else
	r_.neon_i32 = vld1q_s32((int32_t const *)mem_addr);
#endif

	return simde__m128i_from_private(r_);
#else
	return *mem_addr;
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_load_si128(mem_addr) simde_mm_load_si128(mem_addr)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_loadh_pd(simde__m128d a, simde_float64 const *mem_addr)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_loadh_pd(a, mem_addr);
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a);
	simde_float64 t;

	simde_memcpy(&t, mem_addr, sizeof(t));
	r_.f64[0] = a_.f64[0];
	r_.f64[1] = t;

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_loadh_pd(a, mem_addr) simde_mm_loadh_pd(a, mem_addr)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_loadl_epi64(simde__m128i const *mem_addr)
{
	simde_assert_aligned(16, mem_addr);

#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_loadl_epi64(
		HEDLEY_REINTERPRET_CAST(__m128i const *, mem_addr));
#else
	simde__m128i_private r_;

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i32 = vcombine_s32(vld1_s32((int32_t const *)mem_addr),
				   vcreate_s32(0));
#else
	r_.i64[0] = *HEDLEY_REINTERPRET_CAST(int64_t const *, mem_addr);
	r_.i64[1] = 0;
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_loadl_epi64(mem_addr) simde_mm_loadl_epi64(mem_addr)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_loadl_pd(simde__m128d a, simde_float64 const *mem_addr)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_loadl_pd(a, mem_addr);
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a);

	r_.f64[0] = *mem_addr;
	r_.u64[1] = a_.u64[1];

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_loadl_pd(a, mem_addr) simde_mm_loadl_pd(a, mem_addr)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d
simde_mm_loadr_pd(simde_float64 const mem_addr[HEDLEY_ARRAY_PARAM(2)])
{
	simde_assert_aligned(16, mem_addr);

#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_loadr_pd(mem_addr);
#else
	simde__m128d_private r_;

	r_.f64[0] = mem_addr[1];
	r_.f64[1] = mem_addr[0];

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_loadr_pd(mem_addr) simde_mm_loadr_pd(mem_addr)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d
simde_mm_loadu_pd(simde_float64 const mem_addr[HEDLEY_ARRAY_PARAM(2)])
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_loadu_pd(mem_addr);
#else
	simde__m128d_private r_;

	simde_memcpy(&r_, mem_addr, sizeof(r_));

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_loadu_pd(mem_addr) simde_mm_loadu_pd(mem_addr)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_loadu_si128(simde__m128i const *mem_addr)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_loadu_si128(HEDLEY_STATIC_CAST(__m128i const *, mem_addr));
#else
	simde__m128i_private r_;

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i32 = vld1q_s32((int32_t const *)mem_addr);
#else
	simde_memcpy(&r_, mem_addr, sizeof(r_));
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_loadu_si128(mem_addr) simde_mm_loadu_si128(mem_addr)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_madd_epi16(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_madd_epi16(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	int32x4_t pl =
		vmull_s16(vget_low_s16(a_.neon_i16), vget_low_s16(b_.neon_i16));
	int32x4_t ph = vmull_s16(vget_high_s16(a_.neon_i16),
				 vget_high_s16(b_.neon_i16));
	int32x2_t rl = vpadd_s32(vget_low_s32(pl), vget_high_s32(pl));
	int32x2_t rh = vpadd_s32(vget_low_s32(ph), vget_high_s32(ph));
	r_.neon_i32 = vcombine_s32(rl, rh);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_) / sizeof(r_.i16[0])); i += 2) {
		r_.i32[i / 2] = (a_.i16[i] * b_.i16[i]) +
				(a_.i16[i + 1] * b_.i16[i + 1]);
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_madd_epi16(a, b) simde_mm_madd_epi16(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
void simde_mm_maskmoveu_si128(simde__m128i a, simde__m128i mask,
			      int8_t mem_addr[HEDLEY_ARRAY_PARAM(16)])
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	_mm_maskmoveu_si128(a, mask, HEDLEY_REINTERPRET_CAST(char *, mem_addr));
#else
	simde__m128i_private a_ = simde__m128i_to_private(a),
			     mask_ = simde__m128i_to_private(mask);

	for (size_t i = 0; i < (sizeof(a_.i8) / sizeof(a_.i8[0])); i++) {
		if (mask_.u8[i] & 0x80) {
			mem_addr[i] = a_.i8[i];
		}
	}
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_maskmoveu_si128(a, mask, mem_addr) \
	simde_mm_maskmoveu_si128(              \
		(a), (mask),                   \
		SIMDE_CHECKED_REINTERPRET_CAST(int8_t *, char *, (mem_addr)))
#endif

SIMDE_FUNCTION_ATTRIBUTES
int32_t simde_mm_movemask_epi8(simde__m128i a)
{
#if defined(SIMDE_X86_SSE2_NATIVE) && !defined(__INTEL_COMPILER)
	/* ICC has trouble with _mm_movemask_epi8 at -O2 and above: */
	return _mm_movemask_epi8(a);
#else
	int32_t r = 0;
	simde__m128i_private a_ = simde__m128i_to_private(a);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	uint8x16_t input = a_.neon_u8;
	SIMDE_ALIGN_AS(16, int8x8_t)
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

	r = ((hi[0] << 8) | (lo[0] & 0xFF));
#elif defined(SIMDE_POWER_ALTIVEC_P8_NATIVE) && !defined(HEDLEY_IBM_VERSION)
	static const SIMDE_POWER_ALTIVEC_VECTOR(unsigned char)
		perm = {120, 112, 104, 96, 88, 80, 72, 64,
			56,  48,  40,  32, 24, 16, 8,  0};
	r = HEDLEY_STATIC_CAST(
		int32_t, vec_extract(vec_vbpermq(a_.altivec_u8, perm), 1));
#else
	SIMDE_VECTORIZE_REDUCTION(| : r)
	for (size_t i = 0; i < (sizeof(a_.u8) / sizeof(a_.u8[0])); i++) {
		r |= (a_.u8[15 - i] >> 7) << (15 - i);
	}
#endif

	return r;
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_movemask_epi8(a) simde_mm_movemask_epi8(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
int32_t simde_mm_movemask_pd(simde__m128d a)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_movemask_pd(a);
#else
	int32_t r = 0;
	simde__m128d_private a_ = simde__m128d_to_private(a);

	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(a_.u64) / sizeof(a_.u64[0])); i++) {
		r |= (a_.u64[i] >> 63) << i;
	}

	return r;
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_movemask_pd(a) simde_mm_movemask_pd(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m64 simde_mm_movepi64_pi64(simde__m128i a)
{
#if defined(SIMDE_X86_SSE2_NATIVE) && defined(SIMDE_X86_MMX_NATIVE)
	return _mm_movepi64_pi64(a);
#else
	simde__m64_private r_;
	simde__m128i_private a_ = simde__m128i_to_private(a);

	r_.i64[0] = a_.i64[0];

	return simde__m64_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_movepi64_pi64(a) simde_mm_movepi64_pi64(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_movpi64_epi64(simde__m64 a)
{
#if defined(SIMDE_X86_SSE2_NATIVE) && defined(SIMDE_X86_MMX_NATIVE)
	return _mm_movpi64_epi64(a);
#else
	simde__m128i_private r_;
	simde__m64_private a_ = simde__m64_to_private(a);

	r_.i64[0] = a_.i64[0];
	r_.i64[1] = 0;

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_movpi64_epi64(a) simde_mm_movpi64_epi64(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_min_epi16(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_min_epi16(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i16 = vminq_s16(a_.neon_i16, b_.neon_i16);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i16) / sizeof(r_.i16[0])); i++) {
		r_.i16[i] = (a_.i16[i] < b_.i16[i]) ? a_.i16[i] : b_.i16[i];
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_min_epi16(a, b) simde_mm_min_epi16(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_min_epu8(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_min_epu8(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_u8 = vminq_u8(a_.neon_u8, b_.neon_u8);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.u8) / sizeof(r_.u8[0])); i++) {
		r_.u8[i] = (a_.u8[i] < b_.u8[i]) ? a_.u8[i] : b_.u8[i];
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_min_epu8(a, b) simde_mm_min_epu8(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_min_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_min_pd(a, b);
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a),
				 b_ = simde__m128d_to_private(b);

	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.f64) / sizeof(r_.f64[0])); i++) {
		r_.f64[i] = (a_.f64[i] < b_.f64[i]) ? a_.f64[i] : b_.f64[i];
	}

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_min_pd(a, b) simde_mm_min_pd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_min_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_min_sd(a, b);
#elif defined(SIMDE_ASSUME_VECTORIZATION)
	return simde_mm_move_sd(a, simde_mm_min_pd(a, b));
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a),
				 b_ = simde__m128d_to_private(b);

	r_.f64[0] = (a_.f64[0] < b_.f64[0]) ? a_.f64[0] : b_.f64[0];
	r_.f64[1] = a_.f64[1];

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_min_sd(a, b) simde_mm_min_sd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_max_epi16(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_max_epi16(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i16 = vmaxq_s16(a_.neon_i16, b_.neon_i16);
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	r_.altivec_i16 = vec_max(a_.altivec_i16, b_.altivec_i16);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i16) / sizeof(r_.i16[0])); i++) {
		r_.i16[i] = (a_.i16[i] > b_.i16[i]) ? a_.i16[i] : b_.i16[i];
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_max_epi16(a, b) simde_mm_max_epi16(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_max_epu8(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_max_epu8(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_u8 = vmaxq_u8(a_.neon_u8, b_.neon_u8);
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	r_.altivec_u8 = vec_max(a_.altivec_u8, b_.altivec_u8);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.u8) / sizeof(r_.u8[0])); i++) {
		r_.u8[i] = (a_.u8[i] > b_.u8[i]) ? a_.u8[i] : b_.u8[i];
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_max_epu8(a, b) simde_mm_max_epu8(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_max_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_max_pd(a, b);
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a),
				 b_ = simde__m128d_to_private(b);

#if defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	r_.altivec_f64 = vec_max(a_.altivec_f64, b_.altivec_f64);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.f64) / sizeof(r_.f64[0])); i++) {
		r_.f64[i] = (a_.f64[i] > b_.f64[i]) ? a_.f64[i] : b_.f64[i];
	}
#endif

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_max_pd(a, b) simde_mm_max_pd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_max_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_max_sd(a, b);
#elif defined(SIMDE_ASSUME_VECTORIZATION)
	return simde_mm_move_sd(a, simde_mm_max_pd(a, b));
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a),
				 b_ = simde__m128d_to_private(b);

	r_.f64[0] = (a_.f64[0] > b_.f64[0]) ? a_.f64[0] : b_.f64[0];
	r_.f64[1] = a_.f64[1];

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_max_sd(a, b) simde_mm_max_sd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_move_epi64(simde__m128i a)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_move_epi64(a);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i64 = vsetq_lane_s64(0, a_.neon_i64, 1);
#else
	r_.i64[0] = a_.i64[0];
	r_.i64[1] = 0;
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_move_epi64(a) simde_mm_move_epi64(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_mul_epu32(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_mul_epu32(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.u64) / sizeof(r_.u64[0])); i++) {
		r_.u64[i] = HEDLEY_STATIC_CAST(uint64_t, a_.u32[i * 2]) *
			    HEDLEY_STATIC_CAST(uint64_t, b_.u32[i * 2]);
	}

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_mul_epu32(a, b) simde_mm_mul_epu32(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_x_mm_mul_epi64(simde__m128i a, simde__m128i b)
{
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.i64 = a_.i64 * b_.i64;
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i64) / sizeof(r_.i64[0])); i++) {
		r_.i64[i] = a_.i64[i] * b_.i64[i];
	}
#endif

	return simde__m128i_from_private(r_);
}

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_x_mm_mod_epi64(simde__m128i a, simde__m128i b)
{
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.i64 = a_.i64 % b_.i64;
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i64) / sizeof(r_.i64[0])); i++) {
		r_.i64[i] = a_.i64[i] % b_.i64[i];
	}
#endif

	return simde__m128i_from_private(r_);
}

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_mul_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_mul_pd(a, b);
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a),
				 b_ = simde__m128d_to_private(b);

#if defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.f64 = a_.f64 * b_.f64;
#elif defined(SIMDE_WASM_SIMD128_NATIVE)
	r_.wasm_v128 = wasm_f64x2_mul(a_.wasm_v128, b_.wasm_v128);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.f64) / sizeof(r_.f64[0])); i++) {
		r_.f64[i] = a_.f64[i] * b_.f64[i];
	}
#endif

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_mul_pd(a, b) simde_mm_mul_pd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_mul_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_mul_sd(a, b);
#elif defined(SIMDE_ASSUME_VECTORIZATION)
	return simde_mm_move_sd(a, simde_mm_mul_pd(a, b));
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a),
				 b_ = simde__m128d_to_private(b);

	r_.f64[0] = a_.f64[0] * b_.f64[0];
	r_.f64[1] = a_.f64[1];

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_mul_sd(a, b) simde_mm_mul_sd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m64 simde_mm_mul_su32(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_X86_SSE2_NATIVE) && defined(SIMDE_X86_MMX_NATIVE) && \
	!defined(__PGI)
	return _mm_mul_su32(a, b);
#else
	simde__m64_private r_, a_ = simde__m64_to_private(a),
			       b_ = simde__m64_to_private(b);

	r_.u64[0] = HEDLEY_STATIC_CAST(uint64_t, a_.u32[0]) *
		    HEDLEY_STATIC_CAST(uint64_t, b_.u32[0]);

	return simde__m64_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_mul_su32(a, b) simde_mm_mul_su32(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_mulhi_epi16(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_mulhi_epi16(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	int16x4_t a3210 = vget_low_s16(a_.neon_i16);
	int16x4_t b3210 = vget_low_s16(b_.neon_i16);
	int32x4_t ab3210 = vmull_s16(a3210, b3210); /* 3333222211110000 */
	int16x4_t a7654 = vget_high_s16(a_.neon_i16);
	int16x4_t b7654 = vget_high_s16(b_.neon_i16);
	int32x4_t ab7654 = vmull_s16(a7654, b7654); /* 7777666655554444 */
	uint16x8x2_t rv = vuzpq_u16(vreinterpretq_u16_s32(ab3210),
				    vreinterpretq_u16_s32(ab7654));
	r_.neon_u16 = rv.val[1];
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i16) / sizeof(r_.i16[0])); i++) {
		r_.u16[i] = HEDLEY_STATIC_CAST(
			uint16_t,
			(HEDLEY_STATIC_CAST(
				 uint32_t,
				 HEDLEY_STATIC_CAST(int32_t, a_.i16[i]) *
					 HEDLEY_STATIC_CAST(int32_t,
							    b_.i16[i])) >>
			 16));
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_mulhi_epi16(a, b) simde_mm_mulhi_epi16(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_mulhi_epu16(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE) && !defined(__PGI)
	return _mm_mulhi_epu16(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.u16) / sizeof(r_.u16[0])); i++) {
		r_.u16[i] = HEDLEY_STATIC_CAST(
			uint16_t,
			HEDLEY_STATIC_CAST(uint32_t, a_.u16[i]) *
					HEDLEY_STATIC_CAST(uint32_t,
							   b_.u16[i]) >>
				16);
	}

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_mulhi_epu16(a, b) simde_mm_mulhi_epu16(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_mullo_epi16(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_mullo_epi16(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i16 = vmulq_s16(a_.neon_i16, b_.neon_i16);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i16) / sizeof(r_.i16[0])); i++) {
		r_.u16[i] = HEDLEY_STATIC_CAST(
			uint16_t,
			HEDLEY_STATIC_CAST(uint32_t, a_.u16[i]) *
				HEDLEY_STATIC_CAST(uint32_t, b_.u16[i]));
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_mullo_epi16(a, b) simde_mm_mullo_epi16(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_or_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_or_pd(a, b);
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a),
				 b_ = simde__m128d_to_private(b);

#if defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.i32f = a_.i32f | b_.i32f;
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i32f) / sizeof(r_.i32f[0])); i++) {
		r_.i32f[i] = a_.i32f[i] | b_.i32f[i];
	}
#endif

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_or_pd(a, b) simde_mm_or_pd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_or_si128(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_or_si128(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i32 = vorrq_s32(a_.neon_i32, b_.neon_i32);
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	r_.altivec_i32 = vec_or(a_.altivec_i32, b_.altivec_i32);
#elif defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.i32f = a_.i32f | b_.i32f;
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i32f) / sizeof(r_.i32f[0])); i++) {
		r_.i32f[i] = a_.i32f[i] | b_.i32f[i];
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_or_si128(a, b) simde_mm_or_si128(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_packs_epi16(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_packs_epi16(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i8 =
		vcombine_s8(vqmovn_s16(a_.neon_i16), vqmovn_s16(b_.neon_i16));
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i16) / sizeof(r_.i16[0])); i++) {
		r_.i8[i] = (a_.i16[i] > INT8_MAX)
				   ? INT8_MAX
				   : ((a_.i16[i] < INT8_MIN)
					      ? INT8_MIN
					      : HEDLEY_STATIC_CAST(int8_t,
								   a_.i16[i]));
		r_.i8[i + 8] = (b_.i16[i] > INT8_MAX)
				       ? INT8_MAX
				       : ((b_.i16[i] < INT8_MIN)
						  ? INT8_MIN
						  : HEDLEY_STATIC_CAST(
							    int8_t, b_.i16[i]));
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_packs_epi16(a, b) simde_mm_packs_epi16(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_packs_epi32(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_packs_epi32(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i16 =
		vcombine_s16(vqmovn_s32(a_.neon_i32), vqmovn_s32(b_.neon_i32));
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	r_.altivec_i16 = vec_packs(a_.altivec_i32, b_.altivec_i32);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i32) / sizeof(r_.i32[0])); i++) {
		r_.i16[i] = (a_.i32[i] > INT16_MAX)
				    ? INT16_MAX
				    : ((a_.i32[i] < INT16_MIN)
					       ? INT16_MIN
					       : HEDLEY_STATIC_CAST(int16_t,
								    a_.i32[i]));
		r_.i16[i + 4] =
			(b_.i32[i] > INT16_MAX)
				? INT16_MAX
				: ((b_.i32[i] < INT16_MIN)
					   ? INT16_MIN
					   : HEDLEY_STATIC_CAST(int16_t,
								b_.i32[i]));
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_packs_epi32(a, b) simde_mm_packs_epi32(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_packus_epi16(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_packus_epi16(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_u8 =
		vcombine_u8(vqmovun_s16(a_.neon_i16), vqmovun_s16(b_.neon_i16));
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	r_.altivec_u8 = vec_packsu(a_.altivec_i16, b_.altivec_i16);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i16) / sizeof(r_.i16[0])); i++) {
		r_.u8[i] = (a_.i16[i] > UINT8_MAX)
				   ? UINT8_MAX
				   : ((a_.i16[i] < 0)
					      ? UINT8_C(0)
					      : HEDLEY_STATIC_CAST(uint8_t,
								   a_.i16[i]));
		r_.u8[i + 8] =
			(b_.i16[i] > UINT8_MAX)
				? UINT8_MAX
				: ((b_.i16[i] < 0)
					   ? UINT8_C(0)
					   : HEDLEY_STATIC_CAST(uint8_t,
								b_.i16[i]));
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_packus_epi16(a, b) simde_mm_packus_epi16(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
void simde_mm_pause(void)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	_mm_pause();
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_pause() (simde_mm_pause())
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_sad_epu8(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_sad_epu8(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

	for (size_t i = 0; i < (sizeof(r_.i64) / sizeof(r_.i64[0])); i++) {
		uint16_t tmp = 0;
		SIMDE_VECTORIZE_REDUCTION(+ : tmp)
		for (size_t j = 0; j < ((sizeof(r_.u8) / sizeof(r_.u8[0])) / 2);
		     j++) {
			const size_t e = j + (i * 8);
			tmp += (a_.u8[e] > b_.u8[e]) ? (a_.u8[e] - b_.u8[e])
						     : (b_.u8[e] - a_.u8[e]);
		}
		r_.i64[i] = tmp;
	}

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_sad_epu8(a, b) simde_mm_sad_epu8(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_set_epi8(int8_t e15, int8_t e14, int8_t e13, int8_t e12,
			       int8_t e11, int8_t e10, int8_t e9, int8_t e8,
			       int8_t e7, int8_t e6, int8_t e5, int8_t e4,
			       int8_t e3, int8_t e2, int8_t e1, int8_t e0)
{

#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_set_epi8(e15, e14, e13, e12, e11, e10, e9, e8, e7, e6, e5,
			    e4, e3, e2, e1, e0);
#else
	simde__m128i_private r_;

#if defined(SIMDE_WASM_SIMD128_NATIVE)
	r_.wasm_v128 = wasm_i8x16_make(e0, e1, e2, e3, e4, e5, e6, e7, e8, e9,
				       e10, e11, e12, e13, e14, e15);
#else
	r_.i8[0] = e0;
	r_.i8[1] = e1;
	r_.i8[2] = e2;
	r_.i8[3] = e3;
	r_.i8[4] = e4;
	r_.i8[5] = e5;
	r_.i8[6] = e6;
	r_.i8[7] = e7;
	r_.i8[8] = e8;
	r_.i8[9] = e9;
	r_.i8[10] = e10;
	r_.i8[11] = e11;
	r_.i8[12] = e12;
	r_.i8[13] = e13;
	r_.i8[14] = e14;
	r_.i8[15] = e15;
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_set_epi8(e15, e14, e13, e12, e11, e10, e9, e8, e7, e6, e5, e4, e3, \
		     e2, e1, e0)                                               \
	simde_mm_set_epi8(e15, e14, e13, e12, e11, e10, e9, e8, e7, e6, e5,    \
			  e4, e3, e2, e1, e0)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_set_epi16(int16_t e7, int16_t e6, int16_t e5, int16_t e4,
				int16_t e3, int16_t e2, int16_t e1, int16_t e0)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_set_epi16(e7, e6, e5, e4, e3, e2, e1, e0);
#else
	simde__m128i_private r_;

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	SIMDE_ALIGN_AS(16, int16x8_t)
	int16_t data[8] = {e0, e1, e2, e3, e4, e5, e6, e7};
	r_.neon_i16 = vld1q_s16(data);
#elif defined(SIMDE_WASM_SIMD128_NATIVE)
	r_.wasm_v128 = wasm_i16x8_make(e0, e1, e2, e3, e4, e5, e6, e7);
#else
	r_.i16[0] = e0;
	r_.i16[1] = e1;
	r_.i16[2] = e2;
	r_.i16[3] = e3;
	r_.i16[4] = e4;
	r_.i16[5] = e5;
	r_.i16[6] = e6;
	r_.i16[7] = e7;
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_set_epi16(e7, e6, e5, e4, e3, e2, e1, e0) \
	simde_mm_set_epi16(e7, e6, e5, e4, e3, e2, e1, e0)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_set_epi32(int32_t e3, int32_t e2, int32_t e1, int32_t e0)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_set_epi32(e3, e2, e1, e0);
#else
	simde__m128i_private r_;

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	SIMDE_ALIGN_AS(16, int32x4_t) int32_t data[4] = {e0, e1, e2, e3};
	r_.neon_i32 = vld1q_s32(data);
#elif defined(SIMDE_WASM_SIMD128_NATIVE)
	r_.wasm_v128 = wasm_i32x4_make(e0, e1, e2, e3);
#else
	r_.i32[0] = e0;
	r_.i32[1] = e1;
	r_.i32[2] = e2;
	r_.i32[3] = e3;
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_set_epi32(e3, e2, e1, e0) simde_mm_set_epi32(e3, e2, e1, e0)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_set_epi64(simde__m64 e1, simde__m64 e0)
{
#if defined(SIMDE_X86_SSE2_NATIVE) && defined(SIMDE_X86_MMX_NATIVE)
	return _mm_set_epi64(e1, e0);
#else
	simde__m128i_private r_;

	r_.m64_private[0] = simde__m64_to_private(e0);
	r_.m64_private[1] = simde__m64_to_private(e1);

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_set_epi64(e1, e0) (simde_mm_set_epi64((e1), (e0)))
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_set_epi64x(int64_t e1, int64_t e0)
{
#if defined(SIMDE_X86_SSE2_NATIVE) && \
	(!defined(HEDLEY_MSVC_VERSION) || HEDLEY_MSVC_VERSION_CHECK(19, 0, 0))
	return _mm_set_epi64x(e1, e0);
#else
	simde__m128i_private r_;

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i64 = vcombine_s64(vdup_n_s64(e0), vdup_n_s64(e1));
#elif defined(SIMDE_WASM_SIMD128_NATIVE)
	r_.wasm_v128 = wasm_i64x2_make(e0, e1);
#else
	r_.i64[0] = e0;
	r_.i64[1] = e1;
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_set_epi64x(e1, e0) simde_mm_set_epi64x(e1, e0)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_x_mm_set_epu8(uint8_t e15, uint8_t e14, uint8_t e13,
				 uint8_t e12, uint8_t e11, uint8_t e10,
				 uint8_t e9, uint8_t e8, uint8_t e7, uint8_t e6,
				 uint8_t e5, uint8_t e4, uint8_t e3, uint8_t e2,
				 uint8_t e1, uint8_t e0)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_set_epi8(
		HEDLEY_STATIC_CAST(char, e15), HEDLEY_STATIC_CAST(char, e14),
		HEDLEY_STATIC_CAST(char, e13), HEDLEY_STATIC_CAST(char, e12),
		HEDLEY_STATIC_CAST(char, e11), HEDLEY_STATIC_CAST(char, e10),
		HEDLEY_STATIC_CAST(char, e9), HEDLEY_STATIC_CAST(char, e8),
		HEDLEY_STATIC_CAST(char, e7), HEDLEY_STATIC_CAST(char, e6),
		HEDLEY_STATIC_CAST(char, e5), HEDLEY_STATIC_CAST(char, e4),
		HEDLEY_STATIC_CAST(char, e3), HEDLEY_STATIC_CAST(char, e2),
		HEDLEY_STATIC_CAST(char, e1), HEDLEY_STATIC_CAST(char, e0));
#else
	simde__m128i_private r_;

	r_.u8[0] = e0;
	r_.u8[1] = e1;
	r_.u8[2] = e2;
	r_.u8[3] = e3;
	r_.u8[4] = e4;
	r_.u8[5] = e5;
	r_.u8[6] = e6;
	r_.u8[7] = e7;
	r_.u8[8] = e8;
	r_.u8[9] = e9;
	r_.u8[10] = e10;
	r_.u8[11] = e11;
	r_.u8[12] = e12;
	r_.u8[13] = e13;
	r_.u8[14] = e14;
	r_.u8[15] = e15;

	return simde__m128i_from_private(r_);
#endif
}

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_x_mm_set_epu16(uint16_t e7, uint16_t e6, uint16_t e5,
				  uint16_t e4, uint16_t e3, uint16_t e2,
				  uint16_t e1, uint16_t e0)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_set_epi16(
		HEDLEY_STATIC_CAST(short, e7), HEDLEY_STATIC_CAST(short, e6),
		HEDLEY_STATIC_CAST(short, e5), HEDLEY_STATIC_CAST(short, e4),
		HEDLEY_STATIC_CAST(short, e3), HEDLEY_STATIC_CAST(short, e2),
		HEDLEY_STATIC_CAST(short, e1), HEDLEY_STATIC_CAST(short, e0));
#else
	simde__m128i_private r_;

	r_.u16[0] = e0;
	r_.u16[1] = e1;
	r_.u16[2] = e2;
	r_.u16[3] = e3;
	r_.u16[4] = e4;
	r_.u16[5] = e5;
	r_.u16[6] = e6;
	r_.u16[7] = e7;

	return simde__m128i_from_private(r_);
#endif
}

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_x_mm_set_epu32(uint32_t e3, uint32_t e2, uint32_t e1,
				  uint32_t e0)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_set_epi32(HEDLEY_STATIC_CAST(int, e3),
			     HEDLEY_STATIC_CAST(int, e2),
			     HEDLEY_STATIC_CAST(int, e1),
			     HEDLEY_STATIC_CAST(int, e0));
#else
	simde__m128i_private r_;

	r_.u32[0] = e0;
	r_.u32[1] = e1;
	r_.u32[2] = e2;
	r_.u32[3] = e3;

	return simde__m128i_from_private(r_);
#endif
}

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_x_mm_set_epu64x(uint64_t e1, uint64_t e0)
{
#if defined(SIMDE_X86_SSE2_NATIVE) && \
	(!defined(HEDLEY_MSVC_VERSION) || HEDLEY_MSVC_VERSION_CHECK(19, 0, 0))
	return _mm_set_epi64x(HEDLEY_STATIC_CAST(int64_t, e1),
			      HEDLEY_STATIC_CAST(int64_t, e0));
#else
	simde__m128i_private r_;

	r_.u64[0] = e0;
	r_.u64[1] = e1;

	return simde__m128i_from_private(r_);
#endif
}

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_set_pd(simde_float64 e1, simde_float64 e0)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_set_pd(e1, e0);
#else
	simde__m128d_private r_;

#if defined(SIMDE_WASM_SIMD128_NATIVE)
	r_.wasm_v128 = wasm_f64x2_make(e0, e1);
#elif defined(SIMDE_WASM_SIMD128_NATIVE)
	r_.wasm_v128 = wasm_f64x2_make(e0, e1);
#else
	r_.f64[0] = e0;
	r_.f64[1] = e1;
#endif

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_set_pd(e1, e0) simde_mm_set_pd(e1, e0)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_set_pd1(simde_float64 a)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_set1_pd(a);
#else
	simde__m128d_private r_;

	r_.f64[0] = a;
	r_.f64[1] = a;

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_set_pd1(a) simde_mm_set1_pd(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_set_sd(simde_float64 a)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_set_sd(a);
#elif defined(SIMDE_ARM_NEON_A64V8_NATIVE)
	return vsetq_lane_f64(a, vdupq_n_f64(SIMDE_FLOAT32_C(0.0)), 0);
#else
	return simde_mm_set_pd(SIMDE_FLOAT64_C(0.0), a);

#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_set_sd(a) simde_mm_set_sd(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_set1_epi8(int8_t a)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_set1_epi8(a);
#else
	simde__m128i_private r_;

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i8 = vdupq_n_s8(a);
#elif defined(SIMDE_WASM_SIMD128_NATIVE)
	r_.wasm_v128 = wasm_i8x16_splat(a);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i8) / sizeof(r_.i8[0])); i++) {
		r_.i8[i] = a;
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_set1_epi8(a) simde_mm_set1_epi8(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_set1_epi16(int16_t a)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_set1_epi16(a);
#else
	simde__m128i_private r_;

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i16 = vdupq_n_s16(a);
#elif defined(SIMDE_WASM_SIMD128_NATIVE)
	r_.wasm_v128 = wasm_i16x8_splat(a);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i16) / sizeof(r_.i16[0])); i++) {
		r_.i16[i] = a;
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_set1_epi16(a) simde_mm_set1_epi16(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_set1_epi32(int32_t a)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_set1_epi32(a);
#else
	simde__m128i_private r_;

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i32 = vdupq_n_s32(a);
#elif defined(SIMDE_WASM_SIMD128_NATIVE)
	r_.wasm_v128 = wasm_i32x4_splat(a);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i32) / sizeof(r_.i32[0])); i++) {
		r_.i32[i] = a;
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_set1_epi32(a) simde_mm_set1_epi32(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_set1_epi64x(int64_t a)
{
#if defined(SIMDE_X86_SSE2_NATIVE) && \
	(!defined(HEDLEY_MSVC_VERSION) || HEDLEY_MSVC_VERSION_CHECK(19, 0, 0))
	return _mm_set1_epi64x(a);
#else
	simde__m128i_private r_;

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i64 = vmovq_n_s64(a);
#elif defined(SIMDE_WASM_SIMD128_NATIVE)
	r_.wasm_v128 = wasm_i64x2_splat(a);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i64) / sizeof(r_.i64[0])); i++) {
		r_.i64[i] = a;
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_set1_epi64x(a) simde_mm_set1_epi64x(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_set1_epi64(simde__m64 a)
{
#if defined(SIMDE_X86_SSE2_NATIVE) && defined(SIMDE_X86_MMX_NATIVE)
	return _mm_set1_epi64(a);
#else
	simde__m64_private a_ = simde__m64_to_private(a);
	return simde_mm_set1_epi64x(a_.i64[0]);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_set1_epi64(a) simde_mm_set1_epi64(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_x_mm_set1_epu8(uint8_t value)
{
	return simde_mm_set1_epi8(HEDLEY_STATIC_CAST(int8_t, value));
}

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_x_mm_set1_epu16(uint16_t value)
{
	return simde_mm_set1_epi16(HEDLEY_STATIC_CAST(int16_t, value));
}

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_x_mm_set1_epu32(uint32_t value)
{
	return simde_mm_set1_epi32(HEDLEY_STATIC_CAST(int32_t, value));
}

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_x_mm_set1_epu64(uint64_t value)
{
	return simde_mm_set1_epi64x(HEDLEY_STATIC_CAST(int64_t, value));
}

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_set1_pd(simde_float64 a)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_set1_pd(a);
#else
	simde__m128d_private r_;

#if defined(SIMDE_WASM_SIMD128_NATIVE)
	r_.wasm_v128 = wasm_f64x2_splat(a);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i64) / sizeof(r_.i64[0])); i++) {
		r_.f64[i] = a;
	}
#endif

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_set1_pd(a) simde_mm_set1_pd(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_setr_epi8(int8_t e15, int8_t e14, int8_t e13, int8_t e12,
				int8_t e11, int8_t e10, int8_t e9, int8_t e8,
				int8_t e7, int8_t e6, int8_t e5, int8_t e4,
				int8_t e3, int8_t e2, int8_t e1, int8_t e0)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_setr_epi8(e15, e14, e13, e12, e11, e10, e9, e8, e7, e6, e5,
			     e4, e3, e2, e1, e0);
#else
	return simde_mm_set_epi8(e0, e1, e2, e3, e4, e5, e6, e7, e8, e9, e10,
				 e11, e12, e13, e14, e15);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_setr_epi8(e15, e14, e13, e12, e11, e10, e9, e8, e7, e6, e5, e4,  \
		      e3, e2, e1, e0)                                        \
	simde_mm_setr_epi8(e15, e14, e13, e12, e11, e10, e9, e8, e7, e6, e5, \
			   e4, e3, e2, e1, e0)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_setr_epi16(int16_t e7, int16_t e6, int16_t e5, int16_t e4,
				 int16_t e3, int16_t e2, int16_t e1, int16_t e0)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_setr_epi16(e7, e6, e5, e4, e3, e2, e1, e0);
#else
	return simde_mm_set_epi16(e0, e1, e2, e3, e4, e5, e6, e7);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_setr_epi16(e7, e6, e5, e4, e3, e2, e1, e0) \
	simde_mm_setr_epi16(e7, e6, e5, e4, e3, e2, e1, e0)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_setr_epi32(int32_t e3, int32_t e2, int32_t e1, int32_t e0)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_setr_epi32(e3, e2, e1, e0);
#else
	return simde_mm_set_epi32(e0, e1, e2, e3);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_setr_epi32(e3, e2, e1, e0) simde_mm_setr_epi32(e3, e2, e1, e0)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_setr_epi64(simde__m64 e1, simde__m64 e0)
{
#if defined(SIMDE_X86_SSE2_NATIVE) && defined(SIMDE_X86_MMX_NATIVE)
	return _mm_setr_epi64(e1, e0);
#else
	return simde_mm_set_epi64(e0, e1);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_setr_epi64(e1, e0) (simde_mm_setr_epi64((e1), (e0)))
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_setr_pd(simde_float64 e1, simde_float64 e0)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_setr_pd(e1, e0);
#else
	return simde_mm_set_pd(e0, e1);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_setr_pd(e1, e0) simde_mm_setr_pd(e1, e0)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_setzero_pd(void)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_setzero_pd();
#else
	simde__m128d_private r_;

	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i32f) / sizeof(r_.i32f[0])); i++) {
		r_.i32f[i] = 0;
	}

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_setzero_pd() simde_mm_setzero_pd()
#endif

#if defined(SIMDE_DIAGNOSTIC_DISABLE_UNINITIALIZED_)
HEDLEY_DIAGNOSTIC_PUSH
SIMDE_DIAGNOSTIC_DISABLE_UNINITIALIZED_
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_undefined_pd(void)
{
	simde__m128d_private r_;

#if defined(SIMDE_X86_SSE2_NATIVE) && defined(SIMDE__HAVE_UNDEFINED128)
	r_.n = _mm_undefined_pd();
#elif !defined(SIMDE_DIAGNOSTIC_DISABLE_UNINITIALIZED_)
	r_ = simde__m128d_to_private(simde_mm_setzero_pd());
#endif

	return simde__m128d_from_private(r_);
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_undefined_pd() simde_mm_undefined_pd()
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_undefined_si128(void)
{
	simde__m128i_private r_;

#if defined(SIMDE_X86_SSE2_NATIVE) && defined(SIMDE__HAVE_UNDEFINED128)
	r_.n = _mm_undefined_si128();
#elif !defined(SIMDE_DIAGNOSTIC_DISABLE_UNINITIALIZED_)
	r_ = simde__m128i_to_private(simde_mm_setzero_si128());
#endif

	return simde__m128i_from_private(r_);
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_undefined_si128() (simde_mm_undefined_si128())
#endif

#if defined(SIMDE_DIAGNOSTIC_DISABLE_UNINITIALIZED_)
HEDLEY_DIAGNOSTIC_POP
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_x_mm_setone_pd(void)
{
	return simde_mm_castps_pd(simde_x_mm_setone_ps());
}

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_x_mm_setone_si128(void)
{
	return simde_mm_castps_si128(simde_x_mm_setone_ps());
}

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_shuffle_epi32(simde__m128i a, const int imm8)
	SIMDE_REQUIRE_RANGE(imm8, 0, 255)
{
	simde__m128i_private r_, a_ = simde__m128i_to_private(a);

	for (size_t i = 0; i < (sizeof(r_.i32) / sizeof(r_.i32[0])); i++) {
		r_.i32[i] = a_.i32[(imm8 >> (i * 2)) & 3];
	}

	return simde__m128i_from_private(r_);
}
#if defined(SIMDE_X86_SSE2_NATIVE)
#define simde_mm_shuffle_epi32(a, imm8) _mm_shuffle_epi32((a), (imm8))
#elif defined(SIMDE_SHUFFLE_VECTOR_)
#define simde_mm_shuffle_epi32(a, imm8)                               \
	(__extension__({                                              \
		const simde__m128i_private simde__tmp_a_ =            \
			simde__m128i_to_private(a);                   \
		simde__m128i_from_private((simde__m128i_private){     \
			.i32 = SIMDE_SHUFFLE_VECTOR_(                 \
				32, 16, (simde__tmp_a_).i32,          \
				(simde__tmp_a_).i32, ((imm8)) & 3,    \
				((imm8) >> 2) & 3, ((imm8) >> 4) & 3, \
				((imm8) >> 6) & 3)});                 \
	}))
#endif
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_shuffle_epi32(a, imm8) simde_mm_shuffle_epi32(a, imm8)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_shuffle_pd(simde__m128d a, simde__m128d b, const int imm8)
	SIMDE_REQUIRE_RANGE(imm8, 0, 3)
{
	simde__m128d_private r_, a_ = simde__m128d_to_private(a),
				 b_ = simde__m128d_to_private(b);

	r_.f64[0] = ((imm8 & 1) == 0) ? a_.f64[0] : a_.f64[1];
	r_.f64[1] = ((imm8 & 2) == 0) ? b_.f64[0] : b_.f64[1];

	return simde__m128d_from_private(r_);
}
#if defined(SIMDE_X86_SSE2_NATIVE) && !defined(__PGI)
#define simde_mm_shuffle_pd(a, b, imm8) _mm_shuffle_pd((a), (b), (imm8))
#elif defined(SIMDE_SHUFFLE_VECTOR_)
#define simde_mm_shuffle_pd(a, b, imm8)                                     \
	(__extension__({                                                    \
		simde__m128d_from_private((simde__m128d_private){           \
			.f64 = SIMDE_SHUFFLE_VECTOR_(                       \
				64, 16, simde__m128d_to_private(a).f64,     \
				simde__m128d_to_private(b).f64,             \
				(((imm8)) & 1), (((imm8) >> 1) & 1) + 2)}); \
	}))
#endif
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_shuffle_pd(a, b, imm8) simde_mm_shuffle_pd(a, b, imm8)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_shufflehi_epi16(simde__m128i a, const int imm8)
	SIMDE_REQUIRE_RANGE(imm8, 0, 255)
{
	simde__m128i_private r_, a_ = simde__m128i_to_private(a);

	SIMDE_VECTORIZE
	for (size_t i = 0; i < ((sizeof(a_.i16) / sizeof(a_.i16[0])) / 2);
	     i++) {
		r_.i16[i] = a_.i16[i];
	}
	for (size_t i = ((sizeof(a_.i16) / sizeof(a_.i16[0])) / 2);
	     i < (sizeof(r_.i16) / sizeof(r_.i16[0])); i++) {
		r_.i16[i] = a_.i16[((imm8 >> ((i - 4) * 2)) & 3) + 4];
	}

	return simde__m128i_from_private(r_);
}
#if defined(SIMDE_X86_SSE2_NATIVE)
#define simde_mm_shufflehi_epi16(a, imm8) _mm_shufflehi_epi16((a), (imm8))
#elif defined(SIMDE_SHUFFLE_VECTOR_)
#define simde_mm_shufflehi_epi16(a, imm8)                                    \
	(__extension__({                                                     \
		const simde__m128i_private simde__tmp_a_ =                   \
			simde__m128i_to_private(a);                          \
		simde__m128i_from_private((simde__m128i_private){            \
			.i16 = SIMDE_SHUFFLE_VECTOR_(                        \
				16, 16, (simde__tmp_a_).i16,                 \
				(simde__tmp_a_).i16, 0, 1, 2, 3,             \
				(((imm8)) & 3) + 4, (((imm8) >> 2) & 3) + 4, \
				(((imm8) >> 4) & 3) + 4,                     \
				(((imm8) >> 6) & 3) + 4)});                  \
	}))
#endif
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_shufflehi_epi16(a, imm8) simde_mm_shufflehi_epi16(a, imm8)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_shufflelo_epi16(simde__m128i a, const int imm8)
	SIMDE_REQUIRE_RANGE(imm8, 0, 255)
{
	simde__m128i_private r_, a_ = simde__m128i_to_private(a);

	for (size_t i = 0; i < ((sizeof(r_.i16) / sizeof(r_.i16[0])) / 2);
	     i++) {
		r_.i16[i] = a_.i16[((imm8 >> (i * 2)) & 3)];
	}
	SIMDE_VECTORIZE
	for (size_t i = ((sizeof(a_.i16) / sizeof(a_.i16[0])) / 2);
	     i < (sizeof(r_.i16) / sizeof(r_.i16[0])); i++) {
		r_.i16[i] = a_.i16[i];
	}

	return simde__m128i_from_private(r_);
}
#if defined(SIMDE_X86_SSE2_NATIVE)
#define simde_mm_shufflelo_epi16(a, imm8) _mm_shufflelo_epi16((a), (imm8))
#elif defined(SIMDE_SHUFFLE_VECTOR_)
#define simde_mm_shufflelo_epi16(a, imm8)                                 \
	(__extension__({                                                  \
		const simde__m128i_private simde__tmp_a_ =                \
			simde__m128i_to_private(a);                       \
		simde__m128i_from_private((simde__m128i_private){         \
			.i16 = SIMDE_SHUFFLE_VECTOR_(                     \
				16, 16, (simde__tmp_a_).i16,              \
				(simde__tmp_a_).i16, (((imm8)) & 3),      \
				(((imm8) >> 2) & 3), (((imm8) >> 4) & 3), \
				(((imm8) >> 6) & 3), 4, 5, 6, 7)});       \
	}))
#endif
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_shufflelo_epi16(a, imm8) simde_mm_shufflelo_epi16(a, imm8)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_sll_epi16(simde__m128i a, simde__m128i count)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_sll_epi16(a, count);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 count_ = simde__m128i_to_private(count);

	if (count_.u64[0] > 15)
		return simde_mm_setzero_si128();

#if defined(SIMDE_VECTOR_SUBSCRIPT_SCALAR)
	r_.u16 = (a_.u16 << count_.u64[0]);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.u16) / sizeof(r_.u16[0])); i++) {
		r_.u16[i] = HEDLEY_STATIC_CAST(uint16_t,
					       (a_.u16[i] << count_.u64[0]));
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_sll_epi16(a, count) simde_mm_sll_epi16((a), (count))
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_sll_epi32(simde__m128i a, simde__m128i count)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_sll_epi32(a, count);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 count_ = simde__m128i_to_private(count);

	if (count_.u64[0] > 31)
		return simde_mm_setzero_si128();

#if defined(SIMDE_VECTOR_SUBSCRIPT_SCALAR)
	r_.u32 = (a_.u32 << count_.u64[0]);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.u32) / sizeof(r_.u32[0])); i++) {
		r_.u32[i] = HEDLEY_STATIC_CAST(uint32_t,
					       (a_.u32[i] << count_.u64[0]));
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_sll_epi32(a, count) (simde_mm_sll_epi32(a, (count)))
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_sll_epi64(simde__m128i a, simde__m128i count)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_sll_epi64(a, count);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 count_ = simde__m128i_to_private(count);

	if (count_.u64[0] > 63)
		return simde_mm_setzero_si128();

	const int_fast16_t s = HEDLEY_STATIC_CAST(int_fast16_t, count_.u64[0]);
#if !defined(SIMDE_BUG_GCC_94488)
	SIMDE_VECTORIZE
#endif
	for (size_t i = 0; i < (sizeof(r_.u64) / sizeof(r_.u64[0])); i++) {
		r_.u64[i] = a_.u64[i] << s;
	}

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_sll_epi64(a, count) (simde_mm_sll_epi64(a, (count)))
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_sqrt_pd(simde__m128d a)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_sqrt_pd(a);
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a);

#if defined(SIMDE_ARM_NEON_A64V8_NATIVE)
	r_.neon_f64 = vsqrtq_f64(a_.neon_f64);
#elif defined(simde_math_sqrt)
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.f64) / sizeof(r_.f64[0])); i++) {
		r_.f64[i] = simde_math_sqrt(a_.f64[i]);
	}
#else
	HEDLEY_UNREACHABLE();
#endif

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_sqrt_pd(a) simde_mm_sqrt_pd(a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_sqrt_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_sqrt_sd(a, b);
#elif defined(SIMDE_ASSUME_VECTORIZATION)
	return simde_mm_move_sd(a, simde_mm_sqrt_pd(b));
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a),
				 b_ = simde__m128d_to_private(b);

#if defined(simde_math_sqrt)
	r_.f64[0] = simde_math_sqrt(b_.f64[0]);
	r_.f64[1] = a_.f64[1];
#else
	HEDLEY_UNREACHABLE();
#endif

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_sqrt_sd(a, b) simde_mm_sqrt_sd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_srl_epi16(simde__m128i a, simde__m128i count)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_srl_epi16(a, count);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 count_ = simde__m128i_to_private(count);

	const int cnt = HEDLEY_STATIC_CAST(
		int, (count_.i64[0] > 16 ? 16 : count_.i64[0]));

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_u16 = vshlq_u16(a_.neon_u16,
				vdupq_n_s16(HEDLEY_STATIC_CAST(int16_t, -cnt)));
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.u16) / sizeof(r_.u16[0])); i++) {
		r_.u16[i] = a_.u16[i] >> cnt;
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_srl_epi16(a, count) (simde_mm_srl_epi16(a, (count)))
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_srl_epi32(simde__m128i a, simde__m128i count)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_srl_epi32(a, count);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 count_ = simde__m128i_to_private(count);

	const int cnt = HEDLEY_STATIC_CAST(
		int, (count_.i64[0] > 32 ? 32 : count_.i64[0]));

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_u32 = vshlq_u32(a_.neon_u32,
				vdupq_n_s32(HEDLEY_STATIC_CAST(int32_t, -cnt)));
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.u32) / sizeof(r_.u32[0])); i++) {
		r_.u32[i] = a_.u32[i] >> cnt;
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_srl_epi32(a, count) (simde_mm_srl_epi32(a, (count)))
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_srl_epi64(simde__m128i a, simde__m128i count)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_srl_epi64(a, count);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 count_ = simde__m128i_to_private(count);

	const int cnt = HEDLEY_STATIC_CAST(
		int, (count_.i64[0] > 64 ? 64 : count_.i64[0]));

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_u64 = vshlq_u64(a_.neon_u64,
				vdupq_n_s64(HEDLEY_STATIC_CAST(int64_t, -cnt)));
#else
#if !defined(SIMDE_BUG_GCC_94488)
	SIMDE_VECTORIZE
#endif
	for (size_t i = 0; i < (sizeof(r_.u64) / sizeof(r_.u64[0])); i++) {
		r_.u64[i] = a_.u64[i] >> cnt;
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_srl_epi64(a, count) (simde_mm_srl_epi64(a, (count)))
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_srai_epi16(simde__m128i a, const int imm8)
	SIMDE_REQUIRE_CONSTANT_RANGE(imm8, 0, 255)
{
	/* MSVC requires a range of (0, 255). */
	simde__m128i_private r_, a_ = simde__m128i_to_private(a);

	const int cnt = (imm8 & ~15) ? 15 : imm8;

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i16 = vshlq_s16(a_.neon_i16, vdupq_n_s16(-cnt));
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_) / sizeof(r_.i16[0])); i++) {
		r_.i16[i] = a_.i16[i] >> cnt;
	}
#endif

	return simde__m128i_from_private(r_);
}
#if defined(SIMDE_X86_SSE2_NATIVE)
#define simde_mm_srai_epi16(a, imm8) _mm_srai_epi16((a), (imm8))
#endif
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_srai_epi16(a, imm8) simde_mm_srai_epi16(a, imm8)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_srai_epi32(simde__m128i a, const int imm8)
	SIMDE_REQUIRE_CONSTANT_RANGE(imm8, 0, 255)
{
	/* MSVC requires a range of (0, 255). */
	simde__m128i_private r_, a_ = simde__m128i_to_private(a);

	const int cnt = (imm8 & ~31) ? 31 : imm8;

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i32 = vshlq_s32(a_.neon_i32, vdupq_n_s32(-cnt));
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_) / sizeof(r_.i32[0])); i++) {
		r_.i32[i] = a_.i32[i] >> cnt;
	}
#endif

	return simde__m128i_from_private(r_);
}
#if defined(SIMDE_X86_SSE2_NATIVE)
#define simde_mm_srai_epi32(a, imm8) _mm_srai_epi32((a), (imm8))
#endif
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_srai_epi32(a, imm8) simde_mm_srai_epi32(a, imm8)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_sra_epi16(simde__m128i a, simde__m128i count)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_sra_epi16(a, count);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 count_ = simde__m128i_to_private(count);

	const int cnt = HEDLEY_STATIC_CAST(
		int, (count_.i64[0] > 15 ? 15 : count_.i64[0]));

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i16 = vshlq_s16(a_.neon_i16,
				vdupq_n_s16(HEDLEY_STATIC_CAST(int16_t, -cnt)));
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i16) / sizeof(r_.i16[0])); i++) {
		r_.i16[i] = a_.i16[i] >> cnt;
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_sra_epi16(a, count) (simde_mm_sra_epi16(a, count))
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_sra_epi32(simde__m128i a, simde__m128i count)
{
#if defined(SIMDE_X86_SSE2_NATIVE) && !defined(SIMDE_BUG_GCC_BAD_MM_SRA_EPI32)
	return _mm_sra_epi32(a, count);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 count_ = simde__m128i_to_private(count);

	const int cnt = count_.u64[0] > 31
				? 31
				: HEDLEY_STATIC_CAST(int, count_.u64[0]);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i32 = vshlq_s32(a_.neon_i32,
				vdupq_n_s32(HEDLEY_STATIC_CAST(int32_t, -cnt)));
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i32) / sizeof(r_.i32[0])); i++) {
		r_.i32[i] = a_.i32[i] >> cnt;
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_sra_epi32(a, count) (simde_mm_sra_epi32(a, (count)))
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_slli_epi16(simde__m128i a, const int imm8)
	SIMDE_REQUIRE_RANGE(imm8, 0, 255)
{
	simde__m128i_private r_, a_ = simde__m128i_to_private(a);

#if defined(SIMDE_VECTOR_SUBSCRIPT_SCALAR)
	r_.i16 = a_.i16 << (imm8 & 0xff);
#else
	const int s =
		(imm8 >
		 HEDLEY_STATIC_CAST(int, sizeof(r_.i16[0]) * CHAR_BIT) - 1)
			? 0
			: imm8;
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i16) / sizeof(r_.i16[0])); i++) {
		r_.i16[i] = HEDLEY_STATIC_CAST(int16_t, a_.i16[i] << s);
	}
#endif

	return simde__m128i_from_private(r_);
}
#if defined(SIMDE_X86_SSE2_NATIVE)
#define simde_mm_slli_epi16(a, imm8) _mm_slli_epi16(a, imm8)
#elif defined(SIMDE_ARM_NEON_A32V7_NATIVE)
#define simde_mm_slli_epi16(a, imm8) \
	simde__m128i_from_neon_u16(  \
		vshlq_n_u16(simde__m128i_to_neon_u16(a), (imm8)))
#endif
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_slli_epi16(a, imm8) simde_mm_slli_epi16(a, imm8)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_slli_epi32(simde__m128i a, const int imm8)
	SIMDE_REQUIRE_RANGE(imm8, 0, 255)
{
	simde__m128i_private r_, a_ = simde__m128i_to_private(a);

#if defined(SIMDE_VECTOR_SUBSCRIPT_SCALAR)
	r_.i32 = a_.i32 << imm8;
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i32) / sizeof(r_.i32[0])); i++) {
		r_.i32[i] = a_.i32[i] << (imm8 & 0xff);
	}
#endif

	return simde__m128i_from_private(r_);
}
#if defined(SIMDE_X86_SSE2_NATIVE)
#define simde_mm_slli_epi32(a, imm8) _mm_slli_epi32(a, imm8)
#elif defined(SIMDE_ARM_NEON_A32V7_NATIVE)
#define simde_mm_slli_epi32(a, imm8) \
	simde__m128i_from_neon_u32(  \
		vshlq_n_u32(simde__m128i_to_neon_u32(a), (imm8)))
#endif
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_slli_epi32(a, imm8) simde_mm_slli_epi32(a, imm8)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_slli_epi64(simde__m128i a, const int imm8)
	SIMDE_REQUIRE_RANGE(imm8, 0, 255)
{
	simde__m128i_private r_, a_ = simde__m128i_to_private(a);

#if defined(SIMDE_VECTOR_SUBSCRIPT_SCALAR)
	r_.i64 = a_.i64 << imm8;
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i64) / sizeof(r_.i64[0])); i++) {
		r_.i64[i] = a_.i64[i] << (imm8 & 0xff);
	}
#endif

	return simde__m128i_from_private(r_);
}
#if defined(SIMDE_X86_SSE2_NATIVE)
#define simde_mm_slli_epi64(a, imm8) _mm_slli_epi64(a, imm8)
#elif defined(SIMDE_ARM_NEON_A32V7_NATIVE)
#define simde_mm_slli_epi64(a, imm8) \
	simde__m128i_from_neon_u64(  \
		vshlq_n_u64(simde__m128i_to_neon_u64(a), (imm8)))
#endif
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_slli_epi64(a, imm8) simde_mm_slli_epi64(a, imm8)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_srli_epi16(simde__m128i a, const int imm8)
	SIMDE_REQUIRE_RANGE(imm8, 0, 255)
{
	simde__m128i_private r_, a_ = simde__m128i_to_private(a);

#if defined(SIMDE_VECTOR_SUBSCRIPT_SCALAR)
	r_.u16 = a_.u16 >> imm8;
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i16) / sizeof(r_.i16[0])); i++) {
		r_.u16[i] = a_.u16[i] >> (imm8 & 0xff);
	}
#endif

	return simde__m128i_from_private(r_);
}
#if defined(SIMDE_X86_SSE2_NATIVE)
#define simde_mm_srli_epi16(a, imm8) _mm_srli_epi16(a, imm8)
#elif defined(SIMDE_ARM_NEON_A32V7_NATIVE)
#define simde_mm_srli_epi16(a, imm8) \
	simde__m128i_from_neon_u16(  \
		vshrq_n_u16(simde__m128i_to_neon_u16(a), imm8))
#endif
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_srli_epi16(a, imm8) simde_mm_srli_epi16(a, imm8)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_srli_epi32(simde__m128i a, const int imm8)
	SIMDE_REQUIRE_RANGE(imm8, 0, 255)
{
	simde__m128i_private r_, a_ = simde__m128i_to_private(a);

#if defined(SIMDE_VECTOR_SUBSCRIPT_SCALAR)
	r_.u32 = a_.u32 >> (imm8 & 0xff);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i32) / sizeof(r_.i32[0])); i++) {
		r_.u32[i] = a_.u32[i] >> (imm8 & 0xff);
	}
#endif

	return simde__m128i_from_private(r_);
}
#if defined(SIMDE_X86_SSE2_NATIVE)
#define simde_mm_srli_epi32(a, imm8) _mm_srli_epi32(a, imm8)
#elif defined(SIMDE_ARM_NEON_A32V7_NATIVE)
#define simde_mm_srli_epi32(a, imm8) \
	simde__m128i_from_neon_u32(  \
		vshrq_n_u32(simde__m128i_to_neon_u32(a), imm8))
#endif
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_srli_epi32(a, imm8) simde_mm_srli_epi32(a, imm8)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_srli_epi64(simde__m128i a, const int imm8)
	SIMDE_REQUIRE_RANGE(imm8, 0, 255)
{
	simde__m128i_private r_, a_ = simde__m128i_to_private(a);

	if (HEDLEY_UNLIKELY((imm8 & 63) != imm8))
		return simde_mm_setzero_si128();

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_u64 = vshlq_u64(a_.neon_u64, vdupq_n_s64(-imm8));
#else
#if defined(SIMDE_VECTOR_SUBSCRIPT_SCALAR) && !defined(SIMDE_BUG_GCC_94488)
	r_.u64 = a_.u64 >> imm8;
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i64) / sizeof(r_.i64[0])); i++) {
		r_.u64[i] = a_.u64[i] >> imm8;
	}
#endif
#endif

	return simde__m128i_from_private(r_);
}
#if defined(SIMDE_X86_SSE2_NATIVE)
#define simde_mm_srli_epi64(a, imm8) _mm_srli_epi64(a, imm8)
#elif defined(SIMDE_ARM_NEON_A32V7_NATIVE) && !defined(__clang__)
#define simde_mm_srli_epi64(a, imm8)                            \
	((imm8 == 0) ? (a)                                      \
		     : (simde__m128i_from_neon_u64(vshrq_n_u64( \
			       simde__m128i_to_neon_u64(a), imm8))))
#endif
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_srli_epi64(a, imm8) simde_mm_srli_epi64(a, imm8)
#endif

SIMDE_FUNCTION_ATTRIBUTES
void simde_mm_store_pd(simde_float64 mem_addr[HEDLEY_ARRAY_PARAM(2)],
		       simde__m128d a)
{
	simde_assert_aligned(16, mem_addr);

#if defined(SIMDE_X86_SSE2_NATIVE)
	_mm_store_pd(mem_addr, a);
#elif defined(SIMDE_ARM_NEON_A64V8_NATIVE)
	vst1q_f64(mem_addr, simde__m128d_to_private(a).neon_f64);
#else
	simde_memcpy(mem_addr, &a, sizeof(a));
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_store_pd(mem_addr, a) \
	simde_mm_store_pd(HEDLEY_REINTERPRET_CAST(double *, mem_addr), a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
void simde_mm_store1_pd(simde_float64 mem_addr[HEDLEY_ARRAY_PARAM(2)],
			simde__m128d a)
{
	simde_assert_aligned(16, mem_addr);

#if defined(SIMDE_X86_SSE2_NATIVE)
	_mm_store1_pd(mem_addr, a);
#else
	simde__m128d_private a_ = simde__m128d_to_private(a);

	mem_addr[0] = a_.f64[0];
	mem_addr[1] = a_.f64[0];
#endif
}
#define simde_mm_store_pd1(mem_addr, a) \
	simde_mm_store1_pd(HEDLEY_REINTERPRET_CAST(double *, mem_addr), a)
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_store1_pd(mem_addr, a) \
	simde_mm_store1_pd(HEDLEY_REINTERPRET_CAST(double *, mem_addr), a)
#define _mm_store_pd1(mem_addr, a) \
	simde_mm_store_pd1(HEDLEY_REINTERPRET_CAST(double *, mem_addr), a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
void simde_mm_store_sd(simde_float64 *mem_addr, simde__m128d a)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	_mm_store_sd(mem_addr, a);
#else
	simde__m128d_private a_ = simde__m128d_to_private(a);

#if defined(SIMDE_ARM_NEON_A64V8_NATIVE)
	simde_float64 v = vgetq_lane_f64(a_.neon_f64, 0);
	simde_memcpy(mem_addr, &v, sizeof(simde_float64));
#else
	simde_float64 v = a_.f64[0];
	simde_memcpy(mem_addr, &v, sizeof(simde_float64));
#endif
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_store_sd(mem_addr, a) \
	simde_mm_store_sd(HEDLEY_REINTERPRET_CAST(double *, mem_addr), a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
void simde_mm_store_si128(simde__m128i *mem_addr, simde__m128i a)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	_mm_store_si128(HEDLEY_STATIC_CAST(__m128i *, mem_addr), a);
#else
	simde__m128i_private a_ = simde__m128i_to_private(a);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	vst1q_s32(HEDLEY_REINTERPRET_CAST(int32_t *, mem_addr), a_.neon_i32);
#else
	simde_memcpy(SIMDE_ASSUME_ALIGNED(16, mem_addr), &a_, sizeof(a_));
#endif
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_store_si128(mem_addr, a) simde_mm_store_si128(mem_addr, a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
void simde_mm_storeh_pd(simde_float64 *mem_addr, simde__m128d a)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	_mm_storeh_pd(mem_addr, a);
#else
	simde__m128d_private a_ = simde__m128d_to_private(a);

#if defined(SIMDE_ARM_NEON_A64V8_NATIVE)
	*mem_addr = vgetq_lane_f64(a_.neon_f64, 1);
#else
	*mem_addr = a_.f64[1];
#endif
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_storeh_pd(mem_addr, a) \
	simde_mm_storeh_pd(HEDLEY_REINTERPRET_CAST(double *, mem_addr), a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
void simde_mm_storel_epi64(simde__m128i *mem_addr, simde__m128i a)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	_mm_storel_epi64(HEDLEY_STATIC_CAST(__m128i *, mem_addr), a);
#else
	simde__m128i_private a_ = simde__m128i_to_private(a);
	int64_t tmp;

	/* memcpy to prevent aliasing, tmp because we can't take the
     * address of a vector element. */

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	tmp = vgetq_lane_s64(a_.neon_i64, 0);
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
#if defined(SIMDE_BUG_GCC_95227)
	(void)a_;
#endif
	tmp = vec_extract(a_.altivec_i64, 0);
#else
	tmp = a_.i64[0];
#endif

	simde_memcpy(mem_addr, &tmp, sizeof(tmp));
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_storel_epi64(mem_addr, a) simde_mm_storel_epi64(mem_addr, a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
void simde_mm_storel_pd(simde_float64 *mem_addr, simde__m128d a)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	_mm_storel_pd(mem_addr, a);
#else
	simde__m128d_private a_ = simde__m128d_to_private(a);

	*mem_addr = a_.f64[0];
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_storel_pd(mem_addr, a) \
	simde_mm_storel_pd(HEDLEY_REINTERPRET_CAST(double *, mem_addr), a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
void simde_mm_storer_pd(simde_float64 mem_addr[2], simde__m128d a)
{
	simde_assert_aligned(16, mem_addr);

#if defined(SIMDE_X86_SSE2_NATIVE)
	_mm_storer_pd(mem_addr, a);
#else
	simde__m128d_private a_ = simde__m128d_to_private(a);

	mem_addr[0] = a_.f64[1];
	mem_addr[1] = a_.f64[0];
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_storer_pd(mem_addr, a) \
	simde_mm_storer_pd(HEDLEY_REINTERPRET_CAST(double *, mem_addr), a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
void simde_mm_storeu_pd(simde_float64 *mem_addr, simde__m128d a)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	_mm_storeu_pd(mem_addr, a);
#else
	simde_memcpy(mem_addr, &a, sizeof(a));
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_storeu_pd(mem_addr, a) \
	simde_mm_storeu_pd(HEDLEY_REINTERPRET_CAST(double *, mem_addr), a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
void simde_mm_storeu_si128(simde__m128i *mem_addr, simde__m128i a)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	_mm_storeu_si128(HEDLEY_STATIC_CAST(__m128i *, mem_addr), a);
#else
	simde__m128i_private a_ = simde__m128i_to_private(a);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	vst1q_s32(HEDLEY_REINTERPRET_CAST(int32_t *, mem_addr), a_.neon_i32);
#else
	simde_memcpy(mem_addr, &a_, sizeof(a_));
#endif
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_storeu_si128(mem_addr, a) simde_mm_storeu_si128(mem_addr, a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
void simde_mm_stream_pd(simde_float64 mem_addr[HEDLEY_ARRAY_PARAM(2)],
			simde__m128d a)
{
	simde_assert_aligned(16, mem_addr);

#if defined(SIMDE_X86_SSE2_NATIVE)
	_mm_stream_pd(mem_addr, a);
#else
	simde_memcpy(mem_addr, &a, sizeof(a));
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_stream_pd(mem_addr, a) \
	simde_mm_stream_pd(HEDLEY_REINTERPRET_CAST(double *, mem_addr), a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
void simde_mm_stream_si128(simde__m128i *mem_addr, simde__m128i a)
{
	simde_assert_aligned(16, mem_addr);

#if defined(SIMDE_X86_SSE2_NATIVE)
	_mm_stream_si128(HEDLEY_STATIC_CAST(__m128i *, mem_addr), a);
#else
	simde_memcpy(mem_addr, &a, sizeof(a));
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_stream_si128(mem_addr, a) simde_mm_stream_si128(mem_addr, a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
void simde_mm_stream_si32(int32_t *mem_addr, int32_t a)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	_mm_stream_si32(mem_addr, a);
#else
	*mem_addr = a;
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_stream_si32(mem_addr, a) simde_mm_stream_si32(mem_addr, a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
void simde_mm_stream_si64(int64_t *mem_addr, int64_t a)
{
	*mem_addr = a;
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_stream_si64(mem_addr, a)                                  \
	simde_mm_stream_si64(SIMDE_CHECKED_REINTERPRET_CAST(          \
				     int64_t *, __int64 *, mem_addr), \
			     a)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_sub_epi8(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_sub_epi8(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i8 = vsubq_s8(a_.neon_i8, b_.neon_i8);
#elif defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.i8 = a_.i8 - b_.i8;
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i8) / sizeof(r_.i8[0])); i++) {
		r_.i8[i] = a_.i8[i] - b_.i8[i];
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_sub_epi8(a, b) simde_mm_sub_epi8(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_sub_epi16(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_sub_epi16(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i16 = vsubq_s16(a_.neon_i16, b_.neon_i16);
#elif defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.i16 = a_.i16 - b_.i16;
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i16) / sizeof(r_.i16[0])); i++) {
		r_.i16[i] = a_.i16[i] - b_.i16[i];
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_sub_epi16(a, b) simde_mm_sub_epi16(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_sub_epi32(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_sub_epi32(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i32 = vsubq_s32(a_.neon_i32, b_.neon_i32);
#elif defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.i32 = a_.i32 - b_.i32;
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i32) / sizeof(r_.i32[0])); i++) {
		r_.i32[i] = a_.i32[i] - b_.i32[i];
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_sub_epi32(a, b) simde_mm_sub_epi32(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_sub_epi64(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_sub_epi64(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i64 = vsubq_s64(a_.neon_i64, b_.neon_i64);
#elif defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.i64 = a_.i64 - b_.i64;
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i64) / sizeof(r_.i64[0])); i++) {
		r_.i64[i] = a_.i64[i] - b_.i64[i];
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_sub_epi64(a, b) simde_mm_sub_epi64(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_x_mm_sub_epu32(simde__m128i a, simde__m128i b)
{
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.u32 = a_.u32 - b_.u32;
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.u32) / sizeof(r_.u32[0])); i++) {
		r_.u32[i] = a_.u32[i] - b_.u32[i];
	}
#endif

	return simde__m128i_from_private(r_);
}

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_sub_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_sub_pd(a, b);
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a),
				 b_ = simde__m128d_to_private(b);

#if defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.f64 = a_.f64 - b_.f64;
#elif defined(SIMDE_WASM_SIMD128_NATIVE)
	r_.wasm_v128 = wasm_f64x2_sub(a_.wasm_v128, b_.wasm_v128);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.f64) / sizeof(r_.f64[0])); i++) {
		r_.f64[i] = a_.f64[i] - b_.f64[i];
	}
#endif

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_sub_pd(a, b) simde_mm_sub_pd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_sub_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_sub_sd(a, b);
#elif defined(SIMDE_ASSUME_VECTORIZATION)
	return simde_mm_move_sd(a, simde_mm_sub_pd(a, b));
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a),
				 b_ = simde__m128d_to_private(b);

	r_.f64[0] = a_.f64[0] - b_.f64[0];
	r_.f64[1] = a_.f64[1];

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_sub_sd(a, b) simde_mm_sub_sd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m64 simde_mm_sub_si64(simde__m64 a, simde__m64 b)
{
#if defined(SIMDE_X86_SSE2_NATIVE) && defined(SIMDE_X86_MMX_NATIVE)
	return _mm_sub_si64(a, b);
#else
	simde__m64_private r_, a_ = simde__m64_to_private(a),
			       b_ = simde__m64_to_private(b);

#if defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.i64 = a_.i64 - b_.i64;
#else
	r_.i64[0] = a_.i64[0] - b_.i64[0];
#endif

	return simde__m64_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_sub_si64(a, b) simde_mm_sub_si64(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_subs_epi8(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_subs_epi8(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i8 = vqsubq_s8(a_.neon_i8, b_.neon_i8);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_) / sizeof(r_.i8[0])); i++) {
		if (((b_.i8[i]) > 0 && (a_.i8[i]) < INT8_MIN + (b_.i8[i]))) {
			r_.i8[i] = INT8_MIN;
		} else if ((b_.i8[i]) < 0 &&
			   (a_.i8[i]) > INT8_MAX + (b_.i8[i])) {
			r_.i8[i] = INT8_MAX;
		} else {
			r_.i8[i] = (a_.i8[i]) - (b_.i8[i]);
		}
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_subs_epi8(a, b) simde_mm_subs_epi8(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_subs_epi16(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_subs_epi16(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i16 = vqsubq_s16(a_.neon_i16, b_.neon_i16);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_) / sizeof(r_.i16[0])); i++) {
		if (((b_.i16[i]) > 0 &&
		     (a_.i16[i]) < INT16_MIN + (b_.i16[i]))) {
			r_.i16[i] = INT16_MIN;
		} else if ((b_.i16[i]) < 0 &&
			   (a_.i16[i]) > INT16_MAX + (b_.i16[i])) {
			r_.i16[i] = INT16_MAX;
		} else {
			r_.i16[i] = (a_.i16[i]) - (b_.i16[i]);
		}
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_subs_epi16(a, b) simde_mm_subs_epi16(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_subs_epu8(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_subs_epu8(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_u8 = vqsubq_u8(a_.neon_u8, b_.neon_u8);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_) / sizeof(r_.i8[0])); i++) {
		const int32_t x = a_.u8[i] - b_.u8[i];
		if (x < 0) {
			r_.u8[i] = 0;
		} else if (x > UINT8_MAX) {
			r_.u8[i] = UINT8_MAX;
		} else {
			r_.u8[i] = HEDLEY_STATIC_CAST(uint8_t, x);
		}
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_subs_epu8(a, b) simde_mm_subs_epu8(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_subs_epu16(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_subs_epu16(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_u16 = vqsubq_u16(a_.neon_u16, b_.neon_u16);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_) / sizeof(r_.i16[0])); i++) {
		const int32_t x = a_.u16[i] - b_.u16[i];
		if (x < 0) {
			r_.u16[i] = 0;
		} else if (x > UINT16_MAX) {
			r_.u16[i] = UINT16_MAX;
		} else {
			r_.u16[i] = HEDLEY_STATIC_CAST(uint16_t, x);
		}
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_subs_epu16(a, b) simde_mm_subs_epu16(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
int simde_mm_ucomieq_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_ucomieq_sd(a, b);
#else
	simde__m128d_private a_ = simde__m128d_to_private(a),
			     b_ = simde__m128d_to_private(b);
	int r;

#if defined(SIMDE_HAVE_FENV_H)
	fenv_t envp;
	int x = feholdexcept(&envp);
	r = a_.f64[0] == b_.f64[0];
	if (HEDLEY_LIKELY(x == 0))
		fesetenv(&envp);
#else
	r = a_.f64[0] == b_.f64[0];
#endif

	return r;
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_ucomieq_sd(a, b) simde_mm_ucomieq_sd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
int simde_mm_ucomige_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_ucomige_sd(a, b);
#else
	simde__m128d_private a_ = simde__m128d_to_private(a),
			     b_ = simde__m128d_to_private(b);
	int r;

#if defined(SIMDE_HAVE_FENV_H)
	fenv_t envp;
	int x = feholdexcept(&envp);
	r = a_.f64[0] >= b_.f64[0];
	if (HEDLEY_LIKELY(x == 0))
		fesetenv(&envp);
#else
	r = a_.f64[0] >= b_.f64[0];
#endif

	return r;
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_ucomige_sd(a, b) simde_mm_ucomige_sd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
int simde_mm_ucomigt_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_ucomigt_sd(a, b);
#else
	simde__m128d_private a_ = simde__m128d_to_private(a),
			     b_ = simde__m128d_to_private(b);
	int r;

#if defined(SIMDE_HAVE_FENV_H)
	fenv_t envp;
	int x = feholdexcept(&envp);
	r = a_.f64[0] > b_.f64[0];
	if (HEDLEY_LIKELY(x == 0))
		fesetenv(&envp);
#else
	r = a_.f64[0] > b_.f64[0];
#endif

	return r;
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_ucomigt_sd(a, b) simde_mm_ucomigt_sd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
int simde_mm_ucomile_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_ucomile_sd(a, b);
#else
	simde__m128d_private a_ = simde__m128d_to_private(a),
			     b_ = simde__m128d_to_private(b);
	int r;

#if defined(SIMDE_HAVE_FENV_H)
	fenv_t envp;
	int x = feholdexcept(&envp);
	r = a_.f64[0] <= b_.f64[0];
	if (HEDLEY_LIKELY(x == 0))
		fesetenv(&envp);
#else
	r = a_.f64[0] <= b_.f64[0];
#endif

	return r;
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_ucomile_sd(a, b) simde_mm_ucomile_sd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
int simde_mm_ucomilt_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_ucomilt_sd(a, b);
#else
	simde__m128d_private a_ = simde__m128d_to_private(a),
			     b_ = simde__m128d_to_private(b);
	int r;

#if defined(SIMDE_HAVE_FENV_H)
	fenv_t envp;
	int x = feholdexcept(&envp);
	r = a_.f64[0] < b_.f64[0];
	if (HEDLEY_LIKELY(x == 0))
		fesetenv(&envp);
#else
	r = a_.f64[0] < b_.f64[0];
#endif

	return r;
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_ucomilt_sd(a, b) simde_mm_ucomilt_sd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
int simde_mm_ucomineq_sd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_ucomineq_sd(a, b);
#else
	simde__m128d_private a_ = simde__m128d_to_private(a),
			     b_ = simde__m128d_to_private(b);
	int r;

#if defined(SIMDE_HAVE_FENV_H)
	fenv_t envp;
	int x = feholdexcept(&envp);
	r = a_.f64[0] != b_.f64[0];
	if (HEDLEY_LIKELY(x == 0))
		fesetenv(&envp);
#else
	r = a_.f64[0] != b_.f64[0];
#endif

	return r;
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_ucomineq_sd(a, b) simde_mm_ucomineq_sd(a, b)
#endif

#if defined(SIMDE_DIAGNOSTIC_DISABLE_UNINITIALIZED_)
HEDLEY_DIAGNOSTIC_PUSH
SIMDE_DIAGNOSTIC_DISABLE_UNINITIALIZED_
#endif

#if defined(SIMDE_DIAGNOSTIC_DISABLE_UNINITIALIZED_)
HEDLEY_DIAGNOSTIC_POP
#endif

SIMDE_FUNCTION_ATTRIBUTES
void simde_mm_lfence(void)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	_mm_lfence();
#else
	simde_mm_sfence();
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_lfence() simde_mm_lfence()
#endif

SIMDE_FUNCTION_ATTRIBUTES
void simde_mm_mfence(void)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	_mm_mfence();
#else
	simde_mm_sfence();
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_mfence() simde_mm_mfence()
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_unpackhi_epi8(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_unpackhi_epi8(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	int8x8_t a1 = vreinterpret_s8_s16(vget_high_s16(a_.neon_i16));
	int8x8_t b1 = vreinterpret_s8_s16(vget_high_s16(b_.neon_i16));
	int8x8x2_t result = vzip_s8(a1, b1);
	r_.neon_i8 = vcombine_s8(result.val[0], result.val[1]);
#elif defined(SIMDE_SHUFFLE_VECTOR_)
	r_.i8 = SIMDE_SHUFFLE_VECTOR_(8, 16, a_.i8, b_.i8, 8, 24, 9, 25, 10, 26,
				      11, 27, 12, 28, 13, 29, 14, 30, 15, 31);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < ((sizeof(r_) / sizeof(r_.i8[0])) / 2); i++) {
		r_.i8[(i * 2)] =
			a_.i8[i + ((sizeof(r_) / sizeof(r_.i8[0])) / 2)];
		r_.i8[(i * 2) + 1] =
			b_.i8[i + ((sizeof(r_) / sizeof(r_.i8[0])) / 2)];
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_unpackhi_epi8(a, b) simde_mm_unpackhi_epi8(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_unpackhi_epi16(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_unpackhi_epi16(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	int16x4_t a1 = vget_high_s16(a_.neon_i16);
	int16x4_t b1 = vget_high_s16(b_.neon_i16);
	int16x4x2_t result = vzip_s16(a1, b1);
	r_.neon_i16 = vcombine_s16(result.val[0], result.val[1]);
#elif defined(SIMDE_SHUFFLE_VECTOR_)
	r_.i16 = SIMDE_SHUFFLE_VECTOR_(16, 16, a_.i16, b_.i16, 4, 12, 5, 13, 6,
				       14, 7, 15);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < ((sizeof(r_) / sizeof(r_.i16[0])) / 2); i++) {
		r_.i16[(i * 2)] =
			a_.i16[i + ((sizeof(r_) / sizeof(r_.i16[0])) / 2)];
		r_.i16[(i * 2) + 1] =
			b_.i16[i + ((sizeof(r_) / sizeof(r_.i16[0])) / 2)];
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_unpackhi_epi16(a, b) simde_mm_unpackhi_epi16(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_unpackhi_epi32(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_unpackhi_epi32(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	int32x2_t a1 = vget_high_s32(a_.neon_i32);
	int32x2_t b1 = vget_high_s32(b_.neon_i32);
	int32x2x2_t result = vzip_s32(a1, b1);
	r_.neon_i32 = vcombine_s32(result.val[0], result.val[1]);
#elif defined(SIMDE_SHUFFLE_VECTOR_)
	r_.i32 = SIMDE_SHUFFLE_VECTOR_(32, 16, a_.i32, b_.i32, 2, 6, 3, 7);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < ((sizeof(r_) / sizeof(r_.i32[0])) / 2); i++) {
		r_.i32[(i * 2)] =
			a_.i32[i + ((sizeof(r_) / sizeof(r_.i32[0])) / 2)];
		r_.i32[(i * 2) + 1] =
			b_.i32[i + ((sizeof(r_) / sizeof(r_.i32[0])) / 2)];
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_unpackhi_epi32(a, b) simde_mm_unpackhi_epi32(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_unpackhi_epi64(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_unpackhi_epi64(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_SHUFFLE_VECTOR_)
	r_.i64 = SIMDE_SHUFFLE_VECTOR_(64, 16, a_.i64, b_.i64, 1, 3);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < ((sizeof(r_) / sizeof(r_.i64[0])) / 2); i++) {
		r_.i64[(i * 2)] =
			a_.i64[i + ((sizeof(r_) / sizeof(r_.i64[0])) / 2)];
		r_.i64[(i * 2) + 1] =
			b_.i64[i + ((sizeof(r_) / sizeof(r_.i64[0])) / 2)];
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_unpackhi_epi64(a, b) simde_mm_unpackhi_epi64(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_unpackhi_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_unpackhi_pd(a, b);
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a),
				 b_ = simde__m128d_to_private(b);

#if defined(SIMDE_SHUFFLE_VECTOR_)
	r_.f64 = SIMDE_SHUFFLE_VECTOR_(64, 16, a_.f64, b_.f64, 1, 3);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < ((sizeof(r_) / sizeof(r_.f64[0])) / 2); i++) {
		r_.f64[(i * 2)] =
			a_.f64[i + ((sizeof(r_) / sizeof(r_.f64[0])) / 2)];
		r_.f64[(i * 2) + 1] =
			b_.f64[i + ((sizeof(r_) / sizeof(r_.f64[0])) / 2)];
	}
#endif

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_unpackhi_pd(a, b) simde_mm_unpackhi_pd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_unpacklo_epi8(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_unpacklo_epi8(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	int8x8_t a1 = vreinterpret_s8_s16(vget_low_s16(a_.neon_i16));
	int8x8_t b1 = vreinterpret_s8_s16(vget_low_s16(b_.neon_i16));
	int8x8x2_t result = vzip_s8(a1, b1);
	r_.neon_i8 = vcombine_s8(result.val[0], result.val[1]);
#elif defined(SIMDE_SHUFFLE_VECTOR_)
	r_.i8 = SIMDE_SHUFFLE_VECTOR_(8, 16, a_.i8, b_.i8, 0, 16, 1, 17, 2, 18,
				      3, 19, 4, 20, 5, 21, 6, 22, 7, 23);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < ((sizeof(r_) / sizeof(r_.i8[0])) / 2); i++) {
		r_.i8[(i * 2)] = a_.i8[i];
		r_.i8[(i * 2) + 1] = b_.i8[i];
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_unpacklo_epi8(a, b) simde_mm_unpacklo_epi8(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_unpacklo_epi16(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_unpacklo_epi16(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	int16x4_t a1 = vget_low_s16(a_.neon_i16);
	int16x4_t b1 = vget_low_s16(b_.neon_i16);
	int16x4x2_t result = vzip_s16(a1, b1);
	r_.neon_i16 = vcombine_s16(result.val[0], result.val[1]);
#elif defined(SIMDE_SHUFFLE_VECTOR_)
	r_.i16 = SIMDE_SHUFFLE_VECTOR_(16, 16, a_.i16, b_.i16, 0, 8, 1, 9, 2,
				       10, 3, 11);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < ((sizeof(r_) / sizeof(r_.i16[0])) / 2); i++) {
		r_.i16[(i * 2)] = a_.i16[i];
		r_.i16[(i * 2) + 1] = b_.i16[i];
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_unpacklo_epi16(a, b) simde_mm_unpacklo_epi16(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_unpacklo_epi32(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_unpacklo_epi32(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	int32x2_t a1 = vget_low_s32(a_.neon_i32);
	int32x2_t b1 = vget_low_s32(b_.neon_i32);
	int32x2x2_t result = vzip_s32(a1, b1);
	r_.neon_i32 = vcombine_s32(result.val[0], result.val[1]);
#elif defined(SIMDE_SHUFFLE_VECTOR_)
	r_.i32 = SIMDE_SHUFFLE_VECTOR_(32, 16, a_.i32, b_.i32, 0, 4, 1, 5);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < ((sizeof(r_) / sizeof(r_.i32[0])) / 2); i++) {
		r_.i32[(i * 2)] = a_.i32[i];
		r_.i32[(i * 2) + 1] = b_.i32[i];
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_unpacklo_epi32(a, b) simde_mm_unpacklo_epi32(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_unpacklo_epi64(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_unpacklo_epi64(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_SHUFFLE_VECTOR_)
	r_.i64 = SIMDE_SHUFFLE_VECTOR_(64, 16, a_.i64, b_.i64, 0, 2);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < ((sizeof(r_) / sizeof(r_.i64[0])) / 2); i++) {
		r_.i64[(i * 2)] = a_.i64[i];
		r_.i64[(i * 2) + 1] = b_.i64[i];
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_unpacklo_epi64(a, b) simde_mm_unpacklo_epi64(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_unpacklo_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_unpacklo_pd(a, b);
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a),
				 b_ = simde__m128d_to_private(b);

#if defined(SIMDE_SHUFFLE_VECTOR_)
	r_.f64 = SIMDE_SHUFFLE_VECTOR_(64, 16, a_.f64, b_.f64, 0, 2);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < ((sizeof(r_) / sizeof(r_.f64[0])) / 2); i++) {
		r_.f64[(i * 2)] = a_.f64[i];
		r_.f64[(i * 2) + 1] = b_.f64[i];
	}
#endif

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_unpacklo_pd(a, b) simde_mm_unpacklo_pd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128d simde_mm_xor_pd(simde__m128d a, simde__m128d b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_xor_pd(a, b);
#else
	simde__m128d_private r_, a_ = simde__m128d_to_private(a),
				 b_ = simde__m128d_to_private(b);

#if defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.i32f = a_.i32f ^ b_.i32f;
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i32f) / sizeof(r_.i32f[0])); i++) {
		r_.i32f[i] = a_.i32f[i] ^ b_.i32f[i];
	}
#endif

	return simde__m128d_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_xor_pd(a, b) simde_mm_xor_pd(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_mm_xor_si128(simde__m128i a, simde__m128i b)
{
#if defined(SIMDE_X86_SSE2_NATIVE)
	return _mm_xor_si128(a, b);
#else
	simde__m128i_private r_, a_ = simde__m128i_to_private(a),
				 b_ = simde__m128i_to_private(b);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i32 = veorq_s32(a_.neon_i32, b_.neon_i32);
#elif defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
	r_.altivec_i32 = vec_xor(a_.altivec_i32, b_.altivec_i32);
#elif defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.i32f = a_.i32f ^ b_.i32f;
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i32f) / sizeof(r_.i32f[0])); i++) {
		r_.i32f[i] = a_.i32f[i] ^ b_.i32f[i];
	}
#endif

	return simde__m128i_from_private(r_);
#endif
}
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _mm_xor_si128(a, b) simde_mm_xor_si128(a, b)
#endif

SIMDE_FUNCTION_ATTRIBUTES
simde__m128i simde_x_mm_not_si128(simde__m128i a)
{
	simde__m128i_private r_, a_ = simde__m128i_to_private(a);

#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
	r_.neon_i32 = vmvnq_s32(a_.neon_i32);
#elif defined(SIMDE_VECTOR_SUBSCRIPT_OPS)
	r_.i32f = ~(a_.i32f);
#else
	SIMDE_VECTORIZE
	for (size_t i = 0; i < (sizeof(r_.i32f) / sizeof(r_.i32f[0])); i++) {
		r_.i32f[i] = ~(a_.i32f[i]);
	}
#endif

	return simde__m128i_from_private(r_);
}

#define SIMDE_MM_SHUFFLE2(x, y) (((x) << 1) | (y))
#if defined(SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES)
#define _MM_SHUFFLE2(x, y) SIMDE_MM_SHUFFLE2(x, y)
#endif

SIMDE_END_DECLS_

HEDLEY_DIAGNOSTIC_POP

#endif /* !defined(SIMDE_X86_SSE2_H) */
