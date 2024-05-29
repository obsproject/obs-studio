/*
 * Copyright (c) 2024 Dennis Sädtler <dennis@obsproject.com>
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

#include "buffered-file-serializer.h"

#include <inttypes.h>

#include "platform.h"
#include "threading.h"
#include "deque.h"
#include "dstr.h"

static const size_t DEFAULT_BUF_SIZE = 256ULL * 1048576ULL; // 256 MiB
static const size_t DEFAULT_CHUNK_SIZE = 1048576;           // 1 MiB

/* ========================================================================== */
/* Buffered writer based on ffmpeg-mux implementation                         */

struct io_header {
	uint64_t seek_offset;
	uint64_t data_length;
};

struct io_buffer {
	bool active;
	bool shutdown_requested;
	bool output_error;
	os_event_t *buffer_space_available_event;
	os_event_t *new_data_available_event;
	pthread_t io_thread;
	pthread_mutex_t data_mutex;
	FILE *output_file;
	struct deque data;
	uint64_t next_pos;

	size_t buffer_size;
	size_t chunk_size;
};

struct file_output_data {
	struct dstr filename;
	struct io_buffer io;
};

static void *io_thread(void *opaque)
{
	struct file_output_data *out = opaque;
	os_set_thread_name("buffered writer i/o thread");

	// Chunk collects the writes into a larger batch
	size_t chunk_used = 0;
	size_t chunk_size = out->io.chunk_size;

	unsigned char *chunk = bmalloc(chunk_size);
	if (!chunk) {
		os_atomic_set_bool(&out->io.output_error, true);
		fprintf(stderr, "Error allocating memory for output\n");
		goto error;
	}

	bool shutting_down;
	bool want_seek = false;
	bool force_flush_chunk = false;

	// current_seek_position is a virtual position updated as we read from
	// the buffer, if it becomes discontinuous due to a seek request we
	// flush the chunk. next_seek_position is the actual offset we should
	// seek to when we write the chunk.
	uint64_t current_seek_position = 0;
	uint64_t next_seek_position;

	for (;;) {
		// Wait for data to be written to the buffer
		os_event_wait(out->io.new_data_available_event);

		// Loop to write in chunk_size chunks
		for (;;) {
			pthread_mutex_lock(&out->io.data_mutex);

			shutting_down = os_atomic_load_bool(
				&out->io.shutdown_requested);

			// Fetch as many writes as possible from the deque
			// and fill up our local chunk. This may involve
			// seeking, so take care of that as well.
			for (;;) {
				size_t available = out->io.data.size;

				// Buffer is empty (now) or was already empty (we got
				// woken up to exit)
				if (!available)
					break;

				// Get seek offset and data size
				struct io_header header;
				deque_peek_front(&out->io.data, &header,
						 sizeof(header));

				// Do we need to seek?
				if (header.seek_offset !=
				    current_seek_position) {

					// If there's already part of a chunk pending,
					// flush it at the current offset. Similarly,
					// if we already plan to seek, then seek.
					if (chunk_used || want_seek) {
						force_flush_chunk = true;
						break;
					}

					// Mark that we need to seek and where to
					want_seek = true;
					next_seek_position = header.seek_offset;

					// Update our virtual position
					current_seek_position =
						header.seek_offset;
				}

				// Make sure there's enough room for the data, if
				// not then force a flush
				if (header.data_length + chunk_used >
				    chunk_size) {
					force_flush_chunk = true;
					break;
				}

				// Remove header that we already read
				deque_pop_front(&out->io.data, NULL,
						sizeof(header));

				// Copy from the buffer to our local chunk
				deque_pop_front(&out->io.data,
						chunk + chunk_used,
						header.data_length);

				// Update offsets
				chunk_used += header.data_length;
				current_seek_position += header.data_length;
			}

			// Signal that there is more room in the buffer
			os_event_signal(out->io.buffer_space_available_event);

			// Try to avoid lots of small writes unless this was the final
			// data left in the buffer. The buffer might be entirely empty
			// if we were woken up to exit.
			if (!force_flush_chunk &&
			    (!chunk_used ||
			     (chunk_used < 65536 && !shutting_down))) {
				os_event_reset(
					out->io.new_data_available_event);
				pthread_mutex_unlock(&out->io.data_mutex);
				break;
			}

			pthread_mutex_unlock(&out->io.data_mutex);

			// Seek if we need to
			if (want_seek) {
				os_fseeki64(out->io.output_file,
					    next_seek_position, SEEK_SET);

				// Update the next virtual position, making sure to take
				// into account the size of the chunk we're about to write.
				current_seek_position =
					next_seek_position + chunk_used;

				want_seek = false;

				// If we did a seek but do not have any data left to write
				// return to the start of the loop.
				if (!chunk_used) {
					force_flush_chunk = false;
					continue;
				}
			}

			// Write the current chunk to the output file
			size_t bytes_written = fwrite(chunk, 1, chunk_used,
						      out->io.output_file);
			if (bytes_written != chunk_used) {
				blog(LOG_ERROR,
				     "Error writing to '%s': %s (%zu != %zu)\n",
				     out->filename.array, strerror(errno),
				     bytes_written, chunk_used);
				os_atomic_set_bool(&out->io.output_error, true);

				goto error;
			}

			chunk_used = 0;
			force_flush_chunk = false;
		}

		// If this was the last chunk, time to exit
		if (shutting_down)
			break;
	}

error:
	if (chunk)
		bfree(chunk);

	fclose(out->io.output_file);
	return NULL;
}

/* ========================================================================== */
/* Serializer Implementation                                                  */

