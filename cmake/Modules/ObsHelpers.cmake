# Set OS-specific constants in non-deprecated way
include(GNUInstallDirs)
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  include(ObsDefaults_macOS)
  set(OS_MACOS ON)
  set(OS_POSIX ON)
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux|FreeBSD|OpenBSD")
  include(ObsDefaults_Linux)
  set(OS_POSIX ON)
  string(TOUPPER "${CMAKE_SYSTEM_NAME}" _SYSTEM_NAME_U)
  set(OS_${_SYSTEM_NAME_U} ON)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  include(ObsDefaults_Windows)
  set(OS_WINDOWS ON)
  set(OS_POSIX OFF)
endif()

# Create global property to hold list of activated modules
set_property(GLOBAL PROPERTY OBS_MODULE_LIST "")

# ##############################################################################
# GLOBAL HELPER FUNCTIONS #
# ##############################################################################

# Helper function to set up runtime or library targets
function(setup_binary_target target)
  # Set up installation paths for program install
  install(
    TARGETS ${target}
    RUNTIME DESTINATION ${OBS_EXECUTABLE_DESTINATION}
            COMPONENT ${target}_Runtime
    LIBRARY DESTINATION ${OBS_LIBRARY_DESTINATION}
            COMPONENT ${target}_Runtime
            NAMELINK_COMPONENT ${target}_Development
    ARCHIVE DESTINATION ${OBS_LIBRARY_DESTINATION}
            COMPONENT ${target}_Development
    PUBLIC_HEADER
      DESTINATION ${OBS_INCLUDE_DESTINATION}
      COMPONENT ${target}_Development
      EXCLUDE_FROM_ALL)

  # Set up installation paths for development rundir
  install(
    TARGETS ${target}
    RUNTIME DESTINATION ${OBS_EXECUTABLE_DESTINATION}
            COMPONENT obs_${target}
            EXCLUDE_FROM_ALL
    LIBRARY DESTINATION ${OBS_LIBRARY_DESTINATION}
            COMPONENT obs_${target}
            EXCLUDE_FROM_ALL
    PUBLIC_HEADER
      DESTINATION ${OBS_INCLUDE_DESTINATION}
      COMPONENT obs_${target}
      EXCLUDE_FROM_ALL)

  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMAND
      "${CMAKE_COMMAND}" --install .. --config $<CONFIG> --prefix
      ${OBS_OUTPUT_DIR}/$<CONFIG> --component obs_${target} >
      "$<IF:$<PLATFORM_ID:Windows>,nul,/dev/null>"
    COMMENT "Installing OBS rundir"
    VERBATIM)

endfunction()

# Helper function to set up OBS plugin targets
function(setup_plugin_target target)
  set_target_properties(${target} PROPERTIES PREFIX "")

  install(
    TARGETS ${target}
    RUNTIME DESTINATION ${OBS_PLUGIN_DESTINATION} COMPONENT ${target}_Runtime
    LIBRARY DESTINATION ${OBS_PLUGIN_DESTINATION}
            COMPONENT ${target}_Runtime
            NAMELINK_COMPONENT ${target}_Development)

  install(
    TARGETS ${target}
    RUNTIME DESTINATION ${OBS_PLUGIN_DESTINATION}
            COMPONENT obs_${target}
            EXCLUDE_FROM_ALL
    LIBRARY DESTINATION ${OBS_PLUGIN_DESTINATION}
            COMPONENT obs_${target}
            EXCLUDE_FROM_ALL)

  setup_target_resources("${target}" "obs-plugins/${target}")
  set_property(GLOBAL APPEND PROPERTY OBS_MODULE_LIST "${target}")
  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMAND
      "${CMAKE_COMMAND}" --install .. --config $<CONFIG> --prefix
      ${OBS_OUTPUT_DIR}/$<CONFIG> --component obs_${target} >
      "$<IF:$<PLATFORM_ID:Windows>,nul,/dev/null>"
    COMMENT "Installing ${target} to OBS rundir"
    VERBATIM)

  obs_status(ENABLED "${target}")
