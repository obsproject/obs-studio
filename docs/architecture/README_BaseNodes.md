# Base Node System

**Location**: `core/src/scene-graph/BaseNodeBackend.h`, `ui/blueprint/nodes/BaseNodeController.h`, `ui/blueprint/nodes/BaseNode.qml`  
**Status**: ✅ Implemented  
**Version**: 1.0.0  
**Date**: 2025-12-13

---

## Overview

The **Base Node System** provides the foundational classes for both the C++ backend and the QML/C++ UI layer of Neural Studio's Blueprint editor. It ensures consistent behavior, standardized property handling, and a unified visual appearance across all node types.

### Key Capabilities
- ✅ **Centralized Logic**: Common functionality (pins, metadata, ID management) implemented once.
- ✅ **Backend Base**: `BaseNodeBackend` for all processing nodes.
- ✅ **UI Controller Base**: `BaseNodeController` for QML<->C++ property binding.
- ✅ **Visual Base**: `BaseNode.qml` for consistent node styling (headers, pins, selection).

---

## Architecture

### Inheritance Hierarchy

#### Backend (C++)
```
IExecutableNode (Interface)
       ↓
BaseNodeBackend (Common Logic)
       ↓
[Concrete Nodes] (e.g., StitchNode, RTXUpscaleNode)
```

#### UI Controller (C++)
```
QObject
   ↓
BaseNodeController (Common Properties)
       ↓
[Concrete Controllers] (e.g., StitchNodeController)
```

#### Visual Component (QML)
```
Rectangle (QtQuick)
    ↓
BaseNode.qml (Visual Template)
    ↓
[Concrete Node QML] (e.g., StitchNode.qml)
```

---

## Components

### 1. Backend: `BaseNodeBackend`
**File**: `core/src/scene-graph/BaseNodeBackend.h`

Provides the heavy lifting for node implementation so concrete nodes can focus on logic.

- **Features**:
  - `NodeMetadata` storage and retrieval.
  - Pin descriptor management (`addInput`, `addOutput`).
  - Type-safe data storage (`std::any` via `setPinData`/`getPinData`).
  - Helper templates: `getInputData<T>(pinId)` for easy safe access.

**Usage Example**:
```cpp
// In Constructor
addInput("video_in", "Video Input", DataType::Texture2D());

// In process()
auto* tex = getInputData<QRhiTexture*>("video_in");
```

### 2. UI Controller: `BaseNodeController`
**File**: `ui/blueprint/nodes/BaseNodeController.h`

The bridge between QML and the C++ backend graph.

- **Features**:
  - `nodeId`: Unique identifier management.
  - `enabled`: Global enable/disable state.
  - `updateProperty(name, value)`: Helper to sync changes to backend.
  - Signals for property changes.

**Usage Example**:
```cpp
class MyNodeController : public BaseNodeController {
    Q_OBJECT
    Q_PROPERTY(float intensity READ intensity WRITE setIntensity NOTIFY intensityChanged)
    // ...
};
```

### 3. Visual Base: `BaseNode.qml`
**File**: `ui/blueprint/nodes/BaseNode.qml`

Standardizes the look and feel of the node graph.

- **Features**:
   - **Header**: Title, colored category bar, icon placeholder.
   - **Body**: Container for custom node controls (sliders, combos).
   - **Interaction**: Drag-and-drop, selection highlighting, z-ordering.
   - **Ports**: Universal directional ports:
     - **Left (Blue)**: Visual Input
     - **Right (Blue)**: Visual Output
     - **Top (Yellow)**: Audio Input
     - **Bottom (Yellow)**: Audio Output
     *Note: Ports are only visible if enabled via Controller.*

**Usage Example**:
```qml
import "../" // Import BaseNode

BaseNode {
    title: "My Filter"
    headerColor: "#E67E22"
    
    // Custom content goes here
    Slider { ... }
}
```

---

## Integration

### Connecting UI to Backend
The `BaseNodeController` is designed to be paired with a corresponding `BaseNodeBackend` instance in the `NodeExecutionGraph`. 

1. **Frontend**: User adjusts property in `StitchNode.qml`.
2. **Controller**: `StitchNodeController` emits `propertyChanged`.
3. **Graph Manager**: Listens to signal, finds backend node by `nodeId`.
4. **Backend**: Calls `node->setProperty()` or updates config.
5. **Execution**: Next `process()` call uses new values.

---

## Future Enhancements

- [ ] **Property Binding**: Automatic two-way binding between backend `QVariantMap` and QML properties.
- [ ] **Error Visualization**: Show backend errors (red halo/tooltip) on the UI node.
- [ ] **Dynamic Pins**: Allow UI to add/remove pins (e.g., for "Merge" node).
