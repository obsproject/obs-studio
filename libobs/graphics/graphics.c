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

#include <assert.h>

#include "../util/base.h"
#include "../util/bmem.h"
#include "../util/platform.h"
#include "graphics-internal.h"
#include "vec2.h"
#include "vec3.h"
#include "quat.h"
#include "axisang.h"
#include "effect-parser.h"
#include "effect.h"

#ifdef _MSC_VER
static __declspec(thread) graphics_t thread_graphics = NULL;
#else /* assume GCC or that other compiler we dare not mention */
static __thread graphics_t thread_graphics = NULL;
#endif

#define IMMEDIATE_COUNT 512

bool load_graphics_imports(struct gs_exports *exports, void *module,
		const char *module_name);

static bool graphics_init_immediate_vb(struct graphics_subsystem *graphics)
{
	struct vb_data *vbd;

	vbd = vbdata_create();
	vbd->num     = IMMEDIATE_COUNT;
	vbd->points  = bmalloc(sizeof(struct vec3)*IMMEDIATE_COUNT);
	vbd->normals = bmalloc(sizeof(struct vec3)*IMMEDIATE_COUNT);
	vbd->colors  = bmalloc(sizeof(uint32_t)   *IMMEDIATE_COUNT);
	vbd->num_tex = 1;
	vbd->tvarray = bmalloc(sizeof(struct tvertarray));
	vbd->tvarray[0].width = 2;
	vbd->tvarray[0].array =
		bmalloc(sizeof(struct vec2) * IMMEDIATE_COUNT);

	graphics->immediate_vertbuffer = graphics->exports.
		device_create_vertexbuffer(graphics->device, vbd, GS_DYNAMIC);
	if (!graphics->immediate_vertbuffer)
		return false;

	return true;
}

static bool graphics_init_sprite_vb(struct graphics_subsystem *graphics)
{
	struct vb_data *vbd;

	vbd = vbdata_create();
	vbd->num     = 4;
	vbd->points  = bmalloc(sizeof(struct vec3) * 4);
	vbd->num_tex = 1;
	vbd->tvarray = bmalloc(sizeof(struct tvertarray));
	vbd->tvarray[0].width = 2;
	vbd->tvarray[0].array = bmalloc(sizeof(struct vec2) * 4);

	memset(vbd->points,           0, sizeof(struct vec3) * 4);
	memset(vbd->tvarray[0].array, 0, sizeof(struct vec2) * 4);

	graphics->sprite_buffer = graphics->exports.
		device_create_vertexbuffer(graphics->device, vbd, GS_DYNAMIC);
	if (!graphics->sprite_buffer)
		return false;

	return true;
}

static bool graphics_init(struct graphics_subsystem *graphics)
{
	struct matrix4 top_mat;

	matrix4_identity(&top_mat);
	da_push_back(graphics->matrix_stack, &top_mat);

	graphics->exports.device_entercontext(graphics->device);

	if (!graphics_init_immediate_vb(graphics))
		return false;
	if (!graphics_init_sprite_vb(graphics))
		return false;
	if (pthread_mutex_init(&graphics->mutex, NULL) != 0)
		return false;

	graphics->exports.device_leavecontext(graphics->device);

	return true;
}

int gs_create(graphics_t *pgraphics, const char *module,
		struct gs_init_data *data)
{
	int errcode = GS_ERROR_FAIL;

	graphics_t graphics = bzalloc(sizeof(struct graphics_subsystem));
	pthread_mutex_init_value(&graphics->mutex);

	if (!data->num_backbuffers)
		data->num_backbuffers = 1;

	graphics->module = os_dlopen(module);
	if (!graphics->module) {
		errcode = GS_ERROR_MODULENOTFOUND;
		goto error;
	}

	if (!load_graphics_imports(&graphics->exports, graphics->module,
	                           module))
		goto error;

	graphics->device = graphics->exports.device_create(data);
	if (!graphics->device)
		goto error;

	if (!graphics_init(graphics))
		goto error;

	*pgraphics = graphics;
	return GS_SUCCESS;

error:
	gs_destroy(graphics);
	return errcode;
}

void gs_destroy(graphics_t graphics)
{
	if (!graphics)
		return;

	while (thread_graphics)
		gs_leavecontext();

	if (graphics->device) {
		graphics->exports.device_entercontext(graphics->device);
		graphics->exports.vertexbuffer_destroy(graphics->sprite_buffer);
		graphics->exports.vertexbuffer_destroy(
				graphics->immediate_vertbuffer);
		graphics->exports.device_destroy(graphics->device);
	}

	pthread_mutex_destroy(&graphics->mutex);
	da_free(graphics->matrix_stack);
	da_free(graphics->viewport_stack);
	if (graphics->module)
		os_dlclose(graphics->module);
	bfree(graphics);
}

void gs_entercontext(graphics_t graphics)
{
	if (!graphics) return;

	bool is_current = thread_graphics == graphics;
	if (thread_graphics && !is_current) {
		while (thread_graphics)
			gs_leavecontext();
	}

	if (!is_current) {
		pthread_mutex_lock(&graphics->mutex);
		graphics->exports.device_entercontext(graphics->device);
		thread_graphics = graphics;
	}

	os_atomic_inc_long(&graphics->ref);
}

void gs_leavecontext(void)
{
	if (thread_graphics) {
		if (!os_atomic_dec_long(&thread_graphics->ref)) {
			graphics_t graphics = thread_graphics;

			graphics->exports.device_leavecontext(graphics->device);
			pthread_mutex_unlock(&graphics->mutex);
			thread_graphics = NULL;
		}
	}
}

graphics_t gs_getcontext(void)
{
	return thread_graphics;
}

static inline struct matrix4 *top_matrix(graphics_t graphics)
{
	return graphics ? 
		(graphics->matrix_stack.array + graphics->cur_matrix) : NULL;
}

