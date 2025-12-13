add_library(ui-wasmnode STATIC
    WasmNodeController.cpp
    WasmNodeController.h
)

target_include_directories(ui-wasmnode PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ui-wasmnode
    Qt6::Core
    Qt6::Quick
    ui-basenode
)

install(FILES WasmNode.qml
    DESTINATION share/neural-studio/qml/blueprint/nodes/WasmNode
)
