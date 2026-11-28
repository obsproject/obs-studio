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

#include <assert.h>
#include <sys/stat.h>

#include "file-watcher.h"
#include "platform.h"
#include "threading.h"
#include "task.h"
#include "bmem.h"

struct file_watcher {
	char *path;
	file_watcher_cb callback;
	void *callback_data;

	pthread_mutex_t callback_mutex;
	volatile bool destroying;
	volatile bool check_pending;

	int64_t size;
	time_t mtime;
};

static pthread_mutex_t watcher_mutex = PTHREAD_MUTEX_INITIALIZER;
static os_task_queue_t *watcher_queue = NULL;
static long watcher_refs = 0;

static bool update_file_state(struct file_watcher *fw)
{
	struct stat st;
	int64_t size = -1;
	time_t mtime = 0;
	bool changed;

	if (os_stat(fw->path, &st) == 0) {
		size = (int64_t)st.st_size;
		mtime = st.st_mtime;
	}

	changed = size != fw->size || mtime != fw->mtime;

	fw->size = size;
	fw->mtime = mtime;
	return changed;
}

static void init_task(void *param)
{
	struct file_watcher *fw = param;

	if (!os_atomic_load_bool(&fw->destroying))
		update_file_state(fw);
}

static void check_task(void *param)
{
	struct file_watcher *fw = param;

	os_atomic_set_bool(&fw->check_pending, false);

	if (os_atomic_load_bool(&fw->destroying))
		return;

	if (!update_file_state(fw))
		return;

	pthread_mutex_lock(&fw->callback_mutex);
	if (!os_atomic_load_bool(&fw->destroying))
		fw->callback(fw->callback_data, fw->path);
	pthread_mutex_unlock(&fw->callback_mutex);
}

static void free_task(void *param)
{
	struct file_watcher *fw = param;

	pthread_mutex_destroy(&fw->callback_mutex);
	bfree(fw->path);
	bfree(fw);
}

file_watcher_t *file_watcher_create(const char *path, file_watcher_cb callback, void *data)
{
	struct file_watcher *fw;

	if (!path || !callback)
		return NULL;

	fw = bzalloc(sizeof(*fw));

	if (pthread_mutex_init(&fw->callback_mutex, NULL) != 0)
		goto fail;

	pthread_mutex_lock(&watcher_mutex);
	if (!watcher_refs)
		watcher_queue = os_task_queue_create();
	if (watcher_queue) {
		watcher_refs++;
	} else {
		goto fail_acquire;
	}
	pthread_mutex_unlock(&watcher_mutex);

	fw->path = bstrdup(path);
	fw->callback = callback;
	fw->callback_data = data;

	os_task_queue_queue_task(watcher_queue, init_task, fw);
	return fw;

fail_acquire:
	pthread_mutex_unlock(&watcher_mutex);
	pthread_mutex_destroy(&fw->callback_mutex);
fail:
	bfree(fw);
	return NULL;
}

void file_watcher_destroy(file_watcher_t *fw)
{
	if (!fw)
		return;

	if (os_task_queue_inside(watcher_queue)) {
		blog(LOG_ERROR,
		     "os_file_watcher_destroy: Cannot call watcher destroy from within a file watcher callback");
		assert(false);
		return;
	}

	/* This mutex guarantees that no callbacks will be executed after this point,
	 * since the task queue can survive past the lifetime of the file watcher. */
	pthread_mutex_lock(&fw->callback_mutex);
	os_atomic_set_bool(&fw->destroying, true);
	pthread_mutex_unlock(&fw->callback_mutex);

	/* Queue a task that frees the watcher using the queue.
	 * This avoids use-after-free if tasks are still pending when the file watcher
	 * is destroyed. */
	os_task_queue_queue_task(watcher_queue, free_task, fw);

	pthread_mutex_lock(&watcher_mutex);
	if (--watcher_refs == 0) {
		os_task_queue_destroy(watcher_queue);
		watcher_queue = NULL;
	}
	assert(watcher_refs >= 0);
	pthread_mutex_unlock(&watcher_mutex);
}

void file_watcher_check(file_watcher_t *fw)
{
	if (!fw)
		return;

	/* Debounce checks to ensure that they don't stack up in a busy queue. */
	if (os_atomic_set_bool(&fw->check_pending, true))
		return;

	assert(watcher_queue);

	os_task_queue_queue_task(watcher_queue, check_task, fw);
}
