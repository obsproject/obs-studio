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

#include "vulkan-helpers.h"
#include <util/blogging.h>
#include <util/mem.h>

const char *vk_get_error_string(VkResult result)
{
	switch (result) {
	case VK_SUCCESS:
		return "VK_SUCCESS";
	case VK_NOT_READY:
		return "VK_NOT_READY";
	case VK_TIMEOUT:
		return "VK_TIMEOUT";
	case VK_EVENT_SET:
		return "VK_EVENT_SET";
	case VK_EVENT_RESET:
		return "VK_EVENT_RESET";
	case VK_INCOMPLETE:
		return "VK_INCOMPLETE";
	case VK_ERROR_OUT_OF_HOST_MEMORY:
		return "VK_ERROR_OUT_OF_HOST_MEMORY";
	case VK_ERROR_OUT_OF_DEVICE_MEMORY:
		return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
	case VK_ERROR_INITIALIZATION_FAILED:
		return "VK_ERROR_INITIALIZATION_FAILED";
	case VK_ERROR_DEVICE_LOST:
		return "VK_ERROR_DEVICE_LOST";
	case VK_ERROR_MEMORY_MAP_FAILED:
		return "VK_ERROR_MEMORY_MAP_FAILED";
	case VK_ERROR_LAYER_NOT_PRESENT:
		return "VK_ERROR_LAYER_NOT_PRESENT";
	case VK_ERROR_EXTENSION_NOT_PRESENT:
		return "VK_ERROR_EXTENSION_NOT_PRESENT";
	case VK_ERROR_FEATURE_NOT_PRESENT:
		return "VK_ERROR_FEATURE_NOT_PRESENT";
	case VK_ERROR_INCOMPATIBLE_DRIVER:
		return "VK_ERROR_INCOMPATIBLE_DRIVER";
	case VK_ERROR_TOO_MANY_OBJECTS:
		return "VK_ERROR_TOO_MANY_OBJECTS";
	case VK_ERROR_FORMAT_NOT_SUPPORTED:
		return "VK_ERROR_FORMAT_NOT_SUPPORTED";
	case VK_ERROR_SURFACE_LOST_KHR:
		return "VK_ERROR_SURFACE_LOST_KHR";
	case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
		return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
	case VK_SUBOPTIMAL_KHR:
		return "VK_SUBOPTIMAL_KHR";
	case VK_ERROR_OUT_OF_DATE_KHR:
		return "VK_ERROR_OUT_OF_DATE_KHR";
	case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
		return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
	case VK_ERROR_VALIDATION_FAILED_EXT:
		return "VK_ERROR_VALIDATION_FAILED_EXT";
	case VK_ERROR_INVALID_SHADER_NV:
		return "VK_ERROR_INVALID_SHADER_NV";
	default:
		return "VK_ERROR_UNKNOWN";
	}
}

uint32_t vk_find_memory_type(struct gs_device *device, uint32_t type_filter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties mem_properties;
	vkGetPhysicalDeviceMemoryProperties(device->gpu, &mem_properties);

	for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
		if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	blog(LOG_ERROR, "Failed to find suitable memory type");
	return 0;
}

bool vk_create_buffer(struct gs_device *device, VkDeviceSize size, VkBufferUsageFlags usage,
		      VkMemoryPropertyFlags properties, VkBuffer *buffer, VkDeviceMemory *memory)
{
	VkBufferCreateInfo buffer_info = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
					   .size = size,
					   .usage = usage,
					   .sharingMode = VK_SHARING_MODE_EXCLUSIVE};

	VkResult result = vkCreateBuffer(device->device, &buffer_info, NULL, buffer);
	if (result != VK_SUCCESS) {
		blog(LOG_ERROR, "Failed to create buffer: %s", vk_get_error_string(result));
		return false;
	}

	VkMemoryRequirements mem_requirements;
	vkGetBufferMemoryRequirements(device->device, *buffer, &mem_requirements);

	VkMemoryAllocateInfo alloc_info = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
					   .allocationSize = mem_requirements.size,
					   .memoryTypeIndex =
						   vk_find_memory_type(device, mem_requirements.memoryTypeBits, properties)};

	result = vkAllocateMemory(device->device, &alloc_info, NULL, memory);
	if (result != VK_SUCCESS) {
		blog(LOG_ERROR, "Failed to allocate buffer memory: %s", vk_get_error_string(result));
		vkDestroyBuffer(device->device, *buffer, NULL);
		return false;
	}

	vkBindBufferMemory(device->device, *buffer, *memory, 0);
	return true;
}

void vk_copy_buffer(struct gs_device *device, VkBuffer src, VkBuffer dst, VkDeviceSize size)
{
	VkCommandBuffer command_buffer = vk_begin_single_time_commands(device);

	VkBufferCopy copy_region = {.size = size};
	vkCmdCopyBuffer(command_buffer, src, dst, 1, &copy_region);

	vk_end_single_time_commands(device, command_buffer);
}

