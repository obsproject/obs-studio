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

#pragma once

#include <util/c99defs.h>
#include <vulkan/vulkan.h>
#include <graphics/graphics.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Shader cache entry
 */
typedef struct {
	char *name;
	VkShaderModule module;
	VkShaderStageFlagBits stage;
	uint32_t ref_count;
} vk_shader_cache_entry_t;

/**
 * Shader manager for Vulkan
 */
typedef struct {
	VkDevice device;
	struct darray shader_cache;
	pthread_mutex_t lock;
} vk_shader_manager_t;

/**
 * Create shader manager
 *
 * @param device    Vulkan logical device
 * @return Shader manager instance
 */
vk_shader_manager_t *vk_shader_manager_create(VkDevice device);

/**
 * Destroy shader manager and release all cached shaders
 *
 * @param manager   Shader manager instance
 */
void vk_shader_manager_destroy(vk_shader_manager_t *manager);

/**
 * Load or compile shader from GLSL source
 *
 * @param manager       Shader manager instance
 * @param name          Unique shader name for caching
 * @param glsl_source   GLSL source code
 * @param stage         Shader stage
 * @return Vulkan shader module handle
 */
VkShaderModule vk_shader_manager_load_glsl(vk_shader_manager_t *manager, const char *name,
					   const char *glsl_source, VkShaderStageFlagBits stage);

/**
 * Load shader from file
 *
 * @param manager   Shader manager instance
 * @param name      Unique shader name for caching
 * @param filename  Path to shader file
 * @return Vulkan shader module handle
 */
VkShaderModule vk_shader_manager_load_file(vk_shader_manager_t *manager, const char *name,
					   const char *filename);

/**
 * Load pre-compiled SPIR-V shader
 *
 * @param manager       Shader manager instance
 * @param name          Unique shader name for caching
 * @param spirv_file    Path to compiled SPIR-V file
 * @param stage         Shader stage
 * @return Vulkan shader module handle
 */
VkShaderModule vk_shader_manager_load_spirv(vk_shader_manager_t *manager, const char *name,
					    const char *spirv_file, VkShaderStageFlagBits stage);

/**
 * Get cached shader by name
 *
 * @param manager   Shader manager instance
 * @param name      Shader name
 * @return Vulkan shader module handle, or VK_NULL_HANDLE if not found
 */
VkShaderModule vk_shader_manager_get(vk_shader_manager_t *manager, const char *name);

/**
 * Release reference to shader (may unload if ref count reaches 0)
 *
 * @param manager   Shader manager instance
 * @param name      Shader name
 */
void vk_shader_manager_release(vk_shader_manager_t *manager, const char *name);

/**
 * Precompile all GLSL shaders in a directory to SPIR-V
 *
 * @param source_dir    Directory containing .vert, .frag, .comp files
 * @param output_dir    Directory to save .spv files
 * @return true on success, false if any shader failed to compile
 */
bool vk_shader_precompile_directory(const char *source_dir, const char *output_dir);

#ifdef __cplusplus
}
#endif
