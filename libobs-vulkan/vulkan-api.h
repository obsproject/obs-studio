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

#include "vulkan-loader.h"

/* 提供方便的宏访问来自函数表的 Vulkan 函数
 * 这样可以在代码中直接使用 vkXxx() 的形式，
 * 但实际上访问的是动态加载的函数指针
 */

/* 全局函数 */
#define vkCreateInstance vk_function_table.vkCreateInstance
#define vkEnumerateInstanceExtensionProperties vk_function_table.vkEnumerateInstanceExtensionProperties
#define vkEnumerateInstanceLayerProperties vk_function_table.vkEnumerateInstanceLayerProperties
#define vkGetInstanceProcAddr vk_function_table.vkGetInstanceProcAddr

/* 实例函数 */
#define vkDestroyInstance vk_function_table.vkDestroyInstance
#define vkEnumeratePhysicalDevices vk_function_table.vkEnumeratePhysicalDevices
#define vkGetPhysicalDeviceProperties vk_function_table.vkGetPhysicalDeviceProperties
#define vkGetPhysicalDeviceFeatures vk_function_table.vkGetPhysicalDeviceFeatures
#define vkGetPhysicalDeviceQueueFamilyProperties vk_function_table.vkGetPhysicalDeviceQueueFamilyProperties
#define vkGetPhysicalDeviceMemoryProperties vk_function_table.vkGetPhysicalDeviceMemoryProperties
#define vkGetPhysicalDeviceFormatProperties vk_function_table.vkGetPhysicalDeviceFormatProperties
#define vkGetPhysicalDeviceImageFormatProperties vk_function_table.vkGetPhysicalDeviceImageFormatProperties
#define vkCreateDevice vk_function_table.vkCreateDevice
#define vkEnumerateDeviceExtensionProperties vk_function_table.vkEnumerateDeviceExtensionProperties
#define vkEnumerateDeviceLayerProperties vk_function_table.vkEnumerateDeviceLayerProperties
#define vkGetPhysicalDeviceSparseImageFormatProperties vk_function_table.vkGetPhysicalDeviceSparseImageFormatProperties

/* 设备函数 */
#define vkDestroyDevice vk_function_table.vkDestroyDevice
#define vkGetDeviceQueue vk_function_table.vkGetDeviceQueue
#define vkQueueWaitIdle vk_function_table.vkQueueWaitIdle
#define vkQueueSubmit vk_function_table.vkQueueSubmit
#define vkDeviceWaitIdle vk_function_table.vkDeviceWaitIdle

/* 内存函数 */
#define vkAllocateMemory vk_function_table.vkAllocateMemory
#define vkFreeMemory vk_function_table.vkFreeMemory
#define vkMapMemory vk_function_table.vkMapMemory
#define vkUnmapMemory vk_function_table.vkUnmapMemory
#define vkFlushMappedMemoryRanges vk_function_table.vkFlushMappedMemoryRanges
#define vkInvalidateMappedMemoryRanges vk_function_table.vkInvalidateMappedMemoryRanges
#define vkGetDeviceMemoryCommitment vk_function_table.vkGetDeviceMemoryCommitment
#define vkBindBufferMemory vk_function_table.vkBindBufferMemory
#define vkBindImageMemory vk_function_table.vkBindImageMemory

/* 缓冲区函数 */
#define vkCreateBuffer vk_function_table.vkCreateBuffer
#define vkDestroyBuffer vk_function_table.vkDestroyBuffer
#define vkCreateBufferView vk_function_table.vkCreateBufferView
#define vkDestroyBufferView vk_function_table.vkDestroyBufferView
#define vkGetBufferMemoryRequirements vk_function_table.vkGetBufferMemoryRequirements

/* 图像函数 */
#define vkCreateImage vk_function_table.vkCreateImage
#define vkDestroyImage vk_function_table.vkDestroyImage
#define vkGetImageSubresourceLayout vk_function_table.vkGetImageSubresourceLayout
#define vkGetImageMemoryRequirements vk_function_table.vkGetImageMemoryRequirements
#define vkCreateImageView vk_function_table.vkCreateImageView
#define vkDestroyImageView vk_function_table.vkDestroyImageView

/* 采样器函数 */
#define vkCreateSampler vk_function_table.vkCreateSampler
#define vkDestroySampler vk_function_table.vkDestroySampler

