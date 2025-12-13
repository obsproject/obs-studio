# LLM Manager

**Location**: `ui/shared_ui/managers/LLMManager/`, `ui/blueprint/nodes/LLMNode/`
**Status**: ✅ Implemented
**Version**: 1.0.0
**Date**: 2025-12-14

---

## Overview

The **LLM Manager** integrates Large Language Models into Neural Studio's workflow. It manages LLM providers (Gemini, OpenAI, Anthropic, etc.), API credentials, and enables AI-powered content generation and scene automation through LLMNodes.

### Key Features
- **Multi-Provider Support**: Gemini, OpenAI, Anthropic, Ollama (local)
- **API Management**: Secure credential storage
- **Multi-Node Tracking**: Multiple LLM nodes with different configurations
- **Preset Management**: Save prompt templates and model configs
- **Provider Branding**: Color-coded UI per provider
- **Hybrid UI**: QML and Qt Widgets interfaces

---

## Architecture

### Backend (`LLMManagerController.h/cpp`)
- **Inheritance**: `BaseManagerController` → `QObject`
- **Responsibilities**:
  - Managing LLM provider configurations
  - API credential storage (encrypted)
  - Tracking all LLMNode instances
  - Preset management for prompts and parameters
  - Provider-specific settings (temperature, top_p, etc.)

### Frontend Options

#### QML View (`LLMManager.qml`)
- **Purpose**: Blueprint Frame AI panel
- **Features**:
  - Provider selection with brand colors:
    - Gemini: Purple/Blue gradient
    - OpenAI: Green
    - Anthropic: Orange/Red
    - Ollama: Blue (local)
  - Model dropdown per provider
  - Preset prompt templates
  - Cost tracking (tokens/USD)
  - "In Use" indicators

#### Qt Widget (`LLMManagerWidget.h/cpp`)
- **Purpose**: Active Frame AI control panel
- **Features**:
  - Provider tabs with branding
  - API key management
  - Model selection
  - Usage statistics
  - Prompt library

---

## Interface

### Controller Properties
| Property | Type | Description |
|----------|------|-------------|
| `providers` | QVariantList | Available LLM providers (Gemini, OpenAI, etc.) |
| `models` | QVariantList | Models for selected provider |
| `managedNodes` | QStringList | Active LLMNode IDs |
| `nodePresets` | QVariantList | Saved prompt/config presets |
| `currentProvider` | QString | Selected provider name |

### Methods
| Method | Parameters | Description |
|--------|------------|-------------|
| `setApiKey()` | provider, apiKey | Store encrypted API key |
| `getApiKey()` | provider | Retrieve API key |
| `listModels()` | provider | Get available models |
| `createLLMNode()` | provider, model, x, y | Create LLMNode |
| `savePreset()` | nodeId, presetName | Save prompt template |
| `loadPreset()` | presetName, x, y | Create node from preset |
| `estimateCost()` | provider, tokens | Calculate API cost |

---

## Supported Providers

### Gemini (Google)
- **Color**: Purple/Blue gradient
- **Models**: gemini-1.5-pro, gemini-1.5-flash
- **Features**: Multimodal (text, image, video), function calling
- **API**: Google AI Studio / Vertex AI

### OpenAI
- **Color**: Green
- **Models**: gpt-4, gpt-3.5-turbo
- **Features**: Text, vision, embeddings
- **API**: OpenAI Platform

### Anthropic
- **Color**: Orange/Red
- **Models**: claude-3-opus, claude-3-sonnet
- **Features**: Long context, JSON mode
- **API**: Anthropic API

### Ollama (Local)
- **Color**: Blue
- **Models**: llama3, mistral, codellama
- **Features**: Fully local, no API costs
- **Runtime**: Local Ollama server

---

## Usage Example

### C++ (Controller)
```cpp
auto llmManager = new LLMManagerController(parent);

// Configure provider
llmManager->setApiKey("gemini", "your-api-key");

// Create LLM node
llmManager->createLLMNode("gemini", "gemini-1.5-pro", 300, 200);

// Save prompt template
llmManager->savePreset("llm_node_789", "Stream Description Generator");
```

### QML (Blueprint UI)
```qml
LLMManager {
    controller: llmManagerController
    
    // Provider selection
    ComboBox {
        model: ["Gemini", "OpenAI", "Anthropic", "Ollama"]
        currentIndex: controller.currentProviderIndex
    }
    
    // Model cards with branding
    ListView {
        model: controller.models
        delegate: ModelCard {
            modelName: modelData.name
            providerColor: controller.getProviderColor(modelData.provider)
            costPerToken: modelData.pricing
        }
    }
}
```

### Qt Widget (Active Frame)
```cpp
auto llmWidget = new LLMManagerWidget(controller, parent);
llmWidget->setTitle("AI Models");
llmWidget->showProviderTab("Gemini");
```

---

## File Organization

```
ui/shared_ui/managers/LLMManager/
├── LLMManagerController.h
├── LLMManagerController.cpp
├── LLMManagerWidget.h
├── LLMManagerWidget.cpp
└── LLMManager.qml

API Keys (encrypted):
~/.config/neural-studio/api_credentials.enc

Presets:
~/.config/neural-studio/LLMNode_presets.json
```

---

## Security

### API Key Storage
- **Encryption**: AES-256 encryption
- **Keyring**: System keyring integration (libsecret on Linux)
- **Never Logged**: API keys never appear in logs
- **Per-Provider**: Separate keys for each provider

### Privacy
- **Local First**: Ollama option for fully local AI
- **No Telemetry**: API calls go directly to chosen provider
- **User Control**: Explicit API selection

---

## Integration Points

### With Scene Graph
- Creates `LLMNode` instances
- Provides AI-generated content to downstream nodes
- Async execution (non-blocking)

### With Voice Control
- LLMNodes can process voice commands
- Convert speech-to-text to scene actions
- Natural language interface

### With Automation
- Generate scene descriptions
- Create automated responses
- Content moderation

---

## Cost Management

### Token Tracking
- Input tokens counted
- Output tokens counted
- Cached tokens (Gemini)
- Real-time cost estimation

### Usage Reports
- Per-provider usage
- Per-project usage
- Monthly cost trends
- Token efficiency metrics

---

## Future Enhancements
- [ ] Fine-tuned model support
- [ ] RAG (Retrieval Augmented Generation) integration
- [ ] Multi-agent workflows
- [ ] Streaming responses in UI
- [ ] Voice input/output nodes
- [ ] Custom system prompts per project