void gs_matrix_push(void)
{
	graphics_t graphics = thread_graphics;
	if (!graphics)
		return;

	struct matrix4 mat, *top_mat = top_matrix(graphics);

	memcpy(&mat, top_mat, sizeof(struct matrix4));
	da_push_back(graphics->matrix_stack, &mat);
	graphics->cur_matrix++;
}

void gs_matrix_pop(void)
{
	graphics_t graphics = thread_graphics;
	if (!graphics)
		return;

	if (graphics->cur_matrix == 0) {
		blog(LOG_ERROR, "Tried to pop last matrix on stack");
		return;
	}

	da_erase(graphics->matrix_stack, graphics->cur_matrix);
	graphics->cur_matrix--;
}

void gs_matrix_identity(void)
{
	struct matrix4 *top_mat = top_matrix(thread_graphics);
	if (top_mat)
		matrix4_identity(top_mat);
}

void gs_matrix_transpose(void)
{
	struct matrix4 *top_mat = top_matrix(thread_graphics);
	if (top_mat)
		matrix4_transpose(top_mat, top_mat);
}

void gs_matrix_set(const struct matrix4 *matrix)
{
	struct matrix4 *top_mat = top_matrix(thread_graphics);
	if (top_mat)
		matrix4_copy(top_mat, matrix);
}

void gs_matrix_get(struct matrix4 *dst)
{
	struct matrix4 *top_mat = top_matrix(thread_graphics);
	if (top_mat)
		matrix4_copy(dst, top_mat);
}

void gs_matrix_mul(const struct matrix4 *matrix)
{
	struct matrix4 *top_mat = top_matrix(thread_graphics);
	if (top_mat)
		matrix4_mul(top_mat, top_mat, matrix);
}

void gs_matrix_rotquat(const struct quat *rot)
{
	struct matrix4 *top_mat = top_matrix(thread_graphics);
	if (top_mat)
		matrix4_rotate(top_mat, top_mat, rot);
}

void gs_matrix_rotaa(const struct axisang *rot)
{
	struct matrix4 *top_mat = top_matrix(thread_graphics);
	if (top_mat)
		matrix4_rotate_aa(top_mat, top_mat, rot);
}

void gs_matrix_translate(const struct vec3 *pos)
{
	struct matrix4 *top_mat = top_matrix(thread_graphics);
	if (top_mat)
		matrix4_translate3v(top_mat, top_mat, pos);
}

void gs_matrix_scale(const struct vec3 *scale)
{
	struct matrix4 *top_mat = top_matrix(thread_graphics);
	if (top_mat)
		matrix4_scale(top_mat, top_mat, scale);
}

void gs_matrix_rotaa4f(float x, float y, float z, float angle)
{
	struct matrix4 *top_mat = top_matrix(thread_graphics);
	struct axisang aa;

	if (top_mat) {
		axisang_set(&aa, x, y, z, angle);
		matrix4_rotate_aa(top_mat, top_mat, &aa);
	}
}

void gs_matrix_translate3f(float x, float y, float z)
{
	struct matrix4 *top_mat = top_matrix(thread_graphics);
	struct vec3 p;

	if (top_mat) {
		vec3_set(&p, x, y, z);
		matrix4_translate3v(top_mat, top_mat, &p);
	}
}

void gs_matrix_scale3f(float x, float y, float z)
{
	struct matrix4 *top_mat = top_matrix(thread_graphics);
	struct vec3 p;

	if (top_mat) {
		vec3_set(&p, x, y, z);
		matrix4_scale(top_mat, top_mat, &p);
	}
}

static inline void reset_immediate_arrays(graphics_t graphics)
{
	da_init(graphics->verts);
	da_init(graphics->norms);
	da_init(graphics->colors);
	for (size_t i = 0; i < 16; i++)
		da_init(graphics->texverts[i]);
}

void gs_renderstart(bool b_new)
{
	graphics_t graphics = thread_graphics;
	if (!graphics)
		return;

	graphics->using_immediate = !b_new;
	reset_immediate_arrays(graphics);

	if (b_new) {
		graphics->vbd = vbdata_create();
	} else {
		graphics->vbd = vertexbuffer_getdata(
				graphics->immediate_vertbuffer);
		memset(graphics->vbd->colors, 0xFF,
				sizeof(uint32_t) * IMMEDIATE_COUNT);

		graphics->verts.array       = graphics->vbd->points;
		graphics->norms.array       = graphics->vbd->normals;
		graphics->colors.array      = graphics->vbd->colors;
		graphics->texverts[0].array = graphics->vbd->tvarray[0].array;

		graphics->verts.capacity       = IMMEDIATE_COUNT;
		graphics->norms.capacity       = IMMEDIATE_COUNT;
		graphics->colors.capacity      = IMMEDIATE_COUNT;
		graphics->texverts[0].capacity = IMMEDIATE_COUNT;
	}
}

static inline size_t min_size(const size_t a, const size_t b)
{
	return (a < b) ? a : b;
}

void gs_renderstop(enum gs_draw_mode mode)
{
	graphics_t graphics = thread_graphics;
	size_t i, num;

	if (!graphics)
		return;

	num = graphics->verts.num;
	if (!num) {
		if (!graphics->using_immediate) {
			da_free(graphics->verts);
			da_free(graphics->norms);
			da_free(graphics->colors);
			for (i = 0; i < 16; i++)
				da_free(graphics->texverts[i]);
			vbdata_destroy(graphics->vbd);
		}

		return;
	}

	if (graphics->norms.num &&
	    (graphics->norms.num != graphics->verts.num)) {
		blog(LOG_ERROR, "gs_renderstop: normal count does "
		                "not match vertex count");
		num = min_size(num, graphics->norms.num);
	}

	if (graphics->colors.num &&
	    (graphics->colors.num != graphics->verts.num)) {
		blog(LOG_ERROR, "gs_renderstop: color count does "
		                "not match vertex count");
		num = min_size(num, graphics->colors.num);
	}

	if (graphics->texverts[0].num &&
	    (graphics->texverts[0].num  != graphics->verts.num)) {
		blog(LOG_ERROR, "gs_renderstop: texture vertex count does "
		                "not match vertex count");
		num = min_size(num, graphics->texverts[0].num);
	}

	if (graphics->using_immediate) {
		vertexbuffer_flush(graphics->immediate_vertbuffer, false);

		gs_load_vertexbuffer(graphics->immediate_vertbuffer);
		gs_load_indexbuffer(NULL);
		gs_draw(mode, 0, (uint32_t)num);

		reset_immediate_arrays(graphics);
	} else {
		vertbuffer_t vb = gs_rendersave();

		gs_load_vertexbuffer(vb);
		gs_load_indexbuffer(NULL);
		gs_draw(mode, 0, 0);

		vertexbuffer_destroy(vb);
	}

	graphics->vbd = NULL;
}

