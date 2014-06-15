/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>
    Copyright (C) 2014 by Zachary Lund <admin@computerquip.com>

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

#include <graphics/matrix3.h>
#include "gl-subsystem.h"

/* Goofy Windows.h macros need to be removed */
#undef far
#undef near

#ifdef _DEBUG
/* Tables for OpenGL debug */
static const char* debug_source_table[] = {
	"API",
	"Window System",
	"Shader Compiler",
	"Third Party"
	"Application",
	"Other"
};

static const char* debug_type_table[] = {
	"Error",
	"Deprecated Behavior",
	"Undefined Behavior",
	"Portability",
	"Performance",
	"Other"
};

static const char* debug_severity_table[] = {
	"High",
	"Medium",
	"Low"
};

/* ARB and core values are the same. They'll always be linear so no hardcoding.
 * The values subtracted are the lowest value in the list of valid values. */
#define GL_DEBUG_SOURCE_OFFSET(x) (x - GL_DEBUG_SOURCE_API_ARB)
#define GL_DEBUG_TYPE_OFFSET(x) (x - GL_DEBUG_TYPE_ERROR_ARB)
#define GL_DEBUG_SEVERITY_OFFSET(x) (x - GL_DEBUG_SEVERITY_HIGH_ARB)

static void APIENTRY gl_debug_proc(
	GLenum source, GLenum type, GLuint id, GLenum severity, 
	GLsizei length, const GLchar *message, const GLvoid *data )
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(data);

	blog(	LOG_DEBUG,
		"[%s][%s]{%s}: %.*s",
		debug_source_table[GL_DEBUG_SOURCE_OFFSET(source)],
		debug_type_table[GL_DEBUG_TYPE_OFFSET(type)],
		debug_severity_table[GL_DEBUG_SEVERITY_OFFSET(severity)],
		length, message
	);
}

static void gl_enable_debug()
{
	if (GLAD_GL_VERSION_4_3) {
		glDebugMessageCallback(gl_debug_proc, NULL);
		gl_enable(GL_DEBUG_OUTPUT);
	} else if (GLAD_GL_ARB_debug_output) {
		glDebugMessageCallbackARB(gl_debug_proc, NULL);
	} else {
		blog(LOG_DEBUG, "Failed to set GL debug callback as it is "
		                "not supported.");
	}
}
#else
static void gl_enable_debug() {}
#endif

static bool gl_init_extensions(struct gs_device* device)
{
	if (!GLAD_GL_VERSION_2_1) {
		blog(LOG_ERROR, "obs-studio requires OpenGL version 2.1 or "
		                "higher.");
		return false;
	}

	gl_enable_debug();

	if (!GLAD_GL_VERSION_3_0 && !GLAD_GL_ARB_framebuffer_object) {
		blog(LOG_ERROR, "OpenGL extension ARB_framebuffer_object "
		                "is required.");
		return false;
	}

	if (GLAD_GL_VERSION_3_2 || GLAD_GL_ARB_seamless_cube_map) {
		gl_enable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	}

	if (!GLAD_GL_VERSION_4_1 && !GLAD_GL_ARB_separate_shader_objects) {
		blog(LOG_ERROR, "OpenGL extension ARB_separate_shader_objects "
		                "is required.");
		return false;
	}

	if (GLAD_GL_VERSION_4_3 || GLAD_GL_ARB_copy_image)
		device->copy_type = COPY_TYPE_ARB;
	else if (GLAD_GL_NV_copy_image)
		device->copy_type = COPY_TYPE_NV;
	else
		device->copy_type = COPY_TYPE_FBO_BLIT;

	return true;
}

static void clear_textures(struct gs_device *device)
{
	GLenum i;
	for (i = 0; i < GS_MAX_TEXTURES; i++) {
		if (device->cur_textures[i]) {
			gl_active_texture(GL_TEXTURE0 + i);
			gl_bind_texture(device->cur_textures[i]->gl_target, 0);
			device->cur_textures[i] = NULL;
		}
	}
}