endfunction()

# Helper function to set up OBS scripting plugin targets
function(setup_script_plugin_target target)
  set_target_properties(${target} PROPERTIES PREFIX "")

  install(
    TARGETS ${target}
    LIBRARY DESTINATION ${OBS_SCRIPT_PLUGIN_DESTINATION}
            COMPONENT ${target}_Runtime
            NAMELINK_COMPONENT ${target}_Development)

  install(
    TARGETS ${target}
    LIBRARY DESTINATION ${OBS_SCRIPT_PLUGIN_DESTINATION}
            COMPONENT obs_${target}
            EXCLUDE_FROM_ALL)

  if(${target} STREQUAL "obspython")
    install(
      FILES "$<TARGET_FILE_DIR:${target}>/$<TARGET_FILE_BASE_NAME:${target}>.py"
      DESTINATION ${OBS_SCRIPT_PLUGIN_DESTINATION}
      COMPONENT ${target}_Runtime)

    install(
      FILES "$<TARGET_FILE_DIR:${target}>/$<TARGET_FILE_BASE_NAME:${target}>.py"
      DESTINATION ${OBS_SCRIPT_PLUGIN_DESTINATION}
      COMPONENT obs_${target}
      EXCLUDE_FROM_ALL)
  endif()
  set_property(GLOBAL APPEND PROPERTY OBS_SCRIPTING_MODULE_LIST "${target}")
  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMAND
      "${CMAKE_COMMAND}" --install .. --config $<CONFIG> --prefix
      ${OBS_OUTPUT_DIR}/$<CONFIG> --component obs_${target} >
      "$<IF:$<PLATFORM_ID:Windows>,nul,/dev/null>"
    COMMENT "Installing ${target} to OBS rundir"
    VERBATIM)

  obs_status(ENABLED "${target}")
endfunction()

# Helper function to set up target resources (e.g. L10N files)
function(setup_target_resources target destination)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data")
    install(
      DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data/
      DESTINATION ${OBS_DATA_DESTINATION}/${destination}
      USE_SOURCE_PERMISSIONS
      COMPONENT ${target}_Runtime)

    install(
      DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data/
      DESTINATION ${OBS_DATA_DESTINATION}/${destination}
      USE_SOURCE_PERMISSIONS
      COMPONENT obs_${target}
      EXCLUDE_FROM_ALL)
  endif()
endfunction()

# Helper function to set up specific resource files for targets
function(add_target_resource target resource destination)
  install(
    FILES ${resource}
    DESTINATION ${OBS_DATA_DESTINATION}/${destination}
    COMPONENT ${target}_Runtime)

  install(
    FILES ${resource}
    DESTINATION ${OBS_DATA_DESTINATION}/${destination}
    COMPONENT obs_${target}
    EXCLUDE_FROM_ALL)
endfunction()

# Helper function to set up OBS app target
function(setup_obs_app target)
  setup_binary_target(${target})

  get_property(OBS_MODULE_LIST GLOBAL PROPERTY OBS_MODULE_LIST)
  list(LENGTH OBS_MODULE_LIST _LEN)
  if(_LEN GREATER 0)
    add_dependencies(${target} ${OBS_MODULE_LIST})
  endif()

  get_property(OBS_SCRIPTING_MODULE_LIST GLOBAL
               PROPERTY OBS_SCRIPTING_MODULE_LIST)
  list(LENGTH OBS_SCRIPTING_MODULE_LIST _LEN)
  if(_LEN GREATER 0)
    add_dependencies(${target} ${OBS_SCRIPTING_MODULE_LIST})
  endif()

  # detect outdated obs-browser submodule
  if(NOT TARGET OBS::browser AND TARGET obs-browser)
    target_compile_features(obs-browser-page PRIVATE cxx_std_17)

    add_library(OBS::browser ALIAS obs-browser)

    if(NOT TARGET OBS::browser-panels AND BROWSER_PANEL_SUPPORT_ENABLED)
      add_library(OBS::browser-panels ALIAS obs-browser)
    endif()
  endif()

  if(TARGET OBS::browser)
    setup_target_browser(${target})
  endif()

  if(TARGET OBS::ffmpeg-mux)
    add_dependencies(${target} OBS::ffmpeg-mux)
  endif()

  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMAND
      "${CMAKE_COMMAND}" --install .. --config $<CONFIG> --prefix
      ${OBS_OUTPUT_DIR}/$<CONFIG> --component obs_rundir >
      "$<IF:$<PLATFORM_ID:Windows>,nul,/dev/null>"
    COMMENT "Installing OBS rundir"
    VERBATIM)
