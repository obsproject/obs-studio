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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Shader compilation options
 */
typedef struct {
	VkShaderStageFlagBits stage;
	const char *entry_point;
	bool optimize;
} vk_shader_compile_options_t;

/**
 * Compiled shader data
 */
typedef struct {
	uint32_t *bytecode;
	size_t bytecode_size;
	char *error_message;
	bool success;
} vk_shader_compile_result_t;

/**
 * Compile GLSL shader source code to SPIR-V bytecode
 * 
 * @param glsl_source    GLSL shader source code
 * @param source_len     Length of source code (0 for null-terminated)
 * @param options        Compilation options
 * @return Compilation result (must be freed with vk_shader_compile_result_free)
 */
vk_shader_compile_result_t vk_shader_compile_glsl(const char *glsl_source,
						   size_t source_len,
						   const vk_shader_compile_options_t *options);

/**
 * Compile GLSL shader from file
 * 
 * @param filename  Path to GLSL shader file
 * @param options   Compilation options
 * @return Compilation result (must be freed with vk_shader_compile_result_free)
 */
vk_shader_compile_result_t vk_shader_compile_glsl_file(const char *filename,
						       const vk_shader_compile_options_t *options);

/**
 * Free compilation result resources
 * 
 * @param result    Result to free
 */
void vk_shader_compile_result_free(vk_shader_compile_result_t *result);

/**
 * Create Vulkan shader module from SPIR-V bytecode
 * 
 * @param device    Vulkan logical device
 * @param bytecode  SPIR-V bytecode
 * @param size      Size of bytecode in bytes
 * @param module    Output shader module handle
 * @return VK_SUCCESS on success, VkResult error code on failure
 */
VkResult vk_shader_module_create_from_spirv(VkDevice device,
					    const uint32_t *bytecode,
					    size_t size,
					    VkShaderModule *module);

/**
 * Create Vulkan shader module from GLSL source (compiled on the fly)
 * 
 * @param device    Vulkan logical device
 * @param glsl_source GLSL source code
 * @param options   Compilation and linking options
 * @param module    Output shader module handle
 * @return VK_SUCCESS on success, VkResult error code on failure
 */
VkResult vk_shader_module_create_from_glsl(VkDevice device,
					   const char *glsl_source,
					   const vk_shader_compile_options_t *options,
					   VkShaderModule *module);

/**
 * Save compiled SPIR-V bytecode to file
 * 
 * @param bytecode  SPIR-V bytecode
 * @param size      Size in bytes
 * @param filename  Output file path
 * @return true on success, false on failure
 */
bool vk_shader_save_spirv(const uint32_t *bytecode, size_t size, const char *filename);

/**
 * Load compiled SPIR-V bytecode from file
 * 
 * @param filename   Path to compiled shader file
 * @param out_bytecode Output bytecode pointer (must be freed with bfree)
 * @param out_size   Output size in bytes
 * @return true on success, false on failure
 */
bool vk_shader_load_spirv(const char *filename, uint32_t **out_bytecode, size_t *out_size);

/**
 * Get shader stage from file extension
 * 
 * @param filename  Shader file path
 * @return Shader stage flags, or 0 if not recognized
 */
VkShaderStageFlagBits vk_shader_get_stage_from_filename(const char *filename);

#ifdef __cplusplus
}
#endif
