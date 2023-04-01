find_qt(COMPONENTS Widgets Network Svg Xml COMPONENTS_LINUX Gui)

target_link_libraries(obs-studio PRIVATE Qt::Widgets Qt::Svg Qt::Xml Qt::Network)

set_target_properties(
  obs-studio
  PROPERTIES AUTOMOC ON
             AUTOUIC ON
             AUTORCC ON)

set_property(
  TARGET obs-studio
  APPEND
  PROPERTY AUTOUIC_SEARCH_PATHS forms forms/source-toolbar)

set(_qt_sources
    forms/obs.qrc
    forms/AutoConfigFinishPage.ui
    forms/AutoConfigStartPage.ui
    forms/AutoConfigStartPage.ui
    forms/AutoConfigStreamPage.ui
    forms/AutoConfigTestPage.ui
    forms/AutoConfigVideoPage.ui
    forms/ColorSelect.ui
    forms/OBSAbout.ui
    forms/OBSAdvAudio.ui
    forms/OBSBasic.ui
    forms/OBSBasicFilters.ui
    forms/OBSBasicInteraction.ui
    forms/OBSBasicSettings.ui
    forms/OBSBasicSourceSelect.ui
    forms/OBSBasicTransform.ui
    forms/OBSBasicVCamConfig.ui
    forms/OBSExtraBrowsers.ui
    forms/OBSImporter.ui
    forms/OBSLogReply.ui
    forms/OBSMissingFiles.ui
    forms/OBSRemux.ui
    forms/OBSUpdate.ui
    forms/OBSYoutubeActions.ui
    forms/source-toolbar/browser-source-toolbar.ui
    forms/source-toolbar/color-source-toolbar.ui
    forms/source-toolbar/device-select-toolbar.ui
    forms/source-toolbar/game-capture-toolbar.ui
    forms/source-toolbar/image-source-toolbar.ui
    forms/source-toolbar/media-controls.ui
    forms/source-toolbar/text-source-toolbar.ui)

target_sources(obs-studio PRIVATE ${_qt_sources})

source_group(
  TREE "${CMAKE_CURRENT_SOURCE_DIR}/forms"
  PREFIX "UI Files"
  FILES ${_qt_sources})

unset(_qt_sources)
