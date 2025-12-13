# Gemini AI Node

**Location**: `core/src/scene-graph/nodes/gemini/`, `ui/blueprint/nodes/GeminiNode/`
**Status**: ✅ Initial Implementation
**Version**: 1.0.0
**Date**: 2025-12-13

---

## Overview

The **Gemini Node** integrates Google's Gemini 2.0 API for cloud-based multimodal AI processing. It enables the use of powerful Large Language Models (LLMs) and Vision Models (VLMs) for understanding video, generating text, or complex reasoning tasks that exceed local hardware capabilities.

### Key Features
- **Multimodal Input**: Accepts Text, Image, and Video inputs.
- **Cloud Power**: Access to "Pro", "Flash", and "Ultra" models.
- **Reasoning**: Capable of description, summarization, and creative generation.

---

## Architecture

### Backend (`GeminiNode.h/cpp`)
- **Inheritance**: `AINodeBase` -> `IExecutableNode`
- **Responsibilities**:
  - Secure management of API Key.
  - Constructing HTTP/gRPC requests to Gemini API.
  - Streaming responses (optional) or blocking data processing.
  - Handling rate limits and connectivity errors.

### Frontend (`GeminiNodeController` + `GeminiNode.qml`)
- **Controller**: Manages `apiKey` and `modelName`.
- **UI**: 
  - Password field for API Key.
  - Dropdown for model selection (Flash/Pro).
  - Inherits Base Node structure.

---

## Configuration

| Property | Type | Description |
|----------|------|-------------|
| `apiKey` | String | Google Cloud API Key for Gemini. |
| `modelName` | String | Model identifier (e.g., `gemini-1.5-flash`, `gemini-1.5-pro`). |

---

## Security Note

API Keys are stored in memory and passed to the backend. Ensure you do not share project files containing sensitive keys. Future versions will support environment variable binding.

---

## Dependencies
- **libcurl** (or Qt Network) for API requests.
- **nlohmann/json** for payload formatting.
