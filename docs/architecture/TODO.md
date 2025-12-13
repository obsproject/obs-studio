Neural Studio Phase 2 - Implementation TODO
Last Updated: 2025-12-13
Status: Ready to Start
Total Tasks: 47 across 5 priority streams

📋 CORE BACKEND TASKS
Priority 0: Stereo Rendering Pipeline (8 tasks)
Goal: Implement VR180/360 stereo rendering with Vulkan

- [x] **Task 1.1**: Create Vulkan Renderer Foundation
  - Files: `core/src/rendering/VulkanRenderer.cpp/h`
  - Setup Vulkan device, queues, command buffers
  - Implement basic render loop
  - **Effort**: 3 days
  - **Status**: ✅ COMPLETE

- [x] **Task 1.2**: Implement Stereo Renderer
  - Files: `core/src/rendering/StereoRenderer.cpp/h`
  - Dual-pass rendering (left/right eyes)
  - IPD offset calculation
  - Per-eye framebuffer management
  - SBS + Dual-Stream input support (AV2 ready)
  - **Depends on**: 1.1
  - **Effort**: 4 days
  - **Status**: ✅ COMPLETE

- [x] **Task 1.3**: Create Framebuffer Manager
  - Files: `core/src/rendering/FramebufferManager.cpp/h`
  - Allocate per-profile framebuffers (Quest 3, Index, Vive Pro 2)
  - Resolution/format management
  - **Triple buffering** (CPU write, GPU render, display read)
  - **Depends on**: 1.1
  - **Effort**: 2 days
  - **Status**: ✅ COMPLETE
- [x] **Task 1.4**: Port STMap Stitching Shader
  - Files: `core/src/rendering/shaders/stitch.comp` (Vulkan GLSL)
  - Convert from `nstudio-rtx-superresolution-vr/data/stmap_stitch.effect`
  - UV remap compute shader
  - TIFF texture support
  - **Depends on**: 1.1
  - **Effort**: 2 days
  - **Status**: ✅ COMPLETE
- [x] **Task 1.5**: Implement STMap Loader
  - Files: `core/src/rendering/STMapLoader.cpp/h`
  - Load TIFF/PNG UV remap textures
  - Upload to Vulkan
  - Per-lens calibration support
  - **Depends on**: 1.4
  - **Effort**: 1 day
  - **Status**: ✅ COMPLETE
- [x] **Task 1.6**: Create Preview Renderer
  - Files: `core/src/rendering/PreviewRenderer.cpp/h`
  - Low-res preview for Blueprint/ActiveFrame
  - Real-time 3D viewport
  - Monoscopic/SBS/Anaglyph preview modes
  - **Depends on**: 1.2
  - **Effort**: 2 days
  - **Status**: ✅ COMPLETE
- [x] **Task 1.7**: Integrate RTX AI Upscaling (Optional)
  - Files: `core/src/rendering/RTXUpscaler.cpp/h`
  - NVIDIA Maxine SDK integration (Ubuntu/Linux)
  - 4K→8K AI upscaling via Tensor Cores
  - Blueprint node with adjustable quality/scale
  - **Depends on**: 1.2
  - **Effort**: 3 days
  - **Status**: ✅ COMPLETE (Skeleton with TODOs for CUDA-Vulkan interop)
- [x] **Task 1.8**: Create HDR Processor (Optional)
  - Files: `core/src/rendering/HDRProcessor.cpp/h`, `shaders/hdr_tonemap.comp`
  - Rec.709 ↔ Rec.2020 color space conversion
  - PQ/HLG EOTF support
  - ACES/Reinhard/Hable tone mapping
  - Blueprint node with exposure/luminance controls
  - **Depends on**: 1.2
  - **Effort**: 2 days
  - **Status**: ✅ COMPLETE (Full shader implementation)
Total Effort: ~19 days

Priority 0: Video Capture Integration (5 tasks)
Goal: Stereo camera capture with low-latency pass-through

 Task 2.1: Implement Qt Multimedia Capture

