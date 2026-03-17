# Vulkan 着色器编译系统

## 概述

libobs-vulkan 集成了完整的 GLSL → SPIR-V 着色器编译系统，支持：

- **运行时编译** - 在应用启动时编译 GLSL 源代码
- **预编译缓存** - 将编译结果保存为 SPIR-V，加快加载速度
- **着色器管理** - 自动缓存和引用计数管理
- **多阶段编译** - 支持所有 Vulkan 着色器阶段

## 架构

```
GLSL 源代码
    ↓
glslang 库
    ↓
SPIR-V 字节码
    ↓
Vulkan ShaderModule
    ↓
图形管道/计算管道
```

## 核心组件

### vulkan-shader-compiler.h/c
**功能**: GLSL → SPIR-V 编译的低级 API

**主要函数**:
- `vk_shader_compile_glsl()` - 从源代码编译
- `vk_shader_compile_glsl_file()` - 从文件编译
- `vk_shader_module_create_from_spirv()` - 创建 ShaderModule
- `vk_shader_module_create_from_glsl()` - 一步编译和创建
- `vk_shader_save_spirv()` - 保存 SPIR-V 字节码
- `vk_shader_load_spirv()` - 加载预编译的 SPIR-V

### vulkan-shader-compiler-glslang.cpp
**功能**: glslang 库的 C++ 包装

**实现内容**:
- glslang 初始化/清理
- GLSL 预处理、解析、链接
- SPIRV 代码生成
- 错误报告

### vulkan-shader-manager.h/c
**功能**: 着色器生命周期管理（高级 API）

**主要函数**:
- `vk_shader_manager_create()` - 创建管理器
- `vk_shader_manager_destroy()` - 清理管理器
- `vk_shader_manager_load_glsl()` - 加载 GLSL 源代码
- `vk_shader_manager_load_file()` - 加载着色器文件
- `vk_shader_manager_load_spirv()` - 加载预编译 SPIR-V
- `vk_shader_manager_release()` - 释放着色器（引用计数）
- `vk_shader_precompile_directory()` - 批量预编译

## 使用示例

### 1. 初始化和管理器创建

```c
// 在 device_create() 中
vk_shader_manager_t *shader_manager = vk_shader_manager_create(device->device);
device->shader_manager = shader_manager;
```

### 2. 运行时编译着色器

```c
#include "vulkan-shader-compiler.h"

const char *glsl_source = R"(
    #version 450
    layout(location = 0) in vec3 pos;
    void main() {
        gl_Position = vec4(pos, 1.0);
    }
)";

vk_shader_compile_options_t options = {
    .stage = VK_SHADER_STAGE_VERTEX_BIT,
    .entry_point = "main",
    .optimize = true,
};

vk_shader_compile_result_t result = vk_shader_compile_glsl(glsl_source, 0, &options);
if (result.success) {
    VkShaderModule vertex_shader;
    vk_shader_module_create_from_spirv(device, result.bytecode, result.bytecode_size, &vertex_shader);
    vk_shader_compile_result_free(&result);
} else {
    blog(LOG_ERROR, "Compilation failed: %s", result.error_message);
}
```

### 3. 使用着色器管理器

```c
// 从文件加载（自动检测阶段）
VkShaderModule vert_shader = vk_shader_manager_load_file(
    shader_manager, "main_vert", "shaders/mesh.vert"
);

VkShaderModule frag_shader = vk_shader_manager_load_file(
    shader_manager, "main_frag", "shaders/mesh.frag"
);

// 从源代码直接加载
VkShaderModule compute_shader = vk_shader_manager_load_glsl(
    shader_manager, "compute_blur", compute_source, VK_SHADER_STAGE_COMPUTE_BIT
);

// 查询缓存的着色器
VkShaderModule cached = vk_shader_manager_get(shader_manager, "main_vert");

// 释放引用（引用计数为 0 时自动卸载）
vk_shader_manager_release(shader_manager, "main_vert");
```

### 4. 预编译编译着色器

```c
// 离线编译所有着色器到 SPIR-V
bool success = vk_shader_precompile_directory(
    "data/shaders",   // GLSL 源文件目录
    "data/shaders/spirv"  // 输出 SPIR-V 目录
);

// 运行时加载预编译的着色器（更快）
VkShaderModule shader = vk_shader_manager_load_spirv(
    shader_manager, "main_vert", "data/shaders/spirv/mesh.vert.spv", 
    VK_SHADER_STAGE_VERTEX_BIT
);
```

### 5. 清理

```c
// 在 device_destroy() 中
vk_shader_manager_destroy(shader_manager);
```

## 着色器文件命名约定

编译系统根据文件扩展名自动检测着色器阶段：

