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

void *load_module_subfunc(void *module, const char *module_name,
		const char *name, const char *func, bool required)
{
	struct dstr func_name;
	void *func_addr = NULL;

	dstr_init_copy(&func_name, name);
	dstr_cat(&func_name, "_");
	dstr_cat(&func_name, func);

	func_addr = os_dlsym(module, func_name.array);
	if (required && !func_addr)
		blog(LOG_ERROR, "Could not load function '%s' from module '%s'",
				func_name.array, module_name);

	dstr_free(&func_name);
	return func_addr;
}

static void module_load_exports(struct obs_module *mod,
		struct darray *output_array, const char *type,
		const size_t data_size, void *callback_ptr)
{
	bool (*enum_func)(size_t idx, const char **name);
	bool (*callback)(void*, const char*, const char*, void*);
	struct dstr enum_name;
	const char *name;
	size_t i = 0;

	callback = callback_ptr;

	dstr_init_copy(&enum_name, "enum_");
	dstr_cat(&enum_name, type);

	enum_func = os_dlsym(mod->module, enum_name.array);
	if (!enum_func)
		goto complete;

	while (enum_func(i++, &name)) {
		void *info = bmalloc(data_size);
		if (!callback(mod->module, mod->name, name, info))
			blog(LOG_ERROR, "Couldn't load '%s' because it "
			                "was missing required functions",
			                name);
		else
			darray_push_back(data_size, output_array, info);

		bfree(info);
	}

complete:
	dstr_free(&enum_name);
}

extern char *find_plugin(const char *plugin);

/* checks API version of module and calls module_load if it exists.
 * if the API version used by the module is incompatible, fails. */
static int call_module_load(void *module, const char *path)
{
	uint32_t (*module_version)(uint32_t obs_ver) = NULL;
	bool (*module_load)(void) = NULL;
	uint32_t version, major, minor;

	module_load = os_dlsym(module, "module_load");

	module_version = os_dlsym(module, "module_version");
	if (!module_version) {
		blog(LOG_WARNING, "Module '%s' failed to load: "
		                  "module_version not found.", path);
		return MODULE_FUNCTIONNOTFOUND;
	}

	version = module_version(LIBOBS_API_VER);
	major = (version >> 16);
	minor = (version & 0xFF);

	if (major != LIBOBS_API_MAJOR_VER) {
		blog(LOG_WARNING, "Module '%s' failed to load: "
		                  "incompatible major version "
		                  "(current API: %u.%u, module version: %u.%u)",
		                  path,
		                  LIBOBS_API_MAJOR_VER, LIBOBS_API_MINOR_VER,
		                  major, minor);
		return MODULE_INCOMPATIBLE_VER;
	}

	if (minor > LIBOBS_API_MINOR_VER) {
		blog(LOG_WARNING, "Module '%s' failed to load: "
		                  "incompatible minor version "
		                  "(current API: %u.%u, module version: %u.%u)",
		                  path,
		                  LIBOBS_API_MAJOR_VER, LIBOBS_API_MINOR_VER,
		                  major, minor);
		return MODULE_INCOMPATIBLE_VER;
	}

	if (module_load && !module_load()) {
		blog(LOG_WARNING, "Module '%s' failed to load: "
		                  "module_load failed", path);
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
	if (!mod.module)
		return MODULE_FILENOTFOUND;

	errorcode = call_module_load(mod.module, path);
	if (errorcode != MODULE_SUCCESS) {
		os_dlclose(mod.module);
		return errorcode;
	}

	mod.name = bstrdup(path);
	module_load_exports(&mod, &obs->input_types.da, "inputs",
			sizeof(struct source_info), load_source_info);
	module_load_exports(&mod, &obs->filter_types.da, "filters",
			sizeof(struct source_info), load_source_info);
	module_load_exports(&mod, &obs->transition_types.da, "transitions",
			sizeof(struct source_info), load_source_info);
	module_load_exports(&mod, &obs->output_types.da, "outputs",
			sizeof(struct output_info), load_output_info);
	module_load_exports(&mod, &obs->encoder_types.da, "encoders",
			sizeof(struct encoder_info), load_encoder_info);

	da_push_back(obs->modules, &mod);
	return MODULE_SUCCESS;
}

void free_module(struct obs_module *mod)
{
	if (!mod)
		return;

	if (mod->module) {
		void (*module_unload)(void);

		module_unload = os_dlsym(mod->module,
				"module_unload");
		if (module_unload)
			module_unload();

		os_dlclose(mod->module);
	}

	bfree(mod->name);
}