void convert_sampler_info(struct gs_sampler_state *sampler,
		struct gs_sampler_info *info)
{
	GLint max_anisotropy_max;
	convert_filter(info->filter, &sampler->min_filter,
			&sampler->mag_filter);
	sampler->address_u      = convert_address_mode(info->address_u);
	sampler->address_v      = convert_address_mode(info->address_v);
	sampler->address_w      = convert_address_mode(info->address_w);
	sampler->max_anisotropy = info->max_anisotropy;

	max_anisotropy_max = 1;
	glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_anisotropy_max);
	gl_success("glGetIntegerv(GL_MAX_TEXTURE_ANISOTROPY_MAX)");

	if (1 <= sampler->max_anisotropy &&
	    sampler->max_anisotropy <= max_anisotropy_max)
		return;

	if (sampler->max_anisotropy < 1)
		sampler->max_anisotropy = 1;
	else if (sampler->max_anisotropy > max_anisotropy_max)
		sampler->max_anisotropy = max_anisotropy_max;

	blog(LOG_INFO, "convert_sampler_info: 1 <= max_anisotropy <= "
	               "%d violated, selected: %d, set: %d",
	               max_anisotropy_max,
	               info->max_anisotropy, sampler->max_anisotropy);
}

const char *device_preprocessor_name(void)
{
	return "_OPENGL";
}

device_t device_create(struct gs_init_data *info)
{
	struct gs_device *device = bzalloc(sizeof(struct gs_device));

	device->plat = gl_platform_create(device, info);
	if (!device->plat)
		goto fail;

	if (!gl_init_extensions(device))
		goto fail;
	
	gl_enable(GL_CULL_FACE);
	
	glGenProgramPipelines(1, &device->pipeline);
	if (!gl_success("glGenProgramPipelines"))
		goto fail;

	glBindProgramPipeline(device->pipeline);
	if (!gl_success("glBindProgramPipeline"))
		goto fail;

	device_leavecontext(device);
	device->cur_swap = gl_platform_getswap(device->plat);

	return device;

fail:
	blog(LOG_ERROR, "device_create (GL) failed");
	bfree(device);
	return NULL;
}

void device_destroy(device_t device)
{
	if (device) {
		size_t i;
		for (i = 0; i < device->fbos.num; i++)
			fbo_info_destroy(device->fbos.array[i]);

		if (device->pipeline)
			glDeleteProgramPipelines(1, &device->pipeline);

		da_free(device->proj_stack);
		da_free(device->fbos);
		gl_platform_destroy(device->plat);
		bfree(device);
	}
}

