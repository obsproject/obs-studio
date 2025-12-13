# ThreeDModelNode Architecture

## Overview

`ThreeDModelNode` loads and manages 3D models within the Blueprint scene graph, integrating with the `SceneManager` for spatial rendering.

## Location
- **Header**: `core/src/scene-graph/nodes/ThreeDModelNode.h`
- **Implementation**: `core/src/scene-graph/nodes/ThreeDModelNode.cpp`
- **Factory**: `core/src/scene-graph/nodes/ThreeDModelNodeFactory.cpp`

## Purpose

Enable 3D model loading and rendering within Neural Studio:
- Load 3D models from disk (OBJ, GLB, GLTF, FBX)
- Register with SceneManager for spatial management
- Provide visual output for rendering pipeline
- Support transform operations (planned)

## Architecture

### Class Hierarchy
```
IExecutableNode (interface)
    ↓
BaseNodeBackend (abstract base)
    ↓
ThreeDModelNode (concrete implementation)
```

### Key Components

#### Properties
- **`m_modelPath`** (std::string): Path to 3D model file
- **`m_sceneObjectId`** (uint32_t): Reference to loaded model in SceneManager
- **`m_dirty`** (bool): Marks when model needs reload

#### Pins
**Outputs**:
- `visual_out`: Mesh data type - Loaded 3D model geometry

**Inputs**: None (source node for 3D geometry)

### Data Flow

```
[3D Model File (.obj, .glb, .fbx)]
        ↓
   ThreeDModelNode::execute()
        ↓
   [Load via SceneManager]
        ↓
   m_sceneObjectId (reference)
        ↓
   visual_out (Mesh output)
        ↓
   [Render Pipeline]
```

## Implementation Details

### Constructor
```cpp
ThreeDModelNode::ThreeDModelNode(const std::string& id)
    : BaseNodeBackend(id, "ThreeDModelNode")
{
    addOutput("visual_out", "Visual Output", 
              DataType(DataCategory::Media, "Mesh"));
}
```

### Execution
```cpp
void ThreeDModelNode::execute(const ExecutionContext& context)
{
    if (m_dirty && !m_modelPath.empty()) {
        std::cout << "Loading 3D model: " << m_modelPath << std::endl;
        // Future: m_sceneObjectId = SceneManager::loadModel(m_modelPath);
        m_dirty = false;
    }

    // Output loaded model reference
    if (m_sceneObjectId > 0) {
        // setOutputData("visual_out", m_sceneObjectId);
    }
}
```

### Property Setters

#### setModelPath
```cpp
void ThreeDModelNode::setModelPath(const std::string& path)
{
    if (m_modelPath != path) {
        m_modelPath = path;
        m_dirty = true;
    }
}
```

Triggers model reload on next execution when path changes.

## Integration Points

### Manager Integration
**ThreeDAssetsManager** discovers and creates nodes:

```cpp
// Manager scans assets/3d for .obj, .glb, .gltf, .fbx
QString nodeId = m_nodeGraphController->createNode("ThreeDModelNode", 0, 0);
m_nodeGraphController->setNodeProperty(nodeId, "modelPath", assetPath);
```

### SceneManager Integration (Planned)
```cpp
// Future implementation
uint32_t objId = SceneManager::instance()->loadModel(m_modelPath);
SceneManager::instance()->setTransform(objId, position, rotation, scale);
```

### Node Factory
```cpp
NodeFactory::registerNodeType("ThreeDModelNode", createThreeDModelNode);
```

## Supported Formats

Currently scanned by ThreeDAssetsManager:
- **OBJ** (`.obj`) - Wavefront format
- **GLB** (`.glb`) - Binary glTF
- **GLTF** (`.gltf`) - Text glTF
- **FBX** (`.fbx`) - Autodesk format

## Future Enhancements

### Phase 1: Basic Loading
- [x] Model path property
- [x] Factory registration
- [x] Manager integration
- [ ] SceneManager model loading
- [ ] Mesh output data

### Phase 2: Transform Support
- [ ] Position property (Vector3)
- [ ] Rotation property (Vector3/Quaternion)
- [ ] Scale property (Vector3)
- [ ] Transform gizmo in VR preview

### Phase 3: Advanced Features
- [ ] Material assignment
- [ ] Animation playback
- [ ] LOD (Level of Detail) support
- [ ] Instancing for performance
- [ ] Physics collision geometry

### Phase 4: VR Optimization
- [ ] Occlusion culling
- [ ] Foveated rendering support
- [ ] Spatial audio attachment points
- [ ] Hand interaction zones

## Dependencies

- **SceneManager**: Spatial scene management
- **Model Loaders**: Assimp, glTF loaders
- **NodeFactory**: Dynamic creation
- **BaseNodeBackend**: Base implementation

## Testing

### Unit Tests
- [x] Property setter functionality
- [x] Factory registration
- [ ] Model loading verification
- [ ] Output pin data validation

### Integration Tests
- [ ] Load various model formats
- [ ] Verify SceneManager registration
- [ ] Test transform updates
- [ ] Performance benchmarks

## Usage Example

### C++ (Backend)
```cpp
auto model = std::make_shared<ThreeDModelNode>("model_1");
model->setModelPath("/assets/3d/character.glb");

ExecutionContext ctx;
model->execute(ctx);  // Load model

uint32_t sceneId = model->getSceneObjectId();
```

### QML (UI via Manager)
```qml
ThreeDAssetsManager {
    controller: threeDAssetsManagerController
    
    // Displays list of 3D files
    // "Add" button creates ThreeDModelNode
}
```

### Blueprint Graph
```
┌──────────────────┐       ┌────────────────┐
│ ThreeDModelNode  │──────▶│ TransformNode  │
│ (source)         │  mesh │ (modifier)     │
└──────────────────┘       └────────────────┘
                                   │
                                   ↓
                           ┌────────────────┐
                           │  OutputNode    │
                           │  (VR render)   │
                           └────────────────┘
```

## Related Documentation
- [Manager System](../ui/shared_ui/managers/README_MANAGERS.md)
- [Scene Graph](./README_SceneGraph.md)
- [SceneManager](./README_SceneManager.md)
- [Node Factory](./README_NodeFactory.md)
