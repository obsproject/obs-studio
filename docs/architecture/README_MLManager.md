# ML Manager

**Location**: `ui/shared_ui/managers/MLManager/`, `ui/blueprint/nodes/MLNode/`
**Status**: ✅ Implemented
**Version**: 1.0.0
**Date**: 2025-12-14

---

## Overview

The **ML Manager** is a central hub for managing ONNX machine learning models within Neural Studio. It scans, tracks, and provides access to ML models, enabling users to create ML inference nodes with preconfigured settings through preset management.

### Key Features
- **Model Discovery**: Automatically scans `/assets/ml/` directory for `.onnx` files
- **Multi-Node Tracking**: Manages multiple ML nodes simultaneously
- **Preset Management**: Save/load model configurations with specific parameters
- **Hybrid UI**: Accessible from both QML (Blueprint) and Qt Widgets (Active Frame)
- **In-Use Indicators**: Visual feedback showing which models are currently in the scene graph

---

## Architecture

### Backend (`MLManagerController.h/cpp`)
- **Inheritance**: `BaseManagerController` → `QObject`
- **Responsibilities**:
  - Scanning `/assets/ml/` for ONNX model files
  - Tracking all created MLNode instances
  - Managing model presets (save/load/delete)
  - Providing model metadata (path, size, format)
  - Auto-cleanup when nodes are deleted

### Frontend Options

#### QML View (`MLManager.qml`)
- **Purpose**: Blueprint Frame integration
- **Features**:
  - Model list with metadata
  - Preset dropdown with loading capability
  - "In Use" badges
  - Refresh button
  - Drag-to-create MLNode functionality

#### Qt Widget (`MLManagerWidget.h/cpp`)
- **Purpose**: Active Frame dock widget / menu integration
- **Features**:
  - Tabbed interface for models/presets
  - Add model button
  - Model preview cards
  - Same controller as QML (shared logic)

---

## Interface

### Controller Properties
| Property | Type | Description |
|----------|------|-------------|
| `modelFiles` | QVariantList | List of discovered ONNX models with metadata |
| `managedNodes` | QStringList | IDs of all MLNodes created by this manager |
| `nodePresets` | QVariantList | Available presets for ML nodes |

### Methods
| Method | Parameters | Description |
|--------|------------|-------------|
| `scanModels()` | - | Rescan `/assets/ml/` directory |
| `createMLNode()` | modelPath, x, y | Create new MLNode at position |
| `savePreset()` | nodeId, presetName | Save current node config as preset |
| `loadPreset()` | presetName, x, y | Create node from preset |
| `deletePreset()` | presetName | Remove a preset |

---

## Usage Example

### C++ (Controller)
```cpp
auto mlManager = new MLManagerController(parent);
mlManager->setGraphController(graphCtrl);
mlManager->scanModels();

// Create an ML node
mlManager->createMLNode("/assets/ml/yolov8.onnx", 100, 100);

// Save configuration as preset
mlManager->savePreset("node_123", "YOLOv8 Person Detection");
```

### QML (Blueprint UI)
```qml
MLManager {
    id: mlManager
    controller: mlManagerController
    
    // Display discovered models
    Repeater {
        model: mlManager.controller.modelFiles
        delegate: ModelCard {
            modelPath: modelData.path
            modelName: modelData.name
            inUse: mlManager.controller.managedNodes.includes(modelData.id)
        }
    }
}
```

### Qt Widget (Active Frame)
```cpp
auto mlWidget = new MLManagerWidget(controller, parent);
mlWidget->setTitle("ML Models");
mlWidget->refresh();

// Appears in dock widget or menu
dockWidget->setWidget(mlWidget);
```

---

## File Organization

```
ui/shared_ui/managers/MLManager/
├── MLManagerController.h      # Hub logic
├── MLManagerController.cpp
├── MLManagerWidget.h          # Qt Widgets UI
├── MLManagerWidget.cpp
└── MLManager.qml             # QML UI for Blueprint

Presets stored in:
~/.config/neural-studio/MLNode_presets.json
```

---

## Integration Points

### With Scene Graph
- Creates `MLNode` instances via `NodeGraphController`
- Tracks node lifecycle via `nodeDeleted` signal
- Maintains list of active ML nodes

### With Blueprint
- QML panel in Blueprint Frame
- Drag-to-create node functionality
- Real-time "in use" status updates

### With Active Frame
- Widget in dock panel or menu
- Quick model browsing
- Preset management interface

---

## Future Enhancements
- [ ] Model format validation (ONNX schema check)
- [ ] Input/output tensor inspection
- [ ] Performance metrics (inference time, memory usage)
- [ ] Model quantization support
- [ ] Cloud model repository integration
