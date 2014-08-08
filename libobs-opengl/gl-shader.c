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

#include <graphics/vec2.h>
#include <graphics/vec3.h>
#include <graphics/vec4.h>
#include <graphics/matrix3.h>
#include <graphics/matrix4.h>
#include "gl-subsystem.h"
#include "gl-shaderparser.h"

static inline void shader_param_init(struct gs_shader_param *param)
{
	memset(param, 0, sizeof(struct gs_shader_param));
}

static inline void shader_param_free(struct gs_shader_param *param)
{
	bfree(param->name);
	da_free(param->cur_value);
	da_free(param->def_value);
}

static void gl_get_program_info(GLuint program, const char *file,
		char **error_string)
{
	char    *errors;
	GLint   info_len = 0;
	GLsizei chars_written = 0;

	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &info_len);
	if (!gl_success("glGetProgramiv") || !info_len)
		return;

	errors = bzalloc(info_len+1);
	glGetProgramInfoLog(program, info_len, &chars_written, errors);
	gl_success("glGetProgramInfoLog");

	blog(LOG_DEBUG, "Compiler warnings/errors for %s:\n%s", file, errors);

	if (error_string)
		*error_string = errors;
	else
		bfree(errors);
}

static bool gl_add_param(struct gs_shader *shader, struct shader_var *var,
		GLint *texture_id)
{
	struct gs_shader_param param = {0};

	param.array_count = var->array_count;
	param.name        = bstrdup(var->name);
	param.shader      = shader;
	param.type        = get_shader_param_type(var->type);

	if (param.type == GS_SHADER_PARAM_TEXTURE) {
		param.sampler_id  = var->gl_sampler_id;
		param.texture_id  = (*texture_id)++;
	} else {
		param.changed = true;
	}

	da_move(param.def_value, var->default_val);
	da_copy(param.cur_value, param.def_value);

	param.param = glGetUniformLocation(shader->program, param.name);
	if (!gl_success("glGetUniformLocation"))
		goto fail;

	if (param.type == GS_SHADER_PARAM_TEXTURE) {
		glProgramUniform1i(shader->program, param.param,
				param.texture_id);
		if (!gl_success("glProgramUniform1i"))
			goto fail;
	}

	da_push_back(shader->params, &param);
	return true;

fail:
	shader_param_free(&param);
	return false;
}

static inline bool gl_add_params(struct gs_shader *shader,
		struct gl_shader_parser *glsp)
{
	size_t i;
	GLint tex_id = 0;

	for (i = 0; i < glsp->parser.params.num; i++)
		if (!gl_add_param(shader, glsp->parser.params.array+i, &tex_id))
			return false;

	shader->viewproj = gs_shader_get_param_by_name(shader, "ViewProj");
	shader->world    = gs_shader_get_param_by_name(shader, "World");

	return true;
}

static inline void gl_add_sampler(struct gs_shader *shader,
		struct shader_sampler *sampler)
{
	gs_samplerstate_t new_sampler;
	struct gs_sampler_info info;

	shader_sampler_convert(sampler, &info);
	new_sampler = device_samplerstate_create(shader->device, &info);

	da_push_back(shader->samplers, &new_sampler);
}

static inline void gl_add_samplers(struct gs_shader *shader,
		struct gl_shader_parser *glsp)
{
	size_t i;
	for (i = 0; i < glsp->parser.samplers.num; i++) {
		struct shader_sampler *sampler = glsp->parser.samplers.array+i;
		gl_add_sampler(shader, sampler);
	}
}

static void get_attrib_type(const char *mapping, enum attrib_type *type,
		size_t *index)
{
	if (strcmp(mapping, "POSITION") == 0) {
		*type  = ATTRIB_POSITION;

	} else if (strcmp(mapping, "NORMAL") == 0) {
		*type  = ATTRIB_NORMAL;

	} else if (strcmp(mapping, "TANGENT") == 0) {
		*type  = ATTRIB_TANGENT;

	} else if (strcmp(mapping, "COLOR") == 0) {
		*type  = ATTRIB_COLOR;

	} else if (astrcmp_n(mapping, "TEXCOORD", 8) == 0) {
		*type  = ATTRIB_TEXCOORD;
		*index = (*(mapping+8)) - '0';
		return;

	} else if (strcmp(mapping, "TARGET") == 0) {
		*type  = ATTRIB_TARGET;
	}

	*index = 0;
}

static inline bool gl_process_attrib(struct gs_shader *shader,
		struct gl_parser_attrib *pa)
{
	struct shader_attrib attrib = {0};

	/* don't parse output attributes */
	if (!pa->input)
		return true;

	get_attrib_type(pa->mapping, &attrib.type, &attrib.index);

	attrib.attrib = glGetAttribLocation(shader->program, pa->name.array);
	if (!gl_success("glGetAttribLocation"))
		return false;

	if (attrib.attrib == -1) {
		blog(LOG_ERROR, "glGetAttribLocation: Could not find "
		                "attribute '%s'", pa->name.array);
		return false;
	}

	da_push_back(shader->attribs, &attrib);
	return true;
}

