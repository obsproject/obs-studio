/*
 * Copyright (c) 2013-2014 Hugh Bailey <obs.jim@gmail.com>
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

enum event_type {
	EVENT_TYPE_AUTO,
	EVENT_TYPE_MANUAL
};

struct event_data;
typedef struct event_data *event_t;

EXPORT int  event_init(event_t *event, enum event_type type);
EXPORT void event_destroy(event_t event);
EXPORT int  event_wait(event_t event);
EXPORT int  event_timedwait(event_t event, unsigned long milliseconds);
EXPORT int  event_try(event_t event);
EXPORT int  event_signal(event_t event);
EXPORT void event_reset(event_t event);


#ifdef __cplusplus
}
#endif
