OBS-RTX-VR-SuperStream
======================

A High-Performance VR180 8K Streaming Engine for OBS Studio.

Transform your simple OBS setup into a dedicated VR180 processing workstation. Use the power of NVIDIA RTX 40/50 Series GPUs to stitch, process, and upscale Canon R5C (Dual Fisheye) input into a pristine 8K 60fps VR180 stream in real-time.

Overview
--------
This project repurposes the NVIDIA RTX Video Super Resolution SDK to provide a complete pipeline for VR Content Creators:

1.  **Input**: 4K Raw HDMI from Canon R5C (Dual Fisheye).
2.  **Stitch (Compute Shader)**: Real-time UV Remapping using STMaps to straighten fisheye to Equirectangular VR180.
3.  **Upscale (Tensor Core)**: NVIDIA AI Super Resolution (Mode 1 - High Quality) to upscale 4K Stitched -> 8K Final Output.
4.  **HDR**: Full Rec.2020 PQ/HLG support (Pass-through or Tone Map).

Key Features
------------
*   **8K Resolution Unlock**: Bypass previous 4K limits to stream at 8192x4320.
*   **VR180 Stitching**: Built-in efficient STMap shader. No need for heavy external stitching software.
*   **Custom Shader Support**: Load your own .effect / .hlsl files for custom lens corrections or color grading.
*   **HDR Ready**: Explicit support for Rec.2020 PQ and HLG workflows.
*   **RTX 50-Series Optimized**: Zero-copy VRAM optimization leveraging 24GB+ buffers on modern GPUs.

Supported Platforms
-------------------
*   **Windows**: Direct3D 11 (Standard OBS)
*   **Linux**: OpenGL / Vulkan (Verified on Ubuntu 22.04+ with NVIDIA Drivers). Uses Zero-Copy CUDA Interop for maximum performance.

Prerequisites
-------------
*   **GPU**: NVIDIA RTX 4090, 5090, or RTX 6000 Ada (24GB+ VRAM Recommended for 8K).
    *   Minimum: RTX 2060 (for basic filtering, not 8K VR).
*   **Drivers**: Latest NVIDIA Studio or Game Ready Drivers.
*   **Software**: OBS Studio 29.1.2 or higher.
*   **SDK**: NVIDIA Video Effects SDK (Required Download for both Windows and Linux).

Installation
------------

Linux (Manual Build)
~~~~~~~~~~~~~~~~~~~~

Since binaries are not yet distributed, you must build from source (Verified working):

1.  **Install Dependencies**:
    ::
        sudo apt-get install libobs-dev nvidia-cuda-toolkit build-essential cmake

2.  **Download NVIDIA SDK**: Extract the Linux Video Effects SDK and set the environment variable:
    ::
        export NV_VIDEO_EFFECTS_PATH=/path/to/NVIDIA_Video_Effects_SDK_Linux

3.  **Build**:
    ::
        cmake --preset linux-x64
        cmake --build --preset linux-x64

4.  **Install Plugin**:
    ::
        mkdir -p ~/.config/obs-studio/plugins/obs-rtx-superresolution/bin/64bit/
        mkdir -p ~/.config/obs-studio/plugins/obs-rtx-superresolution/data/

        cp build/linux-x64/rtx-superresolution.so ~/.config/obs-studio/plugins/obs-rtx-superresolution/bin/64bit/
        cp -r data/* ~/.config/obs-studio/plugins/obs-rtx-superresolution/data/

Credits
-------
Based on the OBS-RTX-SuperResolution project by Bemjo. Adapted and optimized for VR180 8K streaming by Subtomic.

Feature Implementation Status
-----------------------------

This project is a clean-room implementation of the OBS Studio core (`libvr`). We are systematically integrating features from the upstream `shared/` directory and re-implementing plugins natively.

**Core Engine (`libvr`)**

+-----------------------+---------------------+--------------------------------------------------------+
| Component             | Status              | Notes                                                  |
+=======================+=====================+========================================================+
| **Render Engine**     | 🟢 **Native**       | Vulkan-based. Supports 3D Scenes, Videos, Pictures.     |
+-----------------------+---------------------+--------------------------------------------------------+
| **Output**            | 🟢 **Native**       | Virtual Camera (V4L2) - 0-Copy Output.                  |
+-----------------------+---------------------+--------------------------------------------------------+
| **Audio**             | 🟢 **Native**       | Spatial Audio (Ambisonics ready).                       |
+-----------------------+---------------------+--------------------------------------------------------+
| **Input Sources**     | 🟢 **Native**       | PipeWire (Wayland), V4L2 (Webcam), Media Playback.     |
+-----------------------+---------------------+--------------------------------------------------------+
| `shared/bpm`          | 🟠 **Integrated**   | Metrics/SEI injection. (Uses compiled `bpm.c`)          |
+-----------------------+---------------------+--------------------------------------------------------+
| `shared/file-updater` | 🟡 **Stubbed/Link** | Mocked Network/JSON. Compiles, but no update logic.    |
+-----------------------+---------------------+--------------------------------------------------------+
| `shared/happy-eyeballs`| 🟠 **Integrated**  | Threading logic verified.                              |
+-----------------------+---------------------+--------------------------------------------------------+
| `shared/ipc-util`     | 🟡 **Stubbed**      | POSIX implementation missing upstream (TODO).          |
+-----------------------+---------------------+--------------------------------------------------------+
| `shared/media-playback`| 🟠 **Integrated**  | FFmpeg decoding linked. Semaphore impl added.          |
+-----------------------+---------------------+--------------------------------------------------------+
| `shared/obs-scripting`| 🟡 **Core Only**    | Logic linked. Python/Lua disabled (missing libs/SWIG). |
+-----------------------+---------------------+--------------------------------------------------------+
| `shared/opts-parser`  | 🟢 **Integrated**   | Fully functional.                                      |
+-----------------------+---------------------+--------------------------------------------------------+
| `shared/tiny-nv12`    | 🟢 **Integrated**   | Linked and available.                                  |
+-----------------------+---------------------+--------------------------------------------------------+
| `shared/shmem-queue`  | 🔴 **Windows Only** | Skipped. Relies on Win32 Handles.                      |
+-----------------------+---------------------+--------------------------------------------------------+
| **3D Lights/Shadows** | 🟢 **Implemented**  | SceneManager supports Point/Dir lights. Shader updated.|
+-----------------------+---------------------+--------------------------------------------------------+
| **.obj Mesh Loader**  | 🟢 **Implemented**  | Native C++ OBJ parser added. Standard Vertex format.   |
+-----------------------+---------------------+--------------------------------------------------------+

**Legend:**
*   🟢 **Native**: Fully implemented in C++20 for `libvr`.
*   🟠 **Integrated**: Upstream C code compiled and working with `libvr` compat layer.
*   🟡 **Stubbed**: Compiles and links, but functionality is mocked/disabled.
*   🔴 **Pending**: Not yet started.

