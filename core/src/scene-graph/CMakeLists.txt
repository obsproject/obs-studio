# Scene Graph module

set(SCENE_GRAPH_SOURCES
    nodes/CameraNode/CameraNode.cpp
    nodes/ThreeDModelNode/ThreeDModelNode.cpp
    nodes/ThreeDModelNode/ThreeDModelNodeFactory.cpp
    nodes/VideoNode/VideoNode.cpp
    nodes/VideoNode/VideoNodeFactory.cpp
    nodes/ImageNode/ImageNode.cpp
    nodes/ImageNode/ImageNodeFactory.cpp
    nodes/AudioNode/AudioNode.cpp
    nodes/AudioNode/AudioNodeFactory.cpp
    nodes/FontNode/FontNode.cpp
    nodes/FontNode/FontNodeFactory.cpp
    nodes/ShaderNode/ShaderNode.cpp
    nodes/ShaderNode/ShaderNodeFactory.cpp
    nodes/TextureNode/TextureNode.cpp
    nodes/TextureNode/TextureNodeFactory.cpp
    nodes/EffectNode/EffectNode.cpp
    nodes/EffectNode/EffectNodeFactory.cpp
    nodes/ScriptNode/ScriptNode.cpp
    nodes/ScriptNode/ScriptNodeFactory.cpp
    nodes/MLNode/MLNode.cpp
    nodes/MLNode/MLNodeFactory.cpp
    nodes/LLMNode/LLMNode.cpp
    nodes/LLMNode/LLMNodeFactory.cpp
    nodes/StitchNode/StitchNode.cpp
    nodes/RTXUpscaleNode/RTXUpscaleNode.cpp
    nodes/HeadsetOutputNode/HeadsetOutputNode.cpp
    NodeExecutionGraph.cpp
    BaseNodeBackend.cpp
    NodeFactory.cpp
)

set(SCENE_GRAPH_HEADERS
    IExecutableNode.h
    NodeExecutionGraph.h
    BaseNodeBackend.h
    NodeFactory.h
)

# Create scene-graph library
add_library(scene-graph STATIC
    ${SCENE_GRAPH_SOURCES}
    ${SCENE_GRAPH_HEADERS}
)

# Include directories
target_include_directories(scene-graph PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(scene-graph
    PRIVATE
        Qt6::Core
        nstudio-scene      # New home for SceneManager
        nstudio-rendering  # Runtime capabilities
        nstudio-pipeline   # Frame routing
        # libvr is gone
)

# Installation
install(TARGETS scene-graph
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(FILES ${SCENE_GRAPH_HEADERS}
    DESTINATION include/scene-graph
)
