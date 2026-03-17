# libobs-vulkan - Vulkan Renderer Module for OBS Studio

## 概述

这是一个为 OBS Studio 创建的 Vulkan 渲染后端模块，仿照 libobs-opengl 的架构用 Vulkan API 来实现。

## 项目结构

```
libobs-vulkan/
├── CMakeLists.txt              # 编译配置
├── vulkan-subsystem.h          # 核心数据结构和接口定义
├── vulkan-subsystem.c          # 设备初始化和销毁，基础 Vulkan 操作
├── vulkan-helpers.h/.c         # Vulkan 帮助函数
├── vulkan-texture2d.c          # 2D 纹理实现
├── vulkan-texture3d.c          # 3D 纹理实现
├── vulkan-texturecube.c        # 立方体纹理实现
├── vulkan-shader.c             # 着色器编译和管理
├── vulkan-vertexbuffer.c       # 顶点缓冲区实现
├── vulkan-indexbuffer.c        # 索引缓冲区实现
├── vulkan-stagesurf.c          # 暂存表面和 Z-Stencil 缓冲区
├── vulkan-zstencil.c           # Z-Stencil 缓冲区扩展
├── vulkan-windows.c            # Windows 平台特定代码
└── cmake/
    └── windows/
        └── obs-module.rc.in    # 资源文件
```

## 主要组件

### 1. 设备管理 (vulkan-subsystem.c)
- `device_create()` - 创建 Vulkan 设备和逻辑设备
- `device_destroy()` - 销毁设备资源
- `device_enter_context()` / `device_leave_context()` - 上下文管理
- 内存、队列、命令池的初始化

### 2. 纹理系统
- **2D 纹理** (`vulkan-texture2d.c`)：支持各种颜色格式，支持 mipmapping
- **3D 纹理** (`vulkan-texture3d.c`)：体积纹理支持
- **立方体纹理** (`vulkan-texturecube.c`)：六面体纹理用于环境贴图

### 3. 着色器系统 (vulkan-shader.c)
- 支持顶点着色器和像素着色器
- 使用 SPIR-V 作为中间格式
- 描述集管理

### 4. 缓冲区系统
- **顶点缓冲区** (`vulkan-vertexbuffer.c`)：存储顶点数据
- **索引缓冲区** (`vulkan-indexbuffer.c`)：支持 U16 和 U32 索引
- **暂存表面** (`vulkan-stagesurf.c`)：用于纹理读回

### 5. 辅助函数 (vulkan-helpers.h/.c)
- 内存分配和缓冲区管理
- 命令缓冲区操作
- 图像布局转换
- 错误处理和字符串转换

### 6. 平台特定代码 (vulkan-windows.c)
- Win32 表面创建
- 交换链初始化
- 窗口管理

## 格式转换

模块提供了从 OBS 的 `gs_color_format` 到 Vulkan `VkFormat` 的完整映射：

```c
GS_RGBA        → VK_FORMAT_R8G8B8A8_SRGB
GS_BGRA        → VK_FORMAT_B8G8R8A8_SRGB
GS_RGBA16F     → VK_FORMAT_R16G16B16A16_SFLOAT
GS_RGBA32F     → VK_FORMAT_R32G32B32A32_SFLOAT
// ... 更多格式映射
```

## 当前实现状态

### ✅ 已实现
- 基础 Vulkan 实例和设备创建
- 物理设备选择和队列管理
- 命令池和命令缓冲区
- 内存分配和缓冲区操作
- 图像创建和视图管理
- 采样器状态创建
- 纹理资源创建（2D、3D、立方体）
- 暂存表面and Z-Stencil 缓冲区
- Windows 表面和交换链

### ⏳ 需要实现
- ⚠️ GLSL 到 SPIR-V 的着色器编译（需要集成 shaderc 或 glslang）
- ⚠️ 图形管道创建
- ⚠️ 描述集和布局
- ⚠️ 渲染通道和帧缓冲区
- ⚠️ 绘制命令记录
- ⚠️ 同步原语（信号量、栅栏）
- ⚠️ 纹理复制操作
- ⚠️ 混合和深度测试状态
- ⚠️ 定时器查询实现

