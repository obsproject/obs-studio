/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#pragma once

/*
 * Okay, so GL error handling is..  unclean to work with.  I don't want
 * to have to keep typing out the same stuff over and over again do I'll just
 * make a bunch of helper functions to make it a bit easier to handle errors
 */

static inline bool gl_success(const char *funcname)
{
	GLenum errorcode = glGetError();
	if (errorcode != GL_NO_ERROR) {
		blog(LOG_ERROR, "%s failed, glGetError returned 0x%X",
				funcname, errorcode);
		return false;
	}

	return true;
}

static inline bool gl_gen_textures(GLsizei num_texture, GLuint *textures)
{
	glGenTextures(num_texture, textures);
	return gl_success("glGenTextures");
}

static inline bool gl_bind_texture(GLenum target, GLuint texture)
{
	glBindTexture(target, texture);
	return gl_success("glBindTexture");
}

static inline void gl_delete_textures(GLsizei num_buffers, GLuint *buffers)
{
	glDeleteTextures(num_buffers, buffers);
	gl_success("glDeleteTextures");
}

static inline bool gl_gen_buffers(GLsizei num_buffers, GLuint *buffers)
{
	glGenBuffers(num_buffers, buffers);
	return gl_success("glGenBuffers");
}

static inline bool gl_bind_buffer(GLenum target, GLuint buffer)
{
	glBindBuffer(target, buffer);
	return gl_success("glBindBuffer");
}

static inline void gl_delete_buffers(GLsizei num_buffers, GLuint *buffers)
{
	glDeleteBuffers(num_buffers, buffers);
	gl_success("glDeleteBuffers");
}

static inline bool gl_gen_vertex_arrays(GLsizei num_arrays, GLuint *arrays)
{
	glGenVertexArrays(num_arrays, arrays);
	return gl_success("glGenVertexArrays");
}

static inline bool gl_bind_vertex_array(GLuint array)
{
	glBindVertexArray(array);
	return gl_success("glBindVertexArray");
}

static inline void gl_delete_vertex_arrays(GLsizei num_arrays, GLuint *arrays)
{
	glDeleteVertexArrays(num_arrays, arrays);
	gl_success("glDeleteVertexArrays");
}

static inline bool gl_bind_renderbuffer(GLenum target, GLuint buffer)
{
	glBindRenderbuffer(target, buffer);
	return gl_success("glBindRendebuffer");
}

static inline bool gl_bind_framebuffer(GLenum target, GLuint buffer)
{
	glBindFramebuffer(target, buffer);
	return gl_success("glBindFramebuffer");
}

static inline bool gl_tex_param_f(GLenum target, GLenum param, GLfloat val)
{
	glTexParameterf(target, param, val);
	return gl_success("glTexParameterf");
}

static inline bool gl_tex_param_i(GLenum target, GLenum param, GLint val)
{
	glTexParameteri(target, param, val);
	return gl_success("glTexParameteri");
}

static inline bool gl_active_texture(GLenum texture_id)
{
	glActiveTexture(texture_id);
	return gl_success("glActiveTexture");
}

static inline bool gl_enable(GLenum capability)
{
	glEnable(capability);
	return gl_success("glEnable");
}

static inline bool gl_disable(GLenum capability)
{
	glDisable(capability);
	return gl_success("glDisable");
}

static inline bool gl_cull_face(GLenum faces)
{
	glCullFace(faces);
	return gl_success("glCullFace");
}

static inline bool gl_get_integer_v(GLenum pname, GLint *params)
{
	glGetIntegerv(pname, params);
	return gl_success("glGetIntegerv");
}

extern bool gl_init_face(GLenum target, GLenum type, uint32_t num_levels,
		GLenum format, GLint internal_format, bool compressed,
		uint32_t width, uint32_t height, uint32_t size,
		const uint8_t ***p_data);

extern bool gl_copy_texture(struct gs_device *device,
		GLuint dst, GLenum dst_target, uint32_t dst_x, uint32_t dst_y,
		GLuint src, GLenum src_target, uint32_t src_x, uint32_t src_y,
		uint32_t width, uint32_t height,
		enum gs_color_format format);

extern bool gl_create_buffer(GLenum target, GLuint *buffer, GLsizeiptr size,
		const GLvoid *data, GLenum usage);

extern bool update_buffer(GLenum target, GLuint buffer, const void *data,
		size_t size);
