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

#include "util/platform.h"
#include "util/dstr.h"

#include "obs-defs.h"
#include "obs-internal.h"
#include "obs-module.h"

extern char *find_plugin(const char *plugin);

/* These variables get the current size of the info structures.  Used to
 * automatically prevent API breakage in case functions have to be added */
static size_t cur_source_info_size  = 0;
static size_t cur_output_info_size  = 0;
static size_t cur_encoder_info_size = 0;
static size_t cur_service_info_size = 0;
static size_t cur_modal_ui_size     = 0;
static size_t cur_modeless_ui_size  = 0;

static inline int req_func_not_found(const char *name, const char *path)
{
	blog(LOG_ERROR, "Required module function '%s' in module '%s' not "
	                "found, loading of module failed",
	                name, path);
	return MODULE_FUNCTION_NOT_FOUND;
}

#define LOAD_REQ_SIZE_FUNC(func, module, path)         \
	func = os_dlsym(module, #func);                \
	if (!func)                                     \
		return req_func_not_found(#func, path)

static int call_module_load(void *module, const char *path)
{
	bool   (*obs_module_load)(uint32_t obs_ver)  = NULL;
	size_t (*obs_module_source_info_size)(void)  = NULL;
	size_t (*obs_module_output_info_size)(void)  = NULL;
	size_t (*obs_module_encoder_info_size)(void) = NULL;
	size_t (*obs_module_service_info_size)(void) = NULL;
	size_t (*obs_module_modal_ui_size)(void)     = NULL;
	size_t (*obs_module_modeless_ui_size)(void)  = NULL;

	obs_module_load = os_dlsym(module, "obs_module_load");
	if (!obs_module_load)
		return req_func_not_found("obs_module_load", path);

	LOAD_REQ_SIZE_FUNC(obs_module_source_info_size,  module, path);
	LOAD_REQ_SIZE_FUNC(obs_module_output_info_size,  module, path);
	LOAD_REQ_SIZE_FUNC(obs_module_encoder_info_size, module, path);
	LOAD_REQ_SIZE_FUNC(obs_module_service_info_size, module, path);
	LOAD_REQ_SIZE_FUNC(obs_module_modal_ui_size,     module, path);
	LOAD_REQ_SIZE_FUNC(obs_module_modeless_ui_size,  module, path);

	cur_source_info_size  = obs_module_source_info_size();
	cur_output_info_size  = obs_module_output_info_size();
	cur_encoder_info_size = obs_module_encoder_info_size();
	cur_service_info_size = obs_module_service_info_size();
	cur_modal_ui_size     = obs_module_modal_ui_size();
	cur_modeless_ui_size  = obs_module_modeless_ui_size();

	if (!obs_module_load(LIBOBS_API_VER)) {
		blog(LOG_ERROR, "Module '%s' failed to load: "
		                "obs_module_load failed", path);
		return MODULE_ERROR;
	}

	cur_source_info_size  = 0;
	cur_output_info_size  = 0;
	cur_encoder_info_size = 0;
	cur_service_info_size = 0;
	cur_modal_ui_size     = 0;
	cur_modeless_ui_size  = 0;

	return MODULE_SUCCESS;
}

int obs_load_module(const char *path)
{
	struct obs_module mod;
	char *plugin_path = find_plugin(path);
	int errorcode;

	mod.module = os_dlopen(plugin_path);
	bfree(plugin_path);
	if (!mod.module)
		return MODULE_FILE_NOT_FOUND;

	errorcode = call_module_load(mod.module, path);
	if (errorcode != MODULE_SUCCESS) {
		os_dlclose(mod.module);
		return errorcode;
	}

	mod.name = bstrdup(path);
	da_push_back(obs->modules, &mod);
	return MODULE_SUCCESS;
}

void free_module(struct obs_module *mod)
{
	if (!mod)
		return;

	if (mod->module) {
		void (*module_unload)(void);

		module_unload = os_dlsym(mod->module, "module_unload");
		if (module_unload)
			module_unload();

		os_dlclose(mod->module);
	}

	bfree(mod->name);
}

#define REGISTER_OBS_DEF(size_var, structure, dest, info)                 \
	do {                                                              \
		struct structure data = {0};                              \
		if (!size_var) {                                          \
			blog(LOG_ERROR, "Tried to register " #structure   \
			               " outside of obs_module_load");    \
			return;                                           \
		}                                                         \
                                                                          \
		memcpy(&data, info, size_var);                            \
		da_push_back(dest, &data);                                \
	} while (false)

#define CHECK_REQUIRED_VAL(info, val, func) \
	do { \
		if (!info->val) {\
			blog(LOG_ERROR, "Required value '" #val " for" \
			                "'%s' not found.  " #func \
			                " failed.", info->id); \
			return; \
		} \
	} while (false)

void obs_register_source(const struct obs_source_info *info)
{
	struct obs_source_info data = {0};
	struct darray *array;

	CHECK_REQUIRED_VAL(info, getname,   obs_register_source);
	CHECK_REQUIRED_VAL(info, create,    obs_register_source);
	CHECK_REQUIRED_VAL(info, destroy,   obs_register_source);

	if (info->type == OBS_SOURCE_TYPE_INPUT &&
	    info->output_flags & OBS_SOURCE_VIDEO) {
		CHECK_REQUIRED_VAL(info, getwidth,  obs_register_source);
		CHECK_REQUIRED_VAL(info, getheight, obs_register_source);
	}

	if (!cur_source_info_size) {
		blog(LOG_ERROR, "Tried to register obs_source_info"
		                " outside of obs_module_load");
		return;
	}

	memcpy(&data, info, cur_source_info_size);

	if (info->type == OBS_SOURCE_TYPE_INPUT) {
		array = &obs->input_types.da;
	} else if (info->type == OBS_SOURCE_TYPE_FILTER) {
		array = &obs->filter_types.da;
	} else if (info->type == OBS_SOURCE_TYPE_TRANSITION) {
		array = &obs->transition_types.da;
	} else {
		blog(LOG_ERROR, "Tried to register unknown source type: %u",
				info->type);
		return;
	}

	darray_push_back(sizeof(struct obs_source_info), array, &data);
}

void obs_register_output(const struct obs_output_info *info)
{
	CHECK_REQUIRED_VAL(info, getname, obs_register_output);
	CHECK_REQUIRED_VAL(info, create,  obs_register_output);
	CHECK_REQUIRED_VAL(info, destroy, obs_register_output);
	CHECK_REQUIRED_VAL(info, start,   obs_register_output);
	CHECK_REQUIRED_VAL(info, stop,    obs_register_output);
	CHECK_REQUIRED_VAL(info, active,  obs_register_output);

	REGISTER_OBS_DEF(cur_output_info_size, obs_output_info,
			obs->output_types, info);
}

void obs_register_encoder(const struct obs_encoder_info *info)
{
	CHECK_REQUIRED_VAL(info, getname,   obs_register_encoder);
	CHECK_REQUIRED_VAL(info, create,    obs_register_encoder);
	CHECK_REQUIRED_VAL(info, destroy,   obs_register_encoder);
	CHECK_REQUIRED_VAL(info, start,     obs_register_encoder);
	CHECK_REQUIRED_VAL(info, encode,    obs_register_encoder);

	REGISTER_OBS_DEF(cur_encoder_info_size, obs_encoder_info,
			obs->encoder_types, info);
}

void obs_register_service(const struct obs_service_info *info)
{
	/* TODO */
	UNUSED_PARAMETER(info);
}

void obs_regsiter_modal_ui(const struct obs_modal_ui *info)
{
	CHECK_REQUIRED_VAL(info, task,   obs_regsiter_modal_ui);
	CHECK_REQUIRED_VAL(info, target, obs_regsiter_modal_ui);
	CHECK_REQUIRED_VAL(info, exec,   obs_regsiter_modal_ui);

	REGISTER_OBS_DEF(cur_modal_ui_size, obs_modal_ui,
			obs->modal_ui_callbacks, info);
}

void obs_regsiter_modeless_ui(const struct obs_modeless_ui *info)
{
	CHECK_REQUIRED_VAL(info, task,   obs_regsiter_modeless_ui);
	CHECK_REQUIRED_VAL(info, target, obs_regsiter_modeless_ui);
	CHECK_REQUIRED_VAL(info, create, obs_regsiter_modeless_ui);

	REGISTER_OBS_DEF(cur_modeless_ui_size, obs_modeless_ui,
			obs->modeless_ui_callbacks, info);
}
