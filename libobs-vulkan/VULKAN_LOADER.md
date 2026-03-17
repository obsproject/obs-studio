# Vulkan 动态加载系统

## 概述

libobs-vulkan 使用动态加载方式导入所有 Vulkan API，而不是在编译时链接 Vulkan SDK。这提供了以下优势：

1. **运行时灵活性** - 应用可以在没有 Vulkan SDK 的系统上运行（仅使用其他渲染器）
2. **优雅降级** - 系统没有 Vulkan 时，应用不会启动失败
3. **减少依赖** - 编译时无需 Vulkan SDK，只需头文件
4. **跨平台支持** - 同一代码可支持 Windows、Linux 等平台的 Vulkan 库

## 架构

### 核心组件

```
┌─────────────────────────────────┐
│   应用代码                       │
│   (使用 vkXxx() 函数)           │
└──────────────┬──────────────────┘
               │
┌──────────────▼──────────────────┐
│   vulkan-api.h                  │
│   (宏定义，映射到函数表)        │
└──────────────┬──────────────────┘
               │
┌──────────────▼──────────────────┐
│   vk_function_table             │
│   (全局函数指针表)              │
└──────────────┬──────────────────┘
               │
┌──────────────▼──────────────────┐
│   vulkan-loader.c/.h            │
│   (动态加载实现)               │
└──────────────┬──────────────────┘
               │
┌──────────────▼──────────────────┐
│   vulkan-1.dll / libvulkan.so   │
│   (系统 Vulkan 库)              │
└─────────────────────────────────┘
```

### 文件说明

#### vulkan-loader.h
定义 Vulkan 函数指针表结构 `vk_function_table_t`，包含：
- 全局函数（创建实例等）
- 实例函数（枚举物理设备等）
- 设备函数（创建缓冲区、绘制等）
- KHR 扩展函数（表面、交换链等）

#### vulkan-loader.c
实现三个加载阶段：

1. **初始化** - `vk_loader_init()`
   - 加载 Vulkan 共享库
   - 加载全局函数（vkCreateInstance, vkGetInstanceProcAddr 等）

2. **实例函数** - `vk_loader_load_instance_functions(VkInstance)`
   - 调用后加载实例级别函数
   - 通过 vkGetInstanceProcAddr 获取

3. **设备函数** - `vk_loader_load_device_functions(VkDevice)`
   - 创建设备后加载设备级别函数
   - 通过 vkGetDeviceProcAddr 获取

#### vulkan-api.h
提供便利宏，将标准 vkXxx() 调用映射到函数表：

```c
#define vkCreateBuffer vk_function_table.vkCreateBuffer
#define vkBeginCommandBuffer vk_function_table.vkBeginCommandBuffer
// ... 数百个函数映射
```

## 使用流程

### 1. 设备初始化

```c
// device_create() 中
if (!vk_loader_init()) {
    blog(LOG_ERROR, "Failed to initialize Vulkan loader");
    return false;
}

VkResult result = vkCreateInstance(&instance_info, NULL, &device->instance);

// 加载实例函数
if (!vk_loader_load_instance_functions(device->instance)) {
    return false;
}

// 创建逻辑设备
result = vkCreateDevice(device->gpu, &device_create_info, NULL, &device->device);

// 加载设备函数
if (!vk_loader_load_device_functions(device->device)) {
    return false;
}
```

### 2. 正常使用

之后所有代码都可以使用标准 Vulkan 函数调用：

```c
// vulkan-texture2d.c  
VkBuffer buffer;
vkCreateBuffer(device->device, &buffer_info, NULL, &buffer);

// vulkan-shader.c
VkShaderModule module;
vkCreateShaderModule(device->device, &shader_info, NULL, &module);
```

宏自动将这些调用重定向到函数表。

### 3. 清理

```c
// device_destroy() 中
vk_loader_free();  // 释放加载器，卸载库
```

## 跨平台支持

### Windows 实现

```c
#define VULKAN_LIB_NAME "vulkan-1.dll"

lib_handle_t handle = LoadLibraryA(VULKAN_LIB_NAME);
void *func = GetProcAddress(handle, "vkCreateInstance");
FreeLibrary(handle);
```

### Linux/Unix 实现