swapchain_t device_create_swapchain(device_t device, struct gs_init_data *info)
{
	struct gs_swap_chain *swap = bzalloc(sizeof(struct gs_swap_chain));

	swap->device = device;
	swap->info   = *info;
	swap->wi     = gl_windowinfo_create(info);
	if (!swap->wi) {
		blog(LOG_ERROR, "device_create_swapchain (GL) failed");
		swapchain_destroy(swap);
		return NULL;
	}

	if (!gl_platform_init_swapchain(swap)) {
		blog(LOG_ERROR, "gl_platform_init_swapchain  failed");
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

	gl_update(device);
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
		enum gs_color_format color_format, uint32_t levels,
		const void **data, uint32_t flags)
{
	/* TODO */
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(width);
	UNUSED_PARAMETER(height);
	UNUSED_PARAMETER(depth);
	UNUSED_PARAMETER(color_format);
	UNUSED_PARAMETER(levels);
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(flags);
	return NULL;
}

samplerstate_t device_create_samplerstate(device_t device,
		struct gs_sampler_info *info)
{
	struct gs_sampler_state *sampler;

	sampler = bzalloc(sizeof(struct gs_sampler_state));
	sampler->device = device;
	sampler->ref    = 1;

	convert_sampler_info(sampler, info);
	return sampler;
}

enum gs_texture_type device_gettexturetype(texture_t texture)
{
	return texture->type;
}

static void strip_mipmap_filter(GLint *filter)
{
	switch (*filter) {
		case GL_NEAREST:
		case GL_LINEAR:
			return;
		case GL_NEAREST_MIPMAP_NEAREST:
		case GL_NEAREST_MIPMAP_LINEAR:
			*filter = GL_NEAREST;
			return;
		case GL_LINEAR_MIPMAP_NEAREST:
		case GL_LINEAR_MIPMAP_LINEAR:
			*filter = GL_LINEAR;
			return;
	}
	*filter = GL_NEAREST;
}

static inline void apply_swizzle(struct gs_texture *tex)
{
	if (tex->format == GS_A8) {
		gl_tex_param_i(tex->gl_target, GL_TEXTURE_SWIZZLE_R, GL_ONE);
		gl_tex_param_i(tex->gl_target, GL_TEXTURE_SWIZZLE_G, GL_ONE);
		gl_tex_param_i(tex->gl_target, GL_TEXTURE_SWIZZLE_B, GL_ONE);
		gl_tex_param_i(tex->gl_target, GL_TEXTURE_SWIZZLE_A, GL_RED);
	}
}

static bool load_texture_sampler(texture_t tex, samplerstate_t ss)
{
	bool  success = true;
	GLint min_filter;

	if (tex->cur_sampler == ss)
		return true;

	if (tex->cur_sampler)
		samplerstate_release(tex->cur_sampler);
	tex->cur_sampler = ss;
	if (!ss)
		return true;

	samplerstate_addref(ss);

	min_filter = ss->min_filter;
	if (texture_isrect(tex))
		strip_mipmap_filter(&min_filter);

	if (!gl_tex_param_i(tex->gl_target, GL_TEXTURE_MIN_FILTER,
				min_filter))
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

	apply_swizzle(tex);

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
	struct gs_texture *cur_tex = device->cur_textures[unit];

	/* need a pixel shader to properly bind textures */
	if (!device->cur_pixel_shader)
		tex = NULL;

	if (cur_tex == tex)
		return;

	if (!gl_active_texture(GL_TEXTURE0 + unit))
		goto fail;

	/* the target for the previous text may not be the same as the
	 * next texture, so unbind the previous texture first to be safe */
	if (cur_tex && (!tex || cur_tex->gl_target != tex->gl_target))
		gl_bind_texture(cur_tex->gl_target, 0);

	device->cur_textures[unit] = tex;
	param = get_texture_param(device, unit);
	if (!param)
		return;

	param->texture = tex;

	if (!tex)
		return;

	sampler = device->cur_samplers[param->sampler_id];

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
		    param->sampler_id == (uint32_t)sampler_unit &&
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

	if (vertshader && vertshader->type != SHADER_VERTEX) {
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

	glUseProgramStages(device->pipeline, GL_VERTEX_SHADER_BIT, program);
	if (!gl_success("glUseProgramStages"))
		goto fail;

	if (cur_vb && !vertexbuffer_load(device, cur_vb))
		goto fail;

	return;

fail:
	blog(LOG_ERROR, "device_load_vertexshader (GL) failed");
}

static void load_default_pixelshader_samplers(struct gs_device *device,
		struct gs_shader *ps)
{
	size_t i;
	if (!ps)
		return;

	for (i = 0; i < ps->samplers.num; i++) {
		struct gs_sampler_state *ss = ps->samplers.array[i];
		device->cur_samplers[i] = ss;
	}

	for (; i < GS_MAX_TEXTURES; i++)
		device->cur_samplers[i] = NULL;
}

void device_load_pixelshader(device_t device, shader_t pixelshader)
{
	GLuint program = 0;
	if (device->cur_pixel_shader == pixelshader)
		return;

	if (pixelshader && pixelshader->type != SHADER_PIXEL) {
		blog(LOG_ERROR, "Specified shader is not a pixel shader");
		goto fail;
	}

	device->cur_pixel_shader = pixelshader;

	if (pixelshader)
		program = pixelshader->program;

	glUseProgramStages(device->pipeline, GL_FRAGMENT_SHADER_BIT, program);
	if (!gl_success("glUseProgramStages"))
		goto fail;

	clear_textures(device);

	if (pixelshader)
		load_default_pixelshader_samplers(device, pixelshader);
	return;

fail:
	blog(LOG_ERROR, "device_load_pixelshader (GL) failed");
}

void device_load_defaultsamplerstate(device_t device, bool b_3d, int unit)
{
	/* TODO */
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(b_3d);
	UNUSED_PARAMETER(unit);
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

static bool get_tex_dimensions(texture_t tex, uint32_t *width, uint32_t *height)
{
	if (tex->type == GS_TEXTURE_2D) {
		struct gs_texture_2d *tex2d = (struct gs_texture_2d*)tex;
		*width  = tex2d->width;
		*height = tex2d->height;
		return true;

	} else if (tex->type == GS_TEXTURE_CUBE) {
		struct gs_texture_cube *cube = (struct gs_texture_cube*)tex;
		*width  = cube->size;
		*height = cube->size;
		return true;
	}

	blog(LOG_ERROR, "Texture must be 2D or cubemap");
	return false;
}

/*
 * This automatically manages FBOs so that render targets are always given
 * an FBO that matches their width/height/format to maximize optimization
 */
struct fbo_info *get_fbo(struct gs_device *device,
		uint32_t width, uint32_t height, enum gs_color_format format)
{
	size_t i;
	GLuint fbo;
	struct fbo_info *ptr;

	for (i = 0; i < device->fbos.num; i++) {
		ptr = device->fbos.array[i];

		if (ptr->width  == width && ptr->height == height &&
		    ptr->format == format)
			return ptr;
	}

	glGenFramebuffers(1, &fbo);
	if (!gl_success("glGenFramebuffers"))
		return NULL;

	ptr = bmalloc(sizeof(struct fbo_info));
	ptr->fbo                 = fbo;
	ptr->width               = width;
	ptr->height              = height;
	ptr->format              = format;
	ptr->cur_render_target   = NULL;
	ptr->cur_render_side     = 0;
	ptr->cur_zstencil_buffer = NULL;

	da_push_back(device->fbos, &ptr);
	return ptr;
}

static inline struct fbo_info *get_fbo_by_tex(struct gs_device *device,
		texture_t tex)
{
	uint32_t width, height;
	if (!get_tex_dimensions(tex, &width, &height))
		return NULL;

	return get_fbo(device, width, height, tex->format);
}

static bool set_current_fbo(device_t device, struct fbo_info *fbo)
{
	if (device->cur_fbo != fbo) {
		GLuint fbo_obj = fbo ? fbo->fbo : 0;
		if (!gl_bind_framebuffer(GL_DRAW_FRAMEBUFFER, fbo_obj))
			return false;
	}

	device->cur_fbo = fbo;
	return true;
}

static bool attach_rendertarget(struct fbo_info *fbo, texture_t tex, int side)
{
	if (fbo->cur_render_target == tex)
		return true;

	fbo->cur_render_target = tex;

	if (tex->type == GS_TEXTURE_2D) {
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
				GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
				tex->texture, 0);

	} else if (tex->type == GS_TEXTURE_CUBE) {
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
				GL_COLOR_ATTACHMENT0,
				GL_TEXTURE_CUBE_MAP_POSITIVE_X + side,
				tex->texture, 0);

	} else {
		return false;
	}

	return gl_success("glFramebufferTexture2D");
}

static bool attach_zstencil(struct fbo_info *fbo, zstencil_t zs)
{
	GLuint zsbuffer = 0;
	GLenum zs_attachment = GL_DEPTH_STENCIL_ATTACHMENT;

	if (fbo->cur_zstencil_buffer == zs)
		return true;

	fbo->cur_zstencil_buffer = zs;

	if (zs) {
		zsbuffer = zs->buffer;
		zs_attachment = zs->attachment;
	}

	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER,
			zs_attachment, GL_RENDERBUFFER, zsbuffer);
	if (!gl_success("glFramebufferRenderbuffer"))
		return false;

	return true;
}

