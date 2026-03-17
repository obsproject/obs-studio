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

#include "vulkan-shader-compiler.h"
#include <util/blogging.h>
#include <util/mem.h>
#include <util/platform.h>

/* Wrapper for glslang compilation - implemented in C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef void (*glslang_lib_init_fn)(void);
typedef void (*glslang_lib_finalize_fn)(void);
typedef void *(*glslang_compiler_new_fn)(void);
typedef void (*glslang_compiler_delete_fn)(void *compiler);
typedef int (*glslang_compile_fn)(void *compiler, int lang, int stage,
				   const char *source, size_t source_len,
				   uint32_t **out_spirv, size_t *out_size,
				   char **out_error);

static glslang_lib_init_fn glslang_lib_init = NULL;
static glslang_lib_finalize_fn glslang_lib_finalize = NULL;
static glslang_compiler_new_fn glslang_compiler_new = NULL;
static glslang_compiler_delete_fn glslang_compiler_delete = NULL;
static glslang_compile_fn glslang_compile = NULL;
static void *glslang_lib = NULL;

#ifdef __cplusplus
}
#endif

/* Language and stage enums for glslang */
enum {
	GLSLANG_LANG_GLSL = 0,
	GLSLANG_LANG_HLSL = 1,
};

enum {
	GLSLANG_STAGE_VERTEX = 0,
	GLSLANG_STAGE_FRAGMENT = 4,
	GLSLANG_STAGE_COMPUTE = 5,
	GLSLANG_STAGE_GEOMETRY = 3,
	GLSLANG_STAGE_TESS_CONTROL = 1,
	GLSLANG_STAGE_TESS_EVAL = 2,
};

/**
 * Load glslang library dynamically
 */
static bool load_glslang_lib(void)
{
	if (glslang_lib != NULL) {
		return true;
	}

#ifdef _WIN32
	const char *lib_names[] = {"glslang.dll", NULL};
#elif __APPLE__
	const char *lib_names[] = {"libglslang.dylib", NULL};
#else
	const char *lib_names[] = {"libglslang.so", "libglslang.so.14", "libglslang.so.13", NULL};
#endif

	for (const char **name = lib_names; *name; name++) {
		glslang_lib = os_dlopen(*name);
		if (glslang_lib != NULL) {
			blog(LOG_INFO, "Loaded glslang library: %s", *name);

			glslang_lib_init =
				(glslang_lib_init_fn)os_dlsym(glslang_lib, "glslang_lib_init");
			glslang_lib_finalize = (glslang_lib_finalize_fn)os_dlsym(glslang_lib, "glslang_lib_finalize");
			glslang_compiler_new = (glslang_compiler_new_fn)os_dlsym(glslang_lib, "glslang_compiler_new");
			glslang_compiler_delete = (glslang_compiler_delete_fn)os_dlsym(glslang_lib, "glslang_compiler_delete");
			glslang_compile = (glslang_compile_fn)os_dlsym(glslang_lib, "glslang_compile");

			if (!glslang_lib_init || !glslang_lib_finalize || !glslang_compiler_new ||
			    !glslang_compiler_delete || !glslang_compile) {
				blog(LOG_WARNING, "glslang library missing required functions");
				os_dlclose(glslang_lib);
				glslang_lib = NULL;
				continue;
			}

			glslang_lib_init();
			return true;
		}
	}

	blog(LOG_WARNING, "Could not find glslang library - shader compilation disabled");
	return false;
}

/**
 * Unload glslang library
 */
static void unload_glslang_lib(void)
{
	if (glslang_lib != NULL) {
		glslang_lib_finalize();
		os_dlclose(glslang_lib);
		glslang_lib = NULL;
	}
}

/**
 * Convert VkShaderStageFlagBits to glslang stage enum
 */