endfunction()

# Helper function to do additional setup for browser source plugin
function(setup_target_browser target)
  install(
    DIRECTORY ${CEF_ROOT_DIR}/Resources/
    DESTINATION ${OBS_PLUGIN_DESTINATION}
    COMPONENT ${target}_Runtime)

  install(
    DIRECTORY ${CEF_ROOT_DIR}/Release/
    DESTINATION ${OBS_PLUGIN_DESTINATION}
    COMPONENT ${target}_Runtime)

  install(
    DIRECTORY ${CEF_ROOT_DIR}/Resources/
    DESTINATION ${OBS_OUTPUT_DIR}/$<CONFIG>/${OBS_PLUGIN_DESTINATION}
    COMPONENT obs_rundir
    EXCLUDE_FROM_ALL)

  install(
    DIRECTORY ${CEF_ROOT_DIR}/Release/
    DESTINATION ${OBS_OUTPUT_DIR}/$<CONFIG>/${OBS_PLUGIN_DESTINATION}
    COMPONENT obs_rundir
    EXCLUDE_FROM_ALL)
endfunction()

# Helper function to export target to build and install tree. Allows usage of
# `find_package(libobs)` by other build trees
function(export_target target)
  set(CMAKE_EXPORT_PACKAGE_REGISTRY OFF)

  install(
    TARGETS ${target}
    EXPORT ${target}Targets
    RUNTIME DESTINATION ${OBS_EXECUTABLE_DESTINATION}
            COMPONENT obs_libraries
            EXCLUDE_FROM_ALL
    LIBRARY DESTINATION ${OBS_LIBRARY_DESTINATION}
            COMPONENT obs_libraries
            EXCLUDE_FROM_ALL
    ARCHIVE DESTINATION ${OBS_LIBRARY_DESTINATION}
            COMPONENT obs_libraries
            EXCLUDE_FROM_ALL
    INCLUDES
    DESTINATION ${OBS_INCLUDE_DESTINATION}
    PUBLIC_HEADER
      DESTINATION ${OBS_INCLUDE_DESTINATION}
      COMPONENT obs_libraries
      EXCLUDE_FROM_ALL)

  include(GenerateExportHeader)
  generate_export_header(${target} EXPORT_FILE_NAME
                         ${CMAKE_CURRENT_BINARY_DIR}/${target}_EXPORT.h)

  target_sources(${target}
                 PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/${target}_EXPORT.h)

  set(TARGETS_EXPORT_NAME "${target}Targets")
  include(CMakePackageConfigHelpers)
  configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/${target}Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${target}Config.cmake
    INSTALL_DESTINATION ${OBS_CMAKE_DESTINATION}/${target}
    PATH_VARS OBS_PLUGIN_DESTINATION OBS_DATA_DESTINATION)

  write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${target}ConfigVersion.cmake
    VERSION ${OBS_VERSION_CANONICAL}
    COMPATIBILITY SameMajorVersion)

  export(
    EXPORT ${target}Targets
    FILE ${CMAKE_CURRENT_BINARY_DIR}/${TARGETS_EXPORT_NAME}.cmake
    NAMESPACE OBS::)

  export(PACKAGE "${target}")

  install(
    EXPORT ${TARGETS_EXPORT_NAME}
    FILE ${TARGETS_EXPORT_NAME}.cmake
    NAMESPACE OBS::
    DESTINATION ${OBS_CMAKE_DESTINATION}/${target}
    COMPONENT obs_libraries
    EXCLUDE_FROM_ALL)

  install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/${target}Config.cmake
          ${CMAKE_CURRENT_BINARY_DIR}/${target}ConfigVersion.cmake
    DESTINATION ${OBS_CMAKE_DESTINATION}/${target}
    COMPONENT obs_libraries
    EXCLUDE_FROM_ALL)
