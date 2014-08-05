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

extern const char *get_module_extension(void);

static inline int req_func_not_found(const char *name, const char *path)
{
	blog(LOG_ERROR, "Required module function '%s' in module '%s' not "
	                "found, loading of module failed",
	                name, path);
	return MODULE_MISSING_EXPORTS;
}

static int load_module_exports(struct obs_module *mod, const char *path)
{
	mod->load = os_dlsym(mod->module, "obs_module_load");
	if (!mod->load)
		return req_func_not_found("obs_module_load", path);

	mod->set_pointer = os_dlsym(mod->module, "obs_module_set_pointer");
	if (!mod->set_pointer)
		return req_func_not_found("obs_module_set_pointer", path);

	mod->ver = os_dlsym(mod->module, "obs_module_ver");
	if (!mod->ver)
		return req_func_not_found("obs_module_ver", path);

	/* optional exports */
	mod->unload      = os_dlsym(mod->module, "obs_module_unload");
	mod->set_locale  = os_dlsym(mod->module, "obs_module_set_locale");
	mod->free_locale = os_dlsym(mod->module, "obs_module_free_locale");
	mod->name        = os_dlsym(mod->module, "obs_module_name");
	mod->description = os_dlsym(mod->module, "obs_module_description");
	mod->author      = os_dlsym(mod->module, "obs_module_author");
	return MODULE_SUCCESS;
}

int obs_open_module(obs_module_t *module, const char *path,
		const char *data_path)
{
	struct obs_module mod = {0};
	int errorcode;

	if (!module || !path || !obs)
		return MODULE_ERROR;

	mod.module = os_dlopen(path);
	if (!mod.module) {
		blog(LOG_WARNING, "Module '%s' not found", path);
		return MODULE_FILE_NOT_FOUND;
	}

	errorcode = load_module_exports(&mod, path);
	if (errorcode != MODULE_SUCCESS)
		return errorcode;

	mod.bin_path  = bstrdup(path);
	mod.file      = strrchr(mod.bin_path, '/');
	mod.file      = (!mod.file) ? mod.bin_path : (mod.file + 1);
	mod.data_path = bstrdup(data_path);
	mod.next      = obs->first_module;

	*module = bmemdup(&mod, sizeof(mod));
	obs->first_module = (*module);
	mod.set_pointer(*module);

	if (mod.set_locale)
		mod.set_locale(obs->locale);

	return MODULE_SUCCESS;
}

bool obs_init_module(obs_module_t module)
{
	if (!module || !obs)
		return false;
	if (module->loaded)
		return true;

	module->loaded = module->load();
	if (!module->loaded)
		blog(LOG_WARNING, "Failed to initialize module '%s'",
				module->file);

	return module->loaded;
}

const char *obs_get_module_file_name(obs_module_t module)
{
	return module ? module->file : NULL;
}

const char *obs_get_module_name(obs_module_t module)
{
	return (module && module->name) ? module->name() : NULL;
}

const char *obs_get_module_author(obs_module_t module)
{
	return (module && module->author) ? module->author() : NULL;
}

const char *obs_get_module_description(obs_module_t module)
{
	return (module && module->description) ? module->description() : NULL;
}

char *obs_find_module_file(obs_module_t module, const char *file)
{
	struct dstr output = {0};

	if (!module)
		return NULL;

	dstr_copy(&output, module->data_path);
	if (!dstr_is_empty(&output) && dstr_end(&output) != '/')
		dstr_cat_ch(&output, '/');
	dstr_cat(&output, file);

	if (!os_file_exists(output.array))
		dstr_free(&output);
	return output.array;
}

void obs_add_module_path(const char *bin, const char *data)
{
	struct obs_module_path omp;

	if (!obs || !bin || !data) return;

	omp.bin  = bstrdup(bin);
	omp.data = bstrdup(data);
	da_push_back(obs->module_paths, &omp);
}

static void load_all_callback(void *param, const struct obs_module_info *info)
{
	obs_module_t module;

	int code = obs_open_module(&module, info->bin_path, info->data_path);
	if (code != MODULE_SUCCESS) {
		blog(LOG_DEBUG, "Failed to load module file '%s': %d",
				info->bin_path, code);
		return;
	}

	obs_init_module(module);

	UNUSED_PARAMETER(param);
}

void obs_load_all_modules(void)
{
	obs_find_modules(load_all_callback, NULL);
}

