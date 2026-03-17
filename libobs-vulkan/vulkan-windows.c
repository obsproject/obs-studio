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
#include <vulkan/vulkan_win32.h>

/* Windows-specific Vulkan initialization */

struct vk_platform {
	void *reserved; /* Reserved for future use */
};

struct vk_windowinfo {
	HWND hwnd;
	HDC hdc;
};

struct vk_platform *vk_platform_create(struct gs_device *device, uint32_t adapter)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(adapter);

	struct vk_platform *plat = bzalloc(sizeof(struct vk_platform));
	blog(LOG_INFO, "Vulkan platform created for Windows");
	return plat;
}

void vk_platform_destroy(struct vk_platform *plat)
{
	if (plat)
		bfree(plat);
}

struct vk_windowinfo *vk_windowinfo_create(const struct gs_init_data *info)
{
	struct vk_windowinfo *wi = bzalloc(sizeof(struct vk_windowinfo));

	wi->hwnd = (HWND)info->window.hwnd;
	wi->hdc = GetDC(wi->hwnd);

	if (!wi->hdc) {
		blog(LOG_ERROR, "Failed to get device context");
		bfree(wi);
		return NULL;
	}

	blog(LOG_INFO, "Vulkan window info created (HWND: %p)", wi->hwnd);
	return wi;
}

void vk_windowinfo_destroy(struct vk_windowinfo *wi)
{
	if (!wi)
		return;

	if (wi->hdc)
		ReleaseDC(wi->hwnd, wi->hdc);

	bfree(wi);
}

bool vk_platform_init_swapchain(struct gs_swap_chain *swap)
{
	struct gs_device *device = swap->device;
	struct vk_windowinfo *wi = swap->wi;

	/* Create Win32 surface */
	VkWin32SurfaceCreateInfoKHR surface_info = {
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.hinstance = GetModuleHandle(NULL),
		.hwnd = wi->hwnd,
	};

	VkResult result = vkCreateWin32SurfaceKHR(device->instance, &surface_info, NULL, &swap->surface);
	if (result != VK_SUCCESS) {
		blog(LOG_ERROR, "Failed to create Win32 surface: %s", vk_get_error_string(result));
		return false;
	}

	/* Check if graphics queue supports presentation */
	VkBool32 present_support = VK_FALSE;
	vkGetPhysicalDeviceSurfaceSupportKHR(device->gpu, device->graphics_queue_family_index, swap->surface,
					     &present_support);

	if (!present_support) {
		blog(LOG_ERROR, "Graphics queue family does not support presentation");
		vkDestroySurfaceKHR(device->instance, swap->surface, NULL);
		return false;
	}

	/* Get surface capabilities */
	VkSurfaceCapabilitiesKHR capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->gpu, swap->surface, &capabilities);

	/* Create swap chain */
	VkSwapchainCreateInfoKHR swapchain_info = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = swap->surface,
		.minImageCount = (capabilities.minImageCount + 1 <= capabilities.maxImageCount) ? capabilities.minImageCount + 1
													     : capabilities.minImageCount,
		.imageFormat = VK_FORMAT_B8G8R8A8_SRGB,
		.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
		.imageExtent = {.width = swap->info.cx, .height = swap->info.cy},
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 1,
		.pQueueFamilyIndices = &device->graphics_queue_family_index,
		.preTransform = capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = VK_PRESENT_MODE_FIFO_KHR,
		.clipped = VK_TRUE,
	};

	result = vkCreateSwapchainKHR(device->device, &swapchain_info, NULL, &swap->swapchain);
	if (result != VK_SUCCESS) {
		blog(LOG_ERROR, "Failed to create swapchain: %s", vk_get_error_string(result));
		vkDestroySurfaceKHR(device->instance, swap->surface, NULL);
		return false;
	}

	/* Get swapchain images */
	uint32_t image_count = 0;
	vkGetSwapchainImagesKHR(device->device, swap->swapchain, &image_count, NULL);

	da_resize(swap->images, image_count);
	vkGetSwapchainImagesKHR(device->device, swap->swapchain, &image_count, swap->images.array);

	/* Create image views */
	da_resize(swap->image_views, image_count);
	for (uint32_t i = 0; i < image_count; i++) {
		VkImageViewCreateInfo view_info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = swap->images.array[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = VK_FORMAT_B8G8R8A8_SRGB,
			.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.subresourceRange.baseMipLevel = 0,
			.subresourceRange.levelCount = 1,
			.subresourceRange.baseArrayLayer = 0,
			.subresourceRange.layerCount = 1,
		};

		result = vkCreateImageView(device->device, &view_info, NULL, &swap->image_views.array[i]);
		if (result != VK_SUCCESS) {
			blog(LOG_ERROR, "Failed to create swapchain image view: %s", vk_get_error_string(result));
			vkDestroySwapchainKHR(device->device, swap->swapchain, NULL);
			vkDestroySurfaceKHR(device->instance, swap->surface, NULL);
			return false;
		}
	}

	blog(LOG_INFO, "Swapchain initialized with %u images", image_count);
	return true;
}

void vk_platform_cleanup_swapchain(struct gs_swap_chain *swap)
{
	if (!swap)
		return;

	struct gs_device *device = swap->device;

	/* Destroy image views */
	for (size_t i = 0; i < swap->image_views.num; i++) {
		vkDestroyImageView(device->device, swap->image_views.array[i], NULL);
	}
	da_free(swap->image_views);
	da_free(swap->images);

	if (swap->swapchain)
		vkDestroySwapchainKHR(device->device, swap->swapchain, NULL);

	if (swap->surface)
		vkDestroySurfaceKHR(device->instance, swap->surface, NULL);
}
