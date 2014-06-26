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

#include "effect.h"
#include "graphics-internal.h"
#include "vec2.h"
#include "vec3.h"
#include "vec4.h"

void effect_destroy(effect_t effect)
{
	if (effect) {
		effect_free(effect);
		bfree(effect);
	}
}

technique_t effect_gettechnique(effect_t effect, const char *name)
{
	if (!effect) return NULL;

	for (size_t i = 0; i < effect->techniques.num; i++) {
		struct effect_technique *tech = effect->techniques.array+i;
		if (strcmp(tech->name, name) == 0)
			return tech;
	}

	return NULL;
}

size_t technique_begin(technique_t tech)
{
	if (!tech) return 0;

	tech->effect->cur_technique = tech;
	tech->effect->graphics->cur_effect = tech->effect;

	return tech->passes.num;
}

void technique_end(technique_t tech)
{
	if (!tech) return;

	struct gs_effect *effect = tech->effect;
	struct effect_param *params = effect->params.array;
	size_t i;

	gs_load_vertexshader(NULL);
	gs_load_pixelshader(NULL);

	tech->effect->cur_technique = NULL;
	tech->effect->graphics->cur_effect = NULL;

	for (i = 0; i < effect->params.num; i++) {
		struct effect_param *param = params+i;

		da_free(param->cur_val);
		param->changed = false;
	}
}

static inline void reset_params(struct darray *shaderparams)
{
	struct pass_shaderparam *params = shaderparams->array;
	size_t i;

	for (i = 0; i < shaderparams->num; i++)
		params[i].eparam->changed = false;
}

static void upload_shader_params(shader_t shader, struct darray *pass_params,
		bool changed_only)
{
	struct pass_shaderparam *params = pass_params->array;
	size_t i;

	for (i = 0; i < pass_params->num; i++) {
		struct pass_shaderparam *param = params+i;
		struct effect_param *eparam = param->eparam;
		sparam_t sparam = param->sparam;

		if (changed_only && !eparam->changed)
			continue;

		if (!eparam->cur_val.num) {
			if (eparam->default_val.num)
				da_copy(eparam->cur_val, eparam->default_val);
			else
				continue;
		}

		shader_setval(sparam, eparam->cur_val.array,
				eparam->cur_val.num);
	}
}

static inline void upload_parameters(struct gs_effect *effect,
		bool changed_only)
{
	struct darray *vshader_params, *pshader_params;

	if (!effect->cur_pass)
		return;

	vshader_params = &effect->cur_pass->vertshader_params.da;
	pshader_params = &effect->cur_pass->pixelshader_params.da;

	upload_shader_params(effect->cur_pass->vertshader, vshader_params,
			changed_only);
	upload_shader_params(effect->cur_pass->pixelshader, pshader_params,
			changed_only);
	reset_params(vshader_params);
	reset_params(pshader_params);
}

void effect_updateparams(effect_t effect)	
{
	if (effect)
		upload_parameters(effect, true);
}

bool technique_beginpass(technique_t tech, size_t idx)
{
	struct effect_pass *passes;
	struct effect_pass *cur_pass;

	if (!tech || idx >= tech->passes.num)
		return false;

	passes = tech->passes.array;
	cur_pass = passes+idx;

	tech->effect->cur_pass = cur_pass;
	gs_load_vertexshader(cur_pass->vertshader);
	gs_load_pixelshader(cur_pass->pixelshader);
	upload_parameters(tech->effect, false);

	return true;
}

bool technique_beginpassbyname(technique_t tech,
		const char *name)
{
	if (!tech)
		return false;

	for (size_t i = 0; i < tech->passes.num; i++) {
		struct effect_pass *pass = tech->passes.array+i;
		if (strcmp(pass->name, name) == 0) {
			technique_beginpass(tech, i);
			return true;
		}
	}

	return false;
}

static inline void clear_tex_params(shader_t shader, struct darray *in_params)
{
	struct pass_shaderparam *params = in_params->array;

	for (size_t i = 0; i < in_params->num; i++) {
		struct pass_shaderparam *param = params+i;
		struct shader_param_info info;

		shader_getparaminfo(param->sparam, &info);
		if (info.type == SHADER_PARAM_TEXTURE)
			shader_settexture(param->sparam, NULL);
	}
}

