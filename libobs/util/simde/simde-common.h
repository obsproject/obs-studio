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
 */

#if !defined(SIMDE_COMMON_H)
#define SIMDE_COMMON_H

#include "hedley.h"

#define SIMDE_VERSION_MAJOR 0
#define SIMDE_VERSION_MINOR 5
#define SIMDE_VERSION_MICRO 0
#define SIMDE_VERSION                                                   \
	HEDLEY_VERSION_ENCODE(SIMDE_VERSION_MAJOR, SIMDE_VERSION_MINOR, \
			      SIMDE_VERSION_MICRO)

#include "simde-arch.h"
#include "simde-features.h"
#include "simde-diagnostic.h"

#include <stddef.h>
#include <stdint.h>

#if HEDLEY_HAS_ATTRIBUTE(aligned) || HEDLEY_GCC_VERSION_CHECK(2, 95, 0) || \
	HEDLEY_CRAY_VERSION_CHECK(8, 4, 0) ||                              \
	HEDLEY_IBM_VERSION_CHECK(11, 1, 0) ||                              \
	HEDLEY_INTEL_VERSION_CHECK(13, 0, 0) ||                            \
	HEDLEY_PGI_VERSION_CHECK(19, 4, 0) ||                              \
	HEDLEY_ARM_VERSION_CHECK(4, 1, 0) ||                               \
	HEDLEY_TINYC_VERSION_CHECK(0, 9, 24) ||                            \
	HEDLEY_TI_VERSION_CHECK(8, 1, 0)
#define SIMDE_ALIGN(alignment) __attribute__((aligned(alignment)))
#elif defined(_MSC_VER) && !(defined(_M_ARM) && !defined(_M_ARM64))
#define SIMDE_ALIGN(alignment) __declspec(align(alignment))
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#define SIMDE_ALIGN(alignment) _Alignas(alignment)
#elif defined(__cplusplus) && (__cplusplus >= 201103L)
#define SIMDE_ALIGN(alignment) alignas(alignment)
#else
#define SIMDE_ALIGN(alignment)
#endif

#if HEDLEY_GNUC_VERSION_CHECK(2, 95, 0) ||   \
	HEDLEY_ARM_VERSION_CHECK(4, 1, 0) || \
	HEDLEY_IBM_VERSION_CHECK(11, 1, 0)
#define SIMDE_ALIGN_OF(T) (__alignof__(T))
#elif (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)) || \
	HEDLEY_HAS_FEATURE(c11_alignof)
#define SIMDE_ALIGN_OF(T) (_Alignof(T))
#elif (defined(__cplusplus) && (__cplusplus >= 201103L)) || \
	HEDLEY_HAS_FEATURE(cxx_alignof)
#define SIMDE_ALIGN_OF(T) (alignof(T))
#endif

#if defined(SIMDE_ALIGN_OF)
#define SIMDE_ALIGN_AS(N, T) SIMDE_ALIGN(SIMDE_ALIGN_OF(T))
#else
#define SIMDE_ALIGN_AS(N, T) SIMDE_ALIGN(N)
#endif

#define simde_assert_aligned(alignment, val)                                \
	simde_assert_int(HEDLEY_REINTERPRET_CAST(                           \
				 uintptr_t, HEDLEY_REINTERPRET_CAST(        \
						    const void *, (val))) % \
				 (alignment),                               \
			 ==, 0)

#if HEDLEY_HAS_BUILTIN(__builtin_constant_p) ||                             \
	HEDLEY_GCC_VERSION_CHECK(3, 4, 0) ||                                \
	HEDLEY_INTEL_VERSION_CHECK(13, 0, 0) ||                             \
	HEDLEY_TINYC_VERSION_CHECK(0, 9, 19) ||                             \
	HEDLEY_ARM_VERSION_CHECK(4, 1, 0) ||                                \
	HEDLEY_IBM_VERSION_CHECK(13, 1, 0) ||                               \
	HEDLEY_TI_CL6X_VERSION_CHECK(6, 1, 0) ||                            \
	(HEDLEY_SUNPRO_VERSION_CHECK(5, 10, 0) && !defined(__cplusplus)) || \
	HEDLEY_CRAY_VERSION_CHECK(8, 1, 0)
#define SIMDE_CHECK_CONSTANT_(expr) (__builtin_constant_p(expr))
#elif defined(__cplusplus) && (__cplusplus > 201703L)
#include <type_traits>
#define SIMDE_CHECK_CONSTANT_(expr) (std::is_constant_evaluated())
#endif