Files: core/src/video/QtMultimediaCapture.cpp/h
Dual camera input (L/R)
Format negotiation (4K/8K)
Frame callbacks
Effort: 3 days
 Task 2.2: Create Stereo Texture Mapper

Files: core/src/video/StereoTextureMapper.cpp/h
Map video frames to Vulkan textures
DMA-BUF zero-copy (Linux)
Stereo pair management
Depends on: 2.1, 1.1
Effort: 3 days
 Task 2.3: Implement Video Plane 3D

Files: core/src/video/VideoPlane3D.cpp/h
Position video at Z=-5m
3D plane mesh generation
Texture coordinate mapping
Depends on: 2.2
Effort: 2 days
 Task 2.4: Low-Latency Pass-Through

Files: core/src/video/LowLatencyPassthrough.cpp/h
Direct camera→display path
Minimize frame buffering
Jitter buffer (< 50ms latency)
Depends on: 2.1, 2.2
Effort: 2 days
 Task 2.5: Video Source Registry

Files: core/src/video/VideoSourceRegistry.cpp/h
Enumerate available cameras
Device capability query
Hot-plug detection
Depends on: 2.1
Effort: 1 day
Total Effort: ~11 days

Priority 0: Scene Graph Backend (4 tasks)
Goal: C++ node execution engine

- [x] **Task 3.1**: Create Node Execution Graph
  - Files: `core/src/scene-graph/NodeExecutionGraph.cpp/h`, `IExecutableNode.h/cpp`, `ProcessingNode.h/cpp`
  - Topological sort for execution order
  - Dependency resolution
  - Cycle detection
  - Plugin architecture (Factory + Registry)
  - Rich type system (8 data categories)
  - Smart caching
  - Parallel execution
  - Backend nodes: StitchNode, RTXUpscaleNode, HeadsetOutputNode
  - UI wrappers: QML visual components + C++ controllers
  - **Effort**: 3 days
  - **Status**: ✅ COMPLETE (Full stack: backend + UI)
  
- [ ] **Task 3.2**: Implement SceneGraph Engine
  - Files: `core/src/scene-graph/SceneGraphEngine.cpp/h`
  - 3D spatial hierarchy
  - Transform propagation
  - Bounding volume hierarchy
  - **Depends on**: 3.1
  - **Effort**: 4 days
  - **Status**: TODO
  
- [ ] **Task 3.3**: Complete C++ Node Backends
  - Files: `core/src/scene-graph/nodes/*.cpp`
  - CameraNode.cpp (header exists in ui/)
  - FilterNode.cpp
  - EffectNode.cpp
  - TransitionNode.cpp
  - LLMTranscriptionNode.cpp
  - **Depends on**: 3.1
  - **Effort**: 5 days (1 day per node)
  - **Status**: 3/7 complete (Stitch, RTXUpscale, HeadsetOutput done)
  
- [ ] **Task 3.4**: Wire QML to C++ Backend

