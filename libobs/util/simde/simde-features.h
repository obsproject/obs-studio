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
 *   2023      Ju-Hung Li <jhlee@pllab.cs.nthu.edu.tw> (Copyright owned by NTHU pllab)
 */

/* simde-arch.h is used to determine which features are available according
   to the compiler.  However, we want to make it possible to forcibly enable
   or disable APIs */

#if !defined(SIMDE_FEATURES_H)
#define SIMDE_FEATURES_H

#include "simde-arch.h"
#include "simde-diagnostic.h"

#if !defined(SIMDE_X86_SVML_NATIVE) && !defined(SIMDE_X86_SVML_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_X86_SVML)
    #define SIMDE_X86_SVML_NATIVE
  #endif
#endif

#if !defined(SIMDE_X86_AVX512VP2INTERSECT_NATIVE) && !defined(SIMDE_X86_AVX512VP2INTERSECT_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_X86_AVX512VP2INTERSECT)
    #define SIMDE_X86_AVX512VP2INTERSECT_NATIVE
  #endif
#endif
#if defined(SIMDE_X86_AVX512VP2INTERSECT_NATIVE) && !defined(SIMDE_X86_AVX512F_NATIVE)
  #define SIMDE_X86_AVX512F_NATIVE
#endif

#if !defined(SIMDE_X86_AVX512VPOPCNTDQ_NATIVE) && !defined(SIMDE_X86_AVX512VPOPCNTDQ_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_X86_AVX512VPOPCNTDQ)
    #define SIMDE_X86_AVX512VPOPCNTDQ_NATIVE
  #endif
#endif
#if defined(SIMDE_X86_AVX512VPOPCNTDQ_NATIVE) && !defined(SIMDE_X86_AVX512F_NATIVE)
  #define SIMDE_X86_AVX512F_NATIVE
#endif

#if !defined(SIMDE_X86_AVX512BITALG_NATIVE) && !defined(SIMDE_X86_AVX512BITALG_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_X86_AVX512BITALG)
    #define SIMDE_X86_AVX512BITALG_NATIVE
  #endif
#endif
#if defined(SIMDE_X86_AVX512BITALG_NATIVE) && !defined(SIMDE_X86_AVX512F_NATIVE)
  #define SIMDE_X86_AVX512F_NATIVE
#endif

#if !defined(SIMDE_X86_AVX512VBMI_NATIVE) && !defined(SIMDE_X86_AVX512VBMI_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_X86_AVX512VBMI)
    #define SIMDE_X86_AVX512VBMI_NATIVE
  #endif
#endif
#if defined(SIMDE_X86_AVX512VBMI_NATIVE) && !defined(SIMDE_X86_AVX512F_NATIVE)
  #define SIMDE_X86_AVX512F_NATIVE
#endif

#if !defined(SIMDE_X86_AVX512VBMI2_NATIVE) && !defined(SIMDE_X86_AVX512VBMI2_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_X86_AVX512VBMI2)
    #define SIMDE_X86_AVX512VBMI2_NATIVE
  #endif
#endif
#if defined(SIMDE_X86_AVX512VBMI2_NATIVE) && !defined(SIMDE_X86_AVX512F_NATIVE)
  #define SIMDE_X86_AVX512F_NATIVE
#endif

#if !defined(SIMDE_X86_AVX512VNNI_NATIVE) && !defined(SIMDE_X86_AVX512VNNI_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_X86_AVX512VNNI)
    #define SIMDE_X86_AVX512VNNI_NATIVE
  #endif
#endif
#if defined(SIMDE_X86_AVX512VNNI_NATIVE) && !defined(SIMDE_X86_AVX512F_NATIVE)
  #define SIMDE_X86_AVX512F_NATIVE
#endif

#if !defined(SIMDE_X86_AVX5124VNNIW_NATIVE) && !defined(SIMDE_X86_AVX5124VNNIW_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_X86_AVX5124VNNIW)
    #define SIMDE_X86_AVX5124VNNIW_NATIVE
  #endif
#endif
#if defined(SIMDE_X86_AVX5124VNNIW_NATIVE) && !defined(SIMDE_X86_AVX512F_NATIVE)
  #define SIMDE_X86_AVX512F_NATIVE
#endif

#if !defined(SIMDE_X86_AVX512CD_NATIVE) && !defined(SIMDE_X86_AVX512CD_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_X86_AVX512CD)
    #define SIMDE_X86_AVX512CD_NATIVE
  #endif
#endif
#if defined(SIMDE_X86_AVX512CD_NATIVE) && !defined(SIMDE_X86_AVX512F_NATIVE)
  #define SIMDE_X86_AVX512F_NATIVE
#endif

#if !defined(SIMDE_X86_AVX512DQ_NATIVE) && !defined(SIMDE_X86_AVX512DQ_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_X86_AVX512DQ)
    #define SIMDE_X86_AVX512DQ_NATIVE
  #endif
