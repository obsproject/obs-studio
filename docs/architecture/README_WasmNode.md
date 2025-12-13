# WASM Node

**Location**: `core/src/scene-graph/nodes/wasm/`, `ui/blueprint/nodes/WasmNode/`
**Status**: ✅ Initial Implementation
**Version**: 1.0.0
**Date**: 2025-12-13

---

## Overview

The **WASM Node** enables the execution of WebAssembly modules within the Neural Studio pipeline. It provides a secure, sandboxed environment for running custom logic, effects, or third-party plugins without recompiling the main application.

### Key Features
- **Sandboxed Execution**: Runs untrusted code safely.
- **Dynamic Loading**: Load `.wasm` modules at runtime.
- **Interoperability**: Standard Input/Output via ABI (Application Binary Interface).
- **Performance**: Near-native speed using WasmEdge/Wasmtime (planned).

---

## Architecture

### Backend (`WasmNode.h/cpp`)
- **Inheritance**: `AINodeBase` -> `IExecutableNode`
- **Responsibilities**:
  - Loading WASM binary from disk.
  - initializing WASM runtime (VM).
  - Marshalling data between Scene Graph `std::any` pins and WASM memory.
  - Executing the entry function (default: `process`).

### Frontend (`WasmNodeController` + `WasmNode.qml`)
- **Controller**: Manages `wasmPath` and `entryFunction` properties.
- **UI**: 
  - File picker for `.wasm` file.
  - Input field for function name.
  - Inherits `BaseNode` layout (Blue Visual Ports, Yellow Audio Ports).

---

## Interface

### Inputs
- **Generic Data**: Can accept any standard type (Texture, Audio, Float) defined by the specific WASM module's contract.

### Outputs
- **Result Data**: Outputs the processed result.

### Properties
| Property | Type | Description |
|----------|------|-------------|
| `wasmPath` | String | Absolute path to the `.wasm` file. |
| `entryFunction` | String | Name of the exported function to call (default: "process"). |

---

## Usage Example

### C++ (Backend)
```cpp
auto node = std::make_shared<WasmNode>("my_plugin");
node->setWasmPath("/path/to/filter.wasm");
node->initialize();
// Execution handled by graph
```

### QML (Frontend)
```qml
WasmNode {
    title: "Custom Filter"
    controller.wasmPath: "/plugins/my_effect.wasm"
    
    // Connect visual input
    controller.visualInputEnabled: true
}
```
