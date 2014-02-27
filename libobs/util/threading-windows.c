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

#include "bmem.h"
#include "threading.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct event_data {
	HANDLE handle;
};

int event_init(event_t *event, enum event_type type)
{
	HANDLE handle;
	struct event_data *data;

	handle = CreateEvent(NULL, (type == EVENT_TYPE_MANUAL), FALSE, NULL);
	if (!handle)
		return -1;

	data = bmalloc(sizeof(struct event_data));
	data->handle = handle;

	*event = data;
	return 0;
}

void event_destroy(event_t event)
{
	if (event) {
		CloseHandle(event->handle);
		bfree(event);
	}
}

int event_wait(event_t event)
{
	DWORD code;

	if (!event)
		return EINVAL;

	code = WaitForSingleObject(event->handle, INFINITE);
	if (code != WAIT_OBJECT_0)
		return EINVAL;

	return 0;
}

int event_timedwait(event_t event, unsigned long milliseconds)
{
	DWORD code;

	if (!event)
		return EINVAL;

	code = WaitForSingleObject(event->handle, milliseconds);
	if (code == WAIT_TIMEOUT)
		return ETIMEDOUT;
	else if (code != WAIT_OBJECT_0)
		return EINVAL;

	return 0;
}

int event_try(event_t event)
{
	DWORD code;

	if (!event)
		return EINVAL;

	code = WaitForSingleObject(event->handle, 0);
	if (code == WAIT_TIMEOUT)
		return EAGAIN;
	else if (code != WAIT_OBJECT_0)
		return EINVAL;

	return 0;
}

int event_signal(event_t event)
{
	if (!event)
		return EINVAL;

	if (!SetEvent(event->handle))
		return EINVAL;

	return 0;
}

void event_reset(event_t event)
{
	if (!event)
		return;

	ResetEvent(event->handle);
}
