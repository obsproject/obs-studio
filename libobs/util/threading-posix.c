/*
 * Copyright (c) 2014 Hugh Bailey <obs.jim@gmail.com>
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

#ifdef __APPLE__
#include <sys/time.h>
#endif

#include "bmem.h"
#include "threading.h"

struct event_data {
	pthread_mutex_t mutex;
	pthread_cond_t  cond;
	volatile bool   signalled;
	bool            manual;
};

int event_init(event_t *event, enum event_type type)
{
	int code = 0;

	struct event_data *data = bzalloc(sizeof(struct event_data));

	if ((code = pthread_mutex_init(&data->mutex, NULL)) < 0) {
		bfree(data);
		return code;
	}

	if ((code = pthread_cond_init(&data->cond, NULL)) < 0) {
		pthread_mutex_destroy(&data->mutex);
		bfree(data);
		return code;
	}

	data->manual = (type == EVENT_TYPE_MANUAL);
	data->signalled = false;
	*event = data;

	return 0;
}

void event_destroy(event_t event)
{
	if (event) {
		pthread_mutex_destroy(&event->mutex);
		pthread_cond_destroy(&event->cond);
		bfree(event);
	}
}

int event_wait(event_t event)
{
	int code = 0;
	pthread_mutex_lock(&event->mutex);
	if (!event->signalled)
		code = pthread_cond_wait(&event->cond, &event->mutex);

	if (code == 0) {
		if (!event->manual)
			event->signalled = false;
		pthread_mutex_unlock(&event->mutex);
	}

	return code;
}

static inline void add_ms_to_ts(struct timespec *ts,
		unsigned long milliseconds)
{
	ts->tv_sec += milliseconds/1000;
	ts->tv_nsec += (milliseconds%1000)*1000000;
	if (ts->tv_nsec > 1000000000) {
		ts->tv_sec += 1;
		ts->tv_nsec -= 1000000000;
	}
}

int event_timedwait(event_t event, unsigned long milliseconds)
{
	int code = 0;
	pthread_mutex_lock(&event->mutex);
	if (!event->signalled) {
		struct timespec ts;
#ifdef __APPLE__
		struct timeval tv;
		gettimeofday(&tv, NULL);
		ts.tv_sec  = tv.tv_sec;
		ts.tv_nsec = tv.tv_usec * 1000;
#else
		clock_gettime(CLOCK_REALTIME, &ts);
#endif
		add_ms_to_ts(&ts, milliseconds);
		code = pthread_cond_timedwait(&event->cond, &event->mutex, &ts);
	}

	if (code == 0) {
		if (!event->manual)
			event->signalled = false;
	}

	pthread_mutex_unlock(&event->mutex);

	return code;
}

int event_try(event_t event)
{
	int ret = EAGAIN;

	pthread_mutex_lock(&event->mutex);
	if (event->signalled) {
		if (!event->manual)
			event->signalled = false;
		ret = 0;
	}
	pthread_mutex_unlock(&event->mutex);

	return ret;
}

int event_signal(event_t event)
{
	int code = 0;

	pthread_mutex_lock(&event->mutex);
	code = pthread_cond_signal(&event->cond);
	event->signalled = true;
	pthread_mutex_unlock(&event->mutex);

	return code;
}

void event_reset(event_t event)
{
	pthread_mutex_lock(&event->mutex);
	event->signalled = false;
	pthread_mutex_unlock(&event->mutex);
}
