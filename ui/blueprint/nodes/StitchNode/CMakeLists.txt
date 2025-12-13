# StitchNode UI Module

add_library(ui-stitchnode STATIC
    StitchNodeController.cpp
    StitchNodeController.h
)

target_include_directories(ui-stitchnode PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ui-stitchnode
    Qt6::Core
    Qt6::Quick
)

# QML files
set(QML_FILES
    StitchNode.qml
)

# Install
install(TARGETS ui-stitchnode
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(FILES ${QML_FILES}
    DESTINATION share/neural-studio/qml/blueprint/nodes/StitchNode
)
