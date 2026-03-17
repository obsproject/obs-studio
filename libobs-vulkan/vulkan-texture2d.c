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

/* 2D Texture Implementation */

gs_texture_t *device_texture_2d_create(gs_device_t *device, uint32_t width, uint32_t height,
				       enum gs_color_format color_format, uint32_t levels, const uint8_t **data,
				       uint32_t flags)
{
	struct gs_texture_2d *tex = bzalloc(sizeof(struct gs_texture_2d));
	struct gs_texture *base_tex = &tex->base;

	base_tex->device = device;
	base_tex->type = GS_TEXTURE_2D;
	base_tex->format = color_format;
	base_tex->vk_format = convert_gs_format(color_format);
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
	tex->gen_mipmaps = (flags & GS_BUILD_MIPMAPS) != 0;

	/* Create Vulkan image */
	VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	if (base_tex->is_render_target)
		usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	if (!vk_create_image(device, width, height, base_tex->vk_format, VK_IMAGE_TILING_OPTIMAL, usage,
			     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &base_tex->image, &base_tex->memory)) {
		blog(LOG_ERROR, "Failed to create image");
		bfree(tex);
		return NULL;
	}

	/* Create image view */
	base_tex->image_view = vk_create_image_view(device, base_tex->image, base_tex->vk_format,
						     VK_IMAGE_ASPECT_COLOR_BIT, levels);
	if (!base_tex->image_view) {
		blog(LOG_ERROR, "Failed to create image view");
		vkDestroyImage(device->device, base_tex->image, NULL);
		vkFreeMemory(device->device, base_tex->memory, NULL);
		bfree(tex);
		return NULL;
	}

	/* Transition image layout if we have initial data */
	if (data && data[0]) {
		vk_transition_image_layout(device, base_tex->image, base_tex->vk_format, VK_IMAGE_LAYOUT_UNDEFINED,
					   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, levels);

		/* Create staging buffer and copy data */
		size_t data_size = 0;
		for (uint32_t i = 0; i < levels; i++) {
			if (data[i]) {
				uint32_t level_width = width >> i;
				uint32_t level_height = height >> i;
				if (level_width == 0)
					level_width = 1;
				if (level_height == 0)
					level_height = 1;

				size_t bytes_per_pixel = gs_get_format_bpp(color_format) / 8;
				data_size += level_width * level_height * bytes_per_pixel;
			}
		}

		if (data_size > 0) {
			if (!vk_create_buffer(device, data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
					      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
						      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					      &tex->staging_buffer, &tex->staging_memory)) {
				blog(LOG_ERROR, "Failed to create staging buffer");
				vkDestroyImageView(device->device, base_tex->image_view, NULL);
				vkDestroyImage(device->device, base_tex->image, NULL);
				vkFreeMemory(device->device, base_tex->memory, NULL);
				bfree(tex);
				return NULL;
			}

			/* Copy data to staging buffer */
			void *mapped;
			vkMapMemory(device->device, tex->staging_memory, 0, data_size, 0, &mapped);
			memcpy(mapped, data[0], data_size);
			vkUnmapMemory(device->device, tex->staging_memory);

			vk_copy_buffer_to_image(device, tex->staging_buffer, base_tex->image, width, height);
		}

		vk_transition_image_layout(device, base_tex->image, base_tex->vk_format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, levels);
	}

	blog(LOG_INFO, "Successfully created 2D texture (%ux%u)", width, height);
	return base_tex;
}

void gs_texture_2d_destroy(struct gs_texture_2d *tex)
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

uint32_t gs_texture_2d_get_width(struct gs_texture_2d *tex)
{
	return tex->width;
}

uint32_t gs_texture_2d_get_height(struct gs_texture_2d *tex)
{
	return tex->height;
}

enum gs_color_format gs_texture_2d_get_color_format(struct gs_texture_2d *tex)
{
	return tex->base.format;
}