static bool set_target(device_t device, texture_t tex, int side, zstencil_t zs)
{
	struct fbo_info *fbo;

	if (device->cur_render_target   == tex &&
	    device->cur_zstencil_buffer == zs  &&
	    device->cur_render_side     == side)
		return true;

	device->cur_render_target   = tex;
	device->cur_render_side     = side;
	device->cur_zstencil_buffer = zs;

	if (!tex)
		return set_current_fbo(device, NULL);

	fbo = get_fbo_by_tex(device, tex);
	if (!fbo)
		return false;

	set_current_fbo(device, fbo);

	if (!attach_rendertarget(fbo, tex, side))
		return false;
	if (!attach_zstencil(fbo, zs))
		return false;

	return true;
}

void device_setrendertarget(device_t device, texture_t tex, zstencil_t zstencil)
{
	if (tex) {
		if (tex->type != GS_TEXTURE_2D) {
			blog(LOG_ERROR, "Texture is not a 2D texture");
			goto fail;
		}

		if (!tex->is_render_target) {
			blog(LOG_ERROR, "Texture is not a render target");
			goto fail;
		}
	}

	if (!set_target(device, tex, 0, zstencil))
		goto fail;

	return;

fail:
	blog(LOG_ERROR, "device_setrendertarget (GL) failed");
}

