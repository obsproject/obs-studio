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

#ifndef BASE_H
#define BASE_H

#include <wctype.h>
#include <stdarg.h>

#include "c99defs.h"

/*
 * Just contains logging/crash related stuff
 */

#ifdef __cplusplus
extern "C" {
#endif

enum log_type {
	LOG_DEBUG,
	LOG_INFO,
	LOG_WARNING,
	LOG_ERROR
};

EXPORT void base_set_log_handler(
		void (*handler)(enum log_type, const char *, va_list));
EXPORT void base_set_crash_handler(void (*handler)(const char *, va_list));

EXPORT void blog(enum log_type type, const char *format, ...);
EXPORT void bcrash(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif
