/*
 * Copyright (c) 2026 Kyle Manning <tt2468@gmail.com>
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

#include "c99defs.h"

#ifdef __cplusplus
extern "C" {
#endif

struct file_watcher;
typedef struct file_watcher file_watcher_t;

typedef void (*file_watcher_cb)(void *data, char *path);

EXPORT file_watcher_t *file_watcher_create(const char *path, file_watcher_cb callback, void *data);
EXPORT void file_watcher_destroy(file_watcher_t *fw);
EXPORT void file_watcher_check(file_watcher_t *fw);

#ifdef __cplusplus
}
#endif