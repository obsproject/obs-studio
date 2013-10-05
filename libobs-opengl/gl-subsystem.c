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

device_t device_create(struct gs_init_data *info)
{
	struct gs_device *device = bmalloc(sizeof(struct gs_device));
	memset(device, 0, sizeof(struct gs_device));

	device->plat = gl_platform_create(device, info);
	if (!device->plat) {
		blog(LOG_ERROR, "device_create (GL) failed");
		bfree(device);
		return NULL;
	}

	return device;
}

void device_destroy(device_t device)
{
	if (device) {
		gl_platform_destroy(device->plat);
		bfree(device);
	}
}

swapchain_t device_create_swapchain(device_t device, struct gs_init_data *info)
{
	struct gs_swap_chain *swap = bmalloc(sizeof(struct gs_swap_chain));
	memset(swap, 0, sizeof(struct gs_swap_chain));

	swap->device = device;
	swap->info   = *info;
	swap->wi     = gl_windowinfo_create(info);
	if (!swap->wi) {
		blog(LOG_ERROR, "device_create_swapchain (GL) failed");
		swapchain_destroy(swap);
		return NULL;
	}

	return swap;
}

void device_resize(device_t device, uint32_t cx, uint32_t cy)
{
	/* GL automatically resizes the device, so it doesn't do much */
	device->cur_swap->info.cx = cx;
	device->cur_swap->info.cy = cy;
}

void device_getsize(device_t device, uint32_t *cx, uint32_t *cy)
{
	*cx = device->cur_swap->info.cx;
	*cy = device->cur_swap->info.cy;
}

uint32_t device_getwidth(device_t device)
{
	return device->cur_swap->info.cx;
}

uint32_t device_getheight(device_t device)
{
	return device->cur_swap->info.cy;
}

texture_t device_create_volumetexture(device_t device, uint32_t width,
		uint32_t height, uint32_t depth,
		enum gs_color_format color_format, uint32_t levels, void **data,
		uint32_t flags)
{
	/* TODO */
	return NULL;
}

zstencil_t device_create_zstencil(device_t device, uint32_t width,
		uint32_t height, enum gs_zstencil_format format)
{
}

stagesurf_t device_create_stagesurface(device_t device, uint32_t width,
		uint32_t height, enum gs_color_format color_format)
{
}

samplerstate_t device_create_samplerstate(device_t device,
		struct gs_sampler_info *info)
{
}

shader_t device_create_vertexshader(device_t device,
		const char *shader, const char *file,
		char **error_string)
{
}

shader_t device_create_pixelshader(device_t device,
		const char *shader, const char *file,
		char **error_string)
{
}

vertbuffer_t device_create_vertexbuffer(device_t device,
		struct vb_data *data, uint32_t flags)
{
}

indexbuffer_t device_create_indexbuffer(device_t device,
		enum gs_index_type type, void *indices, size_t num,
		uint32_t flags)
{
}

enum gs_texture_type device_gettexturetype(device_t device,
		texture_t texture)
{
}

void device_load_vertexbuffer(device_t device, vertbuffer_t vertbuffer)
{
}

void device_load_indexbuffer(device_t device, indexbuffer_t indexbuffer)
{
}

void device_load_texture(device_t device, texture_t tex, int unit)
{
}

void device_load_cubetexture(device_t device, texture_t cubetex,
		int unit)
{
}

void device_load_volumetexture(device_t device, texture_t voltex,
		int unit)
{
}

void device_load_samplerstate(device_t device,
		samplerstate_t samplerstate, int unit)
{
}

void device_load_vertexshader(device_t device, shader_t vertshader)
{
}

void device_load_pixelshader(device_t device, shader_t pixelshader)
{
}

void device_load_defaultsamplerstate(device_t device, bool b_3d,
		int unit)
{
}

shader_t device_getvertexshader(device_t device)
{
}

shader_t device_getpixelshader(device_t device)
{
}

texture_t device_getrendertarget(device_t device)
{
}

zstencil_t device_getzstenciltarget(device_t device)
{
}

void device_setrendertarget(device_t device, texture_t tex,
		zstencil_t zstencil)
{
}

void device_setcuberendertarget(device_t device, texture_t cubetex,
		int side, zstencil_t zstencil)
{
}

void device_copy_texture(device_t device, texture_t dst, texture_t src)
{
}

void device_stage_texture(device_t device, stagesurf_t dst,
		texture_t src)
{
}

void device_beginscene(device_t device)
{
}

void device_draw(device_t device, enum gs_draw_mode draw_mode,
		uint32_t start_vert, uint32_t num_verts)
{
}

void device_endscene(device_t device)
{
}

void device_load_swapchain(device_t device, swapchain_t swapchain)
{
}

void device_clear(device_t device, uint32_t clear_flags,
		struct vec4 *color, float depth, uint8_t stencil)
{
}

void device_present(device_t device)
{
}

void device_setcullmode(device_t device, enum gs_cull_mode mode)
{
}

enum gs_cull_mode device_getcullmode(device_t device)
{
}

void device_enable_blending(device_t device, bool enable)
{
}

void device_enable_depthtest(device_t device, bool enable)
{
}

