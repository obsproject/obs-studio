/******************************************************************************
    Copyright (C) 2013-2014 by Hugh Bailey <obs.jim@gmail.com>

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

	obs_module_load = os_dlsym(module, "obs_module_load");
	if (!obs_module_load)
		return req_func_not_found("obs_module_load", path);

	if (!obs_module_load(LIBOBS_API_VER)) {
		blog(LOG_ERROR, "Module '%s' failed to load: "
		                "obs_module_load failed", path);
		return MODULE_ERROR;
	}

	return MODULE_SUCCESS;
}

int obs_load_module(const char *path)
{
	struct obs_module mod;
	char *plugin_path = find_plugin(path);
	int errorcode;

	mod.module = os_dlopen(plugin_path);
	bfree(plugin_path);
	if (!mod.module) {
		blog(LOG_DEBUG, "Module '%s' not found", path);
		return MODULE_FILE_NOT_FOUND;
	}

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

void obs_register_source_s(const struct obs_source_info *info, size_t size)
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

	memcpy(&data, info, size);

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

void obs_register_output_s(const struct obs_output_info *info, size_t size)
{
	CHECK_REQUIRED_VAL(info, getname, obs_register_output);
	CHECK_REQUIRED_VAL(info, create,  obs_register_output);
	CHECK_REQUIRED_VAL(info, destroy, obs_register_output);
	CHECK_REQUIRED_VAL(info, start,   obs_register_output);
	CHECK_REQUIRED_VAL(info, stop,    obs_register_output);

	if (info->flags & OBS_OUTPUT_ENCODED) {
		CHECK_REQUIRED_VAL(info, encoded_packet, obs_register_output);
	} else {
		if (info->flags & OBS_OUTPUT_VIDEO)
			CHECK_REQUIRED_VAL(info, raw_video,
					obs_register_output);

		if (info->flags & OBS_OUTPUT_AUDIO)
			CHECK_REQUIRED_VAL(info, raw_audio,
					obs_register_output);
	}

	REGISTER_OBS_DEF(size, obs_output_info, obs->output_types, info);
}

void obs_register_encoder_s(const struct obs_encoder_info *info, size_t size)
{
	CHECK_REQUIRED_VAL(info, getname,    obs_register_encoder);
	CHECK_REQUIRED_VAL(info, create,     obs_register_encoder);
	CHECK_REQUIRED_VAL(info, destroy,    obs_register_encoder);
	CHECK_REQUIRED_VAL(info, encode,     obs_register_encoder);

	if (info->type == OBS_ENCODER_AUDIO)
		CHECK_REQUIRED_VAL(info, frame_size, obs_register_encoder);

	REGISTER_OBS_DEF(size, obs_encoder_info, obs->encoder_types, info);
}

void obs_register_service_s(const struct obs_service_info *info, size_t size)
{
	/* TODO */
	UNUSED_PARAMETER(size);
	UNUSED_PARAMETER(info);
}

void obs_regsiter_modal_ui_s(const struct obs_modal_ui *info, size_t size)
{
	CHECK_REQUIRED_VAL(info, task,   obs_regsiter_modal_ui);
	CHECK_REQUIRED_VAL(info, target, obs_regsiter_modal_ui);
	CHECK_REQUIRED_VAL(info, exec,   obs_regsiter_modal_ui);

	REGISTER_OBS_DEF(size, obs_modal_ui, obs->modal_ui_callbacks, info);
}

void obs_regsiter_modeless_ui_s(const struct obs_modeless_ui *info, size_t size)
{
	CHECK_REQUIRED_VAL(info, task,   obs_regsiter_modeless_ui);
	CHECK_REQUIRED_VAL(info, target, obs_regsiter_modeless_ui);
	CHECK_REQUIRED_VAL(info, create, obs_regsiter_modeless_ui);

	REGISTER_OBS_DEF(size, obs_modeless_ui, obs->modeless_ui_callbacks,
			info);
}
