#include "system-info.hpp"

#ifdef __aarch64__
#include <util/platform.h>

#include <sys/types.h>
#include <sys/sysctl.h>
#import <Foundation/Foundation.h>
#import <Foundation/NSProcessInfo.h>

static std::optional<std::string> getCpuName()
{
    std::string name;
    size_t size;
    int ret;

    ret = sysctlbyname("machdep.cpu.brand_string", NULL, &size, NULL, 0);
    if (ret != 0)
        return std::nullopt;

    name.resize(size);

    ret = sysctlbyname("machdep.cpu.brand_string", name.data(), &size, NULL, 0);
    if (ret != 0)
        return std::nullopt;

    // Remove null terminator
    name.resize(name.find('\0'));
    return name;
}

// Apple Silicon Macs have a single SoC that contains both GPU and CPU, the same information is valid for bith.
static void fillSoCInfo(GoLiveApi::Capabilities &capabilities)
{
    capabilities.cpu.name = getCpuName();
    // Getting the frequency is not supported on Apple Silicon
    capabilities.cpu.physical_cores = os_get_physical_cores();
    capabilities.cpu.logical_cores = os_get_logical_cores();

    capabilities.memory.total = os_get_sys_total_size();
    capabilities.memory.free = os_get_sys_free_size();

    // Apple Silicon does not support dGPUs, there's only going to be one (the SoC).
    GoLiveApi::Gpu gpu;
    gpu.model = capabilities.cpu.name.value_or("Unknown");
    gpu.vendor_id = 0x106b;  // Always Apple
    gpu.device_id = 0;       // Always 0 for Apple Silicon

    std::vector<GoLiveApi::Gpu> gpus;
    gpus.push_back(std::move(gpu));
    capabilities.gpu = gpus;
}

static void fillSystemInfo(GoLiveApi::System &sysinfo)
{
    NSProcessInfo *procInfo = [NSProcessInfo processInfo];
    NSOperatingSystemVersion versionObj = [procInfo operatingSystemVersion];

    sysinfo.name = "macOS";
    sysinfo.bits = 64;  // 32-bit macOS is long deprecated.
    sysinfo.version = [[procInfo operatingSystemVersionString] UTF8String];

    switch (versionObj.majorVersion) {
        case 11:
            sysinfo.release = "Big Sur";
            break;
        case 12:
            sysinfo.release = "Monterey";
            break;
        case 13:
            sysinfo.release = "Ventura";
            break;
        case 14:
            sysinfo.release = "Sonoma";
            break;
        case 15:
            sysinfo.release = "Sequoia";
            break;
        default:
            sysinfo.release = "unknown";
    }

    sysinfo.arm = true;
    sysinfo.armEmulation = false;
}
#endif

void system_info(GoLiveApi::Capabilities &capabilities)
{
#ifdef __aarch64__
    fillSoCInfo(capabilities);
    fillSystemInfo(capabilities.system);
#else
    UNUSED_PARAMETER(capabilities);
#endif
}
