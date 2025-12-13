# Managers System Architecture

## Overview

The **Managers System** provides a unified interface for discovering, browsing, and integrating various resources into the Blueprint node graph. Managers bridge the gap between external resources (filesystem, hardware, network) and the internal scene graph.

## Location
- **Controllers**: `ui/shared_ui/managers/*/Controller.{h,cpp}`
- **QML Panels**: `ui/shared_ui/managers/*/*.qml`
- **Base Classes**: `ui/shared_ui/managers/BaseManagerController.{h,cpp}` and `BaseManager.qml`
- **Documentation**: `ui/shared_ui/managers/README_MANAGERS.md`

## Purpose

Enable users to:
- Discover available resources (assets, devices, AI models)
- Browse resources in organized panels
- Add resources to Blueprint graph with one click
- Automatically configure node properties

## Architecture

### System Overview

```
┌─────────────────────────────────────────────────┐
│           Managers System                       │
│                                                 │
│  ┌──────────────────────────────────────────┐  │
│  │  Asset Managers                          │  │
│  │  - ThreeDAssetsManager (3D models)       │  │
│  │  - VideoManager (video files)            │  │
│  │  - PhotoManager (images)                 │  │
│  │  - AudioManager (audio files)            │  │
│  └──────────────────────────────────────────┘  │
│                                                 │
│  ┌──────────────────────────────────────────┐  │
│  │  Source Managers                         │  │
│  │  - CameraManager (V4L2 devices)          │  │
│  │  - MicrophoneManager (audio input)       │  │
│  │  - ScreenCaptureManager (displays)       │  │
│  └──────────────────────────────────────────┘  │
│                                                 │
│  ┌──────────────────────────────────────────┐  │
│  │  Logic/AI Managers                       │  │
│  │  - MLManager (ONNX models)               │  │
│  │  - AIManager (Gemini/cloud AI)           │  │
│  │  - ScriptManager (Lua/Python)            │  │
│  └──────────────────────────────────────────┘  │
└─────────────────────────────────────────────────┘
                    ↓
        ┌───────────────────────┐
        │ NodeGraphController   │
        │ - createNode()        │
        │ - setNodeProperty()   │
        └───────────────────────┘
                    ↓
        ┌───────────────────────┐
        │  Blueprint Graph      │
        │  (Backend Nodes)      │
        └───────────────────────┘
```

### Class Hierarchy

```
QObject
    ↓
BaseManagerController (abstract)
    ↓
    ├── CameraManagerController
    ├── ThreeDAssetsManagerController
    ├── VideoManagerController
    ├── PhotoManagerController (planned)
    ├── AudioManagerController (planned)
    └── ...
```

### Key Components

#### BaseManagerController
**Location**: `ui/shared_ui/managers/BaseManagerController.h`

**Responsibilities**:
- Holds reference to `NodeGraphController`
- Provides common interface for all managers

**Properties**:
```cpp
NodeGraphController* m_nodeGraphController;
```

**Methods**:
```cpp
void setNodeGraphController(NodeGraphController* controller);
```

#### BaseManager.qml
**Location**: `ui/shared_ui/managers/BaseManager.qml`

**Provides**:
- Common panel UI structure
- Title and description
- Content area for custom manager UI

## Implementation Pattern

### 1. Backend Node
Create node type in `core/src/scene-graph/nodes/`:

```cpp
class YourNode : public BaseNodeBackend {
public:
    YourNode(const std::string& id);
    void execute(const ExecutionContext& context) override;
    
    void setProperty(const std::string& value);
    std::string getProperty() const;
    
private:
    std::string m_property;
};
```

### 2. Node Factory Registration
```cpp
// YourNodeFactory.cpp
NodeFactory::registerNodeType("YourNode", createYourNode);
```

### 3. Manager Controller
```cpp
class YourManagerController : public BaseManagerController {
    Q_OBJECT
    Q_PROPERTY(QVariantList resources READ getResources NOTIFY resourcesChanged)

public:
    Q_INVOKABLE void scanResources();
    Q_INVOKABLE void createNode(const QString& resourceId);

signals:
    void resourcesChanged();
};
```

**Key Methods**:
- **`scanResources()`**: Discover available resources
- **`createNode(resourceId)`**: Create backend node via NodeGraphController

### 4. Manager QML Panel
```qml
BaseManager {
    title: "Your Resources"
    description: "Browse and add resources"
    
    property YourManagerController controller: null
    
    content: ColumnLayout {
        Button {
            text: "Refresh"
            onClicked: controller.scanResources()
        }
        
        ListView {
            model: controller.resources
            delegate: ItemDelegate {
                onClicked: controller.createNode(modelData.id)
            }
        }
    }
}
```

## Data Flow

### Resource Discovery → Node Creation