#endif
#if defined(SIMDE_X86_AVX512DQ_NATIVE) && !defined(SIMDE_X86_AVX512F_NATIVE)
  #define SIMDE_X86_AVX512F_NATIVE
#endif

#if !defined(SIMDE_X86_AVX512VL_NATIVE) && !defined(SIMDE_X86_AVX512VL_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_X86_AVX512VL)
    #define SIMDE_X86_AVX512VL_NATIVE
  #endif
#endif
#if defined(SIMDE_X86_AVX512VL_NATIVE) && !defined(SIMDE_X86_AVX512F_NATIVE)
  #define SIMDE_X86_AVX512F_NATIVE
#endif

#if !defined(SIMDE_X86_AVX512BW_NATIVE) && !defined(SIMDE_X86_AVX512BW_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_X86_AVX512BW)
    #define SIMDE_X86_AVX512BW_NATIVE
  #endif
#endif
#if defined(SIMDE_X86_AVX512BW_NATIVE) && !defined(SIMDE_X86_AVX512F_NATIVE)
  #define SIMDE_X86_AVX512F_NATIVE
#endif

#if !defined(SIMDE_X86_AVX512FP16_NATIVE) && !defined(SIMDE_X86_AVX512FP16_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_X86_AVX512FP16)
    #define SIMDE_X86_AVX512FP16_NATIVE
  #endif
#endif
#if defined(SIMDE_X86_AVX512BW_NATIVE) && !defined(SIMDE_X86_AVX512F_NATIVE)
  #define SIMDE_X86_AVX512F_NATIVE
#endif

#if !defined(SIMDE_X86_AVX512BF16_NATIVE) && !defined(SIMDE_X86_AVX512BF16_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_X86_AVX512BF16)
    #define SIMDE_X86_AVX512BF16_NATIVE
  #endif
#endif
#if defined(SIMDE_X86_AVX512BF16_NATIVE) && !defined(SIMDE_X86_AVX512F_NATIVE)
  #define SIMDE_X86_AVX512F_NATIVE
#endif

#if !defined(SIMDE_X86_AVX512F_NATIVE) && !defined(SIMDE_X86_AVX512F_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_X86_AVX512F)
    #define SIMDE_X86_AVX512F_NATIVE
  #endif
#endif
#if defined(SIMDE_X86_AVX512F_NATIVE) && !defined(SIMDE_X86_AVX2_NATIVE)
  #define SIMDE_X86_AVX2_NATIVE
#endif

#if !defined(SIMDE_X86_FMA_NATIVE) && !defined(SIMDE_X86_FMA_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_X86_FMA)
    #define SIMDE_X86_FMA_NATIVE
  #endif
#endif
#if defined(SIMDE_X86_FMA_NATIVE) && !defined(SIMDE_X86_AVX_NATIVE)
  #define SIMDE_X86_AVX_NATIVE
#endif

#if !defined(SIMDE_X86_AVX2_NATIVE) && !defined(SIMDE_X86_AVX2_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_X86_AVX2)
    #define SIMDE_X86_AVX2_NATIVE
  #endif
#endif
#if defined(SIMDE_X86_AVX2_NATIVE) && !defined(SIMDE_X86_AVX_NATIVE)
  #define SIMDE_X86_AVX_NATIVE
#endif

#if !defined(SIMDE_X86_AVX_NATIVE) && !defined(SIMDE_X86_AVX_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_X86_AVX)
    #define SIMDE_X86_AVX_NATIVE
  #endif
#endif
#if defined(SIMDE_X86_AVX_NATIVE) && !defined(SIMDE_X86_SSE4_2_NATIVE)
  #define SIMDE_X86_SSE4_2_NATIVE
#endif

#if !defined(SIMDE_X86_XOP_NATIVE) && !defined(SIMDE_X86_XOP_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_X86_XOP)
    #define SIMDE_X86_XOP_NATIVE
  #endif
#endif
#if defined(SIMDE_X86_XOP_NATIVE) && !defined(SIMDE_X86_SSE4_2_NATIVE)
  #define SIMDE_X86_SSE4_2_NATIVE
#endif

#if !defined(SIMDE_X86_SSE4_2_NATIVE) && !defined(SIMDE_X86_SSE4_2_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_X86_SSE4_2)
    #define SIMDE_X86_SSE4_2_NATIVE
  #endif
#endif
#if defined(SIMDE_X86_SSE4_2_NATIVE) && !defined(SIMDE_X86_SSE4_1_NATIVE)
  #define SIMDE_X86_SSE4_1_NATIVE
#endif

#if !defined(SIMDE_X86_SSE4_1_NATIVE) && !defined(SIMDE_X86_SSE4_1_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_X86_SSE4_1)
    #define SIMDE_X86_SSE4_1_NATIVE
  #endif
