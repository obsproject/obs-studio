# Audio Manager

**Location**: `ui/shared_ui/managers/AudioManager/`, `ui/blueprint/nodes/AudioNode/`
**Status**: ✅ Implemented
**Version**: 1.0.0
**Date**: 2025-12-14

---

## Overview

The **Audio Manager** manages all audio file sources in Neural Studio, including music, sound effects, voiceovers, and spatial/ambisonic audio for VR. It provides audio discovery, waveform visualization, and AudioNode creation.

### Key Features
- **Format Support**: WAV, MP3, FLAC, OGG, AAC, Opus
- **Spatial Audio**: Ambisonics (1st-3rd order), binaural HRTF
- **Waveform Preview**: Visual audio representation
- **Multi-Channel**: Stereo, 5.1, 7.1, Ambisonics
- **Preset Management**: Save audio configurations
- **Multi-Node Tracking**: Manage multiple audio sources

---

## Architecture

### Backend (`AudioManagerController.h/cpp`)
- **Inheritance**: `BaseManagerController` → `QObject`
- **Responsibilities**:
  - Scanning `/assets/audio/` directory
  - Audio metadata extraction (format, channels, sample rate, duration)
  - Waveform data generation
  - Tracking all AudioNode instances
  - Preset management

### Frontend Options

#### QML View (`AudioManager.qml`)
- **Purpose**: Blueprint Frame audio library
- **Features**:
  - Audio file list with waveforms
  - Playback preview
  - Spatial audio badges
  - Drag-to-create AudioNode
  - "In Use" indicators

#### Qt Widget (`AudioManagerWidget.h/cpp`)
- **Purpose**: Active Frame audio browser
- **Features**:
  - Sortable audio library
  - Inline audio player
  - Spectrum analyzer
  - Import audio button

---

## Interface

### Controller Properties
| Property | Type | Description |
|----------|------|-------------|
| `audioFiles` | QVariantList | Discovered audio files with metadata |
| `managedNodes` | QStringList | Active AudioNode IDs |
| `nodePresets` | QVariantList | Saved audio configurations |

### Methods
| Method | Parameters | Description |
|--------|------------|-------------|
| `scanAudio()` | - | Rescan audio directory |
| `createAudioNode()` | audioPath, x, y | Create AudioNode |
| `savePreset()` | nodeId, presetName | Save audio config |
| `loadPreset()` | presetName, x, y | Create from preset |
| `getAudioMetadata()` | audioPath | Extract format info |
| `generateWaveform()` | audioPath | Create waveform data |

---

## Supported Formats

### Lossless
- **WAV** (.wav) - Uncompressed, professional
- **FLAC** (.flac) - Lossless compression
- **AIFF** (.aiff) - Apple uncompressed

### Lossy
- **MP3** (.mp3) - Universal compatibility
- **AAC** (.aac, .m4a) - Better quality than MP3
- **OGG Vorbis** (.ogg) - Open format
- **Opus** (.opus) - Low-latency streaming

### Spatial Audio
- **Ambisonics** (.amb, .ambi) - Full-sphere surround
- **Binaural** (.bin) - Headphone-optimized 3D
- **5.1/7.1 Surround** - Multi-channel

---

## Usage Example

### C++ (Controller)
```cpp
auto audioManager = new AudioManagerController(parent);
audioManager->scanAudio();

// Create audio node
audioManager->createAudioNode("/assets/audio/bgm_ambient.flac", 150, 200);

// Save configuration
audioManager->savePreset("audio_node_456", "Ambient Music Loop");
```

### QML (Blueprint UI)
```qml
AudioManager {
    controller: audioManagerController
    
    ListView {
        model: controller.audioFiles
        delegate: AudioCard {
            audioName: modelData.name
            waveform: modelData.waveformData
            duration: modelData.duration
            channelConfig: modelData.channels
            isSpatial: modelData.isAmbisonics
            inUse: controller.managedNodes.includes(modelData.id)
        }
    }
}
```

---

## Metadata Extraction

```cpp
QVariantMap metadata = audioManager->getAudioMetadata(audioPath);

// Available fields:
metadata["format"]      // "flac"
metadata["sampleRate"]  // "48000"
metadata["channels"]    // "2" or "ambix_4" (ambisonics)
metadata["bitDepth"]    // "24"
metadata["duration"]    // "180.5" (seconds)
metadata["bitrate"]     // "1411" (kbps)
metadata["isLossless"]  // true/false
```

---

## File Organization

```
/assets/audio/
├── music/           # Background music
├── sfx/             # Sound effects
├── voice/           # Voiceovers, dialogue
├── ambience/        # Ambient soundscapes
└── spatial/         # Ambisonics, binaural

Presets:
~/.config/neural-studio/AudioNode_presets.json

Waveform cache:
~/.cache/neural-studio/audio_waveforms/
```

---

## Spatial Audio Support

### Ambisonics
- **1st Order** (4 channels) - Basic surround
- **2nd Order** (9 channels) - Higher resolution
- **3rd Order** (16 channels) - Professional VR

### HRTF (Head-Related Transfer Function)
- Binaural rendering for headphones
- Personalized HRTF profiles (planned)

### 3D Positioning
- Audio sources anchored to 3D scene nodes
- Distance attenuation
- Doppler effect
- Occlusion/reverberation (planned)

---

## Integration Points

### With Scene Graph
- Creates `AudioNode` instances
- Provides audio buffers to mixer
- Synchronized with video playback

### With VR Pipeline
- Spatial audio mixing
- Ambisonic decoding
- Headphone virtualization

---

## Future Enhancements
- [ ] Audio effects (EQ, compression, reverb)
- [ ] Multi-track editing
- [ ] Voice activity detection
- [ ] Noise reduction
- [ ] Auto-ducking
- [ ] Beat sync / tempo detection
