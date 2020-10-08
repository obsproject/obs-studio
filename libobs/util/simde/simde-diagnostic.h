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

/* SIMDe targets a very wide range of standards and compilers, and our
 * goal is to compile cleanly even with extremely aggressive warnings
 * (i.e., -Weverything in clang, -Wextra in GCC, /W4 for MSVC, etc.)
 * treated as errors.
 *
 * While our preference is to resolve the underlying issue a given
 * diagnostic is warning us about, sometimes that's not possible.
 * Fixing a warning in one compiler may cause problems in another.
 * Sometimes a warning doesn't really apply to us (false positives),
 * and sometimes adhering to a warning would mean dropping a feature
 * we *know* the compiler supports since we have tested specifically
 * for the compiler or feature.
 *
 * When practical, warnings are only disabled for specific code.  For
 * a list of warnings which are enabled by default in all SIMDe code,
 * see SIMDE_DISABLE_UNWANTED_DIAGNOSTICS.  Note that we restore the
 * warning stack when SIMDe is done parsing, so code which includes
 * SIMDe is not deprived of these warnings.
 */

#if !defined(SIMDE_DIAGNOSTIC_H)

#include "hedley.h"

/* This is only to help us implement functions like _mm_undefined_ps. */
#if defined(SIMDE_DIAGNOSTIC_DISABLE_UNINITIALIZED_)
#undef SIMDE_DIAGNOSTIC_DISABLE_UNINITIALIZED_
#endif
#if HEDLEY_HAS_WARNING("-Wuninitialized")
#define SIMDE_DIAGNOSTIC_DISABLE_UNINITIALIZED_ \
	_Pragma("clang diagnostic ignored \"-Wuninitialized\"")
#elif HEDLEY_GCC_VERSION_CHECK(4, 2, 0)
#define SIMDE_DIAGNOSTIC_DISABLE_UNINITIALIZED_ \
	_Pragma("GCC diagnostic ignored \"-Wuninitialized\"")
#elif HEDLEY_PGI_VERSION_CHECK(19, 10, 0)
#define SIMDE_DIAGNOSTIC_DISABLE_UNINITIALIZED_ _Pragma("diag_suppress 549")
#elif HEDLEY_SUNPRO_VERSION_CHECK(5, 14, 0) && defined(__cplusplus)
#define SIMDE_DIAGNOSTIC_DISABLE_UNINITIALIZED_ \
	_Pragma("error_messages(off,SEC_UNINITIALIZED_MEM_READ,SEC_UNDEFINED_RETURN_VALUE,unassigned)")
#elif HEDLEY_SUNPRO_VERSION_CHECK(5, 14, 0)
#define SIMDE_DIAGNOSTIC_DISABLE_UNINITIALIZED_ \
	_Pragma("error_messages(off,SEC_UNINITIALIZED_MEM_READ,SEC_UNDEFINED_RETURN_VALUE)")
#elif HEDLEY_SUNPRO_VERSION_CHECK(5, 12, 0) && defined(__cplusplus)
#define SIMDE_DIAGNOSTIC_DISABLE_UNINITIALIZED_ \
	_Pragma("error_messages(off,unassigned)")
#elif HEDLEY_TI_VERSION_CHECK(16, 9, 9) ||       \
	HEDLEY_TI_CL6X_VERSION_CHECK(8, 0, 0) || \
	HEDLEY_TI_CL7X_VERSION_CHECK(1, 2, 0) || \
	HEDLEY_TI_CLPRU_VERSION_CHECK(2, 3, 2)
#define SIMDE_DIAGNOSTIC_DISABLE_UNINITIALIZED_ _Pragma("diag_suppress 551")
#elif HEDLEY_INTEL_VERSION_CHECK(13, 0, 0)
#define SIMDE_DIAGNOSTIC_DISABLE_UNINITIALIZED_ _Pragma("warning(disable:592)")
#elif HEDLEY_MSVC_VERSION_CHECK(19, 0, 0) && !defined(__MSVC_RUNTIME_CHECKS)
#define SIMDE_DIAGNOSTIC_DISABLE_UNINITIALIZED_ \
	__pragma(warning(disable : 4700))
