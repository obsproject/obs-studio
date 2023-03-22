# OBS CMake macOS CPack configuration module

include(cpackconfig_common)

# Set architecture suffix for package file name based on project target
if(CMAKE_OSX_ARCHITECTURES STREQUAL arm64)
  set(_cpack_arch_suffix Apple)
elseif(CMAKE_OSX_ARCHITECTURES STREQUAL x86_64)
  set(_cpack_arch_suffix Intel)
else()
  set(_cpack_arch_suffix Universal)
endif()

if(ENABLE_RELEASE_BUILD)
  set(CPACK_PACKAGE_VERSION ${OBS_VERSION_CANONICAL})
else()
  set(CPACK_PACKAGE_VERSION ${OBS_VERSION})
endif()

set(CPACK_PACKAGE_NAME "OBS")
set(CPACK_PACKAGE_FILE_NAME "obs-studio-${CPACK_PACKAGE_VERSION}-macOS-${_cpack_arch_suffix}")
set(CPACK_COMPONENTS_ALL Application)
set(CPACK_COMPONENT_Application_DISPLAY_NAME "OBS Studio")

# Set background image and icon for generated Drag&Drop disk image
set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_SOURCE_DIR}/cmake/macos/resources/background.tiff")
set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/cmake/macos/resources/AppIcon.icns")
cmake_path(GET CPACK_DMG_BACKGROUND_IMAGE FILENAME _cpack_dmg_background_filename)

set(CPACK_GENERATOR DragNDrop)
set(CPACK_DMG_FORMAT UDZO)
set(CPACK_DMG_FILESYSTEM APFS)
set(CPACK_DMG_DS_STORE_SETUP_SCRIPT "${CMAKE_BINARY_DIR}/package.applescript")

set(_dmg_window_x 100)
set(_dmg_window_y 100)
set(_dmg_window_width 540)
set(_dmg_window_height 380)
set(_dmg_icon_size 96)
set(_dmg_text_size 16)
set(_dmg_obs_x 124)
set(_dmg_obs_y 180)
set(_dmg_app_link_x 416)
set(_dmg_app_link_y 180)

configure_file("${CMAKE_SOURCE_DIR}/cmake/macos/resources/package.applescript.in"
               "${CMAKE_BINARY_DIR}/package.applescript" @ONLY)

include(CPack)
