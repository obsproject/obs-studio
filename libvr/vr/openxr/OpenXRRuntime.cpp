#include "libvr/OpenXRRuntime.h"
#include <iostream>
#include <vector>
#include <cstring>

namespace libvr {

#define XR_CHECK(x) \
    do { \
        XrResult err = x; \
        if (XR_FAILED(err)) { \
            std::cerr << "[OpenXR] Error: " << err << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            return false; \
        } \
    } while (0)

OpenXRRuntime::OpenXRRuntime() = default;

OpenXRRuntime::~OpenXRRuntime() {
    Shutdown();
}

bool OpenXRRuntime::Initialize(const Config& config) {
    if (!CreateInstance(config)) return false;
    if (!GetSystem()) return false;
    
    // Note: Session creation usually requires the Graphics Device (Vulkan) to be created first
    // and passed to XrGraphicsBindingVulkanKHR. 
    // For this Stage 1, we are just initializing the Instance/System to verify basic OpenXR presence.
    // Creating the complete session with Vulkan binding will be done when we integrate the RenderEngine.
    // So for now, we stop after System if we don't have a graphics binding yet.
    std::cout << "[OpenXR] Init successful (Instance/System)." << std::endl;
    initialized = true;
    return true;
}

void OpenXRRuntime::Shutdown() {
    if (session != XR_NULL_HANDLE) {
        xrDestroySession(session);
        session = XR_NULL_HANDLE;
    }
    if (debugMessenger != XR_NULL_HANDLE) {
        // Destroy debug messenger if extensions loaded
    }
    if (instance != XR_NULL_HANDLE) {
        xrDestroyInstance(instance);
        instance = XR_NULL_HANDLE;
    }
    initialized = false;
}

bool OpenXRRuntime::CreateInstance(const Config& config) {
    // 1. Enumerate Extensions
    uint32_t extCount = 0;
    xrEnumerateInstanceExtensionProperties(nullptr, 0, &extCount, nullptr);
    std::vector<XrExtensionProperties> extensions(extCount, {XR_TYPE_EXTENSION_PROPERTIES});
    xrEnumerateInstanceExtensionProperties(nullptr, extCount, &extCount, extensions.data());

    std::vector<const char*> enabledExtensions;
    // Essential for Vulkan
    if (CheckExtension("XR_KHR_vulkan_enable2", extensions)) {
        enabledExtensions.push_back("XR_KHR_vulkan_enable2");
    } else if (CheckExtension("XR_KHR_vulkan_enable", extensions)) {
        enabledExtensions.push_back("XR_KHR_vulkan_enable");
    } else {
        std::cerr << "[OpenXR] Vulkan extension not found!" << std::endl;
        // Proceeding might fail later, but let's try
    }

    if (config.enable_debug && CheckExtension(XR_EXT_DEBUG_UTILS_EXTENSION_NAME, extensions)) {
        enabledExtensions.push_back(XR_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    XrApplicationInfo appInfo = {};
    strncpy(appInfo.applicationName, config.app_name.c_str(), XR_MAX_APPLICATION_NAME_SIZE - 1);
    appInfo.applicationVersion = config.app_version;
    strncpy(appInfo.engineName, "libvr", XR_MAX_ENGINE_NAME_SIZE - 1);
    appInfo.engineVersion = 1;
    appInfo.apiVersion = XR_CURRENT_API_VERSION;

    XrInstanceCreateInfo createInfo = {XR_TYPE_INSTANCE_CREATE_INFO};
    createInfo.applicationInfo = appInfo;
    createInfo.enabledExtensionCount = (uint32_t)enabledExtensions.size();
    createInfo.enabledExtensionNames = enabledExtensions.data();

    XR_CHECK(xrCreateInstance(&createInfo, &instance));
    std::cout << "[OpenXR] Instance created." << std::endl;
    return true;
}

bool OpenXRRuntime::GetSystem() {
    XrSystemGetInfo systemInfo = {XR_TYPE_SYSTEM_GET_INFO};
    systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    
    XrResult res = xrGetSystem(instance, &systemInfo, &systemId);
    if (XR_FAILED(res)) {
        std::cerr << "[OpenXR] Failed to get system (HMD not found?)" << std::endl;
        return false;
    }
    
    XrSystemProperties props = {XR_TYPE_SYSTEM_PROPERTIES};
    xrGetSystemProperties(instance, systemId, &props);
    std::cout << "[OpenXR] System Name: " << props.systemName << std::endl;
    return true;
}

bool OpenXRRuntime::CheckExtension(const char* name, const std::vector<XrExtensionProperties>& available) {
    for (const auto& ext : available) {
        if (strcmp(name, ext.extensionName) == 0) return true;
    }
    return false;
}

bool OpenXRRuntime::GetRequiredVulkanExtensions(std::vector<std::string>& outExtensions) {
    if (!initialized || systemId == XR_NULL_SYSTEM_ID) return false;

    // Use OpenXR to figure out what Vulkan extensions are needed
    PFN_xrGetVulkanGraphicsRequirements2KHR pfnGetVulkanGraphicsRequirements2KHR = nullptr;
    xrGetInstanceProcAddr(instance, "xrGetVulkanGraphicsRequirements2KHR", (PFN_xrVoidFunction*)&pfnGetVulkanGraphicsRequirements2KHR);

    // If binding is not available (e.g. extension not loaded), this is a problem
    if (!pfnGetVulkanGraphicsRequirements2KHR) {
        std::cerr << "[OpenXR] Failed to load xrGetVulkanGraphicsRequirements2KHR. Extension missing?" << std::endl;
        return false;
    }
    
    XrGraphicsRequirementsVulkan2KHR graphicsRequirements = {XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN2_KHR};
    if (XR_FAILED(pfnGetVulkanGraphicsRequirements2KHR(instance, systemId, &graphicsRequirements))) {
        std::cerr << "[OpenXR] Failed to get Vulkan graphics requirements." << std::endl;
        return false;
    }
    
    // Actually, OpenXR doesn't give us the list of EXTENSIONS here, it gives us Version requirements.
    // The extensions (like VK_KHR_external_memory_fd) are "implied" by the OpenXR extension we enabled (XR_KHR_vulkan_enable2).
    // HOWEVER, legacy xrGetVulkanInstanceExtensionsKHR IS what we want to list them for vkCreateInstance.
    // Let's use that one.
    
    PFN_xrGetVulkanInstanceExtensionsKHR pfnGetVulkanInstanceExtensionsKHR = nullptr;
    xrGetInstanceProcAddr(instance, "xrGetVulkanInstanceExtensionsKHR", (PFN_xrVoidFunction*)&pfnGetVulkanInstanceExtensionsKHR);

    if (!pfnGetVulkanInstanceExtensionsKHR) {
         std::cerr << "[OpenXR] Failed to load xrGetVulkanInstanceExtensionsKHR." << std::endl;
         return false;
    }

    uint32_t bufferSize = 0;
    pfnGetVulkanInstanceExtensionsKHR(instance, systemId, 0, &bufferSize, nullptr);
    std::string buffer(bufferSize, '\0');
    pfnGetVulkanInstanceExtensionsKHR(instance, systemId, bufferSize, &bufferSize, &buffer[0]);
    
    // Parse space-separated string
    size_t pos = 0;
    while ((pos = buffer.find(' ')) != std::string::npos) {
        outExtensions.push_back(buffer.substr(0, pos));
        buffer.erase(0, pos + 1);
    }
    if (!buffer.empty()) {
        // Remove null terminator if present
        if (buffer.back() == '\0') buffer.pop_back();
        if (!buffer.empty()) outExtensions.push_back(buffer);
    }
    
    return true;
}

bool OpenXRRuntime::CreateVulkanSession(VkInstance vkInstance, VkPhysicalDevice vkPhysDev, VkDevice vkDevice, uint32_t queueFamilyIndex) {
    if (!initialized) return false;

    XrGraphicsBindingVulkanKHR binding = {XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR};
    binding.instance = vkInstance;
    binding.physicalDevice = vkPhysDev;
    binding.device = vkDevice;
    binding.queueFamilyIndex = queueFamilyIndex;
    binding.queueIndex = 0;

    XrSessionCreateInfo createInfo = {XR_TYPE_SESSION_CREATE_INFO};
    createInfo.next = &binding;
    createInfo.systemId = systemId;
    
    if (XR_FAILED(xrCreateSession(instance, &createInfo, &session))) {
         std::cerr << "[OpenXR] Failed to create Session with Vulkan binding." << std::endl;
         return false;
    }
    
    // Create Reference Space (e.g. Local or Stage)
    XrReferenceSpaceCreateInfo spaceInfo = {XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    spaceInfo.poseInReferenceSpace.orientation.w = 1.0f;
    
    if (XR_FAILED(xrCreateReferenceSpace(session, &spaceInfo, &appSpace))) {
        std::cerr << "[OpenXR] Failed to create Reference Space." << std::endl;
        return false;
    }

    std::cout << "[OpenXR] Session created successfully with Vulkan binding." << std::endl;
    return true;
}

bool OpenXRRuntime::CreateSwapchains(int width, int height) {
    // Scaffold impl
    return true;
}

void OpenXRRuntime::RunLoop(std::function<void()> frame_callback) {
    if (!initialized) return;

    XrEventDataBuffer event = {XR_TYPE_EVENT_DATA_BUFFER};
    bool running = true;

    while (running) {
        // 1. Poll Events
        while (xrPollEvent(instance, &event) == XR_SUCCESS) {
            switch (event.type) {
                case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
                    auto sessionStateChanged = reinterpret_cast<XrEventDataSessionStateChanged*>(&event);
                    sessionState = sessionStateChanged->state;
                    if (sessionState == XR_SESSION_STATE_READY) {
                        XrSessionBeginInfo beginInfo = {XR_TYPE_SESSION_BEGIN_INFO};
                        beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
                        xrBeginSession(session, &beginInfo);
                        sessionRunning = true;
                        std::cout << "[OpenXR] Session Begin." << std::endl;
                    } else if (sessionState == XR_SESSION_STATE_STOPPING) {
                        xrEndSession(session);
                        sessionRunning = false;
                        std::cout << "[OpenXR] Session End." << std::endl;
                    } else if (sessionState == XR_SESSION_STATE_EXITING) {
                        running = false;
                    }
                    break;
                }
                // Handle interaction profiles, reference spaces, etc.
                default:
                    break;
            }
            event = {XR_TYPE_EVENT_DATA_BUFFER};
        }

        if (!sessionRunning) {
             // Sleep or wait for events
             // std::this_thread::sleep_for(std::chrono::milliseconds(10));
             continue;
        }

        // 2. Wait Frame
        XrFrameState frameState = {XR_TYPE_FRAME_STATE};
        XrFrameWaitInfo waitInfo = {XR_TYPE_FRAME_WAIT_INFO};
        if (XR_FAILED(xrWaitFrame(session, &waitInfo, &frameState))) {
            continue;
        }

        // 3. Begin Frame
        XrFrameBeginInfo beginInfo = {XR_TYPE_FRAME_BEGIN_INFO};
        if (XR_FAILED(xrBeginFrame(session, &beginInfo))) {
            continue;
        }

        // 4. Update & Render (Callback)
        // Pass frameState.predictedDisplayTime to callback for simulation
        if (frameState.shouldRender) {
             // Locate Views (Projections)
             // XR_TYPE_VIEW_LOCATE_INFO ...
             
             if (frame_callback) frame_callback();
             
             // In real impl: Update Scene -> Draw -> Release Swapchain Image
        }

        // 5. End Frame
        XrFrameEndInfo endInfo = {XR_TYPE_FRAME_END_INFO};
        endInfo.displayTime = frameState.predictedDisplayTime;
        endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE; // or determined by system
        // Layer submission...
        // For now, submit 0 layers
        endInfo.layerCount = 0;
        endInfo.layers = nullptr;
        
        xrEndFrame(session, &endInfo);
    }
}

} // namespace libvr
