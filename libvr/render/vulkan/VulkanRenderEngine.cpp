#include "libvr/IRenderEngine.h"
#include <iostream>
#include <vector>
#include <stdexcept>
#include <vulkan/vulkan.h>

namespace libvr {

// Helper macros for error checking
#define VK_CHECK(x) \
    do { \
        VkResult err = x; \
        if (err) { \
            std::cerr << "[Vulkan] Error: " << err << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            return false; \
        } \
    } while (0)

class VulkanRenderEngine : public IRenderEngine {
public:
    VulkanRenderEngine() = default;
    
    ~VulkanRenderEngine() override {
        Shutdown();
    }

    bool Initialize(const RenderConfig& config) override {
        std::cout << "[Vulkan] Initializing..." << std::endl;

        // 1. Create Instance
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "OBS-VR";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "libvr";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_2;

        std::vector<const char*> layers;
        if (config.enable_validation_layers) {
            layers.push_back("VK_LAYER_KHRONOS_validation");
        }

        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledLayerCount = (uint32_t)layers.size();
        createInfo.ppEnabledLayerNames = layers.data();

        VK_CHECK(vkCreateInstance(&createInfo, nullptr, &instance));
        std::cout << "[Vulkan] Instance created." << std::endl;

        // 2. Pick Physical Device
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0) {
            std::cerr << "[Vulkan] No GPU with Vulkan support found." << std::endl;
            return false;
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        // Simple selection: Pick the first discrete GPU, or fallback to first device
        physicalDevice = devices[0];
        for (const auto& device : devices) {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(device, &props);
            if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                physicalDevice = device;
                std::cout << "[Vulkan] Selected Discrete GPU: " << props.deviceName << std::endl;
                break;
            }
        }

        // 3. Create Logical Device & Queue
        // Find graphics queue family
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

        int graphicsFamily = -1;
        for (int i = 0; i < queueFamilies.size(); i++) {
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                graphicsFamily = i;
                break;
            }
        }

        if (graphicsFamily == -1) {
            std::cerr << "[Vulkan] No graphics queue family found." << std::endl;
            return false;
        }

        float queuePriority = 1.0f;
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = graphicsFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
        deviceCreateInfo.queueCreateInfoCount = 1;
        // Enable features if needed...

        VK_CHECK(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device));
        vkGetDeviceQueue(device, graphicsFamily, 0, &graphicsQueue);
        std::cout << "[Vulkan] Logical Device created." << std::endl;

        initialized = true;
        return true;
    }

    void Shutdown() override {
        if (device) {
            vkDestroyDevice(device, nullptr);
            device = nullptr;
        }
        if (instance) {
            vkDestroyInstance(instance, nullptr);
            instance = nullptr;
        }
        initialized = false;
        std::cout << "[Vulkan] Shutdown complete." << std::endl;
    }

    bool BeginFrame() override {
        if (!initialized) return false;
        // Real implementation: Reset command pool, acquire image
        return true;
    }

    void DrawScene() override {
        // Real implementation: Record command buffer
    }

    GPUFrameView GetOutputFrame() override {
        GPUFrameView view = {};
        // Placeholder handles for now, but backed by real device structure member
        // In next step, we will implement actual VkImage allocation
        return view;
    }

private:
    bool initialized = false;
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    
    // Future: VmaAllocator allocator;
};

// Factory function for testing/creation
IRenderEngine* CreateVulkanRenderEngine() {
    return new VulkanRenderEngine();
}

} // namespace libvr