endfunction()

# Helper function to define available graphics modules for targets
function(define_graphic_modules target)
  foreach(_GRAPHICS_API metal d3d11 opengl d3d9)
    string(TOUPPER ${_GRAPHICS_API} _GRAPHICS_API_u)
    if(TARGET OBS::libobs-${_GRAPHICS_API})
      if(OS_POSIX AND NOT LINUX_PORTABLE)
        target_compile_definitions(
          ${target}
          PRIVATE
            DL_${_GRAPHICS_API_u}="$<TARGET_SONAME_FILE_NAME:libobs-${_GRAPHICS_API}>"
        )
      else()
        target_compile_definitions(
          ${target}
          PRIVATE
            DL_${_GRAPHICS_API_u}="$<TARGET_FILE_NAME:libobs-${_GRAPHICS_API}>")
      endif()
      add_dependencies(${target} OBS::libobs-${_GRAPHICS_API})
    else()
      target_compile_definitions(${target} PRIVATE DL_${_GRAPHICS_API_u}="")
    endif()
  endforeach()
endfunction()

macro(find_qt)
  set(oneValueArgs VERSION)
  set(multiValueArgs COMPONENTS COMPONENTS_WIN COMPONENTS_MAC COMPONENTS_LINUX)
  cmake_parse_arguments(FIND_QT "" "${oneValueArgs}" "${multiValueArgs}"
                        ${ARGN})

  if(OS_WINDOWS)
    find_package(
      Qt${FIND_QT_VERSION}
      COMPONENTS ${FIND_QT_COMPONENTS} ${FIND_QT_COMPONENTS_WIN}
      REQUIRED)
  elseif(OS_MACOS)
    find_package(
      Qt${FIND_QT_VERSION}
      COMPONENTS ${FIND_QT_COMPONENTS} ${FIND_QT_COMPONENTS_MAC}
      REQUIRED)
  else()
    find_package(
      Qt${FIND_QT_VERSION}
      COMPONENTS ${FIND_QT_COMPONENTS} ${FIND_QT_COMPONENTS_LINUX}
      REQUIRED)
  endif()

  foreach(_COMPONENT IN LISTS FIND_QT_COMPONENTS FIND_QT_COMPONENTS_WIN
                              FIND_QT_COMPONENTS_MAC FIND_QT_COMPONENTS_LINUX)
    if(NOT TARGET Qt::${_COMPONENT} AND TARGET
                                        Qt${FIND_QT_VERSION}::${_COMPONENT})

      add_library(Qt::${_COMPONENT} INTERFACE IMPORTED)
      set_target_properties(
        Qt::${_COMPONENT} PROPERTIES INTERFACE_LINK_LIBRARIES
                                     "Qt${FIND_QT_VERSION}::${_COMPONENT}")
    endif()
  endforeach()
endmacro()

# Idea adapted from: https://github.com/edsiper/cmake-options
macro(set_option option value)
  set(${option}
      ${value}
      CACHE INTERNAL "")
endmacro()

