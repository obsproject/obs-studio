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

#include <assert.h>
#include "graphics/vec2.h"
#include "graphics/vec3.h"
#include "graphics/vec4.h"
#include "graphics/matrix3.h"
#include "graphics/matrix4.h"
#include "gl-subsystem.h"
#include "gl-shaderparser.h"

static inline void shader_param_init(struct shader_param *param)
{
	memset(param, 0, sizeof(struct shader_param));
}

static inline void shader_param_free(struct shader_param *param)
{
	bfree(param->name);
	da_free(param->cur_value);
	da_free(param->def_value);
}

static void gl_get_program_info(GLuint program, char **error_string)
{
	char    *errors;
	GLint   info_len = 0;
	GLsizei chars_written = 0;

	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &info_len);
	if (!gl_success("glGetProgramiv") || !info_len)
		return;

	errors = bmalloc(info_len+1);
	memset(errors, 0, info_len+1);
	glGetProgramInfoLog(program, info_len, &chars_written, errors);
	gl_success("glGetProgramInfoLog");

	*error_string = errors;
}

static bool gl_add_param(struct gs_shader *shader, struct shader_var *var,
		GLint *texture_id)
{
	struct shader_param param = {0};

	param.array_count = var->array_count;
	param.name        = bstrdup(var->name);
	param.type        = get_shader_param_type(var->type);

	if (param.type == SHADER_PARAM_TEXTURE) {
		param.sampler_id  = var->gl_sampler_id;
		param.texture_id  = (*texture_id)++;
	} else {
		param.changed = true;
	}

	da_move(param.def_value, var->default_val);
	da_copy(param.cur_value, param.def_value);

	param.param = glGetUniformLocation(shader->program, param.name);
	if (!gl_success("glGetUniformLocation"))
		return false;

	if (param.type == SHADER_PARAM_TEXTURE)
		glUniform1i(param.param, param.texture_id);

	return true;
}

static inline bool gl_add_params(struct gs_shader *shader,
		struct gl_shader_parser *glsp)
{
	size_t i;
	GLint tex_id = 0;

	for (i = 0; i < glsp->parser.params.num; i++)
		if (!gl_add_param(shader, glsp->parser.params.array+i, &tex_id))
			return false;

	return true;
}

static inline void gl_add_sampler(struct gs_shader *shader,
		struct shader_sampler *sampler)
{
	samplerstate_t new_sampler;
	struct gs_sampler_info info;

	shader_sampler_convert(sampler, &info);
	new_sampler = device_create_samplerstate(shader->device, &info);

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

static inline bool gl_add_attrib(struct gs_shader *shader,
		struct gl_parser_attrib *pa)
{
	struct shader_attrib attrib = {0};
	get_attrib_type(pa->mapping, &attrib.type, &attrib.index);

	attrib.attrib = glGetAttribLocation(shader->program, pa->name.array);
	if (!gl_success("glGetAttribLocation"))
		return false;

	if (attrib.attrib == -1)
		return false;

	if (attrib.type == ATTRIB_TARGET) {
		glBindFragDataLocation(shader->program, 0, pa->name.array);
		if (!gl_success("glBindFragDataLocation"))
			return false;
	}

	da_push_back(shader->attribs, &attrib);
	return true;
}

static inline bool gl_add_attribs(struct gs_shader *shader,
		struct gl_shader_parser *glsp)
{
	size_t i;
	for (i = 0; i < glsp->attribs.num; i++) {
		struct gl_parser_attrib *pa = glsp->attribs.array+i;
		if (!gl_add_attrib(shader, pa))
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
			&glsp->gl_string.array);
	gl_success("glCreateShaderProgramv");
	if (!shader->program)
		return false;

	glGetProgramiv(shader->program, GL_VALIDATE_STATUS, &compiled);
	if (!gl_success("glGetProgramiv"))
		return false;

	if (!compiled)
		success = false;

	gl_get_program_info(shader->program, error_string);

	if (success)
		success = gl_add_params(shader, glsp);
	if (success)
		success = gl_add_attribs(shader, glsp);
	if (success)
		gl_add_samplers(shader, glsp);

