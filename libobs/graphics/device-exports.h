/******************************************************************************
    Copyright (C) 2013-2014 by Hugh Bailey <obs.jim@gmail.com>

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

#include <util/c99defs.h>

#ifdef __cplusplus
extern "C" {
#endif

EXPORT const char *device_name(void);
EXPORT int device_type(void);
EXPORT const char *device_preprocessor_name(void);
EXPORT int device_create(device_t *device, struct gs_init_data *data);
EXPORT void device_destroy(device_t device);
EXPORT void device_entercontext(device_t device);
EXPORT void device_leavecontext(device_t device);
EXPORT swapchain_t device_create_swapchain(device_t device,
		struct gs_init_data *data);
EXPORT void device_resize(device_t device, uint32_t x, uint32_t y);
EXPORT void device_getsize(device_t device, uint32_t *x, uint32_t *y);
EXPORT uint32_t device_getwidth(device_t device);
EXPORT uint32_t device_getheight(device_t device);
EXPORT texture_t device_create_texture(device_t device, uint32_t width,
		uint32_t height, enum gs_color_format color_format,
		uint32_t levels, const uint8_t **data, uint32_t flags);
EXPORT texture_t device_create_cubetexture(device_t device, uint32_t size,
		enum gs_color_format color_format, uint32_t levels,
		const uint8_t **data, uint32_t flags);
EXPORT texture_t device_create_volumetexture(device_t device, uint32_t width,
		uint32_t height, uint32_t depth,
		enum gs_color_format color_format, uint32_t levels,
		const uint8_t **data, uint32_t flags);
EXPORT zstencil_t device_create_zstencil(device_t device, uint32_t width,
		uint32_t height, enum gs_zstencil_format format);
EXPORT stagesurf_t device_create_stagesurface(device_t device, uint32_t width,
		uint32_t height, enum gs_color_format color_format);
EXPORT samplerstate_t device_create_samplerstate(device_t device,
		struct gs_sampler_info *info);
EXPORT shader_t device_create_vertexshader(device_t device,
		const char *shader, const char *file,
		char **error_string);
EXPORT shader_t device_create_pixelshader(device_t device,
		const char *shader, const char *file,
		char **error_string);
EXPORT vertbuffer_t device_create_vertexbuffer(device_t device,
		struct vb_data *data, uint32_t flags);
EXPORT indexbuffer_t device_create_indexbuffer(device_t device,
		enum gs_index_type type, void *indices, size_t num,
		uint32_t flags);
EXPORT enum gs_texture_type device_gettexturetype(texture_t texture);
EXPORT void device_load_vertexbuffer(device_t device, vertbuffer_t vertbuffer);
EXPORT void device_load_indexbuffer(device_t device, indexbuffer_t indexbuffer);
EXPORT void device_load_texture(device_t device, texture_t tex, int unit);
EXPORT void device_load_samplerstate(device_t device,
		samplerstate_t samplerstate, int unit);
EXPORT void device_load_vertexshader(device_t device, shader_t vertshader);
EXPORT void device_load_pixelshader(device_t device, shader_t pixelshader);
EXPORT void device_load_defaultsamplerstate(device_t device, bool b_3d,
		int unit);
EXPORT shader_t device_getvertexshader(device_t device);
EXPORT shader_t device_getpixelshader(device_t device);
EXPORT texture_t device_getrendertarget(device_t device);
EXPORT zstencil_t device_getzstenciltarget(device_t device);
EXPORT void device_setrendertarget(device_t device, texture_t tex,
		zstencil_t zstencil);
EXPORT void device_setcuberendertarget(device_t device, texture_t cubetex,
		int side, zstencil_t zstencil);
EXPORT void device_copy_texture(device_t device, texture_t dst, texture_t src);
EXPORT void device_copy_texture_region(device_t device,
		texture_t dst, uint32_t dst_x, uint32_t dst_y,
		texture_t src, uint32_t src_x, uint32_t src_y,
		uint32_t src_w, uint32_t src_h);
EXPORT void device_stage_texture(device_t device, stagesurf_t dst,
		texture_t src);
EXPORT void device_beginscene(device_t device);
EXPORT void device_draw(device_t device, enum gs_draw_mode draw_mode,
		uint32_t start_vert, uint32_t num_verts);
EXPORT void device_endscene(device_t device);
EXPORT void device_load_swapchain(device_t device, swapchain_t swapchain);
EXPORT void device_clear(device_t device, uint32_t clear_flags,
		struct vec4 *color, float depth, uint8_t stencil);
EXPORT void device_present(device_t device);
EXPORT void device_flush(device_t device);
EXPORT void device_setcullmode(device_t device, enum gs_cull_mode mode);
EXPORT enum gs_cull_mode device_getcullmode(device_t device);
EXPORT void device_enable_blending(device_t device, bool enable);
EXPORT void device_enable_depthtest(device_t device, bool enable);
EXPORT void device_enable_stenciltest(device_t device, bool enable);
EXPORT void device_enable_stencilwrite(device_t device, bool enable);
EXPORT void device_enable_color(device_t device, bool red, bool green,
		bool blue, bool alpha);
EXPORT void device_blendfunction(device_t device, enum gs_blend_type src,
		enum gs_blend_type dest);
EXPORT void device_depthfunction(device_t device, enum gs_depth_test test);
EXPORT void device_stencilfunction(device_t device, enum gs_stencil_side side,
		enum gs_depth_test test);
EXPORT void device_stencilop(device_t device, enum gs_stencil_side side,
		enum gs_stencil_op fail, enum gs_stencil_op zfail,
		enum gs_stencil_op zpass);
EXPORT void device_enable_fullscreen(device_t device, bool enable);
EXPORT int device_fullscreen_enabled(device_t device);
EXPORT void device_setdisplaymode(device_t device,
		const struct gs_display_mode *mode);
EXPORT void device_getdisplaymode(device_t device,
		struct gs_display_mode *mode);
EXPORT void device_setcolorramp(device_t device, float gamma, float brightness,
		float contrast);
EXPORT void device_setviewport(device_t device, int x, int y, int width,
		int height);
EXPORT void device_getviewport(device_t device, struct gs_rect *rect);
EXPORT void device_setscissorrect(device_t device, struct gs_rect *rect);
EXPORT void device_ortho(device_t device, float left, float right,
		float top, float bottom, float znear, float zfar);
EXPORT void device_frustum(device_t device, float left, float right,
		float top, float bottom, float znear, float zfar);
EXPORT void device_projection_push(device_t device);
EXPORT void device_projection_pop(device_t device);

#ifdef __cplusplus
}
#endif