#endif
#if defined(SIMDE_X86_SSE4_1_NATIVE) && !defined(SIMDE_X86_SSSE3_NATIVE)
  #define SIMDE_X86_SSSE3_NATIVE
#endif

#if !defined(SIMDE_X86_SSSE3_NATIVE) && !defined(SIMDE_X86_SSSE3_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_X86_SSSE3)
    #define SIMDE_X86_SSSE3_NATIVE
  #endif
#endif
#if defined(SIMDE_X86_SSSE3_NATIVE) && !defined(SIMDE_X86_SSE3_NATIVE)
  #define SIMDE_X86_SSE3_NATIVE
#endif

#if !defined(SIMDE_X86_SSE3_NATIVE) && !defined(SIMDE_X86_SSE3_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_X86_SSE3)
    #define SIMDE_X86_SSE3_NATIVE
  #endif
#endif
#if defined(SIMDE_X86_SSE3_NATIVE) && !defined(SIMDE_X86_SSE2_NATIVE)
  #define SIMDE_X86_SSE2_NATIVE
#endif

#if !defined(SIMDE_X86_AES_NATIVE) && !defined(SIMDE_X86_AES_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_X86_AES)
    #define SIMDE_X86_AES_NATIVE
  #endif
#endif
#if defined(SIMDE_X86_AES_NATIVE) && !defined(SIMDE_X86_SSE2_NATIVE)
  #define SIMDE_X86_SSE2_NATIVE
#endif

#if !defined(SIMDE_X86_SSE2_NATIVE) && !defined(SIMDE_X86_SSE2_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_X86_SSE2)
    #define SIMDE_X86_SSE2_NATIVE
  #endif
#endif
#if defined(SIMDE_X86_SSE2_NATIVE) && !defined(SIMDE_X86_SSE_NATIVE)
  #define SIMDE_X86_SSE_NATIVE
#endif

#if !defined(SIMDE_X86_SSE_NATIVE) && !defined(SIMDE_X86_SSE_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_X86_SSE)
    #define SIMDE_X86_SSE_NATIVE
  #endif
#endif

#if !defined(SIMDE_X86_MMX_NATIVE) && !defined(SIMDE_X86_MMX_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_X86_MMX)
    #define SIMDE_X86_MMX_NATIVE
  #endif
#endif

#if !defined(SIMDE_X86_GFNI_NATIVE) && !defined(SIMDE_X86_GFNI_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_X86_GFNI)
    #define SIMDE_X86_GFNI_NATIVE
  #endif
#endif

#if !defined(SIMDE_X86_PCLMUL_NATIVE) && !defined(SIMDE_X86_PCLMUL_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_X86_PCLMUL)
    #define SIMDE_X86_PCLMUL_NATIVE
  #endif
#endif

#if !defined(SIMDE_X86_VPCLMULQDQ_NATIVE) && !defined(SIMDE_X86_VPCLMULQDQ_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_X86_VPCLMULQDQ)
    #define SIMDE_X86_VPCLMULQDQ_NATIVE
  #endif
#endif

#if !defined(SIMDE_X86_F16C_NATIVE) && !defined(SIMDE_X86_F16C_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_X86_F16C)
    #define SIMDE_X86_F16C_NATIVE
  #endif
#endif

#if !defined(SIMDE_X86_SVML_NATIVE) && !defined(SIMDE_X86_SVML_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_X86) && (defined(__INTEL_COMPILER) || (HEDLEY_MSVC_VERSION_CHECK(14, 20, 0) && !defined(__clang__)))
    #define SIMDE_X86_SVML_NATIVE
  #endif
#endif

#if defined(HEDLEY_MSVC_VERSION)
  #pragma warning(push)
  #pragma warning(disable:4799)
#endif

#if \
    defined(SIMDE_X86_AVX_NATIVE) || defined(SIMDE_X86_GFNI_NATIVE) || defined(SIMDE_X86_SVML_NATIVE)
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

#if defined(SIMDE_X86_XOP_NATIVE)
  #if defined(_MSC_VER)
    #include <intrin.h>
  #else
    #include <x86intrin.h>
  #endif
#endif

#if defined(SIMDE_X86_AES_NATIVE)
  #include <wmmintrin.h>
#endif

#if defined(HEDLEY_MSVC_VERSION)
  #pragma warning(pop)
#endif

#if !defined(SIMDE_ARM_NEON_A64V8_NATIVE) && !defined(SIMDE_ARM_NEON_A64V8_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_ARM_NEON) && defined(SIMDE_ARCH_AARCH64) && SIMDE_ARCH_ARM_CHECK(8,0)
    #define SIMDE_ARM_NEON_A64V8_NATIVE
  #endif
#endif
#if defined(SIMDE_ARM_NEON_A64V8_NATIVE) && !defined(SIMDE_ARM_NEON_A32V8_NATIVE)
  #define SIMDE_ARM_NEON_A32V8_NATIVE
