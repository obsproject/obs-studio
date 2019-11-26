/* Copyright (c) 2017-2019 Evan Nemerson <evan@nemerson.com>
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

#if !defined(SIMDE_COMMON_H)
#define SIMDE_COMMON_H

#include "hedley.h"
#include "check.h"
#include "simde-arch.h"

#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L)
#define SIMDE_ALIGN(alignment) _Alignas(alignment)
#elif (defined(__cplusplus) && (__cplusplus >= 201103L))
#define SIMDE_ALIGN(alignment) alignas(alignment)
#elif HEDLEY_GCC_VERSION_CHECK(2, 95, 0) ||     \
	HEDLEY_CRAY_VERSION_CHECK(8, 4, 0) ||   \
	HEDLEY_IBM_VERSION_CHECK(11, 1, 0) ||   \
	HEDLEY_INTEL_VERSION_CHECK(13, 0, 0) || \
	HEDLEY_PGI_VERSION_CHECK(19, 4, 0) ||   \
	HEDLEY_ARM_VERSION_CHECK(4, 1, 0) ||    \
	HEDLEY_TINYC_VERSION_CHECK(0, 9, 24) || \
	HEDLEY_TI_VERSION_CHECK(8, 1, 0)
#define SIMDE_ALIGN(alignment) __attribute__((aligned(alignment)))
#elif defined(_MSC_VER) && (!defined(_M_IX86) || defined(_M_AMD64))
#define SIMDE_ALIGN(alignment) __declspec(align(alignment))
#else
#define SIMDE_ALIGN(alignment)
#endif

#define simde_assert_aligned(alignment, val) \
	simde_assert_int(((uintptr_t)(val)) % (alignment), ==, 0)

#if HEDLEY_GCC_HAS_ATTRIBUTE(vector_size, 4, 6, 0)
#define SIMDE__ENABLE_GCC_VEC_EXT
#endif

#if !defined(SIMDE_ENABLE_OPENMP) &&                   \
	((defined(_OPENMP) && (_OPENMP >= 201307L)) || \
	 (defined(_OPENMP_SIMD) && (_OPENMP_SIMD >= 201307L)))
#define SIMDE_ENABLE_OPENMP
#endif

#if !defined(SIMDE_ENABLE_CILKPLUS) && defined(__cilk)
#define SIMDE_ENABLE_CILKPLUS
#endif

#if defined(SIMDE_ENABLE_OPENMP)
#define SIMDE__VECTORIZE _Pragma("omp simd")
#define SIMDE__VECTORIZE_SAFELEN(l) HEDLEY_PRAGMA(omp simd safelen(l))
#define SIMDE__VECTORIZE_REDUCTION(r) HEDLEY_PRAGMA(omp simd reduction(r))
#define SIMDE__VECTORIZE_ALIGNED(a) HEDLEY_PRAGMA(omp simd aligned(a))
#elif defined(SIMDE_ENABLE_CILKPLUS)
#define SIMDE__VECTORIZE _Pragma("simd")
#define SIMDE__VECTORIZE_SAFELEN(l) HEDLEY_PRAGMA(simd vectorlength(l))
#define SIMDE__VECTORIZE_REDUCTION(r) HEDLEY_PRAGMA(simd reduction(r))
#define SIMDE__VECTORIZE_ALIGNED(a) HEDLEY_PRAGMA(simd aligned(a))
#elif defined(__INTEL_COMPILER)
#define SIMDE__VECTORIZE _Pragma("simd")
#define SIMDE__VECTORIZE_SAFELEN(l) HEDLEY_PRAGMA(simd vectorlength(l))
#define SIMDE__VECTORIZE_REDUCTION(r) HEDLEY_PRAGMA(simd reduction(r))
#define SIMDE__VECTORIZE_ALIGNED(a)
#elif defined(__clang__)
#define SIMDE__VECTORIZE _Pragma("clang loop vectorize(enable)")
#define SIMDE__VECTORIZE_SAFELEN(l) HEDLEY_PRAGMA(clang loop vectorize_width(l))
#define SIMDE__VECTORIZE_REDUCTION(r) SIMDE__VECTORIZE
#define SIMDE__VECTORIZE_ALIGNED(a)
#elif HEDLEY_GCC_VERSION_CHECK(4, 9, 0)
#define SIMDE__VECTORIZE _Pragma("GCC ivdep")
#define SIMDE__VECTORIZE_SAFELEN(l) SIMDE__VECTORIZE
#define SIMDE__VECTORIZE_REDUCTION(r) SIMDE__VECTORIZE
#define SIMDE__VECTORIZE_ALIGNED(a)
#elif HEDLEY_CRAY_VERSION_CHECK(5, 0, 0)
#define SIMDE__VECTORIZE _Pragma("_CRI ivdep")
#define SIMDE__VECTORIZE_SAFELEN(l) SIMDE__VECTORIZE
#define SIMDE__VECTORIZE_REDUCTION(r) SIMDE__VECTORIZE
#define SIMDE__VECTORIZE_ALIGNED(a)
#else
#define SIMDE__VECTORIZE
#define SIMDE__VECTORIZE_SAFELEN(l)
#define SIMDE__VECTORIZE_REDUCTION(r)
#define SIMDE__VECTORIZE_ALIGNED(a)
#endif

#if HEDLEY_GCC_HAS_ATTRIBUTE(unused, 3, 1, 0)
#define SIMDE__UNUSED __attribute__((__unused__))
#else
#define SIMDE__UNUSED
#endif

#if HEDLEY_GCC_HAS_ATTRIBUTE(artificial, 4, 3, 0)
#define SIMDE__ARTIFICIAL __attribute__((__artificial__))
#else
#define SIMDE__ARTIFICIAL
#endif

/* Intended for checking coverage, you should never use this in
   production. */
