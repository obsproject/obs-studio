# ONNX Node

**Location**: `core/src/scene-graph/nodes/onnx/`, `ui/blueprint/nodes/OnnxNode/`
**Status**: ✅ Initial Implementation
**Version**: 1.0.0
**Date**: 2025-12-13

---

## Overview

The **ONNX Node** provides local Machine Learning interference capabilities using the ONNX Runtime. It allows executing standard `.onnx` models for tasks such as computer vision, audio analysis, or style transfer directly within the node graph.

### Key Features
- **Local Inference**: Privacy-focused, offline capable.
- **Hardware Acceleration**: Supports CPU and GPU (CUDA/TensorRT) execution providers.
- **Broad Compatibility**: Runs models from PyTorch, TensorFlow, etc., exported to ONNX.

---

## Architecture

### Backend (`OnnxNode.h/cpp`)
- **Inheritance**: `AINodeBase` -> `IExecutableNode`
- **Responsibilities**:
  - Managing `Ort::Session` and `Ort::Env`.
  - Loading models from `modelPath`.
  - Converting input tensors (Images/Audio) to ONNX tensors.
  - Running inference.
  - Converting output tensors back to Scene Graph types.

### Frontend (`OnnxNodeController` + `OnnxNode.qml`)
- **Controller**: Manages `modelPath` and `useGpu`.
- **UI**: 
  - Path input for `.onnx` model.
  - Checkbox for "Enable GPU Acceleration".
  - Base Node styling.

---

## Configuration

| Property | Type | Description |
|----------|------|-------------|
| `modelPath` | String | Path to the `.onnx` model file. |
| `useGpu` | Boolean | If true, attempts to use CUDA execution provider. |

---

## Usage

1. **Add Node**: Drag "ONNX Runtime" from the library.
2. **Select Model**: Point to your `yolov8.onnx` or similar.
3. **Enable GPU**: Check the box if you have a compatible NVIDIA GPU.
4. **Connect**: Link Video Output from a Camera to the Input of the ONNX node.

---

## Dependencies
- **ONNX Runtime** (C++ API)
- **CUDA Toolkit** (Optional, for GPU support)
