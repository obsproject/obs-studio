# Shader Manager

**Location**: `ui/shared_ui/managers/ShaderManager/`
**Status**: ✅ Implemented
**Version**: 1.0.0
**Date**: 2025-12-14

---

## Overview

The **Shader Manager** manages GLSL shaders for custom rendering effects in Neural Studio. It provides shader discovery, compilation validation, and enables users to create custom visual effects and compute kernels.

### Key Features
- **GLSL Support**: Vertex, Fragment, Geometry, Compute shaders
- **Vulkan SPIR-V**: Compiled shader bytecode
- **Compilation Validation**: Real-time error checking
- **Uniform Inspection**: Auto-detect shader parameters
- **Hot Reload**: Live shader updates during editing
- **Preset Management**: Save shader configurations

---

## Architecture

### Backend (`ShaderManagerController.h/cpp`)
- **Inheritance**: `BaseManagerController` → `QObject`
- **Responsibilities**:
  - Scanning `/assets/shaders/` directory
  - Shader compilation via glslang/shaderc
  - Uniform extraction and validation
  - Tracking shader usage in effect nodes
  - Preset management

### Frontend Options

#### QML View (`ShaderManager.qml`)
- **Purpose**: Blueprint Frame shader library
- **Features**:
  - Shader list with type badges (vert, frag, comp)
  - Live preview thumbnails
  - Compilation status indicators
  - Drag-to-create shader nodes
  - "In Use" indicators

#### Qt Widget (`ShaderManagerWidget.h/cpp`)
- **Purpose**: Active Frame shader browser
- **Features**:
  - Shader editor with syntax highlighting
  - Compile button with error output
  - Uniform inspector
  - Import shader button

---

## Interface

### Controller Properties
| Property | Type | Description |
|----------|------|-------------|
| `shaderFiles` | QVariantList | Discovered shaders with metadata |
| `managedNodes` | QStringList | Nodes using shaders |
| `nodePresets` | QVariantList | Saved shader configs |

### Methods
| Method | Parameters | Description |
|--------|------------|-------------|
| `scanShaders()` | - | Rescan shaders directory |
| `compileShader()` | shaderPath | Validate GLSL code |
| `getUniforms()` | shaderPath | Extract shader parameters |
| `createShaderNode()` | shaderPath, x, y | Create shader effect node |
| `savePreset()` | nodeId, presetName | Save shader config |

---

## Supported Shader Types

### Graphics Pipeline
- **Vertex Shaders** (.vert) - Vertex transformation
- **Fragment Shaders** (.frag) - Pixel/color output
- **Geometry Shaders** (.geom) - Geometry generation
- **Tessellation Shaders** (.tesc, .tese) - Surface subdivision

### Compute Pipeline
- **Compute Shaders** (.comp) - GPU parallel processing
  - Image filters
  - Particle simulation
  - AI inference
  - Video effects

---

## Usage Example

### C++ (Controller)
```cpp
auto shaderManager = new ShaderManagerController(parent);
shaderManager->scanShaders();

// Compile and validate
bool success = shaderManager->compileShader("/assets/shaders/bloom.frag");

// Get shader uniforms
QVariantList uniforms = shaderManager->getUniforms("/assets/shaders/bloom.frag");
// Returns: [{"name": "threshold", "type": "float"}, ...]

// Create shader node
shaderManager->createShaderNode("/assets/shaders/bloom.frag", 300, 100);
```

### QML (Blueprint UI)
```qml
ShaderManager {
    controller: shaderManagerController
    
    ListView {
        model: controller.shaderFiles
        delegate: ShaderCard {
            shaderName: modelData.name
            shaderType: modelData.type  // "vert", "frag", "comp"
            compilationStatus: modelData.valid ? "✓" : "✗"
            uniforms: modelData.uniformCount
            inUse: controller.managedNodes.includes(modelData.id)
        }
    }
}
```

---

## Shader Compilation

### Validation Pipeline
1. **Parse** - glslang syntax check
2. **Optimize** - SPIR-V optimizer
3. **Reflection** - Extract uniforms/inputs/outputs
4. **Cache** - Store compiled SPIR-V

### Error Reporting
```
shader.frag:15:20: error: undeclared identifier 'uTexture'
    vec4 color = texture(uTexture, vTexCoord);
                          ^
```

---

## Uniform Inspection

Automatically detected shader parameters:
```glsl
// Fragment shader example
uniform sampler2D uInputTexture;
uniform float uBlurAmount;
uniform vec2 uResolution;
uniform mat4 uModelMatrix;
```

Extracted as:
```cpp
[
  {"name": "uInputTexture", "type": "sampler2D", "location": 0},
  {"name": "uBlurAmount", "type": "float", "location": 1},
  {"name": "uResolution", "type": "vec2", "location": 2},
  {"name": "uModelMatrix", "type": "mat4", "location": 3}
]
```

---

## File Organization

```
/assets/shaders/
├── vertex/          # Vertex shaders
├── fragment/        # Fragment shaders
├── compute/         # Compute shaders
├── post/            # Post-processing
├── particles/       # Particle effects
└── vr/              # VR-specific (distortion, etc.)

Compiled cache:
~/.cache/neural-studio/spirv/

Presets:
~/.config/neural-studio/ShaderNode_presets.json
```

---

## Built-in Shaders

### Post-Processing
- Bloom
- Chromatic Aberration
- Depth of Field
- Motion Blur
- Color Grading (LUT)

### VR-Specific
- Lens Distortion Correction
- Chromatic Aberration Correction
- Timewarp
- Foveated Rendering

### Compute
- Gaussian Blur
- Edge Detection
- Background Removal
- Upscaling (FSR, DLSS-like)

---

## Integration Points

### With Vulkan Renderer
- SPIR-V shader loading
- Descriptor set management
- Push constant updates

### With Effect Nodes
- Shader parameter binding
- Texture input/output
- Real-time updates

---

## Future Enhancements
- [ ] Shader graph visual editor
- [ ] ShaderToy import
- [ ] Shader profiling/debugging
- [ ] Automatic uniform UI generation
- [ ] Shader library (community shaders)
- [ ] Real-time collaborative shader editing