static int64_t file_output_seek(void *opaque, int64_t offset,
				enum serialize_seek_type seek_type)
{
	struct file_output_data *out = opaque;

	// If the output thread failed, signal that back up the stack
	if (os_atomic_load_bool(&out->io.output_error))
		return -1;

	// Update where the next write should go
	pthread_mutex_lock(&out->io.data_mutex);

	switch (seek_type) {
	case SERIALIZE_SEEK_START:
		out->io.next_pos = offset;
		break;
	case SERIALIZE_SEEK_CURRENT:
		out->io.next_pos += offset;
		break;
	case SERIALIZE_SEEK_END:
		out->io.next_pos -= offset;
		break;
	}

	pthread_mutex_unlock(&out->io.data_mutex);

	return (int64_t)out->io.next_pos;
}

#ifndef _WIN32
static inline size_t max(size_t a, size_t b)
{
	return a > b ? a : b;
}

static inline size_t min(size_t a, size_t b)
{
	return a < b ? a : b;
}
#endif

static size_t file_output_write(void *opaque, const void *buf, size_t buf_size)
{
	struct file_output_data *out = opaque;

	if (!buf_size)
		return 0;

	// Split writes into at chunks that are at most chunk_size bytes
	uintptr_t ptr = (uintptr_t)buf;
	size_t remaining = buf_size;

	while (remaining) {
		if (os_atomic_load_bool(&out->io.output_error))
			return 0;

		pthread_mutex_lock(&out->io.data_mutex);

		size_t next_chunk_size = min(remaining, out->io.chunk_size);

		// Avoid unbounded growth of the deque, cap to buffer_size
		size_t cap = max(out->io.data.capacity, out->io.buffer_size);
		size_t free_space = cap - out->io.data.size;

		if (free_space < next_chunk_size + sizeof(struct io_header)) {
			blog(LOG_DEBUG, "Waiting for I/O thread...");
			// No space, wait for the I/O thread to make space
			os_event_reset(out->io.buffer_space_available_event);
			pthread_mutex_unlock(&out->io.data_mutex);
			os_event_wait(out->io.buffer_space_available_event);
			continue;
		}

		// Calculate how many chunks we can fit into the buffer
		size_t num_chunks = free_space / (next_chunk_size +
						  sizeof(struct io_header));

		while (remaining && num_chunks--) {
			struct io_header header = {
				.data_length = next_chunk_size,
				.seek_offset = out->io.next_pos,
			};

			// Copy the data into the buffer
			deque_push_back(&out->io.data, &header, sizeof(header));
			deque_push_back(&out->io.data, (const void *)ptr,
					next_chunk_size);

			// Advance the next write position
			out->io.next_pos += next_chunk_size;

			// Update remainder and advance data pointer
			remaining -= next_chunk_size;
			ptr += next_chunk_size;
			next_chunk_size = min(remaining, out->io.chunk_size);
		}

		// Tell the I/O thread that there's new data to be written
		os_event_signal(out->io.new_data_available_event);

		pthread_mutex_unlock(&out->io.data_mutex);
	}

	return buf_size - remaining;
}

static int64_t file_output_get_pos(void *opaque)
{
	struct file_output_data *out = opaque;
	// If thread failed return -1
	if (os_atomic_load_bool(&out->io.output_error))
		return -1;

	return (int64_t)out->io.next_pos;
}

bool buffered_file_serializer_init_defaults(struct serializer *s,
					    const char *path)
{
	return buffered_file_serializer_init(s, path, 0, 0);
}

bool buffered_file_serializer_init(struct serializer *s, const char *path,
				   size_t max_bufsize, size_t chunk_size)
{
	struct file_output_data *out;

	out = bzalloc(sizeof(*out));

	dstr_init_copy(&out->filename, path);

	out->io.output_file = os_fopen(path, "wb");
	if (!out->io.output_file)
		return false;

	out->io.buffer_size = max_bufsize ? max_bufsize : DEFAULT_BUF_SIZE;
	out->io.chunk_size = chunk_size ? chunk_size : DEFAULT_CHUNK_SIZE;

	// Start at 1MB, this can grow up to max_bufsize depending
	// on how fast data is going in and out.
	deque_reserve(&out->io.data, 1048576);

	pthread_mutex_init(&out->io.data_mutex, NULL);

	os_event_init(&out->io.buffer_space_available_event,
		      OS_EVENT_TYPE_AUTO);
	os_event_init(&out->io.new_data_available_event, OS_EVENT_TYPE_AUTO);

	pthread_create(&out->io.io_thread, NULL, io_thread, out);

	out->io.active = true;

	s->data = out;
	s->read = NULL;
	s->write = file_output_write;
	s->seek = file_output_seek;
	s->get_pos = file_output_get_pos;
	return true;
}

void buffered_file_serializer_free(struct serializer *s)
{
	struct file_output_data *out = s->data;

	if (!out)
		return;

	if (out->io.active) {
		os_atomic_set_bool(&out->io.shutdown_requested, true);

		// Wakes up the I/O thread and waits for it to finish
		pthread_mutex_lock(&out->io.data_mutex);
		os_event_signal(out->io.new_data_available_event);
		pthread_mutex_unlock(&out->io.data_mutex);
		pthread_join(out->io.io_thread, NULL);

		os_event_destroy(out->io.new_data_available_event);
		os_event_destroy(out->io.buffer_space_available_event);

		pthread_mutex_destroy(&out->io.data_mutex);

		blog(LOG_DEBUG, "Final buffer capacity: %zu KiB",
		     out->io.data.capacity / 1024);

		deque_free(&out->io.data);
	}

	dstr_free(&out->filename);
	bfree(out);
}
