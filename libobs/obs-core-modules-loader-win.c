#include "obs-core-modules.h"

#include <obs-internal.h>
#include <obs.h>
#include <util/dstr.h>
#include <util/platform.h>

const char *core_module_bin = "../../core/%module%/%module%";
const char *core_module_data = "../../core/%module%/data";

extern bool find_core_module(struct obs_runtime_module_info *info, obs_find_module_callback2_t callback, void *data);

void load_core_modules(obs_find_module_callback2_t callback, void *data)
{
	char *core_bin_path = os_get_abs_path_ptr(core_module_bin);
	char *core_data_path = os_get_abs_path_ptr(core_module_data);

	for (unsigned int i = 0; i < obs_core_modules_count; i++) {
		const char *name = obs_core_modules[i];
		struct dstr bin_path = {0};
		struct dstr data_path = {0};
		dstr_init_copy(&bin_path, core_bin_path);
		dstr_init_copy(&data_path, core_data_path);
		dstr_replace(&bin_path, "%module%", name);
		dstr_replace(&data_path, "%module%", name);

		// Convert windows backslash to forward slash
		dstr_replace(&bin_path, "\\", "/");
		dstr_replace(&data_path, "\\", "/");

		struct obs_runtime_module_info module_info = {.path_info = {.binary = bin_path.array,
									    .data = data_path.array},
							      .type = MODULE_TYPE_CORE,
							      .name = name};

		if (!find_core_module(&module_info, callback, data)) {
			blog(LOG_ERROR, "Failed to load core module %s", name);
			return;
		}

		dstr_free(&bin_path);
		dstr_free(&data_path);
	}

	bfree(core_bin_path);
	bfree(core_data_path);
}