void device_setcuberendertarget(device_t device, texture_t cubetex,
		int side, zstencil_t zstencil)
{
	if (cubetex) {
		if (cubetex->type != GS_TEXTURE_CUBE) {
			blog(LOG_ERROR, "Texture is not a cube texture");
			goto fail;
		}

		if (!cubetex->is_render_target) {
			blog(LOG_ERROR, "Texture is not a render target");
			goto fail;
		}
	}

	if (!set_target(device, cubetex, side, zstencil))
		goto fail;

	return;

fail:
	blog(LOG_ERROR, "device_setcuberendertarget (GL) failed");
}

void device_copy_texture_region(device_t device,
		texture_t dst, uint32_t dst_x, uint32_t dst_y,
		texture_t src, uint32_t src_x, uint32_t src_y,
		uint32_t src_w, uint32_t src_h)
{
	struct gs_texture_2d *src2d = (struct gs_texture_2d*)src;
	struct gs_texture_2d *dst2d = (struct gs_texture_2d*)dst;

	if (!src) {
		blog(LOG_ERROR, "Source texture is NULL");
		goto fail;
	}

	if (!dst) {
		blog(LOG_ERROR, "Destination texture is NULL");
		goto fail;
	}

	if (dst->type != GS_TEXTURE_2D || src->type != GS_TEXTURE_2D) {
		blog(LOG_ERROR, "Source and destination textures must be 2D "
						"textures");
		goto fail;
	}

	if (dst->format != src->format) {
		blog(LOG_ERROR, "Source and destination formats do not match");
		goto fail;
	}

	uint32_t nw = (uint32_t)src_w ? (uint32_t)src_w : (src2d->width - src_x);
	uint32_t nh = (uint32_t)src_h ? (uint32_t)src_h : (src2d->height - src_y);

	if (dst2d->width - dst_x < nw || dst2d->height - dst_y < nh) {
		blog(LOG_ERROR, "Destination texture region is not big "
						"enough to hold the source region");
		goto fail;
	}

	if (!gl_copy_texture(device, dst->texture, dst->gl_target, dst_x, dst_y,
				src->texture, src->gl_target, src_x, src_y,
				nw, nh, src->format))
		goto fail;

	return;

fail:
	blog(LOG_ERROR, "device_copy_texture (GL) failed");
}

void device_copy_texture(device_t device, texture_t dst, texture_t src)
{
	device_copy_texture_region(device, dst, 0, 0, src, 0, 0, 0, 0);
}

void device_beginscene(device_t device)
{
	clear_textures(device);
}

static inline bool can_render(device_t device)
{
	if (!device->cur_vertex_shader) {
		blog(LOG_ERROR, "No vertex shader specified");
		return false;
	}

	if (!device->cur_pixel_shader) {
		blog(LOG_ERROR, "No pixel shader specified");
		return false;
	}

	if (!device->cur_vertex_buffer) {
		blog(LOG_ERROR, "No vertex buffer specified");
		return false;
	}

	return true;
}

static void update_viewproj_matrix(struct gs_device *device)
{
	struct gs_shader *vs = device->cur_vertex_shader;
	gs_matrix_get(&device->cur_view);

	matrix4_mul(&device->cur_viewproj, &device->cur_view,
			&device->cur_proj);
	matrix4_transpose(&device->cur_viewproj, &device->cur_viewproj);

	if (vs->viewproj)
		shader_setmatrix4(vs, vs->viewproj, &device->cur_viewproj);
}

