if(NOT TARGET OBS::qt-vertical-scroll-area)
  add_subdirectory(
    "${CMAKE_SOURCE_DIR}/shared/qt/vertical-scroll-area"
    "${CMAKE_BINARY_DIR}/shared/qt/vertical-scroll-area"
  )
endif()

target_link_libraries(obs-studio PRIVATE OBS::qt-vertical-scroll-area)

target_sources(
  obs-studio
  PRIVATE
    widgets/ColorSelect.cpp
    widgets/ColorSelect.hpp
    widgets/OBSBasic.cpp
    widgets/OBSBasic.hpp
    widgets/OBSBasic_Browser.cpp
    widgets/OBSBasic_Clipboard.cpp
    widgets/OBSBasic_ContextToolbar.cpp
    widgets/OBSBasic_Docks.cpp
    widgets/OBSBasic_Dropfiles.cpp
    widgets/OBSBasic_Hotkeys.cpp
    widgets/OBSBasic_Icons.cpp
    widgets/OBSBasic_MainControls.cpp
    widgets/OBSBasic_OutputHandler.cpp
    widgets/OBSBasic_Preview.cpp
    widgets/OBSBasic_Profiles.cpp
    widgets/OBSBasic_Projectors.cpp
    widgets/OBSBasic_Recording.cpp
    widgets/OBSBasic_ReplayBuffer.cpp
    widgets/OBSBasic_SceneCollections.cpp
    widgets/OBSBasic_SceneItems.cpp
    widgets/OBSBasic_Scenes.cpp
    widgets/OBSBasic_Screenshots.cpp
    widgets/OBSBasic_Service.cpp
    widgets/OBSBasic_StatusBar.cpp
    widgets/OBSBasic_Streaming.cpp
    widgets/OBSBasic_StudioMode.cpp
    widgets/OBSBasic_SysTray.cpp
    widgets/OBSBasic_Transitions.cpp
    widgets/OBSBasic_Updater.cpp
    widgets/OBSBasic_VirtualCam.cpp
    widgets/OBSBasic_VolControl.cpp
    widgets/OBSBasic_YouTube.cpp
    widgets/OBSBasicControls.cpp
    widgets/OBSBasicControls.hpp
    widgets/OBSBasicPreview.cpp
    widgets/OBSBasicPreview.hpp
    widgets/OBSBasicStats.cpp
    widgets/OBSBasicStats.hpp
    widgets/OBSBasicStatusBar.cpp
    widgets/OBSBasicStatusBar.hpp
    widgets/OBSMainWindow.hpp
    widgets/OBSProjector.cpp
    widgets/OBSProjector.hpp
    widgets/OBSQTDisplay.cpp
    widgets/OBSQTDisplay.hpp
    widgets/OBSSourceView.cpp
    widgets/OBSSourceView.hpp
    widgets/StatusBarWidget.cpp
    widgets/StatusBarWidget.hpp
    widgets/VolControl.cpp
    widgets/VolControl.hpp
    widgets/VolumeAccessibleInterface.cpp
    widgets/VolumeAccessibleInterface.hpp
    widgets/VolumeMeter.cpp
    widgets/VolumeMeter.hpp
)