static inline bool gl_process_attribs(struct gs_shader *shader,
		struct gl_shader_parser *glsp)
{
	size_t i;
	for (i = 0; i < glsp->attribs.num; i++) {
		struct gl_parser_attrib *pa = glsp->attribs.array+i;
		if (!gl_process_attrib(shader, pa))
			return false;
	}

	return true;
}

static bool gl_shader_init(struct gs_shader *shader,
		struct gl_shader_parser *glsp,
		const char *file, char **error_string)
{
	GLenum type = convert_shader_type(shader->type);
	int compiled = 0;
	bool success = true;

	shader->program = glCreateShaderProgramv(type, 1,
			(const GLchar**)&glsp->gl_string.array);
	if (!gl_success("glCreateShaderProgramv") || !shader->program)
		return false;

#if 1
	blog(LOG_DEBUG, "+++++++++++++++++++++++++++++++++++");
	blog(LOG_DEBUG, "  GL shader string for: %s", file);
	blog(LOG_DEBUG, "-----------------------------------");
	blog(LOG_DEBUG, "%s", glsp->gl_string.array);
	blog(LOG_DEBUG, "+++++++++++++++++++++++++++++++++++");
#endif

	glGetProgramiv(shader->program, GL_LINK_STATUS, &compiled);
	if (!gl_success("glGetProgramiv"))
		return false;

	if (!compiled)
		success = false;

	gl_get_program_info(shader->program, file, error_string);

	if (success)
		success = gl_add_params(shader, glsp);
	/* Only vertex shaders actually require input attributes */
	if (success && shader->type == GS_SHADER_VERTEX)
		success = gl_process_attribs(shader, glsp);
	if (success)
		gl_add_samplers(shader, glsp);

	return success;
}

static struct gs_shader *shader_create(gs_device_t device,
		enum gs_shader_type type, const char *shader_str,
		const char *file, char **error_string)
{
	struct gs_shader *shader = bzalloc(sizeof(struct gs_shader));
	struct gl_shader_parser glsp;
	bool success = true;

	shader->device = device;
	shader->type   = type;

	gl_shader_parser_init(&glsp, type);
	if (!gl_shader_parse(&glsp, shader_str, file))
		success = false;
	else
		success = gl_shader_init(shader, &glsp, file, error_string);

	if (!success) {
		gs_shader_destroy(shader);
		shader = NULL;
	}

	gl_shader_parser_free(&glsp);
	return shader;
}

gs_shader_t device_vertexshader_create(gs_device_t device,
		const char *shader, const char *file,
		char **error_string)
{
	struct gs_shader *ptr;
	ptr = shader_create(device, GS_SHADER_VERTEX, shader, file,
			error_string);
	if (!ptr)
		blog(LOG_ERROR, "device_vertexshader_create (GL) failed");
	return ptr;
}

gs_shader_t device_pixelshader_create(gs_device_t device,
		const char *shader, const char *file,
		char **error_string)
{
	struct gs_shader *ptr;
	ptr = shader_create(device, GS_SHADER_PIXEL, shader, file,
			error_string);
	if (!ptr)
		blog(LOG_ERROR, "device_pixelshader_create (GL) failed");
	return ptr;
}

void gs_shader_destroy(gs_shader_t shader)
{
	size_t i;

	if (!shader)
		return;

	for (i = 0; i < shader->samplers.num; i++)
		gs_samplerstate_destroy(shader->samplers.array[i]);

	for (i = 0; i < shader->params.num; i++)
		shader_param_free(shader->params.array+i);

	if (shader->program) {
		glDeleteProgram(shader->program);
		gl_success("glDeleteProgram");
	}

	da_free(shader->samplers);
	da_free(shader->params);
	da_free(shader->attribs);
	bfree(shader);
}

int gs_shader_get_num_params(gs_shader_t shader)
{
	return (int)shader->params.num;
}

gs_sparam_t gs_shader_get_param_by_idx(gs_shader_t shader, uint32_t param)
{
	assert(param < shader->params.num);
	return shader->params.array+param;
}

gs_sparam_t gs_shader_get_param_by_name(gs_shader_t shader, const char *name)
{
	size_t i;
	for (i = 0; i < shader->params.num; i++) {
		struct gs_shader_param *param = shader->params.array+i;

		if (strcmp(param->name, name) == 0)
			return param;
	}

	return NULL;
}

gs_sparam_t gs_shader_get_viewproj_matrix(gs_shader_t shader)
{
	return shader->viewproj;
}

gs_sparam_t gs_shader_get_world_matrix(gs_shader_t shader)
{
	return shader->world;
}

void gs_shader_get_param_info(gs_sparam_t param,
		struct gs_shader_param_info *info)
{
	info->type = param->type;
	info->name = param->name;
}

void gs_shader_set_bool(gs_sparam_t param, bool val)
{
	struct gs_shader *shader = param->shader;
	glProgramUniform1i(shader->program, param->param, (GLint)val);
	gl_success("glProgramUniform1i");
}

void gs_shader_set_float(gs_sparam_t param, float val)
{
	struct gs_shader *shader = param->shader;
	glProgramUniform1f(shader->program, param->param, val);
	gl_success("glProgramUniform1f");
}

