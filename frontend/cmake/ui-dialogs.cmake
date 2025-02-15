if(NOT TARGET OBS::properties-view)
  add_subdirectory("${CMAKE_SOURCE_DIR}/shared/properties-view" "${CMAKE_BINARY_DIR}/shared/properties-view")
endif()

target_link_libraries(obs-studio PRIVATE OBS::properties-view)

target_sources(
  obs-studio
  PRIVATE
    dialogs/NameDialog.cpp
    dialogs/NameDialog.hpp
    dialogs/OAuthLogin.cpp
    dialogs/OAuthLogin.hpp
    dialogs/OBSAbout.cpp
    dialogs/OBSAbout.hpp
    dialogs/OBSBasicAdvAudio.cpp
    dialogs/OBSBasicAdvAudio.hpp
    dialogs/OBSBasicFilters.cpp
    dialogs/OBSBasicFilters.hpp
    dialogs/OBSBasicInteraction.cpp
    dialogs/OBSBasicInteraction.hpp
    dialogs/OBSBasicProperties.cpp
    dialogs/OBSBasicProperties.hpp
    dialogs/OBSBasicSourceSelect.cpp
    dialogs/OBSBasicSourceSelect.hpp
    dialogs/OBSBasicTransform.cpp
    dialogs/OBSBasicTransform.hpp
    dialogs/OBSBasicVCamConfig.cpp
    dialogs/OBSBasicVCamConfig.hpp
    dialogs/OBSLogReply.cpp
    dialogs/OBSLogReply.hpp
    dialogs/OBSLogViewer.cpp
    dialogs/OBSLogViewer.hpp
    dialogs/OBSMissingFiles.cpp
    dialogs/OBSMissingFiles.hpp
    dialogs/OBSRemux.cpp
    dialogs/OBSRemux.hpp
    dialogs/OBSWhatsNew.cpp
    dialogs/OBSWhatsNew.hpp
)