#endif

/* GCC emits a lot of "notes" about the ABI being different for things
 * in newer versions of GCC.  We don't really care because all our
 * functions are inlined and don't generate ABI. */
#if HEDLEY_GCC_VERSION_CHECK(7, 0, 0)
#define SIMDE_DIAGNOSTIC_DISABLE_PSABI_ \
	_Pragma("GCC diagnostic ignored \"-Wpsabi\"")
#else
#define SIMDE_DIAGNOSTIC_DISABLE_PSABI_
#endif

/* Since MMX uses x87 FP registers, you're supposed to call _mm_empty()
 * after each MMX function before any floating point instructions.
 * Some compilers warn about functions which use MMX functions but
 * don't call _mm_empty().  However, since SIMDe is implementyng the
 * MMX API we shouldn't be calling _mm_empty(); we leave it to the
 * caller to invoke simde_mm_empty(). */
#if HEDLEY_INTEL_VERSION_CHECK(19, 0, 0)
#define SIMDE_DIAGNOSTIC_DISABLE_NO_EMMS_INSTRUCTION_ \
	_Pragma("warning(disable:13200 13203)")
#elif defined(HEDLEY_MSVC_VERSION)
#define SIMDE_DIAGNOSTIC_DISABLE_NO_EMMS_INSTRUCTION_ \
	__pragma(warning(disable : 4799))
#else
#define SIMDE_DIAGNOSTIC_DISABLE_NO_EMMS_INSTRUCTION_
#endif

/* Intel is pushing people to use OpenMP SIMD instead of Cilk+, so they
 * emit a diagnostic if you use #pragma simd instead of
 * #pragma omp simd.  SIMDe supports OpenMP SIMD, you just need to
 * compile with -qopenmp or -qopenmp-simd and define
 * SIMDE_ENABLE_OPENMP.  Cilk+ is just a fallback. */
#if HEDLEY_INTEL_VERSION_CHECK(18, 0, 0)
#define SIMDE_DIAGNOSTIC_DISABLE_SIMD_PRAGMA_DEPRECATED_ \
	_Pragma("warning(disable:3948)")
#else
#define SIMDE_DIAGNOSTIC_DISABLE_SIMD_PRAGMA_DEPRECATED_
#endif

#if defined(HEDLEY_MSVC_VERSION)
#define SIMDE_DIAGNOSTIC_DISABLE_NON_CONSTANT_AGGREGATE_INITIALIZER_ \
	__pragma(warning(disable : 4204))
#else
#define SIMDE_DIAGNOSTIC_DISABLE_NON_CONSTANT_AGGREGATE_INITIALIZER_
#endif

/* This warning needs a lot of work.  It is triggered if all you do is
 * pass the value to memcpy/__builtin_memcpy, or if you initialize a
 * member of the union, even if that member takes up the entire union.
 * Last tested with clang-10, hopefully things will improve in the
 * future; if clang fixes this I'd love to enable it. */
#if HEDLEY_HAS_WARNING("-Wconditional-uninitialized")
#define SIMDE_DIAGNOSTIC_DISABLE_CONDITIONAL_UNINITIALIZED_ \
	_Pragma("clang diagnostic ignored \"-Wconditional-uninitialized\"")
#else
#define SIMDE_DIAGNOSTIC_DISABLE_CONDITIONAL_UNINITIALIZED_
#endif

/* This warning is meant to catch things like `0.3 + 0.4 == 0.7`, which
 * will is false.  However, SIMDe uses these operations exclusively
 * for things like _mm_cmpeq_ps, for which we really do want to check
 * for equality (or inequality).
 *
 * If someone wants to put together a SIMDE_FLOAT_EQUAL(a, op, b) macro
 * which just wraps a check in some code do disable this diagnostic I'd
 * be happy to accept it. */