static inline bool check_shader_pipeline_validity(device_t device)
{
	int valid = false;

	glValidateProgramPipeline(device->pipeline);
	if (!gl_success("glValidateProgramPipeline"))
		return false;

	glGetProgramPipelineiv(device->pipeline, GL_VALIDATE_STATUS, &valid);
	if (!gl_success("glGetProgramPipelineiv"))
		return false;

	if (!valid)
		blog(LOG_ERROR, "Shader pipeline appears to be invalid");

	return valid != 0;
}

void device_draw(device_t device, enum gs_draw_mode draw_mode,
		uint32_t start_vert, uint32_t num_verts)
{
	struct gs_index_buffer *ib = device->cur_index_buffer;
	GLenum  topology = convert_gs_topology(draw_mode);
	effect_t effect = gs_geteffect();

	if (!can_render(device))
		goto fail;

	if (effect)
		effect_updateparams(effect);

	shader_update_textures(device->cur_pixel_shader);

	update_viewproj_matrix(device);


#ifdef _DEBUG
	if (!check_shader_pipeline_validity(device))
		goto fail;
#endif

	if (ib) {
		if (num_verts == 0)
			num_verts = (uint32_t)device->cur_index_buffer->num;
		glDrawElements(topology, num_verts, ib->gl_type,
				(const GLvoid*)(start_vert * ib->width));
		if (!gl_success("glDrawElements"))
			goto fail;

	} else {
		if (num_verts == 0)
			num_verts = (uint32_t)device->cur_vertex_buffer->num;
		glDrawArrays(topology, start_vert, num_verts);
		if (!gl_success("glDrawArrays"))
			goto fail;
	}

	return;

fail:
	blog(LOG_ERROR, "device_draw (GL) failed");
}

void device_endscene(device_t device)
{
	/* does nothing */
	UNUSED_PARAMETER(device);
}

void device_clear(device_t device, uint32_t clear_flags,
		struct vec4 *color, float depth, uint8_t stencil)
{
	GLbitfield gl_flags = 0;

	if (clear_flags & GS_CLEAR_COLOR) {
		glClearColor(color->x, color->y, color->z, color->w);
		gl_flags |= GL_COLOR_BUFFER_BIT;
	}

	if (clear_flags & GS_CLEAR_DEPTH) {
		glClearDepth(depth);
		gl_flags |= GL_DEPTH_BUFFER_BIT;
	}

	if (clear_flags & GS_CLEAR_STENCIL) {
		glClearStencil(stencil);
		gl_flags |= GL_STENCIL_BUFFER_BIT;
	}

	glClear(gl_flags);
	if (!gl_success("glClear"))
		blog(LOG_ERROR, "device_clear (GL) failed");

	UNUSED_PARAMETER(device);
}

void device_setcullmode(device_t device, enum gs_cull_mode mode)
{
	if (device->cur_cull_mode == mode)
		return;

	if (device->cur_cull_mode == GS_NEITHER)
		gl_enable(GL_CULL_FACE);

	device->cur_cull_mode = mode;

	if (mode == GS_BACK)
		gl_cull_face(GL_BACK);
	else if (mode == GS_FRONT)
		gl_cull_face(GL_FRONT);
	else
		gl_disable(GL_CULL_FACE);
}

enum gs_cull_mode device_getcullmode(device_t device)
{
	return device->cur_cull_mode;
}

void device_enable_blending(device_t device, bool enable)
{
	if (enable)
		gl_enable(GL_BLEND);
	else
		gl_disable(GL_BLEND);

	UNUSED_PARAMETER(device);
}

void device_enable_depthtest(device_t device, bool enable)
{
	if (enable)
		gl_enable(GL_DEPTH_TEST);
	else
		gl_disable(GL_DEPTH_TEST);

	UNUSED_PARAMETER(device);
}

void device_enable_stenciltest(device_t device, bool enable)
{
	if (enable)
		gl_enable(GL_STENCIL_TEST);
	else
		gl_disable(GL_STENCIL_TEST);

	UNUSED_PARAMETER(device);
}