bool vk_create_image(struct gs_device *device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
		     VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage *image, VkDeviceMemory *memory)
{
	VkImageCreateInfo image_info = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
					 .imageType = VK_IMAGE_TYPE_2D,
					 .extent.width = width,
					 .extent.height = height,
					 .extent.depth = 1,
					 .mipLevels = 1,
					 .arrayLayers = 1,
					 .format = format,
					 .tiling = tiling,
					 .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
					 .usage = usage,
					 .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
					 .samples = VK_SAMPLE_COUNT_1_BIT};

	VkResult result = vkCreateImage(device->device, &image_info, NULL, image);
	if (result != VK_SUCCESS) {
		blog(LOG_ERROR, "Failed to create image: %s", vk_get_error_string(result));
		return false;
	}

	VkMemoryRequirements mem_requirements;
	vkGetImageMemoryRequirements(device->device, *image, &mem_requirements);

	VkMemoryAllocateInfo alloc_info = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
					   .allocationSize = mem_requirements.size,
					   .memoryTypeIndex =
						   vk_find_memory_type(device, mem_requirements.memoryTypeBits, properties)};

	result = vkAllocateMemory(device->device, &alloc_info, NULL, memory);
	if (result != VK_SUCCESS) {
		blog(LOG_ERROR, "Failed to allocate image memory: %s", vk_get_error_string(result));
		vkDestroyImage(device->device, *image, NULL);
		return false;
	}

	vkBindImageMemory(device->device, *image, *memory, 0);
	return true;
}

VkImageView vk_create_image_view(struct gs_device *device, VkImage image, VkFormat format,
				  VkImageAspectFlags aspect_mask, uint32_t mip_levels)
{
	VkImageViewCreateInfo view_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = format,
		.subresourceRange.aspectMask = aspect_mask,
		.subresourceRange.baseMipLevel = 0,
		.subresourceRange.levelCount = mip_levels,
		.subresourceRange.baseArrayLayer = 0,
		.subresourceRange.layerCount = 1,
	};

	VkImageView image_view;
	VkResult result = vkCreateImageView(device->device, &view_info, NULL, &image_view);
	if (result != VK_SUCCESS) {
		blog(LOG_ERROR, "Failed to create image view: %s", vk_get_error_string(result));
		return NULL;
	}

	return image_view;
}

VkCommandBuffer vk_begin_single_time_commands(struct gs_device *device)
{
	VkCommandBufferAllocateInfo alloc_info = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
						   .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
						   .commandPool = device->command_pool,
						   .commandBufferCount = 1};

	VkCommandBuffer command_buffer;
	vkAllocateCommandBuffers(device->device, &alloc_info, &command_buffer);

	VkCommandBufferBeginInfo begin_info = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
					       .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

	vkBeginCommandBuffer(command_buffer, &begin_info);
	return command_buffer;
}

void vk_end_single_time_commands(struct gs_device *device, VkCommandBuffer command_buffer)
{
	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo submit_info = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
				     .commandBufferCount = 1,
				     .pCommandBuffers = &command_buffer};

	vkQueueSubmit(device->graphics_queue, 1, &submit_info, NULL);
	vkQueueWaitIdle(device->graphics_queue);

	vkFreeCommandBuffers(device->device, device->command_pool, 1, &command_buffer);
}

void vk_transition_image_layout(struct gs_device *device, VkImage image, VkFormat format, VkImageLayout old_layout,
				VkImageLayout new_layout, uint32_t mip_levels)
{
	VkCommandBuffer command_buffer = vk_begin_single_time_commands(device);

	VkImageMemoryBarrier barrier = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					 .oldLayout = old_layout,
					 .newLayout = new_layout,
					 .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					 .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					 .image = image,
					 .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					 .subresourceRange.baseMipLevel = 0,
					 .subresourceRange.levelCount = mip_levels,
					 .subresourceRange.baseArrayLayer = 0,
					 .subresourceRange.layerCount = 1};

	VkPipelineStageFlags source_stage;
	VkPipelineStageFlags destination_stage;

	if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
		   new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else {
		blog(LOG_ERROR, "Unsupported layout transition!");
		return;
	}

	vkCmdPipelineBarrier(command_buffer, source_stage, destination_stage, 0, 0, NULL, 0, NULL, 1, &barrier);

	vk_end_single_time_commands(device, command_buffer);
}

void vk_copy_buffer_to_image(struct gs_device *device, VkBuffer buffer, VkImage image, uint32_t width,
			     uint32_t height)
{
	VkCommandBuffer command_buffer = vk_begin_single_time_commands(device);

	VkBufferImageCopy region = {.bufferOffset = 0,
				     .bufferRowLength = 0,
				     .bufferImageHeight = 0,
				     .imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				     .imageSubresource.mipLevel = 0,
				     .imageSubresource.baseArrayLayer = 0,
				     .imageSubresource.layerCount = 1,
				     .imageOffset = {0, 0, 0},
				     .imageExtent = {width, height, 1}};

	vkCmdCopyBufferToImage(command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	vk_end_single_time_commands(device, command_buffer);
}

bool vk_create_descriptor_pool(struct gs_device *device)
{
	VkDescriptorPoolSize pool_size = {.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 256};

	VkDescriptorPoolCreateInfo pool_info = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
						 .poolSizeCount = 1,
						 .pPoolSizes = &pool_size,
						 .maxSets = 256};

	VkResult result = vkCreateDescriptorPool(device->device, &pool_info, NULL, &device->descriptor_pool);
	if (result != VK_SUCCESS) {
		blog(LOG_ERROR, "Failed to create descriptor pool: %s", vk_get_error_string(result));
		return false;
	}

	return true;
}
