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

#include "util/platform.h"
#include "util/dstr.h"

#include "obs-defs.h"
#include "obs-data.h"
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

int obs_load_module(const char *path)
{
	struct obs_module mod;
	bool (*module_load)(void) = NULL;

	mod.module = os_dlopen(path);
	if (!mod.module)
		return MODULE_FILENOTFOUND;

	module_load = os_dlsym(mod.module, "module_load");
	if (module_load) {
		if (!module_load()) {
			os_dlclose(mod.module);
			return MODULE_ERROR;
		}
	}

	mod.name = bstrdup(path);
	module_load_exports(&mod, &obs->input_types.da, "inputs",
			sizeof(struct source_info), get_source_info);
	module_load_exports(&mod, &obs->filter_types.da, "filters",
			sizeof(struct source_info), get_source_info);
	module_load_exports(&mod, &obs->transition_types.da, "transitions",
			sizeof(struct source_info), get_source_info);
	module_load_exports(&mod, &obs->output_types.da, "outputs",
			sizeof(struct output_info), get_output_info);

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