vertbuffer_t gs_rendersave(void)
{
	graphics_t graphics = thread_graphics;
	size_t num_tex, i;

	if (!graphics)
		return NULL;

	if (graphics->using_immediate)
		return NULL;

	if (!graphics->verts.num) {
		vbdata_destroy(graphics->vbd);
		return NULL;
	}

	for (num_tex = 0; num_tex < 16; num_tex++)
		if (!graphics->texverts[num_tex].num)
			break;

	graphics->vbd->points  = graphics->verts.array;
	graphics->vbd->normals = graphics->norms.array;
	graphics->vbd->colors  = graphics->colors.array;
	graphics->vbd->num     = graphics->verts.num;
	graphics->vbd->num_tex = num_tex;

	if (graphics->vbd->num_tex) {
		graphics->vbd->tvarray =
			bmalloc(sizeof(struct tvertarray) * num_tex);

		for (i = 0; i < num_tex; i++) {
			graphics->vbd->tvarray[i].width = 2;
			graphics->vbd->tvarray[i].array =
				graphics->texverts[i].array;
		}
	}

	reset_immediate_arrays(graphics);

	return gs_create_vertexbuffer(graphics->vbd, 0);
}

void gs_vertex2f(float x, float y)
{
	struct vec3 v3;
       
	vec3_set(&v3, x, y, 0.0f);
	gs_vertex3v(&v3);
}

void gs_vertex3f(float x, float y, float z)
{
	struct vec3 v3;

	vec3_set(&v3, x, y, z);
	gs_vertex3v(&v3);
}

void gs_normal3f(float x, float y, float z)
{
	struct vec3 v3;

	vec3_set(&v3, x, y, z);
	gs_normal3v(&v3);
}

static inline bool validvertsize(graphics_t graphics, size_t num,
		const char *name)
{
	if (graphics->using_immediate && num == IMMEDIATE_COUNT) {
		blog(LOG_ERROR, "%s: tried to use over %u "
				"for immediate rendering",
				name, IMMEDIATE_COUNT);
		return false;
	}

	return true;
}

void gs_color(uint32_t color)
{
	graphics_t graphics = thread_graphics;
	if (!graphics)
		return;
	if (!validvertsize(graphics, graphics->colors.num, "gs_color"))
		return;
	
	da_push_back(graphics->colors, &color);
}

void gs_texcoord(float x, float y, int unit)
{
	struct vec2 v2;

	vec2_set(&v2, x, y);
	gs_texcoord2v(&v2, unit);
}

void gs_vertex2v(const struct vec2 *v)
{
	struct vec3 v3;

	vec3_set(&v3, v->x, v->y, 0.0f);
	gs_vertex3v(&v3);
}

void gs_vertex3v(const struct vec3 *v)
{
	graphics_t graphics = thread_graphics;
	if (!graphics)
		return;
	if (!validvertsize(graphics, graphics->verts.num, "gs_vertex"))
		return;

	da_push_back(graphics->verts, v);
}

void gs_normal3v(const struct vec3 *v)
{
	graphics_t graphics = thread_graphics;
	if (!graphics)
		return;
	if (!validvertsize(graphics, graphics->norms.num, "gs_normal"))
		return;
	
	da_push_back(graphics->norms, v);
}

void gs_color4v(const struct vec4 *v)
{
	/* TODO */
	UNUSED_PARAMETER(v);
}

void gs_texcoord2v(const struct vec2 *v, int unit)
{
	graphics_t graphics = thread_graphics;
	if (!graphics)
		return;
	if (!validvertsize(graphics, graphics->texverts[unit].num,
				"gs_texcoord"))
		return;

	da_push_back(graphics->texverts[unit], v);
}

input_t gs_getinput(void)
{
	/* TODO */
	return NULL;
}

effect_t gs_geteffect(void)
{
	return thread_graphics ? thread_graphics->cur_effect : NULL;
}

effect_t gs_create_effect_from_file(const char *file, char **error_string)
{
	char *file_string;
	effect_t effect = NULL;

	if (!thread_graphics || !file)
		return NULL;

	file_string = os_quick_read_utf8_file(file);
	if (!file_string) {
		blog(LOG_ERROR, "Could not load effect file '%s'", file);
		return NULL;
	}

	effect = gs_create_effect(file_string, file, error_string);
	bfree(file_string);

	return effect;
}

effect_t gs_create_effect(const char *effect_string, const char *filename,
		char **error_string)
{
	if (!thread_graphics || !effect_string)
		return NULL;

	struct gs_effect *effect = bzalloc(sizeof(struct gs_effect));
	struct effect_parser parser;
	bool success;

	effect->graphics = thread_graphics;

	ep_init(&parser);
	success = ep_parse(&parser, effect, effect_string, filename);
	if (!success) {
		if (error_string)
			*error_string = error_data_buildstring(
					&parser.cfp.error_list);
		effect_destroy(effect);
		effect = NULL;
	}

	ep_free(&parser);
	return effect;
}

