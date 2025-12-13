# VirtualCam / LayerMixer System

**Neural Studio's Multi-Headset Spatial Streaming Engine**

---

## Table of Contents
1. [Overview](#overview)
2. [Core Features](#core-features)
3. [Architecture](#architecture)
4. [3D Scene Composition](#3d-scene-composition)
5. [Multi-Headset Profiles](#multi-headset-profiles)
6. [Spatial Audio](#spatial-audio)
7. [Qt Integration](#qt-integration)
8. [Current Implementation Status](#current-implementation-status)
9. [Future Roadmap](#future-roadmap)
10. [Technical Specifications](#technical-specifications)

---

## Overview

The **VirtualCam/LayerMixer** is the heart of Neural Studio's VR broadcasting system. It composites stereoscopic video feeds, 3D objects, physics simulations, and spatial audio into multiple optimized streams for different VR headsets.

**Key Principle**: Stereoscopic video is positioned as a 3D plane in Z-space, with other elements properly depth-ordered around it.

```
Z-Axis Spatial Ordering:
  Z = -10m  Background (skybox, environment)
  Z = -5m   STEREOSCOPIC VIDEO PLANE (primary content)
  Z = -2m   3D avatars, effects (in front of video)
  Z = 0     VR Camera (viewer position)
```

---

## Core Features

### 1. Video-First 3D Composition
- **Stereoscopic video as 3D spatial object** (not flat background)
- Configurable Z-depth positioning
- Proper stereo disparity rendering
- Low-latency pass-through mode

### 2. Multi-Headset Streaming
- **Simultaneous output** to multiple VR headset profiles
- Per-profile optimization:
  - Resolution (1920x1920 to 2448x2448 per eye)
  - Frame rate (90Hz, 120Hz)
  - FOV (110° to 130°)
  - IPD (interpupillary distance)
  - Codec (H.265, AV1)

### 3. 3D Scene Graph Integration
- Qt Quick 3D for hardware-accelerated rendering
- Physics simulation (Qt Quick 3D Physics / PhysX)
- Depth-based occlusion and sorting
- Real-time object positioning

### 4. Spatial Audio
- Qt Spatial Audio (Resonance Audio engine)
- 3D positioned sound sources
- Room acoustics simulation
- HRTF (Head-Related Transfer Function)

### 5. SRT Network Streaming
- Unique SRT stream per headset profile
- Embedded spatial metadata (SEI messages)
- Adaptive bitrate (future)
- Low-latency transport

---

## Architecture

### Data Flow Pipeline

```
┌──────────────────────────────────────────────────────┐
│  INPUT SOURCES                                       │
│  • Stereoscopic camera (L/R)                         │
│  • 3D models, avatars                                │
│  • Audio tracks                                      │
└────────────────┬─────────────────────────────────────┘
                 │
                 ▼
┌──────────────────────────────────────────────────────┐
│  SCENE GRAPH (Semantic + Spatial)                    │
│  • Nodes with 3D position (x, y, z)                  │
│  • Semantic labels, categories                       │
│  • Relationships (NEAR, FACING, SUPPORTED_BY)        │
└────────────────┬─────────────────────────────────────┘
                 │
                 ▼
┌──────────────────────────────────────────────────────┐
│  SPATIAL COMPOSITOR (Qt Quick 3D)                    │
│  ┌────────────────────────────────────────────────┐  │
│  │ Video Plane (Z=-5m)                            │  │
│  │ • Stereo L/R textures                          │  │
│  │ • Full resolution                              │  │
│  └────────────────────────────────────────────────┘  │
│  ┌────────────────────────────────────────────────┐  │
│  │ 3D Overlays                                    │  │
│  │ • Positioned at various Z-depths               │  │
│  │ • Physics simulation                           │  │
│  └────────────────────────────────────────────────┘  │
│  ┌────────────────────────────────────────────────┐  │
│  │ Spatial Audio Mix (Qt Spatial Audio)           │  │
│  │ • Sources at 3D positions                      │  │
│  │ • Listener = VR camera                         │  │
│  └────────────────────────────────────────────────┘  │
└────────────────┬─────────────────────────────────────┘
                 │
                 ▼
┌──────────────────────────────────────────────────────┐
│  VIRTUALCAM MANAGER (Multi-Output)                   │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │
│  │ Quest 3     │  │ Valve Index │  │ Vive Pro 2  │  │
│  │ Profile     │  │ Profile     │  │ Profile     │  │
│  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘  │
└─────────┼──────────────────┼──────────────────┼───────┘
          │                  │                  │
          ▼                  ▼                  ▼
     ┌────────┐         ┌────────┐        ┌────────┐
     │H.265   │         │H.265   │        │AV1     │
     │Encoder │         │Encoder │        │Encoder │
     └────┬───┘         └────┬───┘        └────┬───┘
          │                  │                  │
          ▼                  ▼                  ▼
     ┌────────┐         ┌────────┐        ┌────────┐
     │SRT:9001│         │SRT:9002│        │SRT:9003│
     └────────┘         └────────┘        └────────┘
```

### Component Responsibilities

#### VirtualCamManager
**Location**: `obs-vr/VirtualCamManager.h/.cpp`

**Responsibilities**:
- Load and manage VR headset profiles
- Allocate framebuffers per profile
- Orchestrate multi-threaded rendering
- Coordinate encoding and streaming
- Provide stream status API

**Key Methods**:
```cpp
void loadProfilesFromDirectory(const std::string& profileDir);
void enableProfile(const std::string& profileId, bool enabled);
void startStreaming(SceneManager* sceneManager);
void renderFrame(SceneManager* sceneManager, double deltaTime);
std::vector<StreamInfo> getActiveStreams() const;
```

#### SpatialAudioManager
**Location**: `libvr/SpatialAudioManager.h/.cpp`

**Responsibilities**:
- Manage Qt Spatial Audio engine
- Position sound sources in 3D space
- Sync listener with VR camera
- Simulate room acoustics
- Output mixed audio for encoding

**Key Methods**:
```cpp
void addSpatialSound(const QString &nodeId, const QString &audioSource);
void updateSoundPosition(const QString &nodeId, const QVector3D &position);
void updateListenerPosition(const QVector3D &position);
void setVirtualRoom(const QVector3D &roomDimensions, float reflectionGain);
```

---

## 3D Scene Composition

### Video Plane Rendering

```qml
Model {
    // Stereoscopic video as 3D plane
    source: "#Rectangle"
    position: Qt.vector3d(0, 0, -5000)  // 5 meters from viewer
    scale: Qt.vector3d(10, 5.6, 1)      // 10m wide, 16:9 aspect
    
    materials: PrincipledMaterial {
        // Left eye texture for left eye rendering pass
        baseColorMap: Texture {
            sourceItem: Camera {
                deviceId: "stereo_left"
            }
        }
    }
}
```

### 3D Overlays

```qml
// Avatar in front of video
Model {
    source: "avatar.glb"
    position: Qt.vector3d(2, 0, -2000)  // 2m in front, 2m to right
    
    // Physics body
    DynamicRigidBody {
        mass: 70
        friction: 0.5
    }
    
    // Spatial audio attached
    SpatialSound {
        source: "avatar_voice.wav"
        position: parent.position
    }
}
```

### Depth Ordering Rules
1. **Z-buffer enabled**: Proper occlusion handling
2. **Transparent objects sorted**: Back-to-front rendering
3. **Video plane**: Fixed depth unless user adjusts
4. **UI elements**: Always on top (Z ≈ -0.5m)

---

## Multi-Headset Profiles

### Profile Structure

```cpp
struct VRHeadsetProfile {
    std::string name;              // "Meta Quest 3"
    std::string id;                // "quest3"
    
    // Display
    uint32_t eyeWidth;             // 1920
    uint32_t eyeHeight;            // 1920
    float fovHorizontal;           // 110.0°
    float fovVertical;             // 96.0°
    float ipd;                     // 63mm
    
    // Encoding
    std::string codec;             // "h265"
    uint32_t bitrate;              // 100000 kbps
    uint32_t framerate;            // 90 fps
    StereoFormat stereoFormat;     // SIDE_BY_SIDE
    
    // Metadata
    bool embedDepthMap;            // true
    bool embedSpatialAudio;        // true
    std::string spatialFormat;     // "equirectangular"
    
    // Streaming
    uint16_t srtPort;              // 9001
    bool enabled;                  // false
};
```

### Predefined Profiles

#### Meta Quest 3
- **Resolution**: 1920x1920 per eye
- **FOV**: 110° H / 96° V
- **Frame Rate**: 90 Hz
- **Codec**: H.265
- **Bitrate**: 100 Mbps
- **Port**: 9001

#### Valve Index
- **Resolution**: 1440x1600 per eye
- **FOV**: 130° H / 105° V (widest)
- **Frame Rate**: 120 Hz (highest)
- **Codec**: H.265
- **Bitrate**: 150 Mbps
- **Port**: 9002

#### HTC Vive Pro 2
- **Resolution**: 2448x2448 per eye (highest)
- **FOV**: 120° H/V
- **Frame Rate**: 90 Hz
- **Codec**: AV1 (best compression)
- **Bitrate**: 200 Mbps
- **Port**: 9003

### Custom Profiles
Users can create custom profiles via JSON:

```json
{
  "name": "Custom High-End",
  "id": "custom_hq",
  "eyeWidth": 2160,
  "eyeHeight": 2160,
  "fovHorizontal": 115.0,
  "fovVertical": 100.0,
  "ipd": 64.0,
  "codec": "av1",
  "bitrate": 250000,
  "framerate": 120,
  "stereoFormat": "side_by_side",
  "srtPort": 9010,
  "enabled": true
}
```

---

## Spatial Audio

### Audio Source Positioning

```cpp
// Video audio at video plane depth
audioManager.addSpatialSound("video_audio", "stream.wav");
audioManager.updateSoundPosition("video_audio", QVector3D(0, 0, -5000));

// Avatar voice in front
audioManager.addSpatialSound("avatar1", "voice.wav");
audioManager.updateSoundPosition("avatar1", QVector3D(2, 1, -2000));

// Background music (non-positional)
audioManager.addAmbientSound("bgm", "music.mp3");
```

### Listener (VR Camera) Sync

```cpp
// Update listener position every frame
void VirtualCamManager::renderFrame(SceneManager* scene, double dt) {
    QVector3D cameraPos = scene->getCameraPosition();
    QVector3D cameraForward = scene->getCameraForward();
    QVector3D cameraUp = scene->getCameraUp();
    
    m_audioManager->updateListenerPosition(cameraPos);
    m_audioManager->updateListenerOrientation(cameraForward, cameraUp);
    
    // Render scene...
}
```

### Room Acoustics

```cpp
// Simulate a 10m x 8m x 4m room
audioManager.setVirtualRoom(
    QVector3D(10000, 8000, 4000),  // Dimensions in mm
    0.5f                            // Reflection gain
);
```

---

## Qt Integration

### Core Qt Modules Used

1. **Qt Quick 3D** - Hardware-accelerated 3D rendering
2. **Qt Quick 3D Physics** - PhysX-based physics simulation
3. **Qt Spatial Audio** - Resonance Audio for 3D sound
4. **Qt Quick 3D Xr** - Native VR headset rendering (for editor preview)
5. **Qt Multimedia** - Camera/video input

### CMake Dependencies

```cmake
find_package(Qt6 REQUIRED COMPONENTS
    Quick
    Quick3D
    Multimedia
    SpatialAudio
)

target_link_libraries(neural_studio PRIVATE
    Qt6::Quick
    Qt6::Quick3D
    Qt6::Multimedia
    Qt6::SpatialAudio
)
```

### QML Example

```qml
import QtQuick
import QtQuick3D
import QtQuick3D.Physics
import QtQuick3D.SpatialAudio

View3D {
    // Video plane
    Model {
        source: "#Rectangle"
        position: Qt.vector3d(0, 0, -5000)
        materials: PrincipledMaterial {
            baseColorMap: videoTexture
        }
    }
    
    // Physics object
    DynamicRigidBody {
        Model {
            source: "#Cube"
            position: Qt.vector3d(0, 2, -3000)
        }
        mass: 10
    }
    
    // Spatial audio
    AudioEngine { id: audioEngine }
    SpatialSound {
        engine: audioEngine
        source: "sound.wav"
        position: Qt.vector3d(0, 0, -5000)
    }
}
```

---

## Current Implementation Status

### ✅ Completed (Phase 1)

- [x] **VirtualCamManager** skeleton
  - Profile management (add, remove, enable/disable)
  - Framebuffer allocation stubs
  - Multi-profile rendering loop
  - Stream query API

- [x] **SpatialAudioManager** implementation
  - Qt Spatial Audio wrapper
  - 3D sound positioning
  - Listener sync
  - Room acoustics

- [x] **VR Headset Profiles**
  - Profile data structure
  - JSON configs (Quest 3, Index, Vive Pro 2)
  - Profile loading system

- [x] **Architecture documents**
  - VirtualCam architecture
  - Video-first compositor design
  - Spatial scene graph spec

### 🚧 In Progress (Phase 2)

- [ ] **Stereo rendering pipeline**
  - Dual-pass rendering (L/R eyes)
  - IPD offset calculation
  - Per-eye framebuffer rendering

- [ ] **Video capture integration**
  - QtMultimedia camera input
  - Stereo texture mapping
  - Low-latency pass-through

### 📋 Planned (Phase 3)

- [ ] **Encoding & Streaming**
  - FFmpeg/libx265 integration
  - Per-profile encoding threads
  - SRT output implementation
  - Spatial metadata embedding (SEI)

- [ ] **UI Integration**
  - VirtualCam settings panel
  - Profile manager UI
  - Stream status display
  - Live preview toggle

- [ ] **Advanced Features**
  - Adaptive bitrate
  - Cloud relay (TURN/STUN)
  - WHIP/WebRTC support
  - Multi-viewer sync

---

## Future Roadmap

### Short Term (Q1 2025)

1. **Stereo Video Pipeline**
   - Implement dual-camera capture
   - Texture-to-3D-plane mapping
   - Verify low-latency performance

2. **Single-Profile Streaming**
   - Quest 3 as reference profile
   - Basic H.265 encoding
   - Local SRT streaming
   - Test with VR headset

3. **Basic 3D Overlays**
   - Simple geometric shapes
   - Alpha blending over video
   - Toggle on/off in UI

### Mid Term (Q2 2025)

1. **Multi-Profile Support**
   - Enable 2-3 simultaneous profiles
   - Per-profile encoding threads
   - Load balancing

2. **Physics Integration**
   - Qt Quick 3D Physics
   - Interactive 3D objects
   - Collision detection

3. **Avatar System**
   - Import glTF/GLB models
   - Basic animations
   - Positional tracking

### Long Term (Q3-Q4 2025)

1. **Cloud Streaming**
   - NAT traversal (TURN/STUN)
   - Global relay network
   - WebRTC fallback

2. **AI Enhancements**
   - ML-based super-resolution
   - Depth estimation from mono video
   - Auto scene labeling

3. **Advanced Spatial Features**
   - Volumetric video (Gaussian splats)
   - Photogrammetry integration
   - Real-time environment mapping

4. **Multi-User Support**
   - Shared VR spaces
   - Multi-viewer sync
   - Collaborative editing

---

## Technical Specifications

### Performance Targets

| Metric | Target | Current |
|--------|--------|---------|
| Latency (glass-to-glass) | <50ms | TBD |
| Video throughput | 4K stereo @ 90fps | - |
| Max simultaneous profiles | 3-5 | 3 (planned) |
| GPU utilization | <80% | - |
| Memory usage (per profile) | <500MB | - |

### System Requirements

**Minimum**:
- GPU: NVIDIA GTX 1060 / AMD RX 580
- RAM: 16GB
- CPU: Intel i5-8600K / AMD Ryzen 5 3600
- Network: 1 Gbps LAN

**Recommended**:
- GPU: NVIDIA RTX 3070 / AMD RX 6800
- RAM: 32GB
- CPU: Intel i7-12700K / AMD Ryzen 7 5800X
- Network: 10 Gbps LAN

### Encoding Specifications

| Codec | Bitrate Range | Resolution | Use Case |
|-------|---------------|------------|----------|
| H.265 | 50-200 Mbps | Up to 4K | Quest 3, Index |
| AV1 | 30-150 Mbps | Up to 5K | Vive Pro 2, high-efficiency |
| VP9 | 40-120 Mbps | Up to 4K | Web streaming |

---

## Development Workflow

### Building the VirtualCam System

```bash
cd Neural-Studio
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target obs-vr
cmake --build build --target vr_blueprint_qml
```

### Testing a Profile

```bash
# Start obs-vr backend
./build/obs-vr/obs-vr

# In another terminal, start QML frontend
./build/frontend/blueprint/vr_blueprint_qml

# Enable Quest 3 profile in UI
# Connect VR headset to srt://localhost:9001
```

### Creating Custom Profiles

1. Copy template: `cp profiles/quest3.json profiles/my_profile.json`
2. Edit parameters
3. Reload profiles in UI or restart app

---

## Contributing

### Adding New Headset Profiles

1. Research headset specifications
2. Create JSON in `profiles/`
3. Test with actual hardware
4. Submit PR with profile + validation results

### Extending the Compositor

Key files to modify:
- `obs-vr/VirtualCamManager.cpp` - Core rendering logic
- `libvr/SpatialAudioManager.cpp` - Audio processing
- `frontend/blueprint/qml/settings/virtualcamSettings.qml` - UI

---

## License

Neural Studio VirtualCam/LayerMixer system
Copyright (c) 2025

Licensed under GPL v3 (see LICENSE)

Third-party components:
- Qt (LGPL/Commercial)
- PhysX (BSD-3)
- Resonance Audio (Apache 2.0)
- FFmpeg (LGPL/GPL)

---

## Contact & Support

For questions, feature requests, or bug reports:
- GitHub Issues: [Neural-Studio/issues](https://github.com/jknohr/Neural-Studio/issues)
- Documentation: `docs/virtualcam/`
- Examples: `examples/virtualcam/`

---

**Last Updated**: 2025-12-13
**Version**: 0.1.0-alpha
**Status**: Early Development
