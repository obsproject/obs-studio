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

#ifndef GL_SUBSYSTEM_H
#define GL_SUBSYSTEM_H

#include "graphics/graphics.h"
#include "glew/include/GL/glew.h"
#include "gl-helpers.h"

struct gl_platform;
struct gl_windowinfo;

static inline GLint convert_gs_format(enum gs_color_format format)
{
	switch (format) {
	case GS_A8:          return GL_RGBA;
	case GS_R8:          return GL_RED;
	case GS_RGBA:        return GL_RGBA;
	case GS_BGRX:        return GL_BGR;
	case GS_BGRA:        return GL_BGRA;
	case GS_R10G10B10A2: return GL_RGBA;
	case GS_RGBA16:      return GL_RGBA;
	case GS_R16:         return GL_RED;
	case GS_RGBA16F:     return GL_RGBA;
	case GS_RGBA32F:     return GL_RGBA;
	case GS_RG16F:       return GL_RG;
	case GS_RG32F:       return GL_RG;
	case GS_R16F:        return GL_RED;
	case GS_R32F:        return GL_RED;
	case GS_DXT1:        return GL_RGB;
	case GS_DXT3:        return GL_RGBA;
	case GS_DXT5:        return GL_RGBA;
	default:             return 0;
	}
}

static inline GLint convert_gs_internal_format(enum gs_color_format format)
{
	switch (format) {
	case GS_A8:          return GL_R8; /* NOTE: use GL_TEXTURE_SWIZZLE_x */
	case GS_R8:          return GL_R8;
	case GS_RGBA:        return GL_RGBA;
	case GS_BGRX:        return GL_RGBA;
	case GS_BGRA:        return GL_RGBA;
	case GS_R10G10B10A2: return GL_RGB10_A2;
	case GS_RGBA16:      return GL_RGBA16;
	case GS_R16:         return GL_R16;
	case GS_RGBA16F:     return GL_RGBA16F;
	case GS_RGBA32F:     return GL_RGBA32F;
	case GS_RG16F:       return GL_RG16F;
	case GS_RG32F:       return GL_RG32F;
	case GS_R16F:        return GL_R16F;
	case GS_R32F:        return GL_R32F;
	case GS_DXT1:        return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
	case GS_DXT3:        return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
	case GS_DXT5:        return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
	default:             return 0;
	}
}

static inline bool is_compressed_format(enum gs_color_format format)
{
	return (format == GS_DXT1 || format == GS_DXT3 || format == GS_DXT5);
}

static inline GLint convert_zstencil_format(enum gs_zstencil_format format)
{
	switch (format) {
	case GS_Z16:         return GL_DEPTH_COMPONENT16;
	case GS_Z24_S8:      return GL_DEPTH24_STENCIL8;
	case GS_Z32F:        return GL_DEPTH_COMPONENT32F;
	case GS_Z32F_S8X24:  return GL_DEPTH32F_STENCIL8;
	default:             return 0;
	}
}

struct gs_texture {
	device_t             device;
	enum gs_texture_type type;
	enum gs_color_format format;
	GLenum               gl_format;
	GLint                gl_internal_format;
	GLuint               texture;
};

struct gs_texture_2d {
	struct gs_texture    base;

	uint32_t             width;
	uint32_t             height;
	bool                 gen_mipmaps;
};

struct gs_texture_cube {
	struct gs_texture    base;

	uint32_t             size;
};

struct gs_swap_chain {
	device_t device;
	struct gl_windowinfo *wi;
	struct gs_init_data  info;
};

struct gs_device {
	struct gl_platform *plat;

	struct gs_swap_chain *cur_swap;
	int                  cur_render_side;
	struct gs_texture    *cur_render_texture;
};

extern struct gl_platform   *gl_platform_create(device_t device,
                                                struct gs_init_data *info);
extern struct gs_swap_chain *gl_platform_getswap(struct gl_platform *platform);
extern void                  gl_platform_destroy(struct gl_platform *platform);

extern struct gl_windowinfo *gl_windowinfo_create(struct gs_init_data *info);
extern void                  gl_windowinfo_destroy(struct gl_windowinfo *wi);

#endif
