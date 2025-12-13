# Script Manager

**Location**: `ui/shared_ui/managers/ScriptManager/`, `ui/blueprint/nodes/ScriptNode/`
**Status**: ✅ Implemented
**Version**: 1.0.0
**Date**: 2025-12-14

---

## Overview

The **Script Manager** manages Lua and Python scripts within Neural Studio's node-based workflow. It provides script discovery, multi-language support, and enables users to create ScriptNodes with preconfigured automation behaviors.

### Key Features
- **Multi-Language Support**: Lua (🌙) and Python (🐍) scripts
- **Script Discovery**: Auto-scans `/assets/scripts/` for `.lua` and `.py` files
- **Multi-Node Tracking**: Manages multiple ScriptNode instances
- **Preset Management**: Save/load script configurations
- **Hybrid UI**: QML (Blueprint) and Qt Widgets (Active Frame) interfaces
- **Language-Specific Icons**: Visual distinction between Lua and Python

---

## Architecture

### Backend (`ScriptManagerController.h/cpp`)
- **Inheritance**: `BaseManagerController` → `QObject`
- **Responsibilities**:
  - Scanning `/assets/scripts/` for `.lua` and `.py` files
  - Language detection and icon assignment
  - Tracking all ScriptNode instances
  - Preset management with script parameters
  - Node lifecycle management

### Frontend Options

#### QML View (`ScriptManager.qml`)
- **Purpose**: Blueprint Frame script panel
- **Features**:
  - Categorized script list (Lua/Python)
  - Language-specific icons (🌙 moon for Lua, 🐍 snake for Python)
  - Preset dropdown
  - Drag-to-create functionality
  - "In Use" indicators

#### Qt Widget (`ScriptManagerWidget.h/cpp`)
- **Purpose**: Active Frame integration
- **Features**:
  - Filterable script list
  - Language tabs
  - Script editor preview
  - Add/edit script buttons

---

## Interface

### Controller Properties
| Property | Type | Description |
|----------|------|-------------|
| `scriptFiles` | QVariantList | Discovered scripts with metadata (path, language, icon) |
| `managedNodes` | QStringList | IDs of all active ScriptNodes |
| `nodePresets` | QVariantList | Saved script configurations |

### Methods
| Method | Parameters | Description |
|--------|------------|-------------|
| `scanScripts()` | - | Rescan scripts directory |
| `createScriptNode()` | scriptPath, language, x, y | Create ScriptNode |
| `savePreset()` | nodeId, presetName | Save script config as preset |
| `loadPreset()` | presetName, x, y | Create node from preset |
| `getScriptLanguage()` | filePath | Detect language from extension |

---

## Usage Example

### C++ (Controller)
```cpp
auto scriptManager = new ScriptManagerController(parent);
scriptManager->scanScripts();

// Create a Lua script node
scriptManager->createScriptNode(
    "/assets/scripts/auto_framing.lua", 
    "lua", 
    200, 150
);

// Save configuration
scriptManager->savePreset("script_node_456", "Auto Framing Default");
```

### QML (Blueprint UI)
```qml
ScriptManager {
    controller: scriptManagerController
    
    ListView {
        model: controller.scriptFiles
        delegate: ScriptCard {
            scriptName: modelData.name
            language: modelData.language
            icon: modelData.language === "lua" ? "🌙" : "🐍"
            inUse: controller.managedNodes.includes(modelData.id)
        }
    }
}
```

### Qt Widget (Active Frame)
```cpp
auto scriptWidget = new ScriptManagerWidget(controller, parent);
scriptWidget->setTitle("Scripts");
scriptWidget->refresh();
```

---

## Supported Languages

### Lua
- **Extension**: `.lua`
- **Icon**: 🌙 (Moon)
- **Use Cases**: Lightweight automation, event triggers, node behavior
- **Runtime**: LuaJIT integration (planned)

### Python
- **Extension**: `.py`
- **Icon**: 🐍 (Snake)
- **Use Cases**: Complex data processing, API integrations, ML pipelines
- **Runtime**: Python embedded interpreter (planned)

---

## File Organization

```
ui/shared_ui/managers/ScriptManager/
├── ScriptManagerController.h
├── ScriptManagerController.cpp
├── ScriptManagerWidget.h
├── ScriptManagerWidget.cpp
└── ScriptManager.qml

Scripts stored in:
/assets/scripts/
├── automation/
├── effects/
└── tools/

Presets:
~/.config/neural-studio/ScriptNode_presets.json
```

---

## Integration Points

### With Scene Graph
- Creates `ScriptNode` instances
- Executes scripts within node execution context
- Provides script output to connected nodes

### With Automation System
- Scripts can trigger scene changes
- Event-driven execution
- Macro recording/playback

---

## Script API (Planned)

Scripts will have access to:
```lua
-- Lua example
neural.setNodeProperty("camera_1", "fov", 60)
neural.createNode("EffectNode", 100, 200)
neural.emit("custom_event", {data = "value"})
```

```python
# Python example
import neural

neural.set_node_property("camera_1", "fov", 60)
neural.create_node("EffectNode", 100, 200)
neural.emit("custom_event", {"data": "value"})
```

---

## Future Enhancements
- [ ] Script editor with syntax highlighting
- [ ] Debugger integration
- [ ] Hot reload support
- [ ] Script dependency management
- [ ] Community script marketplace
