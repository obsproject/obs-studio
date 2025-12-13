# HeadsetOutputNode UI Module

add_library(ui-headsetoutputnode STATIC
    HeadsetOutputNodeController.cpp
    HeadsetOutputNodeController.h
)

target_include_directories(ui-headsetoutputnode PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/ui/blueprint/nodes
)

target_link_libraries(ui-headsetoutputnode
    Qt6::Core
    Qt6::Quick
)

# QML files
set(QML_FILES
    HeadsetOutputNode.qml
)

# Install
install(TARGETS ui-headsetoutputnode
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(FILES ${QML_FILES}
    DESTINATION share/neural-studio/qml/blueprint/nodes/HeadsetOutputNode
)