void technique_endpass(technique_t tech)
{
	if (!tech) return;

	struct effect_pass *pass = tech->effect->cur_pass;
	if (!pass)
		return;

	clear_tex_params(pass->vertshader, &pass->vertshader_params.da);
	clear_tex_params(pass->pixelshader, &pass->pixelshader_params.da);
	tech->effect->cur_pass = NULL;
}

size_t effect_numparams(effect_t effect)
{
	return effect ? effect->params.num : 0;
}

eparam_t effect_getparambyidx(effect_t effect, size_t param)
{
	if (!effect) return NULL;

	struct effect_param *params = effect->params.array;
	if (param >= effect->params.num)
		return NULL;

	return params+param;
}

eparam_t effect_getparambyname(effect_t effect, const char *name)
{
	if (!effect) return NULL;

	struct effect_param *params = effect->params.array;

	for (size_t i = 0; i < effect->params.num; i++) {
		struct effect_param *param = params+i;

		if (strcmp(param->name, name) == 0)
			return param;
	}

	return NULL;
}

static inline bool matching_effect(effect_t effect, eparam_t param)
{
	if (effect != param->effect) {
		blog(LOG_ERROR, "Effect and effect parameter do not match");
		return false;
	}

	return true;
}

void effect_getparaminfo(effect_t effect, eparam_t param,
		struct effect_param_info *info)
{
	if (!effect || !param)
		return;

	if (!matching_effect(effect, param))
		return;

	info->name = param->name;
	info->type = param->type;
}

eparam_t effect_getviewprojmatrix(effect_t effect)
{
	return effect ? effect->view_proj : NULL;
}

eparam_t effect_getworldmatrix(effect_t effect)
{
	return effect ? effect->world : NULL;
}

static inline void effect_setval_inline(effect_t effect, eparam_t param,
		const void *data, size_t size)
{
	bool size_changed;

	if (!effect) {
		blog(LOG_ERROR, "effect_setval_inline: invalid effect");
		return;
	}

	if (!param) {
		blog(LOG_ERROR, "effect_setval_inline: invalid param");
		return;
	}

	if (!data) {
		blog(LOG_ERROR, "effect_setval_inline: invalid data");
		return;
	}

	size_changed = param->cur_val.num != size;
	if (!matching_effect(effect, param))
		return;

	if (size_changed)
		da_resize(param->cur_val, size);

	if (size_changed || memcmp(param->cur_val.array, data, size) != 0) {
		memcpy(param->cur_val.array, data, size);
		param->changed = true;
	}
}

void effect_setbool(effect_t effect, eparam_t param, bool val)
{
	effect_setval_inline(effect, param, &val, sizeof(bool));
}

void effect_setfloat(effect_t effect, eparam_t param, float val)
{
	effect_setval_inline(effect, param, &val, sizeof(float));
}

void effect_setint(effect_t effect, eparam_t param, int val)
{
	effect_setval_inline(effect, param, &val, sizeof(int));
}

void effect_setmatrix4(effect_t effect, eparam_t param,
		const struct matrix4 *val)
{
	effect_setval_inline(effect, param, val, sizeof(struct matrix4));
}

void effect_setvec2(effect_t effect, eparam_t param,
		const struct vec2 *val)
{
	effect_setval_inline(effect, param, val, sizeof(struct vec2));
}

void effect_setvec3(effect_t effect, eparam_t param,
		const struct vec3 *val)
{
	effect_setval_inline(effect, param, val, sizeof(float) * 3);
}

void effect_setvec4(effect_t effect, eparam_t param,
		const struct vec4 *val)
{
	effect_setval_inline(effect, param, val, sizeof(struct vec4));
}

void effect_settexture(effect_t effect, eparam_t param, texture_t val)
{
	effect_setval_inline(effect, param, &val, sizeof(texture_t));
}

void effect_setval(effect_t effect, eparam_t param, const void *val,
		size_t size)
{
	effect_setval_inline(effect, param, val, size);
}

void effect_setdefault(effect_t effect, eparam_t param)
{
	effect_setval_inline(effect, param, param->default_val.array,
			param->default_val.num);
}
