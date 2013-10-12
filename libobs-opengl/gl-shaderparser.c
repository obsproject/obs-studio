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

static void gl_write_function_contents(struct gl_shader_parser *glsp,
		struct cf_token **p_token, const char *end);

static inline struct shader_var *sp_getparam(struct gl_shader_parser *glsp,
		struct cf_token *token)
{
	size_t i;
	for (i = 0; i < glsp->parser.params.num; i++) {
		struct shader_var *param = glsp->parser.params.array+i;
		if (strref_cmp(&token->str, param->name) == 0)
			return param;
	}

	return NULL;
}

static inline size_t sp_getsampler(struct gl_shader_parser *glsp,
		struct cf_token *token)
{
	size_t i;
	for (i = 0; i < glsp->parser.samplers.num; i++) {
		struct shader_sampler *sampler = glsp->parser.samplers.array+i;
		if (strref_cmp(&token->str, sampler->name) == 0)
			return i;
	}

	return -1;
}

static bool gl_write_type_n(struct gl_shader_parser *glsp,
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
		return false;

	return true;
}

static inline void gl_write_type(struct gl_shader_parser *glsp,
		const char *type)
{
	if (!gl_write_type_n(glsp, type, strlen(type)))
		dstr_cat(&glsp->gl_string, type);
}

static inline bool gl_write_type_token(struct gl_shader_parser *glsp,
		struct cf_token *token)
{
	return gl_write_type_n(glsp, token->str.array, token->str.len);
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
}

static inline void gl_write_params(struct gl_shader_parser *glsp)
{
	size_t i;
	for (i = 0; i < glsp->parser.params.num; i++) {
		struct shader_var *var = glsp->parser.params.array+i;
		gl_write_var(glsp, var);
		dstr_cat(&glsp->gl_string, ";\n");
	}

	dstr_cat(&glsp->gl_string, "\n");
}

static void gl_write_storage_var(struct gl_shader_parser *glsp,
		struct shader_var *var, const char *storage,
		const char *prefix);

/* unwraps a structure that's used for input/output */
static void gl_unwrap_storage_struct(struct gl_shader_parser *glsp,
		struct shader_struct *st, const char *name, const char *storage,
		const char *prefix)
{
	struct dstr prefix_str;
	size_t i;

	dstr_init(&prefix_str);
	if (prefix)
		dstr_copy(&prefix_str, prefix);
	dstr_cat(&prefix_str, name);
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
		gl_unwrap_storage_struct(glsp, st, var->name, storage, prefix);
	} else {
		struct gl_parser_attrib attrib = {0};

		if (storage) {
			dstr_cat(&glsp->gl_string, storage);
			dstr_cat(&glsp->gl_string, " ");
		}

		if (prefix)
			dstr_cat(&attrib.name, prefix);
		dstr_cat(&attrib.name, var->name);

		gl_write_type(glsp, var->type);
		dstr_cat(&glsp->gl_string, " ");
		dstr_cat_dstr(&glsp->gl_string, &attrib.name);
		dstr_cat(&glsp->gl_string, ";\n");

		attrib.mapping = var->mapping;
		da_push_back(glsp->attribs, &attrib);
	}
}

static inline void gl_write_inputs(struct gl_shader_parser *glsp,
		struct shader_func *main)
{
	size_t i;
	for (i = 0; i < main->params.num; i++)
		gl_write_storage_var(glsp, main->params.array+i, "in",
				"inputval_");
	dstr_cat(&glsp->gl_string, "\n");
}

static void gl_write_outputs(struct gl_shader_parser *glsp,
		struct shader_func *main)
{
	struct shader_var var = {0};
	var.type = main->return_type;
	var.name = "outputval";
	if (main->mapping)
		var.mapping = main->mapping;

	gl_write_storage_var(glsp, &var, "out", NULL);
	dstr_cat(&glsp->gl_string, "\n");
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
		dstr_cat(&glsp->gl_string, ";\n");
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

static bool gl_write_mul(struct gl_shader_parser *glsp,
		struct cf_token **p_token)
{
	struct cf_parser *cfp = &glsp->parser.cfp;
	bool written = false;
	cfp->cur_token = *p_token;

	if (!next_token(cfp))    return false;
	if (!token_is(cfp, "(")) return false;

	dstr_cat(&glsp->gl_string, "(");
	gl_write_function_contents(glsp, &cfp->cur_token, ",");
	dstr_cat(&glsp->gl_string, ") * (");
	next_token(cfp);
	gl_write_function_contents(glsp, &cfp->cur_token, ")");
	dstr_cat(&glsp->gl_string, "))");

	*p_token = cfp->cur_token;
	return true;
}

static bool gl_write_saturate(struct gl_shader_parser *glsp,
		struct cf_token **p_token)
{
	struct cf_parser *cfp = &glsp->parser.cfp;
	bool written = false;
	cfp->cur_token = *p_token;

