# Rendering Engine Module

**Location**: `core/src/rendering/`  
**Status**: ✅ Phase 2 Complete  
**Version**: 1.0.0  
**Date**: 2025-12-13

---

## Overview

The **Rendering Engine** provides the core VR180/360 stereo rendering pipeline for Neural Studio. It handles Vulkan-based GPU rendering, multi-profile framebuffer management, STMap fisheye stitching, preview rendering, and optional AI upscaling/HDR processing.

### Key Capabilities
- ✅ Vulkan rendering via Qt 6.9 RHI
- ✅ Stereo rendering (dual-pass L/R eyes with IPD offset)
- ✅ Multi-profile framebuffer management (Quest 3, Index, Vive Pro 2)
- ✅ STMap fisheye stitching (compute shader)
- ✅ Preview rendering (monoscopic/SBS/anaglyph)
- ✅ Optional RTX AI upscaling (4K→8K)
- ✅ Optional HDR processing (Rec.709↔Rec.2020, PQ/HLG, tone mapping)
- ✅ Triple buffering for smooth rendering

---

## Files

### Core Rendering
- **`VulkanRenderer.h/cpp`** - Vulkan initialization and render loop
  - Qt RHI backend setup
  - Swap chain management
  - Command buffer allocation
  - framebuffer lifecycle

- **`StereoRenderer.h/cpp`** - Stereo rendering (L/R eyes)
  - Dual-pass rendering with IPD offset
  - SBS input support (single stream)
  - Dual-stream support (AV2 multi-stream)
  - 3D overlay composition

- **`FramebufferManager.h/cpp`** - Multi-profile framebuffer allocation
  - Per-headset resolution management
  - Triple buffering (CPU write, GPU render, display read)
  - Dynamic profile enable/disable

### Stitching
- **`STMapLoader.h/cpp`** - Load TIFF/PNG UV remap textures
  - Per-lens calibration support
  - Vulkan texture upload

- **`shaders/stitch.comp`** - Fisheye→Equirect stitching shader
  - UV remapping compute shader
  - Bilinear sampling

### Preview
- **`PreviewRenderer.h/cpp`** - Real-time preview for Blueprint/ActiveFrame
  - Low-res preview (1920x1080)
  - Monoscopic/SBS/Anaglyph modes
  - 3D viewport integration

### Optional Processing
- **`RTXUpscaler.h/cpp`** - NVIDIA AI upscaling (4K→8K)
  - CUDA-GL interop (Ubuntu/Linux)
  - NVIDIA Maxine SDK integration
  - Adjustable quality/scale modes

- **`HDRProcessor.h/cpp`** - HDR color management
  - Rec.709 ↔ Rec.2020 conversion
  - PQ/HLG EOTF support
  - ACES/Reinhard/Hable tone mapping

- **`shaders/hdr_tonemap.comp`** - HDR tone mapping shader
  - Color space conversion matrices
  - EOTF functions
  - Tone mapping operators

### Shaders
- **`shaders/stereo.vert`** - Full-screen quad vertex shader
- **`shaders/stereo.frag`** - Stereo compositing fragment shader
- **`shaders/compile_shaders.sh`** - GLSL→SPIR-V compilation script

### Build
- **`CMakeLists.txt`** - Build configuration with Qt6 RHI dependencies

---

## Architecture

### Rendering Pipeline
```
Camera Input → STMap Stitching → Stereo Rendering → [RTX Upscale] → [HDR Process] → Output
                    ↓                   ↓                  ↓               ↓
               stitch.comp      Dual-pass L/R        Optional AI      Optional HDR
                                  + 3D Overlay       (4K→8K)          (tone mapping)
```

### Node-Based Design
All rendering components are designed as **connectable Blueprint nodes**:
- Each module is an **optional processing step**
- Users can enable/disable in Blueprint mode
- Parameters are **adjustable** via node properties

---

## VR Headset Profiles

### Supported Profiles
| Profile | Resolution (per eye) | Framerate | Codec | FOV |
|---------|---------------------|-----------|-------|-----|
| Quest 3 | 1920x1920 | 90 Hz | H.265 | 110° |
| Valve Index | 1440x1600 | 120 Hz | H.265 | 130° |
| Vive Pro 2 | 2448x2448 | 90 Hz | AV1 | 120° |

### Profile Structure (`VRHeadsetProfile.h`)
```cpp
struct VRHeadsetProfile {
    std::string id;           // "quest3"
    std::string name;         // "Meta Quest 3"
    uint32_t eyeWidth;        // 1920
    uint32_t eyeHeight;       // 1920
    double framerate;         // 90.0
    VideoCodec codec;         // H265, AV1
    uint16_t srtPort;         // 9000
    float ipd;                // 0.063 (meters)
    bool enabled;             // true
};
```

---

## Shader Pipeline

### Stitching (`stitch.comp`)
**Input**: Fisheye video (4 cameras)  
**Process**: UV remap via STMap texture  
**Output**: Equirectangular 360° video

```glsl
// Load UV offset from STMap
vec2 uv_offset = texture(stmapTexture, texCoord).rg;
vec2 source_uv = texCoord + uv_offset;

// Sample from fisheye input
vec4 color = texture(inputTexture, source_uv);
imageStore(outputImage, pixel_coords, color);
```

### Stereo Composition (`stereo.frag`)
**Input**: Stitched video + 3D overlay  
**Process**: Layer blending  
**Output**: Composited stereo frame

```glsl
vec4 stitched = texture(stitchedTexture, uv);
vec4 overlay = texture(overlayTexture, uv);
fragColor = mix(stitched, overlay, overlay.a);  // Alpha blend
```