| 扩展名 | 阶段 |
|--------|------|
| .vert  | VK_SHADER_STAGE_VERTEX_BIT |
| .frag  | VK_SHADER_STAGE_FRAGMENT_BIT |
| .comp  | VK_SHADER_STAGE_COMPUTE_BIT |
| .geom  | VK_SHADER_STAGE_GEOMETRY_BIT |
| .tesc  | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT |
| .tese  | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT |

## 编译集成

### CMakeLists.txt 配置

```cmake
find_package(glslang REQUIRED)

target_sources(libobs-vulkan PRIVATE
    vulkan-shader-compiler.h
    vulkan-shader-compiler.c
    vulkan-shader-compiler-glslang.cpp
    vulkan-shader-manager.h
    vulkan-shader-manager.c
)

target_link_libraries(libobs-vulkan PRIVATE
    glslang::glslang
    glslang::SPIRV
)

target_compile_definitions(libobs-vulkan PRIVATE VULKAN_SHADER_COMPILER_ENABLED)
```

### 可选支持

如果系统上没有 glslang，编译系统会自动禁用：
- `ENABLE_VULKAN_SHADER_COMPILER` 标志控制条件编译
- 应用仍可运行，但只能使用预编译的 SPIR-V 着色器
- 运行时编译功能会返回错误

## 性能考虑

| 操作 | 开销 | 备注 |
|------|------|------|
| 首次运行时编译 | 50-500ms | 取决于着色器复杂度 |
| 加载预编译 SPIR-V | 1-10ms | 推荐用于生产 |
| ShaderModule 创建 | 1-5ms | 每个着色器 |
| 缓存查询 | <1μs | 哈希查找 |

### 优化建议

1. **生产环境**: 预编译所有着色器，在运行时加载 SPIR-V
2. **开发环境**: 使用运行时编译以快速测试迭代
3. **启动时间**: 使用线程池并行编译多个着色器
4. **内存**: 着色器缓存自动管理，只需正确调用 release()

## GLSL 兼容性

支持的 GLSL 特性：
- ✅ GLSL 4.50（Vulkan 标准）
- ✅ 所有着色器阶段
- ✅ 布局限定符（layout）
- ✅ 推送常量（push constants）
- ✅ 描述符集（descriptor sets）
- ✅ 扩展指令（#extension）

示例：

```glsl
#version 450
#extension GL_ARB_separate_shader_objects : enable

// 统一缓冲
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

// 采样器
layout(binding = 1) uniform sampler2D texSampler;

// 推送常量
layout(push_constant) uniform PushConstants {
    float time;
} pc;

// 输入/输出
layout(location = 0) in vec3 inPosition;
layout(location = 0) out vec4 outColor;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    outColor = texture(texSampler, inPosition.xy) + vec4(pc.time);
}
```

## 调试

### 启用详细日志

在 vulkan-shader-compiler.c 中编辑日志级别：

```c
blog(LOG_DEBUG, "Compiling shader...");  // 详细编译过程
```

### 验证 SPIR-V

使用 Vulkan SDK 工具验证生成的字节码：

```bash
spirv-val shader.spv
spirv-dis shader.spv  # 反汇编查看
```

### 常见错误

```
error: Cannot determine shader stage from filename
→ 检查文件扩展名是否正确

error: glslang library not available
→ 安装 glslang 或使用预编译的 SPIR-V

error: vkCreateShaderModule failed
→ SPIR-V 字节码可能损坏，使用 spirv-val 验证
```

## 扩展支持

### 添加新的着色器阶段

编辑 vulkan-shader-compiler.c：

```c
// 1. 在 glslang_compile() 中添加阶段映射
if (esh_stage == EShLangRayGen) {
    // ... 处理光线追踪
}

// 2. 在 stage_to_glslang_stage() 中添加
case VK_SHADER_STAGE_RAYGEN_BIT_KHR:
    return GLSLANG_STAGE_RAYGEN;
```

### 使用高级编译选项

传递 glslang 编译标志：

```c
vk_shader_compile_options_t options = {
    .stage = VK_SHADER_STAGE_VERTEX_BIT,
    .entry_point = "main",
    .optimize = true,  // 启用优化
};
```

## 许可证

遵循 OBS Studio GNU GPL v2+ 许可证

## 参考资源

- glslang 文档: https://github.com/KhronosGroup/glslang
- GLSL 规范: https://www.opengl.org/registry/doc/GLSLangSpec.4.60.pdf
- Vulkan SPIR-V 环境: https://www.ux.uis.no/~carlos/N5291/glsl-450-core-profile-reference-cardv1-2.pdf

---

**文档版本**: 1.0
**更新日期**: 2024年3月