#endif

#if !defined(SIMDE_ARM_NEON_A32V8_NATIVE) && !defined(SIMDE_ARM_NEON_A32V8_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_ARM_NEON) && SIMDE_ARCH_ARM_CHECK(8,0) && (__ARM_NEON_FP & 0x02)
    #define SIMDE_ARM_NEON_A32V8_NATIVE
  #endif
#endif
#if defined(__ARM_ACLE)
  #include <arm_acle.h>
#endif
#if defined(SIMDE_ARM_NEON_A32V8_NATIVE) && !defined(SIMDE_ARM_NEON_A32V7_NATIVE)
  #define SIMDE_ARM_NEON_A32V7_NATIVE
#endif

#if !defined(SIMDE_ARM_NEON_A32V7_NATIVE) && !defined(SIMDE_ARM_NEON_A32V7_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_ARM_NEON) && SIMDE_ARCH_ARM_CHECK(7,0)
    #define SIMDE_ARM_NEON_A32V7_NATIVE
  #endif
#endif
#if defined(SIMDE_ARM_NEON_A32V7_NATIVE)
  #include <arm_neon.h>
  #if defined(__ARM_FEATURE_FP16_VECTOR_ARITHMETIC)
    #include <arm_fp16.h>
  #endif
#endif

#if !defined(SIMDE_ARM_SVE_NATIVE) && !defined(SIMDE_ARM_SVE_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_ARM_SVE)
    #define SIMDE_ARM_SVE_NATIVE
    #include <arm_sve.h>
  #endif
#endif

#if !defined(SIMDE_RISCV_V_NATIVE) && !defined(SIMDE_RISCV_V_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_RISCV_V)
    #define SIMDE_RISCV_V_NATIVE
  #endif
#endif
#if defined(SIMDE_RISCV_V_NATIVE)
  #include <riscv_vector.h>
#endif

#if !defined(SIMDE_WASM_SIMD128_NATIVE) && !defined(SIMDE_WASM_SIMD128_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_WASM_SIMD128)
    #define SIMDE_WASM_SIMD128_NATIVE
  #endif
#endif

#if !defined(SIMDE_WASM_RELAXED_SIMD_NATIVE) && !defined(SIMDE_WASM_RELAXED_SIMD_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_WASM_RELAXED_SIMD)
    #define SIMDE_WASM_RELAXED_SIMD_NATIVE
  #endif
#endif
#if defined(SIMDE_WASM_SIMD128_NATIVE) || defined(SIMDE_WASM_RELAXED_SIMD_NATIVE)
  #include <wasm_simd128.h>
#endif

#if !defined(SIMDE_POWER_ALTIVEC_P9_NATIVE) && !defined(SIMDE_POWER_ALTIVEC_P9_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if SIMDE_ARCH_POWER_ALTIVEC_CHECK(900)
    #define SIMDE_POWER_ALTIVEC_P9_NATIVE
  #endif
#endif
#if defined(SIMDE_POWER_ALTIVEC_P9_NATIVE) && !defined(SIMDE_POWER_ALTIVEC_P8)
  #define SIMDE_POWER_ALTIVEC_P8_NATIVE
#endif

#if !defined(SIMDE_POWER_ALTIVEC_P8_NATIVE) && !defined(SIMDE_POWER_ALTIVEC_P8_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if SIMDE_ARCH_POWER_ALTIVEC_CHECK(800)
    #define SIMDE_POWER_ALTIVEC_P8_NATIVE
  #endif
#endif
#if defined(SIMDE_POWER_ALTIVEC_P8_NATIVE) && !defined(SIMDE_POWER_ALTIVEC_P7)
  #define SIMDE_POWER_ALTIVEC_P7_NATIVE
#endif

#if !defined(SIMDE_POWER_ALTIVEC_P7_NATIVE) && !defined(SIMDE_POWER_ALTIVEC_P7_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if SIMDE_ARCH_POWER_ALTIVEC_CHECK(700)
    #define SIMDE_POWER_ALTIVEC_P7_NATIVE
  #endif
#endif
#if defined(SIMDE_POWER_ALTIVEC_P7_NATIVE) && !defined(SIMDE_POWER_ALTIVEC_P6)
  #define SIMDE_POWER_ALTIVEC_P6_NATIVE
#endif

#if !defined(SIMDE_POWER_ALTIVEC_P6_NATIVE) && !defined(SIMDE_POWER_ALTIVEC_P6_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if SIMDE_ARCH_POWER_ALTIVEC_CHECK(600)
    #define SIMDE_POWER_ALTIVEC_P6_NATIVE
  #endif
#endif
#if defined(SIMDE_POWER_ALTIVEC_P6_NATIVE) && !defined(SIMDE_POWER_ALTIVEC_P5)
  #define SIMDE_POWER_ALTIVEC_P5_NATIVE
#endif

