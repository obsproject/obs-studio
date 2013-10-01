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

#ifndef EFFECT_H
#define EFFECT_H

#include "effect-parser.h"
#include "graphics.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Effects introduce a means of bundling together shader text into one
 * file with shared functions and parameters.  This is done because often
 * shaders must be duplicated when you need to alter minor aspects of the code
 * that cannot be done via constants.  Effects allow developers to easily
 * switch shaders and set constants that can be used between shaders.
 *
 * Effects are built via the effect parser, and shaders are automatically
 * generated for each technique's pass.
 */

/* ------------------------------------------------------------------------- */

enum effect_section {
	EFFECT_PARAM,
	EFFECT_TECHNIQUE,
	EFFECT_SAMPLER,
	EFFECT_PASS
};

/* ------------------------------------------------------------------------- */

struct effect_param {
	char *name;
	enum effect_section section;

	enum shader_param_type type;

	bool changed;
	DARRAY(uint8_t) cur_val;
	DARRAY(uint8_t) default_val;

	/*char *full_name;
	float scroller_min, scroller_max, scroller_inc, scroller_mul;*/
};

static inline void effect_param_init(struct effect_param *param)
{
	memset(param, 0, sizeof(struct effect_param));
}

static inline void effect_param_free(struct effect_param *param)
{
	bfree(param->name);
	//bfree(param->full_name);
	da_free(param->cur_val);
	da_free(param->default_val);
}

EXPORT void effect_param_parse_property(eparam_t param,
		const char *property);

/* ------------------------------------------------------------------------- */

struct pass_shaderparam {
	struct effect_param *eparam;
	sparam_t sparam;
};

struct effect_pass {
	char *name;
	enum effect_section section;

	shader_t vertshader;
	shader_t pixelshader;
	DARRAY(struct pass_shaderparam) vertshader_params;
	DARRAY(struct pass_shaderparam) pixelshader_params;
};

static inline void effect_pass_init(struct effect_pass *pass)
{
	memset(pass, 0, sizeof(struct effect_pass));
}

static inline void effect_pass_free(struct effect_pass *pass)
{
	bfree(pass->name);
	da_free(pass->vertshader_params);
	da_free(pass->pixelshader_params);
	shader_destroy(pass->vertshader);
	shader_destroy(pass->pixelshader);
}

/* ------------------------------------------------------------------------- */

struct effect_technique {
	char *name;
	enum effect_section section;
	struct gs_effect *effect;

	DARRAY(struct effect_pass) passes;
};

static inline void effect_technique_init(struct effect_technique *t)
{
	memset(t, 0, sizeof(struct effect_technique));
}

static inline void effect_technique_free(struct effect_technique *t)
{
	size_t i;
	for (i = 0; i < t->passes.num; i++)
		effect_pass_free(t->passes.array+i);
	da_free(t->passes);
	bfree(t->name);
}

/* ------------------------------------------------------------------------- */

struct gs_effect {
	bool processing;
	char *effect_path, *effect_dir;

	DARRAY(struct effect_param) params;
	DARRAY(struct effect_technique) techniques;

	struct effect_technique *cur_technique;
	struct effect_pass *cur_pass;

	eparam_t view_proj, world, scale;
	graphics_t graphics;
};

static inline void effect_init(effect_t effect)
{
	memset(effect, 0, sizeof(struct gs_effect));
}

static inline void effect_free(effect_t effect)
{
	size_t i;
	for (i = 0; i < effect->params.num; i++)
		effect_param_free(effect->params.array+i);
	for (i = 0; i < effect->techniques.num; i++)
		effect_technique_free(effect->techniques.array+i);

	da_free(effect->params);
	da_free(effect->techniques);

	bfree(effect->effect_path);
	bfree(effect->effect_dir);
	effect->effect_path = NULL;
	effect->effect_dir = NULL;
}

EXPORT void effect_upload_params(effect_t effect, bool changed_only);
EXPORT void effect_upload_shader_params(effect_t effect, shader_t shader,
		struct darray *pass_params, bool changed_only);

#ifdef __cplusplus
}
#endif

#endif
