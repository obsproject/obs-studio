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

struct gs_texture *
gl_egl_create_texture_from_pixmap(EGLDisplay egl_display, uint32_t width,
				  uint32_t height,
				  enum gs_color_format color_format,
				  EGLint target, EGLClientBuffer pixmap);

bool gl_egl_enum_adapters(EGLDisplay display,
			  bool (*callback)(void *param, const char *name,
					   uint32_t id),
			  void *param);
uint32_t gs_get_adapter_count();