#if !defined(SIMDE_POWER_ALTIVEC_P5_NATIVE) && !defined(SIMDE_POWER_ALTIVEC_P5_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if SIMDE_ARCH_POWER_ALTIVEC_CHECK(500)
    #define SIMDE_POWER_ALTIVEC_P5_NATIVE
  #endif
#endif

#if !defined(SIMDE_ZARCH_ZVECTOR_15_NATIVE) && !defined(SIMDE_ZARCH_ZVECTOR_15_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if SIMDE_ARCH_ZARCH_CHECK(13) && defined(SIMDE_ARCH_ZARCH_ZVECTOR)
    #define SIMDE_ZARCH_ZVECTOR_15_NATIVE
  #endif
#endif

#if !defined(SIMDE_ZARCH_ZVECTOR_14_NATIVE) && !defined(SIMDE_ZARCH_ZVECTOR_14_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if SIMDE_ARCH_ZARCH_CHECK(12) && defined(SIMDE_ARCH_ZARCH_ZVECTOR)
    #define SIMDE_ZARCH_ZVECTOR_14_NATIVE
  #endif
#endif

#if !defined(SIMDE_ZARCH_ZVECTOR_13_NATIVE) && !defined(SIMDE_ZARCH_ZVECTOR_13_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if SIMDE_ARCH_ZARCH_CHECK(11) && defined(SIMDE_ARCH_ZARCH_ZVECTOR)
    #define SIMDE_ZARCH_ZVECTOR_13_NATIVE
  #endif
#endif

#if defined(SIMDE_POWER_ALTIVEC_P6_NATIVE) || defined(SIMDE_ZARCH_ZVECTOR_13_NATIVE)
  /* AltiVec conflicts with lots of stuff.  The bool keyword conflicts
   * with the bool keyword in C++ and the bool macro in C99+ (defined
   * in stdbool.h).  The vector keyword conflicts with std::vector in
   * C++ if you are `using std;`.
   *
   * Luckily AltiVec allows you to use `__vector`/`__bool`/`__pixel`
   * instead, but altivec.h will unconditionally define
   * `vector`/`bool`/`pixel` so we need to work around that.
   *
   * Unfortunately this means that if your code uses AltiVec directly
   * it may break.  If this is the case you'll want to define
   * `SIMDE_POWER_ALTIVEC_NO_UNDEF` before including SIMDe.  Or, even
   * better, port your code to use the double-underscore versions. */
  #if defined(bool)
    #undef bool
  #endif

  #if defined(SIMDE_POWER_ALTIVEC_P6_NATIVE)
    #include <altivec.h>

    #if !defined(SIMDE_POWER_ALTIVEC_NO_UNDEF)
      #if defined(vector)
        #undef vector
      #endif
      #if defined(pixel)
        #undef pixel
      #endif
      #if defined(bool)
        #undef bool
      #endif
    #endif /* !defined(SIMDE_POWER_ALTIVEC_NO_UNDEF) */
  #elif defined(SIMDE_ZARCH_ZVECTOR_13_NATIVE)
    #include <vecintrin.h>
  #endif

  /* Use these intsead of vector/pixel/bool in SIMDe. */
  #define SIMDE_POWER_ALTIVEC_VECTOR(T) __vector T
  #define SIMDE_POWER_ALTIVEC_PIXEL __pixel
  #define SIMDE_POWER_ALTIVEC_BOOL __bool

  /* Re-define bool if we're using stdbool.h */
  #if !defined(__cplusplus) && defined(__bool_true_false_are_defined) && !defined(SIMDE_POWER_ALTIVEC_NO_UNDEF)
    #define bool _Bool
  #endif
#endif

#if !defined(SIMDE_MIPS_LOONGSON_MMI_NATIVE) && !defined(SIMDE_MIPS_LOONGSON_MMI_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_MIPS_LOONGSON_MMI)
    #define SIMDE_MIPS_LOONGSON_MMI_NATIVE  1
  #endif
#endif
#if defined(SIMDE_MIPS_LOONGSON_MMI_NATIVE)
  #include <loongson-mmiintrin.h>
#endif

#if !defined(SIMDE_MIPS_MSA_NATIVE) && !defined(SIMDE_MIPS_MSA_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_MIPS_MSA)
    #define SIMDE_MIPS_MSA_NATIVE  1
  #endif
#endif
#if defined(SIMDE_MIPS_MSA_NATIVE)
  #include <msa.h>
#endif

/* This is used to determine whether or not to fall back on a vector
 * function in an earlier ISA extensions, as well as whether
 * we expected any attempts at vectorization to be fruitful or if we
 * expect to always be running serial code.
 *
 * Note that, for some architectures (okay, *one* architecture) there
 * can be a split where some types are supported for one vector length
 * but others only for a shorter length.  Therefore, it is possible to
 * provide separate values for float/int/double types. */