void device_enable_stenciltest(device_t device, bool enable)
{
}

void device_enable_stencilwrite(device_t device, bool enable)
{
}

void device_enable_color(device_t device, bool red, bool blue,
		bool green, bool alpha)
{
}

void device_blendfunction(device_t device, enum gs_blend_type src,
		enum gs_blend_type dest)
{
}

void device_depthfunction(device_t device, enum gs_depth_test test)
{
}

void device_stencilfunction(device_t device, enum gs_stencil_side side,
		enum gs_depth_test test)
{
}

void device_stencilop(device_t device, enum gs_stencil_side side,
		enum gs_stencil_op fail, enum gs_stencil_op zfail,
		enum gs_stencil_op zpass)
{
}

void device_enable_fullscreen(device_t device, bool enable)
{
}

int device_fullscreen_enabled(device_t device)
{
}

void device_setdisplaymode(device_t device,
		const struct gs_display_mode *mode)
{
}

void device_getdisplaymode(device_t device,
		struct gs_display_mode *mode)
{
}

void device_setcolorramp(device_t device, float gamma, float brightness,
		float contrast)
{
}

void device_setviewport(device_t device, int x, int y, int width,
		int height)
{
}

void device_getviewport(device_t device, struct gs_rect *rect)
{
}

void device_setscissorrect(device_t device, struct gs_rect *rect)
{
}

void device_ortho(device_t device, float left, float right,
		float top, float bottom, float znear, float zfar)
{
}

void device_frustum(device_t device, float left, float right,
		float top, float bottom, float znear, float zfar)
{
}

void device_perspective(device_t device, float fovy, float aspect,
		float znear, float zfar)
{
}

void device_set_view_matrix(device_t device, struct matrix3 *mat)
{
}

void device_projection_push(device_t device)
{
}

void device_projection_pop(device_t device)
{
}

void swapchain_destroy(swapchain_t swapchain)
{
}

void volumetexture_destroy(texture_t voltex)
{
}

uint32_t volumetexture_getwidth(texture_t voltex)
{
}

uint32_t volumetexture_getheight(texture_t voltex)
{
}

uint32_t volumetexture_getdepth(texture_t voltex)
{
}

enum gs_color_format volumetexture_getcolorformat(texture_t voltex)
{
}

void stagesurface_destroy(stagesurf_t stagesurf)
{
}

uint32_t stagesurface_getwidth(stagesurf_t stagesurf)
{
}

uint32_t stagesurface_getheight(stagesurf_t stagesurf)
{
}

enum gs_color_format stagesurface_getcolorformat(stagesurf_t stagesurf)
{
}

bool stagesurface_map(stagesurf_t stagesurf, const void **data,
		uint32_t *byte_width)
{
}

void stagesurface_unmap(stagesurf_t stagesurf)
{
}

void zstencil_destroy(zstencil_t zstencil)
{
}

void samplerstate_destroy(samplerstate_t samplerstate)
{
}

void vertexbuffer_destroy(vertbuffer_t vertbuffer)
{
}

void vertexbuffer_flush(vertbuffer_t vertbuffer, bool rebuild)
{
}

struct vb_data *vertexbuffer_getdata(vertbuffer_t vertbuffer)
{
}

void indexbuffer_destroy(indexbuffer_t indexbuffer)
{
}

void indexbuffer_flush(indexbuffer_t indexbuffer)
{
}

void *indexbuffer_getdata(indexbuffer_t indexbuffer)
{
}

size_t indexbuffer_numindices(indexbuffer_t indexbuffer)
{
}

enum gs_index_type indexbuffer_gettype(indexbuffer_t indexbuffer)
{
}

void shader_destroy(shader_t shader)
{
}

int shader_numparams(shader_t shader)
{
}

sparam_t shader_getparambyidx(shader_t shader, int param)
{
}

sparam_t shader_getparambyname(shader_t shader, const char *name)
{
}

void shader_getparaminfo(shader_t shader, sparam_t param,
		struct shader_param_info *info)
{
}

sparam_t shader_getviewprojmatrix(shader_t shader)
{
}

sparam_t shader_getworldmatrix(shader_t shader)
{
}

void shader_setbool(shader_t shader, sparam_t param, bool val)
{
}

void shader_setfloat(shader_t shader, sparam_t param, float val)
{
}

void shader_setint(shader_t shader, sparam_t param, int val)
{
}

void shader_setmatrix3(shader_t shader, sparam_t param,
		const struct matrix3 *val)
{
}

void shader_setmatrix4(shader_t shader, sparam_t param,
		const struct matrix4 *val)
{
}

void shader_setvec2(shader_t shader, sparam_t param,
		const struct vec2 *val)
{
}

void shader_setvec3(shader_t shader, sparam_t param,
		const struct vec3 *val)
{
}

void shader_setvec4(shader_t shader, sparam_t param,
		const struct vec4 *val)
{
}

void shader_settexture(shader_t shader, sparam_t param, texture_t val)
{
}

void shader_setval(shader_t shader, sparam_t param, const void *val,
		size_t size)
{
}

void shader_setdefault(shader_t shader, sparam_t param)
{
}
