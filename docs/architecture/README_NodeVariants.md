# Node Variant System Architecture

**Location**: `ui/blueprint/nodes/`  
**Pattern**: Compositional Node Variants  
**Status**: ✅ Implemented  
**Version**: 1.0.0  
**Date**: 2025-12-14  

---

## Overview

Neural Studio's nodes use a **variant pattern** where each node type (AudioNode, VideoNode, etc.) can have multiple specialized variants sharing a common backend but with unique UIs optimized for specific use cases.

**Core Architectural Principle**: 

> **ALL VARIANTS ARE FUNCTION-BASED, NOT PROVIDER/TECHNOLOGY-BASED**
> 
> - ✅ Variant = What the user wants to accomplish ("Stream Description", "Gesture Recognition")
> - ❌ Variant ≠ Provider or technology ("OpenAI", "ONNX")
> - 🔧 Providers/Technologies = Widgets loaded inside function variants

**Pattern**: One backend type → Many function-based variants → Provider/tech widgets inside each

---

## Folder Structure Pattern

### General Pattern
```
<NodeType>/
├── <NodeType>.qml                    # Main compositor - loads variants
├── <VariantName>/                    # Variant-specific node
│   └── <variantname>node.qml
├── <SharedWidget>/                   # Reusable widget
│   └── <widgetname>widget.qml
└── (more variants & widgets...)
```

### AudioNode Example (Current Implementation)
```
ui/blueprint/nodes/AudioNode/
├── audionode.qml                     # Main compositor
├── AudioClip/                        # Audio file playback variant
│   └── audioclipnode.qml
├── AudioStream/                      # Network audio stream variant
│   └── audiostreamnode.qml
├── AudioClipPlayerWidget/            # Shared playback widget
│   └── audioclipplayerwidget.qml    ✅ Implemented
├── AudioClipSettingsWidget/          # Shared settings widget
│   └── audioclipsettingswidget.qml
└── (more variants as needed...)
```

---

## How It Works

### 1. Manager Selection
User opens **AudioManager** → Selects variant type (Audio Clip, Audio Stream, etc.)

### 2. Node Creation
Manager creates **AudioNode** with `variantType` property set to variant name

### 3. Dynamic Loading
`audionode.qml` uses `Loader` to dynamically load the correct variant:
```qml
// audionode.qml (Main Compositor)
import QtQuick

Item {
    property string variantType: "audioclip"  // Set by manager
    
    Loader {
        id: variantLoader
        source: {
            switch(variantType) {
                case "audioclip": return "AudioClip/audioclipnode.qml"
                case "audiostream": return "AudioStream/audiostreamnode.qml"
                default: return ""
            }
        }
    }
}
```

### 4. Variant Composition
Each variant node loads shared widgets:
```qml
// AudioClip/audioclipnode.qml (Variant-Specific)
import QtQuick
import "../AudioClipPlayerWidget"
import "../AudioClipSettingsWidget"

Item {
    AudioClipPlayerWidget {
        id: player
        anchors.fill: parent
    }
    
    AudioClipSettingsWidget {
        id: settings
        anchors.right: parent.right
    }
}
```

---

## Shared Widgets

Widgets are **reusable components** that can be used across multiple variants:

### AudioClipPlayerWidget
**Purpose**: Playback controls for audio files  
**Features**:
- Waveform visualization
- Track list with album art
- Swipe navigation
- Information display (track name, artist, album)
- Playback controls

**Usage**:
```qml
import "../AudioClipPlayerWidget"

AudioClipPlayerWidget {
    id: player
}
```

**Located**: `AudioNode/AudioClipPlayerWidget/audioclipplayerwidget.qml` ✅

---

## Planned Variants Per Node Type

### AudioNode Variants (Phase 1)
| Variant (Function) | File | Status | Purpose |
|---------|------|--------|---------|
| **Audio File Playback** | `AudioFilePlayback/audiofileplaybacknode.qml` | 🔄 Stub | Play audio files (MP3, WAV, FLAC) |
| **Audio Stream Capture** | `AudioStreamCapture/audiostreamcapturenode.qml` | 🔄 Stub | Network audio (NDI/RTSP/HTTP) |
| **Microphone Input** | `MicrophoneInput/microphoneinputnode.qml` | 📋 Planned | Live mic recording |
| **AI Background Music** | `AIBackgroundMusic/aibackgroundmusicnode.qml` | 📋 Planned | AI-generated adaptive BGM |

**Widget Examples**: `QtMultimediaPlayerWidget.qml`, `FFmpegDecoderWidget.qml`, `NDIStreamWidget.qml`

### VideoNode Variants (Phase 1)
| Variant (Function) | File | Status | Purpose |
|---------|------|--------|---------|
| **Video File Playback** | `VideoFilePlayback/videofileplaybacknode.qml` | 📋 Planned | Play video files (MP4, MKV, WebM) |
| **VR 360° Playback** | `VR360Playback/vr360playbacknode.qml` | 📋 Planned | Full-sphere VR content |
| **VR 180° Playback** | `VR180Playback/vr180playbacknode.qml` | 📋 Planned | Half-sphere VR content |
| **Screen Capture** | `ScreenCapture/screencapturenode.qml` | 📋 Planned | Desktop/window recording |

**Widget Examples**: `QtMultimediaVideoWidget.qml`, `FFmpegVideoWidget.qml`, `EquirectangularProjectionWidget.qml`

