/******************************************************************************
    Copyright (C) 2023-2024 by OBS Project

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

#include "vulkan-shader-manager.h"
#include "vulkan-shader-compiler.h"
#include <util/blogging.h>
#include <util/mem.h>
#include <util/darray.h>
#include <util/threading.h>
#include <util/platform.h>
#include <util/c99defs.h>
#include <string.h>

vk_shader_manager_t *vk_shader_manager_create(VkDevice device)
{
	vk_shader_manager_t *manager = bzalloc(sizeof(vk_shader_manager_t));
	manager->device = device;
	da_init(manager->shader_cache);
	pthread_mutex_init(&manager->lock, NULL);

	blog(LOG_INFO, "Created Vulkan shader manager");
	return manager;
}

void vk_shader_manager_destroy(vk_shader_manager_t *manager)
{
	if (!manager) {
		return;
	}

	pthread_mutex_lock(&manager->lock);

	for (size_t i = 0; i < da_count(manager->shader_cache); i++) {
		vk_shader_cache_entry_t *entry = &da_array(manager->shader_cache)[i];
		if (entry->module != VK_NULL_HANDLE) {
			vkDestroyShaderModule(manager->device, entry->module, NULL);
		}
		bfree(entry->name);
	}

	da_free(manager->shader_cache);
	pthread_mutex_unlock(&manager->lock);
	pthread_mutex_destroy(&manager->lock);

	bfree(manager);
	blog(LOG_INFO, "Destroyed Vulkan shader manager");
}

static vk_shader_cache_entry_t *find_shader(vk_shader_manager_t *manager, const char *name)
{
	for (size_t i = 0; i < da_count(manager->shader_cache); i++) {
		vk_shader_cache_entry_t *entry = &da_array(manager->shader_cache)[i];
		if (strcmp(entry->name, name) == 0) {
			return entry;
		}
	}
	return NULL;
}

VkShaderModule vk_shader_manager_load_glsl(vk_shader_manager_t *manager, const char *name,
					   const char *glsl_source, VkShaderStageFlagBits stage)
{
	if (!manager || !name || !glsl_source) {
		return VK_NULL_HANDLE;
	}

	pthread_mutex_lock(&manager->lock);

	vk_shader_cache_entry_t *cached = find_shader(manager, name);
	if (cached) {
		cached->ref_count++;
		VkShaderModule module = cached->module;
		pthread_mutex_unlock(&manager->lock);
		blog(LOG_DEBUG, "Shader '%s' found in cache (ref_count=%u)", name, cached->ref_count);
		return module;
	}

	vk_shader_compile_options_t options = {
		.stage = stage,
		.entry_point = "main",
		.optimize = true,
	};

	vk_shader_compile_result_t result = vk_shader_compile_glsl(glsl_source, 0, &options);

	if (!result.success) {
		blog(LOG_ERROR, "Failed to compile shader '%s': %s", name,
		     result.error_message ? result.error_message : "unknown error");
		vk_shader_compile_result_free(&result);
		pthread_mutex_unlock(&manager->lock);
		return VK_NULL_HANDLE;
	}

	VkShaderModule module;
	VkResult vk_result = vk_shader_module_create_from_spirv(manager->device, result.bytecode,
							        result.bytecode_size, &module);

	vk_shader_compile_result_free(&result);

	if (vk_result != VK_SUCCESS) {
		blog(LOG_ERROR, "Failed to create shader module for '%s': %d", name, vk_result);
		pthread_mutex_unlock(&manager->lock);
		return VK_NULL_HANDLE;
	}

	vk_shader_cache_entry_t entry = {
		.name = bstrdup(name),
		.module = module,
		.stage = stage,
		.ref_count = 1,
	};

	da_push_back(manager->shader_cache, &entry);

	blog(LOG_INFO, "Compiled and cached shader '%s' (stage=%d)", name, stage);

	pthread_mutex_unlock(&manager->lock);
	return module;
}

VkShaderModule vk_shader_manager_load_file(vk_shader_manager_t *manager, const char *name,
					   const char *filename)
{
	if (!manager || !name || !filename) {
		return VK_NULL_HANDLE;
	}

	pthread_mutex_lock(&manager->lock);

	vk_shader_cache_entry_t *cached = find_shader(manager, name);
	if (cached) {
		cached->ref_count++;
		VkShaderModule module = cached->module;
		pthread_mutex_unlock(&manager->lock);
		return module;
	}

	VkShaderStageFlagBits stage = vk_shader_get_stage_from_filename(filename);
	if (stage == 0) {
		blog(LOG_ERROR, "Cannot determine shader stage from filename: %s", filename);
		pthread_mutex_unlock(&manager->lock);
		return VK_NULL_HANDLE;
	}

	vk_shader_compile_options_t options = {
		.stage = stage,
		.entry_point = "main",
		.optimize = true,
	};

	vk_shader_compile_result_t result = vk_shader_compile_glsl_file(filename, &options);

	if (!result.success) {
		blog(LOG_ERROR, "Failed to compile shader file '%s': %s", filename,
		     result.error_message ? result.error_message : "unknown error");
		vk_shader_compile_result_free(&result);
		pthread_mutex_unlock(&manager->lock);
		return VK_NULL_HANDLE;
	}

	VkShaderModule module;
	VkResult vk_result = vk_shader_module_create_from_spirv(manager->device, result.bytecode,
							        result.bytecode_size, &module);

	vk_shader_compile_result_free(&result);

	if (vk_result != VK_SUCCESS) {
		blog(LOG_ERROR, "Failed to create shader module from '%s': %d", filename, vk_result);
		pthread_mutex_unlock(&manager->lock);
		return VK_NULL_HANDLE;
	}

	vk_shader_cache_entry_t entry = {
		.name = bstrdup(name),
		.module = module,
		.stage = stage,
		.ref_count = 1,
	};

	da_push_back(manager->shader_cache, &entry);

	blog(LOG_INFO, "Loaded and cached shader '%s' from file: %s", name, filename);

	pthread_mutex_unlock(&manager->lock);
	return module;
}

VkShaderModule vk_shader_manager_load_spirv(vk_shader_manager_t *manager, const char *name,
					    const char *spirv_file, VkShaderStageFlagBits stage)
{
	if (!manager || !name || !spirv_file) {
		return VK_NULL_HANDLE;
	}

	pthread_mutex_lock(&manager->lock);

	vk_shader_cache_entry_t *cached = find_shader(manager, name);
	if (cached) {
		cached->ref_count++;
		VkShaderModule module = cached->module;
		pthread_mutex_unlock(&manager->lock);
		return module;
	}

	uint32_t *bytecode = NULL;
	size_t bytecode_size = 0;

	if (!vk_shader_load_spirv(spirv_file, &bytecode, &bytecode_size)) {
		blog(LOG_ERROR, "Failed to load SPIR-V file: %s", spirv_file);
		pthread_mutex_unlock(&manager->lock);
		return VK_NULL_HANDLE;
	}

	VkShaderModule module;
	VkResult vk_result =
		vk_shader_module_create_from_spirv(manager->device, bytecode, bytecode_size, &module);
	bfree(bytecode);

	if (vk_result != VK_SUCCESS) {
		blog(LOG_ERROR, "Failed to create shader module from SPIR-V: %d", vk_result);
		pthread_mutex_unlock(&manager->lock);
		return VK_NULL_HANDLE;
	}

	vk_shader_cache_entry_t entry = {
		.name = bstrdup(name),
		.module = module,
		.stage = stage,
		.ref_count = 1,
	};

	da_push_back(manager->shader_cache, &entry);

	blog(LOG_INFO, "Loaded and cached shader '%s' from SPIR-V: %s", name, spirv_file);

	pthread_mutex_unlock(&manager->lock);
	return module;
}

VkShaderModule vk_shader_manager_get(vk_shader_manager_t *manager, const char *name)
{
	if (!manager || !name) {
		return VK_NULL_HANDLE;
	}

	pthread_mutex_lock(&manager->lock);

	vk_shader_cache_entry_t *entry = find_shader(manager, name);
	VkShaderModule module = entry ? entry->module : VK_NULL_HANDLE;

	pthread_mutex_unlock(&manager->lock);

	return module;
}

void vk_shader_manager_release(vk_shader_manager_t *manager, const char *name)
{
	if (!manager || !name) {
		return;
	}

	pthread_mutex_lock(&manager->lock);

	vk_shader_cache_entry_t *entry = find_shader(manager, name);
	if (entry) {
		if (entry->ref_count > 0) {
			entry->ref_count--;
		}

		if (entry->ref_count == 0) {
			vkDestroyShaderModule(manager->device, entry->module, NULL);
			bfree(entry->name);
			da_erase(manager->shader_cache, da_find(manager->shader_cache, entry));
			blog(LOG_DEBUG, "Released and unloaded shader: %s", name);
		} else {
			blog(LOG_DEBUG, "Released shader '%s' (ref_count=%u)", name, entry->ref_count);
		}
	}

	pthread_mutex_unlock(&manager->lock);
}

bool vk_shader_precompile_directory(const char *source_dir, const char *output_dir)
{
	if (!source_dir || !output_dir) {
		return false;
	}

	os_dir_t *dir = os_opendir(source_dir);
	if (!dir) {
		blog(LOG_ERROR, "Failed to open shader directory: %s", source_dir);
		return false;
	}

	struct os_dirent *entry;
	int error_count = 0;
	int compile_count = 0;

	while ((entry = os_readdir(dir)) != NULL) {
		const char *name = entry->d_name;
		size_t name_len = strlen(name);

		/* Check for shader file extensions */
		if (ends_with(name, ".vert") || ends_with(name, ".frag") || ends_with(name, ".comp") ||
		    ends_with(name, ".geom") || ends_with(name, ".tesc") || ends_with(name, ".tese")) {

			char full_source_path[512];
			snprintf(full_source_path, sizeof(full_source_path), "%s/%s", source_dir, name);

			vk_shader_compile_options_t options = {
				.stage = vk_shader_get_stage_from_filename(name),
				.entry_point = "main",
				.optimize = true,
			};

			vk_shader_compile_result_t result =
				vk_shader_compile_glsl_file(full_source_path, &options);

			if (!result.success) {
				blog(LOG_ERROR, "Failed to compile: %s", full_source_path);
				error_count++;
				vk_shader_compile_result_free(&result);
				continue;
			}

			/* Generate output filename */
			char output_path[512];
			snprintf(output_path, sizeof(output_path), "%s/%.*s.spv", output_dir,
				 (int)(name_len - 5), name);

			if (!vk_shader_save_spirv(result.bytecode, result.bytecode_size, output_path)) {
				blog(LOG_ERROR, "Failed to save SPIR-V: %s", output_path);
				error_count++;
			} else {
				blog(LOG_INFO, "Compiled: %s → %s", name, output_path);
				compile_count++;
			}

			vk_shader_compile_result_free(&result);
		}
	}

	os_closedir(dir);

	blog(LOG_INFO, "Shader compilation complete: %d compiled, %d failed", compile_count,
	     error_count);

	return error_count == 0;
}
