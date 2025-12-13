# Texture Manager

**Location**: `ui/shared_ui/managers/TextureManager/`
**Status**: ✅ Implemented
**Version**: 1.0.0
**Date**: 2025-12-14

---

## Overview

The **Texture Manager** manages 2D image assets and HDR environment maps in Neural Studio. It provides texture discovery, format detection, and enables material texturing, skyboxes, and image-based lighting.

### Key Features
- **Format Support**: PNG, JPG, TGA, EXR, HDR, DDS
- **HDR Support**: HDR10, OpenEXR for environment lighting
- **Mipmap Generation**: Automatic LOD creation
- **Cubemap Support**: Skybox textures
- **Compression**: BC, ASTC, ETC2 support
- **Preset Management**: Save material texture sets

---

## Architecture

### Backend (`TextureManagerController.h/cpp`)
- **Inheritance**: `BaseManagerController` → `QObject`
- **Responsibilities**:
  - Scanning `/assets/textures/` directory
  - Texture metadata extraction (resolution, format, channels)
  - Thumbnail generation
  - Tracking texture usage in materials
  - Preset management

### Frontend Options

#### QML View (`TextureManager.qml`)
- **Purpose**: Blueprint Frame texture library
- **Features**:
  - Grid view with texture previews
  - Format/resolution badges
  - HDR indicators
  - Drag-to-apply to materials
  - "In Use" indicators

#### Qt Widget (`TextureManagerWidget.h/cpp`)
- **Purpose**: Active Frame texture browser
- **Features**:
  - Categorized texture library
  - Preview with alpha channel
  - Metadata inspector
  - Import texture button

---

## Interface

### Controller Properties
| Property | Type | Description |
|----------|------|-------------|
| `textureFiles` | QVariantList | Discovered textures with metadata |
| `managedNodes` | QStringList | Materials using textures |
| `nodePresets` | QVariantList | Saved material configs |

### Methods
| Method | Parameters | Description |
|--------|------------|-------------|
| `scanTextures()` | - | Rescan textures directory |
| `getTextureMetadata()` | texturePath | Extract image info |
| `generateMipmaps()` | texturePath | Create LOD chain |
| `savePreset()` | nodeId, presetName | Save material preset |
| `loadPreset()` | presetName, x, y | Apply texture set |

---

## Supported Formats

### Standard Images
- **PNG** (.png) - Lossless with alpha
- **JPEG** (.jpg, .jpeg) - Lossy compression
- **TGA** (.tga) - Uncompressed or RLE
- **BMP** (.bmp) - Windows bitmap

### HDR/High-Bit-Depth
- **OpenEXR** (.exr) - Film/VFX standard
- **Radiance HDR** (.hdr) - Environment maps
- **TIFF** (.tif, .tiff) - 16-bit support

### Compressed
- **DDS** (.dds) - DirectX texture
- **KTX** (.ktx, .ktx2) - Khronos texture
- **ASTC** - ARM texture compression
- **BC1-7** - Block compression

---

## Texture Types

### PBR Material Maps
- **Albedo/Base Color** - Surface color
- **Normal Map** - Surface detail
- **Roughness** - Surface smoothness
- **Metallic** - Metal vs dielectric
- **Ambient Occlusion** - Cavity shadows
- **Height/Displacement** - Geometric detail

### Environment Maps
- **Equirectangular** - 360° panorama
- **Cubemap** - 6 faces
- **Spherical** - Single sphere map
- **HDR IBL** - Image-based lighting

---

## Usage Example

### C++ (Controller)
```cpp
auto textureManager = new TextureManagerController(parent);
textureManager->scanTextures();

// Get texture metadata
QVariantMap metadata = textureManager->getTextureMetadata(
    "/assets/textures/pbr/metal_albedo.png"
);
// Returns: {"resolution": "2048x2048", "format": "RGBA8", "hdr": false}

// Generate mipmaps
textureManager->generateMipmaps("/assets/textures/skybox.hdr");
```

### QML (Blueprint UI)
```qml
TextureManager {
    controller: textureManagerController
    
    GridView {
        model: controller.textureFiles
        delegate: TextureCard {
            thumbnail: modelData.thumbnailPath
            textureName: modelData.name
            resolution: modelData.resolution
            isHDR: modelData.hdr
            channels: modelData.channels  // "RGB", "RGBA", "R"
            inUse: controller.managedNodes.includes(modelData.id)
        }
    }
}
```

---

## File Organization

```
/assets/textures/
├── pbr/             # PBR material maps
├── environment/     # HDR skyboxes
├── ui/              # Interface textures
├── decals/          # Overlay textures
├── particles/       # Particle sprites
└── video/           # Video frame textures

Presets:
~/.config/neural-studio/material_presets.json

Thumbnail cache:
~/.cache/neural-studio/texture_thumbs/
```

---

## Integration Points

### With Material System
- PBR texture loading
- Material preset application
- Real-time updates

### With 3D Renderer
- Vulkan texture upload
- Mipmap generation
- Anisotropic filtering

### With VR Pipeline
- Environment lighting
- Spherical harmonics for IBL
- Reflection probes

---

## Future Enhancements
- [ ] Normal map generation from height
- [ ] Texture atlas packing
- [ ] Seamless texture tiling
- [ ] AI upscaling integration
- [ ] Substance material import
- [ ] Texture streaming for large assets