shader_t gs_create_vertexshader_from_file(const char *file, char **error_string)
{
	if (!thread_graphics || !file)
		return NULL;

	char *file_string;
	shader_t shader = NULL;

	file_string = os_quick_read_utf8_file(file);
	if (!file_string) {
		blog(LOG_ERROR, "Could not load vertex shader file '%s'",
				file);
		return NULL;
	}

	shader = gs_create_vertexshader(file_string, file, error_string);
	bfree(file_string);

	return shader;
}

shader_t gs_create_pixelshader_from_file(const char *file, char **error_string)
{
	char *file_string;
	shader_t shader = NULL;

	if (!thread_graphics || !file)
		return NULL;

	file_string = os_quick_read_utf8_file(file);
	if (!file_string) {
		blog(LOG_ERROR, "Could not load pixel shader file '%s'",
				file);
		return NULL;
	}

	shader = gs_create_pixelshader(file_string, file, error_string);
	bfree(file_string);

	return shader;
}

texture_t gs_create_texture_from_file(const char *file, uint32_t flags)
{
	/* TODO */
	UNUSED_PARAMETER(file);
	UNUSED_PARAMETER(flags);
	return NULL;
}

texture_t gs_create_cubetexture_from_file(const char *file, uint32_t flags)
{
	/* TODO */
	UNUSED_PARAMETER(file);
	UNUSED_PARAMETER(flags);
	return NULL;
}

texture_t gs_create_volumetexture_from_file(const char *file, uint32_t flags)
{
	/* TODO */
	UNUSED_PARAMETER(file);
	UNUSED_PARAMETER(flags);
	return NULL;
}

static inline void assign_sprite_rect(float *start, float *end, float size,
		bool flip)
{
	if (!flip) {
		*start = 0.0f;
		*end   = size;
	} else {
		*start = size;
		*end   = 0.0f;
	}
}

static inline void assign_sprite_uv(float *start, float *end, bool flip)
{
	if (!flip) {
		*start = 0.0f;
		*end   = 1.0f;
	} else {
		*start = 1.0f;
		*end   = 0.0f;
	}
}

static void build_sprite(struct vb_data *data, float fcx, float fcy,
		float start_u, float end_u, float start_v, float end_v)
{
	struct vec2 *tvarray = data->tvarray[0].array;

	vec3_zero(data->points);
	vec3_set(data->points+1,  fcx, 0.0f, 0.0f);
	vec3_set(data->points+2, 0.0f,  fcy, 0.0f);
	vec3_set(data->points+3,  fcx,  fcy, 0.0f);
	vec2_set(tvarray,   start_u, start_v);
	vec2_set(tvarray+1, end_u,   start_v);
	vec2_set(tvarray+2, start_u, end_v);
	vec2_set(tvarray+3, end_u,   end_v);
}

static inline void build_sprite_norm(struct vb_data *data, float fcx, float fcy,
		uint32_t flip)
{
	float start_u, end_u;
	float start_v, end_v;

	assign_sprite_uv(&start_u, &end_u, (flip & GS_FLIP_U) != 0);
	assign_sprite_uv(&start_v, &end_v, (flip & GS_FLIP_V) != 0);
	build_sprite(data, fcx, fcy, start_u, end_u, start_v, end_v);
}

static inline void build_sprite_rect(struct vb_data *data, texture_t tex,
		float fcx, float fcy, uint32_t flip)
{
	float start_u, end_u;
	float start_v, end_v;
	float width  = (float)texture_getwidth(tex);
	float height = (float)texture_getheight(tex);

	assign_sprite_rect(&start_u, &end_u, width,  (flip & GS_FLIP_U) != 0);
	assign_sprite_rect(&start_v, &end_v, height, (flip & GS_FLIP_V) != 0);
	build_sprite(data, fcx, fcy, start_u, end_u, start_v, end_v);
}

void gs_draw_sprite(texture_t tex, uint32_t flip, uint32_t width,
		uint32_t height)
{
	graphics_t graphics = thread_graphics;
	float fcx, fcy;
	struct vb_data *data;

	assert(tex);
	if (!tex || !thread_graphics)
		return;

	if (gs_gettexturetype(tex) != GS_TEXTURE_2D) {
		blog(LOG_ERROR, "A sprite must be a 2D texture");
		return;
	}

	fcx = width  ? (float)width  : (float)texture_getwidth(tex);
	fcy = height ? (float)height : (float)texture_getheight(tex);

	data = vertexbuffer_getdata(graphics->sprite_buffer);
	if (texture_isrect(tex))
		build_sprite_rect(data, tex, fcx, fcy, flip);
	else
		build_sprite_norm(data, fcx, fcy, flip);

	vertexbuffer_flush(graphics->sprite_buffer, false);
	gs_load_vertexbuffer(graphics->sprite_buffer);
	gs_load_indexbuffer(NULL);

	gs_draw(GS_TRISTRIP, 0, 0);
}

void gs_draw_cube_backdrop(texture_t cubetex, const struct quat *rot,
		float left, float right, float top, float bottom, float znear)
{
	/* TODO */
	UNUSED_PARAMETER(cubetex);
	UNUSED_PARAMETER(rot);
	UNUSED_PARAMETER(left);
	UNUSED_PARAMETER(right);
	UNUSED_PARAMETER(top);
	UNUSED_PARAMETER(bottom);
	UNUSED_PARAMETER(znear);
}

void gs_resetviewport(void)
{
	uint32_t cx, cy;
	assert(thread_graphics != NULL);
	gs_getsize(&cx, &cy);

	gs_setviewport(0, 0, (int)cx, (int)cy);
}

void gs_set2dmode(void)
{
	uint32_t cx, cy;
	assert(thread_graphics != NULL);
	gs_getsize(&cx, &cy);

	gs_ortho(0.0f, (float)cx, 0.0f, (float)cy, -1.0, -1024.0f);
}

void gs_set3dmode(double fovy, double znear, double zvar)
{
	/* TODO */
	UNUSED_PARAMETER(fovy);
	UNUSED_PARAMETER(znear);
	UNUSED_PARAMETER(zvar);
}

