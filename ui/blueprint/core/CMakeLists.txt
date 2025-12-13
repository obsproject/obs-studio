add_library(blueprint-core
    NodeGraphController.cpp
    NodeGraphController.h
)

target_link_libraries(blueprint-core
    PUBLIC
    Qt6::Core
    Qt6::Qml
    Qt6::Quick
    scene-graph
)

target_include_directories(blueprint-core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
