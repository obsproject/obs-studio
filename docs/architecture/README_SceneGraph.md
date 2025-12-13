# Scene Graph Module - NodeExecutionGraph

**Location**: `core/src/scene-graph/`  
**Status**: ✅ Core Implementation Complete  
**Version**: 1.0.0  
**Date**: 2025-12-13

---

## Overview

The **Scene Graph Module** provides an industrial-strength **node execution graph system** that powers Neural Studio's Blueprint-based processing pipeline. It enables users to create custom workflows by connecting processing nodes in a visual graph, supporting unlimited extensibility through a plugin architecture.

### Key Capabilities
- ✅ Dynamic node graph with topological sort
- ✅ Type-safe pin connections
- ✅ Plugin-based node registration (unlimited node types)
- ✅ Parallel execution of independent nodes
- ✅ Smart caching system
- ✅ Cycle detection and validation
- ✅ Comprehensive error handling

---

## Architecture

### Design Pattern
**Factory + Registry** - Core system agnostic to specific node types

```
Plugin Registration → Node Factory → Graph Compilation → Execution
        ↓                  ↓              ↓                ↓
   REGISTER_NODE     NodeFactory    Topological Sort   executeParallel()
```

### Data Flow
```
User Creates Graph → Compile (validate + sort) → Execute (per frame)
        ↓                      ↓                        ↓
   addNode()             compile()                  execute()
   connectPins()         - Type check               - Propagate data
                         - Detect cycles            - Cache results
                         - Topological sort         - Handle errors
```

---

## Files

### Core Interfaces
- **`IExecutableNode.h/cpp`** - Node interface, type system, factory
  - `IExecutableNode` - Base interface for all nodes
  - `DataType` - Rich type system (8 categories)
  - `NodeFactory` - Plugin registration and creation
  - `NodeMetadata` - Self-describing node capabilities
  - `ExecutionContext/Result` - Execution environment