void device_enable_stencilwrite(device_t device, bool enable)
{
	if (enable)
		glStencilMask(0xFFFFFFFF);
	else
		glStencilMask(0);

	UNUSED_PARAMETER(device);
}

void device_enable_color(device_t device, bool red, bool green,
		bool blue, bool alpha)
{
	glColorMask(red, green, blue, alpha);

	UNUSED_PARAMETER(device);
}

void device_blendfunction(device_t device, enum gs_blend_type src,
		enum gs_blend_type dest)
{
	GLenum gl_src = convert_gs_blend_type(src);
	GLenum gl_dst = convert_gs_blend_type(dest);

	glBlendFunc(gl_src, gl_dst);
	if (!gl_success("glBlendFunc"))
		blog(LOG_ERROR, "device_blendfunction (GL) failed");

	UNUSED_PARAMETER(device);
}

void device_depthfunction(device_t device, enum gs_depth_test test)
{
	GLenum gl_test = convert_gs_depth_test(test);

	glDepthFunc(gl_test);
	if (!gl_success("glDepthFunc"))
		blog(LOG_ERROR, "device_depthfunction (GL) failed");

	UNUSED_PARAMETER(device);
}

void device_stencilfunction(device_t device, enum gs_stencil_side side,
		enum gs_depth_test test)
{
	GLenum gl_side = convert_gs_stencil_side(side);
	GLenum gl_test = convert_gs_depth_test(test);

	glStencilFuncSeparate(gl_side, gl_test, 0, 0xFFFFFFFF);
	if (!gl_success("glStencilFuncSeparate"))
		blog(LOG_ERROR, "device_stencilfunction (GL) failed");

	UNUSED_PARAMETER(device);
}

void device_stencilop(device_t device, enum gs_stencil_side side,
		enum gs_stencil_op fail, enum gs_stencil_op zfail,
		enum gs_stencil_op zpass)
{
	GLenum gl_side  = convert_gs_stencil_side(side);
	GLenum gl_fail  = convert_gs_stencil_op(fail);
	GLenum gl_zfail = convert_gs_stencil_op(zfail);
	GLenum gl_zpass = convert_gs_stencil_op(zpass);

	glStencilOpSeparate(gl_side, gl_fail, gl_zfail, gl_zpass);
	if (!gl_success("glStencilOpSeparate"))
		blog(LOG_ERROR, "device_stencilop (GL) failed");

	UNUSED_PARAMETER(device);
}

void device_enable_fullscreen(device_t device, bool enable)
{
	/* TODO */
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(enable);
}

int device_fullscreen_enabled(device_t device)
{
	/* TODO */
	UNUSED_PARAMETER(device);
	return false;
}

void device_setdisplaymode(device_t device,
		const struct gs_display_mode *mode)
{
	/* TODO */
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(mode);
}

void device_getdisplaymode(device_t device,
		struct gs_display_mode *mode)
{
	/* TODO */
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(mode);
}

void device_setcolorramp(device_t device, float gamma, float brightness,
		float contrast)
{
	/* TODO */
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(gamma);
	UNUSED_PARAMETER(brightness);
	UNUSED_PARAMETER(contrast);
}

static inline uint32_t get_target_height(struct gs_device *device)
{
	if (!device->cur_render_target)
		return device_getheight(device);

	if (device->cur_render_target->type == GS_TEXTURE_2D)
		return texture_getheight(device->cur_render_target);
	else /* cube map */
		return cubetexture_getsize(device->cur_render_target);
}

void device_setviewport(device_t device, int x, int y, int width,
		int height)
{
	uint32_t base_height;

	/* GL uses bottom-up coordinates for viewports.  We want top-down */
	if (device->cur_render_target) {
		base_height = get_target_height(device);
	} else {
		uint32_t dw;
		gl_getclientsize(device->cur_swap, &dw, &base_height);
	}

	glViewport(x, base_height - y - height, width, height);
	if (!gl_success("glViewport"))
		blog(LOG_ERROR, "device_setviewport (GL) failed");

	device->cur_viewport.x  = x;
	device->cur_viewport.y  = y;
	device->cur_viewport.cx = width;
	device->cur_viewport.cy = height;
}

