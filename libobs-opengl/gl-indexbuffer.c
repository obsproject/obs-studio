/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

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

#include "gl-subsystem.h"

static inline bool init_ib(struct gs_index_buffer *ib)
{
	GLenum usage = ib->dynamic ? GL_DYNAMIC_COPY : GL_STATIC_DRAW;
	bool success;

	success = gl_create_buffer(GL_ARRAY_BUFFER, &ib->buffer, ib->size,
			ib->data, usage);

	if (!ib->dynamic) {
		bfree(ib->data);
		ib->data = NULL;
	}

	return success;
}

indexbuffer_t device_create_indexbuffer(device_t device,
		enum gs_index_type type, void *indices, size_t num,
		uint32_t flags)
{
	struct gs_index_buffer *ib = bmalloc(sizeof(struct gs_index_buffer));
	size_t width = type == GS_UNSIGNED_LONG ? sizeof(long) : sizeof(short);
	memset(ib, 0, sizeof(struct gs_index_buffer));

	ib->device  = device;
	ib->data    = indices;
	ib->dynamic = flags & GS_DYNAMIC;
	ib->num     = num;
	ib->size    = width * num;
	ib->type    = type;
	ib->gl_type = type == GS_UNSIGNED_LONG ? GL_UNSIGNED_INT :
	                                         GL_UNSIGNED_SHORT;

	if (!init_ib(ib)) {
		blog(LOG_ERROR, "device_create_indexbuffer (GL) failed");
		indexbuffer_destroy(ib);
		return NULL;
	}

	return ib;
}

void indexbuffer_destroy(indexbuffer_t ib)
{
	if (ib) {
		if (ib->buffer)
			gl_delete_buffers(1, &ib->buffer);

		bfree(ib->data);
		bfree(ib);
	}
}

void indexbuffer_flush(indexbuffer_t ib)
{
	if (!ib->dynamic) {
		blog(LOG_ERROR, "Index buffer is not dynamic");
		goto fail;
	}

	if (!update_buffer(GL_ARRAY_BUFFER, ib->buffer, ib->data, ib->size))
		goto fail;

	return;

fail:
	blog(LOG_ERROR, "indexbuffer_flush (GL) failed");
}

void *indexbuffer_getdata(indexbuffer_t ib)
{
	return ib->data;
}

size_t indexbuffer_numindices(indexbuffer_t ib)
{
	return ib->num;
}

enum gs_index_type indexbuffer_gettype(indexbuffer_t ib)
{
	return ib->type;
}
