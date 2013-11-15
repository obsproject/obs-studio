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

static bool create_buffers(struct gs_vertex_buffer *vb)
{
	GLenum usage = vb->dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
	size_t i;

	if (!gl_create_buffer(GL_ARRAY_BUFFER, &vb->vertex_buffer,
				vb->data->num * sizeof(struct vec3),
				vb->data->points, usage))
		return false;

	if (vb->data->normals) {
		if (!gl_create_buffer(GL_ARRAY_BUFFER, &vb->normal_buffer,
					vb->data->num * sizeof(struct vec3),
					vb->data->normals, usage))
			return false;
	}

	if (vb->data->tangents) {
		if (!gl_create_buffer(GL_ARRAY_BUFFER, &vb->tangent_buffer,
					vb->data->num * sizeof(struct vec3),
					vb->data->tangents, usage))
			return false;
	}

	if (vb->data->colors) {
		if (!gl_create_buffer(GL_ARRAY_BUFFER, &vb->color_buffer,
					vb->data->num * sizeof(uint32_t),
					vb->data->colors, usage))
			return false;
	}

	da_reserve(vb->uv_buffers, vb->data->num_tex);
	da_reserve(vb->uv_sizes,   vb->data->num_tex);

	for (i = 0; i < vb->data->num_tex; i++) {
		GLuint tex_buffer;
		struct tvertarray *tv = vb->data->tvarray+i;
		size_t size = vb->data->num * sizeof(float) * tv->width;

		if (!gl_create_buffer(GL_ARRAY_BUFFER, &tex_buffer, size,
					tv->array, usage))
			return false;

		da_push_back(vb->uv_buffers, &tex_buffer);
		da_push_back(vb->uv_sizes,   &tv->width);
	}

	if (!vb->dynamic) {
		vbdata_destroy(vb->data);
		vb->data = NULL;
	}

	if (!gl_gen_vertex_arrays(1, &vb->vao))
		return false;

	return true;
}

vertbuffer_t device_create_vertexbuffer(device_t device,
		struct vb_data *data, uint32_t flags)
{
	struct gs_vertex_buffer *vb = bmalloc(sizeof(struct gs_vertex_buffer));
	memset(vb, 0, sizeof(struct gs_vertex_buffer));

	vb->device  = device;
	vb->data    = data;
	vb->num     = data->num;
	vb->dynamic = flags & GS_DYNAMIC;

	if (!create_buffers(vb)) {
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

		if (vb->vao)
			gl_delete_vertex_arrays(1, &vb->vao);

		da_free(vb->uv_sizes);
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

	if (!update_buffer(GL_ARRAY_BUFFER, vb->vertex_buffer,
				vb->data->points,
				vb->data->num * sizeof(struct vec3)))
		goto failed;

	if (vb->normal_buffer) {
		if (!update_buffer(GL_ARRAY_BUFFER, vb->normal_buffer,
					vb->data->normals,
					vb->data->num * sizeof(struct vec3)))
			goto failed;
	}

	if (vb->tangent_buffer) {
		if (!update_buffer(GL_ARRAY_BUFFER, vb->tangent_buffer,
					vb->data->tangents,
					vb->data->num * sizeof(struct vec3)))
			goto failed;
	}

	if (vb->color_buffer) {
		if (!update_buffer(GL_ARRAY_BUFFER, vb->color_buffer,
					vb->data->colors,
					vb->data->num * sizeof(uint32_t)))
			goto failed;
	}

	for (i = 0; i < vb->data->num_tex; i++) {
		GLuint buffer = vb->uv_buffers.array[i];
		struct tvertarray *tv = vb->data->tvarray+i;
		size_t size = vb->data->num * tv->width * sizeof(float);

		if (!update_buffer(GL_ARRAY_BUFFER, buffer, tv->array, size))
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

static inline GLuint get_vb_buffer(struct gs_vertex_buffer *vb,
		enum attrib_type type, size_t index, GLint *width,
		GLenum *gl_type)
{
	*gl_type = GL_FLOAT;
	*width   = 4;

	if (type == ATTRIB_POSITION) {
		return vb->vertex_buffer;
	} else if (type == ATTRIB_NORMAL) {
		return vb->normal_buffer;
	} else if (type == ATTRIB_TANGENT) {
		return vb->tangent_buffer;
	} else if (type == ATTRIB_COLOR) {
		*gl_type = GL_UNSIGNED_BYTE;
		return vb->color_buffer;
	} else if (type == ATTRIB_TEXCOORD) {
		if (vb->uv_buffers.num <= index)
			return 0;

		*width = (GLint)vb->uv_sizes.array[index];
		return vb->uv_buffers.array[index];
	}

	return 0;
}

static bool load_vb_buffer(struct gs_shader *shader,
		struct shader_attrib *attrib,
		struct gs_vertex_buffer *vb)
{
	GLenum type;
	GLint width;
	GLuint buffer;
	bool success = true;

	buffer = get_vb_buffer(vb, attrib->type, attrib->index, &width, &type);
	if (!buffer) {
		blog(LOG_ERROR, "Vertex buffer does not have the required "
		                "inputs for vertex shader");
		return false;
	}

	if (!gl_bind_buffer(GL_ARRAY_BUFFER, buffer))
		return false;

	glVertexAttribPointer(attrib->attrib, width, type, GL_FALSE, 0, 0);
	if (!gl_success("glVertexAttribPointer"))
		success = false;

	glEnableVertexAttribArray(attrib->attrib);
	if (!gl_success("glEnableVertexAttribArray"))
		success = false;

	if (!gl_bind_buffer(GL_ARRAY_BUFFER, 0))
		success = false;

	return success;
}

static inline bool load_vb_buffers(struct gs_shader *shader,
		struct gs_vertex_buffer *vb)
{
	size_t i;

	if (!gl_bind_vertex_array(vb->vao))
		return false;

	for (i = 0; i < shader->attribs.num; i++) {
		struct shader_attrib *attrib = shader->attribs.array+i;
		if (!load_vb_buffer(shader, attrib, vb))
			return false;
	}

	return true;
}

bool vertexbuffer_load(device_t device, vertbuffer_t vb)
{
	if (device->cur_vertex_buffer == vb)
		return true;

	device->cur_vertex_buffer = vb;
	if (!device->cur_vertex_shader || !vb) {
		gl_bind_vertex_array(0);
		return true;
	}

	if (!load_vb_buffers(device->cur_vertex_shader, vb))
		return false;

	return true;
}

void device_load_vertexbuffer(device_t device, vertbuffer_t vb)
{
	if (!vertexbuffer_load(device, vb))
		blog(LOG_ERROR, "device_load_vertexbuffer (GL) failed");
}
