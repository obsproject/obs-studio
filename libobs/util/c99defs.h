/******************************************************************************
  Copyright (c) 2013 by Hugh Bailey <obs.jim@gmail.com>

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

     1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

     2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

     3. This notice may not be removed or altered from any source
     distribution.
******************************************************************************/

#pragma once

/*
 * Contains hacks for getting some C99 stuff working in VC, things like
 * bool, inline, stdint
 *
 * TODO: Check for VC2013, because it actually supports this C99 stuff.
 */

#ifdef _MSC_VER
#define EXPORT extern __declspec(dllexport)
#else
#define EXPORT extern 
#endif

#ifdef _MSC_VER

#pragma warning (disable : 4996)

#ifndef __cplusplus
#define inline __inline
#endif

#include "vc/stdint.h"
#include "vc/stdbool.h"

#ifndef __off_t_defined
#define __off_t_defined
#if _FILE_OFFSET_BITS == 64
typedef long long off_t;
#else
typedef long off_t;
#endif
typedef int64_t off64_t;
#endif /* __off_t_defined */

#ifdef _WIN64
typedef long long ssize_t;
#else
typedef long ssize_t;
#endif

#else

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

#endif /* _MSC_VER */
