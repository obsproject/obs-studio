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

#include "vulkan-subsystem.h"
#include <graphics/graphics.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Error handling */
#define vk_check(result)                                                 \
	do {                                                                \
		if (result != VK_SUCCESS) {                                  \
			blog(LOG_ERROR, "Vulkan error: %s", vk_get_error_string(result)); \
			return false;                                            \
		}                                                             \
	} while (0)

#define vk_check_ret(result, ret)                                        \
	do {                                                                \
		if (result != VK_SUCCESS) {                                  \
			blog(LOG_ERROR, "Vulkan error: %s", vk_get_error_string(result)); \
			return ret;                                              \
		}                                                             \
	} while (0)

/* Memory allocation helpers */
uint32_t vk_find_memory_type(struct gs_device *device, uint32_t type_filter, VkMemoryPropertyFlags properties);

bool vk_create_buffer(struct gs_device *device, VkDeviceSize size, VkBufferUsageFlags usage,
		      VkMemoryPropertyFlags properties, VkBuffer *buffer, VkDeviceMemory *memory);

void vk_copy_buffer(struct gs_device *device, VkBuffer src, VkBuffer dst, VkDeviceSize size);

bool vk_create_image(struct gs_device *device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
		     VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage *image, VkDeviceMemory *memory);

VkImageView vk_create_image_view(struct gs_device *device, VkImage image, VkFormat format,
				  VkImageAspectFlags aspect_mask, uint32_t mip_levels);

/* Command buffer helpers */
VkCommandBuffer vk_begin_single_time_commands(struct gs_device *device);
void vk_end_single_time_commands(struct gs_device *device, VkCommandBuffer command_buffer);

void vk_transition_image_layout(struct gs_device *device, VkImage image, VkFormat format, VkImageLayout old_layout,
				VkImageLayout new_layout, uint32_t mip_levels);

void vk_copy_buffer_to_image(struct gs_device *device, VkBuffer buffer, VkImage image, uint32_t width,
			     uint32_t height);

/* Descriptor helpers */
bool vk_create_descriptor_pool(struct gs_device *device);

#ifdef __cplusplus
}
#endif
