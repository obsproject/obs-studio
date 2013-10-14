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

/*
 *   Allows posix thread usage on windows as well as other operating systems.
 * Use this header if you want to make your code more platform independent.
 *
 *   Also provides a custom platform-independent "event" handler via
 * pthread conditional waits.
 */

#include "c99defs.h"

#ifdef _MSC_VER
#include "../../w32-pthreads/pthread.h"
#include "../../w32-pthreads/semaphore.h"
#else
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

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
	if ((code = pthread_cond_wait(&event->cond, &event->mutex)) == 0) {
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
