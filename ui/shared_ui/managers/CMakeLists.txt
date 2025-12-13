# Blueprint Managers

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find required Qt modules for managers
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Quick
    Widgets
    Multimedia          # Video, Audio, Camera playback & capture
    SpatialAudio        # VR 3D audio positioning
    ShaderTools         # GLSL compilation & validation
    Svg                 # Vector graphics (logos, icons, overlays)
    Quick3D             # 3D models, textures, PBR materials
    Charts              # Analytics dashboards for ML/LLM
    QuickTimeline       # Video/audio sequencing
    Scxml               # Script state machines
    UiTools             # Dynamic UI loading
)

set(MANAGERS_SOURCES
    BaseManagerController.cpp
    BaseManagerWidget.cpp
    CameraManager/CameraManagerController.cpp
    ThreeDAssetsManager/ThreeDAssetsManagerController.cpp
    VideoManager/VideoManagerController.cpp
    GraphicsManager/GraphicsManagerController.cpp
    AudioManager/AudioManagerController.cpp
    FontManager/FontManagerController.cpp
    ShaderManager/ShaderManagerController.cpp
    TextureManager/TextureManagerController.cpp
    EffectsManager/EffectsManagerController.cpp
    ScriptManager/ScriptManagerController.cpp
    ScriptManager/ScriptManagerWidget.cpp
    MLManager/MLManagerController.cpp
    MLManager/MLManagerWidget.cpp
    LLMManager/LLMManagerController.cpp
    LLMManager/LLMManagerWidget.cpp
)

set(MANAGERS_HEADERS
    BaseManagerController.h
    BaseManagerWidget.h
    CameraManager/CameraManagerController.h
    ThreeDAssetsManager/ThreeDAssetsManagerController.h
    VideoManager/VideoManagerController.h
    GraphicsManager/GraphicsManagerController.h
    AudioManager/AudioManagerController.h
    FontManager/FontManagerController.h
    ShaderManager/ShaderManagerController.h
    TextureManager/TextureManagerController.h
    EffectsManager/EffectsManagerController.h
    ScriptManager/ScriptManagerController.h
    ScriptManager/ScriptManagerWidget.h
    MLManager/MLManagerController.h
    MLManager/MLManagerWidget.h
    LLMManager/LLMManagerController.h
    LLMManager/LLMManagerWidget.h
)

add_library(ui-managers STATIC
    ${MANAGERS_SOURCES}
    ${MANAGERS_HEADERS}
)

target_link_libraries(ui-managers
    PRIVATE
        Qt6::Core
        Qt6::Quick
        Qt6::Widgets
        Qt6::Multimedia          # Video, Audio, Camera playback & capture
        Qt6::SpatialAudio        # VR 3D audio positioning
        Qt6::ShaderTools         # GLSL compilation & validation
        Qt6::Svg                 # Vector graphics (logos, icons, overlays)
        Qt6::Quick3D             # 3D models, textures, PBR materials
        Qt6::Charts              # Analytics dashboards for ML/LLM
        Qt6::QuickTimeline       # Video/audio sequencing
        Qt6::Scxml               # Script state machines
        Qt6::UiTools             # Dynamic UI loading
        ui-basenode # For BaseManagerController dependencies
        ui-core # For NodeGraphController
)

target_include_directories(ui-managers PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Install QML
install(FILES
    BaseManager.qml
    CameraManager/CameraManager.qml
    ThreeDAssetsManager/ThreeDAssetsManager.qml
    VideoManager/VideoManager.qml
    GraphicsManager/GraphicsManager.qml
    AudioManager/AudioManager.qml
    FontManager/FontManager.qml
    ShaderManager/ShaderManager.qml
    TextureManager/TextureManager.qml
    EffectsManager/EffectsManager.qml
    ScriptManager/ScriptManager.qml
    MLManager/MLManager.qml
    LLMManager/LLMManager.qml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/neural-studio/qml/managers
)