	return success;
}

static struct gs_shader *shader_create(device_t device, enum shader_type type,
		const char *shader_str, const char *file, char **error_string)
{
	struct gs_shader *shader = bmalloc(sizeof(struct gs_shader));
	struct gl_shader_parser glsp;
	bool success = true;

	memset(shader, 0, sizeof(struct gs_shader));
	shader->device = device;
	shader->type   = type;

	gl_shader_parser_init(&glsp);
	if (!gl_shader_parse(&glsp, shader_str, file))
		success = false;
	else
		success = gl_shader_init(shader, &glsp, file, error_string);

	if (!success) {
		shader_destroy(shader);
		shader = NULL;
	}

	gl_shader_parser_free(&glsp);
	return shader;
}

shader_t device_create_vertexshader(device_t device,
		const char *shader, const char *file,
		char **error_string)
{
	struct gs_shader *ptr;
	ptr = shader_create(device, SHADER_VERTEX, shader, file, error_string);
	if (!ptr)
		blog(LOG_ERROR, "device_create_vertexshader (GL) failed");
	return ptr;
}

shader_t device_create_pixelshader(device_t device,
		const char *shader, const char *file,
		char **error_string)
{
	struct gs_shader *ptr;
	ptr = shader_create(device, SHADER_PIXEL, shader, file, error_string);
	if (!ptr)
		blog(LOG_ERROR, "device_create_pixelshader (GL) failed");
	return ptr;
}

void shader_destroy(shader_t shader)
{
	size_t i;

	if (!shader)
		return;

	for (i = 0; i < shader->samplers.num; i++)
		samplerstate_destroy(shader->samplers.array[i]);

	for (i = 0; i < shader->params.num; i++)
		shader_param_free(shader->params.array+i);

	if (shader->program) {
		glDeleteProgram(shader->program);
		gl_success("glDeleteProgram");
	}

	da_free(shader->samplers);
	da_free(shader->params);
	bfree(shader);
}

int shader_numparams(shader_t shader)
{
	return (int)shader->params.num;
}

sparam_t shader_getparambyidx(shader_t shader, int param)
{
	assert(param < shader->params.num);
	return shader->params.array+param;
}

sparam_t shader_getparambyname(shader_t shader, const char *name)
{
	size_t i;
	for (i = 0; i < shader->params.num; i++) {
		struct shader_param *param = shader->params.array+i;

		if (strcmp(param->name, name) == 0)
			return param;
	}

	return NULL;
}

void shader_getparaminfo(shader_t shader, sparam_t param,
		struct shader_param_info *info)
{
	info->type = param->type;
	info->name = param->name;
}

sparam_t shader_getviewprojmatrix(shader_t shader)
{
	return shader->viewproj;
}

sparam_t shader_getworldmatrix(shader_t shader)
{
	return shader->world;
}

void shader_setbool(shader_t shader, sparam_t param, bool val)
{
	glProgramUniform1i(shader->program, param->param, (GLint)val);
	gl_success("glProgramUniform1i");
}

void shader_setfloat(shader_t shader, sparam_t param, float val)
{
	glProgramUniform1f(shader->program, param->param, val);
	gl_success("glProgramUniform1f");
}

void shader_setint(shader_t shader, sparam_t param, int val)
{
	glProgramUniform1i(shader->program, param->param, val);
	gl_success("glProgramUniform1i");
}

void shader_setmatrix3(shader_t shader, sparam_t param,
		const struct matrix3 *val)
{
	struct matrix4 mat;
	matrix4_from_matrix3(&mat, val);

	glProgramUniformMatrix4fv(shader->program, param->param, 1, true,
			mat.x.ptr);
	gl_success("glProgramUniformMatrix4fv");
}

void shader_setmatrix4(shader_t shader, sparam_t param,
		const struct matrix4 *val)
{
	glProgramUniformMatrix4fv(shader->program, param->param, 1, true,
			val->x.ptr);
	gl_success("glProgramUniformMatrix4fv");
}

void shader_setvec2(shader_t shader, sparam_t param,
		const struct vec2 *val)
{
	glProgramUniform2fv(shader->program, param->param, 1, val->ptr);
	gl_success("glProgramUniform2fv");
}

void shader_setvec3(shader_t shader, sparam_t param,
		const struct vec3 *val)
{
	glProgramUniform3fv(shader->program, param->param, 1, val->ptr);
	gl_success("glProgramUniform3fv");
}

void shader_setvec4(shader_t shader, sparam_t param,
		const struct vec4 *val)
{
	glProgramUniform4fv(shader->program, param->param, 1, val->ptr);
	gl_success("glProgramUniform4fv");
}

void shader_settexture(shader_t shader, sparam_t param, texture_t val)
{
	param->texture = val;
}

static void shader_setval_data(shader_t shader, sparam_t param,
		const void *val, int count)
{
	if (param->type == SHADER_PARAM_BOOL ||
	    param->type == SHADER_PARAM_INT) {
		glProgramUniform1iv(shader->program, param->param, count, val);
		gl_success("glProgramUniform1iv");

	} else if (param->type == SHADER_PARAM_FLOAT) {
		glProgramUniform1fv(shader->program, param->param, count, val);
		gl_success("glProgramUniform1fv");

	} else if (param->type == SHADER_PARAM_VEC2) {
		glProgramUniform2fv(shader->program, param->param, count, val);
		gl_success("glProgramUniform2fv");

	} else if (param->type == SHADER_PARAM_VEC3) {
		glProgramUniform3fv(shader->program, param->param, count, val);
		gl_success("glProgramUniform3fv");

	} else if (param->type == SHADER_PARAM_VEC4) {
		glProgramUniform4fv(shader->program, param->param, count, val);
		gl_success("glProgramUniform4fv");

	} else if (param->type == SHADER_PARAM_MATRIX4X4) {
		glProgramUniformMatrix4fv(shader->program, param->param,
				count, false, val);
		gl_success("glProgramUniformMatrix4fv");
	}
}

void shader_setval(shader_t shader, sparam_t param, const void *val,
		size_t size)
{
	int count = param->array_count;
	size_t expected_size = 0;
	if (!count)
		count = 1;

	switch (param->type) {
	case SHADER_PARAM_FLOAT:     expected_size = sizeof(float); break;
	case SHADER_PARAM_BOOL:
	case SHADER_PARAM_INT:       expected_size = sizeof(int); break;
	case SHADER_PARAM_VEC2:      expected_size = sizeof(float)*2; break;
	case SHADER_PARAM_VEC3:      expected_size = sizeof(float)*3; break;
	case SHADER_PARAM_VEC4:      expected_size = sizeof(float)*4; break;
	case SHADER_PARAM_MATRIX4X4: expected_size = sizeof(float)*4*4; break;
	}

	expected_size *= count;
	if (!expected_size)
		return;

	if (expected_size != size) {
		blog(LOG_ERROR, "shader_setval (GL): Size of shader param does "
		                "not match the size of the input");
		return;
	}

	shader_setval_data(shader, param, val, count);
}

void shader_setdefault(shader_t shader, sparam_t param)
{
	shader_setval(shader, param, param->def_value.array,
			param->def_value.num);
}
