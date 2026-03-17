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

#include "vulkan-loader.h"
#include <util/blogging.h>
#include <util/mem.h>

#ifdef _WIN32
#include <windows.h>
#define VULKAN_LIB_NAME "vulkan-1.dll"
typedef HMODULE lib_handle_t;

static inline lib_handle_t load_library(const char *name)
{
	return LoadLibraryA(name);
}

static inline void *get_proc_address(lib_handle_t handle, const char *name)
{
	return (void *)GetProcAddress(handle, name);
}

static inline void free_library(lib_handle_t handle)
{
	FreeLibrary(handle);
}
#else
#include <dlfcn.h>
#define VULKAN_LIB_NAME "libvulkan.so.1"
typedef void *lib_handle_t;

static inline lib_handle_t load_library(const char *name)
{
	return dlopen(name, RTLD_LAZY);
}

static inline void *get_proc_address(lib_handle_t handle, const char *name)
{
	return dlsym(handle, name);
}

static inline void free_library(lib_handle_t handle)
{
	dlclose(handle);
}
#endif

vk_function_table_t vk_function_table = {0};

static lib_handle_t vk_lib_handle = NULL;
static bool vk_lib_loaded = false;

#define LOAD_GLOBAL_FUNC(name)                                                 \
	vk_function_table.name = (PFN_##name)get_proc_address(vk_lib_handle, #name); \
	if (!vk_function_table.name) {                                              \
		blog(LOG_WARNING, "Failed to load global function: " #name);           \
		return false;                                                           \
	}

#define LOAD_INSTANCE_FUNC(instance, name)                                     \
	vk_function_table.name = (PFN_##name)vk_function_table.vkGetInstanceProcAddr(instance, #name); \
	if (!vk_function_table.name) {                                              \
		blog(LOG_WARNING, "Failed to load instance function: " #name);         \
		return false;                                                           \
	}

#define LOAD_DEVICE_FUNC(device, name)                                         \
	vk_function_table.name = (PFN_##name)vk_function_table.vkGetDeviceProcAddr(device, #name); \
	if (!vk_function_table.name) {                                              \
		blog(LOG_WARNING, "Failed to load device function: " #name);           \
		return false;                                                           \
	}

bool vk_loader_init(void)
{
	if (vk_lib_loaded)
		return true;

	blog(LOG_INFO, "Loading Vulkan library: %s", VULKAN_LIB_NAME);

	vk_lib_handle = load_library(VULKAN_LIB_NAME);
	if (!vk_lib_handle) {
#ifdef _WIN32
		blog(LOG_ERROR, "Failed to load Vulkan library. Make sure Vulkan SDK is installed.");
#else
		blog(LOG_ERROR, "Failed to load Vulkan library: %s", dlerror());
#endif
		return false;
	}

	blog(LOG_INFO, "Vulkan library loaded successfully");

	/* 加载全局函数 */
	LOAD_GLOBAL_FUNC(vkCreateInstance);
	LOAD_GLOBAL_FUNC(vkEnumerateInstanceExtensionProperties);
	LOAD_GLOBAL_FUNC(vkEnumerateInstanceLayerProperties);
	LOAD_GLOBAL_FUNC(vkGetInstanceProcAddr);

	vk_lib_loaded = true;
	return true;
}

bool vk_loader_load_instance_functions(VkInstance instance)
{
	if (!vk_lib_loaded)
		return false;

	blog(LOG_DEBUG, "Loading instance-level Vulkan functions");

	/* 实例级别函数 */
	LOAD_INSTANCE_FUNC(instance, vkDestroyInstance);
	LOAD_INSTANCE_FUNC(instance, vkEnumeratePhysicalDevices);
	LOAD_INSTANCE_FUNC(instance, vkGetPhysicalDeviceProperties);
	LOAD_INSTANCE_FUNC(instance, vkGetPhysicalDeviceFeatures);
	LOAD_INSTANCE_FUNC(instance, vkGetPhysicalDeviceQueueFamilyProperties);
	LOAD_INSTANCE_FUNC(instance, vkGetPhysicalDeviceMemoryProperties);
	LOAD_INSTANCE_FUNC(instance, vkGetPhysicalDeviceFormatProperties);
	LOAD_INSTANCE_FUNC(instance, vkGetPhysicalDeviceImageFormatProperties);
	LOAD_INSTANCE_FUNC(instance, vkCreateDevice);
	LOAD_INSTANCE_FUNC(instance, vkEnumerateDeviceExtensionProperties);
	LOAD_INSTANCE_FUNC(instance, vkEnumerateDeviceLayerProperties);
	LOAD_INSTANCE_FUNC(instance, vkGetPhysicalDeviceSparseImageFormatProperties);

	/* Surface 扩展函数 */
	LOAD_INSTANCE_FUNC(instance, vkDestroySurfaceKHR);
	LOAD_INSTANCE_FUNC(instance, vkGetPhysicalDeviceSurfaceSupportKHR);
	LOAD_INSTANCE_FUNC(instance, vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
	LOAD_INSTANCE_FUNC(instance, vkGetPhysicalDeviceSurfaceFormatsKHR);
	LOAD_INSTANCE_FUNC(instance, vkGetPhysicalDeviceSurfacePresentModesKHR);

#ifdef _WIN32
	/* Win32 表面函数 */
	LOAD_INSTANCE_FUNC(instance, vkCreateWin32SurfaceKHR);
	LOAD_INSTANCE_FUNC(instance, vkGetPhysicalDeviceWin32PresentationSupportKHR);
#endif

	blog(LOG_DEBUG, "Instance functions loaded successfully");
	return true;
}

bool vk_loader_load_device_functions(VkDevice device)
{
	if (!vk_lib_loaded)
		return false;

	blog(LOG_DEBUG, "Loading device-level Vulkan functions");

	/* 基本设备函数 */
	LOAD_DEVICE_FUNC(device, vkGetDeviceProcAddr);
	LOAD_DEVICE_FUNC(device, vkDestroyDevice);
	LOAD_DEVICE_FUNC(device, vkGetDeviceQueue);
	LOAD_DEVICE_FUNC(device, vkQueueWaitIdle);
	LOAD_DEVICE_FUNC(device, vkQueueSubmit);
	LOAD_DEVICE_FUNC(device, vkDeviceWaitIdle);

	/* 内存函数 */
	LOAD_DEVICE_FUNC(device, vkAllocateMemory);
	LOAD_DEVICE_FUNC(device, vkFreeMemory);
	LOAD_DEVICE_FUNC(device, vkMapMemory);
	LOAD_DEVICE_FUNC(device, vkUnmapMemory);
	LOAD_DEVICE_FUNC(device, vkFlushMappedMemoryRanges);
	LOAD_DEVICE_FUNC(device, vkInvalidateMappedMemoryRanges);
	LOAD_DEVICE_FUNC(device, vkGetDeviceMemoryCommitment);
	LOAD_DEVICE_FUNC(device, vkBindBufferMemory);
	LOAD_DEVICE_FUNC(device, vkBindImageMemory);

	/* 缓冲区函数 */
	LOAD_DEVICE_FUNC(device, vkCreateBuffer);
	LOAD_DEVICE_FUNC(device, vkDestroyBuffer);
	LOAD_DEVICE_FUNC(device, vkCreateBufferView);
	LOAD_DEVICE_FUNC(device, vkDestroyBufferView);
	LOAD_DEVICE_FUNC(device, vkGetBufferMemoryRequirements);

	/* 图像函数 */
	LOAD_DEVICE_FUNC(device, vkCreateImage);
	LOAD_DEVICE_FUNC(device, vkDestroyImage);
	LOAD_DEVICE_FUNC(device, vkGetImageSubresourceLayout);
	LOAD_DEVICE_FUNC(device, vkGetImageMemoryRequirements);
	LOAD_DEVICE_FUNC(device, vkCreateImageView);
	LOAD_DEVICE_FUNC(device, vkDestroyImageView);

	/* 采样器函数 */
	LOAD_DEVICE_FUNC(device, vkCreateSampler);
	LOAD_DEVICE_FUNC(device, vkDestroySampler);

	/* 描述符函数 */
	LOAD_DEVICE_FUNC(device, vkCreateDescriptorSetLayout);
	LOAD_DEVICE_FUNC(device, vkDestroyDescriptorSetLayout);
	LOAD_DEVICE_FUNC(device, vkCreateDescriptorPool);
	LOAD_DEVICE_FUNC(device, vkDestroyDescriptorPool);
	LOAD_DEVICE_FUNC(device, vkResetDescriptorPool);
	LOAD_DEVICE_FUNC(device, vkAllocateDescriptorSets);
	LOAD_DEVICE_FUNC(device, vkFreeDescriptorSets);
	LOAD_DEVICE_FUNC(device, vkUpdateDescriptorSets);

	/* 着色器函数 */
	LOAD_DEVICE_FUNC(device, vkCreateShaderModule);
	LOAD_DEVICE_FUNC(device, vkDestroyShaderModule);

	/* 管道函数 */
	LOAD_DEVICE_FUNC(device, vkCreatePipelineLayout);
	LOAD_DEVICE_FUNC(device, vkDestroyPipelineLayout);
	LOAD_DEVICE_FUNC(device, vkCreateGraphicsPipelines);
	LOAD_DEVICE_FUNC(device, vkCreateComputePipelines);
	LOAD_DEVICE_FUNC(device, vkDestroyPipeline);

	/* 渲染通道函数 */
	LOAD_DEVICE_FUNC(device, vkCreateRenderPass);
	LOAD_DEVICE_FUNC(device, vkDestroyRenderPass);

	/* 帧缓冲区函数 */
	LOAD_DEVICE_FUNC(device, vkCreateFramebuffer);
	LOAD_DEVICE_FUNC(device, vkDestroyFramebuffer);

	/* 命令池函数 */
	LOAD_DEVICE_FUNC(device, vkCreateCommandPool);
	LOAD_DEVICE_FUNC(device, vkDestroyCommandPool);
	LOAD_DEVICE_FUNC(device, vkResetCommandPool);

	/* 命令缓冲区函数 */
	LOAD_DEVICE_FUNC(device, vkAllocateCommandBuffers);
	LOAD_DEVICE_FUNC(device, vkFreeCommandBuffers);
	LOAD_DEVICE_FUNC(device, vkBeginCommandBuffer);
	LOAD_DEVICE_FUNC(device, vkEndCommandBuffer);
	LOAD_DEVICE_FUNC(device, vkResetCommandBuffer);
	LOAD_DEVICE_FUNC(device, vkCmdBindPipeline);
	LOAD_DEVICE_FUNC(device, vkCmdSetViewport);
	LOAD_DEVICE_FUNC(device, vkCmdSetScissor);
	LOAD_DEVICE_FUNC(device, vkCmdSetLineWidth);
	LOAD_DEVICE_FUNC(device, vkCmdSetDepthBias);
	LOAD_DEVICE_FUNC(device, vkCmdSetBlendConstants);
	LOAD_DEVICE_FUNC(device, vkCmdSetDepthBounds);
	LOAD_DEVICE_FUNC(device, vkCmdSetStencilCompareMask);
	LOAD_DEVICE_FUNC(device, vkCmdSetStencilWriteMask);
	LOAD_DEVICE_FUNC(device, vkCmdSetStencilReference);
	LOAD_DEVICE_FUNC(device, vkCmdBindDescriptorSets);
	LOAD_DEVICE_FUNC(device, vkCmdBindIndexBuffer);
	LOAD_DEVICE_FUNC(device, vkCmdBindVertexBuffers);
	LOAD_DEVICE_FUNC(device, vkCmdDraw);
	LOAD_DEVICE_FUNC(device, vkCmdDrawIndexed);
	LOAD_DEVICE_FUNC(device, vkCmdDrawIndirect);
	LOAD_DEVICE_FUNC(device, vkCmdDrawIndexedIndirect);
	LOAD_DEVICE_FUNC(device, vkCmdDispatch);
	LOAD_DEVICE_FUNC(device, vkCmdDispatchIndirect);
	LOAD_DEVICE_FUNC(device, vkCmdCopyBuffer);
	LOAD_DEVICE_FUNC(device, vkCmdCopyImage);
	LOAD_DEVICE_FUNC(device, vkCmdBlitImage);
	LOAD_DEVICE_FUNC(device, vkCmdCopyBufferToImage);
	LOAD_DEVICE_FUNC(device, vkCmdCopyImageToBuffer);
	LOAD_DEVICE_FUNC(device, vkCmdUpdateBuffer);
	LOAD_DEVICE_FUNC(device, vkCmdFillBuffer);
	LOAD_DEVICE_FUNC(device, vkCmdClearColorImage);
	LOAD_DEVICE_FUNC(device, vkCmdClearDepthStencilImage);
	LOAD_DEVICE_FUNC(device, vkCmdClearAttachments);
	LOAD_DEVICE_FUNC(device, vkCmdResolveImage);
	LOAD_DEVICE_FUNC(device, vkCmdSetEvent);
	LOAD_DEVICE_FUNC(device, vkCmdResetEvent);
	LOAD_DEVICE_FUNC(device, vkCmdWaitEvents);
	LOAD_DEVICE_FUNC(device, vkCmdPipelineBarrier);
	LOAD_DEVICE_FUNC(device, vkCmdBeginQuery);
	LOAD_DEVICE_FUNC(device, vkCmdEndQuery);
	LOAD_DEVICE_FUNC(device, vkCmdResetQueryPool);
	LOAD_DEVICE_FUNC(device, vkCmdWriteTimestamp);
	LOAD_DEVICE_FUNC(device, vkCmdCopyQueryPoolResults);
	LOAD_DEVICE_FUNC(device, vkCmdPushConstants);
	LOAD_DEVICE_FUNC(device, vkCmdBeginRenderPass);
	LOAD_DEVICE_FUNC(device, vkCmdNextSubpass);
	LOAD_DEVICE_FUNC(device, vkCmdEndRenderPass);
	LOAD_DEVICE_FUNC(device, vkCmdExecuteCommands);

	/* 查询函数 */
	LOAD_DEVICE_FUNC(device, vkCreateQueryPool);
	LOAD_DEVICE_FUNC(device, vkDestroyQueryPool);
	LOAD_DEVICE_FUNC(device, vkGetQueryPoolResults);

	/* 栅栏和信号量 */
	LOAD_DEVICE_FUNC(device, vkCreateFence);
	LOAD_DEVICE_FUNC(device, vkDestroyFence);
	LOAD_DEVICE_FUNC(device, vkResetFences);
	LOAD_DEVICE_FUNC(device, vkGetFenceStatus);
	LOAD_DEVICE_FUNC(device, vkWaitForFences);
	LOAD_DEVICE_FUNC(device, vkCreateSemaphore);
	LOAD_DEVICE_FUNC(device, vkDestroySemaphore);
	LOAD_DEVICE_FUNC(device, vkCreateEvent);
	LOAD_DEVICE_FUNC(device, vkDestroyEvent);
	LOAD_DEVICE_FUNC(device, vkGetEventStatus);
	LOAD_DEVICE_FUNC(device, vkSetEvent);
	LOAD_DEVICE_FUNC(device, vkResetEvent);

	/* Swapchain 扩展函数 */
	LOAD_DEVICE_FUNC(device, vkCreateSwapchainKHR);
	LOAD_DEVICE_FUNC(device, vkDestroySwapchainKHR);
	LOAD_DEVICE_FUNC(device, vkGetSwapchainImagesKHR);
	LOAD_DEVICE_FUNC(device, vkAcquireNextImageKHR);
	LOAD_DEVICE_FUNC(device, vkQueuePresentKHR);

	blog(LOG_DEBUG, "Device functions loaded successfully");
	return true;
}

void vk_loader_free(void)
{
	if (!vk_lib_handle)
		return;

	free_library(vk_lib_handle);

	vk_lib_handle = NULL;
	vk_lib_loaded = false;
	memset(&vk_function_table, 0, sizeof(vk_function_table));

	blog(LOG_INFO, "Vulkan library unloaded");
}

bool vk_loader_available(void)
{
	return vk_lib_loaded;
}