static int stage_to_glslang_stage(VkShaderStageFlagBits stage)
{
	switch (stage) {
	case VK_SHADER_STAGE_VERTEX_BIT:
		return GLSLANG_STAGE_VERTEX;
	case VK_SHADER_STAGE_FRAGMENT_BIT:
		return GLSLANG_STAGE_FRAGMENT;
	case VK_SHADER_STAGE_COMPUTE_BIT:
		return GLSLANG_STAGE_COMPUTE;
	case VK_SHADER_STAGE_GEOMETRY_BIT:
		return GLSLANG_STAGE_GEOMETRY;
	case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
		return GLSLANG_STAGE_TESS_CONTROL;
	case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
		return GLSLANG_STAGE_TESS_EVAL;
	default:
		return -1;
	}
}

vk_shader_compile_result_t vk_shader_compile_glsl(const char *glsl_source, size_t source_len,
						   const vk_shader_compile_options_t *options)
{
	vk_shader_compile_result_t result = {0};

	if (!glsl_source || !options) {
		result.success = false;
		result.error_message = bstrdup("Invalid input parameters");
		return result;
	}

	if (!load_glslang_lib()) {
		result.success = false;
		result.error_message = bstrdup("glslang library not available");
		return result;
	}

	if (source_len == 0) {
		source_len = strlen(glsl_source);
	}

	int glslang_stage = stage_to_glslang_stage(options->stage);
	if (glslang_stage < 0) {
		result.success = false;
		result.error_message = bstrdup("Unsupported shader stage");
		return result;
	}

	void *compiler = glslang_compiler_new();
	if (!compiler) {
		result.success = false;
		result.error_message = bstrdup("Failed to create glslang compiler");
		return result;
	}

	char *error_msg = NULL;
	uint32_t *bytecode = NULL;
	size_t bytecode_size = 0;

	int compile_result = glslang_compile(compiler, GLSLANG_LANG_GLSL, glslang_stage,
					     glsl_source, source_len, &bytecode, &bytecode_size,
					     &error_msg);

	glslang_compiler_delete(compiler);

	if (compile_result != 0) {
		result.success = false;
		result.error_message = error_msg ? bstrdup(error_msg) : bstrdup("Compilation failed");
		if (error_msg) {
			bfree(error_msg);
		}
		if (bytecode) {
			bfree(bytecode);
		}
		return result;
	}

	result.success = true;
	result.bytecode = bytecode;
	result.bytecode_size = bytecode_size;
	result.error_message = error_msg ? bstrdup(error_msg) : NULL;

	if (error_msg) {
		bfree(error_msg);
	}

	blog(LOG_INFO, "Compiled shader successfully: %zu bytes", bytecode_size);
	return result;
}

vk_shader_compile_result_t vk_shader_compile_glsl_file(const char *filename,
						       const vk_shader_compile_options_t *options)
{
	vk_shader_compile_result_t result = {0};

	struct os_mstat stat;
	int stat_result = os_stat(filename, &stat);
	if (stat_result != 0) {
		result.success = false;
		result.error_message = bstrdup("File not found or not accessible");
		return result;
	}

	char *file_data = NULL;
	FILE *file = fopen(filename, "rb");
	if (!file) {
		result.success = false;
		result.error_message = bstrdup("Failed to open file");
		return result;
	}

	file_data = (char *)bmalloc(stat.size + 1);
	size_t read_size = fread(file_data, 1, stat.size, file);
	fclose(file);

	if (read_size != stat.size) {
		result.success = false;
		result.error_message = bstrdup("Failed to read file");
		bfree(file_data);
		return result;
	}

	file_data[stat.size] = '\0';

	result = vk_shader_compile_glsl(file_data, stat.size, options);
	bfree(file_data);

	return result;
}

void vk_shader_compile_result_free(vk_shader_compile_result_t *result)
{
	if (!result) {
		return;
	}

	if (result->bytecode) {
		bfree(result->bytecode);
		result->bytecode = NULL;
	}

	if (result->error_message) {
		bfree(result->error_message);
		result->error_message = NULL;
	}

	result->bytecode_size = 0;
	result->success = false;
}

