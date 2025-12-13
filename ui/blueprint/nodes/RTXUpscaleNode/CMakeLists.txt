# RTXUpscaleNode UI Module

add_library(ui-rtxupscalenode STATIC
    RTXUpscaleNodeController.cpp
    RTXUpscaleNodeController.h
)

target_include_directories(ui-rtxupscalenode PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/ui/blueprint/nodes
)

target_link_libraries(ui-rtxupscalenode
    Qt6::Core
    Qt6::Quick
)

# QML files
set(QML_FILES
    RTXUpscaleNode.qml
)

# Install
install(TARGETS ui-rtxupscalenode
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(FILES ${QML_FILES}
    DESTINATION share/neural-studio/qml/blueprint/nodes/RTXUpscaleNode
)
