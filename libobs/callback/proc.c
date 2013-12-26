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

#include "../util/darray.h"

#include "proc.h"

struct proc_info {
	char *name;
	void (*proc)(calldata_t, void*);
};

static inline void proc_info_free(struct proc_info *pi)
{
	bfree(pi->name);
}

struct proc_handler {
	void                     *data;

	/* TODO: replace with hash table lookup? */
	DARRAY(struct proc_info) procs;
};

proc_handler_t proc_handler_create(void *data)
{
	struct proc_handler *handler = bmalloc(sizeof(struct proc_handler));
	handler->data = data;
	da_init(handler->procs);
	return handler;
}

void proc_handler_destroy(proc_handler_t handler)
{
	if (handler) {
		for (size_t i = 0; i < handler->procs.num; i++)
			proc_info_free(handler->procs.array+i);
		da_free(handler->procs);
		bfree(handler);
	}
}

void proc_handler_add(proc_handler_t handler, const char *name,
		void (*proc)(calldata_t, void*))
{
	struct proc_info pi = {bstrdup(name), proc};
	da_push_back(handler->procs, &pi);
}

bool proc_handler_call(proc_handler_t handler, const char *name,
		calldata_t params)
{
	for (size_t i = 0; i < handler->procs.num; i++) {
		struct proc_info *info = handler->procs.array+i;

		if (strcmp(info->name, name) == 0) {
			info->proc(params, handler->data);
			return true;
		}
	}

	return false;
}