void gs_viewport_push(void)
{
	if (!thread_graphics) return;

	struct gs_rect *rect = da_push_back_new(
			thread_graphics->viewport_stack);
	gs_getviewport(rect);
}

void gs_viewport_pop(void)
{
	struct gs_rect *rect;
	if (!thread_graphics || !thread_graphics->viewport_stack.num)
		return;

	rect = da_end(thread_graphics->viewport_stack);
	gs_setviewport(rect->x, rect->y, rect->cx, rect->cy);
	da_pop_back(thread_graphics->viewport_stack);
}

void texture_setimage(texture_t tex, const void *data, uint32_t linesize,
		bool flip)
{
	if (!thread_graphics || !tex)
		return;

	void *ptr;
	uint32_t linesize_out;
	uint32_t row_copy;
	int32_t height = (int32_t)texture_getheight(tex);
	int32_t y;

	if (!texture_map(tex, &ptr, &linesize_out))
		return;

	row_copy = (linesize < linesize_out) ? linesize : linesize_out;

	if (flip) {
		for (y = height-1; y >= 0; y--)
			memcpy((uint8_t*)ptr  + (uint32_t)y * linesize_out,
			       (uint8_t*)data + (uint32_t)y * linesize,
			       row_copy);

	} else if (linesize == linesize_out) {
		memcpy(ptr, data, row_copy * height);

	} else {
		for (y = 0; y < height; y++)
			memcpy((uint8_t*)ptr  + (uint32_t)y * linesize_out,
			       (uint8_t*)data + (uint32_t)y * linesize,
			       row_copy);
	}

	texture_unmap(tex);
}

void cubetexture_setimage(texture_t cubetex, uint32_t side, const void *data,
		uint32_t linesize, bool invert)
{
	/* TODO */
	UNUSED_PARAMETER(cubetex);
	UNUSED_PARAMETER(side);
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(linesize);
	UNUSED_PARAMETER(invert);
}

void gs_perspective(float angle, float aspect, float near, float far)
{
	graphics_t graphics = thread_graphics;
	float xmin, xmax, ymin, ymax;

	if (!graphics) return;

	ymax = near * tanf(RAD(angle)*0.5f);
	ymin = -ymax;

	xmin = ymin * aspect;
	xmax = ymax * aspect;

	graphics->exports.device_frustum(graphics->device, xmin, xmax,
			ymin, ymax, near, far);
}

/* ------------------------------------------------------------------------- */

const char *gs_preprocessor_name(void)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return NULL;

	return graphics->exports.device_preprocessor_name();
}

swapchain_t gs_create_swapchain(struct gs_init_data *data)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return NULL;

	return graphics->exports.device_create_swapchain(graphics->device,
			data);
}

void gs_resize(uint32_t x, uint32_t y)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_resize(graphics->device, x, y);
}

void gs_getsize(uint32_t *x, uint32_t *y)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_getsize(graphics->device, x, y);
}

uint32_t gs_getwidth(void)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return 0;

	return graphics->exports.device_getwidth(graphics->device);
}

uint32_t gs_getheight(void)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return 0;

	return graphics->exports.device_getheight(graphics->device);
}

static inline bool is_pow2(uint32_t size)
{
	return size >= 2 && (size & (size-1)) == 0;
}

texture_t gs_create_texture(uint32_t width, uint32_t height,
		enum gs_color_format color_format, uint32_t levels,
		const void **data, uint32_t flags)
{
	graphics_t graphics = thread_graphics;
	bool pow2tex = is_pow2(width) && is_pow2(height);
	bool uses_mipmaps = (flags & GS_BUILDMIPMAPS || levels != 1);

	if (!graphics)
		return NULL;

	if (uses_mipmaps && !pow2tex) {
		blog(LOG_WARNING, "Cannot use mipmaps with a "
		                  "non-power-of-two texture.  Disabling "
		                  "mipmaps for this texture.");

		uses_mipmaps = false;
		flags &= ~GS_BUILDMIPMAPS;
		levels = 1;
	}

	if (uses_mipmaps && flags & GS_RENDERTARGET) {
		blog(LOG_WARNING, "Cannot use mipmaps with render targets.  "
		                  "Disabling mipmaps for this texture.");
		flags &= ~GS_BUILDMIPMAPS;
		levels = 1;
	}

	return graphics->exports.device_create_texture(graphics->device,
			width, height, color_format, levels, data, flags);
}

texture_t gs_create_cubetexture(uint32_t size,
		enum gs_color_format color_format, uint32_t levels,
		const void **data, uint32_t flags)
{
	graphics_t graphics = thread_graphics;
	bool pow2tex = is_pow2(size);
	bool uses_mipmaps = (flags & GS_BUILDMIPMAPS || levels != 1);

	if (!graphics)
		return NULL;

	if (uses_mipmaps && !pow2tex) {
		blog(LOG_WARNING, "Cannot use mipmaps with a "
		                  "non-power-of-two texture.  Disabling "
		                  "mipmaps for this texture.");

		uses_mipmaps = false;
		flags &= ~GS_BUILDMIPMAPS;
		levels = 1;
	}

	if (uses_mipmaps && flags & GS_RENDERTARGET) {
		blog(LOG_WARNING, "Cannot use mipmaps with render targets.  "
		                  "Disabling mipmaps for this texture.");
		flags &= ~GS_BUILDMIPMAPS;
		levels = 1;
		data   = NULL;
	}

	return graphics->exports.device_create_cubetexture(graphics->device,
			size, color_format, levels, data, flags);
}

texture_t gs_create_volumetexture(uint32_t width, uint32_t height,
		uint32_t depth, enum gs_color_format color_format,
		uint32_t levels, const void **data, uint32_t flags)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return NULL;

	return graphics->exports.device_create_volumetexture(graphics->device,
			width, height, depth, color_format, levels, data,
			flags);
}

