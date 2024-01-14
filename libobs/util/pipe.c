/*
 * Copyright (c) 2023 Dennis Sädtler <dennis@obsproject.com>
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

#include "pipe.h"
#include "darray.h"
#include "dstr.h"

struct os_process_args {
	DARRAY(char *) arguments;
};

struct os_process_args *os_process_args_create(const char *executable)
{
	struct os_process_args *args = bzalloc(sizeof(struct os_process_args));
	char *str = bstrdup(executable);
	da_push_back(args->arguments, &str);
	/* Last item in argv must be NULL. */
	char *terminator = NULL;
	da_push_back(args->arguments, &terminator);
	return args;
}

void os_process_args_add_arg(struct os_process_args *args, const char *arg)
{
	char *str = bstrdup(arg);
	/* Insert before NULL list terminator. */
	da_insert(args->arguments, args->arguments.num - 1, &str);
}

void os_process_args_add_argf(struct os_process_args *args, const char *format,
			      ...)
{
	va_list va_args;
	struct dstr tmp = {0};

	va_start(va_args, format);
	dstr_vprintf(&tmp, format, va_args);
	da_insert(args->arguments, args->arguments.num - 1, &tmp.array);
	va_end(va_args);
}

size_t os_process_args_get_argc(struct os_process_args *args)
{
	/* Do not count terminating NULL. */
	return args->arguments.num - 1;
}

char **os_process_args_get_argv(const struct os_process_args *args)
{
	return args->arguments.array;
}

void os_process_args_destroy(struct os_process_args *args)
{
	if (!args)
		return;

	for (size_t idx = 0; idx < args->arguments.num; idx++)
		bfree(args->arguments.array[idx]);

	da_free(args->arguments);
	bfree(args);
}
