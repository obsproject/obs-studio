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
#include "../util/threading.h"

#include "decl.h"
#include "proc.h"

struct proc_info {
	struct decl_info func;
	void *data;
	proc_handler_proc_t callback;
};

static inline void proc_info_free(struct proc_info *pi)
{
	decl_info_free(&pi->func);
}

struct proc_handler {
	/* TODO: replace with hash table lookup? */
	pthread_mutex_t mutex;
	DARRAY(struct proc_info) procs;
};

static struct proc_info *getproc(proc_handler_t *handler, const char *name)
{
	for (size_t i = 0; i < handler->procs.num; i++) {
		struct proc_info *info = handler->procs.array + i;

		if (strcmp(info->func.name, name) == 0) {
			return info;
		}
	}

	return NULL;
}

/* ------------------------------------------------------------------------- */

proc_handler_t *proc_handler_create(void)
{
	struct proc_handler *handler = bmalloc(sizeof(struct proc_handler));

	if (pthread_mutex_init_recursive(&handler->mutex) != 0) {
		blog(LOG_ERROR, "Couldn't create proc_handler mutex");
		bfree(handler);
		return NULL;
	}

	da_init(handler->procs);
	return handler;
}

void proc_handler_destroy(proc_handler_t *handler)
{
	if (!handler)
		return;

	for (size_t i = 0; i < handler->procs.num; i++)
		proc_info_free(handler->procs.array + i);

	da_free(handler->procs);
	pthread_mutex_destroy(&handler->mutex);
	bfree(handler);
}

void proc_handler_add(proc_handler_t *handler, const char *decl_string,
		      proc_handler_proc_t proc, void *data)
{
	if (!handler)
		return;

	struct proc_info pi;
	memset(&pi, 0, sizeof(struct proc_info));

	if (!parse_decl_string(&pi.func, decl_string)) {
		blog(LOG_ERROR, "Function declaration invalid: %s",
		     decl_string);
		return;
	}

	pi.callback = proc;
	pi.data = data;

	pthread_mutex_lock(&handler->mutex);

	struct proc_info *existing = getproc(handler, pi.func.name);
	if (existing) {
		blog(LOG_WARNING, "Procedure '%s' already exists",
		     pi.func.name);
		proc_info_free(&pi);
	} else {
		da_push_back(handler->procs, &pi);
	}

	pthread_mutex_unlock(&handler->mutex);
}

bool proc_handler_call(proc_handler_t *handler, const char *name,
		       calldata_t *params)
{
	if (!handler)
		return false;

	pthread_mutex_lock(&handler->mutex);
	struct proc_info *info = getproc(handler, name);
	struct proc_info info_copy;
	if (info)
		info_copy = *info;
	pthread_mutex_unlock(&handler->mutex);

	if (!info)
		return false;

	info_copy.callback(info_copy.data, params);
	return true;
}
