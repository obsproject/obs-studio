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

#include "../util/bmem.h"
#include "input.h"
#ifdef __APPLE__
#include <objc/objc-runtime.h>
#endif

/*
 * This is an API-independent graphics subsystem wrapper.
 *
 *   This allows the use of OpenGL and different Direct3D versions through
 * one shared interface.
 */

#ifdef __cplusplus
extern "C" {
#endif

#define GS_MAX_TEXTURES 8

struct vec2;
struct vec3;
struct vec4;
struct quat;
struct axisang;
struct plane;
struct matrix3;
struct matrix4;

enum gs_draw_mode {
	GS_POINTS,
	GS_LINES,
	GS_LINESTRIP,
	GS_TRIS,
	GS_TRISTRIP
};

enum gs_color_format {
	GS_UNKNOWN,
	GS_A8,
	GS_R8,
	GS_RGBA,
	GS_BGRX,
	GS_BGRA,
	GS_R10G10B10A2,
	GS_RGBA16,
	GS_R16,
	GS_RGBA16F,
	GS_RGBA32F,
	GS_RG16F,
	GS_RG32F,
	GS_R16F,
	GS_R32F,
	GS_DXT1,
	GS_DXT3,
	GS_DXT5
};

enum gs_zstencil_format {
	GS_ZS_NONE,
	GS_Z16,
	GS_Z24_S8,
	GS_Z32F,
	GS_Z32F_S8X24
};

enum gs_index_type {
	GS_UNSIGNED_SHORT,
	GS_UNSIGNED_LONG
};

enum gs_cull_mode {
	GS_BACK,
	GS_FRONT,
	GS_NEITHER
};

enum gs_blend_type {
	GS_BLEND_ZERO,
	GS_BLEND_ONE,
	GS_BLEND_SRCCOLOR,
	GS_BLEND_INVSRCCOLOR,
	GS_BLEND_SRCALPHA,
	GS_BLEND_INVSRCALPHA,
	GS_BLEND_DSTCOLOR,
	GS_BLEND_INVDSTCOLOR,
	GS_BLEND_DSTALPHA,
	GS_BLEND_INVDSTALPHA,
	GS_BLEND_SRCALPHASAT
};

enum gs_depth_test {
	GS_NEVER,
	GS_LESS,
	GS_LEQUAL,
	GS_EQUAL,
	GS_GEQUAL,
	GS_GREATER,
	GS_NOTEQUAL,
	GS_ALWAYS
};

enum gs_stencil_side {
	GS_STENCIL_FRONT=1,
	GS_STENCIL_BACK,
	GS_STENCIL_BOTH
};

enum gs_stencil_op {
	GS_KEEP,
	GS_ZERO,
	GS_REPLACE,
	GS_INCR,
	GS_DECR,
	GS_INVERT
};

enum gs_cube_sides {
	GS_POSITIVE_X,
	GS_NEGATIVE_X,
	GS_POSITIVE_Y,
	GS_NEGATIVE_Y,
	GS_POSITIVE_Z,
	GS_NEGATIVE_Z
};

enum gs_sample_filter {
	GS_FILTER_POINT,
	GS_FILTER_LINEAR,
	GS_FILTER_ANISOTROPIC,
	GS_FILTER_MIN_MAG_POINT_MIP_LINEAR,
	GS_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT,
	GS_FILTER_MIN_POINT_MAG_MIP_LINEAR,
	GS_FILTER_MIN_LINEAR_MAG_MIP_POINT,
	GS_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
	GS_FILTER_MIN_MAG_LINEAR_MIP_POINT,
};

enum gs_address_mode {
	GS_ADDRESS_CLAMP,
	GS_ADDRESS_WRAP,
	GS_ADDRESS_MIRROR,
	GS_ADDRESS_BORDER,
	GS_ADDRESS_MIRRORONCE
};

enum gs_texture_type {
	GS_TEXTURE_2D,
	GS_TEXTURE_3D,
	GS_TEXTURE_CUBE
};

struct tvertarray {
	size_t width;
	void *array;
};

struct vb_data {
	size_t num;
	struct vec3 *points;
	struct vec3 *normals;
	struct vec3 *tangents;
	uint32_t *colors;

	size_t num_tex;
	struct tvertarray *tvarray;
};

static inline struct vb_data *vbdata_create(void)
{
	return (struct vb_data*)bzalloc(sizeof(struct vb_data));
}

static inline void vbdata_destroy(struct vb_data *data)
{
	uint32_t i;
	if (!data)
		return;

	bfree(data->points);
	bfree(data->normals);
	bfree(data->tangents);
	bfree(data->colors);
	for (i = 0; i < data->num_tex; i++)
		bfree(data->tvarray[i].array);
	bfree(data->tvarray);
	bfree(data);
}

struct gs_sampler_info {
	enum gs_sample_filter filter;
	enum gs_address_mode address_u;
	enum gs_address_mode address_v;
	enum gs_address_mode address_w;
	int max_anisotropy;
	uint32_t border_color;
};

struct gs_display_mode {
	uint32_t width;
	uint32_t height;
	uint32_t bits;
	uint32_t freq;
};

struct gs_rect {
	int x;
	int y;
	int cx;
	int cy;
};

/* wrapped opaque data types */

struct gs_texture;
struct gs_stage_surface;
struct gs_zstencil_buffer;
struct gs_vertex_buffer;
struct gs_index_buffer;
struct gs_sampler_state;
struct gs_shader;
struct gs_swap_chain;
struct gs_texrender;
struct shader_param;
struct gs_effect;
struct effect_technique;
struct effect_pass;
struct effect_param;
struct gs_device;
struct graphics_subsystem;

typedef struct gs_texture         *texture_t;
typedef struct gs_stage_surface   *stagesurf_t;
typedef struct gs_zstencil_buffer *zstencil_t;
typedef struct gs_vertex_buffer   *vertbuffer_t;
typedef struct gs_index_buffer    *indexbuffer_t;
typedef struct gs_sampler_state   *samplerstate_t;
typedef struct gs_swap_chain      *swapchain_t;
typedef struct gs_texture_render  *texrender_t;
typedef struct gs_shader          *shader_t;
typedef struct shader_param       *sparam_t;
typedef struct gs_effect          *effect_t;
typedef struct effect_technique   *technique_t;
typedef struct effect_param       *eparam_t;
typedef struct gs_device          *device_t;
typedef struct graphics_subsystem *graphics_t;

/* ---------------------------------------------------
 * shader functions
 * --------------------------------------------------- */

enum shader_param_type {
	SHADER_PARAM_UNKNOWN,
	SHADER_PARAM_BOOL,
	SHADER_PARAM_FLOAT,
	SHADER_PARAM_INT,
	SHADER_PARAM_STRING,
	SHADER_PARAM_VEC2,
	SHADER_PARAM_VEC3,
	SHADER_PARAM_VEC4,
	SHADER_PARAM_MATRIX4X4,
	SHADER_PARAM_TEXTURE,
};

struct shader_param_info {
	enum shader_param_type type;
	const char *name;
};

enum shader_type {
	SHADER_VERTEX,
	SHADER_PIXEL,
};

EXPORT void shader_destroy(shader_t shader);

EXPORT int shader_numparams(shader_t shader);
EXPORT sparam_t shader_getparambyidx(shader_t shader, uint32_t param);
EXPORT sparam_t shader_getparambyname(shader_t shader, const char *name);
EXPORT void shader_getparaminfo(shader_t shader, sparam_t param,
		struct shader_param_info *info);

EXPORT sparam_t shader_getviewprojmatrix(shader_t shader);
EXPORT sparam_t shader_getworldmatrix(shader_t shader);

EXPORT void shader_setbool(shader_t shader, sparam_t param, bool val);
EXPORT void shader_setfloat(shader_t shader, sparam_t param, float val);
EXPORT void shader_setint(shader_t shader, sparam_t param, int val);
EXPORT void shader_setmatrix3(shader_t shader, sparam_t param,
		const struct matrix3 *val);
EXPORT void shader_setmatrix4(shader_t shader, sparam_t param,
		const struct matrix4 *val);
EXPORT void shader_setvec2(shader_t shader, sparam_t param,
		const struct vec2 *val);
EXPORT void shader_setvec3(shader_t shader, sparam_t param,
		const struct vec3 *val);
EXPORT void shader_setvec4(shader_t shader, sparam_t param,
		const struct vec4 *val);
EXPORT void shader_settexture(shader_t shader, sparam_t param, texture_t val);
EXPORT void shader_setval(shader_t shader, sparam_t param, const void *val,
		size_t size);
EXPORT void shader_setdefault(shader_t shader, sparam_t param);

/* ---------------------------------------------------
 * effect functions
 * --------------------------------------------------- */

/*enum effect_property_type {
	EFFECT_NONE,
	EFFECT_BOOL,
	EFFECT_FLOAT,
	EFFECT_COLOR,
	EFFECT_TEXTURE
};*/

struct effect_param_info {
	const char *name;
	enum shader_param_type type;

	/* const char *full_name;
	enum effect_property_type prop_type;

	float min, max, inc, mul; */
};

EXPORT void effect_destroy(effect_t effect);

EXPORT technique_t effect_gettechnique(effect_t effect, const char *name);

EXPORT size_t technique_begin(technique_t technique);
EXPORT void technique_end(technique_t technique);
EXPORT bool technique_beginpass(technique_t technique, size_t pass);
EXPORT bool technique_beginpassbyname(technique_t technique,
		const char *name);
EXPORT void technique_endpass(technique_t technique);

EXPORT size_t effect_numparams(effect_t effect);
EXPORT eparam_t effect_getparambyidx(effect_t effect, size_t param);
EXPORT eparam_t effect_getparambyname(effect_t effect, const char *name);
EXPORT void effect_getparaminfo(effect_t effect, eparam_t param,
		struct effect_param_info *info);

/** used internally */
EXPORT void effect_updateparams(effect_t effect);

EXPORT eparam_t effect_getviewprojmatrix(effect_t effect);
EXPORT eparam_t effect_getworldmatrix(effect_t effect);

EXPORT void effect_setbool(effect_t effect, eparam_t param, bool val);
EXPORT void effect_setfloat(effect_t effect, eparam_t param, float val);
EXPORT void effect_setint(effect_t effect, eparam_t param, int val);
EXPORT void effect_setmatrix4(effect_t effect, eparam_t param,
		const struct matrix4 *val);
EXPORT void effect_setvec2(effect_t effect, eparam_t param,
		const struct vec2 *val);
EXPORT void effect_setvec3(effect_t effect, eparam_t param,
		const struct vec3 *val);
EXPORT void effect_setvec4(effect_t effect, eparam_t param,
		const struct vec4 *val);
EXPORT void effect_settexture(effect_t effect, eparam_t param, texture_t val);
EXPORT void effect_setval(effect_t effect, eparam_t param, const void *val,
		size_t size);
EXPORT void effect_setdefault(effect_t effect, eparam_t param);

/* ---------------------------------------------------
 * texture render helper functions
 * --------------------------------------------------- */

EXPORT texrender_t texrender_create(enum gs_color_format format,
		enum gs_zstencil_format zsformat);
EXPORT void texrender_destroy(texrender_t texrender);
EXPORT bool texrender_begin(texrender_t texrender, uint32_t cx, uint32_t cy);
EXPORT void texrender_end(texrender_t texrender);
EXPORT void texrender_reset(texrender_t texrender);
EXPORT texture_t texrender_gettexture(texrender_t texrender);

/* ---------------------------------------------------
 * graphics subsystem
 * --------------------------------------------------- */

#define GS_BUILDMIPMAPS (1<<0)
#define GS_DYNAMIC      (1<<1)
#define GS_RENDERTARGET (1<<2)
#define GS_GL_DUMMYTEX  (1<<3) /**<< texture with no allocated texture data */

/* ---------------- */
/* global functions */

#define GS_SUCCESS               0
#define GS_ERROR_MODULENOTFOUND -1
#define GS_ERROR_FAIL           -2

struct gs_window {
#if defined(_WIN32)
	void                    *hwnd;
#elif defined(__APPLE__)
	__unsafe_unretained id  view;
#elif defined(__linux__)
	/* I'm not sure how portable defining id to uint32_t is. */
	uint32_t id;
	void* display;
#endif
};

struct gs_init_data {
	struct gs_window        window;
	uint32_t                cx, cy;
	uint32_t                num_backbuffers;
	enum gs_color_format    format;
	enum gs_zstencil_format zsformat;
	uint32_t                adapter;
};

EXPORT int gs_create(graphics_t *graphics, const char *module,
		struct gs_init_data *data);
EXPORT void gs_destroy(graphics_t graphics);

EXPORT void gs_entercontext(graphics_t graphics);
EXPORT void gs_leavecontext(void);
EXPORT graphics_t gs_getcontext(void);

EXPORT void gs_matrix_push(void);
EXPORT void gs_matrix_pop(void);
EXPORT void gs_matrix_identity(void);
EXPORT void gs_matrix_transpose(void);
EXPORT void gs_matrix_set(const struct matrix4 *matrix);
EXPORT void gs_matrix_get(struct matrix4 *dst);
EXPORT void gs_matrix_mul(const struct matrix4 *matrix);
EXPORT void gs_matrix_rotquat(const struct quat *rot);
EXPORT void gs_matrix_rotaa(const struct axisang *rot);
EXPORT void gs_matrix_translate(const struct vec3 *pos);
EXPORT void gs_matrix_scale(const struct vec3 *scale);
EXPORT void gs_matrix_rotaa4f(float x, float y, float z, float angle);
EXPORT void gs_matrix_translate3f(float x, float y, float z);
EXPORT void gs_matrix_scale3f(float x, float y, float z);

EXPORT void gs_renderstart(bool b_new);
EXPORT void gs_renderstop(enum gs_draw_mode mode);
EXPORT vertbuffer_t gs_rendersave(void);
EXPORT void gs_vertex2f(float x, float y);
EXPORT void gs_vertex3f(float x, float y, float z);
EXPORT void gs_normal3f(float x, float y, float z);
EXPORT void gs_color(uint32_t color);
EXPORT void gs_texcoord(float x, float y, int unit);
EXPORT void gs_vertex2v(const struct vec2 *v);
EXPORT void gs_vertex3v(const struct vec3 *v);
EXPORT void gs_normal3v(const struct vec3 *v);
EXPORT void gs_color4v(const struct vec4 *v);
EXPORT void gs_texcoord2v(const struct vec2 *v, int unit);

EXPORT input_t gs_getinput(void);
EXPORT effect_t gs_geteffect(void);

EXPORT effect_t gs_create_effect_from_file(const char *file,
		char **error_string);
EXPORT effect_t gs_create_effect(const char *effect_string,
		const char *filename, char **error_string);

EXPORT shader_t gs_create_vertexshader_from_file(const char *file,
		char **error_string);
EXPORT shader_t gs_create_pixelshader_from_file(const char *file,
		char **error_string);

EXPORT texture_t gs_create_texture_from_file(const char *file,
		uint32_t flags);
EXPORT texture_t gs_create_cubetexture_from_file(const char *flie,
		uint32_t flags);
EXPORT texture_t gs_create_volumetexture_from_file(const char *flie,
		uint32_t flags);

#define GS_FLIP_U (1<<0)
#define GS_FLIP_V (1<<1)

/**
 * Draws a 2D sprite
 *
 *   If width or height is 0, the width or height of the texture will be used.
 * The flip value specifies whether the texture shoudl be flipped on the U or V
 * axis with GS_FLIP_U and GS_FLIP_V.
 */
EXPORT void gs_draw_sprite(texture_t tex, uint32_t flip, uint32_t width,
		uint32_t height);

EXPORT void gs_draw_cube_backdrop(texture_t cubetex, const struct quat *rot,
		float left, float right, float top, float bottom, float znear);

/** sets the viewport to current swap chain size */
EXPORT void gs_resetviewport(void);

/** sets default screen-sized orthographich mode */
EXPORT void gs_set2dmode(void);
/** sets default screen-sized perspective mode */
EXPORT void gs_set3dmode(double fovy, double znear, double zvar);

EXPORT void gs_viewport_push(void);
EXPORT void gs_viewport_pop(void);

EXPORT void texture_setimage(texture_t tex, const void *data,
		uint32_t linesize, bool invert);
EXPORT void cubetexture_setimage(texture_t cubetex, uint32_t side,
		const void *data, uint32_t linesize, bool invert);

EXPORT void gs_perspective(float fovy, float aspect, float znear, float zfar);

/* -------------------------- */
/* library-specific functions */

EXPORT swapchain_t gs_create_swapchain(struct gs_init_data *data);

EXPORT void gs_resize(uint32_t x, uint32_t y);
EXPORT void gs_getsize(uint32_t *x, uint32_t *y);
EXPORT uint32_t gs_getwidth(void);
EXPORT uint32_t gs_getheight(void);

EXPORT texture_t gs_create_texture(uint32_t width, uint32_t height,
		enum gs_color_format color_format, uint32_t levels,
		const void **data, uint32_t flags);
EXPORT texture_t gs_create_cubetexture(uint32_t size,
		enum gs_color_format color_format, uint32_t levels,
		const void **data, uint32_t flags);
EXPORT texture_t gs_create_volumetexture(uint32_t width, uint32_t height,
		uint32_t depth, enum gs_color_format color_format,
		uint32_t levels, const void **data, uint32_t flags);

EXPORT zstencil_t gs_create_zstencil(uint32_t width, uint32_t height,
		enum gs_zstencil_format format);

EXPORT stagesurf_t gs_create_stagesurface(uint32_t width, uint32_t height,
		enum gs_color_format color_format);

EXPORT samplerstate_t gs_create_samplerstate(struct gs_sampler_info *info);

EXPORT shader_t gs_create_vertexshader(const char *shader,
		const char *file, char **error_string);
EXPORT shader_t gs_create_pixelshader(const char *shader,
		const char *file, char **error_string);

EXPORT vertbuffer_t gs_create_vertexbuffer(struct vb_data *data,
		uint32_t flags);
EXPORT indexbuffer_t gs_create_indexbuffer(enum gs_index_type type,
		void *indices, size_t num, uint32_t flags);

EXPORT enum gs_texture_type gs_gettexturetype(texture_t texture);

EXPORT void gs_load_vertexbuffer(vertbuffer_t vertbuffer);
EXPORT void gs_load_indexbuffer(indexbuffer_t indexbuffer);
EXPORT void gs_load_texture(texture_t tex, int unit);
EXPORT void gs_load_samplerstate(samplerstate_t samplerstate, int unit);
EXPORT void gs_load_vertexshader(shader_t vertshader);
EXPORT void gs_load_pixelshader(shader_t pixelshader);

EXPORT void gs_load_defaultsamplerstate(bool b_3d, int unit);

EXPORT shader_t gs_getvertexshader(void);
EXPORT shader_t gs_getpixelshader(void);

EXPORT texture_t  gs_getrendertarget(void);
EXPORT zstencil_t gs_getzstenciltarget(void);

EXPORT void gs_setrendertarget(texture_t tex, zstencil_t zstencil);
EXPORT void gs_setcuberendertarget(texture_t cubetex, int side,
		zstencil_t zstencil);

EXPORT void gs_copy_texture(texture_t dst, texture_t src);
EXPORT void gs_copy_texture_region(
		texture_t dst, uint32_t dst_x, uint32_t dst_y,
		texture_t src, uint32_t src_x, uint32_t src_y,
		uint32_t src_w, uint32_t src_h);
EXPORT void gs_stage_texture(stagesurf_t dst, texture_t src);

EXPORT void gs_beginscene(void);
EXPORT void gs_draw(enum gs_draw_mode draw_mode, uint32_t start_vert,
		uint32_t num_verts);
EXPORT void gs_endscene(void);

#define GS_CLEAR_COLOR   (1<<0)
#define GS_CLEAR_DEPTH   (1<<1)
#define GS_CLEAR_STENCIL (1<<2)

EXPORT void gs_load_swapchain(swapchain_t swapchain);
EXPORT void gs_clear(uint32_t clear_flags, struct vec4 *color,
		float depth, uint8_t stencil);
EXPORT void gs_present(void);

EXPORT void gs_setcullmode(enum gs_cull_mode mode);
EXPORT enum gs_cull_mode gs_getcullmode(void);

EXPORT void gs_enable_blending(bool enable);
EXPORT void gs_enable_depthtest(bool enable);
EXPORT void gs_enable_stenciltest(bool enable);
EXPORT void gs_enable_stencilwrite(bool enable);
EXPORT void gs_enable_color(bool red, bool green, bool blue, bool alpha);

EXPORT void gs_blendfunction(enum gs_blend_type src, enum gs_blend_type dest);
EXPORT void gs_depthfunction(enum gs_depth_test test);

EXPORT void gs_stencilfunction(enum gs_stencil_side side,
		enum gs_depth_test test);
EXPORT void gs_stencilop(enum gs_stencil_side side, enum gs_stencil_op fail,
		enum gs_stencil_op zfail, enum gs_stencil_op zpass);

EXPORT void gs_setclip(struct plane *p);

EXPORT void gs_enable_fullscreen(bool enable);
EXPORT int gs_fullscreen_enabled(void);
EXPORT void gs_setdisplaymode(const struct gs_display_mode *mode);
EXPORT void gs_getdisplaymode(struct gs_display_mode *mode);
EXPORT void gs_setcolorramp(float gamma, float brightness, float contrast);

EXPORT void gs_setviewport(int x, int y, int width, int height);
EXPORT void gs_getviewport(struct gs_rect *rect);
EXPORT void gs_setscissorrect(struct gs_rect *rect);

EXPORT void gs_ortho(float left, float right, float top, float bottom,
		float znear, float zfar);
EXPORT void gs_frustum(float left, float right, float top, float bottom,
		float znear, float zfar);

EXPORT void gs_projection_push(void);
EXPORT void gs_projection_pop(void);

EXPORT void     swapchain_destroy(swapchain_t swapchain);

EXPORT void     texture_destroy(texture_t tex);
EXPORT uint32_t texture_getwidth(texture_t tex);
EXPORT uint32_t texture_getheight(texture_t tex);
EXPORT enum gs_color_format texture_getcolorformat(texture_t tex);
EXPORT bool     texture_map(texture_t tex, void **ptr, uint32_t *linesize);
EXPORT void     texture_unmap(texture_t tex);
/** special-case function (GL only) - specifies whether the texture is a
 * GL_TEXTURE_RECTANGLE type, which doesn't use normalized texture
 * coordinates, doesn't support mipmapping, and requires address clamping */
EXPORT bool     texture_isrect(texture_t tex);
/**
 * Gets a pointer to the context-specific object associated with the texture.
 * For example, for GL, this is a GLuint*.  For D3D11, ID3D11Texture2D*.
 */
EXPORT void    *texture_getobj(texture_t tex);

EXPORT void     cubetexture_destroy(texture_t cubetex);
EXPORT uint32_t cubetexture_getsize(texture_t cubetex);
EXPORT enum gs_color_format cubetexture_getcolorformat(texture_t cubetex);

EXPORT void     volumetexture_destroy(texture_t voltex);
EXPORT uint32_t volumetexture_getwidth(texture_t voltex);
EXPORT uint32_t volumetexture_getheight(texture_t voltex);
EXPORT uint32_t volumetexture_getdepth(texture_t voltex);
EXPORT enum gs_color_format volumetexture_getcolorformat(texture_t voltex);

EXPORT void     stagesurface_destroy(stagesurf_t stagesurf);
EXPORT uint32_t stagesurface_getwidth(stagesurf_t stagesurf);
EXPORT uint32_t stagesurface_getheight(stagesurf_t stagesurf);
EXPORT enum gs_color_format stagesurface_getcolorformat(stagesurf_t stagesurf);
EXPORT bool     stagesurface_map(stagesurf_t stagesurf, uint8_t **data,
		uint32_t *linesize);
EXPORT void     stagesurface_unmap(stagesurf_t stagesurf);

EXPORT void     zstencil_destroy(zstencil_t zstencil);

EXPORT void     samplerstate_destroy(samplerstate_t samplerstate);

EXPORT void     vertexbuffer_destroy(vertbuffer_t vertbuffer);
EXPORT void     vertexbuffer_flush(vertbuffer_t vertbuffer, bool rebuild);
EXPORT struct vb_data *vertexbuffer_getdata(vertbuffer_t vertbuffer);

EXPORT void     indexbuffer_destroy(indexbuffer_t indexbuffer);
EXPORT void     indexbuffer_flush(indexbuffer_t indexbuffer);
EXPORT void     *indexbuffer_getdata(indexbuffer_t indexbuffer);
EXPORT size_t   indexbuffer_numindices(indexbuffer_t indexbuffer);
EXPORT enum gs_index_type indexbuffer_gettype(indexbuffer_t indexbuffer);

#ifdef __APPLE__

/** platform specific function for creating (GL_TEXTURE_RECTANGLE) textures
 * from shared surface resources */
EXPORT texture_t gs_create_texture_from_iosurface(void *iosurf);
EXPORT bool     texture_rebind_iosurface(texture_t texture, void *iosurf);

#elif _WIN32

EXPORT bool gs_gdi_texture_available(void);

/** creates a windows GDI-lockable texture */
EXPORT texture_t gs_create_gdi_texture(uint32_t width, uint32_t height);

EXPORT void *texture_get_dc(texture_t gdi_tex);
EXPORT void texture_release_dc(texture_t gdi_tex);

#endif

/* inline functions used by modules */

static inline uint32_t gs_get_format_bpp(enum gs_color_format format)
{
	switch (format) {
	case GS_A8:          return 8;
	case GS_R8:          return 8;
	case GS_RGBA:        return 32;
	case GS_BGRX:        return 32;
	case GS_BGRA:        return 32;
	case GS_R10G10B10A2: return 32;
	case GS_RGBA16:      return 64;
	case GS_R16:         return 16;
	case GS_RGBA16F:     return 64;
	case GS_RGBA32F:     return 128;
	case GS_RG16F:       return 32;
	case GS_RG32F:       return 64;
	case GS_R16F:        return 16;
	case GS_R32F:        return 32;
	case GS_DXT1:        return 4;
	case GS_DXT3:        return 8;
	case GS_DXT5:        return 8;
	case GS_UNKNOWN:     return 0;
	}

	return 0;
}

static inline bool gs_is_compressed_format(enum gs_color_format format)
{
	return (format == GS_DXT1 || format == GS_DXT3 || format == GS_DXT5);
}

static inline uint32_t gs_num_total_levels(uint32_t width, uint32_t height)
{
	uint32_t size = width > height ? width : height;
	uint32_t num_levels = 0;

	while (size > 1) {
		size /= 2;
		num_levels++;
	}

	return num_levels;
}

#ifdef __cplusplus
}
#endif