#if HEDLEY_HAS_WARNING("-Wfloat-equal") || HEDLEY_GCC_VERSION_CHECK(3, 0, 0)
#define SIMDE_DIAGNOSTIC_DISABLE_FLOAT_EQUAL_ \
	_Pragma("GCC diagnostic ignored \"-Wfloat-equal\"")
#else
#define SIMDE_DIAGNOSTIC_DISABLE_FLOAT_EQUAL_
#endif

/* This is because we use HEDLEY_STATIC_ASSERT for static assertions.
 * If Hedley can't find an implementation it will preprocess to
 * nothing, which means there will be a trailing semi-colon. */
#if HEDLEY_HAS_WARNING("-Wextra-semi")
#define SIMDE_DIAGNOSTIC_DISABLE_EXTRA_SEMI_ \
	_Pragma("clang diagnostic ignored \"-Wextra-semi\"")
#elif HEDLEY_GCC_VERSION_CHECK(8, 1, 0) && defined(__cplusplus)
#define SIMDE_DIAGNOSTIC_DISABLE_EXTRA_SEMI_ \
	_Pragma("GCC diagnostic ignored \"-Wextra-semi\"")
#else
#define SIMDE_DIAGNOSTIC_DISABLE_EXTRA_SEMI_
#endif

/* We do use a few variadic macros, which technically aren't available
 * until C99 and C++11, but every compiler I'm aware of has supported
 * them for much longer.  That said, usage is isolated to the test
 * suite and compilers known to support them. */
#if HEDLEY_HAS_WARNING("-Wvariadic-macros") || HEDLEY_GCC_VERSION_CHECK(4, 0, 0)
#if HEDLEY_HAS_WARNING("-Wc++98-compat-pedantic")
#define SIMDE_DIAGNOSTIC_DISABLE_VARIADIC_MACROS_                          \
	_Pragma("clang diagnostic ignored \"-Wvariadic-macros\"") _Pragma( \
		"clang diagnostic ignored \"-Wc++98-compat-pedantic\"")
#else
#define SIMDE_DIAGNOSTIC_DISABLE_VARIADIC_MACROS_ \
	_Pragma("GCC diagnostic ignored \"-Wvariadic-macros\"")
#endif
#else
#define SIMDE_DIAGNOSTIC_DISABLE_VARIADIC_MACROS_
#endif

/* Triggered when assigning a float to a double implicitly.  We use
 * explicit casts in SIMDe, this is only used in the test suite. */
#if HEDLEY_HAS_WARNING("-Wdouble-promotion")
#define SIMDE_DIAGNOSTIC_DISABLE_DOUBLE_PROMOTION_ \
	_Pragma("clang diagnostic ignored \"-Wdouble-promotion\"")
#else
#define SIMDE_DIAGNOSTIC_DISABLE_DOUBLE_PROMOTION_
#endif

/* Several compilers treat conformant array parameters as VLAs.  We
 * test to make sure we're in C mode (C++ doesn't support CAPs), and
 * that the version of the standard supports CAPs.  We also blacklist
 * some buggy compilers like MSVC (the logic is in Hedley if you want
 * to take a look), but with certain warnings enabled some compilers
 * still like to emit a diagnostic. */
#if HEDLEY_HAS_WARNING("-Wvla")
#define SIMDE_DIAGNOSTIC_DISABLE_VLA_ \
	_Pragma("clang diagnostic ignored \"-Wvla\"")
#elif HEDLEY_GCC_VERSION_CHECK(4, 3, 0)
#define SIMDE_DIAGNOSTIC_DISABLE_VLA_ \
	_Pragma("GCC diagnostic ignored \"-Wvla\"")
#else
#define SIMDE_DIAGNOSTIC_DISABLE_VLA_
#endif

#if HEDLEY_HAS_WARNING("-Wused-but-marked-unused")
#define SIMDE_DIAGNOSTIC_DISABLE_USED_BUT_MARKED_UNUSED_ \
	_Pragma("clang diagnostic ignored \"-Wused-but-marked-unused\"")
