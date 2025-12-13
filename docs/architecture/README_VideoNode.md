# VideoNode Architecture

## Overview

`VideoNode` is a Blueprint node that loads and plays video files, providing synchronized visual and audio outputs to the scene graph pipeline.

## Location
- **Header**: `core/src/scene-graph/nodes/VideoNode.h`
- **Implementation**: `core/src/scene-graph/nodes/VideoNode.cpp`
- **Factory**: `core/src/scene-graph/nodes/VideoNodeFactory.cpp`

## Purpose

Enable video playback within the Neural Studio scene graph, supporting:
- Video decoding and frame extraction
- Synchronized audio output
- Looping playback
- Integration with rendering pipeline

## Architecture

### Class Hierarchy
```
IExecutableNode (interface)
    ↓
BaseNodeBackend (abstract base)
    ↓
VideoNode (concrete implementation)
```

### Key Components

#### Properties
- **`m_videoPath`** (std::string): Absolute path to video file
- **`m_loop`** (bool): Loop playback when video ends
- **`m_videoPlayerId`** (uint32_t): Handle to video decoder/player instance
- **`m_dirty`** (bool): Marks when video needs reload

#### Pins
**Outputs**:
- `visual_out`: **Video** data type - Complete video clip (not individual frames)
  - Represents the entire media asset
  - Suitable for timeline editing, playlists, compositing
  - Frame decoding is an implementation detail
- `audio_out`: **AudioBuffer** data type - Synchronized audio track

**Inputs**: None (video is a source node)

**Note**: The `Video` type represents a **video clip** as a semantic unit (like a file reference + playback state), not individual decoded frames. This enables high-level operations like timeline sequencing, playlist management, and stream queuing.

### Data Flow

```
[Video File on Disk]
        ↓
   VideoNode::execute()
        ↓
   [Initialize Video Player]
        ↓
   ┌─────────────┬─────────────┐
   ↓             ↓             ↓
visual_out   audio_out    [State Update]
(Video Clip) (Audio Track)
   ↓             ↓
[Renderer]   [Audio Mix]
   ↓
[Decode Frames as needed]
```

**Key Concept**: The output is the **video clip** (media reference + playback state), not individual frames. Downstream nodes (renderers, effects) handle frame-by-frame processing as needed. This allows for:
- **Timeline**: Arrange multiple video clips in sequence
- **Playlists**: Queue videos for streaming
- **Effects**: Apply filters to the entire clip
- **Compositing**: Layer multiple video sources

## Implementation Details

### Constructor
```cpp
VideoNode::VideoNode(const std::string& id)
    : BaseNodeBackend(id, "VideoNode")
{
    addOutput("visual_out", "Visual Output", DataType::Video());
    addOutput("audio_out", "Audio Output", DataType::Audio());
}
```

Initializes the node with:
- Unique ID
- Node type identifier
- Output pin definitions

### Execution
```cpp
void VideoNode::execute(const ExecutionContext& context)
{
    if (m_dirty && !m_videoPath.empty()) {
        // Initialize video decoder
        m_dirty = false;
    }

    if (m_videoPlayerId > 0) {
        // Decode current frame
        // setOutputData("visual_out", currentFrameTexture);
        // setOutputData("audio_out", currentAudioBuffer);
    }
}
```

**Execution Flow**:
1. Check if video path changed (`m_dirty`)
2. Initialize/reload video decoder if needed
3. Decode current frame
4. Set output data (texture + audio)
5. Advance playback position

### Property Setters

#### setVideoPath
```cpp
void VideoNode::setVideoPath(const std::string& path)
{
    if (m_videoPath != path) {
        m_videoPath = path;
        m_dirty = true;  // Trigger reload on next execute
    }
}
```

#### setLoop
```cpp
void VideoNode::setLoop(bool loop)
{
    m_loop = loop;
}
```

## Integration Points

### Manager Integration
**VideoManager** creates and configures VideoNode instances:

```cpp
QString nodeId = m_nodeGraphController->createNode("VideoNode", 0.0f, 0.0f);
m_nodeGraphController->setNodeProperty(nodeId, "videoPath", assetPath);
m_nodeGraphController->setNodeProperty(nodeId, "loop", true);
```

### Node Factory
Registered in `VideoNodeFactory.cpp`:
```cpp
NodeFactory::registerNodeType("VideoNode", createVideoNode);
```

Enables dynamic creation via string identifier.

## Supported Formats

Currently scanned formats (extendable):
- MP4 (`.mp4`, `.m4v`)
- MOV (`.mov`)
- AVI (`.avi`)
- MKV (`.mkv`)
- WebM (`.webm`)

## Future Enhancements

### Phase 1: Basic Playback
- [ ] Integrate FFmpeg for decoding
- [ ] Implement frame buffering
- [ ] Add playback controls (play/pause/seek)
- [ ] Sync audio with video timeline

### Phase 2: Advanced Features
- [ ] Hardware-accelerated decoding (NVDEC, VAAPI)
- [ ] Multiple video format support
- [ ] Real-time effects/filters
- [ ] Thumbnail generation for preview

### Phase 3: VR Integration
- [ ] Stereoscopic video support (side-by-side, over-under)
- [ ] 360° video playback
- [ ] Spatial audio positioning
- [ ] VR headset-optimized decoding

## Dependencies

- **FFmpeg** (planned): Video decoding library
- **SceneGraph**: Node execution framework
- **NodeFactory**: Dynamic node creation
- **BaseNodeBackend**: Base node implementation

## Testing

### Unit Tests
- [ ] Test video path property setting
- [ ] Test loop property toggling
- [ ] Test output pin data types
- [ ] Test factory registration

### Integration Tests
- [ ] Test video file loading
- [ ] Test frame decoding
- [ ] Test audio synchronization
- [ ] Test loop behavior

## Usage Example

### C++ (Backend)
```cpp
auto video = std::make_shared<VideoNode>("video_1");
video->setVideoPath("/path/to/video.mp4");
video->setLoop(true);

ExecutionContext ctx;
video->execute(ctx);  // Decode frame

auto frame = video->getOutputData("visual_out");
auto audio = video->getOutputData("audio_out");
```

### QML (UI via Manager)
```qml
VideoManager {
    controller: videoManagerController
    
    // User clicks "Add" on video asset
    // -> Creates VideoNode automatically
}
```

### Blueprint Graph
```
┌─────────────┐       ┌──────────────┐
│ VideoNode   │──────▶│ EffectNode   │
│ (source)    │ tex   │ (filter)     │
└─────────────┘       └──────────────┘
      │ audio               │
      ↓                     ↓
┌─────────────┐       ┌──────────────┐
│ AudioMixer  │       │ OutputNode   │
└─────────────┘       └──────────────┘
```

## Related Documentation
- [Manager System](../ui/shared_ui/managers/README_MANAGERS.md)
- [Scene Graph](./README_SceneGraph.md)
- [Node Factory](./README_NodeFactory.md)
