
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(vr_active
    main.cpp
    MonitorWidget.cpp
    MonitorGrid.cpp
    AudioMixer.cpp
    SceneButtons.cpp
    VideoModule.cpp
    CamModule.cpp
    ThreeDModule.cpp
    ExtraModules.cpp
    MoreModules.cpp
)

target_link_libraries(vr_active PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    vr_frontend_shared
)

# Link headers for VRProtocol
target_include_directories(vr_active PRIVATE
    ${CMAKE_SOURCE_DIR}/shared/vr-protocol
)
