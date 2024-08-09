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

#include "c99defs.h"

#ifdef __cplusplus
extern "C" {
#endif

struct os_process_pipe;
typedef struct os_process_pipe os_process_pipe_t;

struct os_process_args;
typedef struct os_process_args os_process_args_t;

EXPORT os_process_pipe_t *os_process_pipe_create(const char *cmd_line,
						 const char *type);
EXPORT os_process_pipe_t *os_process_pipe_create2(const os_process_args_t *args,
						  const char *type);
EXPORT int os_process_pipe_destroy(os_process_pipe_t *pp);

EXPORT size_t os_process_pipe_read(os_process_pipe_t *pp, uint8_t *data,
				   size_t len);
EXPORT size_t os_process_pipe_read_err(os_process_pipe_t *pp, uint8_t *data,
				       size_t len);
EXPORT size_t os_process_pipe_write(os_process_pipe_t *pp, const uint8_t *data,
				    size_t len);

EXPORT struct os_process_args *os_process_args_create(const char *executable);
EXPORT void os_process_args_add_arg(struct os_process_args *args,
				    const char *arg);
#ifndef _MSC_VER
__attribute__((__format__(__printf__, 2, 3)))
#endif
EXPORT void
os_process_args_add_argf(struct os_process_args *args, const char *format, ...);
EXPORT char **os_process_args_get_argv(const struct os_process_args *args);
EXPORT size_t os_process_args_get_argc(struct os_process_args *args);
EXPORT void os_process_args_destroy(struct os_process_args *args);

#ifdef __cplusplus
}
#endif
