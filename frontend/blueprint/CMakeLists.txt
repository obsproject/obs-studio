
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(vr_blueprint 
    main.cpp
    NodeGraph.cpp
    NodeItem.cpp
    NodeRegistry.cpp
    PortItem.cpp
    ConnectionItem.cpp
    GraphSerializer.cpp
)

target_link_libraries(vr_blueprint PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Link headers for VRProtocol
target_include_directories(vr_blueprint PRIVATE
    ${CMAKE_SOURCE_DIR}/shared/vr-protocol
)