	if (!next_token(cfp))    return false;
	if (!token_is(cfp, "(")) return false;

	dstr_cat(&glsp->gl_string, "clamp");
	gl_write_function_contents(glsp, &cfp->cur_token, ")");
	dstr_cat(&glsp->gl_string, ", 0.0, 1.0)");

	*p_token = cfp->cur_token;
	return true;
}

static inline bool gl_write_texture_call(struct gl_shader_parser *glsp,
		struct shader_var *var, const char *call)
{
	struct cf_parser *cfp = &glsp->parser.cfp;
	size_t sampler_id = -1;

	if (!next_token(cfp))    return false;
	if (!token_is(cfp, "(")) return false;
	if (!next_token(cfp))    return false;

	sampler_id = sp_getsampler(glsp, cfp->cur_token);
	if (sampler_id == -1) return false;

	var->gl_sampler_id = sampler_id;

	dstr_cat(&glsp->gl_string, call);
	dstr_cat(&glsp->gl_string, "(");
	dstr_cat(&glsp->gl_string, var->name);
	dstr_cat(&glsp->gl_string, ", ");
	return true;
}

/* processes texture.Sample(sampler, texcoord) */
static bool gl_write_texture_code(struct gl_shader_parser *glsp,
		struct cf_token **p_token, struct shader_var *var)
{
	struct cf_parser *cfp = &glsp->parser.cfp;
	bool written = false;
	cfp->cur_token = *p_token;

	if (!next_token(cfp))    return false;
	if (!token_is(cfp, ".")) return false;
	if (!next_token(cfp))    return false;

	if (token_is(cfp, "Sample"))
		written = gl_write_texture_call(glsp, var, "texture");
	else if (token_is(cfp, "SampleBias"))
		written = gl_write_texture_call(glsp, var, "texture");
	else if (token_is(cfp, "SampleGrad"))
		written = gl_write_texture_call(glsp, var, "textureGrad");
	else if (token_is(cfp, "SampleLevel"))
		written = gl_write_texture_call(glsp, var, "textureLod");

	if (!written)
		return false;

	if (!next_token(cfp)) return false;

	gl_write_function_contents(glsp, &cfp->cur_token, ")");
	dstr_cat(&glsp->gl_string, ")");

	*p_token = cfp->cur_token;
	return true;
}

static bool gl_write_intrinsic(struct gl_shader_parser *glsp,
		struct cf_token **p_token)
{
	struct cf_token *token = *p_token;
	bool written = true;

	if (strref_cmp(&token->str, "atan2") == 0) {
		dstr_cat(&glsp->gl_string, "atan2");
	} else if (strref_cmp(&token->str, "ddx") == 0) {
		dstr_cat(&glsp->gl_string, "dFdx");
	} else if (strref_cmp(&token->str, "ddy") == 0) {
		dstr_cat(&glsp->gl_string, "dFdy");
	} else if (strref_cmp(&token->str, "frac") == 0) {
		dstr_cat(&glsp->gl_string, "fract");
	} else if (strref_cmp(&token->str, "lerp") == 0) {
		dstr_cat(&glsp->gl_string, "mix");
	} else if (strref_cmp(&token->str, "rsqrt") == 0) {
		dstr_cat(&glsp->gl_string, "inversesqrt");
	} else if (strref_cmp(&token->str, "saturate") == 0) {
		written = gl_write_saturate(glsp, &token);
	} else if (strref_cmp(&token->str, "mul") == 0) {
		written = gl_write_mul(glsp, &token);
	} else {
		struct shader_var *var = sp_getparam(glsp, token);
		if (var && astrcmp_n(var->type, "texture", 7) == 0)
			written = gl_write_texture_code(glsp, &token, var);
		else
			written = false;
	}

	if (written)
		*p_token = token;
	return written;
}

static void gl_write_function_contents(struct gl_shader_parser *glsp,
		struct cf_token **p_token, const char *end)
{
	struct cf_token *token = *p_token;

	if (token->type != CFTOKEN_NAME
	    || (  !gl_write_type_token(glsp, token)
	       && !gl_write_intrinsic(glsp, &token)))
		dstr_cat_strref(&glsp->gl_string, &token->str);

	while (token->type != CFTOKEN_NONE) {
		token++;

		if (end && strref_cmp(&token->str, end) == 0)
			break;

		if (token->type == CFTOKEN_NAME) {
			if (!gl_write_type_token(glsp, token) &&
			    !gl_write_intrinsic(glsp, &token))
				dstr_cat_strref(&glsp->gl_string, &token->str);

		} else if (token->type == CFTOKEN_OTHER) {
			if (*token->str.array == '{')
				gl_write_function_contents(glsp, &token, "}");
			else if (*token->str.array == '(')
				gl_write_function_contents(glsp, &token, ")");

			dstr_cat_strref(&glsp->gl_string, &token->str);

		} else {
			dstr_cat_strref(&glsp->gl_string, &token->str);
		}
	}

