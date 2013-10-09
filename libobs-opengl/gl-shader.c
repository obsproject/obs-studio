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

static void gl_add_sampler(struct gs_shader *shader,
		struct shader_sampler *sampler)
{
	struct gs_sampler new_sampler = {0};
	struct gs_sampler_info info;

	shader_sampler_convert(sampler, &info);
	convert_sampler_info(&new_sampler, &info);
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
	shader->type = type;

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
	return shader_create(device, SHADER_VERTEX, shader, file, error_string);
}

shader_t device_create_pixelshader(device_t device,
		const char *shader, const char *file,
		char **error_string)
{
	return shader_create(device, SHADER_PIXEL, shader, file, error_string);
}

void shader_destroy(shader_t shader)
{
	size_t i;

	if (!shader)
		return;

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
