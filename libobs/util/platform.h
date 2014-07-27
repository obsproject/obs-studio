/*
 * Copyright (c) 2013 Hugh Bailey <obs.jim@gmail.com>
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
EXPORT int64_t os_fgetsize(FILE *file);

EXPORT int os_fseeki64(FILE *file, int64_t offset, int origin);
EXPORT int64_t os_ftelli64(FILE *file);

EXPORT size_t os_fread_mbs(FILE *file, char **pstr);
EXPORT size_t os_fread_utf8(FILE *file, char **pstr);

/* functions purely for convenience */
EXPORT char *os_quick_read_utf8_file(const char *path);
EXPORT bool os_quick_write_utf8_file(const char *path, const char *str,
		size_t len, bool marker);
EXPORT char *os_quick_read_mbs_file(const char *path);
EXPORT bool os_quick_write_mbs_file(const char *path, const char *str,
		size_t len);

EXPORT size_t os_mbs_to_wcs(const char *str, size_t str_len, wchar_t *dst,
		size_t dst_size);
EXPORT size_t os_utf8_to_wcs(const char *str, size_t len, wchar_t *dst,
		size_t dst_size);
EXPORT size_t os_wcs_to_mbs(const wchar_t *str, size_t len, char *dst,
		size_t dst_size);
EXPORT size_t os_wcs_to_utf8(const wchar_t *str, size_t len, char *dst,
		size_t dst_size);

EXPORT size_t os_mbs_to_wcs_ptr(const char *str, size_t len, wchar_t **pstr);
EXPORT size_t os_utf8_to_wcs_ptr(const char *str, size_t len, wchar_t **pstr);
EXPORT size_t os_wcs_to_mbs_ptr(const wchar_t *str, size_t len, char **pstr);
EXPORT size_t os_wcs_to_utf8_ptr(const wchar_t *str, size_t len, char **pstr);

EXPORT size_t os_utf8_to_mbs_ptr(const char *str, size_t len, char **pstr);
EXPORT size_t os_mbs_to_utf8_ptr(const char *str, size_t len, char **pstr);

EXPORT void *os_dlopen(const char *path);
EXPORT void *os_dlsym(void *module, const char *func);
EXPORT void os_dlclose(void *module);

struct os_cpu_usage_info;
typedef struct os_cpu_usage_info *os_cpu_usage_info_t;

EXPORT os_cpu_usage_info_t os_cpu_usage_info_start(void);
EXPORT double              os_cpu_usage_info_query(os_cpu_usage_info_t info);
EXPORT void                os_cpu_usage_info_destroy(os_cpu_usage_info_t info);

/**
 * Sleeps to a specific time (in nanoseconds).  Doesn't have to be super
 * accurate in terms of actual slept time because the target time is ensured.
 * Returns false if already at or past target time.
 */
EXPORT bool os_sleepto_ns(uint64_t time_target);
EXPORT void os_sleep_ms(uint32_t duration);

EXPORT uint64_t os_gettime_ns(void);

EXPORT char *os_get_config_path(const char *name);

EXPORT bool os_file_exists(const char *path);

struct os_dir;
typedef struct os_dir *os_dir_t;

struct os_dirent {
	char d_name[256];
	bool directory;
};

EXPORT os_dir_t os_opendir(const char *path);
EXPORT struct os_dirent *os_readdir(os_dir_t dir);
EXPORT void os_closedir(os_dir_t dir);

struct os_globent {
	char *path;
	bool directory;
};

struct os_glob_info {
	size_t             gl_pathc;
	struct os_globent *gl_pathv;
};

typedef struct os_glob_info *os_glob_t;

/* currently no flags available */

EXPORT int os_glob(const char *pattern, int flags, os_glob_t *pglob);
EXPORT void os_globfree(os_glob_t pglob);

EXPORT int os_unlink(const char *path);

#define MKDIR_EXISTS   1
#define MKDIR_SUCCESS  0
#define MKDIR_ERROR   -1

EXPORT int os_mkdir(const char *path);

#ifdef _MSC_VER
#define strtoll _strtoi64
#endif

#ifdef __cplusplus
}
#endif