static inline void make_data_dir(struct dstr *parsed_data_dir,
		const char *data_dir, const char *name)
{
	dstr_copy(parsed_data_dir, data_dir);
	dstr_replace(parsed_data_dir, "%module%", name);
	if (dstr_end(parsed_data_dir) == '/')
		dstr_resize(parsed_data_dir, parsed_data_dir->len - 1);
}

static char *make_data_directory(const char *module_name, const char *data_dir)
{
	struct dstr parsed_data_dir = {0};
	bool found = false;

	make_data_dir(&parsed_data_dir, data_dir, module_name);

	found = os_file_exists(parsed_data_dir.array);

	if (!found && astrcmpi_n(module_name, "lib", 3) == 0) {
		make_data_dir(&parsed_data_dir, data_dir, module_name + 3);
		found = os_file_exists(parsed_data_dir.array);
	}

	if (!found)
		dstr_free(&parsed_data_dir);

	return parsed_data_dir.array;
}

static bool parse_binary_from_directory(struct dstr *parsed_bin_path,
		const char *bin_path, const char *file)
{
	struct dstr directory = {0};
	bool found = true;

	dstr_copy(&directory, bin_path);
	dstr_replace(&directory, "%module%", file);
	if (dstr_end(&directory) != '/')
		dstr_cat_ch(&directory, '/');

	dstr_copy_dstr(parsed_bin_path, &directory);
	dstr_cat(parsed_bin_path, file);
	dstr_cat(parsed_bin_path, get_module_extension());

	if (!os_file_exists(parsed_bin_path->array)) {
		/* if the file doesn't exist, check with 'lib' prefix */
		dstr_copy_dstr(parsed_bin_path, &directory);
		dstr_cat(parsed_bin_path, "lib");
		dstr_cat(parsed_bin_path, file);
		dstr_cat(parsed_bin_path, get_module_extension());

		/* if neither exist, don't include this as a library */
		if (!os_file_exists(parsed_bin_path->array)) {
			dstr_free(parsed_bin_path);
			found = false;
		}
	}

	dstr_free(&directory);
	return found;
}

static void process_found_module(struct obs_module_path *omp,
		const char *path, bool directory,
		obs_find_module_callback_t callback, void *param)
{
	struct obs_module_info info;
	struct dstr            name = {0};
	struct dstr            parsed_bin_path = {0};
	const char             *file;
	char                   *parsed_data_dir;
	bool                   bin_found = true;

	file = strrchr(path, '/');
	file = file ? (file + 1) : path;

	dstr_copy(&name, file);
	if (!directory) {
		char *ext = strrchr(name.array, '.');
		if (ext)
			dstr_resize(&name, ext - name.array);

		dstr_copy(&parsed_bin_path, path);
	} else {
		bin_found = parse_binary_from_directory(&parsed_bin_path,
				omp->bin, file);
	}

	parsed_data_dir = make_data_directory(name.array, omp->data);

	if (parsed_data_dir && bin_found) {
		info.bin_path  = parsed_bin_path.array;
		info.data_path = parsed_data_dir;
		callback(param, &info);
	}

	bfree(parsed_data_dir);
	dstr_free(&name);
	dstr_free(&parsed_bin_path);
}

static void find_modules_in_path(struct obs_module_path *omp,
		obs_find_module_callback_t callback, void *param)
{
	struct dstr search_path = {0};
	char *module_start;
	bool search_directories = false;
	os_glob_t gi;

	dstr_copy(&search_path, omp->bin);

	module_start = strstr(search_path.array, "%module%");
	if (module_start) {
		dstr_resize(&search_path, module_start - search_path.array);
		search_directories = true;
	}

	if (!dstr_is_empty(&search_path) && dstr_end(&search_path) != '/')
		dstr_cat_ch(&search_path, '/');

	dstr_cat_ch(&search_path, '*');
	if (!search_directories)
		dstr_cat(&search_path, get_module_extension());

	if (os_glob(search_path.array, 0, &gi) == 0) {
		for (size_t i = 0; i < gi->gl_pathc; i++) {
			if (search_directories == gi->gl_pathv[i].directory)
				process_found_module(omp,
						gi->gl_pathv[i].path,
						search_directories,
						callback, param);
		}

		os_globfree(gi);
	}

	dstr_free(&search_path);
}

void obs_find_modules(obs_find_module_callback_t callback, void *param)
{
	if (!obs)
		return;

	for (size_t i = 0; i < obs->module_paths.num; i++) {
		struct obs_module_path *omp = obs->module_paths.array + i;
		find_modules_in_path(omp, callback, param);
	}
}

void obs_enum_modules(obs_enum_module_callback_t callback, void *param)
{
	struct obs_module *module;
	if (!obs)
		return;

	module = obs->first_module;
	while (module) {
		callback(param, module);
		module = module->next;
	}
}

