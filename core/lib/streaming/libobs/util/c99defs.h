/*
 * Copyright (c) 2023 Lain Bailey <lain@obsproject.com>
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

/*
 * Contains hacks for getting some C99 stuff working in VC, things like
 * bool, stdint
 */

#define UNUSED_PARAMETER(param) (void)param

#ifdef _MSC_VER
#define _OBS_DEPRECATED __declspec(deprecated)
#define OBS_NORETURN __declspec(noreturn)
#define FORCE_INLINE __forceinline
#else
#define _OBS_DEPRECATED __attribute__((deprecated))
#define OBS_NORETURN __attribute__((noreturn))
#define FORCE_INLINE inline __attribute__((always_inline))
#endif

#if defined(SWIG_TYPE_TABLE)
#define OBS_DEPRECATED
#else
#define OBS_DEPRECATED _OBS_DEPRECATED
#endif

#if defined(IS_LIBOBS)
#define OBS_EXTERNAL_DEPRECATED
#else
#define OBS_EXTERNAL_DEPRECATED OBS_DEPRECATED
#endif

#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif

#ifdef _MSC_VER
#define PRAGMA_WARN_PUSH _Pragma("warning(push)")
#define PRAGMA_WARN_POP _Pragma("warning(pop)")
#define PRAGMA_WARN_DEPRECATION _Pragma("warning(disable: 4996)")
#define PRAGMA_DISABLE_DEPRECATION _Pragma("warning(disable: 4996)")
#elif defined(__clang__)
#define PRAGMA_WARN_PUSH _Pragma("clang diagnostic push")
#define PRAGMA_WARN_POP _Pragma("clang diagnostic pop")
#define PRAGMA_WARN_DEPRECATION _Pragma("clang diagnostic warning \"-Wdeprecated-declarations\"")
#define PRAGMA_DISABLE_DEPRECATION _Pragma("clang diagnostic ignored \"-Wdeprecated-declarations\"")
#elif defined(__GNUC__)
#define PRAGMA_WARN_PUSH _Pragma("GCC diagnostic push")
#define PRAGMA_WARN_POP _Pragma("GCC diagnostic pop")
#define PRAGMA_WARN_DEPRECATION _Pragma("GCC diagnostic warning \"-Wdeprecated-declarations\"")
#define PRAGMA_DISABLE_DEPRECATION _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
#else
#define PRAGMA_WARN_PUSH
#define PRAGMA_WARN_POP
#define PRAGMA_WARN_DEPRECATION
#define PRAGMA_DISABLE_DEPRECATION
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
