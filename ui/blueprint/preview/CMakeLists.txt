# Blueprint Preview Module
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)


add_library(ui-preview STATIC
    BasePreviewController.cpp
    BasePreviewController.h
)

target_include_directories(ui-preview PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ui-preview
    Qt6::Core
    Qt6::Quick
)

# Install QML
install(FILES
    BasePreview.qml
    ScenePreview.qml
    StereoPreview.qml
    DESTINATION share/neural-studio/qml/blueprint/preview
)
