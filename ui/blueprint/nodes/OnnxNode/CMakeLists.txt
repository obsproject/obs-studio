add_library(ui-onnxnode STATIC
    OnnxNodeController.cpp
    OnnxNodeController.h
)

target_include_directories(ui-onnxnode PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ui-onnxnode
    Qt6::Core
    Qt6::Quick
    ui-basenode
)

install(FILES OnnxNode.qml
    DESTINATION share/neural-studio/qml/blueprint/nodes/OnnxNode
)