#if defined(SIMDE_NO_INLINE)
#define SIMDE__FUNCTION_ATTRIBUTES HEDLEY_NEVER_INLINE SIMDE__UNUSED static
#else
#define SIMDE__FUNCTION_ATTRIBUTES HEDLEY_INLINE SIMDE__ARTIFICIAL static
#endif

#if defined(_MSC_VER)
#define SIMDE__BEGIN_DECLS                                            \
	HEDLEY_DIAGNOSTIC_PUSH __pragma(warning(disable : 4996 4204)) \
		HEDLEY_BEGIN_C_DECLS
#define SIMDE__END_DECLS HEDLEY_DIAGNOSTIC_POP HEDLEY_END_C_DECLS
#else
#define SIMDE__BEGIN_DECLS HEDLEY_BEGIN_C_DECLS
#define SIMDE__END_DECLS HEDLEY_END_C_DECLS
#endif

#if defined(__SIZEOF_INT128__)
#define SIMDE__HAVE_INT128
typedef __int128 simde_int128;
typedef unsigned __int128 simde_uint128;
#endif

/* TODO: we should at least make an attempt to detect the correct
   types for simde_float32/float64 instead of just assuming float and
   double. */

#if !defined(SIMDE_FLOAT32_TYPE)
#define SIMDE_FLOAT32_TYPE float
#define SIMDE_FLOAT32_C(value) value##f
#else
#define SIMDE_FLOAT32_C(value) ((SIMDE_FLOAT32_TYPE)value)
#endif
typedef SIMDE_FLOAT32_TYPE simde_float32;
HEDLEY_STATIC_ASSERT(sizeof(simde_float32) == 4,
		     "Unable to find 32-bit floating-point type.");

#if !defined(SIMDE_FLOAT64_TYPE)
#define SIMDE_FLOAT64_TYPE double
#define SIMDE_FLOAT64_C(value) value
#else
#define SIMDE_FLOAT32_C(value) ((SIMDE_FLOAT64_TYPE)value)
#endif
typedef SIMDE_FLOAT64_TYPE simde_float64;
HEDLEY_STATIC_ASSERT(sizeof(simde_float64) == 8,
		     "Unable to find 64-bit floating-point type.");

/* Whether to assume that the compiler can auto-vectorize reasonably
   well.  This will cause SIMDe to attempt to compose vector
   operations using more simple vector operations instead of minimize
   serial work.

   As an example, consider the _mm_add_ss(a, b) function from SSE,
   which returns { a0 + b0, a1, a2, a3 }.  This pattern is repeated
   for other operations (sub, mul, etc.).

   The naïve implementation would result in loading a0 and b0, adding
   them into a temporary variable, then splicing that value into a new
   vector with the remaining elements from a.

   On platforms which support vectorization, it's generally faster to
   simply perform the operation on the entire vector to avoid having
   to move data between SIMD registers and non-SIMD registers.
   Basically, instead of the temporary variable being (a0 + b0) it
   would be a vector of (a + b), which is then combined with a to form
   the result.

   By default, SIMDe will prefer the pure-vector versions if we detect
   a vector ISA extension, but this can be overridden by defining
   SIMDE_NO_ASSUME_VECTORIZATION.  You can also define
   SIMDE_ASSUME_VECTORIZATION if you want to force SIMDe to use the
   vectorized version. */
#if !defined(SIMDE_NO_ASSUME_VECTORIZATION) && \
	!defined(SIMDE_ASSUME_VECTORIZATION)
