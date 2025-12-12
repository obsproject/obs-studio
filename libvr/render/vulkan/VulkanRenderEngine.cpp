#include "libvr/IRenderEngine.h"
#include "libvr/SceneManager.h"
#include <iostream>
#include <vector>
#include <stdexcept>
#include "gpu/vulkan_utils.h"

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

        std::vector<const char*> instanceExtensions;
        for (const auto& ext : config.required_extensions) {
            instanceExtensions.push_back(ext.c_str());
        }

        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledLayerCount = (uint32_t)layers.size();
        createInfo.ppEnabledLayerNames = layers.data();
        createInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
        createInfo.ppEnabledExtensionNames = instanceExtensions.data();

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
                graphicsQueueFamily = i;
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
        
        std::vector<const char*> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME, // Usually needed for presentation
            VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
            VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
            VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME,
            VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME
        };

        deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
        deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

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

    // Accessors
    void* GetInstance() const override { return instance; }
    void* GetPhysicalDevice() const override { return physicalDevice; }
    void* GetDevice() const override { return device; }
    uint32_t GetGraphicsQueueFamily() const override { return graphicsQueueFamily; }


    bool BeginFrame() override {
        if (!initialized) return false;
        // Real implementation: Reset command pool, acquire image
        return true;
    }


    
    void DrawScene(const SceneManager* scene) override {
         if (!scene) return;
         
         const auto& nodes = scene->GetNodes();
         std::cout << "[Vulkan] Drawing Scene with " << nodes.size() << " nodes..." << std::endl;
         
         // Mock command recording
         for (const auto& node : nodes) {
             if (node.mesh_id != 0) {
                 std::cout << "  - Recording Draw for Node " << node.id << " (Mesh " << node.mesh_id << ")" << std::endl;
                 // vkCmdBindPipeline...
                 // vkCmdBindVertexBuffers...
                 // vkCmdDraw...
             }
         }
    }

    GPUFrameView GetOutputFrame() override {
        // Lazy allocation of the output frame for testing Interop
        if (outputImage == VK_NULL_HANDLE) {
            VkExternalMemoryImageCreateInfo extImageInfo = {};
            extImageInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
            extImageInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;

            VkImageCreateInfo imageInfo = {};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.pNext = &extImageInfo;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = 1920; // Default test res
            imageInfo.extent.height = 1080;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

            if (vkCreateImage(device, &imageInfo, nullptr, &outputImage) != VK_SUCCESS) {
                std::cerr << "[Vulkan] Failed to create output image" << std::endl;
                return {};
            }

            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(device, outputImage, &memRequirements);

            VkExportMemoryAllocateInfo exportAllocInfo = {};
            exportAllocInfo.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
            exportAllocInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;

            VkMemoryAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.pNext = &exportAllocInfo;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            if (vkAllocateMemory(device, &allocInfo, nullptr, &outputMemory) != VK_SUCCESS) {
                std::cerr << "[Vulkan] Failed to allocate output image memory" << std::endl;
                return {};
            }

            vkBindImageMemory(device, outputImage, outputMemory, 0);
            
            // Export the FD
            if (!ExportImageFd(device, outputMemory, &outputFd)) {
                 std::cerr << "[Vulkan] Failed to export FD" << std::endl;
            } else {
                 std::cout << "[Vulkan] Exported Image FD: " << outputFd << std::endl;
            }
        }

        GPUFrameView view = {};
        // Populate view with our specific handle struct
        // We need to manage the lifecycle of this struct. 
        // For this phase, we use a static or member struct, or strict C-ABI handling
        // But GPUFrameView.handle is void*. 
        // We will pass the VulkanImageHandle pointer.
        
        static VulkanImageHandle vkhz; // Simple static for single-instance test
        vkhz.image = outputImage;
        vkhz.memory = outputMemory;
        vkhz.fd = outputFd; // The consumer must NOT close this if we want to reuse it, OR we dupe it.
        // Opaque FD ownership rules: usually transferring ownership closes it on sender side?
        // Wait, transferring FD usually means 'sendmsg' over socket, or just dup.
        // If we pass `int` in process, it's just an index.
        // Import consumes it? "VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT... importing it transfers ownership of the file descriptor to the Vulkan implementation."
        // So correct flow: We keeping owning it. Consumer uses dup() if they import it into another API that consumes it?
        // Or we export a fresh FD every time? "Multiple file descriptors may be exported from the same memory object".
        // Let's re-export to be safe for now, OR rely on dup.
        // Actually, let's keep it simple: We hold the FD. If the consumer imports it to CUDA, CUDA likely takes ownership or needs a dup.
        // NOTE: Standard POSIX: `dup(fd)`
        
        vkhz.width = 1920;
        vkhz.height = 1080;
        vkhz.format = VK_FORMAT_R8G8B8A8_UNORM;
        
        view.handle = &vkhz; 
        view.width = 1920;
        view.height = 1080;
        view.color.space = Colorspace_Rec709;
        
        return view;
    }

private:
    bool initialized = false;
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamily = 0;
    
    // Test Output Frame State
    VkImage outputImage = VK_NULL_HANDLE;
    VkDeviceMemory outputMemory = VK_NULL_HANDLE;
    int outputFd = -1;
};

// Factory function for testing/creation
std::unique_ptr<IRenderEngine> CreateVulkanRenderEngine() {
    return std::make_unique<VulkanRenderEngine>();
}

} // namespace libvr