### HDR Tone Mapping (`hdr_tonemap.comp`)
**Input**: HDR image (Rec.2020 PQ)  
**Process**: Tone mapping + color conversion  
**Output**: SDR image (Rec.709 sRGB)

```glsl
// PQ EOTF decode
vec3 linearRec2020 = applyPQEOTF(hdrColor);

// Convert to Rec.709
vec3 linearRec709 = rec2020ToRec709Matrix * linearRec2020;

// Tone mapping (ACES)
vec3 sdrColor = acesToneMap(linearRec709);

// sRGB EOTF encode
fragColor = applySRGBEOTF(sdrColor);
```

---

## Triple Buffering

**Purpose**: Smooth rendering without stalls

```
Frame N:   CPU writes → GPU renders → Display reads
Frame N+1:            CPU writes     → GPU renders → Display reads
Frame N+2:                          CPU writes     → GPU renders
```

**Implementation** (`FramebufferManager.cpp`):
```cpp
struct FramebufferSet {
    QRhiTexture* buffers[3];  // Triple buffering
    uint32_t currentBuffer = 0;
};

void swapBuffers(FramebufferSet& set) {
    set.currentBuffer = (set.currentBuffer + 1) % 3;
}
```

---

## RTX Upscaling

### CUDA-GL Interop Workflow
1. **Register** GL texture with CUDA (`cudaGraphicsGLRegisterImage`)
2. **Map** to CUDA array (`cudaGraphicsMapResources`)
3. **Process** with NVVFX (`NvVFX_Run`)
4. **Unmap** from CUDA (`cudaGraphicsUnmapResources`)
5. **Unregister** on cleanup

### Quality Modes
- **Performance**: Fast, lower quality (1.33x scale)
- **HighQuality**: Slower, higher quality (2x+ scale)

---

## HDR Processing

### Color Spaces
- **Rec.709**: Standard HD (sRGB, BT.709 primaries)
- **Rec.2020**: Wide color gamut (UHD, BT.2020 primaries)

### EOTF (Electro-Optical Transfer Function)
- **Linear**: No gamma correction
- **sRGB**: ~2.2 gamma
- **PQ (ST 2084)**: Perceptual quantizer for HDR
- **HLG (Hybrid Log-Gamma)**: Backward-compatible HDR

### Tone Mapping Operators
- **Passthrough**: No tone mapping
- **ACES**: Film-like response
- **Reinhard**: Simple, smooth
- **Hable (Uncharted 2)**: Game-tested curve

---

## Usage

### Basic Rendering
```cpp
#include "rendering/VulkanRenderer.h"
#include "rendering/StereoRenderer.h"

// Initialize Vulkan
VulkanRenderer renderer;
renderer.initialize(window);

// Setup stereo rendering
StereoRenderer stereo(&renderer);
stereo.initialize(1920, 1920, StereoRenderer::InputMode::SideBySide);

// Render loop
while (running) {
    renderer.beginFrame();
    stereo.renderFrame(leftEyeTexture, rightEyeTexture);
    renderer.endFrame();
}
```

### Optional Processing
```cpp
// Add RTX upscaling (optional)
RTXUpscaler upscaler(&renderer);
upscaler.initialize(1920, 1920, 3840, 3840,  // 2x upscale
                   RTXUpscaler::QualityMode::HighQuality,
                   RTXUpscaler::ScaleFactor::Scale2x);
upscaler.upscale(inputTexture, outputTexture);

// Add HDR processing (optional)
HDRProcessor hdr(&renderer);
HDRProcessor::HDRConfig config;
config.inputColorSpace = HDRProcessor::ColorSpace::Rec2020;
config.inputEOTF = HDRProcessor::EOTF::PQ;
config.toneMapMode = HDRProcessor::ToneMapMode::ACES;
hdr.setConfig(config);
hdr.process(inputTexture, outputTexture);
```

---

## Dependencies

### External
- Qt 6.9 (RHI, Core)
- Vulkan SDK
- CUDA Toolkit (for RTX upscaling)
- NVIDIA Video Effects SDK (Maxine) - optional

### Internal
- `core/lib/vr/` - VR headset profiles
- `core/src/scene-graph/` - Node execution (future)

---

## Integration Points

### Current
- **VirtualCamManager**: Uses rendering for per-profile output
- **FrameRouter**: Processing pipeline (will integrate with NodeExecutionGraph)

### Planned
- **NodeExecutionGraph**: Rendering nodes as Blueprint components
- **SceneManager**: 3D scene rendering integration
- **Video Capture**: Camera input integration

---

## Performance

| Component | Resolution | FPS | Notes |
|-----------|-----------|-----|-------|
| Stitching | 4K | 60+ | GPU compute shader |
| Stereo Rendering | 4K per eye | 90-120 | Dual-pass |
| RTX Upscaling | 4K→8K | 30-60 | NVIDIA GPU required |
| HDR Processing | 4K | 60+ | GPU compute shader |

---

## Future Enhancements

- [ ] Foveated rendering (eye-tracking optimization)
- [ ] Temporal anti-aliasing (TAA)
- [ ] Dynamic resolution scaling
- [ ] Multi-GPU support (AFR/SFR)
- [ ] VK_KHR_timeline_semaphore for better sync

---

## Documentation

- **API Reference**: See header files (`VulkanRenderer.h`, etc.)
- **Shaders**: See `shaders/` directory
- **Architecture**: [TODO.md](file:///home/subtomic/Documents/GitHub/Neural-Studio/docs/architecture/TODO.md)

---

**Maintainer**: Neural Studio Team  
**Last Updated**: 2025-12-13