```c
#define VULKAN_LIB_NAME "libvulkan.so.1"

lib_handle_t handle = dlopen(VULKAN_LIB_NAME, RTLD_LAZY);
void *func = dlsym(handle, "vkCreateInstance");
dlclose(handle);
```

## 加载函数概览

### 全局函数（动态库加载后）
| 函数 | 用途 |
|------|------|
| vkCreateInstance | 创建 Vulkan 实例 |
| vkGetInstanceProcAddr | 获取实例函数地址 |
| vkEnumerateInstanceExtensionProperties | 查询实例扩展 |

### 实例函数（创建实例后）
| 函数 | 用途 |
|------|------|
| vkEnumeratePhysicalDevices | 列举物理设备 |
| vkGetPhysicalDeviceProperties | 获取 GPU 属性 |
| vkCreateDevice | 创建逻辑设备 |
| vkCreateWin32SurfaceKHR | 创建 Win32 表面 |

### 设备函数（创建设备后）
| 函数 | 用途 |
|------|------|
| vkCreateBuffer | 创建缓冲区 |
| vkCreateImage | 创建图像 |
| vkCmdDraw | 绘制命令 |
| vkQueueSubmit | 提交命令 |
| vkCreateSwapchainKHR | 创建交换链 |

## 错误处理

### 加载失败场景

1. **Vulkan 库不存在**
   ```c
   if (!vk_loader_init()) {
       blog(LOG_ERROR, "Vulkan not available");
       // 使用其他渲染器或警告用户
   }
   ```

2. **函数不存在**
   ```c
   if (!vk_function_table.vkSomeFunction) {
       blog(LOG_WARNING, "Function not available");
       // 使用备选实现或功能降级
   }
   ```

3. **创建实例失败**
   ```c
   VkResult result = vkCreateInstance(...);
   if (result != VK_SUCCESS) {
       blog(LOG_ERROR, "Failed: %s", vk_get_error_string(result));
   }
   ```

## 编译配置

### CMakeLists.txt 变化

```cmake
# 仅用于头文件，不链接库
find_package(Vulkan REQUIRED)

target_include_directories(
  libobs-vulkan
  PRIVATE ${Vulkan_INCLUDE_DIRS}
)

# 不链接 Vulkan 库
# target_link_libraries(...Vulkan::Vulkan) <- 移除

target_sources(
  libobs-vulkan
  PRIVATE
    vulkan-loader.c      # 新增
    vulkan-loader.h      # 新增
    vulkan-api.h         # 新增
    # ... 其他源文件
)
```

## 性能考虑

1. **初始化开销** - 首次加载库时会有轻微开销（通常 <10ms）
2. **函数调用** - 通过指针调用的开销可忽略不计
3. **缓存友好** - 函数指针表保存在内存中，访问很快

## 兼容性

| 平台 | 库名 | 状态 |
|------|------|------|
| Windows (x64) | vulkan-1.dll | ✅ 实现 |
| Linux | libvulkan.so.1 | ✅ 实现 |
| macOS | libvulkan.1.dylib | ⏳ 待实现 |

## 调试技巧

### 检查函数是否加载

```c
if (!vk_function_table.vkCreateBuffer) {
    blog(LOG_ERROR, "vkCreateBuffer not loaded!");
}
```

### 启用详细日志

编辑 vulkan-loader.c 中的 blog() 调用以获取详细信息：
```c
blog(LOG_DEBUG, "Loading function: %s", function_name);
```

### 验证库加载

```c
bool available = vk_loader_available();
blog(LOG_INFO, "Vulkan available: %s", available ? "yes" : "no");
```

## 常见问题

### Q1: 如何确认 Vulkan 库已安装？
A: 检查系统中是否存在 vulkan-1.dll (Windows) 或 libvulkan.so* (Linux)

### Q2: vk_loader_init() 返回 false 怎么办？
A: 安装 Vulkan SDK 或更新 GPU 驱动程序

### Q3: 某个函数加载失败怎么办？
A: 该函数可能不可用（需要特定扩展或驱动），使用备选实现

### Q4: 能否同时使用多个 Vulkan 版本？
A: 否，使用全局函数表限制了一个库实例

## 参考资源

- Vulkan 规范: https://www.khronos.org/vulkan/
- 动态加载最佳实践: https://vulkan-tutorial.com/
- OBS 设计文档: ../../docs/

---

**最后更新**: 2024年3月
