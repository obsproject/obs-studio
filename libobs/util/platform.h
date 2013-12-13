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

#include <stdio.h>
#include <wchar.h>
#include <sys/types.h>
#include "c99defs.h"

/*
 * Platform-independent functions for Accessing files, encoding, DLLs,
 * sleep, timer, and timing.
 */

#ifdef __cplusplus
extern "C" {
#endif

EXPORT FILE *os_wfopen(const wchar_t *path, const char *mode);
EXPORT FILE *os_fopen(const char *path, const char *mode);
EXPORT off_t os_fgetsize(FILE *file);

EXPORT size_t os_fread_mbs(FILE *file, char **pstr);
EXPORT size_t os_fread_utf8(FILE *file, char **pstr);

/* functions purely for convenience */
EXPORT char *os_quick_read_utf8_file(const char *path);
EXPORT bool os_quick_write_utf8_file(const char *path, const char *str,
		size_t len, bool marker);
EXPORT char *os_quick_read_mbs_file(const char *path);
EXPORT bool os_quick_write_mbs_file(const char *path, const char *str,
		size_t len);

EXPORT size_t os_mbs_to_wcs(const char *str, size_t len, wchar_t **pstr);
EXPORT size_t os_utf8_to_wcs(const char *str, size_t len, wchar_t **pstr);
EXPORT size_t os_wcs_to_mbs(const wchar_t *str, size_t len, char **pstr);
EXPORT size_t os_wcs_to_utf8(const wchar_t *str, size_t len, char **pstr);

EXPORT size_t os_utf8_to_mbs(const char *str, size_t len, char **pstr);
EXPORT size_t os_mbs_to_utf8(const char *str, size_t len, char **pstr);

EXPORT void *os_dlopen(const char *path);
EXPORT void *os_dlsym(void *module, const char *func);
EXPORT void os_dlclose(void *module);

EXPORT void os_sleepto_ns(uint64_t time_target);
EXPORT void os_sleep_ms(uint32_t duration);

EXPORT uint64_t os_gettime_ns(void);

EXPORT char *os_get_home_path(void);

EXPORT bool os_file_exists(const char *path);

#define MKDIR_EXISTS   1
#define MKDIR_SUCCESS  0
#define MKDIR_ERROR   -1

EXPORT int os_mkdir(const char *path);

#ifdef _MSC_VER
EXPORT int fseeko(FILE *stream, off_t offset, int whence);
EXPORT off_t ftello(FILE *stream);
#define strtoll _strtoi64
#endif

#ifdef __cplusplus
}
#endif