void free_module(struct obs_module *mod)
{
	if (!mod)
		return;

	if (mod->module) {
		if (mod->free_locale)
			mod->free_locale();

		if (mod->loaded && mod->unload)
			mod->unload();

		os_dlclose(mod->module);
	}

	bfree(mod->bin_path);
	bfree(mod->data_path);
	bfree(mod);
}

lookup_t obs_module_load_locale(obs_module_t module, const char *default_locale,
		const char *locale)
{
	struct dstr str    = {0};
	lookup_t    lookup = NULL;

	if (!module || !default_locale || !locale) {
		blog(LOG_WARNING, "obs_module_load_locale: Invalid parameters");
		return NULL;
	}

	dstr_copy(&str, "locale/");
	dstr_cat(&str, default_locale);
	dstr_cat(&str, ".ini");

	char *file = obs_find_module_file(module, str.array);
	if (file)
		lookup = text_lookup_create(file);

	bfree(file);

	if (!lookup) {
		blog(LOG_WARNING, "Failed to load '%s' text for module: '%s'",
				default_locale, module->file);
		goto cleanup;
	}

	if (astrcmpi(locale, default_locale) == 0)
		goto cleanup;

	dstr_copy(&str, "/locale/");
	dstr_cat(&str, locale);
	dstr_cat(&str, ".ini");

	file = obs_find_module_file(module, str.array);

	if (!text_lookup_add(lookup, file))
		blog(LOG_WARNING, "Failed to load '%s' text for module: '%s'",
				locale, module->file);

	bfree(file);
cleanup:
	dstr_free(&str);
	return lookup;
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

	if (find_source(array, info->id)) {
		blog(LOG_WARNING, "Source d '%s' already exists!  "
		                  "Duplicate library?", info->id);
		return;
	}

	CHECK_REQUIRED_VAL(info, get_name, obs_register_source);
	CHECK_REQUIRED_VAL(info, create,   obs_register_source);
	CHECK_REQUIRED_VAL(info, destroy,  obs_register_source);

	if (info->type == OBS_SOURCE_TYPE_INPUT          &&
	    (info->output_flags & OBS_SOURCE_VIDEO) != 0 &&
	    (info->output_flags & OBS_SOURCE_ASYNC) == 0) {
		CHECK_REQUIRED_VAL(info, get_width,  obs_register_source);
		CHECK_REQUIRED_VAL(info, get_height, obs_register_source);
	}

	memcpy(&data, info, size);

	darray_push_back(sizeof(struct obs_source_info), array, &data);
}

void obs_register_output_s(const struct obs_output_info *info, size_t size)
{
	if (find_output(info->id)) {
		blog(LOG_WARNING, "Output id '%s' already exists!  "
		                  "Duplicate library?", info->id);
		return;
	}

	CHECK_REQUIRED_VAL(info, get_name, obs_register_output);
	CHECK_REQUIRED_VAL(info, create,   obs_register_output);
	CHECK_REQUIRED_VAL(info, destroy,  obs_register_output);
	CHECK_REQUIRED_VAL(info, start,    obs_register_output);
	CHECK_REQUIRED_VAL(info, stop,     obs_register_output);

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
	if (find_encoder(info->id)) {
		blog(LOG_WARNING, "Encoder id '%s' already exists!  "
		                  "Duplicate library?", info->id);
		return;
	}

	CHECK_REQUIRED_VAL(info, get_name, obs_register_encoder);
	CHECK_REQUIRED_VAL(info, create,   obs_register_encoder);
	CHECK_REQUIRED_VAL(info, destroy,  obs_register_encoder);
	CHECK_REQUIRED_VAL(info, encode,   obs_register_encoder);

	if (info->type == OBS_ENCODER_AUDIO)
		CHECK_REQUIRED_VAL(info, get_frame_size, obs_register_encoder);

	REGISTER_OBS_DEF(size, obs_encoder_info, obs->encoder_types, info);
}

void obs_register_service_s(const struct obs_service_info *info, size_t size)
{
	if (find_service(info->id)) {
		blog(LOG_WARNING, "Service id '%s' already exists!  "
		                  "Duplicate library?", info->id);
		return;
	}

	CHECK_REQUIRED_VAL(info, get_name, obs_register_service);
	CHECK_REQUIRED_VAL(info, create,   obs_register_service);
	CHECK_REQUIRED_VAL(info, destroy,  obs_register_service);

	REGISTER_OBS_DEF(size, obs_service_info, obs->service_types, info);
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
