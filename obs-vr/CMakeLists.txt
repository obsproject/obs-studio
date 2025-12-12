project(obs-vr)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui)

add_executable(obs-vr
    main.cpp
    VirtualCamManager.cpp
    MixerRouter.cpp
    MixerEngine.cpp
)

target_include_directories(obs-vr PRIVATE
    ${CMAKE_SOURCE_DIR}/shared
    ${CMAKE_SOURCE_DIR}/frontend/blueprint
)

target_link_libraries(obs-vr PRIVATE
    libvr
    nlohmann_json::nlohmann_json
    Qt6::Core
    Qt6::Gui
)

set_target_properties(obs-vr PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)
