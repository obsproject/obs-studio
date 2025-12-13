# Blueprint Managers System

The **Managers** module provides a unified interface for discovering and integrating various resources (assets, hardware, AI models) into the Blueprint node graph.

## Overview

Managers bridge the **ActiveFrame UI** (traditional view) and the **Blueprint graph** (node-based view). They:
- Scan/discover available resources (filesystem, hardware, network)
- Display resources in organized panels
- Create corresponding nodes in the Blueprint graph on user action
- Set initial node properties automatically

## Architecture

### Base Pattern

All managers inherit from `BaseManagerController` and use `BaseManager.qml`:

```
┌─────────────────────────────────────┐
│       BaseManagerController         │
│  ┌──────────────────────────────┐   │
│  │ m_nodeGraphController        │   │  ← Reference to graph
│  └──────────────────────────────┘   │
│                                     │
│  + scanResources()                  │  ← Resource discovery
│  + createNode(resourceId)           │  ← Node creation
│  + setNodeProperty(nodeId, ...)    │  ← Property sync
└─────────────────────────────────────┘
                ▲
                │ inherits
     ┌──────────┴───────────┐
     │                      │
CameraManager      ThreeDAssetsManager
VideoManager       AudioManager
...                ...
```

### Communication Flow

```
[User clicks "Add" in Manager Panel]
           ↓
[Manager Controller]
  → createNode() via NodeGraphController
  → setNodeProperty() to configure node
           ↓
[NodeGraphController]
  → Delegates to SceneGraphManager
           ↓
[Backend Node Created]
  → ThreeDModelNode, CameraNode, etc.
           ↓
[Signal emitted: nodeCreated]
           ↓
[Blueprint Canvas updates]
```

## Implemented Managers

### CameraManager
- **Location**: `CameraManager/`
- **Scans**: V4L2 devices (`/dev/video*`)
- **Creates**: `CameraNode`
- **Properties**: `deviceId`
- **Backend**: Integrates with V4L2 video capture

### ThreeDAssetsManager
- **Location**: `ThreeDAssetsManager/`
- **Scans**: `assets/3d/*.{obj,glb,gltf,fbx}`
- **Creates**: `ThreeDModelNode`
- **Properties**: `modelPath`
- **Backend**: Integrates with `SceneManager` for 3D rendering

## Adding a New Manager

### 1. Create Directory Structure

```bash
ui/shared_ui/managers/
└── YourManager/
    ├── YourManagerController.h
    ├── YourManagerController.cpp
    ├── YourManager.qml
    └── CMakeLists.txt (optional, if separate build)
```

### 2. Controller Header

```cpp
#pragma once
#include "../BaseManagerController.h"

namespace NeuralStudio {
namespace Blueprint {

class YourManagerController : public BaseManagerController {
    Q_OBJECT
    Q_PROPERTY(QVariantList availableResources READ getResources NOTIFY resourcesChanged)

public:
    explicit YourManagerController(QObject* parent = nullptr);
    
    QVariantList getResources() const;
    
    Q_INVOKABLE void scanResources();
    Q_INVOKABLE void createYourNode(const QString& resourceId);

signals:
    void resourcesChanged();

private:
    QVariantList m_resources;
};

} // namespace Blueprint
} // namespace NeuralStudio
```

### 3. Controller Implementation

```cpp
#include "YourManagerController.h"
#include "../../blueprint/core/NodeGraphController.h"

void YourManagerController::scanResources()
{
    m_resources.clear();
    
    // Your resource discovery logic here
    // Example: filesystem scan, hardware enumeration, API query
    
    emit resourcesChanged();
}

void YourManagerController::createYourNode(const QString& resourceId)
{
    if (!m_nodeGraphController) return;
    
    QString nodeId = m_nodeGraphController->createNode("YourNodeType", 0.0f, 0.0f);
    
    if (!nodeId.isEmpty()) {
        // Set node properties
        m_nodeGraphController->setNodeProperty(nodeId, "propertyName", resourceId);
        qDebug() << "Created YourNode:" << nodeId;
    }
}
```

### 4. QML Panel

```qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import NeuralStudio.Blueprint 1.0

BaseManager {
    id: root
    title: "Your Resources"
    description: "Manage your resources"
    
    property YourManagerController controller: null
    
    content: ColumnLayout {
        anchors.fill: parent
        
        Button {
            text: "Refresh"
            onClicked: controller.scanResources()
        }
        
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: controller ? controller.availableResources : []
            
            delegate: ItemDelegate {
                width: parent.width
                text: modelData.name
                
                onClicked: {
                    controller.createYourNode(modelData.id)
                }
            }
        }
    }
}
```

### 5. Update CMakeLists

Add to `ui/shared_ui/managers/CMakeLists.txt`:

```cmake
set(MANAGERS_SOURCES
    # ... existing ...
    YourManager/YourManagerController.cpp
)

set(MANAGERS_HEADERS
    # ... existing ...
    YourManager/YourManagerController.h
)

install(FILES
    # ... existing ...
    YourManager/YourManager.qml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/neural-studio/qml/managers
)
```

## Manager Categories

### Asset Managers
Scan filesystem for media files:
- **ThreeDAssetsManager**: 3D models (`.obj`, `.glb`, `.fbx`)
- **VideoManager**: Video files (`.mp4`, `.mov`, `.avi`)
- **PhotoManager**: Images (`.jpg`, `.png`, `.tiff`)
- **AudioManager**: Audio files (`.wav`, `.mp3`, `.ogg`)
- **ScriptManager**: Scripts (`.lua`, `.py`)

### Source Managers
Detect hardware/network sources:
- **CameraManager**: V4L2 video devices
- **MicrophoneManager**: Audio input devices
- **ScreenCaptureManager**: Display outputs
- **NDIManager**: NDI network sources

### Logic/AI Managers
Browse AI models and logic:
- **MLManager**: ONNX models
- **AIManager**: Gemini/cloud AI services
- **EffectsManager**: Video effects
- **FiltersManager**: Image filters
- **TransitionsManager**: Transition effects

## API Reference

### BaseManagerController

**Properties:**
- `NodeGraphController* nodeGraphController`: Reference to the graph controller

**Methods:**
- `void setNodeGraphController(NodeGraphController*)`: Set graph reference

### NodeGraphController Integration

**Key Methods:**
```cpp
// Create a node
QString createNode(const QString& nodeType, float x, float y, float z = 0.0f);

// Set node properties after creation
void setNodeProperty(const QString& nodeId, 
                     const QString& propertyName, 
                     const QVariant& value);
```

## Best Practices

1. **Resource Discovery**: Cache scan results, provide refresh button
2. **Error Handling**: Check if `m_nodeGraphController` exists before creating nodes
3. **Logging**: Use `qDebug()` for successful operations, `qWarning()` for errors
4. **Properties**: Set all required node properties immediately after creation
5. **UI Feedback**: Emit signals when resources change, show loading states
6. **Performance**: Scan asynchronously for slow operations (network, large filesystems)

## Testing

To test a manager:
1. Implement backend node type first
2. Register node with `NodeFactory`
3. Verify node creation in `NodeGraphController`
4. Test property setting with `setNodeProperty`
5. Check QML UI loads correctly
6. Verify resource discovery logic
7. Test end-to-end: scan → create → configure

## Future Enhancements

- **Drag-and-Drop**: From manager panel to Blueprint canvas
- **Thumbnails**: Preview images for visual assets
- **Filters**: Search and category filters in panels
- **Batch Operations**: Multi-select and create
- **Hot Reload**: Watch filesystem for new assets
- **Cloud Integration**: Browse remote asset libraries