/* 描述符函数 */
#define vkCreateDescriptorSetLayout vk_function_table.vkCreateDescriptorSetLayout
#define vkDestroyDescriptorSetLayout vk_function_table.vkDestroyDescriptorSetLayout
#define vkCreateDescriptorPool vk_function_table.vkCreateDescriptorPool
#define vkDestroyDescriptorPool vk_function_table.vkDestroyDescriptorPool
#define vkResetDescriptorPool vk_function_table.vkResetDescriptorPool
#define vkAllocateDescriptorSets vk_function_table.vkAllocateDescriptorSets
#define vkFreeDescriptorSets vk_function_table.vkFreeDescriptorSets
#define vkUpdateDescriptorSets vk_function_table.vkUpdateDescriptorSets

/* 着色器函数 */
#define vkCreateShaderModule vk_function_table.vkCreateShaderModule
#define vkDestroyShaderModule vk_function_table.vkDestroyShaderModule

/* 管道函数 */
#define vkCreatePipelineLayout vk_function_table.vkCreatePipelineLayout
#define vkDestroyPipelineLayout vk_function_table.vkDestroyPipelineLayout
#define vkCreateGraphicsPipelines vk_function_table.vkCreateGraphicsPipelines
#define vkCreateComputePipelines vk_function_table.vkCreateComputePipelines
#define vkDestroyPipeline vk_function_table.vkDestroyPipeline

/* 渲染通道函数 */
#define vkCreateRenderPass vk_function_table.vkCreateRenderPass
#define vkDestroyRenderPass vk_function_table.vkDestroyRenderPass

/* 帧缓冲区函数 */
#define vkCreateFramebuffer vk_function_table.vkCreateFramebuffer
#define vkDestroyFramebuffer vk_function_table.vkDestroyFramebuffer

/* 命令池函数 */
#define vkCreateCommandPool vk_function_table.vkCreateCommandPool
#define vkDestroyCommandPool vk_function_table.vkDestroyCommandPool
#define vkResetCommandPool vk_function_table.vkResetCommandPool

/* 命令缓冲区函数 */
#define vkAllocateCommandBuffers vk_function_table.vkAllocateCommandBuffers
#define vkFreeCommandBuffers vk_function_table.vkFreeCommandBuffers
#define vkBeginCommandBuffer vk_function_table.vkBeginCommandBuffer
#define vkEndCommandBuffer vk_function_table.vkEndCommandBuffer
#define vkResetCommandBuffer vk_function_table.vkResetCommandBuffer
#define vkCmdBindPipeline vk_function_table.vkCmdBindPipeline
#define vkCmdSetViewport vk_function_table.vkCmdSetViewport
#define vkCmdSetScissor vk_function_table.vkCmdSetScissor
#define vkCmdSetLineWidth vk_function_table.vkCmdSetLineWidth
#define vkCmdSetDepthBias vk_function_table.vkCmdSetDepthBias
#define vkCmdSetBlendConstants vk_function_table.vkCmdSetBlendConstants
#define vkCmdSetDepthBounds vk_function_table.vkCmdSetDepthBounds
#define vkCmdSetStencilCompareMask vk_function_table.vkCmdSetStencilCompareMask
#define vkCmdSetStencilWriteMask vk_function_table.vkCmdSetStencilWriteMask
#define vkCmdSetStencilReference vk_function_table.vkCmdSetStencilReference
#define vkCmdBindDescriptorSets vk_function_table.vkCmdBindDescriptorSets
#define vkCmdBindIndexBuffer vk_function_table.vkCmdBindIndexBuffer
#define vkCmdBindVertexBuffers vk_function_table.vkCmdBindVertexBuffers
#define vkCmdDraw vk_function_table.vkCmdDraw
#define vkCmdDrawIndexed vk_function_table.vkCmdDrawIndexed
#define vkCmdDrawIndirect vk_function_table.vkCmdDrawIndirect
#define vkCmdDrawIndexedIndirect vk_function_table.vkCmdDrawIndexedIndirect
#define vkCmdDispatch vk_function_table.vkCmdDispatch
#define vkCmdDispatchIndirect vk_function_table.vkCmdDispatchIndirect
#define vkCmdCopyBuffer vk_function_table.vkCmdCopyBuffer
#define vkCmdCopyImage vk_function_table.vkCmdCopyImage
#define vkCmdBlitImage vk_function_table.vkCmdBlitImage
#define vkCmdCopyBufferToImage vk_function_table.vkCmdCopyBufferToImage
#define vkCmdCopyImageToBuffer vk_function_table.vkCmdCopyImageToBuffer
#define vkCmdUpdateBuffer vk_function_table.vkCmdUpdateBuffer
#define vkCmdFillBuffer vk_function_table.vkCmdFillBuffer
#define vkCmdClearColorImage vk_function_table.vkCmdClearColorImage
#define vkCmdClearDepthStencilImage vk_function_table.vkCmdClearDepthStencilImage
#define vkCmdClearAttachments vk_function_table.vkCmdClearAttachments
#define vkCmdResolveImage vk_function_table.vkCmdResolveImage
#define vkCmdSetEvent vk_function_table.vkCmdSetEvent
#define vkCmdResetEvent vk_function_table.vkCmdResetEvent
#define vkCmdWaitEvents vk_function_table.vkCmdWaitEvents
#define vkCmdPipelineBarrier vk_function_table.vkCmdPipelineBarrier
#define vkCmdBeginQuery vk_function_table.vkCmdBeginQuery
#define vkCmdEndQuery vk_function_table.vkCmdEndQuery
#define vkCmdResetQueryPool vk_function_table.vkCmdResetQueryPool
#define vkCmdWriteTimestamp vk_function_table.vkCmdWriteTimestamp
#define vkCmdCopyQueryPoolResults vk_function_table.vkCmdCopyQueryPoolResults
#define vkCmdPushConstants vk_function_table.vkCmdPushConstants
#define vkCmdBeginRenderPass vk_function_table.vkCmdBeginRenderPass
#define vkCmdNextSubpass vk_function_table.vkCmdNextSubpass
#define vkCmdEndRenderPass vk_function_table.vkCmdEndRenderPass
#define vkCmdExecuteCommands vk_function_table.vkCmdExecuteCommands