### Graph Engine
- **`NodeExecutionGraph.h/cpp`** - Main graph execution engine
  - `NodeExecutionGraph` - Core graph manager
  - Topological sort (Kahn's algorithm)
  - Cycle detection (DFS)
  - Pin connection management
  - Parallel execution engine
  - Caching system

### Base Classes
- **`ProcessingNode.h/cpp`** - Convenient base class for node development
  - Pin management helpers
  - Data accessor templates
  - Metadata setup

### Build
- **`CMakeLists.txt`** - Build configuration

---

## Type System

### Data Categories
```cpp
enum class DataCategory {
    Primitive,  // Scalar, Vector3, Color, Boolean, String
    Media,      // Texture2D, AudioBuffer, VideoFrame, Mesh
    Spatial,    // Transform, Camera, Light
    Effect,     // Shader, Material, Filter, LUT
    AI,         // Tensor, Model, Embedding
    Composite   // Array, Map, Variant, Stream
};
```

### Common Types
- `DataType::Texture2D()` - GPU textures
- `DataType::Audio()` - Audio buffers
- `DataType::Video()` - Video frames
- `DataType::Mesh()` - 3D meshes
- `DataType::Transform()` - 4x4 matrices
- `DataType::Scalar()` - Float values
- `DataType::Any()` - Type-erased (compatible with all)

---

## Usage

### Creating a Custom Node

```cpp
#include "scene-graph/ProcessingNode.h"

class MyFilterNode : public ProcessingNode {
public:
    MyFilterNode(const std::string& id) 
        : ProcessingNode(id, "MyFilter") 
    {
        // Define metadata
        NodeMetadata meta;
        meta.displayName = "My Filter";
        meta.category = "Effect";
        meta.supportsGPU = true;
        setMetadata(meta);
        
        // Define pins
        addInput("input", "Video In", DataType::Texture2D());
        addInput("strength", "Strength", DataType::Scalar());
        addOutput("output", "Filtered Out", DataType::Texture2D());
    }
    
    ExecutionResult process(ExecutionContext& ctx) override {
        auto* inputTex = getInputData<QRhiTexture>("input");
        float strength = getInputData<float>("strength", 1.0f);
        
        // Your processing logic here
        QRhiTexture* output = applyFilter(inputTex, strength);
        
        setOutputData("output", output);
        return ExecutionResult::success();
    }
};

// Register the node type
REGISTER_NODE_TYPE(MyFilterNode, "MyFilter");
```

### Building and Executing a Graph

```cpp
#include "scene-graph/NodeExecutionGraph.h"

// Create graph
NodeExecutionGraph graph;

// Create nodes
auto camera = NodeFactory::create("CameraInput", "camera1");
auto filter = NodeFactory::create("MyFilter", "filter1");
auto output = NodeFactory::create("Output", "output1");

// Add to graph
graph.addNode(camera);
graph.addNode(filter);
graph.addNode(output);

// Connect pins
graph.connectPins("camera1", "video", "filter1", "input");
graph.connectPins("filter1", "output", "output1", "input");

// Compile (validates and sorts)
if (!graph.compile()) {
    for (auto& error : graph.getValidationErrors()) {
        std::cerr << error.message << std::endl;
    }
    return;
}

// Execute (every frame)
ExecutionContext ctx;
ctx.renderer = myRenderer;
ctx.deltaTime = 1.0 / 60.0;

graph.executeParallel(ctx);  // Parallel execution
// or
graph.execute(ctx);           // Sequential execution
```

---

## Algorithms

### Topological Sort (Kahn's Algorithm)
**Purpose**: Determine execution order respecting dependencies  
**Complexity**: O(V + E) where V = nodes, E = connections  
**Steps**:
1. Compute in-degrees for all nodes
2. Queue nodes with in-degree 0 (no dependencies)
3. Process queue, reducing dependent nodes' in-degrees
4. Result is execution order

### Cycle Detection (DFS)
**Purpose**: Prevent infinite loops in graph  
**Complexity**: O(V + E)  
**Algorithm**: Depth-first search with recursion stack tracking  
**Result**: `true` if cycle detected, `false` if DAG (valid)

---

## Performance

| Operation | Complexity | Notes |
|-----------|-----------|-------|
| Compilation | O(V + E) | One-time per graph change |
| Execution | O(V) | Per frame, sequential |
| Parallel Execution | O(longest_path) | With unlimited parallelism |
| Cache Lookup | O(1) | Hash-based |
| Type Validation | O(E) | Per connection |

**Memory**: O(V + E) graph structure + O(V) cache storage

---

## Features

### Plugin Architecture
- **Runtime Registration**: Register node types at any time
- **Factory Pattern**: Create instances via string type name
- **Decoupled**: Core graph knows nothing about specific nodes
- **Extensible**: Unlimited node types supported

### Type Safety
- **Compile-Time**: Strong C++ typing for node implementations
- **Runtime**: Pin type validation prevents incompatible connections
- **Compatibility**: Flexible type matching (e.g., Any type, base classes)

### Caching System
- **Hash-Based**: Cache invalidation based on input data hash
- **Per-Node**: Individual nodes can opt-in/out of caching
- **Performance**: Avoids redundant computation when inputs unchanged

### Parallel Execution
- **Automatic**: Detects independent nodes via topological levels
- **Safe**: Thread-safe execution with proper data propagation
- **Scalable**: Near-linear speedup for independent subgraphs

### Error Handling
- **Graceful Degradation**: Continue execution with fallback outputs
- **Retry Policies**: Temporary failures can be retried
- **Error Callbacks**: Custom error handlers for logging/recovery

---

## Integration Points

### Current
- **Standalone**: Can be used independently

### Planned
- **FrameRouter Integration**: Replace hardcoded pipeline (Task 3.1)
- **SceneManager**: 3D scene graph backend (Task 3.2)
- **Blueprint UI**: QML visual node editor (Task 3.4)
- **Specialized Nodes**: Camera, Filter, Effect, AI nodes (Task 3.3)

---

## Dependencies

### External
- C++17 standard library
- `<functional>`, `<memory>`, `<any>`, `<future>` for async

### Internal (Planned)
- Qt6::Core (for QML integration)
- Qt6::RHI (for texture handling in nodes)
- `core/src/rendering/*` (for rendering nodes)

---

## Future Enhancements

### Phase 1 (Completed)
- [x] Core graph engine
- [x] Plugin system
- [x] Topological sort
- [x] Cycle detection
- [x] Type validation
- [x] Caching
- [x] Parallel execution

### Phase 2 (In Progress)
- [ ] Specific node types (Camera, Stitch, Filter, Output)
- [x] AI Nodes (Wasm, Onnx, Gemini)
- [ ] FrameRouter integration
- [ ] Unit tests

### Phase 3 (Future)
- [ ] QML integration
- [ ] Visual node editor
- [ ] Hot-reload for node plugins
- [ ] Profiling/debugging tools
- [ ] Graph serialization/deserialization
- [ ] Subgraph support (nested graphs)

---

## Examples

See [`implementation_plan.md`](file:///home/subtomic/.gemini/antigravity/brain/e1490c41-82cd-4b33-a882-c7db658f56a2/implementation_plan.md) and [`walkthrough.md`](file:///home/subtomic/.gemini/antigravity/brain/e1490c41-82cd-4b33-a882-c7db658f56a2/walkthrough.md) for detailed usage examples and architecture documentation.

---

## Documentation

- **API Reference**: See header files (`IExecutableNode.h`, `NodeExecutionGraph.h`)
- **Architecture**: [Scene Graph Analysis](file:///home/subtomic/.gemini/antigravity/brain/e1490c41-82cd-4b33-a882-c7db658f56a2/scene_graph_analysis.md)
- **Implementation Plan**: [NodeExecutionGraph Plan](file:///home/subtomic/.gemini/antigravity/brain/e1490c41-82cd-4b33-a882-c7db658f56a2/implementation_plan.md)
- **Walkthrough**: [Implementation Walkthrough](file:///home/subtomic/.gemini/antigravity/brain/e1490c41-82cd-4b33-a882-c7db658f56a2/walkthrough.md)

---

**Maintainer**: Neural Studio Team  
**Last Updated**: 2025-12-13
