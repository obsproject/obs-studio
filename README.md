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

* ✔ A **Spatial SceneGraph**
* ✔ A **3D Node Blueprint Editor**
* ✔ A **Spatial Renderer** able to composite 3D + Video + Audio in real time
* ✔ A **Neural Engine** capable of AI assistance, prediction and content-aware automation

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

Neural Studio employs a modular, node-based design where every component is an independent agent or service.

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

# **Build Instructions**

### Prerequisites
- **Linux** (Primary development platform) or Windows.
- **C++ Compiler**: GCC 13+ or Clang 16+ (C++20 support required).
- **CMake**: Version 3.28 or later.
- **Dependencies**: `libcurl`, `vulkan-sdk`, `libobs-dev` (legacy compat), `qt6-base-dev`.

### Building
```bash
# 1. Clone the repository
git clone https://github.com/jknohr/Neural-Studio.git
cd Neural-Studio

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

---

# **License**
GPLv2 (Derived from OBS Studio). See `COPYING` for details.