### CameraNode Variants (Phase 1)
| Variant (Function) | File | Status | Purpose |
|---------|------|--------|---------|
| **Live Camera Capture** | `LiveCameraCapture/livecameracapturenode.qml` | 📋 Planned | Local camera (USB/V4L2) |
| **Stereo VR Capture** | `StereoVRCapture/stereovrcapturenode.qml` | 📋 Planned | Dual camera VR 180° |
| **Remote Camera Stream** | `RemoteCameraStream/remotecamerastreamnode.qml` | 📋 Planned | IP camera (RTSP/HTTP) |
| **VR Headset Passthrough** | `VRHeadsetPassthrough/vrheadsetpassthroughnode.qml` | 📋 Planned | OpenXR passthrough |

**Widget Examples**: `QtCameraWidget.qml`, `V4L2Widget.qml`, `RTSPStreamWidget.qml`, `OpenXRCameraWidget.qml`

### MLNode Variants (Phase 1)
| Variant | File | Status | Purpose |
|---------|------|--------|---------|
| Video Segmentation | `VideoSegmentation/videosegmentationnode.qml` | 📋 Planned | Semantic segmentation |
| Video Object Masking | `VideoObjectMasking/videoobjectmaskingnode.qml` | 📋 Planned | Object detection & masking |
| Video Gesture Recognition | `VideoGestureRecognition/videogesturerecognitionnode.qml` | 📋 Planned | Hand/body gesture detection |
| Background Removal | `BackgroundRemoval/backgroundremovalnode.qml` | 📋 Planned | Remove/replace background |

**ML Widget Pattern**: Each variant loads model-source widgets (ONNX, TensorFlow, PyTorch, etc.)

**Example Structure**:
```
MLNode/
├── mlnode.qml
├── VideoSegmentation/
│   ├── videosegmentationnode.qml
│   └── widgets/
│       ├── ONNXSegmentationWidget.qml      # ONNX model loader
│       ├── TensorFlowSegmentationWidget.qml # TF model loader
│       └── PyTorchSegmentationWidget.qml    # PyTorch model loader
├── VideoObjectMasking/
│   ├── videoobjectmaskingnode.qml
│   └── widgets/
│       ├── ONNXObjectMaskingWidget.qml
│       └── YOLOWidget.qml                   # Specialized YOLO widget
└── (more function-based variants...)
```

---

## Widget Reusability

Widgets can be shared across multiple variants:

### Example: AudioClipPlayerWidget
Used by:
- `AudioClip` variant (file playback)
- Future `Podcast` variant (episode playback)
- Future `Playlist` variant (multi-track)

### Example: Common Settings Widget
```qml
// Shared by all audio variants
AudioCommonSettings {
    volume: 0.8
    mute: false
    pan: 0.0
}
```

---

## Backend Integration

### C++ Controller
The **same C++ controller** is used for all variants:

```cpp
// AudioNodeController.h
class AudioNodeController : public BaseNodeController {
    Q_OBJECT
    Q_PROPERTY(QString variantType READ variantType NOTIFY variantTypeChanged)
    
public:
    QString variantType() const { return m_variantType; }
    
signals:
    void variantTypeChanged();
    
private:
    QString m_variantType;
    // Backend logic is shared
    // Only UI differs per variant
};
```

### Scene Graph Backend
The **same backend node** (`AudioNode` in `core/src/scene-graph/nodes/AudioNode/`) handles all variants. Variant type is a property, not a different class.

---

## Manager Integration

Managers act as **variant factories**:

```cpp
// AudioManagerWidget.cpp
QComboBox *variantSelector = new QComboBox();
variantSelector->addItem("🎵 Audio Clip", "audioclip");
variantSelector->addItem("📡 Audio Stream", "audiostream");
variantSelector->addItem("🎤 Microphone", "microphone");

connect(variantSelector, &QComboBox::activated, [this](int index) {
    QString variant = variantSelector->itemData(index).toString();
    m_controller->createNode("AudioNode", {
        {"variantType", variant},
        {"x", 100},
        {"y", 100}
    });
});
```

---

## Benefits

✅ **Code Reuse**: Widgets shared across variants  
✅ **Single Backend**: One C++ controller for all variants  
✅ **User Experience**: Each variant optimized for its use case  
✅ **Extensibility**: Add new variants without changing backend  
✅ **Lazy Loading**: Loaders reduce memory footprint  

---

## Future: Custom Variants

Users will be able to create **custom variants** by:
1. Selecting "Custom" in manager
2. Choosing which widgets to include
3. Arranging widget layout
4. Saving as custom variant template

---

## Implementation Status

**Current** (2025-12-14):
- ✅ Folder structure defined
- ✅ AudioClipPlayerWidget implemented
- 🔄 AudioClip variant (stub)
- 🔄 AudioStream variant (stub)

**Next Steps**:
1. Complete AudioClip variant node
2. Implement variant selector in AudioManager
3. Add Loader logic to audionode.qml
4. Create additional widgets (settings, visualizer)
5. Replicate pattern for VideoNode, CameraNode

---

## Related Documentation

- [README_AudioManager.md](README_AudioManager.md) - Audio manager architecture
- [README_VideoManager.md](README_VideoManager.md) - Video manager architecture
- [README_WasmNode.md](README_WasmNode.md) - Example node documentation

---

**Status**: Foundation implemented, variants in progress. Widget composition system ready for expansion.
