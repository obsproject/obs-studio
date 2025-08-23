find_package(Qt6 REQUIRED Widgets Network Svg Xml)

if(OS_LINUX OR OS_FREEBSD OR OS_OPENBSD)
  find_package(Qt6 REQUIRED Gui DBus)
endif()

if(NOT TARGET OBS::qt-wrappers)
  add_subdirectory("${CMAKE_SOURCE_DIR}/shared/qt/wrappers" "${CMAKE_BINARY_DIR}/shared/qt/wrappers")
endif()

target_link_libraries(
  obs-studio
  PRIVATE Qt::Widgets Qt::Svg Qt::Xml Qt::Network OBS::qt-wrappers
)

set_target_properties(
  obs-studio
  PROPERTIES AUTOMOC TRUE AUTOUIC TRUE AUTORCC TRUE AUTOGEN_PARALLEL AUTO
)

set_property(TARGET obs-studio APPEND PROPERTY AUTOUIC_SEARCH_PATHS forms forms/source-toolbar)

target_sources(
  obs-studio
  PRIVATE
    forms/AutoConfigFinishPage.ui
    forms/AutoConfigStartPage.ui
    forms/AutoConfigStartPage.ui
    forms/AutoConfigStreamPage.ui
    forms/AutoConfigTestPage.ui
    forms/AutoConfigVideoPage.ui
    forms/ColorSelect.ui
    forms/LogUploadDialog.ui
    forms/OBSAbout.ui
    forms/OBSAdvAudio.ui
    forms/OBSBasic.ui
    forms/OBSBasicControls.ui
    forms/OBSBasicFilters.ui
    forms/OBSBasicInteraction.ui
    forms/OBSBasicProperties.ui
    forms/OBSBasicSettings.ui
    forms/OBSBasicSourceSelect.ui
    forms/OBSBasicVCamConfig.ui
    forms/OBSExtraBrowsers.ui
    forms/OBSImporter.ui
    forms/OBSMissingFiles.ui
    forms/OBSRemux.ui
    forms/StatusBarWidget.ui
    forms/obs.qrc
    forms/PluginManagerWindow.ui
    forms/source-toolbar/browser-source-toolbar.ui
    forms/source-toolbar/color-source-toolbar.ui
    forms/source-toolbar/device-select-toolbar.ui
    forms/source-toolbar/game-capture-toolbar.ui
    forms/source-toolbar/image-source-toolbar.ui
    forms/source-toolbar/media-controls.ui
    forms/source-toolbar/text-source-toolbar.ui
)