/* 查询函数 */
#define vkCreateQueryPool vk_function_table.vkCreateQueryPool
#define vkDestroyQueryPool vk_function_table.vkDestroyQueryPool
#define vkGetQueryPoolResults vk_function_table.vkGetQueryPoolResults

/* 栅栏和信号量 */
#define vkCreateFence vk_function_table.vkCreateFence
#define vkDestroyFence vk_function_table.vkDestroyFence
#define vkResetFences vk_function_table.vkResetFences
#define vkGetFenceStatus vk_function_table.vkGetFenceStatus
#define vkWaitForFences vk_function_table.vkWaitForFences
#define vkCreateSemaphore vk_function_table.vkCreateSemaphore
#define vkDestroySemaphore vk_function_table.vkDestroySemaphore
#define vkCreateEvent vk_function_table.vkCreateEvent
#define vkDestroyEvent vk_function_table.vkDestroyEvent
#define vkGetEventStatus vk_function_table.vkGetEventStatus
#define vkSetEvent vk_function_table.vkSetEvent
#define vkResetEvent vk_function_table.vkResetEvent

/* 设备过程地址 */
#define vkGetDeviceProcAddr vk_function_table.vkGetDeviceProcAddr

/* KHR 扩展 - Win32 */
#define vkCreateWin32SurfaceKHR vk_function_table.vkCreateWin32SurfaceKHR
#define vkGetPhysicalDeviceWin32PresentationSupportKHR vk_function_table.vkGetPhysicalDeviceWin32PresentationSupportKHR

/* KHR 扩展 - Surface */
#define vkDestroySurfaceKHR vk_function_table.vkDestroySurfaceKHR
#define vkGetPhysicalDeviceSurfaceSupportKHR vk_function_table.vkGetPhysicalDeviceSurfaceSupportKHR
#define vkGetPhysicalDeviceSurfaceCapabilitiesKHR vk_function_table.vkGetPhysicalDeviceSurfaceCapabilitiesKHR
#define vkGetPhysicalDeviceSurfaceFormatsKHR vk_function_table.vkGetPhysicalDeviceSurfaceFormatsKHR
#define vkGetPhysicalDeviceSurfacePresentModesKHR vk_function_table.vkGetPhysicalDeviceSurfacePresentModesKHR

/* KHR 扩展 - Swapchain */
#define vkCreateSwapchainKHR vk_function_table.vkCreateSwapchainKHR
#define vkDestroySwapchainKHR vk_function_table.vkDestroySwapchainKHR
#define vkGetSwapchainImagesKHR vk_function_table.vkGetSwapchainImagesKHR
#define vkAcquireNextImageKHR vk_function_table.vkAcquireNextImageKHR
#define vkQueuePresentKHR vk_function_table.vkQueuePresentKHR

#endif
