cmake_minimum_required(VERSION 3.28...3.30)

add_subdirectory(api)

option(ENABLE_FRONTEND "Enable building with UI frontend (requires Qt6)" ON)

if(NOT ENABLE_FRONTEND)
  target_disable_feature(obs "User Interface")
  return()
else()
  target_enable_feature(obs "User Interface")
endif()

find_package(FFmpeg REQUIRED COMPONENTS avcodec avutil avformat)
find_package(CURL REQUIRED)

if(NOT TARGET OBS::json11)
  add_subdirectory("${CMAKE_SOURCE_DIR}/deps/json11" json11)
endif()

if(NOT TARGET OBS::libobs)
  add_subdirectory("${CMAKE_SOURCE_DIR}/libobs" libobs)
endif()

if(NOT TARGET OBS::bpm)
  add_subdirectory("${CMAKE_SOURCE_DIR}/shared/bpm" bpm)
endif()

add_executable(obs-studio)
add_executable(OBS::studio ALIAS obs-studio)

target_link_libraries(
  obs-studio
  PRIVATE
    CURL::libcurl
    FFmpeg::avcodec
    FFmpeg::avutil
    FFmpeg::avformat
    OBS::libobs
    OBS::frontend-api
    OBS::json11
    OBS::bpm
)

include(cmake/ui-components.cmake)
include(cmake/ui-dialogs.cmake)
include(cmake/ui-docks.cmake)
include(cmake/feature-importers.cmake)
include(cmake/feature-plugin-manager.cmake)
include(cmake/ui-models.cmake)
include(cmake/ui-oauth.cmake)
include(cmake/feature-browserpanels.cmake)
include(cmake/ui-qt.cmake)
include(cmake/ui-settings.cmake)
include(cmake/ui-utility.cmake)
include(cmake/ui-widgets.cmake)
include(cmake/ui-wizards.cmake)

if(NOT OAUTH_BASE_URL)
  set(OAUTH_BASE_URL "https://auth.obsproject.com/" CACHE STRING "Default OAuth base URL")
  mark_as_advanced(OAUTH_BASE_URL)
endif()
include(cmake/feature-twitch.cmake)
include(cmake/feature-restream.cmake)
include(cmake/feature-youtube.cmake)
include(cmake/feature-whatsnew.cmake)
include(cmake/feature-idian-playground.cmake)

add_subdirectory(plugins)

configure_file(cmake/templates/ui-config.h.in ui-config.h)

target_sources(
  obs-studio
  PRIVATE obs-main.cpp OBSStudioAPI.cpp OBSStudioAPI.hpp OBSApp.cpp OBSApp.hpp OBSApp_Themes.cpp ui-config.h
)

if(OS_WINDOWS)
  include(cmake/os-windows.cmake)
elseif(OS_MACOS)
  include(cmake/os-macos.cmake)
elseif(OS_LINUX)
  include(cmake/os-linux.cmake)
elseif(OS_FREEBSD OR OS_OPENBSD)
  include(cmake/os-freebsd.cmake)
endif()

foreach(graphics_library IN ITEMS opengl metal d3d11)
  string(TOUPPER ${graphics_library} graphics_library_U)
  if(TARGET OBS::libobs-${graphics_library})
    target_compile_definitions(
      obs-studio
      PRIVATE
        DL_${graphics_library_U}="$<$<IF:$<PLATFORM_ID:Windows>,TARGET_FILE_NAME,TARGET_SONAME_FILE_NAME>:OBS::libobs-${graphics_library}>"
    )
  else()
    target_compile_definitions(obs-studio PRIVATE DL_${graphics_library_U}="")
  endif()
endforeach()

get_property(obs_module_list GLOBAL PROPERTY OBS_MODULES_ENABLED)
list(JOIN obs_module_list "|" SAFE_MODULES)
target_compile_definitions(obs-studio PRIVATE "SAFE_MODULES=\"${SAFE_MODULES}\"")

get_target_property(target_sources obs-studio SOURCES)
set(target_cpp_sources ${target_sources})
set(target_hpp_sources ${target_sources})
set(target_qt_sources ${target_sources})
list(FILTER target_cpp_sources INCLUDE REGEX ".+\\.(cpp|mm|c|m)")
list(SORT target_cpp_sources COMPARE NATURAL CASE SENSITIVE ORDER ASCENDING)
list(FILTER target_hpp_sources INCLUDE REGEX ".+\\.(hpp|h)")
list(SORT target_hpp_sources COMPARE NATURAL CASE SENSITIVE ORDER ASCENDING)
list(FILTER target_qt_sources INCLUDE REGEX ".+\\.(ui|qrc)")
list(SORT target_qt_sources COMPARE NATURAL CASE SENSITIVE ORDER ASCENDING)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Source Files" FILES ${target_cpp_sources})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Header Files" FILES ${target_hpp_sources})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Qt Files" FILES ${target_qt_sources})

set_target_properties_obs(obs-studio PROPERTIES FOLDER frontend OUTPUT_NAME "$<IF:$<PLATFORM_ID:Windows>,obs64,obs>")
