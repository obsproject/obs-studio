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

#include "graphics/vec3.h"
#include "gl-subsystem.h"

static bool update_buffer(GLuint buffer, void *data, size_t size)
{
	void *ptr;
	bool success = true;

	if (!gl_bind_buffer(GL_ARRAY_BUFFER, buffer))
		return false;

	ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	success = gl_success("glMapBuffer");
	if (success && ptr) {
		memcpy(ptr, data, size);
		glUnmapBuffer(GL_ARRAY_BUFFER);
	}

	gl_bind_buffer(GL_ARRAY_BUFFER, 0);
	return success;
}

static bool init_vb(struct gs_vertex_buffer *vb)
{
	GLenum usage = vb->dynamic ? GL_DYNAMIC_COPY : GL_STATIC_DRAW;
	size_t i;

	if (!gl_create_buffer(&vb->vertex_buffer,
				vb->data->num * sizeof(struct vec3),
				vb->data->points, usage))
		return false;

	if (vb->data->normals) {
		if (!gl_create_buffer(&vb->normal_buffer,
					vb->data->num * sizeof(struct vec3),
					vb->data->normals, usage))
			return false;
	}

	if (vb->data->tangents) {
		if (!gl_create_buffer(&vb->tangent_buffer,
					vb->data->num * sizeof(struct vec3),
					vb->data->tangents, usage))
			return false;
	}

	if (vb->data->colors) {
		if (!gl_create_buffer(&vb->color_buffer,
					vb->data->num * sizeof(uint32_t),
					vb->data->colors, usage))
			return false;
	}

	da_reserve(vb->uv_buffers, vb->data->num_tex);

	for (i = 0; i < vb->data->num_tex; i++) {
		GLuint tex_buffer;
		struct tvertarray *tv = vb->data->tvarray+i;
		size_t size = vb->data->num * sizeof(float) * tv->width;

		if (!gl_create_buffer(&tex_buffer, size, tv->array, usage))
			return false;

		da_push_back(vb->uv_buffers, &tex_buffer);
	}

	if (!vb->dynamic) {
		vbdata_destroy(vb->data);
		vb->data = NULL;
	}

	return true;
}

vertbuffer_t device_create_vertexbuffer(device_t device,
		struct vb_data *data, uint32_t flags)
{
	struct gs_vertex_buffer *vb = bmalloc(sizeof(struct gs_vertex_buffer));
	memset(vb, 0, sizeof(struct gs_vertex_buffer));

	vb->data      = data;
	vb->dynamic   = flags & GS_DYNAMIC;

	if (!init_vb(vb)) {
		blog(LOG_ERROR, "device_create_vertexbuffer (GL) failed");
		vertexbuffer_destroy(vb);
		return NULL;
	}

	return vb;
}

void vertexbuffer_destroy(vertbuffer_t vb)
{
	if (vb) {
		if (vb->vertex_buffer)
			gl_delete_buffers(1, &vb->vertex_buffer);

		if (vb->normal_buffer)
			gl_delete_buffers(1, &vb->normal_buffer);

		if (vb->tangent_buffer)
			gl_delete_buffers(1, &vb->tangent_buffer);

		if (vb->color_buffer)
			gl_delete_buffers(1, &vb->color_buffer);

		if (vb->uv_buffers.num)
			gl_delete_buffers((GLsizei)vb->uv_buffers.num,
					vb->uv_buffers.array);

		da_free(vb->uv_buffers);
		vbdata_destroy(vb->data);

		bfree(vb);
	}
}

void vertexbuffer_flush(vertbuffer_t vb, bool rebuild)
{
	size_t i;

	if (!vb->dynamic) {
		blog(LOG_ERROR, "vertex buffer is not dynamic");
		goto failed;
	}

	if (!update_buffer(vb->vertex_buffer, vb->data->points,
				vb->data->num * sizeof(struct vec3)))
		goto failed;

	if (vb->normal_buffer) {
		if (!update_buffer(vb->normal_buffer, vb->data->normals,
					vb->data->num * sizeof(struct vec3)))
			goto failed;
	}

	if (vb->tangent_buffer) {
		if (!update_buffer(vb->tangent_buffer, vb->data->tangents,
					vb->data->num * sizeof(struct vec3)))
			goto failed;
	}

	if (vb->color_buffer) {
		if (!update_buffer(vb->color_buffer, vb->data->colors,
					vb->data->num * sizeof(uint32_t)))
			goto failed;
	}

	for (i = 0; i < vb->data->num_tex; i++) {
		GLuint buffer = vb->uv_buffers.array[i];
		struct tvertarray *tv = vb->data->tvarray+i;
		size_t size = vb->data->num * tv->width * sizeof(float);

		if (!update_buffer(buffer, tv->array, size))
			goto failed;
	}

	return;

failed:
	blog(LOG_ERROR, "vertexbuffer_flush (GL) failed");
}

struct vb_data *vertexbuffer_getdata(vertbuffer_t vb)
{
	return vb->data;
}