```
1. User Opens Manager Panel
   ↓
2. Manager.scanResources()
   ├─ Filesystem scan (Assets)
   ├─ Hardware enumeration (Sources)
   └─ API query (Cloud/AI)
   ↓
3. Populate QVariantList
   ↓
4. Display in ListView
   ↓
5. User clicks "Add"
   ↓
6. Controller.createNode(resourceId)
   ↓
7. NodeGraphController.createNode(nodeType)
   ↓
8. NodeGraphController.setNodeProperty(nodeId, "prop", value)
   ↓
9. Backend Node Created & Configured
   ↓
10. Signal: nodeCreated(nodeId)
    ↓
11. Blueprint Canvas Updates
```

## Implemented Managers

### CameraManager ✓
- **Scans**: `/dev/video*` (Linux V4L2)
- **Creates**: `CameraNode`
- **Properties**: `deviceId`

### ThreeDAssetsManager ✓
- **Scans**: `assets/3d/*.{obj,glb,gltf,fbx}`
- **Creates**: `ThreeDModelNode`
- **Properties**: `modelPath`

### VideoManager ✓
- **Scans**: `assets/video/*.{mp4,mov,avi,mkv,webm}`
- **Creates**: `VideoNode`
- **Properties**: `videoPath`, `loop`

## Planned Managers

### Asset Managers
- **PhotoManager**: `assets/images/*.{jpg,png,tiff,exr}` → `ImageNode`
- **AudioManager**: `assets/audio/*.{wav,mp3,ogg,flac}` → `AudioNode`
- **ScriptManager**: `assets/scripts/*.{lua,py}` → `ScriptNode`

### Source Managers
- **MicrophoneManager**: PulseAudio/ALSA devices → `AudioInputNode`
- **ScreenCaptureManager**: X11/Wayland outputs → `ScreenCaptureNode`
- **NDIManager**: Network NDI sources → `NDIInputNode`

### Logic/AI Managers
- **MLManager**: ONNX models → `OnnxNode`
- **AIManager**: Gemini/LLM connections → `GeminiNode`
- **EffectsManager**: Video effects → `EffectNode`

## NodeGraphController Integration

The central integration point for all managers:

```cpp
class NodeGraphController {
public:
    // Create node in graph
    Q_INVOKABLE QString createNode(
        const QString& nodeType, 
        float x, float y, float z = 0.0f
    );
    
    // Set node properties after creation
    Q_INVOKABLE void setNodeProperty(
        const QString& nodeId,
        const QString& propertyName,
        const QVariant& value
    );
};
```

**Implementation** (simplified):
```cpp
void setNodeProperty(const QString& nodeId, 
                     const QString& propertyName, 
                     const QVariant& value)
{
    auto node = getNode(nodeId);
    
    if (nodeType == "VideoNode") {
        auto videoNode = dynamic_pointer_cast<VideoNode>(node);
        if (propName == "videoPath") {
            videoNode->setVideoPath(value.toString().toStdString());
        }
    }
    // ... other node types
}
```

## Build System

### CMakeLists.txt Structure

```cmake
# ui/shared_ui/managers/CMakeLists.txt

set(MANAGERS_SOURCES
    BaseManagerController.cpp
    CameraManager/CameraManagerController.cpp
    ThreeDAssetsManager/ThreeDAssetsManagerController.cpp
    VideoManager/VideoManagerController.cpp
)

add_library(ui-managers STATIC ${MANAGERS_SOURCES})

install(FILES
    BaseManager.qml
    CameraManager/CameraManager.qml
    ThreeDAssetsManager/ThreeDAssetsManager.qml
    VideoManager/VideoManager.qml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/neural-studio/qml/managers
)
```

## Testing

### Unit Tests
- Manager controller initialization
- Resource scanning logic
- Node creation requests
- Property setting

### Integration Tests
- End-to-end: scan → create → configure
- Multiple managers simultaneously
- Node graph updates
- UI panel rendering

## Best Practices

### 1. Resource Discovery
- **Cache results**: Don't rescan on every access
- **Provide refresh**: Let users manually update
- **Handle errors**: Show meaningful messages

### 2. Node Creation
- **Check controller**: Verify `m_nodeGraphController` exists
- **Set properties**: Configure nodes immediately after creation
- **Log actions**: Debug output for troubleshooting

### 3. UI Design
- **Show metadata**: Name, type, size, etc.
- **Empty states**: Guide users when no resources found
- **Loading states**: Indicate scanning in progress

## Future Enhancements

### Short Term
- [ ] Drag-and-drop from manager to canvas
- [ ] Asset thumbnails/previews
- [ ] Search and filter UI
- [ ] Multi-select and batch operations

### Medium Term
- [ ] Hot-reload (filesystem watching)
- [ ] Cloud asset libraries
- [ ] Asset tags and organization
- [ ] Recently used list

### Long Term
- [ ] Asset preprocessing pipeline
- [ ] Format conversion utilities
- [ ] Asset versioning
- [ ] Collaborative asset sharing

## Related Documentation
- [Blueprint System](./README_Blueprint.md)
- [Node Graph](./README_NodeGraph.md)
- [Scene Graph](./README_SceneGraph.md)
- [Individual Node READMEs](./README_VideoNode.md)
