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

struct os_event_data {
	HANDLE handle;
};

struct os_sem_data {
	HANDLE handle;
};

int os_event_init(os_event_t *event, enum os_event_type type)
{
	HANDLE handle;
	struct os_event_data *data;

	handle = CreateEvent(NULL, (type == OS_EVENT_TYPE_MANUAL), FALSE, NULL);
	if (!handle)
		return -1;

	data = bmalloc(sizeof(struct os_event_data));
	data->handle = handle;

	*event = data;
	return 0;
}

void os_event_destroy(os_event_t event)
{
	if (event) {
		CloseHandle(event->handle);
		bfree(event);
	}
}

int os_event_wait(os_event_t event)
{
	DWORD code;

	if (!event)
		return EINVAL;

	code = WaitForSingleObject(event->handle, INFINITE);
	if (code != WAIT_OBJECT_0)
		return EINVAL;

	return 0;
}

int os_event_timedwait(os_event_t event, unsigned long milliseconds)
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

int os_event_try(os_event_t event)
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

int os_event_signal(os_event_t event)
{
	if (!event)
		return EINVAL;

	if (!SetEvent(event->handle))
		return EINVAL;

	return 0;
}

void os_event_reset(os_event_t event)
{
	if (!event)
		return;

	ResetEvent(event->handle);
}

int  os_sem_init(os_sem_t *sem, int value)
{
	HANDLE handle = CreateSemaphore(NULL, (LONG)value, 0x7FFFFFFF, NULL);
	if (!handle)
		return -1;

	*sem = bzalloc(sizeof(struct os_sem_data));
	(*sem)->handle = handle;
	return 0;
}

void os_sem_destroy(os_sem_t sem)
{
	if (sem) {
		CloseHandle(sem->handle);
		bfree(sem);
	}
}

int  os_sem_post(os_sem_t sem)
{
	if (!sem) return -1;
	return ReleaseSemaphore(sem->handle, 1, NULL) ? 0 : -1;
}

int  os_sem_wait(os_sem_t sem)
{
	DWORD ret;

	if (!sem) return -1;
	ret = WaitForSingleObject(sem->handle, INFINITE);
	return (ret == WAIT_OBJECT_0) ? 0 : -1;
}

long os_atomic_inc_long(volatile long *val)
{
	return InterlockedIncrement(val);
}

long os_atomic_dec_long(volatile long *val)
{
	return InterlockedDecrement(val);
}
