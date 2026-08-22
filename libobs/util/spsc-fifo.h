/******************************************************************************
    Copyright (C) 2026 pkv <pkv@obsproject.com>

    Tiny spsc fifo circular buffer for audio floats.
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#pragma once

#include "threading.h"

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

struct spsc_fifo {
	float *data;
	long capacity;
	volatile long read_position;
	volatile long write_position;
};

static inline long spsc_fifo_get_readable_frames(const struct spsc_fifo *fifo, long read_position, long write_position)
{
	if (write_position >= read_position) {
		return write_position - read_position;
	}

	return fifo->capacity - (read_position - write_position);
}

static inline long spsc_fifo_readable_frames(const struct spsc_fifo *fifo)
{
	if (!fifo || !fifo->data || fifo->capacity < 2) {
		return 0;
	}

	const long read_position = os_atomic_load_long(&fifo->read_position);
	const long write_position = os_atomic_load_long(&fifo->write_position);

	return spsc_fifo_get_readable_frames(fifo, read_position, write_position);
}

static inline long spsc_fifo_writable_frames(const struct spsc_fifo *fifo)
{
	if (!fifo || !fifo->data || fifo->capacity < 2) {
		return 0;
	}

	const long read_position = os_atomic_load_long(&fifo->read_position);
	const long write_position = os_atomic_load_long(&fifo->write_position);

	const long readable = spsc_fifo_get_readable_frames(fifo, read_position, write_position);

	return fifo->capacity - readable - 1;
}

static inline bool spsc_fifo_discard(struct spsc_fifo *fifo, long frames)
{
	if (!fifo || !fifo->data || frames <= 0 || frames >= fifo->capacity) {
		return false;
	}

	const long read_position = os_atomic_load_long(&fifo->read_position);
	const long write_position = os_atomic_load_long(&fifo->write_position);

	const long readable = spsc_fifo_get_readable_frames(fifo, read_position, write_position);

	if (frames > readable) {
		return false;
	}

	long new_read_position = read_position + frames;
	if (new_read_position >= fifo->capacity) {
		new_read_position -= fifo->capacity;
	}

	os_atomic_store_long(&fifo->read_position, new_read_position);

	return true;
}

static inline bool spsc_fifo_init(struct spsc_fifo *fifo, long capacity)
{
	if (!fifo || fifo->data || capacity < 2 || capacity > LONG_MAX / 2) {
		return false;
	}

	if ((size_t)capacity > SIZE_MAX / sizeof(float)) {
		return false;
	}

	float *data = calloc((size_t)capacity, sizeof(*data));
	if (!data) {
		return false;
	}

	fifo->data = data;
	fifo->capacity = capacity;
	os_atomic_store_long(&fifo->read_position, 0);
	os_atomic_store_long(&fifo->write_position, 0);

	return true;
}

static inline void spsc_fifo_clear(struct spsc_fifo *fifo)
{
	if (!fifo || !fifo->data) {
		return;
	}

	const long write_position = os_atomic_load_long(&fifo->write_position);

	os_atomic_store_long(&fifo->read_position, write_position);
}

static inline void spsc_fifo_free(struct spsc_fifo *fifo)
{
	if (!fifo) {
		return;
	}

	free(fifo->data);
	fifo->data = NULL;
	fifo->capacity = 0;
	os_atomic_store_long(&fifo->read_position, 0);
	os_atomic_store_long(&fifo->write_position, 0);
}

/* That writer drops the data if the producer tries to write more frames than available. If you prefer a no-drop
 * overflow policy, call first spsc_fifo_writable_frames().
 */
static inline void spsc_fifo_write(struct spsc_fifo *fifo, const float *src, long frames)
{
	if (!fifo || !fifo->data || !src || frames <= 0 || frames >= fifo->capacity) {
		return;
	}

	const long write_position = os_atomic_load_long(&fifo->write_position);
	const long read_position = os_atomic_load_long(&fifo->read_position);
	const long readable_frames = spsc_fifo_get_readable_frames(fifo, read_position, write_position);
	const long writable_frames = fifo->capacity - readable_frames - 1;

	if (frames > writable_frames) {
		return;
	}

	const long first_block = frames < fifo->capacity - write_position ? frames : fifo->capacity - write_position;
	const long second_block = frames - first_block;

	memcpy(fifo->data + write_position, src, (size_t)first_block * sizeof(float));

	if (second_block > 0) {
		memcpy(fifo->data, src + first_block, (size_t)second_block * sizeof(float));
	}

	long new_write_pos = write_position + frames;
	if (new_write_pos >= fifo->capacity) {
		new_write_pos -= fifo->capacity;
	}

	os_atomic_store_long(&fifo->write_position, new_write_pos);

	return;
}

static inline long spsc_fifo_read(struct spsc_fifo *fifo, float *dst, long frames)
{
	if (!fifo || !fifo->data || !dst || frames <= 0 || frames >= fifo->capacity) {
		return 0;
	}

	const long read_position = os_atomic_load_long(&fifo->read_position);
	const long write_position = os_atomic_load_long(&fifo->write_position);
	const long readable_frames = spsc_fifo_get_readable_frames(fifo, read_position, write_position);
	const long frames_to_read = readable_frames < frames ? readable_frames : frames;

	const long first_block = frames_to_read < fifo->capacity - read_position ? frames_to_read
										 : fifo->capacity - read_position;
	const long second_block = frames_to_read - first_block;

	memcpy(dst, fifo->data + read_position, (size_t)first_block * sizeof(*dst));

	if (second_block > 0) {
		memcpy(dst + first_block, fifo->data, (size_t)second_block * sizeof(*dst));
	}

	long new_read_position = read_position + frames_to_read;
	if (new_read_position >= fifo->capacity) {
		new_read_position -= fifo->capacity;
	}

	os_atomic_store_long(&fifo->read_position, new_read_position);

	if (frames_to_read < frames) {
		memset(dst + frames_to_read, 0, (size_t)(frames - frames_to_read) * sizeof(*dst));
	}

	return frames_to_read;
}

#ifdef __cplusplus
}
#endif
