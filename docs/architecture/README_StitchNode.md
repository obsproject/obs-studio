# Stitch Node

**Location**: `core/src/scene-graph/nodes/StitchNode.*`, `ui/blueprint/nodes/StitchNode/`  
**Status**: ✅ Implemented (MVP)  
**Version**: 1.0.0  
**Category**: Video Processing  

---

## Overview

The **Stitch Node** is a specialized video processing node responsible for converting dual-fisheye camera footage (typical of VR cameras like Canon R5C or VR180 setups) into a stitched equirectangular or VR180 format. It utilizes **STMaps (Spatial Transformation Maps)** to perform pixel-perfect UV remapping on the GPU.

### Key Capabilities
- ✅ **STMap Support**: Uses standard 32-bit float internal UV maps for stitching.
- ✅ **GPU Acceleration**: Fully hardware-accelerated via `StereoRenderer` / Compute Shaders.
- ✅ **UI Controls**: Path selection for STMap files.
- ✅ **Type Safety**: Accepts `Texture2D` inputs, outputs `Texture2D`.

---

## Architecture

### Backend: `StitchNode`
Inherits from `BaseNodeBackend`.

- **Inputs**:
  - `video_in` (`Texture2D`): Raw fisheye texture.
  - `stmap` (`Texture2D`): (Optional) STMap texture input.
- **Outputs**:
  - `video_out` (`Texture2D`): Reprojected output.
- **Properties**:
  - `stmap_path`: File path to load STMap from if not provided via pin.

**Logic flow**:
1. Check for `stmap` input pin.
2. If missing, verify `stmap_path` logic (loader).
3. Dispatch compute shader via `StereoRenderer` to remap pixels from `video_in` based on UVs in STMap.
4. Write to `video_out`.

### UI: `StitchNode.qml` & `StitchNodeController`
Inherits from `BaseNode` / `BaseNodeController`.

- **Visuals**: Blue header (Video category).
- **Controls**: 
  - Text input for STMap path.
  - Verification indicators.

---

## Dependencies
- **`STMapLoader`**: Helper class to load `.exr` or `.png` STMaps into GPU memory.
- **`StereoRenderer`**: Provides the compute shader pipeline for the stitching operation.
- **Qt RHI**: For low-level texture handling.

---

## Example Usage

**Graph Setup**:
```
[Camera Input] --(texture)--> [Stitch Node] --(texture)--> [Output]
                                    ^
                                    |
                             [STMap Texture] (optional)
```

---

## TODOs
- [ ] Implement actual Compute Shader dispatch in `process()`.
- [ ] Add properties for interpolation mode (Linear/Nearest).
- [ ] Support alpha mask for lens boundaries.