void device_getviewport(device_t device, struct gs_rect *rect)
{
	*rect = device->cur_viewport;
}

void device_setscissorrect(device_t device, struct gs_rect *rect)
{
	UNUSED_PARAMETER(device);

	if (rect != NULL) {
		glScissor(rect->x, rect->y, rect->cx, rect->cy);
		if (gl_success("glScissor") && gl_enable(GL_SCISSOR_TEST))
			return;

	} else if (gl_disable(GL_SCISSOR_TEST)) {
		return;
	}

	blog(LOG_ERROR, "device_setscissorrect (GL) failed");
}

void device_ortho(device_t device, float left, float right,
		float top, float bottom, float near, float far)
{
	struct matrix4 *dst = &device->cur_proj;

	float rml = right-left;
	float bmt = bottom-top;
	float fmn = far-near;

	vec4_zero(&dst->x);
	vec4_zero(&dst->y);
	vec4_zero(&dst->z);
	vec4_zero(&dst->t);

	dst->x.x =         2.0f /  rml;
	dst->t.x = (left+right) / -rml;

	dst->y.y =         2.0f / -bmt;
	dst->t.y = (bottom+top) /  bmt;

	dst->z.z =        -2.0f /  fmn;
	dst->t.z =   (far+near) / -fmn;

	dst->t.w = 1.0f;
}

void device_frustum(device_t device, float left, float right,
		float top, float bottom, float near, float far)
{
	struct matrix4 *dst = &device->cur_proj;

	float rml    = right-left;
	float tmb    = top-bottom;
	float nmf    = near-far;
	float nearx2 = 2.0f*near;

	vec4_zero(&dst->x);
	vec4_zero(&dst->y);
	vec4_zero(&dst->z);
	vec4_zero(&dst->t);

	dst->x.x =            nearx2 / rml;
	dst->z.x =      (left+right) / rml;
					   
	dst->y.y =            nearx2 / tmb;
	dst->z.y =      (bottom+top) / tmb;

	dst->z.z =        (far+near) / nmf;
	dst->t.z = 2.0f * (near*far) / nmf;

	dst->z.w = -1.0f;
}

void device_projection_push(device_t device)
{
	da_push_back(device->proj_stack, &device->cur_proj);
}

void device_projection_pop(device_t device)
{
	struct matrix4 *end;
	if (!device->proj_stack.num)
		return;

	end = da_end(device->proj_stack);
	device->cur_proj = *end;
	da_pop_back(device->proj_stack);
}

void swapchain_destroy(swapchain_t swapchain)
{
	if (!swapchain)
		return;

	if (swapchain->device->cur_swap == swapchain)
		device_load_swapchain(swapchain->device, NULL);

	gl_platform_cleanup_swapchain(swapchain);

	gl_windowinfo_destroy(swapchain->wi);
	bfree(swapchain);
}

void volumetexture_destroy(texture_t voltex)
{
	/* TODO */
	UNUSED_PARAMETER(voltex);
}

uint32_t volumetexture_getwidth(texture_t voltex)
{
	/* TODO */
	UNUSED_PARAMETER(voltex);
	return 0;
}

uint32_t volumetexture_getheight(texture_t voltex)
{
	/* TODO */
	UNUSED_PARAMETER(voltex);
	return 0;
}

uint32_t volumetexture_getdepth(texture_t voltex)
{
	/* TODO */
	UNUSED_PARAMETER(voltex);
	return 0;
}

enum gs_color_format volumetexture_getcolorformat(texture_t voltex)
{
	/* TODO */
	UNUSED_PARAMETER(voltex);
	return GS_UNKNOWN;
}

void samplerstate_destroy(samplerstate_t samplerstate)
{
	if (!samplerstate)
		return;

	if (samplerstate->device)
		for (int i = 0; i < GS_MAX_TEXTURES; i++)
			if (samplerstate->device->cur_samplers[i] ==
					samplerstate)
				samplerstate->device->cur_samplers[i] = NULL;

	samplerstate_release(samplerstate);
}
