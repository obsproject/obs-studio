#include "vulkan_utils.h"
#include <iostream>
#include <vulkan/vulkan.h>
#include <unistd.h> // for close()

// Usually provided by dynamic loader, but we need to fetch it if not static
static PFN_vkGetMemoryFdKHR fpGetMemoryFdKHR = nullptr;

namespace libvr {

bool LoadInteropFunctions(VkDevice device) {
    fpGetMemoryFdKHR = (PFN_vkGetMemoryFdKHR)vkGetDeviceProcAddr(device, "vkGetMemoryFdKHR");
    if (!fpGetMemoryFdKHR) {
        std::cerr << "[Vulkan] Failed to load vkGetMemoryFdKHR" << std::endl;
        return false;
    }
    return true;
}

bool ExportImageFd(VkDevice device, VkDeviceMemory memory, int* out_fd) {
    if (!fpGetMemoryFdKHR && !LoadInteropFunctions(device)) return false;

    VkMemoryGetFdInfoKHR fdInfo = {};
    fdInfo.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
    fdInfo.memory = memory;
    fdInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;

    VkResult res = fpGetMemoryFdKHR(device, &fdInfo, out_fd);
    if (res != VK_SUCCESS) {
        std::cerr << "[Vulkan] Failed to export memory FD: " << res << std::endl;
        return false;
    }
    return true;
}

uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return -1;
}

} // namespace libvr
