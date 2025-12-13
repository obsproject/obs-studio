# ThreeDAssets Manager

**Location**: `ui/shared_ui/managers/ThreeDAssetsManager/`
**Status**: ✅ Implemented
**Version**: 1.0.0
**Date**: 2025-12-14

---

## Overview

The **ThreeDAssets Manager** manages 3D models, scenes, and volumetric assets for Neural Studio's spatial rendering pipeline. It supports industry-standard formats and enables VR-native 3D compositing.

### Key Features
- **Format Support**: GLTF, GLB, FBX, OBJ, USD, Alembic
- **Animation Support**: Skeletal, morph targets, transform animation
- **Material PBR**: Metallic-roughness workflow
- **Level of Detail (LOD)**: Multi-resolution models
- **Volumetric Support**: Point clouds, voxels (planned)
- **Multi-Node Tracking**: Track all 3D asset usage

---

## Architecture

### Backend (`ThreeDAssetsManagerController.h/cpp`)
- **Inheritance**: `BaseManagerController` → `QObject`
- **Responsibilities**:
  - Scanning `/assets/3d/` directory
  - 3D model metadata extraction (vertices, materials, animations)
  - Thumbnail rendering (3D preview)
  - Tracking all 3DModelNode instances
  - Preset management

### Frontend Options

#### QML View (`ThreeDAssetsManager.qml`)
- **Purpose**: Blueprint Frame 3D library
- **Features**:
  - 3D model cards with turntable previews
  - Metadata display (poly count, materials, animations)
  - LOD indicators
  - Drag-to-add to 3D scene
  - "In Use" indicators

#### Qt Widget (`ThreeDAssetsManagerWidget.h/cpp`)
- **Purpose**: Active Frame 3D browser
- **Features**:
  - Categorized model library
  - 3D preview widget (Qt Quick 3D)
  - Metadata inspector
  - Import model button

---

## Interface

### Controller Properties
| Property | Type | Description |
|----------|------|-------------|
| `assetFiles` | QVariantList | Discovered 3D assets with metadata |
| `managedNodes` | QStringList | Active 3DModelNode IDs |
| `nodePresets` | QVariantList | Saved model configs |

### Methods
| Method | Parameters | Description |
|--------|------------|-------------|
| `scanAssets()` | - | Rescan 3D assets directory |
| `getAssetMetadata()` | assetPath | Extract model info |
| `generateThumbnail()` | assetPath | Render preview |
| `create3DModelNode()` | assetPath, x, y | Create model node |
| `savePreset()` | nodeId, presetName | Save model config |
| `loadPreset()` | presetName, x, y | Load preset |

---

## Supported Formats

### Modern Formats
- **GLTF 2.0** (.gltf, .glb)
  - PBR materials
  - Animations
  - Morph targets
  - Industry standard, VR-optimized

- **USD** (.usd, .usda, .usdc)
  - Pixar Universal Scene Description
  - Scene graphs
  - Animation layers
  - Film/VFX workflows

### Legacy Formats
- **FBX** (.fbx) - Autodesk exchange format
- **OBJ** (.obj) - Simple geometry, no animation
- **Collada (DAE)** (.dae) - Open standard
- **3DS** (.3ds) - Legacy 3D Studio

### Volumetric/Advanced
- **Alembic** (.abc) - Cached animation data
- **Point Clouds** (.ply, .pcd) - Photogrammetry
- **Voxels** (.vox, .qb) - MagicaVoxel (planned)

---

## Model Categories

### Characters
- Rigged characters
- Facial blend shapes
- Skeletal animations
- VRM avatars

### Props
- Furniture
- Equipment
- Decorative objects
- Interactive items

### Environments
- Rooms/stages
- Outdoor scenes
- Skyboxes (mesh-based)
- Architectural models

### VR-Specific
- Hand models
- Controller models
- Headset representations
- Teleport markers

---

## Usage Example

### C++ (Controller)
```cpp
auto threeDManager = new ThreeDAssetsManagerController(parent);
threeDManager->scanAssets();

// Get model metadata
QVariantMap metadata = threeDManager->getAssetMetadata(
    "/assets/3d/characters/avatar.glb"
);
// Returns: {
//   "vertices": 15432,
//   "triangles": 18920,
//   "materials": 3,
//   "animations": ["idle", "walk", "run"],
//   "hasSkeleton": true
// }

// Create 3D model node
threeDManager->create3DModelNode(
    "/assets/3d/props/desk.glb", 
    200, 300
);

// Save configuration
threeDManager->savePreset("model_avatar_001", "Default Avatar Pose");
```

### QML (Blueprint UI)
```qml
ThreeDAssetsManager {
    controller: threeDAssetsManagerController
    
    GridView {
        model: controller.assetFiles
        delegate: ModelCard {
            thumbnail3D: modelData.thumbnailPath
            modelName: modelData.name
            polyCount: modelData.triangles
            animationCount: modelData.animations.length
            hasLOD: modelData.lodLevels > 1
            inUse: controller.managedNodes.includes(modelData.id)
            
            // 3D turntable preview on hover
            Model {
                source: modelData.path
                autoRotate: true
            }
        }
    }
}
```

---

## Metadata Extraction

```cpp
QVariantMap metadata = threeDManager->getAssetMetadata(assetPath);

// Available fields:
metadata["vertices"]      // 15432
metadata["triangles"]     // 18920
metadata["materials"]     // 3
metadata["textures"]      // 5
metadata["animations"]    // ["idle", "walk", "run"]
metadata["hasSkeleton"]   // true
metadata["bones"]         // 45
metadata["morphTargets"]  // ["smile", "blink", "frown"]
metadata["lodLevels"]     // 3
metadata["bounds"]        // {"min": [-1, 0, -1], "max": [1, 2, 1]}
```

---

## File Organization

```
/assets/3d/
├── characters/      # Rigged characters, avatars
├── props/           # Furniture, objects
├── environments/    # Rooms, stages, scenes
├── effects/         # Particle meshes, effects
├── vr/              # VR-specific assets
│   ├── hands/
│   ├── controllers/
│   └── markers/
└── volumetric/      # Point clouds, voxels

Presets:
~/.config/neural-studio/3DModelNode_presets.json

Thumbnail cache:
~/.cache/neural-studio/3d_thumbs/
```

---

## Performance Optimizations

### LOD (Level of Detail)
- Automatic LOD generation
- Distance-based switching
- Poly reduction algorithms

### Instancing
- GPU instancing for repeated models
- Crowd simulation support

### Culling
- Frustum culling
- Occlusion culling
- Small object culling

---

## Animation System

### Skeletal Animation
- Bone hierarchies
- Inverse kinematics (IK)
- Animation blending
- Retargeting

### Morph Targets (Blend Shapes)
- Facial expressions
- Cloth deformation
- Custom shape keys

### Transform Animation
- Position/rotation/scale keyframes
- Curve interpolation
- Animation loops

---

## Integration Points

### With Scene Graph
- Creates 3DModelNode instances
- Spatial transforms
- Physics collision (planned)

### With VR Pipeline
- Stereoscopic rendering
- VR-optimized shading
- Hand tracking integration

### With Animation System
- Skeletal animation playback
- Blend shape control
- IK solver

---

## Future Enhancements
- [ ] Procedural modeling (geometry nodes)
- [ ] Physics simulation integration
- [ ] Cloth/soft body simulation
- [ ] Real-time photogrammetry
- [ ] NeRF (Neural Radiance Fields) support
- [ ] 3D marketplace integration
- [ ] Collaborative 3D editing