#if !defined(SIMDE_NATURAL_VECTOR_SIZE)
  #if defined(SIMDE_X86_AVX512F_NATIVE)
    #define SIMDE_NATURAL_VECTOR_SIZE (512)
  #elif defined(SIMDE_X86_AVX2_NATIVE)
    #define SIMDE_NATURAL_VECTOR_SIZE (256)
  #elif defined(SIMDE_X86_AVX_NATIVE)
    #define SIMDE_NATURAL_FLOAT_VECTOR_SIZE (256)
    #define SIMDE_NATURAL_INT_VECTOR_SIZE (128)
    #define SIMDE_NATURAL_DOUBLE_VECTOR_SIZE (128)
  #elif \
      defined(SIMDE_X86_SSE2_NATIVE) || \
      defined(SIMDE_ARM_NEON_A32V7_NATIVE) || \
      defined(SIMDE_WASM_SIMD128_NATIVE) || \
      defined(SIMDE_POWER_ALTIVEC_P5_NATIVE) || \
      defined(SIMDE_ZARCH_ZVECTOR_13_NATIVE) || \
      defined(SIMDE_MIPS_MSA_NATIVE)
    #define SIMDE_NATURAL_VECTOR_SIZE (128)
  #elif defined(SIMDE_X86_SSE_NATIVE)
    #define SIMDE_NATURAL_FLOAT_VECTOR_SIZE (128)
    #define SIMDE_NATURAL_INT_VECTOR_SIZE (64)
    #define SIMDE_NATURAL_DOUBLE_VECTOR_SIZE (0)
  #elif defined(SIMDE_RISCV_V_NATIVE) && defined(__riscv_v_fixed_vlen)
        //FIXME : SIMDE_NATURAL_VECTOR_SIZE == __riscv_v_fixed_vlen
        #define SIMDE_NATURAL_VECTOR_SIZE (128)
  #endif

  #if !defined(SIMDE_NATURAL_VECTOR_SIZE)
    #if defined(SIMDE_NATURAL_FLOAT_VECTOR_SIZE)
      #define SIMDE_NATURAL_VECTOR_SIZE SIMDE_NATURAL_FLOAT_VECTOR_SIZE
    #elif defined(SIMDE_NATURAL_INT_VECTOR_SIZE)
      #define SIMDE_NATURAL_VECTOR_SIZE SIMDE_NATURAL_INT_VECTOR_SIZE
    #elif defined(SIMDE_NATURAL_DOUBLE_VECTOR_SIZE)
      #define SIMDE_NATURAL_VECTOR_SIZE SIMDE_NATURAL_DOUBLE_VECTOR_SIZE
    #else
      #define SIMDE_NATURAL_VECTOR_SIZE (0)
    #endif
  #endif

  #if !defined(SIMDE_NATURAL_FLOAT_VECTOR_SIZE)
    #define SIMDE_NATURAL_FLOAT_VECTOR_SIZE SIMDE_NATURAL_VECTOR_SIZE
  #endif
  #if !defined(SIMDE_NATURAL_INT_VECTOR_SIZE)
    #define SIMDE_NATURAL_INT_VECTOR_SIZE SIMDE_NATURAL_VECTOR_SIZE
  #endif
  #if !defined(SIMDE_NATURAL_DOUBLE_VECTOR_SIZE)
    #define SIMDE_NATURAL_DOUBLE_VECTOR_SIZE SIMDE_NATURAL_VECTOR_SIZE
  #endif
#endif

#define SIMDE_NATURAL_VECTOR_SIZE_LE(x) ((SIMDE_NATURAL_VECTOR_SIZE > 0) && (SIMDE_NATURAL_VECTOR_SIZE <= (x)))
#define SIMDE_NATURAL_VECTOR_SIZE_GE(x) ((SIMDE_NATURAL_VECTOR_SIZE > 0) && (SIMDE_NATURAL_VECTOR_SIZE >= (x)))
#define SIMDE_NATURAL_FLOAT_VECTOR_SIZE_LE(x) ((SIMDE_NATURAL_FLOAT_VECTOR_SIZE > 0) && (SIMDE_NATURAL_FLOAT_VECTOR_SIZE <= (x)))
#define SIMDE_NATURAL_FLOAT_VECTOR_SIZE_GE(x) ((SIMDE_NATURAL_FLOAT_VECTOR_SIZE > 0) && (SIMDE_NATURAL_FLOAT_VECTOR_SIZE >= (x)))
#define SIMDE_NATURAL_INT_VECTOR_SIZE_LE(x) ((SIMDE_NATURAL_INT_VECTOR_SIZE > 0) && (SIMDE_NATURAL_INT_VECTOR_SIZE <= (x)))
#define SIMDE_NATURAL_INT_VECTOR_SIZE_GE(x) ((SIMDE_NATURAL_INT_VECTOR_SIZE > 0) && (SIMDE_NATURAL_INT_VECTOR_SIZE >= (x)))
#define SIMDE_NATURAL_DOUBLE_VECTOR_SIZE_LE(x) ((SIMDE_NATURAL_DOUBLE_VECTOR_SIZE > 0) && (SIMDE_NATURAL_DOUBLE_VECTOR_SIZE <= (x)))
#define SIMDE_NATURAL_DOUBLE_VECTOR_SIZE_GE(x) ((SIMDE_NATURAL_DOUBLE_VECTOR_SIZE > 0) && (SIMDE_NATURAL_DOUBLE_VECTOR_SIZE >= (x)))

