#pragma once

#include "gl-nix.h"

#include <glad/glad_egl.h>

const char *gl_egl_error_to_string(EGLint error_number);

struct gs_texture *
gl_egl_create_dmabuf_image(EGLDisplay egl_display, unsigned int width,
			   unsigned int height, uint32_t drm_format,
			   enum gs_color_format color_format, uint32_t n_planes,
			   const int *fds, const uint32_t *strides,
			   const uint32_t *offsets, const uint64_t *modifiers);

bool gl_egl_query_dmabuf_capabilities(EGLDisplay egl_display,
				      enum gs_dmabuf_flags *dmabuf_flags,
				      uint32_t **drm_format, size_t *n_formats);

bool gl_egl_query_dmabuf_modifiers_for_format(EGLDisplay egl_display,
					      uint32_t drm_format,
					      uint64_t **modifiers,
					      size_t *n_modifiers);
