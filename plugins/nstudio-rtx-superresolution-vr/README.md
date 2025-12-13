# OBS-RTX-VR-SuperStream

**A High-Performance VR180 8K Streaming Engine for OBS Studio**

Transform your simple OBS setup into a dedicated VR180 processing workstation. Use the power of NVIDIA RTX 40/50 Series GPUs to stitch, process, and upscale Canon R5C (Dual Fisheye) input into a pristine 8K 60fps VR180 stream in real-time.

## Overview
This plugin repurposes the NVIDIA RTX Video Super Resolution SDK to provide a complete pipeline for VR Content Creators:
1.  **Input**: 4K Raw HDMI from Canon R5C (Dual Fisheye).
2.  **Stitch (Compute Shader)**: Real-time UV Remapping using STMaps to straighten fisheye to Equirectangular VR180.
3.  **Upscale (Tensor Core)**: NVIDIA AI Super Resolution (Mode 1 - High Quality) to upscale 4K Stitched $\to$ 8K Final Output.
4.  **HDR**: Full Rec.2020 PQ/HLG support (Pass-through or Tone Map).

## Key Features
*   **8K Resolution Unlock**: Bypass previous 4K limits to stream at **8192x4320**.
*   **VR180 Stitching**: Built-in efficient STMap shader. No need for heavy external stitching software.
*   **Custom Shader Support**: Load your own `.effect` / `.hlsl` files for custom lens corrections or color grading.
*   **HDR Ready**: Explicit support for Rec.2020 PQ and HLG workflows.
*   **RTX 50-Series Optimized**: Zero-copy VRAM optimization leveraging 24GB+ buffers on modern GPUs.

## Supported Platforms
- **Windows**: Direct3D 11 (Standard OBS)
- **Linux**: OpenGL (Verified on Ubuntu 22.04+ with NVIDIA Drivers). Uses **Zero-Copy CUDA-GL Interop** for maximum performance.

## Prerequisites
*   **GPU**: NVIDIA RTX 4090, 5090, or RTX 6000 Ada (24GB+ VRAM Recommended for 8K).
    *   Minimum: RTX 2060 (for basic filtering, not 8K VR).
*   **Drivers**: Latest NVIDIA Studio or Game Ready Drivers.
*   **Software**: OBS Studio 29.1.2 or higher.
*   **SDK**: [NVIDIA Video Effects SDK](https://www.nvidia.com/en-us/geforce/broadcasting/broadcast-sdk/resources/) (Required Download for both Windows and Linux).

## Installation

### Linux (Manual Build)
Since binaries are not yet distributed, you must build from source (Verified working):
1.  **Install Dependencies**:
    ```bash
    sudo apt-get install libobs-dev nvidia-cuda-toolkit build-essential cmake
    ```
2.  **Download NVIDIA SDK**: Extract the Linux Video Effects SDK and set the environment variable:
    ```bash
    export NV_VIDEO_EFFECTS_PATH=/path/to/NVIDIA_Video_Effects_SDK_Linux
    ```
3.  **Build**:
    ```bash
    cmake --preset linux-x64
    cmake --build --preset linux-x64
    ```
4.  **Install Plugin**:
    ```bash
    mkdir -p ~/.config/obs-studio/plugins/obs-rtx-superresolution/bin/64bit/
    mkdir -p ~/.config/obs-studio/plugins/obs-rtx-superresolution/data/
    
    cp build/linux-x64/rtx-superresolution.so ~/.config/obs-studio/plugins/obs-rtx-superresolution/bin/64bit/
    cp -r data/* ~/.config/obs-studio/plugins/obs-rtx-superresolution/data/
    ```

### Windows (Manual)
1.  Download the latest release.
2.  Unzip the files into your OBS Studio installation directory (e.g., `C:/Program Files/obs-studio/`).
3.  Ensure the `data/obs-plugins/obs-super-resolution/` folder contains `stmap_stitch.effect`.

## Usage Guide
1.  **Add Filter**: Right-click your Video Capture Device > Filters > **NVIDIA Super Resolution**.
2.  **Enable Stitching** (VR180): 
    *   Toggle **"Enable Stitching (VR180)"**.
    *   **STMap Path**: Browse and select your Canon R5C STMap (`.tiff` or `.png`).
3.  **Configure Upscaling**:
    *   **Filter Type**: Super Resolution.
    *   **Scale**: **2.0x** (Input 4K $\to$ Output 8K).
    *   **Mode**: **High Quality / Lossless (Mode 1)**.
    *   **Upscale Target**: Select "2.0x" or "Pass-Through" if you only want stitching.
4.  **Custom Shaders (Optional)**:
    *   If you have a custom HLSL effect, select it in "Custom Stitching Shader" to override the default stitcher.

## Limitations & Notes
*   **HDR on Linux**: Rec.2100 PQ/HLG support is currently disabled in the build configuration if using older `libobs-dev` (v30.0.2). To enable, compile OBS from source and uncomment the HDR blocks in `src/nvidia-superresolution-filter.c`.
*   **8K Requirements**: Processing 8K 60fps requires significant VRAM (~3GB per instance) and compute. Ensure no other VRAM-heavy apps are running.

## Original Credits
Based on the [OBS-RTX-SuperResolution](https://github.com/Bemjo/OBS-RTX-SuperResolution-Gallery) project by Bemjo.
Adapted and optimized for VR180 8K streaming by Subtomic.