/* Native aliases */
#if defined(SIMDE_ENABLE_NATIVE_ALIASES)
  #if !defined(SIMDE_X86_MMX_NATIVE)
    #define SIMDE_X86_MMX_ENABLE_NATIVE_ALIASES
  #endif
  #if !defined(SIMDE_X86_SSE_NATIVE)
    #define SIMDE_X86_SSE_ENABLE_NATIVE_ALIASES
  #endif
  #if !defined(SIMDE_X86_SSE2_NATIVE)
    #define SIMDE_X86_SSE2_ENABLE_NATIVE_ALIASES
  #endif
  #if !defined(SIMDE_X86_SSE3_NATIVE)
    #define SIMDE_X86_SSE3_ENABLE_NATIVE_ALIASES
  #endif
  #if !defined(SIMDE_X86_SSSE3_NATIVE)
    #define SIMDE_X86_SSSE3_ENABLE_NATIVE_ALIASES
  #endif
  #if !defined(SIMDE_X86_SSE4_1_NATIVE)
    #define SIMDE_X86_SSE4_1_ENABLE_NATIVE_ALIASES
  #endif
  #if !defined(SIMDE_X86_SSE4_2_NATIVE)
    #define SIMDE_X86_SSE4_2_ENABLE_NATIVE_ALIASES
  #endif
  #if !defined(SIMDE_X86_AVX_NATIVE)
    #define SIMDE_X86_AVX_ENABLE_NATIVE_ALIASES
  #endif
  #if !defined(SIMDE_X86_AVX2_NATIVE)
    #define SIMDE_X86_AVX2_ENABLE_NATIVE_ALIASES
  #endif
  #if !defined(SIMDE_X86_FMA_NATIVE)
    #define SIMDE_X86_FMA_ENABLE_NATIVE_ALIASES
  #endif
  #if !defined(SIMDE_X86_AVX512F_NATIVE)
    #define SIMDE_X86_AVX512F_ENABLE_NATIVE_ALIASES
  #endif
  #if !defined(SIMDE_X86_AVX512VL_NATIVE)
    #define SIMDE_X86_AVX512VL_ENABLE_NATIVE_ALIASES
  #endif
  #if !defined(SIMDE_X86_AVX512VBMI_NATIVE)
    #define SIMDE_X86_AVX512VBMI_ENABLE_NATIVE_ALIASES
  #endif
  #if !defined(SIMDE_X86_AVX512VBMI2_NATIVE)
    #define SIMDE_X86_AVX512VBMI2_ENABLE_NATIVE_ALIASES
  #endif
  #if !defined(SIMDE_X86_AVX512BW_NATIVE)
    #define SIMDE_X86_AVX512BW_ENABLE_NATIVE_ALIASES
  #endif
  #if !defined(SIMDE_X86_AVX512VNNI_NATIVE)
    #define SIMDE_X86_AVX512VNNI_ENABLE_NATIVE_ALIASES
  #endif
  #if !defined(SIMDE_X86_AVX5124VNNIW_NATIVE)
    #define SIMDE_X86_AVX5124VNNIW_ENABLE_NATIVE_ALIASES
  #endif
  #if !defined(SIMDE_X86_AVX512BF16_NATIVE)
    #define SIMDE_X86_AVX512BF16_ENABLE_NATIVE_ALIASES
  #endif
  #if !defined(SIMDE_X86_AVX512BITALG_NATIVE)
    #define SIMDE_X86_AVX512BITALG_ENABLE_NATIVE_ALIASES
  #endif
  #if !defined(SIMDE_X86_AVX512VPOPCNTDQ_NATIVE)
    #define SIMDE_X86_AVX512VPOPCNTDQ_ENABLE_NATIVE_ALIASES
  #endif
  #if !defined(SIMDE_X86_AVX512VP2INTERSECT_NATIVE)
    #define SIMDE_X86_AVX512VP2INTERSECT_ENABLE_NATIVE_ALIASES
  #endif
  #if !defined(SIMDE_X86_AVX512DQ_NATIVE)
    #define SIMDE_X86_AVX512DQ_ENABLE_NATIVE_ALIASES
  #endif
  #if !defined(SIMDE_X86_AVX512CD_NATIVE)
    #define SIMDE_X86_AVX512CD_ENABLE_NATIVE_ALIASES
  #endif
  #if !defined(SIMDE_X86_AVX512FP16_NATIVE)
    #define SIMDE_X86_AVX512FP16_ENABLE_NATIVE_ALIASES
  #endif
  #if !defined(SIMDE_X86_GFNI_NATIVE)
    #define SIMDE_X86_GFNI_ENABLE_NATIVE_ALIASES
  #endif
  #if !defined(SIMDE_X86_PCLMUL_NATIVE)
    #define SIMDE_X86_PCLMUL_ENABLE_NATIVE_ALIASES
  #endif
  #if !defined(SIMDE_X86_VPCLMULQDQ_NATIVE)
    #define SIMDE_X86_VPCLMULQDQ_ENABLE_NATIVE_ALIASES
  #endif
  #if !defined(SIMDE_X86_F16C_NATIVE)
    #define SIMDE_X86_F16C_ENABLE_NATIVE_ALIASES
  #endif
  #if !defined(SIMDE_X86_AES_NATIVE)
    #define SIMDE_X86_AES_ENABLE_NATIVE_ALIASES
  #endif
  #if !defined(SIMDE_X86_SVML_NATIVE)
    #define SIMDE_X86_SVML_ENABLE_NATIVE_ALIASES
  #endif

  #if !defined(SIMDE_ARM_NEON_A32V7_NATIVE)
    #define SIMDE_ARM_NEON_A32V7_ENABLE_NATIVE_ALIASES
  #endif
  #if !defined(SIMDE_ARM_NEON_A32V8_NATIVE)
    #define SIMDE_ARM_NEON_A32V8_ENABLE_NATIVE_ALIASES
  #endif
  #if !defined(SIMDE_ARM_NEON_A64V8_NATIVE)
    #define SIMDE_ARM_NEON_A64V8_ENABLE_NATIVE_ALIASES
  #endif

  #if !defined(SIMDE_ARM_SVE_NATIVE)
    #define SIMDE_ARM_SVE_ENABLE_NATIVE_ALIASES
  #endif

  #if !defined(SIMDE_RISCV_V_NATIVE)
    #define SIMDE_RISCV_V_ENABLE_NATIVE_ALIASES
  #endif

  #if !defined(SIMDE_MIPS_MSA_NATIVE)
    #define SIMDE_MIPS_MSA_ENABLE_NATIVE_ALIASES
  #endif

  #if !defined(SIMDE_WASM_SIMD128_NATIVE)
    #define SIMDE_WASM_SIMD128_ENABLE_NATIVE_ALIASES
  #endif
