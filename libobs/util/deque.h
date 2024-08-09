/*
 * Copyright (c) 2023 Lain Bailey <lain@obsproject.com>
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

#include "c99defs.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "bmem.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Double-ended Queue */

struct deque {
	void *data;
	size_t size;

	size_t start_pos;
	size_t end_pos;
	size_t capacity;
};

static inline void deque_init(struct deque *dq)
{
	memset(dq, 0, sizeof(struct deque));
}

static inline void deque_free(struct deque *dq)
{
	bfree(dq->data);
	memset(dq, 0, sizeof(struct deque));
}

static inline void deque_reorder_data(struct deque *dq, size_t new_capacity)
{
	size_t difference;
	uint8_t *data;

	if (!dq->size || !dq->start_pos || dq->end_pos > dq->start_pos)
		return;

	difference = new_capacity - dq->capacity;
	data = (uint8_t *)dq->data + dq->start_pos;
	memmove(data + difference, data, dq->capacity - dq->start_pos);
	dq->start_pos += difference;
}

static inline void deque_ensure_capacity(struct deque *dq)
{
	size_t new_capacity;
	if (dq->size <= dq->capacity)
		return;

	new_capacity = dq->capacity * 2;
	if (dq->size > new_capacity)
		new_capacity = dq->size;

	dq->data = brealloc(dq->data, new_capacity);
	deque_reorder_data(dq, new_capacity);
	dq->capacity = new_capacity;
}

static inline void deque_reserve(struct deque *dq, size_t capacity)
{
	if (capacity <= dq->capacity)
		return;

	dq->data = brealloc(dq->data, capacity);
	deque_reorder_data(dq, capacity);
	dq->capacity = capacity;
}

static inline void deque_upsize(struct deque *dq, size_t size)
{
	size_t add_size = size - dq->size;
	size_t new_end_pos = dq->end_pos + add_size;

	if (size <= dq->size)
		return;

	dq->size = size;
	deque_ensure_capacity(dq);

	if (new_end_pos > dq->capacity) {
		size_t back_size = dq->capacity - dq->end_pos;
		size_t loop_size = add_size - back_size;

		if (back_size)
			memset((uint8_t *)dq->data + dq->end_pos, 0, back_size);

		memset(dq->data, 0, loop_size);
		new_end_pos -= dq->capacity;
	} else {
		memset((uint8_t *)dq->data + dq->end_pos, 0, add_size);
	}

	dq->end_pos = new_end_pos;
}

/** Overwrites data at a specific point in the buffer (relative).  */
static inline void deque_place(struct deque *dq, size_t position,
			       const void *data, size_t size)
{
	size_t end_point = position + size;
	size_t data_end_pos;

	if (end_point > dq->size)
		deque_upsize(dq, end_point);

	position += dq->start_pos;
	if (position >= dq->capacity)
		position -= dq->capacity;

	data_end_pos = position + size;
	if (data_end_pos > dq->capacity) {
		size_t back_size = data_end_pos - dq->capacity;
		size_t loop_size = size - back_size;

		memcpy((uint8_t *)dq->data + position, data, loop_size);
		memcpy(dq->data, (uint8_t *)data + loop_size, back_size);
	} else {
		memcpy((uint8_t *)dq->data + position, data, size);
	}
}

static inline void deque_push_back(struct deque *dq, const void *data,
				   size_t size)
{
	size_t new_end_pos = dq->end_pos + size;

	dq->size += size;
	deque_ensure_capacity(dq);

	if (new_end_pos > dq->capacity) {
		size_t back_size = dq->capacity - dq->end_pos;
		size_t loop_size = size - back_size;

		if (back_size)
			memcpy((uint8_t *)dq->data + dq->end_pos, data,
			       back_size);
		memcpy(dq->data, (uint8_t *)data + back_size, loop_size);

		new_end_pos -= dq->capacity;
	} else {
		memcpy((uint8_t *)dq->data + dq->end_pos, data, size);
	}

	dq->end_pos = new_end_pos;
}