#if defined(__SSE__) || defined(__ARM_NEON) || defined(__mips_msa) || \
	defined(__ALTIVEC__)
#define SIMDE_ASSUME_VECTORIZATION
#endif
#endif

/* GCC and clang have built-in functions to handle shuffling of
   vectors, but the implementations are slightly different.  This
   macro is just an abstraction over them.  Note that elem_size is in
   bits but vec_size is in bytes. */
#if HEDLEY_CLANG_HAS_BUILTIN(__builtin_shufflevector)
#define SIMDE__SHUFFLE_VECTOR(elem_size, vec_size, a, b, ...) \
	__builtin_shufflevector(a, b, __VA_ARGS__)
#elif HEDLEY_GCC_HAS_BUILTIN(__builtin_shuffle, 4, 7, 0) && \
	!defined(__INTEL_COMPILER)
#define SIMDE__SHUFFLE_VECTOR(elem_size, vec_size, a, b, ...) \
	__builtin_shuffle(a, b,                               \
			  (int##elem_size##_t __attribute__(  \
				  (__vector_size__(vec_size)))){__VA_ARGS__})
#endif

/* Some algorithms are iterative, and fewer iterations means less
   accuracy.  Lower values here will result in faster, but less
   accurate, calculations for some functions. */
#if !defined(SIMDE_ACCURACY_ITERS)
#define SIMDE_ACCURACY_ITERS 2
#endif

/* This will probably move into Hedley at some point, but I'd like to
   more thoroughly check for other compilers which define __GNUC__
   first. */
#if defined(SIMDE__REALLY_GCC)
#undef SIMDE__REALLY_GCC
#endif
#if !defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER)
#define SIMDE__REALLY_GCC 0
#else
#define SIMDE__REALLY_GCC 1
#endif

#if defined(SIMDE__ASSUME_ALIGNED)
#undef SIMDE__ASSUME_ALIGNED
#endif
#if HEDLEY_INTEL_VERSION_CHECK(9, 0, 0)
#define SIMDE__ASSUME_ALIGNED(ptr, align) __assume_aligned(ptr, align)
#elif HEDLEY_MSVC_VERSION_CHECK(13, 10, 0)
#define SIMDE__ASSUME_ALIGNED(ptr, align) \
	__assume((((char *)ptr) - ((char *)0)) % (align) == 0)
#elif HEDLEY_GCC_HAS_BUILTIN(__builtin_assume_aligned, 4, 7, 0)
#define SIMDE__ASSUME_ALIGNED(ptr, align) \
	(ptr = (__typeof__(ptr))__builtin_assume_aligned((ptr), align))
#elif HEDLEY_CLANG_HAS_BUILTIN(__builtin_assume)
#define SIMDE__ASSUME_ALIGNED(ptr, align) \
	__builtin_assume((((char *)ptr) - ((char *)0)) % (align) == 0)
#elif HEDLEY_GCC_HAS_BUILTIN(__builtin_unreachable, 4, 5, 0)
#define SIMDE__ASSUME_ALIGNED(ptr, align)              \
	((((char *)ptr) - ((char *)0)) % (align) == 0) \
		? (1)                                  \
		: (__builtin_unreachable(), 0)
#else
#define SIMDE__ASSUME_ALIGNED(ptr, align)
#endif

/* Sometimes we run into problems with specific versions of compilers
   which make the native versions unusable for us.  Often this is due
   to missing functions, sometimes buggy implementations, etc.  These
   macros are how we check for specific bugs.  As they are fixed we'll
   start only defining them for problematic compiler versions. */

#if !defined(SIMDE_IGNORE_COMPILER_BUGS)
#if SIMDE__REALLY_GCC
#if !HEDLEY_GCC_VERSION_CHECK(4, 9, 0)
#define SIMDE_BUG_GCC_REV_208793
#endif
#if !HEDLEY_GCC_VERSION_CHECK(5, 0, 0)
#define SIMDE_BUG_GCC_BAD_MM_SRA_EPI32 /* TODO: find relevant bug or commit */
#endif
#if !HEDLEY_GCC_VERSION_CHECK(4, 6, 0)
#define SIMDE_BUG_GCC_BAD_MM_EXTRACT_EPI8 /* TODO: find relevant bug or commit */
#endif
#endif
#if defined(__EMSCRIPTEN__)
#define SIMDE_BUG_EMSCRIPTEN_MISSING_IMPL /* Placeholder for (as yet) unfiled issues. */
#define SIMDE_BUG_EMSCRIPTEN_5242
#endif
#endif

#endif /* !defined(SIMDE_COMMON_H) */
