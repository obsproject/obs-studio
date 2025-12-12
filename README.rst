Here is your **EXTENSIVE README** for **Neural Studio** — written as a polished, production-grade project introduction suitable for GitHub, investors, developers, and contributors.

If you want, I can also generate a **Sleek Logo**, **Badge Set**, **Folder Structure**, or **Marketing Website Copy**.

---

# **Neural Studio**

### *Next-Generation 3D + VR Broadcasting Suite*

*The successor to OBS — built for spatial computing, AI-native workflows, and immersive livestreaming.*

Neural Studio is a **VR-first**, **3D-native**, **AI-integrated broadcasting platform** designed for next-generation creators.
Where OBS assumes a 2D screen with flat layers, Neural Studio builds a **true 3D SceneGraph**, a **node-based Blueprint editor**, and a **VR spatial workflow** capable of producing 180°/360° immersive livestreams using modern protocols like **WHIP, WebRTC, SRT, and RTMP**.

Neural Studio is modular, GPU-accelerated, AI-powered and architected for Ubuntu, Vulkan, WebAssembly, and multi-format immersive streaming up to **8K VR**.

---

# **Table of Contents**

1. [Overview](#overview)
2. [Key Features](#key-features)
3. [Dual-View UI System](#dual-view-ui-system)
4. [Modules & Managers](#modules--managers)
5. [3D & Spatial Pipeline](#3d--spatial-pipeline)
6. [AI Integration](#ai-integration)
7. [Architecture](#architecture)
8. [Technology Stack](#technology-stack)
9. [Roadmap](#roadmap)
10. [Philosophy](#philosophy)

---

# **Overview**

Neural Studio rethinks content creation for the era of XR, spatial computing, AI assistants, and high-bandwidth WebRTC broadcasting.

Instead of flat 2D layers, Neural Studio uses:

✔ A **Spatial SceneGraph**
✔ A **3D Node Blueprint Editor**
✔ A **Spatial Renderer** able to composite 3D + Video + Audio in real time
✔ A **Neural Engine** capable of AI assistance, prediction and content-aware automation

Neural Studio supports:

* **3D cameras** (virtual, physical, stereoscopic)
* **180° and 360° VR livestreaming**
* **multi-view VR previews**
* **spatial audio mixing**
* **3D models, volumetric video, and photogrammetry assets**
* **AI-driven workflows**
* **WASM plugin runtime**
* **Vulkan compute video processing**
* **8K VR encoding**

---

# **Key Features**

## 🎥 **Next-Gen 3D Broadcasting**

* Full 3D SceneGraph (like Unreal Engine, but streaming-optimized)
* Compose scenes with 3D models, volumetric captures, video, cameras, and procedural assets
* Spatial transforms (position/rotation/scale in world space)
* Multi-camera VR support with real-time switching and transitions

## 🧩 **Dual Interface System**

Neural Studio offers two synchronized views:

### **1. ActiveFrame (Classic Action View)**

A clean operator dashboard optimized for live streaming.

* Wayland-accelerated UI (using Niri or similar tiling system)
* Source + Preview modules for:

  * Video
  * Cameras
  * 3D
  * Photos
  * Incoming Streams
  * Audio
  * Scripts
* SceneGraph Inspector / VirtualCam LayerMixer
* Control Buttons (Start Stream, Record, Studio Mode, etc.)
* Macro/Automation Hotkeys
* Multiview display

### **2. BlueprintFrame (Node-Based Creation View)**

A visual editor inspired by Unreal Engine’s Blueprint Editor.

* Drag & drop nodes for:

  * Cameras
  * Video processors
  * 3D assets
  * Filters
  * Transitions
  * Audio processors
  * ML/AI models
  * Streaming outputs
  * Scripted behaviors
  * Automation flows
* 3D spatial node placement (QtQuick3D)
* ONNX and AI operator nodes
* WASM plugin nodes
* 3D gizmos for transform editing

---

# **Modules & Managers**

Each module exists in both **ActiveFrame** and **BlueprintFrame**, but with different purposes.

---

## **ActiveFrame Modules (Live Operation)**

### **Source & Preview Video Module**

Live view of video feeds, clips, media sources, or screen captures.

### **Source & Preview Cam Module**

Handles:

* Stereoscopic cameras
* VR head cameras
* Depth cameras
* Virtual cameras inside the 3D SceneGraph

### **Source & Preview 3D Module**

Real-time preview of 3D models, scenes, animations, skeletal rigs, particles.

### **Source & Preview Photo Module**

Static images, textures, HDRi environments.

### **Source & Preview Incoming Stream Module**

WHIP, SRT, RTMP inputs.

### **Source & Preview Audio Module**

Waveforms, spectrograms, spatial audio previews.

### **Source & Preview Script Module**

Script triggers, automation states, event listeners.

### **SceneGraph / VirtualCam / LayerMixer**

Real-time compositor with:

* World layers
* Depth-based compositing
* VR projection modes
* Virtual camera feeds

### **Control Buttons**

Start/Stop streaming, recording, transitions, macro triggers.

### **Macros**

Automated actions based on events, states, AI cues, or sensor inputs.

---

## **BlueprintFrame Managers (Creation & Configuration)**

Each Manager acts as a Node Factory plus Inspector panel.

* **Manager Cameras**
  Virtual cams, 3D cams, stereoscopic rigs, tracking behaviors.

* **Manager Video**
  Video decoding, video effects, LUTs, transforms.

* **Manager Photo**
  Static textures, skyboxes, HDR lighting.

* **Manager Audio**
  Spatial audio nodes, filters, procedural audio.

* **Manager Script**
  Node-based scripting + Code editor.

* **Manager 3D Assets**
  GLTF/GLB/USD import, rigging, animations.

* **Manager Transitions**
  Classic fades + fully 3D spatial transitions (portals, flythroughs).

* **Manager Effects**
  Post-processing (bloom, DOF, SSAO), procedural particle effects.

* **Manager Filters**
  Audio filters, video filters, visual masks, AI segmenters.

* **Manager ML**
  ONNX models for detection, segmentation, upscaling.

* **Manager AI**
  LLM nodes, prompt nodes, AI prediction nodes.

* **Manager Broadcasting**
  RTMP, WHIP, WebRTC, SRT, Multi-output.

* **Manager Incoming Streams**
  External VR feeds, remote collaborators, event-based triggers.

---

# **3D & Spatial Pipeline**

Neural Studio is built from the ground up for spatial media.

### **Spatial Rendering**

* Vulkan rendering
* Vulkan compute video processing
* HDR support
* 180° & 360° reprojection
* Stereoscopic output (SBS/TAB)

### **Spatial Compositing**

* Depth-aware blending
* Volumetric layers
* Parallax mapping
* Shader-based transforms

### **Spatial Audio**

* Ambisonics (1st–3rd order)
* HRTF spatialization
* Audio anchored to 3D nodes
* Doppler and distance attenuation

---

# **AI Integration**

Neural Studio is designed as an AI-native application.

### **AI Assistant Dock**

* Built-in LLM (Gemini) with:

  * Scene management commands
  * Effect editing
  * Voice control
  * "Fix my audio" one-click repairs
  * "Improve lighting" scene adjustments
  * Automated stream moderation

### **AI Prediction Engine**

Predicts:

* Which nodes you may add next
* Automations you might need
* Scenes you are likely to switch to

### **AI Content-Aware Pipeline**

* Background removal
* Face tracking
* Pose estimation
* Scene segmentation
* Auto-framing for VR cameras
* Real-time relighting
* Color grading suggestions

---

# **Architecture**

(INCLUDED EXACTLY AS YOU PROVIDED — polished)

*(Mermaid diagram preserved and cleaned)*

```mermaid
graph TD
    subgraph "Frontend Layer (User Space)"
        Studio[Neural Studio (Qt6)]
        Studio --> ActiveView[ActiveFrame (Control Dashboard)]
        Studio --> Blueprint[BlueprintFrame (3D Node Editor)]

        Studio --> Docks[Extension Docks]
        Docks --> AIChat[AI Assistant]
        Docks --> StreamChat[Audience Chat]
    end

    subgraph "Inter-Process Communication"
        IPC[Shared Memory / Unix Domain Sockets]
        Studio <-->|Commands & State| IPC
        IPC <-->|Status & Preview Frames| Engine
    end

    subgraph "Backend Engine Layer (System Space)"
        Engine[libneural]
        
        Engine --> Pipeline[Spatial Video Pipeline]
        Pipeline --> Sources[Inputs (Cam/Media/3D)]
        Pipeline --> Filters[Processing]
        Pipeline --> Encoders[Encoding (NVENC/Vulkan/x264)]
        Pipeline --> Outputs[Streaming (RTMP/WHIP/SRT)]

        Engine --> PluginSys[Plugin System]
        PluginSys --> Native[Native Plugins (.so)]
        PluginSys --> WASM[WASM Plugins (Wasmtime)]

        Engine --> ML[AI Subsystem]
        ML --> ONNX[Local Inference (ONNX)]
        ML --> Gemini[Cloud Integration]
    end
```

---

# **Technology Stack**

### Frontend (vr_studio)

* Qt 6.8+ (Widgets + Quick + Quick3D)
* QML for 3D UI
* C++20 (logic)
* Wayland-native (Niri / wlroots)
* WebSockets for remote control

### Backend (libneural)

* C++20
* Vulkan (rendering + compute)
* FFmpeg (encoding/decoding)
* ONNX Runtime (local ML)
* Wasmtime (WASM plugins)
* PipeWire/ALSA (audio)

---

# **Roadmap**

## Phase 1 — DONE / NEAR DONE

✔ Architecture
✔ DualView Shell
✔ 3D Node Graph foundation
✔ Ray casting & object selection

## Phase 2 — In Progress

▫ Spatial gizmos
▫ Preview rendering
▫ C++ node subclasses

## Phase 3 — Engine

▫ WASM plugin host
▫ Vulkan compute pipeline
▫ Spatial audio mixer

## Phase 4 — AI & Polish

▫ Voice control
▫ AI Auto-setup SceneWizard
▫ UX polish

---

# **Philosophy**

**Neural Studio is built on five principles:**

1. **Spatial First – Not retrofitted 3D, but native 3D workflows**
2. **AI Native – The assistant is part of the workflow, not an add-on**
3. **Modular – Every unit is a node; everything can be rewired**
4. **Open – Plugins are sandboxed and safe (WASM)**
5. **Creator-Focused – Fast, predictable, joyful**

---

If you'd like, I can also generate:

✅ A **short README** version
✅ A **marketing page** version
✅ A **logo + color palette**
✅ A **full developer documentation site**
Just tell me!


> **The Next-Gen Agentic VR Streaming Engine.**

**VRobs-studio** (formerly OBS-RTX-VR-SuperStream) is a high-performance, graph-driven **Spatial Video Broadcasting Engine**. Designed specifically for the immersive web, it transforms standard setups into dedicated VR180/360 processing workstations.

It moves beyond 2D broadcasting by combining raw C++20 Vulkan rendering with agentic AI workflows and a secure, WebAssembly-based plugin architecture to deliver pristine 8K+ Spatial Video.

## 🏗 Architecture

VRobs-studio moves away from the monolithic architecture of traditional OBS tools. Instead, it employs a modular, node-based design where every component is an independent agent or service.

```mermaid
graph TD
    UI[Frontend (Qt6/QML)] <--> |IPC/PipeWire| Core[Core Engine (libvr)]
    Core <--> |Wasmtime| WASM[WASM Plugins]
    Core <--> |Vulkan| Graph[Render Graph]
    Core <--> |ONNX/Gemini| ML[ML Brain]
    
    subgraph "libvr (C++20)"
        Graph --> |Draw| Output[VR Output / Stream]
        ML --> |Predict| Workflow[Workflow Engine]
    end

    subgraph "Extensions"
        WASM --> |Sandboxed| Sources[Custom Sources]
        WASM --> |Sandboxed| Filters[Audio/Video Filters]
    end
```

### Core Components

#### 1. Core Engine (`libvr`)
The high-performance heart of the system.
- **Technology**: C++20, Vulkan, OpenXR.
- **Responsibility**: Manages the render graph, audio mixing, and hardware resource interactions.
- **Rendering**: Custom Vulkan engine optimized for VR180/360 stitching and high-resolution (8K+) throughput.

#### 2. The ML Brain
An integrated AI system that understands content and user intent.
- **Workflow Prediction**: Learns from user actions to suggest the next logical node in a graph.
- **Media Understanding**: Uses ONNX Runtime (Edge) and Google Gemini (Cloud) to analyze video content in real-time.
- **Agentic Helper**: Capable of generating assets or modifying scene configurations autonomously.

#### 3. Plugin System
A dual-architecture plugin system for maximum flexibility and security.
- **Native Plugins**: Traditional C/C++ shared custom plugins (e.g., NVIDIA Broadcast integration).
- **WASM Plugins**: **[NEW]** Secure, sandboxed plugins built on the WebAssembly Component Model. Supports plugins written in Rust, C, or any WASM-compatible language, run via the **Wasmtime** runtime.

### 4. Dual View Frontend (`vr_studio`)
A unified Qt6 application with two distinct modes:

*   **Active Studio View**: The live dashboard for mixing scenes, managing audio, and monitoring feeds.
*   **Blueprint Node View**: A 3D node-graph editor for constructing complex spatial pipelines.

#### New Integration Modules
*   **AI Command Center**: Integrated Chat Dock for talking to Gemini/local agents.
*   **Stream Interaction**: Real-time chat dock for RTMP/WHIP audience interaction.
*   **Qt6 Tech**: Uses `Qt Quick 3D` for the graph, `Qt State Machine` logic (simplified), and `WebSockets`.

## 🚀 Tech Stack

| Component | Technology | Description |
|-----------|------------|-------------|
| **Language** | C++20 | Core logic, relying on modern features (concepts, modules). |
| **Language** | Rust | Preferred for secure WASM plugin development. |
| **Graphics** | Vulkan 1.3 | Low-overhead graphics API for rendering. |
| **XR** | OpenXR | Standardized VR/AR device integration. |
| **UI** | Qt6 / QML | Hardware-accelerated user interface. |
| **ML Runtime** | OnnxRuntime | Efficient local inference. |
| **WASM Runtime** | Wasmtime | Robust WebAssembly runtime for plugins. |
| **Build** | CMake 3.28+ | Cross-platform build configuration. |

## 🛠️ Build Instructions

### Prerequisites
- **Linux** (Primary development platform) or Windows.
- **C++ Compiler**: GCC 13+ or Clang 16+ (C++20 support required).
- **CMake**: Version 3.28 or later.
- **Dependencies**: `libcurl`, `vulkan-sdk`, `libobs-dev` (legacy compat), `qt6-base-dev`.

### Building
```bash
# 1. Clone the repository
git clone https://github.com/jknohr/VRobs-studio.git
cd VRobs-studio

# 2. Configure (Fetches Wasmtime & other deps)
cmake -S . -B build -DENABLE_PLUGINS=ON

# 3. Build
cmake --build build --parallel

# 4. Run
./build/obs-vr/obs-vr
```

### Building WASM Plugins
To build a Rust plugin (e.g., `plugins/wasm-sample`):
```bash
# Add WASM target
rustup target add wasm32-wasip1

# Build
cd plugins/wasm-sample
cargo build --target wasm32-wasip1 --release
```

## 🗺️ Roadmap

- [x] **Repository Cleanup**: Removal of legacy MacOS/Metal code.
- [x] **WASM Integration**: Initial `libvr` integration with Wasmtime.
- [ ] **ML Agent**: Full implementation of the Gemini-powered assistant.
- [ ] **VR180 Stitching**: Porting CUDA stitchers to compute shaders (Vulkan).
- [ ] **Cloud Sync**: Workflow synchronization across devices.

## 📄 License
GPLv2 (Derived from OBS Studio). See `COPYING` for details.
