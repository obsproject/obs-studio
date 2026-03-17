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
#include "vulkan-helpers.h"
#include <util/blogging.h>
#include <util/mem.h>

/* Vertex Buffer Implementation */

gs_vertbuffer_t *gs_vertexbuffer_create(struct gs_vb_data *data, uint32_t flags)
{
	if (!data)
		return NULL;

	struct gs_vertex_buffer *vb = bzalloc(sizeof(struct gs_vertex_buffer));

	/* TODO: Implement vertex buffer creation */
	blog(LOG_WARNING, "Vertex buffer creation not yet implemented for Vulkan");

	bfree(vb);
	return NULL;
}

void gs_vertexbuffer_destroy(gs_vertbuffer_t *vertbuffer)
{
	if (!vertbuffer)
		return;

	struct gs_vertex_buffer *vb = vertbuffer;
	struct gs_device *device = vb->device;

	if (vb->buffer)
		vkDestroyBuffer(device->device, vb->buffer, NULL);
	if (vb->memory)
		vkFreeMemory(device->device, vb->memory, NULL);
	if (vb->staging_buffer)
		vkDestroyBuffer(device->device, vb->staging_buffer, NULL);
	if (vb->staging_memory)
		vkFreeMemory(device->device, vb->staging_memory, NULL);

	if (vb->data)
		gs_vb_data_destroy(vb->data);

	bfree(vb);
}

size_t gs_vertexbuffer_get_num_verts(const gs_vertbuffer_t *vb)
{
	if (!vb)
		return 0;
	return ((struct gs_vertex_buffer *)vb)->num;
}

struct gs_vb_data *gs_vertexbuffer_get_data(const gs_vertbuffer_t *vb)
{
	if (!vb)
		return NULL;
	return ((struct gs_vertex_buffer *)vb)->data;
}