zstencil_t gs_create_zstencil(uint32_t width, uint32_t height,
		enum gs_zstencil_format format)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return NULL;

	return graphics->exports.device_create_zstencil(graphics->device,
			width, height, format);
}

stagesurf_t gs_create_stagesurface(uint32_t width, uint32_t height,
		enum gs_color_format color_format)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return NULL;

	return graphics->exports.device_create_stagesurface(graphics->device,
			width, height, color_format);
}

samplerstate_t gs_create_samplerstate(struct gs_sampler_info *info)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return NULL;

	return graphics->exports.device_create_samplerstate(graphics->device,
			info);
}

shader_t gs_create_vertexshader(const char *shader, const char *file,
		char **error_string)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return NULL;

	return graphics->exports.device_create_vertexshader(graphics->device,
			shader, file, error_string);
}

shader_t gs_create_pixelshader(const char *shader,
		const char *file, char **error_string)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return NULL;

	return graphics->exports.device_create_pixelshader(graphics->device,
			shader, file, error_string);
}

vertbuffer_t gs_create_vertexbuffer(struct vb_data *data,
		uint32_t flags)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return NULL;

	return graphics->exports.device_create_vertexbuffer(graphics->device,
			data, flags);
}

indexbuffer_t gs_create_indexbuffer(enum gs_index_type type,
		void *indices, size_t num, uint32_t flags)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return NULL;

	return graphics->exports.device_create_indexbuffer(graphics->device,
			type, indices, num, flags);
}

enum gs_texture_type gs_gettexturetype(texture_t texture)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return GS_TEXTURE_2D;

	return graphics->exports.device_gettexturetype(texture);
}

void gs_load_vertexbuffer(vertbuffer_t vertbuffer)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_load_vertexbuffer(graphics->device,
			vertbuffer);
}

void gs_load_indexbuffer(indexbuffer_t indexbuffer)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_load_indexbuffer(graphics->device,
			indexbuffer);
}

void gs_load_texture(texture_t tex, int unit)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_load_texture(graphics->device, tex, unit);
}

void gs_load_samplerstate(samplerstate_t samplerstate, int unit)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_load_samplerstate(graphics->device,
			samplerstate, unit);
}

void gs_load_vertexshader(shader_t vertshader)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_load_vertexshader(graphics->device,
			vertshader);
}

void gs_load_pixelshader(shader_t pixelshader)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_load_pixelshader(graphics->device,
			pixelshader);
}

void gs_load_defaultsamplerstate(bool b_3d, int unit)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_load_defaultsamplerstate(graphics->device,
			b_3d, unit);
}

shader_t gs_getvertexshader(void)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return NULL;

	return graphics->exports.device_getvertexshader(graphics->device);
}

shader_t gs_getpixelshader(void)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return NULL;

	return graphics->exports.device_getpixelshader(graphics->device);
}

texture_t gs_getrendertarget(void)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return NULL;

	return graphics->exports.device_getrendertarget(graphics->device);
}

zstencil_t gs_getzstenciltarget(void)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return NULL;

	return graphics->exports.device_getzstenciltarget(graphics->device);
}

void gs_setrendertarget(texture_t tex, zstencil_t zstencil)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_setrendertarget(graphics->device, tex,
			zstencil);
}

void gs_setcuberendertarget(texture_t cubetex, int side, zstencil_t zstencil)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_setcuberendertarget(graphics->device, cubetex,
			side, zstencil);
}

void gs_copy_texture(texture_t dst, texture_t src)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_copy_texture(graphics->device, dst, src);
}

void gs_copy_texture_region(texture_t dst, uint32_t dst_x, uint32_t dst_y,
		texture_t src, uint32_t src_x, uint32_t src_y,
		uint32_t src_w, uint32_t src_h)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_copy_texture_region(graphics->device,
			dst, dst_x, dst_y,
			src, src_x, src_y, src_w, src_h);
}

void gs_stage_texture(stagesurf_t dst, texture_t src)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_stage_texture(graphics->device, dst, src);
}

void gs_beginscene(void)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_beginscene(graphics->device);
}

void gs_draw(enum gs_draw_mode draw_mode, uint32_t start_vert,
		uint32_t num_verts)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_draw(graphics->device, draw_mode,
			start_vert, num_verts);
}

void gs_endscene(void)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_endscene(graphics->device);
}

void gs_load_swapchain(swapchain_t swapchain)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_load_swapchain(graphics->device, swapchain);
}

void gs_clear(uint32_t clear_flags, struct vec4 *color, float depth,
		uint8_t stencil)
{
	graphics_t graphics = thread_graphics;
	graphics->exports.device_clear(graphics->device, clear_flags, color,
			depth, stencil);
}

void gs_present(void)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_present(graphics->device);
}

void gs_setcullmode(enum gs_cull_mode mode)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_setcullmode(graphics->device, mode);
}

enum gs_cull_mode gs_getcullmode(void)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return GS_NEITHER;

	return graphics->exports.device_getcullmode(graphics->device);
}

void gs_enable_blending(bool enable)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_enable_blending(graphics->device, enable);
}

void gs_enable_depthtest(bool enable)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_enable_depthtest(graphics->device, enable);
}

void gs_enable_stenciltest(bool enable)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_enable_stenciltest(graphics->device, enable);
}

void gs_enable_stencilwrite(bool enable)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_enable_stencilwrite(graphics->device, enable);
}

void gs_enable_color(bool red, bool green, bool blue, bool alpha)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_enable_color(graphics->device, red, green,
			blue, alpha);
}

void gs_blendfunction(enum gs_blend_type src, enum gs_blend_type dest)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_blendfunction(graphics->device, src, dest);
}

void gs_depthfunction(enum gs_depth_test test)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_depthfunction(graphics->device, test);
}

void gs_stencilfunction(enum gs_stencil_side side, enum gs_depth_test test)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_stencilfunction(graphics->device, side, test);
}

void gs_stencilop(enum gs_stencil_side side, enum gs_stencil_op fail,
		enum gs_stencil_op zfail, enum gs_stencil_op zpass)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_stencilop(graphics->device, side, fail, zfail,
			zpass);
}