VkResult vk_shader_module_create_from_spirv(VkDevice device, const uint32_t *bytecode,
					    size_t size, VkShaderModule *module)
{
	if (!device || !bytecode || size == 0 || !module) {
		return VK_ERROR_INVALID_EXTERNAL_HANDLE;
	}

	VkShaderModuleCreateInfo create_info = {.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
						 .codeSize = size,
						 .pCode = bytecode};

	return vkCreateShaderModule(device, &create_info, NULL, module);
}

VkResult vk_shader_module_create_from_glsl(VkDevice device, const char *glsl_source,
					   const vk_shader_compile_options_t *options,
					   VkShaderModule *module)
{
	if (!device || !glsl_source || !options || !module) {
		return VK_ERROR_INVALID_EXTERNAL_HANDLE;
	}

	vk_shader_compile_result_t result = vk_shader_compile_glsl(glsl_source, 0, options);

	if (!result.success) {
		blog(LOG_ERROR, "Failed to compile shader: %s", result.error_message);
		vk_shader_compile_result_free(&result);
		return VK_ERROR_INVALID_SHADER_NV;
	}

	VkResult vk_result =
		vk_shader_module_create_from_spirv(device, result.bytecode, result.bytecode_size, module);

	vk_shader_compile_result_free(&result);

	return vk_result;
}

bool vk_shader_save_spirv(const uint32_t *bytecode, size_t size, const char *filename)
{
	if (!bytecode || size == 0 || !filename) {
		return false;
	}

	FILE *file = fopen(filename, "wb");
	if (!file) {
		blog(LOG_ERROR, "Failed to open file for writing: %s", filename);
		return false;
	}

	size_t written = fwrite(bytecode, 1, size, file);
	fclose(file);

	if (written != size) {
		blog(LOG_ERROR, "Failed to write complete SPIR-V data to file");
		return false;
	}

	blog(LOG_INFO, "Saved SPIR-V bytecode to: %s (%zu bytes)", filename, size);
	return true;
}

bool vk_shader_load_spirv(const char *filename, uint32_t **out_bytecode, size_t *out_size)
{
	if (!filename || !out_bytecode || !out_size) {
		return false;
	}

	struct os_mstat stat;
	if (os_stat(filename, &stat) != 0) {
		blog(LOG_ERROR, "SPIR-V file not found: %s", filename);
		return false;
	}

	if (stat.size % sizeof(uint32_t) != 0) {
		blog(LOG_ERROR, "Invalid SPIR-V file size (not aligned to uint32)");
		return false;
	}

	FILE *file = fopen(filename, "rb");
	if (!file) {
		blog(LOG_ERROR, "Failed to open SPIR-V file: %s", filename);
		return false;
	}

	*out_bytecode = (uint32_t *)bmalloc(stat.size);
	size_t read_size = fread(*out_bytecode, 1, stat.size, file);
	fclose(file);

	if (read_size != stat.size) {
		blog(LOG_ERROR, "Failed to read complete SPIR-V file");
		bfree(*out_bytecode);
		*out_bytecode = NULL;
		return false;
	}

	*out_size = stat.size;
	blog(LOG_INFO, "Loaded SPIR-V bytecode from: %s (%zu bytes)", filename, stat.size);
	return true;
}

VkShaderStageFlagBits vk_shader_get_stage_from_filename(const char *filename)
{
	if (!filename) {
		return 0;
	}

	const char *ext = strrchr(filename, '.');
	if (!ext) {
		return 0;
	}

	if (strcmp(ext, ".vert") == 0) {
		return VK_SHADER_STAGE_VERTEX_BIT;
	} else if (strcmp(ext, ".frag") == 0) {
		return VK_SHADER_STAGE_FRAGMENT_BIT;
	} else if (strcmp(ext, ".comp") == 0) {
		return VK_SHADER_STAGE_COMPUTE_BIT;
	} else if (strcmp(ext, ".geom") == 0) {
		return VK_SHADER_STAGE_GEOMETRY_BIT;
	} else if (strcmp(ext, ".tesc") == 0) {
		return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
	} else if (strcmp(ext, ".tese") == 0) {
		return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
	}

	return 0;
}
