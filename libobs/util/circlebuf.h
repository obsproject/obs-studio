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

#include "c99defs.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "bmem.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Dynamic circular buffer */

struct circlebuf {
	void   *data;
	size_t size;

	size_t start_pos;
	size_t end_pos;
	size_t capacity;
};

static inline void circlebuf_init(struct circlebuf *cb)
{
	memset(cb, 0, sizeof(struct circlebuf));
}

static inline void circlebuf_free(struct circlebuf *cb)
{
	bfree(cb->data);
	memset(cb, 0, sizeof(struct circlebuf));
}

static inline void circlebuf_reorder_data(struct circlebuf *cb,
		size_t new_capacity)
{
	size_t difference;
	uint8_t *data;

	if (!cb->size || !cb->start_pos || cb->end_pos > cb->start_pos)
		return;

	difference = new_capacity - cb->capacity;
	data = (uint8_t*)cb->data + cb->start_pos;
	memmove(data+difference, data, cb->capacity - cb->start_pos);
	cb->start_pos += difference;
}

static inline void circlebuf_ensure_capacity(struct circlebuf *cb)
{
	size_t new_capacity;
	if (cb->size <= cb->capacity)
		return;

	new_capacity = cb->capacity*2;
	if (cb->size > new_capacity)
		new_capacity = cb->size;

	cb->data = brealloc(cb->data, new_capacity);
	circlebuf_reorder_data(cb, new_capacity);
	cb->capacity = new_capacity;
}

static inline void circlebuf_reserve(struct circlebuf *cb, size_t capacity)
{
	if (capacity <= cb->capacity)
		return;

	cb->data = brealloc(cb->data, capacity);
	circlebuf_reorder_data(cb, capacity);
	cb->capacity = capacity;
}

static inline void circlebuf_upsize(struct circlebuf *cb, size_t size)
{
	size_t add_size = size - cb->size;
	size_t new_end_pos = cb->end_pos + add_size;

	if (size <= cb->size)
		return;

	cb->size = size;
	circlebuf_ensure_capacity(cb);

	if (new_end_pos > cb->capacity) {
		size_t back_size = cb->capacity - cb->end_pos;
		size_t loop_size = add_size - back_size;

		if (back_size)
			memset((uint8_t*)cb->data + cb->end_pos, 0, back_size);

		memset(cb->data, 0, loop_size);
		new_end_pos -= cb->capacity;
	} else {
		memset((uint8_t*)cb->data + cb->end_pos, 0, add_size);
	}

	cb->end_pos = new_end_pos;
}

/** Overwrites data at a specific point in the buffer (relative).  */
static inline void circlebuf_place(struct circlebuf *cb, size_t position,
		const void *data, size_t size)
{
	size_t end_point = position + size;
	size_t data_end_pos;

	if (end_point > cb->size)
		circlebuf_upsize(cb, end_point);

	position += cb->start_pos;
	if (position >= cb->capacity)
		position -= cb->capacity;

	data_end_pos = position + size;
	if (data_end_pos > cb->capacity) {
		size_t back_size = cb->capacity - data_end_pos;
		size_t loop_size = size - back_size;

		if (back_size)
			memcpy((uint8_t*)cb->data + position, data, back_size);
		memcpy(cb->data, (uint8_t*)data + back_size, loop_size);
	} else {
		memcpy((uint8_t*)cb->data + position, data, size);
	}
}

static inline void circlebuf_push_back(struct circlebuf *cb, const void *data,
		size_t size)
{
	size_t new_end_pos = cb->end_pos + size;

	cb->size += size;
	circlebuf_ensure_capacity(cb);

	if (new_end_pos > cb->capacity) {
		size_t back_size = cb->capacity - cb->end_pos;
		size_t loop_size = size - back_size;

		if (back_size)
			memcpy((uint8_t*)cb->data + cb->end_pos, data,
					back_size);
		memcpy(cb->data, (uint8_t*)data + back_size, loop_size);

		new_end_pos -= cb->capacity;
	} else {
		memcpy((uint8_t*)cb->data + cb->end_pos, data, size);
	}

	cb->end_pos = new_end_pos;
}

static inline void circlebuf_pop_front(struct circlebuf *cb, void *data,
		size_t size)
{
	size_t start_size;
	assert(size <= cb->size);

	start_size = cb->capacity - cb->start_pos;

	if (start_size < size) {
		memcpy(data, (uint8_t*)cb->data + cb->start_pos, start_size);
		memcpy((uint8_t*)data + start_size, cb->data,
				size - start_size);
	} else {
		memcpy(data, (uint8_t*)cb->data + cb->start_pos, size);
	}

	cb->size -= size;
	cb->start_pos += size;
	if (cb->start_pos >= cb->capacity)
		cb->start_pos -= cb->capacity;
}

#ifdef __cplusplus
}
#endif