#endif

/* Are floating point values stored using IEEE 754?  Knowing
 * this at during preprocessing is a bit tricky, mostly because what
 * we're curious about is how values are stored and not whether the
 * implementation is fully conformant in terms of rounding, NaN
 * handling, etc.
 *
 * For example, if you use -ffast-math or -Ofast on
 * GCC or clang IEEE 754 isn't strictly followed, therefore IEE 754
 * support is not advertised (by defining __STDC_IEC_559__).
 *
 * However, what we care about is whether it is safe to assume that
 * floating point values are stored in IEEE 754 format, in which case
 * we can provide faster implementations of some functions.
 *
 * Luckily every vaugely modern architecture I'm aware of uses IEEE 754-
 * so we just assume IEEE 754 for now.  There is a test which verifies
 * this, if that test fails sowewhere please let us know and we'll add
 * an exception for that platform.  Meanwhile, you can define
 * SIMDE_NO_IEEE754_STORAGE. */
#if !defined(SIMDE_IEEE754_STORAGE) && !defined(SIMDE_NO_IEE754_STORAGE)
  #define SIMDE_IEEE754_STORAGE
#endif

#if defined(SIMDE_ARCH_ARM_NEON_FP16)
  #define SIMDE_ARM_NEON_FP16
#endif

#if defined(SIMDE_ARCH_ARM_NEON_BF16)
  #define SIMDE_ARM_NEON_BF16
#endif

#if !defined(SIMDE_LOONGARCH_LASX_NATIVE) && !defined(SIMDE_LOONGARCH_LASX_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_LOONGARCH_LASX)
    #define SIMDE_LOONGARCH_LASX_NATIVE
  #endif
#endif

#if !defined(SIMDE_LOONGARCH_LSX_NATIVE) && !defined(SIMDE_LOONGARCH_LSX_NO_NATIVE) && !defined(SIMDE_NO_NATIVE)
  #if defined(SIMDE_ARCH_LOONGARCH_LSX)
    #define SIMDE_LOONGARCH_LSX_NATIVE
  #endif
#endif

#if defined(SIMDE_LOONGARCH_LASX_NATIVE)
  #include <lasxintrin.h>
#endif
#if defined(SIMDE_LOONGARCH_LSX_NATIVE)
  #include <lsxintrin.h>
#endif

#endif /* !defined(SIMDE_FEATURES_H) */
