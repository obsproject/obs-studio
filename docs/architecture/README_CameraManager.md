# Camera Manager

**Location**: `ui/shared_ui/managers/CameraManager/`, `ui/blueprint/nodes/CameraNode/`
**Status**: ✅ Implemented
**Version**: 1.0.0
**Date**: 2025-12-14

---

## Overview

The **Camera Manager** discovers and manages all camera sources in Neural Studio, including physical webcams, virtual cameras, stereoscopic rigs, and VR headset cameras. It provides a unified interface for camera discovery, tracking, and node creation.

### Key Features
- **V4L2 Detection**: Auto-scans `/dev/video*` for Linux camera devices
- **Multi-Camera Support**: Track multiple simultaneous camera sources
- **Stereoscopic Cameras**: Support for 180°/360° VR camera rigs
- **Virtual Cameras**: Manage software-based camera sources
- **Preset Management**: Save camera configurations (resolution, FPS, format)
- **Hybrid UI**: QML and Qt Widgets interfaces

---

## Architecture

### Backend (`CameraManagerController.h/cpp`)
- **Inheritance**: `BaseManagerController` → `QObject`
- **Responsibilities**:
  - Scanning `/dev/video*` for V4L2 devices  
  - Detecting camera capabilities (resolutions, formats, FPS)
  - Tracking all CameraNode instances
  - Managing camera presets
  - Device availability monitoring

### Frontend Options

#### QML View (`CameraManager.qml`)
- **Purpose**: Blueprint Frame camera panel
- **Features**:
  - Live camera previews
  - Device list with capabilities
  - "In Use" indicators
  - Resolution/FPS selection
  - Drag-to-create CameraNode

#### Qt Widget (`CameraManagerWidget.h/cpp`)
- **Purpose**: Active Frame camera control
- **Features**:
  - Camera device list
  - Live preview thumbnails
  - Quick settings (brightness, contrast)
  - Add camera button

---

## Interface

### Controller Properties
| Property | Type | Description |
|----------|------|-------------|
| `availableCameras` | QVariantList | Discovered camera devices with metadata |
| `managedNodes` | QStringList | Active CameraNode IDs |
| `nodePresets` | QVariantList | Saved camera configurations |

### Methods
| Method | Parameters | Description |
|--------|------------|-------------|
| `scanCameras()` | - | Rescan V4L2 devices |
| `createCameraNode()` | deviceId, deviceName | Create CameraNode |
| `savePreset()` | nodeId, presetName | Save camera config |
| `loadPreset()` | presetName, x, y | Create node from preset |
| `getCameraCapabilities()` | deviceId | Get formats, resolutions, FPS |

---

## Camera Types

### Physical Webcams
- **Detection**: V4L2 enumeration
- **Formats**: MJPEG, YUYV, H.264
- **Resolutions**: 480p to 4K
- **Use Case**: Standard desktop streaming

### Stereoscopic Cameras
- **Detection**: Dual V4L2 devices (matched pair)
- **Synchronization**: Hardware or software sync
- **Output**: Side-by-side or top-bottom
- **Use Case**: VR 180° content

### VR Headset Cameras
- **Detection**: OpenXR/SteamVR passthrough
- **Resolutions**: Up to 8K per eye
- **Formats**: Distorted fisheye (requires correction)
- **Use Case**: Mixed reality streaming

### Virtual Cameras
- **Detection**: Software loopback devices (v4l2loopback)
- **Sources**: OBS Virtual Camera, Unity, Unreal
- **Use Case**: Composited output as input

---

## Usage Example

### C++ (Controller)
```cpp
auto cameraManager = new CameraManagerController(parent);
cameraManager->scanCameras();

// Create camera node from device
cameraManager->createCameraNode("/dev/video0", "Logitech C920");

// Save configuration
cameraManager->savePreset("cam_node_001", "Webcam 1080p60");
```

### QML (Blueprint UI)
```qml
CameraManager {
    controller: cameraManagerController
    
    GridView {
        model: controller.availableCameras
        delegate: CameraCard {
            deviceName: modelData.name
            devicePath: modelData.path
            resolution: modelData.defaultResolution
            fps: modelData.defaultFps
            inUse: controller.managedNodes.includes(modelData.id)
        }
    }
}
```

---

## File Organization

```
ui/shared_ui/managers/CameraManager/
├── CameraManagerController.h
├── CameraManagerController.cpp
├── CameraManagerWidget.h
├── CameraManagerWidget.cpp
└── CameraManager.qml

Presets:
~/.config/neural-studio/CameraNode_presets.json
```

---

## Integration Points

### With Scene Graph
- Creates `CameraNode` instances
- Provides video frames to graph
- Hardware-accelerated capture (V4L2 DMA-BUF)

### With VR Pipeline
- Stereoscopic camera pairing
- Spatial audio anchoring
- Headset passthrough mixing

---

## Platform Support

### Linux (V4L2)
- ✅ Full support via Video4Linux2
- ✅ Hardware decoding (VAAPI)
- ✅ DMA-BUF zero-copy

### Windows (DirectShow/Media Foundation)
- 🔄 Planned

### macOS (AVFoundation)
- 🔄 Planned

---

## Future Enhancements
- [ ] USB hotplug detection
- [ ] NDI camera sources
- [ ] IP camera (RTSP) support
- [ ] Camera calibration tools
- [ ] Multi-camera synchronization
- [ ] Auto white balance/exposure
