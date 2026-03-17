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

/* Stage Surface Implementation */

gs_stagesurf_t *gs_stagesurface_create(gs_device_t *device, uint32_t width, uint32_t height,
				      enum gs_color_format color_format)
{
	struct gs_stage_surface *surf = bzalloc(sizeof(struct gs_stage_surface));

	surf->device = device;
	surf->format = color_format;
	surf->width = width;
	surf->height = height;
	surf->bytes_per_pixel = gs_get_format_bpp(color_format) / 8;

	VkDeviceSize bufferSize = width * height * surf->bytes_per_pixel;

	if (!vk_create_buffer(device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &surf->buffer,
			      &surf->memory)) {
		blog(LOG_ERROR, "Failed to create stage surface buffer");
		bfree(surf);
		return NULL;
	}

	return surf;
}

void gs_stagesurface_destroy(gs_stagesurf_t *surf)
{
	if (!surf)
		return;

	if (surf->buffer)
		vkDestroyBuffer(surf->device->device, surf->buffer, NULL);
	if (surf->memory)
		vkFreeMemory(surf->device->device, surf->memory, NULL);

	bfree(surf);
}

uint32_t gs_stagesurface_get_width(const gs_stagesurf_t *surf)
{
	if (!surf)
		return 0;
	return ((struct gs_stage_surface *)surf)->width;
}

uint32_t gs_stagesurface_get_height(const gs_stagesurf_t *surf)
{
	if (!surf)
		return 0;
	return ((struct gs_stage_surface *)surf)->height;
}

enum gs_color_format gs_stagesurface_get_color_format(const gs_stagesurf_t *surf)
{
	if (!surf)
		return GS_UNKNOWN;
	return ((struct gs_stage_surface *)surf)->format;
}

/* Z-Stencil Buffer Implementation */

gs_zstencil_t *gs_zstencilbuffer_create(gs_device_t *device, uint32_t width, uint32_t height, enum gs_zstencil_format format)
{
	struct gs_zstencil_buffer *zs = bzalloc(sizeof(struct gs_zstencil_buffer));

	zs->device = device;

	/* Convert format */
	switch (format) {
	case GS_Z16:
		zs->format = VK_FORMAT_D16_UNORM;
		break;
	case GS_Z24_S8:
		zs->format = VK_FORMAT_D24_UNORM_S8_UINT;
		break;
	case GS_Z32F:
		zs->format = VK_FORMAT_D32_SFLOAT;
		break;
	case GS_Z32F_S8X24:
		zs->format = VK_FORMAT_D32_SFLOAT_S8_UINT;
		break;
	default:
		blog(LOG_ERROR, "Invalid z-stencil format");
		bfree(zs);
		return NULL;
	}

	if (!vk_create_image(device, width, height, zs->format, VK_IMAGE_TILING_OPTIMAL,
			     VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &zs->image,
			     &zs->memory)) {
		blog(LOG_ERROR, "Failed to create z-stencil image");
		bfree(zs);
		return NULL;
	}

	VkImageAspectFlags aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
	if (format == GS_Z24_S8 || format == GS_Z32F_S8X24)
		aspect_mask |= VK_IMAGE_ASPECT_STENCIL_BIT;

	zs->image_view = vk_create_image_view(device, zs->image, zs->format, aspect_mask, 1);
	if (!zs->image_view) {
		blog(LOG_ERROR, "Failed to create z-stencil image view");
		vkDestroyImage(device->device, zs->image, NULL);
		vkFreeMemory(device->device, zs->memory, NULL);
		bfree(zs);
		return NULL;
	}

	return zs;
}

void gs_zstencilbuffer_destroy(gs_zstencil_t *zs)
{
	if (!zs)
		return;

	if (zs->image_view)
		vkDestroyImageView(zs->device->device, zs->image_view, NULL);
	if (zs->image)
		vkDestroyImage(zs->device->device, zs->image, NULL);
	if (zs->memory)
		vkFreeMemory(zs->device->device, zs->memory, NULL);

	bfree(zs);
}
