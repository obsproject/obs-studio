# Headset Output Node

**Location**: `core/src/scene-graph/nodes/HeadsetOutputNode.*`, `ui/blueprint/nodes/HeadsetOutputNode/`  
**Status**: ✅ Implemented (Wrapper)  
**Version**: 1.0.0  
**Category**: Output / VR  

---

## Overview

The **Headset Output Node** serves as the final destination for VR video pipelines. It interfaces with the `VirtualCamManager` to route processed frames to connected VR headsets (via SteamVR/OpenVR) or virtual camera drivers.

### Key Capabilities
- ✅ **Profile Switching**: Toggles between target device profiles (Quest 3, Index, Vive).
- ✅ **Streaming**: Pushes textures to the VR runtime.
- ✅ **Visual Feedback**: Indicates streaming status in UI.

---

## Architecture

### Backend: `HeadsetOutputNode`
Inherits from `BaseNodeBackend`.

- **Inputs**:
  - `video_in` (`Texture2D`): Final composed stereo frame (SBS or Top-Bottom).
- **Outputs**: None (Sink node).
- **Properties**:
  - `profile_id`: String ID of the target headset profile.

**Logic flow**:
1. Receive texture on `video_in`.
2. Retrieve `VirtualCamManager` instance.
3. Call `pushFrame(texture, profile_id)` (conceptually) to send data to the headset driver.

### UI: `HeadsetOutputNode.qml` & `HeadsetOutputNodeController`
Inherits from `BaseNode` / `BaseNodeController`.

- **Visuals**: Red header (Output category).
- **Controls**: 
  - Dropdown ComboBox for Profile selection (Quest 3, Index, etc.).
  - Connection status indicator.

---

## Dependencies
- **`VirtualCamManager`**: The core subsystem managing VR hardware connections and streams.
- **`FramebufferManager`**: Handles swapchains for specific headsets.

---

## TODOs
- [ ] Implement actual `pushFrame` logic connecting Node Execution to VirtualCamManager.
- [ ] Add properties for bitrate/latency/resolution overrides.
- [ ] Support audio channel output for VR headset headphones.
