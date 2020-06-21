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
 *   2020      Evan Nemerson <evan@nemerson.com>
 */

/* simde-arch.h is used to determine which features are available according
   to the compiler.  However, we want to make it possible to forcibly enable
   or disable APIs */

#if !defined(SIMDE_FEATURES_H)
#define SIMDE_FEATURES_H

#include "simde-arch.h"

#if !defined(SIMDE_X86_SVML_NATIVE) && !defined(SIMDE_X86_SVML_NO_NATIVE) && \
	!defined(SIMDE_NO_NATIVE)
#if defined(SIMDE_ARCH_X86_SVML)
#define SIMDE_X86_SVML_NATIVE
#endif
#endif
#if defined(SIMDE_X86_SVML_NATIVE) && !defined(SIMDE_X86_AVX512F_NATIVE)
#define SIMDE_X86_AVX512F_NATIVE
#endif

#if !defined(SIMDE_X86_AVX512CD_NATIVE) && \
	!defined(SIMDE_X86_AVX512CD_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
#if defined(SIMDE_ARCH_X86_AVX512CD)
#define SIMDE_X86_AVX512CD_NATIVE
#endif
#endif
#if defined(SIMDE_X86_AVX512CD_NATIVE) && !defined(SIMDE_X86_AVX512F_NATIVE)
#define SIMDE_X86_AVX512F_NATIVE
#endif

#if !defined(SIMDE_X86_AVX512DQ_NATIVE) && \
	!defined(SIMDE_X86_AVX512DQ_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
#if defined(SIMDE_ARCH_X86_AVX512DQ)
#define SIMDE_X86_AVX512DQ_NATIVE
#endif
#endif
#if defined(SIMDE_X86_AVX512DQ_NATIVE) && !defined(SIMDE_X86_AVX512F_NATIVE)
#define SIMDE_X86_AVX512F_NATIVE
#endif

#if !defined(SIMDE_X86_AVX512VL_NATIVE) && \
	!defined(SIMDE_X86_AVX512VL_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
#if defined(SIMDE_ARCH_X86_AVX512VL)
#define SIMDE_X86_AVX512VL_NATIVE
#endif
#endif
#if defined(SIMDE_X86_AVX512VL_NATIVE) && !defined(SIMDE_X86_AVX512F_NATIVE)
#define SIMDE_X86_AVX512F_NATIVE
#endif

#if !defined(SIMDE_X86_AVX512BW_NATIVE) && \
	!defined(SIMDE_X86_AVX512BW_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
#if defined(SIMDE_ARCH_X86_AVX512BW)
#define SIMDE_X86_AVX512BW_NATIVE
#endif
#endif
#if defined(SIMDE_X86_AVX512BW_NATIVE) && !defined(SIMDE_X86_AVX512F_NATIVE)
#define SIMDE_X86_AVX512F_NATIVE
#endif

#if !defined(SIMDE_X86_AVX512F_NATIVE) && \
	!defined(SIMDE_X86_AVX512F_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
#if defined(SIMDE_ARCH_X86_AVX512F)
#define SIMDE_X86_AVX512F_NATIVE
#endif
#endif
#if defined(SIMDE_X86_AVX512F_NATIVE) && !defined(SIMDE_X86_AVX2_NATIVE)
#define SIMDE_X86_AVX2_NATIVE
#endif

#if !defined(SIMDE_X86_FMA_NATIVE) && !defined(SIMDE_X86_FMA_NO_NATIVE) && \
	!defined(SIMDE_NO_NATIVE)
#if defined(SIMDE_ARCH_X86_FMA)
#define SIMDE_X86_FMA_NATIVE
#endif
#endif
#if defined(SIMDE_X86_FMA_NATIVE) && !defined(SIMDE_X86_AVX_NATIVE)
#define SIMDE_X86_AVX_NATIVE
#endif

#if !defined(SIMDE_X86_AVX2_NATIVE) && !defined(SIMDE_X86_AVX2_NO_NATIVE) && \
	!defined(SIMDE_NO_NATIVE)
#if defined(SIMDE_ARCH_X86_AVX2)
#define SIMDE_X86_AVX2_NATIVE
#endif
#endif
#if defined(SIMDE_X86_AVX2_NATIVE) && !defined(SIMDE_X86_AVX_NATIVE)
#define SIMDE_X86_AVX_NATIVE
#endif

#if !defined(SIMDE_X86_AVX_NATIVE) && !defined(SIMDE_X86_AVX_NO_NATIVE) && \
	!defined(SIMDE_NO_NATIVE)
#if defined(SIMDE_ARCH_X86_AVX)
#define SIMDE_X86_AVX_NATIVE
#endif
#endif
#if defined(SIMDE_X86_AVX_NATIVE) && !defined(SIMDE_X86_SSE4_1_NATIVE)
#define SIMDE_X86_SSE4_2_NATIVE
#endif

#if !defined(SIMDE_X86_SSE4_2_NATIVE) && \
	!defined(SIMDE_X86_SSE4_2_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
#if defined(SIMDE_ARCH_X86_SSE4_2)
#define SIMDE_X86_SSE4_2_NATIVE
#endif
#endif
#if defined(SIMDE_X86_SSE4_2_NATIVE) && !defined(SIMDE_X86_SSE4_1_NATIVE)
#define SIMDE_X86_SSE4_1_NATIVE
#endif

#if !defined(SIMDE_X86_SSE4_1_NATIVE) && \
	!defined(SIMDE_X86_SSE4_1_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
#if defined(SIMDE_ARCH_X86_SSE4_1)
#define SIMDE_X86_SSE4_1_NATIVE
#endif
#endif
#if defined(SIMDE_X86_SSE4_1_NATIVE) && !defined(SIMDE_X86_SSSE3_NATIVE)
#define SIMDE_X86_SSSE3_NATIVE
#endif

#if !defined(SIMDE_X86_SSSE3_NATIVE) && !defined(SIMDE_X86_SSSE3_NO_NATIVE) && \
	!defined(SIMDE_NO_NATIVE)
#if defined(SIMDE_ARCH_X86_SSSE3)
#define SIMDE_X86_SSSE3_NATIVE
#endif
#endif
#if defined(SIMDE_X86_SSSE3_NATIVE) && !defined(SIMDE_X86_SSE3_NATIVE)
#define SIMDE_X86_SSE3_NATIVE
#endif

#if !defined(SIMDE_X86_SSE3_NATIVE) && !defined(SIMDE_X86_SSE3_NO_NATIVE) && \
	!defined(SIMDE_NO_NATIVE)
#if defined(SIMDE_ARCH_X86_SSE3)
#define SIMDE_X86_SSE3_NATIVE
#endif
#endif
#if defined(SIMDE_X86_SSE3_NATIVE) && !defined(SIMDE_X86_SSE2_NATIVE)
#define SIMDE_X86_SSE2_NATIVE
#endif

#if !defined(SIMDE_X86_SSE2_NATIVE) && !defined(SIMDE_X86_SSE2_NO_NATIVE) && \
	!defined(SIMDE_NO_NATIVE)
#if defined(SIMDE_ARCH_X86_SSE2)
#define SIMDE_X86_SSE2_NATIVE
#endif
#endif
#if defined(SIMDE_X86_SSE2_NATIVE) && !defined(SIMDE_X86_SSE_NATIVE)
#define SIMDE_X86_SSE_NATIVE
#endif

#if !defined(SIMDE_X86_SSE_NATIVE) && !defined(SIMDE_X86_SSE_NO_NATIVE) && \
	!defined(SIMDE_NO_NATIVE)
#if defined(SIMDE_ARCH_X86_SSE)
#define SIMDE_X86_SSE_NATIVE
#endif
#endif

#if !defined(SIMDE_X86_MMX_NATIVE) && !defined(SIMDE_X86_MMX_NO_NATIVE) && \
	!defined(SIMDE_NO_NATIVE)
#if defined(SIMDE_ARCH_X86_MMX)
#define SIMDE_X86_MMX_NATIVE
#endif
#endif

#if !defined(SIMDE_X86_GFNI_NATIVE) && !defined(SIMDE_X86_GFNI_NO_NATIVE) && \
	!defined(SIMDE_NO_NATIVE)
#if defined(SIMDE_ARCH_X86_GFNI)
#define SIMDE_X86_GFNI_NATIVE
#endif
#endif

#if !defined(SIMDE_X86_SVML_NATIVE) && !defined(SIMDE_X86_SVML_NO_NATIVE) && \
	!defined(SIMDE_NO_NATIVE)
#if defined(__INTEL_COMPILER)
#define SIMDE_X86_SVML_NATIVE
#endif
#endif

#if defined(HEDLEY_MSVC_VERSION)
#pragma warning(push)
#pragma warning(disable : 4799)
#endif

#if defined(SIMDE_X86_AVX_NATIVE) || defined(SIMDE_X86_GFNI_NATIVE) || \
	defined(SIMDE_X86_SVML_NATIVE)
#include <immintrin.h>
#elif defined(SIMDE_X86_SSE4_2_NATIVE)
#include <nmmintrin.h>
#elif defined(SIMDE_X86_SSE4_1_NATIVE)
#include <smmintrin.h>
#elif defined(SIMDE_X86_SSSE3_NATIVE)
#include <tmmintrin.h>
#elif defined(SIMDE_X86_SSE3_NATIVE)
#include <pmmintrin.h>
#elif defined(SIMDE_X86_SSE2_NATIVE)
#include <emmintrin.h>
#elif defined(SIMDE_X86_SSE_NATIVE)
#include <xmmintrin.h>
#elif defined(SIMDE_X86_MMX_NATIVE)
#include <mmintrin.h>
#endif

#if defined(HEDLEY_MSVC_VERSION)
#pragma warning(pop)
#endif

#if !defined(SIMDE_ARM_NEON_A64V8_NATIVE) && \
	!defined(SIMDE_ARM_NEON_A64V8_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
#if defined(SIMDE_ARCH_ARM_NEON) && defined(SIMDE_ARCH_AARCH64) && \
	SIMDE_ARCH_ARM_CHECK(80)
#define SIMDE_ARM_NEON_A64V8_NATIVE
#endif
#endif
#if defined(SIMDE_ARM_NEON_A64V8_NATIVE) && \
	!defined(SIMDE_ARM_NEON_A32V8_NATIVE)
#define SIMDE_ARM_NEON_A32V8_NATIVE
#endif

#if !defined(SIMDE_ARM_NEON_A32V8_NATIVE) && \
	!defined(SIMDE_ARM_NEON_A32V8_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
#if defined(SIMDE_ARCH_ARM_NEON) && SIMDE_ARCH_ARM_CHECK(80)
#define SIMDE_ARM_NEON_A32V8_NATIVE
#endif
#endif
#if defined(SIMDE_ARM_NEON_A32V8_NATIVE) && \
	!defined(SIMDE_ARM_NEON_A32V7_NATIVE)
#define SIMDE_ARM_NEON_A32V7_NATIVE
#endif

#if !defined(SIMDE_ARM_NEON_A32V7_NATIVE) && \
	!defined(SIMDE_ARM_NEON_A32V7_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
#if defined(SIMDE_ARCH_ARM_NEON) && SIMDE_ARCH_ARM_CHECK(70)
#define SIMDE_ARM_NEON_A32V7_NATIVE
#endif
#endif
#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
#include <arm_neon.h>
#endif

#if !defined(SIMDE_WASM_SIMD128_NATIVE) && \
	!defined(SIMDE_WASM_SIMD128_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
#if defined(SIMDE_ARCH_WASM_SIMD128)
#define SIMDE_WASM_SIMD128_NATIVE
#endif
#endif
#if defined(SIMDE_WASM_SIMD128_NATIVE)
#if !defined(__wasm_unimplemented_simd128__)
#define __wasm_unimplemented_simd128__
#endif
#include <wasm_simd128.h>
#endif

#if !defined(SIMDE_POWER_ALTIVEC_P9_NATIVE) &&        \
	!defined(SIMDE_POWER_ALTIVEC_P9_NO_NATIVE) && \
	!defined(SIMDE_NO_NATIVE)
#if SIMDE_ARCH_POWER_ALTIVEC_CHECK(900)
#define SIMDE_POWER_ALTIVEC_P9_NATIVE
#endif
#endif
#if defined(SIMDE_POWER_ALTIVEC_P9_NATIVE) && !defined(SIMDE_POWER_ALTIVEC_P8)
#define SIMDE_POWER_ALTIVEC_P8_NATIVE
#endif

#if !defined(SIMDE_POWER_ALTIVEC_P8_NATIVE) &&        \
	!defined(SIMDE_POWER_ALTIVEC_P8_NO_NATIVE) && \
	!defined(SIMDE_NO_NATIVE)
#if SIMDE_ARCH_POWER_ALTIVEC_CHECK(800)
#define SIMDE_POWER_ALTIVEC_P8_NATIVE
#endif
#endif
#if defined(SIMDE_POWER_ALTIVEC_P8_NATIVE) && !defined(SIMDE_POWER_ALTIVEC_P7)
#define SIMDE_POWER_ALTIVEC_P7_NATIVE
#endif

#if !defined(SIMDE_POWER_ALTIVEC_P7_NATIVE) &&        \
	!defined(SIMDE_POWER_ALTIVEC_P7_NO_NATIVE) && \
	!defined(SIMDE_NO_NATIVE)
#if SIMDE_ARCH_POWER_ALTIVEC_CHECK(700)
#define SIMDE_POWER_ALTIVEC_P7_NATIVE
#endif
#endif
#if defined(SIMDE_POWER_ALTIVEC_P7_NATIVE) && !defined(SIMDE_POWER_ALTIVEC_P6)
#define SIMDE_POWER_ALTIVEC_P6_NATIVE
#endif

#if !defined(SIMDE_POWER_ALTIVEC_P6_NATIVE) &&        \
	!defined(SIMDE_POWER_ALTIVEC_P6_NO_NATIVE) && \
	!defined(SIMDE_NO_NATIVE)
#if SIMDE_ARCH_POWER_ALTIVEC_CHECK(600)
#define SIMDE_POWER_ALTIVEC_P6_NATIVE
#endif
#endif
#if defined(SIMDE_POWER_ALTIVEC_P6_NATIVE) && !defined(SIMDE_POWER_ALTIVEC_P5)
#define SIMDE_POWER_ALTIVEC_P5_NATIVE
#endif

#if !defined(SIMDE_POWER_ALTIVEC_P5_NATIVE) &&        \
	!defined(SIMDE_POWER_ALTIVEC_P5_NO_NATIVE) && \
	!defined(SIMDE_NO_NATIVE)
#if SIMDE_ARCH_POWER_ALTIVEC_CHECK(500)
#define SIMDE_POWER_ALTIVEC_P5_NATIVE
#endif
#endif
#if defined(SIMDE_POWER_ALTIVEC_P5_NATIVE)
/* stdbool.h conflicts with the bool in altivec.h */
#if defined(bool) && !defined(SIMDE_POWER_ALTIVEC_NO_UNDEF_BOOL_)
#undef bool
#endif
#include <altivec.h>
/* GCC allows you to undefine these macros to prevent conflicts with
   * standard types as they become context-sensitive keywords. */
#if defined(__cplusplus)
#if defined(vector)
#undef vector
#endif
#if defined(pixel)
#undef pixel
#endif
#if defined(bool)
#undef bool
#endif
#define SIMDE_POWER_ALTIVEC_VECTOR(T) vector T
#define SIMDE_POWER_ALTIVEC_PIXEL pixel
#define SIMDE_POWER_ALTIVEC_BOOL bool
#else
#define SIMDE_POWER_ALTIVEC_VECTOR(T) __vector T
#define SIMDE_POWER_ALTIVEC_PIXEL __pixel
#define SIMDE_POWER_ALTIVEC_BOOL __bool
#endif /* defined(__cplusplus) */
#endif

#endif /* !defined(SIMDE_FEATURES_H) */
