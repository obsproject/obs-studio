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

/* Index Buffer Implementation */

gs_indexbuffer_t *gs_indexbuffer_create(enum gs_index_type type, void *indices, size_t num, uint32_t flags)
{
	struct gs_index_buffer *ib = bzalloc(sizeof(struct gs_index_buffer));

	ib->type = type;
	ib->num = num;

	/* Set up Vulkan index type */
	switch (type) {
	case GS_INDEX_U16:
		ib->vk_type = VK_INDEX_TYPE_UINT16;
		ib->width = 2;
		break;
	case GS_INDEX_U32:
		ib->vk_type = VK_INDEX_TYPE_UINT32;
		ib->width = 4;
		break;
	default:
		blog(LOG_ERROR, "Invalid index type");
		bfree(ib);
		return NULL;
	}

	ib->size = ib->width * num;
	ib->dynamic = (flags & GS_DYNAMIC) != 0;

	if (indices) {
		ib->data = bmemdup(indices, ib->size);
	} else {
		ib->data = bzalloc(ib->size);
	}

	/* TODO: Create Vulkan buffer */
	blog(LOG_WARNING, "Index buffer creation not yet implemented for Vulkan");

	return ib;
}

void gs_indexbuffer_destroy(gs_indexbuffer_t *ib)
{
	if (!ib)
		return;

	if (ib->buffer)
		vkDestroyBuffer(ib->device->device, ib->buffer, NULL);
	if (ib->memory)
		vkFreeMemory(ib->device->device, ib->memory, NULL);
	if (ib->staging_buffer)
		vkDestroyBuffer(ib->device->device, ib->staging_buffer, NULL);
	if (ib->staging_memory)
		vkFreeMemory(ib->device->device, ib->staging_memory, NULL);

	bfree(ib->data);
	bfree(ib);
}

size_t gs_indexbuffer_get_num_indices(const gs_indexbuffer_t *ib)
{
	if (!ib)
		return 0;
	return ib->num;
}

enum gs_index_type gs_indexbuffer_get_type(const gs_indexbuffer_t *ib)
{
	if (!ib)
		return GS_INDEX_U16;
	return ib->type;
}
