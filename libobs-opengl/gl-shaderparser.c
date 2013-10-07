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
#include "gl-shaderparser.h"

static void gl_write_type_n(struct gl_shader_parser *glsp,
		const char *type, size_t len)
{
	if (astrcmp_n(type, "float2", len) == 0)
		dstr_cat(&glsp->gl_string, "vec2");
	else if (astrcmp_n(type, "float3", len) == 0)
		dstr_cat(&glsp->gl_string, "vec3");
	else if (astrcmp_n(type, "float4", len) == 0)
		dstr_cat(&glsp->gl_string, "vec4");
	else if (astrcmp_n(type, "float3x3", len) == 0)
		dstr_cat(&glsp->gl_string, "mat3x3");
	else if (astrcmp_n(type, "float3x4", len) == 0)
		dstr_cat(&glsp->gl_string, "mat3x4");
	else if (astrcmp_n(type, "float4x4", len) == 0)
		dstr_cat(&glsp->gl_string, "mat4x4");
	else if (astrcmp_n(type, "texture2d", len) == 0)
		dstr_cat(&glsp->gl_string, "sampler2D");
	else if (astrcmp_n(type, "texture3d", len) == 0)
		dstr_cat(&glsp->gl_string, "sampler3D");
	else if (astrcmp_n(type, "texture_cube", len) == 0)
		dstr_cat(&glsp->gl_string, "samplerCube");
	else
		dstr_ncat(&glsp->gl_string, type, len);
}

static inline void gl_write_type(struct gl_shader_parser *glsp,
		const char *type)
{
	gl_write_type_n(glsp, type, strlen(type));
}

static inline void gl_write_type_token(struct gl_shader_parser *glsp,
		struct cf_token *token)
{
	gl_write_type_n(glsp, token->str.array, token->str.len);
}

static void gl_write_var(struct gl_shader_parser *glsp, struct shader_var *var)
{
	if (var->var_type == SHADER_VAR_UNIFORM)
		dstr_cat(&glsp->gl_string, "uniform ");
	else if (var->var_type == SHADER_VAR_CONST)
		dstr_cat(&glsp->gl_string, "const ");

	gl_write_type(glsp, var->type);
	dstr_cat(&glsp->gl_string, " ");
	dstr_cat(&glsp->gl_string, var->name);
	dstr_cat(&glsp->gl_string, ";\n");
}

static inline void gl_write_params(struct gl_shader_parser *glsp)
{
	size_t i;
	for (i = 0; i < glsp->parser.params.num; i++) {
		struct shader_var *var = glsp->parser.params.array+i;
		gl_write_var(glsp, var);
	}

	dstr_cat(&glsp->gl_string, "\n");
}

static void gl_write_storage_var(struct gl_shader_parser *glsp,
		struct shader_var *var, const char *storage,
		const char *prefix);

/* unwraps a structure that's used for input/output */
static void gl_unwrap_storage_struct(struct gl_shader_parser *glsp,
		struct shader_struct *st, const char *storage,
		const char *prefix)
{
	struct dstr prefix_str;
	size_t i;

	dstr_init(&prefix_str);
	if (prefix)
		dstr_copy(&prefix_str, prefix);
	dstr_cat(&prefix_str, st->name);
	dstr_cat(&prefix_str, "_");

	for (i = 0; i < st->vars.num; i++) {
		struct shader_var *st_var = st->vars.array+i;
		gl_write_storage_var(glsp, st_var, storage, prefix_str.array);
	}

	dstr_free(&prefix_str);
}

static void gl_write_storage_var(struct gl_shader_parser *glsp,
		struct shader_var *var, const char *storage, const char *prefix)
{
	struct shader_struct *st = shader_parser_getstruct(&glsp->parser,
			var->type);

	if (st) {
		gl_unwrap_storage_struct(glsp, st, storage, prefix);
	} else {
		if (storage) {
			dstr_cat(&glsp->gl_string, storage);
			dstr_cat(&glsp->gl_string, " ");
		}

		gl_write_type(glsp, var->type);
		dstr_cat(&glsp->gl_string, " ");
		if (prefix)
			dstr_cat(&glsp->gl_string, prefix);
		dstr_cat(&glsp->gl_string, var->name);
		dstr_cat(&glsp->gl_string, ";\n");
	}
}

static inline void gl_write_inputs(struct gl_shader_parser *glsp,
		struct shader_func *main)
{
	size_t i;
	for (i = 0; i < main->params.num; i++) {
		struct shader_var *var = main->params.array+i;
		gl_write_storage_var(glsp, var, "in", "in_");
	}
}

static void gl_write_outputs(struct gl_shader_parser *glsp,
		struct shader_func *main)
{
	struct shader_var var;

	shader_var_init(&var);
	var.type = bstrdup(main->return_type);
	var.name = bstrdup("return_val");
	if (main->return_mapping)
		var.mapping = bstrdup(main->return_mapping);

	gl_write_storage_var(glsp, &var, "out", "out_");
	shader_var_free(&var);
}

static void gl_write_struct(struct gl_shader_parser *glsp,
		struct shader_struct *st)
{
	size_t i;
	dstr_cat(&glsp->gl_string, "struct ");
	dstr_cat(&glsp->gl_string, st->name);
	dstr_cat(&glsp->gl_string, " {\n");

	for (i = 0; i < st->vars.num; i++) {
		struct shader_var *var = st->vars.array+i;

		dstr_cat(&glsp->gl_string, "\t");
		gl_write_var(glsp, var);
	}

	dstr_cat(&glsp->gl_string, "};\n\n");
}

static inline void gl_write_structs(struct gl_shader_parser *glsp)
{
	size_t i;
	for (i = 0; i < glsp->parser.structs.num; i++) {
		struct shader_struct *st = glsp->parser.structs.array+i;
		gl_write_struct(glsp, st);
	}
}

/*
 * NOTE: HLSL-> GLSL intrinsic conversions
 *   atan2    -> atan
 *   clip     -> (unsupported)
 *   ddx      -> dFdx
 *   ddy      -> dFdy
 *   fmod     -> (unsupported)
 *   frac     -> fract
 *   lerp     -> mix
 *   lit      -> (unsupported)
 *   log10    -> (unsupported)
 *   mul      -> (change to operator)
 *   rsqrt    -> inversesqrt
 *   saturate -> (use clamp)
 *   tex*     -> texture
 *   tex*grad -> textureGrad
 *   tex*lod  -> textureLod
 *   tex*bias -> (use optional 'bias' value)
 *   tex*proj -> textureProj
 *
 *   All else can be left as-is
 */

static void gl_write_functions(struct gl_shader_parser *glsp)
{
}

static bool gl_shader_buildstring(struct gl_shader_parser *glsp)
{
	struct shader_func *main = shader_parser_getfunc(&glsp->parser, "main");
	if (!main) {
		blog(LOG_ERROR, "function 'main' not found");
		return false;
	}

	dstr_copy(&glsp->gl_string, "#version 140\n\n");
	gl_write_params(glsp);
	gl_write_inputs(glsp, main);
	gl_write_outputs(glsp, main);
	gl_write_structs(glsp);
	gl_write_functions(glsp);
	// gl_write_main(glsp);

	return true;
}

bool gl_shader_parse(struct gl_shader_parser *glsp,
		const char *shader_str, const char *file)
{
	bool success = shader_parse(&glsp->parser, shader_str, file);
	char *str = shader_parser_geterrors(&glsp->parser);
	if (str) {
		blog(LOG_WARNING, "Shader parser errors/warnings:\n%s\n", str);
		bfree(str);
	}

	return success;
}