static inline void deque_push_front(struct deque *dq, const void *data,
				    size_t size)
{
	dq->size += size;
	deque_ensure_capacity(dq);

	if (dq->size == size) {
		dq->start_pos = 0;
		dq->end_pos = size;
		memcpy((uint8_t *)dq->data, data, size);

	} else if (dq->start_pos < size) {
		size_t back_size = size - dq->start_pos;

		if (dq->start_pos)
			memcpy(dq->data, (uint8_t *)data + back_size,
			       dq->start_pos);

		dq->start_pos = dq->capacity - back_size;
		memcpy((uint8_t *)dq->data + dq->start_pos, data, back_size);
	} else {
		dq->start_pos -= size;
		memcpy((uint8_t *)dq->data + dq->start_pos, data, size);
	}
}

static inline void deque_push_back_zero(struct deque *dq, size_t size)
{
	size_t new_end_pos = dq->end_pos + size;

	dq->size += size;
	deque_ensure_capacity(dq);

	if (new_end_pos > dq->capacity) {
		size_t back_size = dq->capacity - dq->end_pos;
		size_t loop_size = size - back_size;

		if (back_size)
			memset((uint8_t *)dq->data + dq->end_pos, 0, back_size);
		memset(dq->data, 0, loop_size);

		new_end_pos -= dq->capacity;
	} else {
		memset((uint8_t *)dq->data + dq->end_pos, 0, size);
	}

	dq->end_pos = new_end_pos;
}

static inline void deque_push_front_zero(struct deque *dq, size_t size)
{
	dq->size += size;
	deque_ensure_capacity(dq);

	if (dq->size == size) {
		dq->start_pos = 0;
		dq->end_pos = size;
		memset((uint8_t *)dq->data, 0, size);

	} else if (dq->start_pos < size) {
		size_t back_size = size - dq->start_pos;

		if (dq->start_pos)
			memset(dq->data, 0, dq->start_pos);

		dq->start_pos = dq->capacity - back_size;
		memset((uint8_t *)dq->data + dq->start_pos, 0, back_size);
	} else {
		dq->start_pos -= size;
		memset((uint8_t *)dq->data + dq->start_pos, 0, size);
	}
}

static inline void deque_peek_front(struct deque *dq, void *data, size_t size)
{
	assert(size <= dq->size);

	if (data) {
		size_t start_size = dq->capacity - dq->start_pos;

		if (start_size < size) {
			memcpy(data, (uint8_t *)dq->data + dq->start_pos,
			       start_size);
			memcpy((uint8_t *)data + start_size, dq->data,
			       size - start_size);
		} else {
			memcpy(data, (uint8_t *)dq->data + dq->start_pos, size);
		}
	}
}

static inline void deque_peek_back(struct deque *dq, void *data, size_t size)
{
	assert(size <= dq->size);

	if (data) {
		size_t back_size = (dq->end_pos ? dq->end_pos : dq->capacity);

		if (back_size < size) {
			size_t front_size = size - back_size;
			size_t new_end_pos = dq->capacity - front_size;

			memcpy((uint8_t *)data + (size - back_size), dq->data,
			       back_size);
			memcpy(data, (uint8_t *)dq->data + new_end_pos,
			       front_size);
		} else {
			memcpy(data, (uint8_t *)dq->data + dq->end_pos - size,
			       size);
		}
	}
}

static inline void deque_pop_front(struct deque *dq, void *data, size_t size)
{
	deque_peek_front(dq, data, size);

	dq->size -= size;
	if (!dq->size) {
		dq->start_pos = dq->end_pos = 0;
		return;
	}

	dq->start_pos += size;
	if (dq->start_pos >= dq->capacity)
		dq->start_pos -= dq->capacity;
}

static inline void deque_pop_back(struct deque *dq, void *data, size_t size)
{
	deque_peek_back(dq, data, size);

	dq->size -= size;
	if (!dq->size) {
		dq->start_pos = dq->end_pos = 0;
		return;
	}

	if (dq->end_pos <= size)
		dq->end_pos = dq->capacity - (size - dq->end_pos);
	else
		dq->end_pos -= size;
}

static inline void *deque_data(struct deque *dq, size_t idx)
{
	uint8_t *ptr = (uint8_t *)dq->data;
	size_t offset = dq->start_pos + idx;

	if (idx >= dq->size)
		return NULL;

	if (offset >= dq->capacity)
		offset -= dq->capacity;

	return ptr + offset;
}

#ifdef __cplusplus
}
#endif