void gs_enable_fullscreen(bool enable)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_enable_fullscreen(graphics->device, enable);
}

int gs_fullscreen_enabled(void)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return 0;

	return graphics->exports.device_fullscreen_enabled(graphics->device);
}

void gs_setdisplaymode(const struct gs_display_mode *mode)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_setdisplaymode(graphics->device, mode);
}

void gs_getdisplaymode(struct gs_display_mode *mode)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_getdisplaymode(graphics->device, mode);
}

void gs_setcolorramp(float gamma, float brightness, float contrast)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_setcolorramp(graphics->device, gamma,
			brightness, contrast);
}

void gs_setviewport(int x, int y, int width, int height)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_setviewport(graphics->device, x, y, width,
			height);
}

void gs_getviewport(struct gs_rect *rect)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_getviewport(graphics->device, rect);
}

void gs_setscissorrect(struct gs_rect *rect)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_setscissorrect(graphics->device, rect);
}

void gs_ortho(float left, float right, float top, float bottom, float znear,
		float zfar)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_ortho(graphics->device, left, right, top,
			bottom, znear, zfar);
}

void gs_frustum(float left, float right, float top, float bottom, float znear,
		float zfar)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_frustum(graphics->device, left, right, top,
			bottom, znear, zfar);
}

void gs_projection_push(void)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_projection_push(graphics->device);
}

void gs_projection_pop(void)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return;

	graphics->exports.device_projection_pop(graphics->device);
}

void swapchain_destroy(swapchain_t swapchain)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !swapchain) return;

	graphics->exports.swapchain_destroy(swapchain);
}

void shader_destroy(shader_t shader)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !shader) return;

	graphics->exports.shader_destroy(shader);
}

int shader_numparams(shader_t shader)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !shader) return 0;

	return graphics->exports.shader_numparams(shader);
}

sparam_t shader_getparambyidx(shader_t shader, uint32_t param)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !shader) return NULL;

	return graphics->exports.shader_getparambyidx(shader, param);
}

sparam_t shader_getparambyname(shader_t shader, const char *name)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !shader) return NULL;

	return graphics->exports.shader_getparambyname(shader, name);
}

void shader_getparaminfo(shader_t shader, sparam_t param,
		struct shader_param_info *info)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !shader || !param) return;

	graphics->exports.shader_getparaminfo(shader, param, info);
}

sparam_t shader_getviewprojmatrix(shader_t shader)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !shader) return NULL;

	return graphics->exports.shader_getviewprojmatrix(shader);
}

sparam_t shader_getworldmatrix(shader_t shader)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !shader) return NULL;

	return graphics->exports.shader_getworldmatrix(shader);
}

void shader_setbool(shader_t shader, sparam_t param, bool val)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !shader || !param) return;

	graphics->exports.shader_setbool(shader, param, val);
}

void shader_setfloat(shader_t shader, sparam_t param, float val)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !shader || !param) return;

	graphics->exports.shader_setfloat(shader, param, val);
}

void shader_setint(shader_t shader, sparam_t param, int val)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !shader || !param) return;

	graphics->exports.shader_setint(shader, param, val);
}

void shader_setmatrix3(shader_t shader, sparam_t param,
		const struct matrix3 *val)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !shader || !param) return;

	graphics->exports.shader_setmatrix3(shader, param, val);
}

void shader_setmatrix4(shader_t shader, sparam_t param,
		const struct matrix4 *val)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !shader || !param) return;

	graphics->exports.shader_setmatrix4(shader, param, val);
}

void shader_setvec2(shader_t shader, sparam_t param,
		const struct vec2 *val)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !shader || !param) return;

	graphics->exports.shader_setvec2(shader, param, val);
}

void shader_setvec3(shader_t shader, sparam_t param,
		const struct vec3 *val)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !shader || !param) return;

	graphics->exports.shader_setvec3(shader, param, val);
}

void shader_setvec4(shader_t shader, sparam_t param,
		const struct vec4 *val)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !shader || !param) return;

	graphics->exports.shader_setvec4(shader, param, val);
}

void shader_settexture(shader_t shader, sparam_t param, texture_t val)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !shader || !param) return;

	graphics->exports.shader_settexture(shader, param, val);
}

void shader_setval(shader_t shader, sparam_t param, const void *val,
		size_t size)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !shader || !param) return;

	graphics->exports.shader_setval(shader, param, val, size);
}

void shader_setdefault(shader_t shader, sparam_t param)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !shader || !param) return;

	graphics->exports.shader_setdefault(shader, param);
}

void texture_destroy(texture_t tex)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !tex) return;

	graphics->exports.texture_destroy(tex);
}

uint32_t texture_getwidth(texture_t tex)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !tex) return 0;

	return graphics->exports.texture_getwidth(tex);
}

uint32_t texture_getheight(texture_t tex)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !tex) return 0;

	return graphics->exports.texture_getheight(tex);
}

enum gs_color_format texture_getcolorformat(texture_t tex)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !tex) return GS_UNKNOWN;

	return graphics->exports.texture_getcolorformat(tex);
}

bool texture_map(texture_t tex, void **ptr, uint32_t *linesize)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !tex) return false;

	return graphics->exports.texture_map(tex, ptr, linesize);
}

void texture_unmap(texture_t tex)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !tex) return;

	graphics->exports.texture_unmap(tex);
}

bool texture_isrect(texture_t tex)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !tex) return false;

	if (graphics->exports.texture_isrect)
		return graphics->exports.texture_isrect(tex);
	else
		return false;
}

void *texture_getobj(texture_t tex)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !tex) return NULL;

	return graphics->exports.texture_getobj(tex);
}

void cubetexture_destroy(texture_t cubetex)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !cubetex) return;

	graphics->exports.cubetexture_destroy(cubetex);
}

uint32_t cubetexture_getsize(texture_t cubetex)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !cubetex) return 0;

	return graphics->exports.cubetexture_getsize(cubetex);
}

