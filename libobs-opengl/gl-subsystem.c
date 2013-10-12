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

void convert_sampler_info(struct gs_sampler_state *sampler,
		struct gs_sampler_info *info)
{
	convert_filter(info->filter, &sampler->min_filter,
			&sampler->mag_filter);
	sampler->address_u      = convert_address_mode(info->address_u);
	sampler->address_v      = convert_address_mode(info->address_v);
	sampler->address_w      = convert_address_mode(info->address_w);
	sampler->max_anisotropy = info->max_anisotropy;
}

device_t device_create(struct gs_init_data *info)
{
	struct gs_device *device = bmalloc(sizeof(struct gs_device));
	memset(device, 0, sizeof(struct gs_device));

	device->plat = gl_platform_create(device, info);
	if (!device->plat)
		goto fail;

	glGenProgramPipelines(1, &device->pipeline);
	if (!gl_success("glGenProgramPipelines"))
		goto fail;

	glBindProgramPipeline(device->pipeline);
	if (!gl_success("glBindProgramPipeline"))
		goto fail;

	return device;

fail:
	blog(LOG_ERROR, "device_create (GL) failed");
	bfree(device);
	return NULL;
}

void device_destroy(device_t device)
{
	if (device) {
		if (device->pipeline)
			glDeleteProgramPipelines(1, &device->pipeline);
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

samplerstate_t device_create_samplerstate(device_t device,
		struct gs_sampler_info *info)
{
	struct gs_sampler_state *sampler;

	sampler = bmalloc(sizeof(struct gs_sampler_state));
	memset(sampler, 0, sizeof(struct gs_sampler_state));
	sampler->device = device;
	sampler->ref    = 1;

	convert_sampler_info(sampler, info);
	return sampler;
}

enum gs_texture_type device_gettexturetype(device_t device,
		texture_t texture)
{
	return texture->type;
}

static bool load_texture_sampler(texture_t tex, samplerstate_t ss)
{
	bool success = true;
	if (tex->cur_sampler == ss)
		return true;

	if (tex->cur_sampler)
		samplerstate_release(tex->cur_sampler);
	tex->cur_sampler = ss;
	if (!ss)
		return true;

	samplerstate_addref(ss);

	if (!gl_tex_param_i(tex->gl_target, GL_TEXTURE_MIN_FILTER,
				ss->min_filter))
		success = false;
	if (!gl_tex_param_i(tex->gl_target, GL_TEXTURE_MAG_FILTER,
				ss->mag_filter))
		success = false;
	if (!gl_tex_param_i(tex->gl_target, GL_TEXTURE_WRAP_S, ss->address_u))
		success = false;
	if (!gl_tex_param_i(tex->gl_target, GL_TEXTURE_WRAP_T, ss->address_v))
		success = false;
	if (!gl_tex_param_i(tex->gl_target, GL_TEXTURE_WRAP_R, ss->address_w))
		success = false;
	if (!gl_tex_param_i(tex->gl_target, GL_TEXTURE_MAX_ANISOTROPY_EXT,
			ss->max_anisotropy))
		success = false;

	return success;
}

static inline struct shader_param *get_texture_param(device_t device, int unit)
{
	struct gs_shader *shader = device->cur_pixel_shader;
	size_t i;

	for (i = 0; i < shader->params.num; i++) {
		struct shader_param *param = shader->params.array+i;
		if (param->type == SHADER_PARAM_TEXTURE) {
			if (param->texture_id == unit)
				return param;
		}
	}

	return NULL;
}

void device_load_texture(device_t device, texture_t tex, int unit)
{
	struct shader_param *param;
	struct gs_sampler_state *sampler;

	/* need a pixel shader to properly bind textures */
	if (!device->cur_pixel_shader)
		tex = NULL;

	if (device->cur_textures[unit] == tex)
		return;

	device->cur_textures[unit] = tex;
	param = get_texture_param(device, unit);
	if (!param)
		return;

	param->texture = tex;

	if (!tex)
		return;

	sampler = device->cur_samplers[param->sampler_id];

	if (!gl_active_texture(GL_TEXTURE0 + unit))
		goto fail;
	if (!gl_bind_texture(tex->gl_target, tex->texture))
		goto fail;
	if (sampler && !load_texture_sampler(tex, sampler))
		goto fail;

	return;

fail:
	blog(LOG_ERROR, "device_load_texture (GL) failed");
}

static bool load_sampler_on_textures(device_t device, samplerstate_t ss,
		int sampler_unit)
{
	struct gs_shader *shader = device->cur_pixel_shader;
	size_t i;

	for (i = 0; i < shader->params.num; i++) {
		struct shader_param *param = shader->params.array+i;

		if (param->type == SHADER_PARAM_TEXTURE &&
		    param->sampler_id == sampler_unit &&
		    param->texture) {
			if (!gl_active_texture(GL_TEXTURE0 + param->texture_id))
				return false;
			if (!load_texture_sampler(param->texture, ss))
				return false;
		}
	}

	return true;
}

void device_load_samplerstate(device_t device, samplerstate_t ss, int unit)
{
	/* need a pixel shader to properly bind samplers */
	if (!device->cur_pixel_shader)
		ss = NULL;

	if (device->cur_samplers[unit] == ss)
		return;

	device->cur_samplers[unit] = ss;

	if (!ss)
		return;

	if (!load_sampler_on_textures(device, ss, unit))
		blog(LOG_ERROR, "device_load_samplerstate (GL) failed");

	return;
}

void device_load_vertexshader(device_t device, shader_t vertshader)
{
	GLuint program = 0;
	vertbuffer_t cur_vb = device->cur_vertex_buffer;

	if (device->cur_vertex_shader == vertshader)
		return;

	if (vertshader->type != SHADER_VERTEX) {
		blog(LOG_ERROR, "Specified shader is not a vertex shader");
		goto fail;
	}

	/* unload and reload the vertex buffer to sync the buffers up with
	 * the specific shader */
	if (cur_vb && !vertexbuffer_load(device, NULL))
		goto fail;

	device->cur_vertex_shader = vertshader;

	if (vertshader)
		program = vertshader->program;

	glUseProgramStages(device->pipeline, GL_VERTEX_SHADER, program);
	if (!gl_success("glUseProgramStages"))
		goto fail;

	if (cur_vb && !vertexbuffer_load(device, cur_vb))
		goto fail;

	return;

fail:
	blog(LOG_ERROR, "device_load_vertexshader (GL) failed");
}

void device_load_pixelshader(device_t device, shader_t pixelshader)
{
	GLuint program = 0;
	if (device->cur_pixel_shader == pixelshader)
		return;

	if (pixelshader->type != SHADER_PIXEL) {
		blog(LOG_ERROR, "Specified shader is not a pixel shader");
		goto fail;
	}

	device->cur_pixel_shader = pixelshader;

	if (pixelshader)
		program = pixelshader->program;

	glUseProgramStages(device->pipeline, GL_FRAGMENT_SHADER,
			pixelshader->program);
	if (!gl_success("glUseProgramStages"))
		goto fail;

	return;

fail:
	blog(LOG_ERROR, "device_load_pixelshader (GL) failed");
}

void device_load_defaultsamplerstate(device_t device, bool b_3d, int unit)
{
	/* TODO */
}

shader_t device_getvertexshader(device_t device)
{
	return device->cur_vertex_shader;
}

shader_t device_getpixelshader(device_t device)
{
	return device->cur_pixel_shader;
}

texture_t device_getrendertarget(device_t device)
{
	return device->cur_render_target;
}

zstencil_t device_getzstenciltarget(device_t device)
{
	return device->cur_zstencil_buffer;
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
	/* TODO */
}

uint32_t volumetexture_getwidth(texture_t voltex)
{
	/* TODO */
	return 0;
}

uint32_t volumetexture_getheight(texture_t voltex)
{
	/* TODO */
	return 0;
}

uint32_t volumetexture_getdepth(texture_t voltex)
{
	/* TODO */
	return 0;
}

enum gs_color_format volumetexture_getcolorformat(texture_t voltex)
{
	/* TODO */
	return GS_UNKNOWN;
}

void samplerstate_destroy(samplerstate_t samplerstate)
{
	samplerstate_release(samplerstate);
}