## 编译指令

### 先决条件
1. Vulkan SDK（包含 vulkan.h 和库）
2. CMake 3.28+
3. OBS Studio 源代码

### 构建步骤

```bash
# 在 OBS Studio 主项目中
mkdir build_x64
cd build_x64

# 配置（启用 Vulkan 渲染器）
cmake -G "Visual Studio 17 2022" -A x64 \
      -DENABLE_VULKAN=ON \
      ..

# 构建
cmake --build . --config Release
```

## 使用示例

```c
// 创建设备
gs_device_t *device = NULL;
device_create(&device, 0);

// 创建纹理
gs_texture_t *tex = device_texture_create(
    device, 1024, 768, GS_RGBA, 1, NULL, GS_RENDER_TARGET
);

// 创建采样器
struct gs_sampler_info sampler_info = {
    .filter = GS_FILTER_LINEAR,
    .address_u = GS_ADDRESS_WRAP,
    .address_v = GS_ADDRESS_WRAP,
    .max_anisotropy = 16
};
gs_samplerstate_t *sampler = device_samplerstate_create(device, &sampler_info);

// 加载纹理到管道
device_load_texture(device, tex, 0);
device_load_samplerstate(device, sampler, 0);

// 清理
device_destroy(device);
```

## 关键 API 映射

| OBS 函数 | 对应 Vulkan 操作 |
|---------|-----------------|
| `device_texture_create` | `vkCreateImage` + `vkCreateImageView` |
| `device_load_texture` | 更新描述集 |
| `device_draw` | `vkCmdDraw` / `vkCmdDrawIndexed` |
| `device_clear` | `vkCmdClearColorImage` |
| `device_copy_texture` | `vkCmdCopyImage` |
| `device_present` | `vkQueuePresentKHR` |

## 中间数据结构

### 核心结构体
- `struct gs_device` - Vulkan 设备及其关联资源
- `struct gs_texture` - 基础纹理对象
- `struct gs_texture_2d/3d/cube` - 特定纹理类型
- `struct gs_sampler_state` - 采样器配置
- `struct gs_shader` - 着色器模块
- `struct gs_program` - 完整的图形管道
- `struct gs_vertex_buffer` / `struct gs_index_buffer` - 缓冲区对象

## 错误处理

模块使用 `vk_check()` 和 `vk_check_ret()` 宏来处理 Vulkan 错误：

```c
VkResult result = vkCreateBuffer(...);
vk_check(result);  // 自动记录错误并返回 false

// 或者返回特定值
vk_check_ret(result, NULL);  // 错误时返回 NULL
```

## 编码约定

- 所有函数以 `gs_` 或 `vk_` 前缀开头
- 错误通过 OBS 的 `blog()` 函数记录
- 内存通过 `bmalloc()` 和 `bfree()` 分配
- 所有外部资源必须显式创建和销毁

## 扩展和优化

### 性能优化机会
1. 实现缓冲区缓存机制
2. 批量渲染调用
3. 动态纹理别名
4. 圆形缓冲区实现

### 功能完善
1. 集成 HDR 支持
2. 变长格式压缩（BC、ASTC）
3. 多线程命令记录
4. 高级同步机制

## 参考资源

- [Vulkan 官方文档](https://www.khronos.org/vulkan/)
- [Vulkan 教程](https://vulkan-tutorial.com/)
- [OBS 项目结构](https://github.com/obsproject/obs-studio)
- [OpenGL 模块源代码](../libobs-opengl/)

## 许可证

GNU General Public License v2.0 或更高版本

## 贡献

欢迎通过以下方式贡献：
1. 实现缺失的功能
2. 优化现有代码
3. 添加平台支持（macOS、Linux）
4. 改进文档

---

**最后更新**: 2024年3月
**维护者**: OBS Project
