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

#include "vulkan-subsystem.h"
#include <util/blogging.h>
#include <util/mem.h>

/* Shader Implementation */

gs_shader_t *gs_shader_create(gs_device_t *device, enum gs_shader_type type, const char *shader_str,
			      const char *file, char **error_string)
{
	struct gs_shader *shader = bzalloc(sizeof(struct gs_shader));
	shader->device = device;
	shader->type = type;

	/* TODO: Implement GLSL to SPIR-V compilation */
	blog(LOG_WARNING, "Shader compilation not yet implemented for Vulkan");

	return shader;
}

void gs_shader_destroy(gs_shader_t *shader)
{
	if (!shader)
		return;

	if (shader->module)
		vkDestroyShaderModule(shader->device->device, shader->module, NULL);

	if (shader->bytecode)
		bfree(shader->bytecode);

	bfree(shader);
}

/* Program Implementation */

struct gs_program *gs_program_create(gs_device_t *device)
{
	struct gs_program *program = bzalloc(sizeof(struct gs_program));
	program->device = device;

	/* TODO: Connect vertex and pixel shaders into a pipeline */

	program->next = device->first_program;
	program->prev_next = &device->first_program;
	device->first_program = program;
	if (program->next)
		program->next->prev_next = &program->next;

	return program;
}

void gs_program_destroy(struct gs_program *program)
{
	if (!program)
		return;

	if (program->pipeline)
		vkDestroyPipeline(program->device->device, program->pipeline, NULL);

	if (program->layout)
		vkDestroyPipelineLayout(program->device->device, program->layout, NULL);

	if (program->prev_next) {
		*program->prev_next = program->next;
		if (program->next)
			program->next->prev_next = program->prev_next;
	}

	da_free(program->params);
	bfree(program);
}

void program_update_params(struct gs_program *program)
{
	/* TODO: Update descriptor sets with current shader parameters */
	UNUSED_PARAMETER(program);
}