enum gs_color_format cubetexture_getcolorformat(texture_t cubetex)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !cubetex) return GS_UNKNOWN;

	return graphics->exports.cubetexture_getcolorformat(cubetex);
}

void volumetexture_destroy(texture_t voltex)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !voltex) return;

	graphics->exports.volumetexture_destroy(voltex);
}

uint32_t volumetexture_getwidth(texture_t voltex)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !voltex) return 0;

	return graphics->exports.volumetexture_getwidth(voltex);
}

uint32_t volumetexture_getheight(texture_t voltex)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !voltex) return 0;

	return graphics->exports.volumetexture_getheight(voltex);
}

uint32_t volumetexture_getdepth(texture_t voltex)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !voltex) return 0;

	return graphics->exports.volumetexture_getdepth(voltex);
}

enum gs_color_format volumetexture_getcolorformat(texture_t voltex)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !voltex) return GS_UNKNOWN;

	return graphics->exports.volumetexture_getcolorformat(voltex);
}

void stagesurface_destroy(stagesurf_t stagesurf)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !stagesurf) return;

	graphics->exports.stagesurface_destroy(stagesurf);
}

uint32_t stagesurface_getwidth(stagesurf_t stagesurf)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !stagesurf) return 0;

	return graphics->exports.stagesurface_getwidth(stagesurf);
}

uint32_t stagesurface_getheight(stagesurf_t stagesurf)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !stagesurf) return 0;

	return graphics->exports.stagesurface_getheight(stagesurf);
}

enum gs_color_format stagesurface_getcolorformat(stagesurf_t stagesurf)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !stagesurf) return GS_UNKNOWN;

	return graphics->exports.stagesurface_getcolorformat(stagesurf);
}

bool stagesurface_map(stagesurf_t stagesurf, uint8_t **data, uint32_t *linesize)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !stagesurf) return false;

	return graphics->exports.stagesurface_map(stagesurf, data, linesize);
}

void stagesurface_unmap(stagesurf_t stagesurf)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !stagesurf) return;

	graphics->exports.stagesurface_unmap(stagesurf);
}

void zstencil_destroy(zstencil_t zstencil)
{
	if (!thread_graphics || !zstencil) return;

	thread_graphics->exports.zstencil_destroy(zstencil);
}

void samplerstate_destroy(samplerstate_t samplerstate)
{
	if (!thread_graphics || !samplerstate) return;

	thread_graphics->exports.samplerstate_destroy(samplerstate);
}

void vertexbuffer_destroy(vertbuffer_t vertbuffer)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !vertbuffer) return;

	graphics->exports.vertexbuffer_destroy(vertbuffer);
}

void vertexbuffer_flush(vertbuffer_t vertbuffer, bool rebuild)
{
	if (!thread_graphics || !vertbuffer) return;

	thread_graphics->exports.vertexbuffer_flush(vertbuffer, rebuild);
}

struct vb_data *vertexbuffer_getdata(vertbuffer_t vertbuffer)
{
	if (!thread_graphics || !vertbuffer) return NULL;

	return thread_graphics->exports.vertexbuffer_getdata(vertbuffer);
}

void indexbuffer_destroy(indexbuffer_t indexbuffer)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !indexbuffer) return;

	graphics->exports.indexbuffer_destroy(indexbuffer);
}

void   indexbuffer_flush(indexbuffer_t indexbuffer)
{
	if (!thread_graphics || !indexbuffer) return;

	thread_graphics->exports.indexbuffer_flush(indexbuffer);
}

void  *indexbuffer_getdata(indexbuffer_t indexbuffer)
{
	if (!thread_graphics || !indexbuffer) return NULL;

	return thread_graphics->exports.indexbuffer_getdata(indexbuffer);
}

size_t indexbuffer_numindices(indexbuffer_t indexbuffer)
{
	if (!thread_graphics || !indexbuffer) return 0;

	return thread_graphics->exports.indexbuffer_numindices(indexbuffer);
}

enum gs_index_type indexbuffer_gettype(indexbuffer_t indexbuffer)
{
	if (!thread_graphics || !indexbuffer) return (enum gs_index_type)0;

	return thread_graphics->exports.indexbuffer_gettype(indexbuffer);
}

#ifdef __APPLE__

/** Platform specific functions */
texture_t gs_create_texture_from_iosurface(void *iosurf)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !iosurf ||
			!graphics->exports.texture_create_from_iosurface)
		return NULL;

	return graphics->exports.texture_create_from_iosurface(
			graphics->device, iosurf);
}

bool texture_rebind_iosurface(texture_t texture, void *iosurf)
{
	graphics_t graphics = thread_graphics;
	if (!graphics || !iosurf || !graphics->exports.texture_rebind_iosurface)
		return false;

	return graphics->exports.texture_rebind_iosurface(texture, iosurf);
}

#elif _WIN32

bool gs_gdi_texture_available(void)
{
	if (!thread_graphics)
		return false;

	return thread_graphics->exports.gdi_texture_available();
}

/** creates a windows GDI-lockable texture */
texture_t gs_create_gdi_texture(uint32_t width, uint32_t height)
{
	graphics_t graphics = thread_graphics;
	if (!graphics) return NULL;

	if (graphics->exports.device_create_gdi_texture)
		return graphics->exports.device_create_gdi_texture(
				graphics->device, width, height);
	return NULL;
}

void *texture_get_dc(texture_t gdi_tex)
{
	if (!thread_graphics || !gdi_tex)
		return NULL;

	if (thread_graphics->exports.texture_get_dc)
		return thread_graphics->exports.texture_get_dc(gdi_tex);
	return NULL;
}

void texture_release_dc(texture_t gdi_tex)
{
	if (!thread_graphics || !gdi_tex)
		return;

	if (thread_graphics->exports.texture_release_dc)
		thread_graphics->exports.texture_release_dc(gdi_tex);
}

#endif