function(obs_status status text)
  set(_OBS_STATUS_DISABLED "OBS:  DISABLED   ")
  set(_OBS_STATUS_ENABLED "OBS:  ENABLED    ")
  set(_OBS_STATUS "OBS:  ")
  if(status STREQUAL "DISABLED")
    message(STATUS "${_OBS_STATUS_DISABLED}${text}")
  elseif(status STREQUAL "ENABLED")
    message(STATUS "${_OBS_STATUS_ENABLED}${text}")
  else()
    message(${status} "${_OBS_STATUS}${text}")
  endif()
endfunction()

if(OS_WINDOWS)
  include(ObsHelpers_Windows)
elseif(OS_MACOS)
  include(ObsHelpers_macOS)
elseif(OS_POSIX)
  include(ObsHelpers_Linux)
endif()

# ##############################################################################
# LEGACY FALLBACKS     #
# ##############################################################################

# Helper function to install OBS plugin with associated resource directory
function(_install_obs_plugin_with_data target source)
  setup_plugin_target(${target})

  if(NOT ${source} STREQUAL "data"
     AND IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${source}"
     AND NOT OS_MACOS)
    install(
      DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${source}/
      DESTINATION ${OBS_DATA_DESTINATION}/obs-plugins/${target}
      USE_SOURCE_PERMISSIONS
      COMPONENT ${target}_Runtime)

    install(
      DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${source}/
      DESTINATION
        ${OBS_OUTPUT_DIR}/$<CONFIG>/${OBS_DATA_DESTINATION}/obs-plugins/${target}
      COMPONENT obs_${target}
      EXCLUDE_FROM_ALL)

    if(OS_WINDOWS AND DEFINED ENV{obsInstallerTempDir})
      install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${source}/
        DESTINATION
          $ENV{obsInstallerTempDir}/${OBS_DATA_DESTINATION}/obs-plugins/${target}
        COMPONENT obs_${target}
        EXCLUDE_FROM_ALL)
    endif()
  endif()
endfunction()

# Helper function to install OBS plugin
function(_install_obs_plugin target)
  setup_plugin_target(${target})
endfunction()

# Helper function to install data for a target only
function(_install_obs_datatarget target destination)
  install(
    TARGETS ${target}
    LIBRARY DESTINATION ${OBS_DATA_DESTINATION}/${destination}
            COMPONENT ${target}_Runtime
            NAMELINK_COMPONENT ${target}_Development
    RUNTIME DESTINATION ${OBS_DATA_DESTINATION}/${destination}
            COMPONENT ${target}_Runtime)

  install(
    TARGETS ${target}
    LIBRARY DESTINATION ${OBS_DATA_DESTINATION}/${destination}
            COMPONENT obs_${target}
            EXCLUDE_FROM_ALL
    RUNTIME DESTINATION ${OBS_DATA_DESTINATION}/${destination}
            COMPONENT obs_${target}
            EXCLUDE_FROM_ALL)

  if(OS_WINDOWS)
    if(MSVC)
      add_target_resource(${target} "$<TARGET_PDB_FILE:${target}>"
                          "${destination}" OPTIONAL)
    endif()

    if(DEFINED ENV{obsInstallerTempDir})
      install(
        TARGETS ${target}
        RUNTIME
          DESTINATION
            $ENV{obsInstallerTempDir}/${OBS_DATA_DESTINATION}/${destination}/$<TARGET_FILE_NAME:${target}>
          COMPONENT obs_${target}
          EXCLUDE_FROM_ALL
        LIBRARY
          DESTINATION
            $ENV{obsInstallerTempDir}/${OBS_DATA_DESTINATION}/${destination}/$<TARGET_FILE_NAME:${target}>
          COMPONENT obs_${target}
          EXCLUDE_FROM_ALL)
    endif()
  endif()

  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMAND
      "${CMAKE_COMMAND}" --install .. --config $<CONFIG> --prefix
      ${OBS_OUTPUT_DIR}/$<CONFIG> --component obs_${target} >
      "$<IF:$<PLATFORM_ID:Windows>,nul,/dev/null>"
    COMMENT "Installing ${target} to OBS rundir"
    VERBATIM)
endfunction()