#else
#define SIMDE_DIAGNOSTIC_DISABLE_USED_BUT_MARKED_UNUSED_
#endif

#if HEDLEY_HAS_WARNING("-Wunused-function")
#define SIMDE_DIAGNOSTIC_DISABLE_UNUSED_FUNCTION_ \
	_Pragma("clang diagnostic ignored \"-Wunused-function\"")
#elif HEDLEY_GCC_VERSION_CHECK(3, 4, 0)
#define SIMDE_DIAGNOSTIC_DISABLE_UNUSED_FUNCTION_ \
	_Pragma("GCC diagnostic ignored \"-Wunused-function\"")
#else
#define SIMDE_DIAGNOSTIC_DISABLE_UNUSED_FUNCTION_
#endif

#if HEDLEY_HAS_WARNING("-Wpass-failed")
#define SIMDE_DIAGNOSTIC_DISABLE_PASS_FAILED_ \
	_Pragma("clang diagnostic ignored \"-Wpass-failed\"")
#else
#define SIMDE_DIAGNOSTIC_DISABLE_PASS_FAILED_
#endif

/* https://github.com/nemequ/simde/issues/277 */
#if defined(HEDLEY_GCC_VERSION) && HEDLEY_GCC_VERSION_CHECK(4, 6, 0) && \
	!HEDLEY_GCC_VERSION_CHECK(6, 0, 0) && defined(__cplusplus)
#define SIMDE_DIAGNOSTIC_DISABLE_BUGGY_UNUSED_BUT_SET_VARIBALE \
	_Pragma("GCC diagnostic ignored \"-Wunused-but-set-variable\"")
#else
#define SIMDE_DIAGNOSTIC_DISABLE_BUGGY_UNUSED_BUT_SET_VARIBALE
#endif

/* Some compilers, such as clang, may use `long long` for 64-bit
 * integers, but `long long` triggers a diagnostic with
 * -Wc++98-compat-pedantic which says 'long long' is incompatible with
 * C++98. */
#if HEDLEY_HAS_WARNING("-Wc++98-compat-pedantic")
#define SIMDE_DIAGNOSTIC_DISABLE_CPP98_COMPAT_PEDANTIC \
	_Pragma("clang diagnostic ignored \"-Wc++98-compat-pedantic\"")
#else
#define SIMDE_DIAGNOSTIC_DISABLE_CPP98_COMPAT_PEDANTIC
#endif

#define SIMDE_DISABLE_UNWANTED_DIAGNOSTICS                           \
	SIMDE_DIAGNOSTIC_DISABLE_PSABI_                              \
	SIMDE_DIAGNOSTIC_DISABLE_NO_EMMS_INSTRUCTION_                \
	SIMDE_DIAGNOSTIC_DISABLE_SIMD_PRAGMA_DEPRECATED_             \
	SIMDE_DIAGNOSTIC_DISABLE_CONDITIONAL_UNINITIALIZED_          \
	SIMDE_DIAGNOSTIC_DISABLE_FLOAT_EQUAL_                        \
	SIMDE_DIAGNOSTIC_DISABLE_NON_CONSTANT_AGGREGATE_INITIALIZER_ \
	SIMDE_DIAGNOSTIC_DISABLE_EXTRA_SEMI_                         \
	SIMDE_DIAGNOSTIC_DISABLE_VLA_                                \
	SIMDE_DIAGNOSTIC_DISABLE_USED_BUT_MARKED_UNUSED_             \
	SIMDE_DIAGNOSTIC_DISABLE_UNUSED_FUNCTION_                    \
	SIMDE_DIAGNOSTIC_DISABLE_PASS_FAILED_                        \
	SIMDE_DIAGNOSTIC_DISABLE_CPP98_COMPAT_PEDANTIC               \
	SIMDE_DIAGNOSTIC_DISABLE_BUGGY_UNUSED_BUT_SET_VARIBALE

#endif
