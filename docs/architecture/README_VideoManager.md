# Video Manager

**Location**: `ui/shared_ui/managers/VideoManager/`, `ui/blueprint/nodes/VideoNode/`
**Status**: ✅ Implemented
**Version**: 1.0.0
**Date**: 2025-12-14

---

## Overview

The **Video Manager** manages video file sources in Neural Studio, including standard video clips, VR 180°/360° videos, and volumetric video assets. It provides video discovery, metadata extraction, and VideoNode creation with preset support.

### Key Features
- **Format Support**: MP4, MKV, WebM, MOV, AVI, HEVC, VP9, AV1
- **VR Video**: 180°/360° equirectangular and cubemap
- **HDR Support**: HDR10, HLG, Dolby Vision
- **Thumbnail Generation**: Auto-generate video previews
- **Codec Detection**: Automatic codec and metadata extraction
- **Multi-Node Tracking**: Manage multiple video sources

---

## Architecture

### Backend (`VideoManagerController.h/cpp`)
- **Inheritance**: `BaseManagerController` → `QObject`
- **Responsibilities**:
  - Scanning `/assets/videos/` directory
  - FFmpeg metadata extraction (resolution, codec, duration, bitrate)
  - Thumbnail generation
  - Tracking all VideoNode instances
  - Preset management

### Frontend Options

#### QML View (`VideoManager.qml`)
- **Purpose**: Blueprint Frame video library
- **Features**:
  - Grid view with video thumbnails
  - Metadata overlay (resolution, duration, codec)
  - VR badge for 180°/360° content
  - Drag-to-create VideoNode
  - "In Use" indicators

#### Qt Widget (`VideoManagerWidget.h/cpp`)
- **Purpose**: Active Frame video browser
- **Features**:
  - Sortable video library
  - Preview player
  - Metadata inspector
  - Import video button

---

## Interface

### Controller Properties
| Property | Type | Description |
|----------|------|-------------|
| `videoFiles` | QVariantList | Discovered videos with metadata |
| `managedNodes` | QStringList | Active VideoNode IDs |
| `nodePresets` | QVariantList | Saved video configurations |

### Methods
| Method | Parameters | Description |
|--------|------------|-------------|
| `scanVideos()` | - | Rescan video directory |
| `createVideoNode()` | videoPath, x, y | Create VideoNode |
| `savePreset()` | nodeId, presetName | Save video config |
| `loadPreset()` | presetName, x, y | Create from preset |
| `getVideoMetadata()` | videoPath | Extract codec info |
| `generateThumbnail()` | videoPath | Create preview image |

---

## Supported Formats

### Containers
- **MP4** (.mp4) - Most common
- **MKV** (.mkv) - Open format, supports HDR
- **WebM** (.webm) - Web-optimized
- **MOV** (.mov) - Apple QuickTime
- **AVI** (.avi) - Legacy support

### Codecs
- **H.264 (AVC)** - Universal compatibility
- **H.265 (HEVC)** - 4K/8K, HDR
- **VP9** - YouTube, web streaming
- **AV1** - Next-gen, high efficiency
- **ProRes** - Professional editing

### Special Formats
- **VR 180°** - Half-sphere equirectangular
- **VR 360°** - Full-sphere equirectangular or cubemap
- **Stereoscopic** - SBS (side-by-side) or TAB (top-and-bottom)
- **Volumetric** - Point cloud video (planned)

---

## Usage Example

### C++ (Controller)
```cpp
auto videoManager = new VideoManagerController(parent);
videoManager->scanVideos();

// Create video node
videoManager->createVideoNode("/assets/videos/intro_4k.mp4", 100, 100);

// Save configuration
videoManager->savePreset("video_node_123", "4K Intro Loop");
```

### QML (Blueprint UI)
```qml
VideoManager {
    controller: videoManagerController
    
    GridView {
        model: controller.videoFiles
        delegate: VideoCard {
            thumbnail: modelData.thumbnailPath
            videoName: modelData.name
            resolution: modelData.resolution
            duration: modelData.duration
            isVR: modelData.projection === "equirectangular"
            inUse: controller.managedNodes.includes(modelData.id)
        }
    }
}
```

---

## Metadata Extraction

### Via FFmpeg
```cpp
// Extract metadata
QVariantMap metadata = videoManager->getVideoMetadata(videoPath);

// Available fields:
metadata["resolution"]  // "3840x2160"
metadata["codec"]       // "hevc"
metadata["fps"]         // "60"
metadata["duration"]    // "120.5" (seconds)
metadata["bitrate"]     // "50000" (kbps)
metadata["hdr"]         // "hdr10"
metadata["projection"]  // "equirectangular" or "flat"
```

---

## File Organization

```
/assets/videos/
├── clips/           # Standard video clips
├── vr_180/          # 180° VR content
├── vr_360/          # 360° VR content
├── volumetric/      # Volumetric video (planned)
└── stock/           # Stock footage

Presets:
~/.config/neural-studio/VideoNode_presets.json

Thumbnails:
~/.cache/neural-studio/video_thumbs/
```

---

## Integration Points

### With Scene Graph
- Creates `VideoNode` instances
- Provides decoded frames to graph
- Hardware-accelerated decoding (VAAPI, NVDEC)

### With VR Pipeline
- Equirectangular projection
- Stereoscopic layout detection
- Spatial audio track extraction

---

## Performance Optimizations

### Hardware Decoding
- **VAAPI** (Linux Intel/AMD)
- **NVDEC** (NVIDIA)
- **QSV** (Intel Quick Sync)
- **VCN** (AMD VCE)

### Caching
- Thumbnail cache
- Metadata cache
- Frame buffer pool

---

## Future Enhancements
- [ ] Video transcoding to optimal format
- [ ] Clip trimming in-app
- [ ] Multi-track audio selection
- [ ] Subtitle/caption support
- [ ] Motion tracking data extraction
- [ ] AI-powered scene detection
