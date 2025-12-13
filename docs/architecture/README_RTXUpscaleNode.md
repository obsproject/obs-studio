# RTX Upscale Node

**Location**: `core/src/scene-graph/nodes/RTXUpscaleNode.*`, `ui/blueprint/nodes/RTXUpscaleNode/`  
**Status**: ✅ Implemented (Wrapper)  
**Version**: 1.0.0  
**Category**: AI / Enhancement  

---

## Overview

The **RTX Upscale Node** leverages NVIDIA's Maxine SDK (Video Effects) to provide AI-powered Super Resolution. It can upscale video usage (e.g., 4K input to 8K output) with significantly higher quality than bicubic interpolation, reconstructing details using deep learning models specialized for video content.

### Key Capabilities
- ✅ **AI Super Resolution**: 2x, 3x, 4x upscaling.
- ✅ **Maxine Integration**: Wraps the `RTXUpscaler` subsystem.
- ✅ **Quality Modes**: Select between "Performance" and "High Quality" models.
- ✅ **Hardware Check**: Validates RTX GPU availability.

---

## Architecture

### Backend: `RTXUpscaleNode`
Inherits from `BaseNodeBackend`.

- **Inputs**:
  - `video_in` (`Texture2D`): Input low-res texture.
- **Outputs**:
  - `video_out` (`Texture2D`): Output high-res texture.
- **Properties**:
  - `scale_factor`: Integer (1-4).
  - `quality_mode`: Enum/Int (0=Performance, 1=Quality).

**Logic flow**:
1. Validate `RTXUpscaler` is initialized.
2. In `process()`, submit `video_in` to the upscaler queue.
3. Retrieve `video_out` from upscaler (potentially async, though graph currently processes sequentially).
4. `setOutputData("video_out", result)`.

### UI: `RTXUpscaleNode.qml` & `RTXUpscaleNodeController`
Inherits from `BaseNode` / `BaseNodeController`.

- **Visuals**: NVIDIA Green header (AI category).
- **Controls**: 
  - Scale Factor SpinBox.
  - Quality Mode ComboBox.

---

## Dependencies
- **`RTXUpscaler`**: Core wrapper around NVIDIA Maxine SDK.
- **CUDA / Vulkan Interop**: Required for sharing textures between rendering engine and AI inference engine.

---

## TODOs
- [ ] Verify CUDA-Vulkan semaphore synchronization in `process()`.
- [ ] Add "Artifact Reduction" toggle (if supported by SDK).
- [ ] Handle VRAM exhaustion gracefully.
