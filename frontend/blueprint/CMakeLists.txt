

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 COMPONENTS Quick Qml QuickControls2 Quick3D Multimedia REQUIRED)

# LEGACY TARGET - DISABLED (Using vr_blueprint_qml instead)
# add_executable(vr_blueprint 
#     main.cpp
#     NodeGraph.cpp
#     NodeItem.cpp
#     NodeRegistry.cpp
#     PortItem.cpp
#     ConnectionItem.cpp
#     GraphSerializer.cpp
#     PropertiesPanel.cpp
#     ../widgets/SpatialPreviewWidget.cpp
# )
# 
# target_link_libraries(vr_blueprint PRIVATE
#     Qt6::Core
#     Qt6::Gui
#     Qt6::Widgets
#     Qt6::3DCore
#     Qt6::3DRender
#     Qt6::3DExtras
#     vr_frontend_shared
# )
# 
# # Link headers for VRProtocol
# find_package(Qt6 COMPONENTS Quick Qml QuickControls2 Quick3D Multimedia 3DCore 3DRender 3DExtras REQUIRED)
# 
# target_include_directories(vr_blueprint PRIVATE
#     ${CMAKE_SOURCE_DIR}/shared/vr-protocol
# )


# --- QML Prototype Target ---
add_executable(vr_blueprint_qml
    qml_main.cpp
    NodeModel.cpp
    ConnectionModel.cpp
    NodeItem.cpp
    PortItem.cpp
    viewmodels/PropertiesViewModel.cpp
    viewmodels/SettingsViewModel.cpp
    viewmodels/MixerViewModel.cpp
    nodes/VideoInputNode.cpp
    nodes/AudioInputNode.cpp
)

target_link_libraries(vr_blueprint_qml PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
    Qt6::QuickControls2
    Qt6::Quick3D
    Qt6::Multimedia
    vr_frontend_shared
)

target_include_directories(vr_blueprint_qml PRIVATE
    ${CMAKE_SOURCE_DIR}/shared/vr-protocol
)