Files: Update existing ui/blueprint/nodes/*/Node*.cpp
Property sync (QML properties → C++ backend)
Signal/slot connections
Data flow execution
Depends on: 3.3
Effort: 2 days
Total Effort: ~14 days

Priority 1: AI/ML Systems (4 tasks)
Goal: Workflow prediction and context compression

 Task 4.1: Workflow Prediction Engine

Files: core/ai/WorkflowPrediction.cpp/h
Learn successful node patterns
Suggest next nodes
History tracking
Effort: 4 days
 Task 4.2: Node Suggestion System

Files: core/ai/NodeSuggestion.cpp/h
Context-aware recommendations
Probability scoring
UI integration hooks
Depends on: 4.1
Effort: 2 days
 Task 4.3: Context Compression for AI Agents

Files: core/ai/ContextCompression.cpp/h
Compress scene graph for LLM
Semantic summarization
Token optimization
Effort: 3 days
 Task 4.4: Gemini Integration

Files: core/ai/GeminiIntegration.cpp/h
API client (REST/gRPC)
Prompt templates
Response parsing
Effort: 2 days
Total Effort: ~11 days

Priority 1: Deep Learning (2 tasks)
Goal: Workflow pattern recognition

 Task 5.1: Workflow Learning System

Files: core/dl/WorkflowLearning.cpp/h
Training pipeline
Pattern extraction
Model persistence
Effort: 5 days
 Task 5.2: Pattern Recognition

Files: core/dl/PatternRecognition.cpp/h
Inference engine
Real-time predictions
Confidence scoring
Depends on: 5.1
Effort: 3 days
Total Effort: ~8 days

🎨 UI FRONTEND TASKS
Priority 0: Blueprint Spatial Gizmos (3 tasks)
Goal: Transform manipulation in 3D space

 Task 6.1: Create Spatial Directory

Dir: ui/blueprint/spatial/
Effort: < 1 hour
 Task 6.2: Implement Transform Gizmos (QML)

Files:
ui/blueprint/spatial/TransformGizmo.qml
ui/blueprint/spatial/TranslateGizmo.qml
ui/blueprint/spatial/RotateGizmo.qml
ui/blueprint/spatial/ScaleGizmo.qml
Qt Quick 3D gizmo visuals
Mouse interaction
Axis constraints
Effort: 4 days
 Task 6.3: Implement Gizmo Controller (C++)

Files: ui/blueprint/spatial/GizmoController.cpp/h
Ray casting for selection
Transform calculation
Apply to scene graph nodes
Depends on: 6.2
Effort: 2 days
Total Effort: ~6 days

Priority 0: Blueprint Preview (3 tasks)
Goal: Real-time 3D scene preview in Blueprint view

 Task 7.1: Create Preview Directory

Dir: ui/blueprint/preview/
Effort: < 1 hour
 Task 7.2: Implement Scene Preview (QML)

Files:
ui/blueprint/preview/ScenePreview.qml
ui/blueprint/preview/StereoPreview.qml
Qt Quick 3D View3D
Camera controls (orbit, pan, zoom)
Stereo side-by-side mode
Effort: 3 days
 Task 7.3: Implement Preview Controller (C++)

Files: ui/blueprint/preview/PreviewController.cpp/h
Connect to core/src/rendering/PreviewRenderer
Frame updates
Interaction handling
Depends on: 7.2, 1.6
Effort: 2 days
Total Effort: ~5 days

Priority 0: Blueprint Managers (12 tasks)
Goal: Node creation panels for all 12 manager types

 Task 8.1: Create Managers Directory

Dir: ui/blueprint/managers/
Effort: < 1 hour
 Task 8.2-8.13: Implement 12 Managers Each manager has same structure:

ManagerName/ManagerName.qml (UI panel)
ManagerName/ManagerName.cpp/h (Controller)
Node palette/factory
Managers to create:

CameraManager
VideoManager
PhotoManager
AudioManager
ScriptManager
ThreeDAssetsManager
TransitionsManager
EffectsManager
FiltersManager
MLManager
AIManager
BroadcastingManager
IncomingStreamsManager (correction: 13 total)
Effort: 1 day per manager × 13 = 13 days

Total Effort: ~13 days

Priority 0: ActiveFrame Modules (10 tasks)
Goal: Live operation view with source/preview modules

 Task 9.1: Create Modules Directory

Dir: ui/activeui/modules/
Effort: < 1 hour
 Task 9.2-9.11: Implement 10 ActiveFrame Modules Each module QML component:

Source selector
Preview viewport
Controls
Modules to create:

VideoModule.qml
CamModule.qml
ThreeDModule.qml
PhotoModule.qml
IncomingStreamModule.qml
AudioModule.qml
ScriptModule.qml
VirtualCamModule.qml (SceneGraph/LayerMixer)
ControlButtons.qml
MacrosPanel.qml
Effort: 1.5 days per module × 10 = 15 days

Total Effort: ~15 days

Priority 1: Settings UI (1 task)
Goal: VR rendering settings (migrated from RTX plugin)

 Task 10.1: Create Rendering Settings
Dir: ui/settings/RenderingSettings/
Files:
RenderingSettings.qml (main panel)
RenderingSettings.cpp/h (backend)
panels/StitchingPanel.qml
panels/UpscalingPanel.qml
panels/HDRPanel.qml
panels/VulkanPanel.qml
Effort: 3 days
Total Effort: ~3 days

Priority 2: Shared VR Widgets (3 tasks)
Goal: Reusable 3D/VR UI widgets

 Task 11.1-11.3: Implement VR Widgets
ui/shared_ui/widgets/VRButton/ (QML + C++)
ui/shared_ui/widgets/VRSlider/ (QML + C++)
ui/shared_ui/widgets/VRFilePicker/ (QML + C++)
Effort: 1 day per widget × 3 = 3 days
Total Effort: ~3 days

Priority 2: Application Menus (1 task)
Goal: Top-level application menus

 Task 12.1: Create Application Menus
Dir: ui/global/menus/
Files: FileMenu.qml, EditMenu.qml, ViewMenu.qml, ToolsMenu.qml, HelpMenu.qml
Effort: 2 days
Total Effort: ~2 days

📊 SUMMARY
Core Backend
Stream	Tasks	Effort	Priority
Stereo Rendering	8	19 days	P0
Video Capture	5	11 days	P0
Scene Graph	4	14 days	P0
AI Systems	4	11 days	P1
Deep Learning	2	8 days	P1
Total	23	63 days	-
UI Frontend
Stream	Tasks	Effort	Priority
Spatial Gizmos	3	6 days	P0
Preview	3	5 days	P0
Managers	13	13 days	P0
ActiveFrame	10	15 days	P0
Settings	1	3 days	P1
VR Widgets	3	3 days	P2
Menus	1	2 days	P2
Total	34	47 days	-
Grand Total
47 tasks
110 days (single developer, sequential)
~22 weeks (with parallelization, 2-3 developers)
🚀 RECOMMENDED IMPLEMENTATION ORDER
Week 1-3: Core Rendering Foundation
Task 1.1: Vulkan Renderer
Task 1.2: Stereo Renderer
Task 1.3: Framebuffer Manager
Week 4-5: Video Capture
Task 2.1: Qt Multimedia Capture
Task 2.2: Stereo Texture Mapper
Task 2.3: Video Plane 3D
Week 6-7: STMap Stitching
Task 1.4: Port Stitching Shader
Task 1.5: STMap Loader
Week 8-9: Preview System
Task 1.6: Preview Renderer
Task 7.1-7.3: Blueprint Preview UI
Week 10-11: Spatial Gizmos
Task 6.1-6.3: Transform Gizmos
Week 12-14: Scene Graph Backend
Task 3.1: Node Execution Graph
Task 3.2: SceneGraph Engine
Task 3.3: C++ Node Backends
Week 15-17: Blueprint Managers
Task 8.1-8.13: All 12 managers
Week 18-20: ActiveFrame Modules
Task 9.1-9.11: All 10 modules
Week 21-22: Settings & Polish
Task 10.1: Rendering Settings
Tasks 1.7, 1.8: RTX Upscaling, HDR (optional)
✅ COMPLETION CRITERIA
Phase 2 is DONE when:

 Stereo rendering working (dual-pass L/R eyes)
 Video capture integrated (dual cameras)
 STMap stitching functional (fisheye→equirect)
 3D preview live in Blueprint
 Spatial gizmos working (translate/rotate/scale)
 All 12 Blueprint managers functional
 All 10 ActiveFrame modules operational
 C++ node backends complete (7 nodes)
 Settings UI for rendering features
 Can stream VR180 to Quest 3
Then move to Phase 3: WASM plugins, Vulkan compute, full AI integration