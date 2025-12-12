#pragma once

#define XR_USE_GRAPHICS_API_VULKAN
#include <vulkan/vulkan.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <vector>
#include <string>
#include <memory>
#include <functional>

namespace libvr {

class OpenXRRuntime {
public:
    struct Config {
        std::string app_name = "OBS-VR";
        uint32_t app_version = 1;
        bool enable_debug = false;
    };

    OpenXRRuntime();
    ~OpenXRRuntime();

    bool Initialize(const Config& config);
    void Shutdown();

    // Vulkan Binding
    bool GetRequiredVulkanExtensions(std::vector<std::string>& outExtensions);
    bool CreateVulkanSession(VkInstance vkInstance, VkPhysicalDevice vkPhysDev, VkDevice vkDevice, uint32_t queueFamilyIndex);
    bool CreateSwapchains(int width, int height); // Simplified for MVP


    // Main Loop
    // The callback is invoked when it's time to process a frame (between wait and begin)
    void RunLoop(std::function<void()> frame_callback);

    // Accessors
    XrInstance GetInstance() const { return instance; }
    XrSession GetSession() const { return session; }
    XrSystemId GetSystemId() const { return systemId; }
    
private:
    bool CreateInstance(const Config& config);
    bool GetSystem();
    bool CreateSession();
    bool CreateSpace();
    
    // Extensions
    bool CheckExtension(const char* name, const std::vector<XrExtensionProperties>& available);

    XrInstance instance = XR_NULL_HANDLE;
    XrSystemId systemId = XR_NULL_SYSTEM_ID;
    XrSession session = XR_NULL_HANDLE;
    XrSpace appSpace = XR_NULL_HANDLE; // Local or Stage space
    
    // Debug
    XrDebugUtilsMessengerEXT debugMessenger = XR_NULL_HANDLE;

    // State
    bool initialized = false;
    bool sessionRunning = false;
    XrSessionState sessionState = XR_SESSION_STATE_UNKNOWN;
};

} // namespace libvr
