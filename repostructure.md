# Neural Studio Repository Structure

This document outlines the current structure of the **Neural Studio** repository (formerly OBS-RTX-VR-SuperStream). The architecture has shifted from a monolithic OBS fork to a modular 3D-native broadcasting engine.

## Directory Layout

```
Neural-Studio/
├── frontend/                 # Qt6/QML User Interface (Dual View: ActiveFrame & BlueprintFrame)
├── libvr/                    # Core C++ VR Engine (Vulkan, OpenXR, ML, Audio)
│   ├── core/                 # Engine lifecycle and main loop
│   ├── render/               # Vulkan renderer and compositor
│   ├── scene/                # 3D SceneGraph management
│   ├── ml/                   # Machine Learning subsystem (ONNX/Gemini)
│   ├── transport/            # Network protocols (SRT, RTMP, WebRTC)
│   ├── shaders/              # Compute and graphics shaders
│   └── test/                 # Unit tests for adapters and core systems
├── obs-vr/                   # Application Entry Point & Legacy Bridge
├── plugins/                  # OBS-compatible plugins (Legacy & Native)
│   ├── obs-nvenc/            # NVIDIA Encoder integration
│   ├── obs-ffmpeg/           # FFmpeg encoder/decoder integration
│   └── ...                   # Various source/filter plugins
├── shared/                   # Shared utilities and protocols
│   ├── vr-protocol/          # IPC definitions
│   └── ...
├── cmake/                    # Build configuration modules
├── docs/                     # Documentation
└── build/                    # Build artifacts (not tracked)
```

## detailed Structure

### `libvr/` (The Engine)
This is the heart of Neural Studio, replacing `libobs`.
- **`render/`**: Contains the Vulkan backend replacing the old OpenGL/D3D11 wrapper.
- **`scene/`**: Implements the true 3D SceneGraph.
- **`ml/`**: Hosts the `ML_Adapter` for AI features.
- **`transport/`**: Contains `SRTAdapter` and other streaming implementations.

### `frontend/` (The UI)
- **`vr_studio`**: The new main executable.
- **`qml/`**: Declarative UI definitions for the Node Editor and Dashboard.

### `plugins/` (Extensions)
- **Native**: C++ plugins that link against `libvr`.
- **WASM**: (Planned) Sandboxed WebAssembly plugins.
- **Legacy**: Specific OBS plugins maintained for compatibility (e.g., specific capture methods).

## Key Files
- `CMakeLists.txt`: Root build configuration.
- `README.md`: Project overview and build instructions.
- `DEVELOPMENTPLAN.md`: Roadmap and architectural vision.