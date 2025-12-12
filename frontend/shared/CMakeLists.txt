
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_library(vr_frontend_shared STATIC
    VRButton.cpp
    VRSlider.cpp
    VRFilePicker.cpp
    ThemeManager.cpp
)

target_link_libraries(vr_frontend_shared PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

target_include_directories(vr_frontend_shared PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