	*p_token = token;
}

static void gl_write_function(struct gl_shader_parser *glsp,
		struct shader_func *func)
{
	size_t i;
	struct cf_token *token;

	gl_write_type(glsp, func->return_type);
	dstr_cat(&glsp->gl_string, " ");

	if (strcmp(func->name, "main") == 0)
		dstr_cat(&glsp->gl_string, "__main__");
	else
		dstr_cat(&glsp->gl_string, func->name);

	dstr_cat(&glsp->gl_string, "(");

	for (i = 0; i < func->params.num; i++) {
		struct shader_var *param = func->params.array+i;

		if (i > 0)
			dstr_cat(&glsp->gl_string, ", ");
		gl_write_var(glsp, param);
	}

	dstr_cat(&glsp->gl_string, ")\n");

	token = func->start;
	gl_write_function_contents(glsp, &token, "}");
	dstr_cat(&glsp->gl_string, "}\n\n");
}

static inline void gl_write_functions(struct gl_shader_parser *glsp)
{
	size_t i;
	for (i = 0; i < glsp->parser.funcs.num; i++) {
		struct shader_func *func = glsp->parser.funcs.array+i;
		gl_write_function(glsp, func);
	}
}

static void gl_write_main_storage_var(struct gl_shader_parser *glsp,
		struct shader_var *var, const char *dst, const char *src,
		bool input)
{
	struct shader_struct *st;
	struct dstr dst_copy = {0};
	char ch_left  = input ? '.' : '_';
	char ch_right = input ? '_' : '.';

	if (dst) {
		dstr_copy(&dst_copy, dst);
		dstr_cat_ch(&dst_copy, ch_left);
	} else {
		dstr_copy(&dst_copy, "\t");
	}

	dstr_cat(&dst_copy, var->name);

	st = shader_parser_getstruct(&glsp->parser, var->type);
	if (st) {
		struct dstr src_copy = {0};
		size_t i;

		if (src)
			dstr_copy(&src_copy, src);
		dstr_cat(&src_copy, var->name);
		dstr_cat_ch(&src_copy, ch_right);

		for (i = 0; i < st->vars.num; i++) {
			struct shader_var *var = st->vars.array+i;
			gl_write_main_storage_var(glsp, var, dst_copy.array,
					src_copy.array, input);
		}

		dstr_free(&src_copy);
	} else {
		if (!dstr_isempty(&dst_copy))
			dstr_cat_dstr(&glsp->gl_string, &dst_copy);
		dstr_cat(&glsp->gl_string, " = ");
		if (src)
			dstr_cat(&glsp->gl_string, src);
		dstr_cat(&glsp->gl_string, var->name);
		dstr_cat(&glsp->gl_string, ";\n");
	}

	dstr_free(&dst_copy);
}

static void gl_write_main_storage_output(struct gl_shader_parser *glsp,
		bool input, const char *type)
{
	struct shader_var var = {0};

	if (input) {
		var.name = "inputval";
		var.type = (char*)type;
		gl_write_main_storage_var(glsp, &var, NULL, NULL, true);
	} else {
		var.name = "outputval";
		var.type = (char*)type;
		gl_write_main_storage_var(glsp, &var, NULL, NULL, false);
	}
}

static void gl_write_main(struct gl_shader_parser *glsp,
		struct shader_func *main)
{
	size_t i;
	dstr_cat(&glsp->gl_string, "void main(void)\n{\n");
	for (i = 0; i < main->params.num; i++) {
		dstr_cat(&glsp->gl_string, "\t");
		dstr_cat(&glsp->gl_string, main->params.array[i].type);
		dstr_cat(&glsp->gl_string, " ");
		dstr_cat(&glsp->gl_string, main->params.array[i].name);
		dstr_cat(&glsp->gl_string, "\n");
	}

	if (!main->mapping) {
		dstr_cat(&glsp->gl_string, "\t");
		dstr_cat(&glsp->gl_string, main->return_type);
		dstr_cat(&glsp->gl_string, " outputval;\n\n");
	}

	/* gl_write_main_storage(glsp, true, main->params.array[0].type); */
	gl_write_main_storage_var(glsp, main->params.array, NULL,
			"inputval_", true);

	dstr_cat(&glsp->gl_string, "\n\toutputval = __main__(");
	for (i = 0; i < main->params.num; i++) {
		if (i)
			dstr_cat(&glsp->gl_string, ", ");
		dstr_cat(&glsp->gl_string, main->params.array[i].name);
	}
	dstr_cat(&glsp->gl_string, ");\n");

	if (!main->mapping) {
		dstr_cat(&glsp->gl_string, "\n");
		gl_write_main_storage_output(glsp, false, main->return_type);
	}

	dstr_cat(&glsp->gl_string, "}\n");
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
	gl_write_main(glsp, main);

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

	if (success)
		success = gl_shader_buildstring(glsp);

	return success;
}