/* diagnose_if + __builtin_constant_p was broken until clang 9,
 * which is when __FILE_NAME__ was added. */
#if defined(SIMDE_CHECK_CONSTANT_) && defined(__FILE_NAME__)
#define SIMDE_REQUIRE_CONSTANT(arg)                    \
	HEDLEY_REQUIRE_MSG(SIMDE_CHECK_CONSTANT_(arg), \
			   "`" #arg "' must be constant")
#else
#define SIMDE_REQUIRE_CONSTANT(arg)
#endif

#define SIMDE_REQUIRE_RANGE(arg, min, max)                         \
	HEDLEY_REQUIRE_MSG((((arg) >= (min)) && ((arg) <= (max))), \
			   "'" #arg "' must be in [" #min ", " #max "]")

#define SIMDE_REQUIRE_CONSTANT_RANGE(arg, min, max) \
	SIMDE_REQUIRE_CONSTANT(arg)                 \
	SIMDE_REQUIRE_RANGE(arg, min, max)

/* SIMDE_ASSUME_ALIGNED allows you to (try to) tell the compiler
 * that a pointer is aligned to an `alignment`-byte boundary. */
#if HEDLEY_HAS_BUILTIN(__builtin_assume_aligned) || \
	HEDLEY_GCC_VERSION_CHECK(4, 7, 0)
#define SIMDE_ASSUME_ALIGNED(alignment, v)     \
	HEDLEY_REINTERPRET_CAST(__typeof__(v), \
				__builtin_assume_aligned(v, alignment))
#elif defined(__cplusplus) && (__cplusplus > 201703L)
#define SIMDE_ASSUME_ALIGNED(alignment, v) std::assume_aligned<alignment>(v)
#elif HEDLEY_INTEL_VERSION_CHECK(13, 0, 0)
#define SIMDE_ASSUME_ALIGNED(alignment, v)                            \
	(__extension__({                                              \
		__typeof__(v) simde_assume_aligned_t_ = (v);          \
		__assume_aligned(simde_assume_aligned_t_, alignment); \
		simde_assume_aligned_t_;                              \
	}))
#else
#define SIMDE_ASSUME_ALIGNED(alignment, v) (v)
#endif

/* SIMDE_ALIGN_CAST allows you to convert to a type with greater
 * aligment requirements without triggering a warning. */
#if HEDLEY_HAS_WARNING("-Wcast-align")
#define SIMDE_ALIGN_CAST(T, v)                                       \
	(__extension__({                                             \
		HEDLEY_DIAGNOSTIC_PUSH                               \
		_Pragma("clang diagnostic ignored \"-Wcast-align\"") \
			T simde_r_ = HEDLEY_REINTERPRET_CAST(T, v);  \
		HEDLEY_DIAGNOSTIC_POP                                \
		simde_r_;                                            \
	}))
#else
#define SIMDE_ALIGN_CAST(T, v) HEDLEY_REINTERPRET_CAST(T, v)
#endif

#if (HEDLEY_HAS_ATTRIBUTE(may_alias) && !defined(HEDLEY_SUNPRO_VERSION)) || \
	HEDLEY_GCC_VERSION_CHECK(3, 3, 0) ||                                \
	HEDLEY_INTEL_VERSION_CHECK(13, 0, 0) ||                             \
	HEDLEY_IBM_VERSION_CHECK(13, 1, 0)
#define SIMDE_MAY_ALIAS __attribute__((__may_alias__))
#else
#define SIMDE_MAY_ALIAS
#endif

/*  Lots of compilers support GCC-style vector extensions, but many
    don't support all the features.  Define different macros depending
    on support for

    * SIMDE_VECTOR - Declaring a vector.
    * SIMDE_VECTOR_OPS - basic operations (binary and unary).
    * SIMDE_VECTOR_SCALAR - For binary operators, the second argument
        can be a scalar, in which case the result is as if that scalar
        had been broadcast to all lanes of a vector.
    * SIMDE_VECTOR_SUBSCRIPT - Supports array subscript notation for
        extracting/inserting a single element.=

    SIMDE_VECTOR can be assumed if any others are defined, the
    others are independent. */
#if !defined(SIMDE_NO_VECTOR)
#if HEDLEY_GCC_VERSION_CHECK(4, 8, 0)
#define SIMDE_VECTOR(size) __attribute__((__vector_size__(size)))
#define SIMDE_VECTOR_OPS
#define SIMDE_VECTOR_SCALAR
#define SIMDE_VECTOR_SUBSCRIPT
#elif HEDLEY_INTEL_VERSION_CHECK(16, 0, 0)
#define SIMDE_VECTOR(size) __attribute__((__vector_size__(size)))
#define SIMDE_VECTOR_OPS
/* ICC only supports SIMDE_VECTOR_SCALAR for constants */
#define SIMDE_VECTOR_SUBSCRIPT
#elif HEDLEY_GCC_VERSION_CHECK(4, 1, 0) || HEDLEY_INTEL_VERSION_CHECK(13, 0, 0)
#define SIMDE_VECTOR(size) __attribute__((__vector_size__(size)))
#define SIMDE_VECTOR_OPS
#elif HEDLEY_SUNPRO_VERSION_CHECK(5, 12, 0)
#define SIMDE_VECTOR(size) __attribute__((__vector_size__(size)))
#elif HEDLEY_HAS_ATTRIBUTE(vector_size)
#define SIMDE_VECTOR(size) __attribute__((__vector_size__(size)))
#define SIMDE_VECTOR_OPS
#define SIMDE_VECTOR_SUBSCRIPT
#if HEDLEY_HAS_ATTRIBUTE(diagnose_if) /* clang 4.0 */
#define SIMDE_VECTOR_SCALAR
#endif
#endif

/* GCC and clang have built-in functions to handle shuffling and
   converting of vectors, but the implementations are slightly
   different.  This macro is just an abstraction over them.  Note that
   elem_size is in bits but vec_size is in bytes. */
#if !defined(SIMDE_NO_SHUFFLE_VECTOR) && defined(SIMDE_VECTOR_SUBSCRIPT)
HEDLEY_DIAGNOSTIC_PUSH
/* We don't care about -Wvariadic-macros; all compilers that support
      * shufflevector/shuffle support them. */
#if HEDLEY_HAS_WARNING("-Wc++98-compat-pedantic")
#pragma clang diagnostic ignored "-Wc++98-compat-pedantic"
#endif
#if HEDLEY_HAS_WARNING("-Wvariadic-macros") || HEDLEY_GCC_VERSION_CHECK(4, 0, 0)
#pragma GCC diagnostic ignored "-Wvariadic-macros"
#endif

#if HEDLEY_HAS_BUILTIN(__builtin_shufflevector)
#define SIMDE_SHUFFLE_VECTOR_(elem_size, vec_size, a, b, ...) \
	__builtin_shufflevector(a, b, __VA_ARGS__)
#elif HEDLEY_GCC_HAS_BUILTIN(__builtin_shuffle, 4, 7, 0) && \
	!defined(__INTEL_COMPILER)
#define SIMDE_SHUFFLE_VECTOR_(elem_size, vec_size, a, b, ...) \
	(__extension__({                                      \
		int##elem_size##_t SIMDE_VECTOR(vec_size)     \
			simde_shuffle_ = {__VA_ARGS__};       \
		__builtin_shuffle(a, b, simde_shuffle_);      \
	}))
#endif
HEDLEY_DIAGNOSTIC_POP
#endif

/* TODO: this actually works on XL C/C++ without SIMDE_VECTOR_SUBSCRIPT
   but the code needs to be refactored a bit to take advantage. */
#if !defined(SIMDE_NO_CONVERT_VECTOR) && defined(SIMDE_VECTOR_SUBSCRIPT)
#if HEDLEY_HAS_BUILTIN(__builtin_convertvector) || \
	HEDLEY_GCC_VERSION_CHECK(9, 0, 0)
#if HEDLEY_GCC_VERSION_CHECK(9, 0, 0) && !HEDLEY_GCC_VERSION_CHECK(9, 3, 0)
/* https://gcc.gnu.org/bugzilla/show_bug.cgi?id=93557 */
#define SIMDE_CONVERT_VECTOR_(to, from)                          \
	((to) = (__extension__({                                 \
		 __typeof__(from) from_ = (from);                \
		 ((void)from_);                                  \
		 __builtin_convertvector(from_, __typeof__(to)); \
	 })))
#else
#define SIMDE_CONVERT_VECTOR_(to, from) \
	((to) = __builtin_convertvector((from), __typeof__(to)))
#endif
#endif
#endif
#endif

/* Since we currently require SUBSCRIPT before using a vector in a
   union, we define these as dependencies of SUBSCRIPT.  They are
   likely to disappear in the future, once SIMDe learns how to make
   use of vectors without using the union members.  Do not use them
   in your code unless you're okay with it breaking when SIMDe
   changes. */
#if defined(SIMDE_VECTOR_SUBSCRIPT)
#if defined(SIMDE_VECTOR_OPS)
#define SIMDE_VECTOR_SUBSCRIPT_OPS
#endif
#if defined(SIMDE_VECTOR_SCALAR)
#define SIMDE_VECTOR_SUBSCRIPT_SCALAR
#endif
#endif

#if !defined(SIMDE_ENABLE_OPENMP) &&                   \
	((defined(_OPENMP) && (_OPENMP >= 201307L)) || \
	 (defined(_OPENMP_SIMD) && (_OPENMP_SIMD >= 201307L)))
#define SIMDE_ENABLE_OPENMP
#endif

#if !defined(SIMDE_ENABLE_CILKPLUS) && \
	(defined(__cilk) || defined(HEDLEY_INTEL_VERSION))
#define SIMDE_ENABLE_CILKPLUS
#endif

#if defined(SIMDE_ENABLE_OPENMP)
#define SIMDE_VECTORIZE _Pragma("omp simd")
#define SIMDE_VECTORIZE_SAFELEN(l) HEDLEY_PRAGMA(omp simd safelen(l))
#define SIMDE_VECTORIZE_REDUCTION(r) HEDLEY_PRAGMA(omp simd reduction(r))
#define SIMDE_VECTORIZE_ALIGNED(a) HEDLEY_PRAGMA(omp simd aligned(a))
#elif defined(SIMDE_ENABLE_CILKPLUS)
#define SIMDE_VECTORIZE _Pragma("simd")
#define SIMDE_VECTORIZE_SAFELEN(l) HEDLEY_PRAGMA(simd vectorlength(l))
#define SIMDE_VECTORIZE_REDUCTION(r) HEDLEY_PRAGMA(simd reduction(r))
#define SIMDE_VECTORIZE_ALIGNED(a) HEDLEY_PRAGMA(simd aligned(a))
#elif defined(__clang__) && !defined(HEDLEY_IBM_VERSION)
#define SIMDE_VECTORIZE _Pragma("clang loop vectorize(enable)")
#define SIMDE_VECTORIZE_SAFELEN(l) HEDLEY_PRAGMA(clang loop vectorize_width(l))
#define SIMDE_VECTORIZE_REDUCTION(r) SIMDE_VECTORIZE
#define SIMDE_VECTORIZE_ALIGNED(a)
#elif HEDLEY_GCC_VERSION_CHECK(4, 9, 0)
#define SIMDE_VECTORIZE _Pragma("GCC ivdep")
#define SIMDE_VECTORIZE_SAFELEN(l) SIMDE_VECTORIZE
#define SIMDE_VECTORIZE_REDUCTION(r) SIMDE_VECTORIZE
#define SIMDE_VECTORIZE_ALIGNED(a)
#elif HEDLEY_CRAY_VERSION_CHECK(5, 0, 0)
#define SIMDE_VECTORIZE _Pragma("_CRI ivdep")
#define SIMDE_VECTORIZE_SAFELEN(l) SIMDE_VECTORIZE
#define SIMDE_VECTORIZE_REDUCTION(r) SIMDE_VECTORIZE
#define SIMDE_VECTORIZE_ALIGNED(a)
#else
#define SIMDE_VECTORIZE
#define SIMDE_VECTORIZE_SAFELEN(l)
#define SIMDE_VECTORIZE_REDUCTION(r)
#define SIMDE_VECTORIZE_ALIGNED(a)
#endif

#define SIMDE_MASK_NZ_(v, mask) (((v) & (mask)) | !((v) & (mask)))

/* Intended for checking coverage, you should never use this in
   production. */
#if defined(SIMDE_NO_INLINE)
#define SIMDE_FUNCTION_ATTRIBUTES HEDLEY_NEVER_INLINE static
#else
#define SIMDE_FUNCTION_ATTRIBUTES HEDLEY_ALWAYS_INLINE static
#endif

#if HEDLEY_HAS_ATTRIBUTE(unused) || HEDLEY_GCC_VERSION_CHECK(2, 95, 0)
#define SIMDE_FUNCTION_POSSIBLY_UNUSED_ __attribute__((__unused__))
#else
#define SIMDE_FUNCTION_POSSIBLY_UNUSED_
#endif

#if HEDLEY_HAS_WARNING("-Wused-but-marked-unused")
#define SIMDE_DIAGNOSTIC_DISABLE_USED_BUT_MARKED_UNUSED \
	_Pragma("clang diagnostic ignored \"-Wused-but-marked-unused\"")
#else
#define SIMDE_DIAGNOSTIC_DISABLE_USED_BUT_MARKED_UNUSED
#endif

#if defined(_MSC_VER)
#define SIMDE_BEGIN_DECLS_                                            \
	HEDLEY_DIAGNOSTIC_PUSH __pragma(warning(disable : 4996 4204)) \
		HEDLEY_BEGIN_C_DECLS
#define SIMDE_END_DECLS_ HEDLEY_DIAGNOSTIC_POP HEDLEY_END_C_DECLS
#else
#define SIMDE_BEGIN_DECLS_                              \
	HEDLEY_DIAGNOSTIC_PUSH                          \
	SIMDE_DIAGNOSTIC_DISABLE_USED_BUT_MARKED_UNUSED \
	HEDLEY_BEGIN_C_DECLS
#define SIMDE_END_DECLS_   \
	HEDLEY_END_C_DECLS \
	HEDLEY_DIAGNOSTIC_POP
#endif

#if HEDLEY_HAS_WARNING("-Wpedantic")
#define SIMDE_DIAGNOSTIC_DISABLE_INT128 \
	_Pragma("clang diagnostic ignored \"-Wpedantic\"")
#elif defined(HEDLEY_GCC_VERSION)
#define SIMDE_DIAGNOSTIC_DISABLE_INT128 \
	_Pragma("GCC diagnostic ignored \"-Wpedantic\"")
#else
#define SIMDE_DIAGNOSTIC_DISABLE_INT128
#endif

#if defined(__SIZEOF_INT128__)
#define SIMDE_HAVE_INT128_
HEDLEY_DIAGNOSTIC_PUSH
SIMDE_DIAGNOSTIC_DISABLE_INT128
typedef __int128 simde_int128;
typedef unsigned __int128 simde_uint128;
HEDLEY_DIAGNOSTIC_POP
#endif

#if !defined(SIMDE_ENDIAN_LITTLE)
#define SIMDE_ENDIAN_LITTLE 1234
#endif
#if !defined(SIMDE_ENDIAN_BIG)
#define SIMDE_ENDIAN_BIG 4321
#endif

#if !defined(SIMDE_ENDIAN_ORDER)
/* GCC (and compilers masquerading as GCC) define  __BYTE_ORDER__. */
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && \
	(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define SIMDE_ENDIAN_ORDER SIMDE_ENDIAN_LITTLE
#elif defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && \
	(__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define SIMDE_ENDIAN_ORDER SIMDE_ENDIAN_BIG
/* TI defines _BIG_ENDIAN or _LITTLE_ENDIAN */
#elif defined(_BIG_ENDIAN)
#define SIMDE_ENDIAN_ORDER SIMDE_ENDIAN_BIG
#elif defined(_LITTLE_ENDIAN)
#define SIMDE_ENDIAN_ORDER SIMDE_ENDIAN_LITTLE
/* We know the endianness of some common architectures.  Common
 * architectures not listed (ARM, POWER, MIPS, etc.) here are
 * bi-endian. */
#elif defined(__amd64) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
#define SIMDE_ENDIAN_ORDER SIMDE_ENDIAN_LITTLE
#elif defined(__s390x__) || defined(__zarch__)
#define SIMDE_ENDIAN_ORDER SIMDE_ENDIAN_BIG
/* Looks like we'll have to rely on the platform.  If we're missing a
 * platform, please let us know. */
#elif defined(_WIN32)
#define SIMDE_ENDIAN_ORDER SIMDE_ENDIAN_LITTLE
#elif defined(sun) || defined(__sun) /* Solaris */
#include <sys/byteorder.h>
#if defined(_LITTLE_ENDIAN)
#define SIMDE_ENDIAN_ORDER SIMDE_ENDIAN_LITTLE
#elif defined(_BIG_ENDIAN)
#define SIMDE_ENDIAN_ORDER SIMDE_ENDIAN_BIG
#endif
#elif defined(__APPLE__)
#include <libkern/OSByteOrder.h>
#if defined(__LITTLE_ENDIAN__)
#define SIMDE_ENDIAN_ORDER SIMDE_ENDIAN_LITTLE
#elif defined(__BIG_ENDIAN__)
#define SIMDE_ENDIAN_ORDER SIMDE_ENDIAN_BIG
#endif
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || \
	defined(__bsdi__) || defined(__DragonFly__) || defined(BSD)
#include <machine/endian.h>
#if defined(__BYTE_ORDER) && (__BYTE_ORDER == __LITTLE_ENDIAN)
#define SIMDE_ENDIAN_ORDER SIMDE_ENDIAN_LITTLE
#elif defined(__BYTE_ORDER) && (__BYTE_ORDER == __BIG_ENDIAN)
#define SIMDE_ENDIAN_ORDER SIMDE_ENDIAN_BIG
#endif
#elif defined(__linux__) || defined(__linux) || defined(__gnu_linux__)
#include <endian.h>
#if defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN) && \
	(__BYTE_ORDER == __LITTLE_ENDIAN)
#define SIMDE_ENDIAN_ORDER SIMDE_ENDIAN_LITTLE
#elif defined(__BYTE_ORDER) && defined(__BIG_ENDIAN) && \
	(__BYTE_ORDER == __BIG_ENDIAN)
#define SIMDE_ENDIAN_ORDER SIMDE_ENDIAN_BIG
#endif
#endif
#endif

#if HEDLEY_HAS_BUILTIN(__builtin_bswap64) ||  \
	HEDLEY_GCC_VERSION_CHECK(4, 3, 0) ||  \
	HEDLEY_IBM_VERSION_CHECK(13, 1, 0) || \
	HEDLEY_INTEL_VERSION_CHECK(13, 0, 0)
#define simde_bswap64(v) __builtin_bswap64(v)
#elif HEDLEY_MSVC_VERSION_CHECK(13, 10, 0)
#define simde_bswap64(v) _byteswap_uint64(v)
#else
SIMDE_FUNCTION_ATTRIBUTES
uint64_t simde_bswap64(uint64_t v)
{
	return ((v & (((uint64_t)0xff) << 56)) >> 56) |
	       ((v & (((uint64_t)0xff) << 48)) >> 40) |
	       ((v & (((uint64_t)0xff) << 40)) >> 24) |
	       ((v & (((uint64_t)0xff) << 32)) >> 8) |
	       ((v & (((uint64_t)0xff) << 24)) << 8) |
	       ((v & (((uint64_t)0xff) << 16)) << 24) |
	       ((v & (((uint64_t)0xff) << 8)) << 40) |
	       ((v & (((uint64_t)0xff))) << 56);
}
#endif

#if !defined(SIMDE_ENDIAN_ORDER)
#error Unknown byte order; please file a bug
#else
#if SIMDE_ENDIAN_ORDER == SIMDE_ENDIAN_LITTLE
#define simde_endian_bswap64_be(value) simde_bswap64(value)
#define simde_endian_bswap64_le(value) (value)
#elif SIMDE_ENDIAN_ORDER == SIMDE_ENDIAN_BIG
#define simde_endian_bswap64_be(value) (value)
#define simde_endian_bswap64_le(value) simde_bswap64(value)
#endif
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

#if !defined(SIMDE_FLOAT64_TYPE)
#define SIMDE_FLOAT64_TYPE double
#define SIMDE_FLOAT64_C(value) value
#else
#define SIMDE_FLOAT32_C(value) ((SIMDE_FLOAT64_TYPE)value)
#endif
typedef SIMDE_FLOAT64_TYPE simde_float64;

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
	defined(__ALTIVEC__) || defined(__wasm_simd128__)
#define SIMDE_ASSUME_VECTORIZATION
#endif
#endif

#if HEDLEY_HAS_WARNING("-Wbad-function-cast")
#define SIMDE_CONVERT_FTOI(T, v)                                    \
	HEDLEY_DIAGNOSTIC_PUSH                                      \
	_Pragma("clang diagnostic ignored \"-Wbad-function-cast\"") \
		HEDLEY_STATIC_CAST(T, (v)) HEDLEY_DIAGNOSTIC_POP
#else
#define SIMDE_CONVERT_FTOI(T, v) ((T)(v))
#endif

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#define SIMDE_CHECKED_REINTERPRET_CAST(to, from, value) \
	(_Generic((value), to : (value), from : ((to)(value))))
#define SIMDE_CHECKED_STATIC_CAST(to, from, value) \
	(_Generic((value), to : (value), from : ((to)(value))))
#else
#define SIMDE_CHECKED_REINTERPRET_CAST(to, from, value) \
	HEDLEY_REINTERPRET_CAST(to, value)
#define SIMDE_CHECKED_STATIC_CAST(to, from, value) HEDLEY_STATIC_CAST(to, value)
#endif

#if HEDLEY_HAS_WARNING("-Wfloat-equal")
#define SIMDE_DIAGNOSTIC_DISABLE_FLOAT_EQUAL \
	_Pragma("clang diagnostic ignored \"-Wfloat-equal\"")
#elif HEDLEY_GCC_VERSION_CHECK(3, 0, 0)
#define SIMDE_DIAGNOSTIC_DISABLE_FLOAT_EQUAL \
	_Pragma("GCC diagnostic ignored \"-Wfloat-equal\"")
#else
#define SIMDE_DIAGNOSTIC_DISABLE_FLOAT_EQUAL
#endif

/* Some functions can trade accuracy for speed.  For those functions
   you can control the trade-off using this macro.  Possible values:

   0: prefer speed
   1: reasonable trade-offs
   2: prefer accuracy */
#if !defined(SIMDE_ACCURACY_PREFERENCE)
#define SIMDE_ACCURACY_PREFERENCE 1
#endif

#if defined(__STDC_HOSTED__)
#define SIMDE_STDC_HOSTED __STDC_HOSTED__
#else
#if defined(HEDLEY_PGI_VERSION_CHECK) || defined(HEDLEY_MSVC_VERSION_CHECK)
#define SIMDE_STDC_HOSTED 1
#else
#define SIMDE_STDC_HOSTED 0
#endif
#endif

/* Try to deal with environments without a standard library. */
#if !defined(simde_memcpy) || !defined(simde_memset)
#if !defined(SIMDE_NO_STRING_H) && defined(__has_include)
#if __has_include(<string.h>)
#include <string.h>
#if !defined(simde_memcpy)
#define simde_memcpy(dest, src, n) memcpy(dest, src, n)
#endif
#if !defined(simde_memset)
#define simde_memset(s, c, n) memset(s, c, n)
#endif
#else
#define SIMDE_NO_STRING_H
#endif
#endif
#endif
#if !defined(simde_memcpy) || !defined(simde_memset)
#if !defined(SIMDE_NO_STRING_H) && (SIMDE_STDC_HOSTED == 1)
#include <string.h>
#if !defined(simde_memcpy)
#define simde_memcpy(dest, src, n) memcpy(dest, src, n)
#endif
#if !defined(simde_memset)
#define simde_memset(s, c, n) memset(s, c, n)
#endif
#elif (HEDLEY_HAS_BUILTIN(__builtin_memcpy) &&  \
       HEDLEY_HAS_BUILTIN(__builtin_memset)) || \
	HEDLEY_GCC_VERSION_CHECK(4, 2, 0)
#if !defined(simde_memcpy)
#define simde_memcpy(dest, src, n) __builtin_memcpy(dest, src, n)
#endif
#if !defined(simde_memset)
#define simde_memset(s, c, n) __builtin_memset(s, c, n)
#endif
#else
/* These are meant to be portable, not fast.  If you're hitting them you
     * should think about providing your own (by defining the simde_memcpy
     * macro prior to including any SIMDe files) or submitting a patch to
     * SIMDe so we can detect your system-provided memcpy/memset, like by
     * adding your compiler to the checks for __builtin_memcpy and/or
     * __builtin_memset. */
#if !defined(simde_memcpy)
SIMDE_FUNCTION_ATTRIBUTES
void simde_memcpy_(void *dest, const void *src, size_t len)
{
	char *dest_ = HEDLEY_STATIC_CAST(char *, dest);
	char *src_ = HEDLEY_STATIC_CAST(const char *, src);
	for (size_t i = 0; i < len; i++) {
		dest_[i] = src_[i];
	}
}
#define simde_memcpy(dest, src, n) simde_memcpy_(dest, src, n)
#endif

#if !defined(simde_memset)
SIMDE_FUNCTION_ATTRIBUTES
void simde_memset_(void *s, int c, size_t len)
{
	char *s_ = HEDLEY_STATIC_CAST(char *, s);
	char c_ = HEDLEY_STATIC_CAST(char, c);
	for (size_t i = 0; i < len; i++) {
		s_[i] = c_[i];
	}
}
#define simde_memset(s, c, n) simde_memset_(s, c, n)
#endif
#endif /* !defined(SIMDE_NO_STRING_H) && (SIMDE_STDC_HOSTED == 1) */
#endif /* !defined(simde_memcpy) || !defined(simde_memset) */

#include "simde-math.h"

#if defined(FE_ALL_EXCEPT)
#define SIMDE_HAVE_FENV_H
#elif defined(__has_include)
#if __has_include(<fenv.h>)
#include <fenv.h>
#define SIMDE_HAVE_FENV_H
#endif
#elif SIMDE_STDC_HOSTED == 1
#include <fenv.h>
#define SIMDE_HAVE_FENV_H
#endif

#if defined(EXIT_FAILURE)
#define SIMDE_HAVE_STDLIB_H
#elif defined(__has_include)
#if __has_include(<stdlib.h>)
#include <stdlib.h>
#define SIMDE_HAVE_STDLIB_H
#endif
#elif SIMDE_STDC_HOSTED == 1
#include <stdlib.h>
#define SIMDE_HAVE_STDLIB_H
#endif

#if defined(__has_include)
#if defined(__cplusplus) && (__cplusplus >= 201103L) && __has_include(<cfenv>)
#include <cfenv>
#elif __has_include(<fenv.h>)
#include <fenv.h>
#endif
#if __has_include(<stdlib.h>)
#include <stdlib.h>
#endif
#elif SIMDE_STDC_HOSTED == 1
#include <stdlib.h>
#include <fenv.h>
#endif

#include "check.h"

/* Sometimes we run into problems with specific versions of compilers
   which make the native versions unusable for us.  Often this is due
   to missing functions, sometimes buggy implementations, etc.  These
   macros are how we check for specific bugs.  As they are fixed we'll
   start only defining them for problematic compiler versions. */

#if !defined(SIMDE_IGNORE_COMPILER_BUGS)
#if defined(HEDLEY_GCC_VERSION)
#if !HEDLEY_GCC_VERSION_CHECK(4, 9, 0)
#define SIMDE_BUG_GCC_REV_208793
#endif
#if !HEDLEY_GCC_VERSION_CHECK(5, 0, 0)
#define SIMDE_BUG_GCC_BAD_MM_SRA_EPI32 /* TODO: find relevant bug or commit */
#endif
#if !HEDLEY_GCC_VERSION_CHECK(4, 6, 0)
#define SIMDE_BUG_GCC_BAD_MM_EXTRACT_EPI8 /* TODO: find relevant bug or commit */
#endif
#if !HEDLEY_GCC_VERSION_CHECK(8, 0, 0)
#define SIMDE_BUG_GCC_REV_247851
#endif
#if !HEDLEY_GCC_VERSION_CHECK(10, 0, 0)
#define SIMDE_BUG_GCC_REV_274313
#define SIMDE_BUG_GCC_91341
#endif
#if !HEDLEY_GCC_VERSION_CHECK(9, 0, 0) && defined(SIMDE_ARCH_AARCH64)
#define SIMDE_BUG_GCC_ARM_SHIFT_SCALAR
#endif
#if defined(SIMDE_ARCH_X86) && !defined(SIMDE_ARCH_AMD64)
#define SIMDE_BUG_GCC_94482
#endif
#if !HEDLEY_GCC_VERSION_CHECK(9, 4, 0) && defined(SIMDE_ARCH_AARCH64)
#define SIMDE_BUG_GCC_94488
#endif
#if defined(SIMDE_ARCH_POWER)
#define SIMDE_BUG_GCC_95227
#endif
#define SIMDE_BUG_GCC_95399
#elif defined(__clang__)
#if defined(SIMDE_ARCH_AARCH64)
#define SIMDE_BUG_CLANG_45541
#endif
#endif
#if defined(HEDLEY_EMSCRIPTEN_VERSION)
#define SIMDE_BUG_EMSCRIPTEN_MISSING_IMPL /* Placeholder for (as yet) unfiled issues. */
#define SIMDE_BUG_EMSCRIPTEN_5242
#endif
#endif

/* GCC and Clang both have the same issue:
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=95144
 * https://bugs.llvm.org/show_bug.cgi?id=45931
 */
#if HEDLEY_HAS_WARNING("-Wsign-conversion") || HEDLEY_GCC_VERSION_CHECK(4, 3, 0)
#define SIMDE_BUG_IGNORE_SIGN_CONVERSION(expr)                                      \
	(__extension__({                                                            \
		HEDLEY_DIAGNOSTIC_PUSH                                              \
		HEDLEY_DIAGNOSTIC_POP                                               \
		_Pragma("GCC diagnostic ignored \"-Wsign-conversion\"") __typeof__( \
			expr) simde_bug_ignore_sign_conversion_v_ = (expr);         \
		HEDLEY_DIAGNOSTIC_PUSH                                              \
		simde_bug_ignore_sign_conversion_v_;                                \
	}))
#else
#define SIMDE_BUG_IGNORE_SIGN_CONVERSION(expr) (expr)
#endif

#endif /* !defined(SIMDE_COMMON_H) */