void gs_shader_set_int(gs_sparam_t param, int val)
{
	struct gs_shader *shader = param->shader;
	glProgramUniform1i(shader->program, param->param, val);
	gl_success("glProgramUniform1i");
}

void gs_shader_setmatrix3(gs_sparam_t param, const struct matrix3 *val)
{
	struct gs_shader *shader = param->shader;
	struct matrix4 mat;
	matrix4_from_matrix3(&mat, val);

	glProgramUniformMatrix4fv(shader->program, param->param, 1,
			false, mat.x.ptr);
	gl_success("glProgramUniformMatrix4fv");
}

void gs_shader_set_matrix4(gs_sparam_t param, const struct matrix4 *val)
{
	struct gs_shader *shader = param->shader;
	glProgramUniformMatrix4fv(shader->program, param->param, 1,
			false, val->x.ptr);
	gl_success("glProgramUniformMatrix4fv");
}

void gs_shader_set_vec2(gs_sparam_t param, const struct vec2 *val)
{
	struct gs_shader *shader = param->shader;
	glProgramUniform2fv(shader->program, param->param, 1, val->ptr);
	gl_success("glProgramUniform2fv");
}

void gs_shader_set_vec3(gs_sparam_t param, const struct vec3 *val)
{
	struct gs_shader *shader = param->shader;
	glProgramUniform3fv(shader->program, param->param, 1, val->ptr);
	gl_success("glProgramUniform3fv");
}

void gs_shader_set_vec4(gs_sparam_t param, const struct vec4 *val)
{
	struct gs_shader *shader = param->shader;
	glProgramUniform4fv(shader->program, param->param, 1, val->ptr);
	gl_success("glProgramUniform4fv");
}

void gs_shader_set_texture(gs_sparam_t param, gs_texture_t val)
{
	param->texture = val;
}

static void shader_setval_data(gs_sparam_t param, const void *val, int count)
{
	struct gs_shader *shader = param->shader;

	if (param->type == GS_SHADER_PARAM_BOOL ||
	    param->type == GS_SHADER_PARAM_INT) {
		glProgramUniform1iv(shader->program, param->param, count, val);
		gl_success("glProgramUniform1iv");

	} else if (param->type == GS_SHADER_PARAM_FLOAT) {
		glProgramUniform1fv(shader->program, param->param, count, val);
		gl_success("glProgramUniform1fv");

	} else if (param->type == GS_SHADER_PARAM_VEC2) {
		glProgramUniform2fv(shader->program, param->param, count, val);
		gl_success("glProgramUniform2fv");

	} else if (param->type == GS_SHADER_PARAM_VEC3) {
		glProgramUniform3fv(shader->program, param->param, count, val);
		gl_success("glProgramUniform3fv");

	} else if (param->type == GS_SHADER_PARAM_VEC4) {
		glProgramUniform4fv(shader->program, param->param, count, val);
		gl_success("glProgramUniform4fv");

	} else if (param->type == GS_SHADER_PARAM_MATRIX4X4) {
		glProgramUniformMatrix4fv(shader->program, param->param,
				count, false, val);
		gl_success("glProgramUniformMatrix4fv");
	}
}

void shader_update_textures(struct gs_shader *shader)
{
	size_t i;
	for (i = 0; i < shader->params.num; i++) {
		struct gs_shader_param *param = shader->params.array+i;

		if (param->type == GS_SHADER_PARAM_TEXTURE)
			device_load_texture(shader->device, param->texture,
					param->texture_id);
	}
}

void gs_shader_set_val(gs_sparam_t param, const void *val, size_t size)
{
	int count = param->array_count;
	size_t expected_size = 0;
	if (!count)
		count = 1;

	switch ((uint32_t)param->type) {
	case GS_SHADER_PARAM_FLOAT:     expected_size = sizeof(float); break;
	case GS_SHADER_PARAM_BOOL:
	case GS_SHADER_PARAM_INT:       expected_size = sizeof(int); break;
	case GS_SHADER_PARAM_VEC2:      expected_size = sizeof(float)*2; break;
	case GS_SHADER_PARAM_VEC3:      expected_size = sizeof(float)*3; break;
	case GS_SHADER_PARAM_VEC4:      expected_size = sizeof(float)*4; break;
	case GS_SHADER_PARAM_MATRIX4X4: expected_size = sizeof(float)*4*4;break;
	case GS_SHADER_PARAM_TEXTURE:   expected_size = sizeof(void*); break;
	default:                     expected_size = 0;
	}

	expected_size *= count;
	if (!expected_size)
		return;

	if (expected_size != size) {
		blog(LOG_ERROR, "gs_shader_set_val (GL): Size of shader "
		                "param does not match the size of the input");
		return;
	}

	if (param->type == GS_SHADER_PARAM_TEXTURE)
		gs_shader_set_texture(param, *(gs_texture_t*)val);
	else
		shader_setval_data(param, val, count);
}

void gs_shader_set_default(gs_sparam_t param)
{
	gs_shader_set_val(param, param->def_value.array, param->def_value.num);
}
