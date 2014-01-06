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

/*
 *   Allows posix thread usage on windows as well as other operating systems.
 * Use this header if you want to make your code more platform independent.
 *
 *   Also provides a custom platform-independent "event" handler via
 * pthread conditional waits.
 */

#include "c99defs.h"

#ifdef _MSC_VER
#include "../../deps/w32-pthreads/pthread.h"
#include "../../deps/w32-pthreads/semaphore.h"
#else
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* this may seem strange, but you can't use it unless it's an initializer */
static inline void pthread_mutex_init_value(pthread_mutex_t *mutex)
{
	pthread_mutex_t init_val = PTHREAD_MUTEX_INITIALIZER;
	*mutex = init_val;
}

struct event_data {
	pthread_mutex_t mutex;
	pthread_cond_t  cond;
	bool            signalled;
	bool            manual;
};

typedef struct event_data event_t;

static inline int event_init(event_t *event, bool manual)
{
	int code = 0;

	if ((code = pthread_mutex_init(&event->mutex, NULL)) < 0)
		return code;

	if ((code = pthread_cond_init(&event->cond, NULL)) < 0)
		pthread_mutex_destroy(&event->mutex);

	event->manual = manual;
	event->signalled = false;

	return code;
}

static inline void event_destroy(event_t *event)
{
	if (event) {
		pthread_mutex_destroy(&event->mutex);
		pthread_cond_destroy(&event->cond);
	}
}

static inline int event_wait(event_t *event)
{
	int code = 0;
	pthread_mutex_lock(&event->mutex);
	if (event->signalled ||
		(code = pthread_cond_wait(&event->cond, &event->mutex)) == 0) {
		if (!event->manual)
			event->signalled = false;
		pthread_mutex_unlock(&event->mutex);
	}

	return code;
}

static inline int event_try(event_t *event)
{
	pthread_mutex_lock(&event->mutex);
	if (event->signalled) {
		if (!event->manual)
			event->signalled = false;
		pthread_mutex_unlock(&event->mutex);
		return 0;
	}
	pthread_mutex_unlock(&event->mutex);

	return EAGAIN;
}

static inline int event_signal(event_t *event)
{
	int code = 0;

	pthread_mutex_lock(&event->mutex);
	code = pthread_cond_signal(&event->cond);
	event->signalled = true;
	pthread_mutex_unlock(&event->mutex);

	return code;
}

static inline void event_reset(event_t *event)
{
	pthread_mutex_lock(&event->mutex);
	event->signalled = false;
	pthread_mutex_unlock(&event->mutex);
}

#ifdef __cplusplus
}
#endif
