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

/* 3D Texture Implementation */

gs_texture_t *device_texture_3d_create(gs_device_t *device, uint32_t width, uint32_t height, uint32_t depth,
				       enum gs_color_format color_format, uint32_t levels, const uint8_t *const *data,
				       uint32_t flags)
{
	struct gs_texture_3d *tex = bzalloc(sizeof(struct gs_texture_3d));
	struct gs_texture *base_tex = &tex->base;

	base_tex->device = device;
	base_tex->type = GS_TEXTURE_3D;
	base_tex->format = color_format;
	base_tex->vk_format = convert_gs_format(color_format);
	base_tex->vk_image_type = VK_IMAGE_TYPE_3D;
	base_tex->levels = levels;
	base_tex->is_dynamic = (flags & GS_DYNAMIC) != 0;
	base_tex->is_render_target = (flags & GS_RENDER_TARGET) != 0;
	base_tex->gen_mipmaps = (flags & GS_BUILD_MIPMAPS) != 0;

	if (base_tex->vk_format == VK_FORMAT_UNDEFINED) {
		blog(LOG_ERROR, "Unsupported color format");
		bfree(tex);
		return NULL;
	}

	tex->width = width;
	tex->height = height;
	tex->depth = depth;
	tex->gen_mipmaps = (flags & GS_BUILD_MIPMAPS) != 0;

	/* Create Vulkan 3D image */
	VkImageCreateInfo image_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_3D,
		.extent = {.width = width, .height = height, .depth = depth},
		.mipLevels = levels,
		.arrayLayers = 1,
		.format = base_tex->vk_format,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.samples = VK_SAMPLE_COUNT_1_BIT,
	};

	VkResult result = vkCreateImage(device->device, &image_info, NULL, &base_tex->image);
	if (result != VK_SUCCESS) {
		blog(LOG_ERROR, "Failed to create 3D image: %s", vk_get_error_string(result));
		bfree(tex);
		return NULL;
	}

	VkMemoryRequirements mem_requirements;
	vkGetImageMemoryRequirements(device->device, base_tex->image, &mem_requirements);

	VkMemoryAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = mem_requirements.size,
		.memoryTypeIndex = vk_find_memory_type(device, mem_requirements.memoryTypeBits,
						       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
	};

	result = vkAllocateMemory(device->device, &alloc_info, NULL, &base_tex->memory);
	if (result != VK_SUCCESS) {
		blog(LOG_ERROR, "Failed to allocate 3D image memory: %s", vk_get_error_string(result));
		vkDestroyImage(device->device, base_tex->image, NULL);
		bfree(tex);
		return NULL;
	}

	vkBindImageMemory(device->device, base_tex->image, base_tex->memory, 0);

	/* Create image view for 3D texture */
	VkImageViewCreateInfo view_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = base_tex->image,
		.viewType = VK_IMAGE_VIEW_TYPE_3D,
		.format = base_tex->vk_format,
		.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.subresourceRange.baseMipLevel = 0,
		.subresourceRange.levelCount = levels,
		.subresourceRange.baseArrayLayer = 0,
		.subresourceRange.layerCount = 1,
	};

	result = vkCreateImageView(device->device, &view_info, NULL, &base_tex->image_view);
	if (result != VK_SUCCESS) {
		blog(LOG_ERROR, "Failed to create 3D image view: %s", vk_get_error_string(result));
		vkDestroyImage(device->device, base_tex->image, NULL);
		vkFreeMemory(device->device, base_tex->memory, NULL);
		bfree(tex);
		return NULL;
	}

	blog(LOG_INFO, "Successfully created 3D texture (%ux%ux%u)", width, height, depth);
	return base_tex;
}

void gs_texture_3d_destroy(struct gs_texture_3d *tex)
{
	if (!tex)
		return;

	struct gs_device *device = tex->base.device;

	if (tex->staging_buffer)
		vkDestroyBuffer(device->device, tex->staging_buffer, NULL);
	if (tex->staging_memory)
		vkFreeMemory(device->device, tex->staging_memory, NULL);

	if (tex->base.image_view)
		vkDestroyImageView(device->device, tex->base.image_view, NULL);
	if (tex->base.image)
		vkDestroyImage(device->device, tex->base.image, NULL);
	if (tex->base.memory)
		vkFreeMemory(device->device, tex->base.memory, NULL);

	bfree(tex);
}

uint32_t gs_texture_3d_get_width(struct gs_texture_3d *tex)
{
	return tex->width;
}

uint32_t gs_texture_3d_get_height(struct gs_texture_3d *tex)
{
	return tex->height;
}

uint32_t gs_texture_3d_get_depth(struct gs_texture_3d *tex)
{
	return tex->depth;
}
