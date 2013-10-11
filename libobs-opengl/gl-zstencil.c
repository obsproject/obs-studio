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

static bool gl_init_zsbuffer(struct gs_zstencil_buffer *zs, uint32_t width,
		uint32_t height)
{
	glGenRenderbuffers(1, &zs->buffer);
	if (!gl_success("glGenRenderbuffers"))
		return false;

	if (!gl_bind_renderbuffer(GL_RENDERBUFFER, zs->buffer))
		return false;

	glRenderbufferStorage(GL_RENDERBUFFER, zs->format, width, height);
	if (!gl_success("glRenderbufferStorage"))
		return false;

	gl_bind_renderbuffer(GL_RENDERBUFFER, 0);
	return true;
}

zstencil_t device_create_zstencil(device_t device, uint32_t width,
		uint32_t height, enum gs_zstencil_format format)
{
	struct gs_zstencil_buffer *zs;

	zs = bmalloc(sizeof(struct gs_zstencil_buffer));
	memset(zs, 0, sizeof(struct gs_zstencil_buffer));
	zs->format = convert_zstencil_format(format);

	if (!gl_init_zsbuffer(zs, width, height)) {
		blog(LOG_ERROR, "device_create_zstencil (GL) failed");
		zstencil_destroy(zs);
		return NULL;
	}

	return zs;
}

void zstencil_destroy(zstencil_t zs)
{
	if (zs) {
		if (zs->buffer) {
			glDeleteRenderbuffers(1, &zs->buffer);
			gl_success("glDeleteRenderbuffers");
		}

		bfree(zs);
	}
}
