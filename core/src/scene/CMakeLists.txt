# Scene Module (Spatial Scene Manager)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(SCENE_SOURCES
    SceneManager.cpp
    OBJLoader.cpp
)

set(SCENE_HEADERS
    SceneManager.h
)

add_library(nstudio-scene STATIC
    ${SCENE_SOURCES}
    ${SCENE_HEADERS}
)

target_link_libraries(nstudio-scene
    PRIVATE
        Qt6::Core
        Qt6::Quick3D # SceneManager likely uses Quick3D or generic logic
)

target_include_directories(nstudio-scene PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
