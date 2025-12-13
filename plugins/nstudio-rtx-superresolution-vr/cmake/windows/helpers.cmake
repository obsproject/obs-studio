# CMake Windows helper functions module

# cmake-format: off
# cmake-lint: disable=C0103
# cmake-format: on

include_guard(GLOBAL)

include(helpers_common)

# set_target_properties_plugin: Set target properties for use in obs-studio
function(set_target_properties_plugin target)
  set(options "")
  set(oneValueArgs "")
  set(multiValueArgs PROPERTIES)
  cmake_parse_arguments(PARSE_ARGV 0 _STPO "${options}" "${oneValueArgs}" "${multiValueArgs}")

  message(DEBUG "Setting additional properties for target ${target}...")

  while(_STPO_PROPERTIES)
    list(POP_FRONT _STPO_PROPERTIES key value)
    set_property(TARGET ${target} PROPERTY ${key} "${value}")
  endwhile()

  string(TIMESTAMP CURRENT_YEAR "%Y")

  set_target_properties(${target} PROPERTIES VERSION 0 SOVERSION ${PLUGIN_VERSION})

  install(
    TARGETS ${target}
    RUNTIME DESTINATION bin/64bit
    LIBRARY DESTINATION obs-plugins/64bit)

  install(
    FILES "$<TARGET_PDB_FILE:${target}>"
    CONFIGURATIONS RelWithDebInfo Debug Release
    DESTINATION obs-plugins/64bit
    OPTIONAL)

  if(OBS_BUILD_DIR)
    add_custom_command(
      TARGET ${target}
      POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${OBS_BUILD_DIR}/obs-plugins/64bit"
      COMMAND
        "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:${target}>"
        "$<$<CONFIG:Debug,RelWithDebInfo,Release>:$<TARGET_PDB_FILE:${target}>>" "${OBS_BUILD_DIR}/obs-plugins/64bit"
      COMMENT "Copy ${target} to obs-studio directory ${OBS_BUILD_DIR}"
      VERBATIM)
  endif()

  if(TARGET plugin-support)
    target_link_libraries(${target} PRIVATE plugin-support)
  endif()

  target_install_resources(${target})

  get_target_property(target_sources ${target} SOURCES)
  set(target_ui_files ${target_sources})
  list(FILTER target_ui_files INCLUDE REGEX ".+\\.(ui|qrc)")
  source_group(
    TREE "${CMAKE_CURRENT_SOURCE_DIR}"
    PREFIX "UI Files"
    FILES ${target_ui_files})

  set(valid_uuid FALSE)
  check_uuid(${_windowsAppUUID} valid_uuid)
  if(NOT valid_uuid)
    message(FATAL_ERROR "Specified Windows package UUID is not a valid UUID value: ${_windowsAppUUID}")
  else()
    set(UUID_APP ${_windowsAppUUID})
  endif()

  configure_file(cmake/windows/resources/installer-Windows.iss.in
                 "${CMAKE_CURRENT_BINARY_DIR}/installer-Windows.generated.iss")

  configure_file(cmake/windows/resources/resource.rc.in "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.rc")
  target_sources(${CMAKE_PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.rc")
endfunction()

# Helper function to add resources into bundle
function(target_install_resources target)
  message(DEBUG "Installing resources for target ${target}...")
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data")
    file(GLOB_RECURSE data_files "${CMAKE_CURRENT_SOURCE_DIR}/data/*")
    foreach(data_file IN LISTS data_files)
      cmake_path(RELATIVE_PATH data_file BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/data/" OUTPUT_VARIABLE
                 relative_path)
      cmake_path(GET relative_path PARENT_PATH relative_path)
      target_sources(${target} PRIVATE "${data_file}")
      source_group("Resources/${relative_path}" FILES "${data_file}")
    endforeach()

    install(
      DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/data/"
      DESTINATION data/obs-plugins/${target}
      USE_SOURCE_PERMISSIONS)

    if(OBS_BUILD_DIR)
      add_custom_command(
        TARGET ${target}
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${OBS_BUILD_DIR}/data/obs-plugins/${target}"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/data"
                "${OBS_BUILD_DIR}/data/obs-plugins/${target}"
        COMMENT "Copy ${target} resources to data directory"
        VERBATIM)
    endif()
  endif()
endfunction()

# Helper function to add a specific resource to a bundle
function(target_add_resource target resource)
  message(DEBUG "Add resource '${resource}' to target ${target} at destination '${target_destination}'...")

  install(
    FILES "${resource}"
    DESTINATION data/obs-plugins/${target}
    COMPONENT Runtime)

  if(OBS_BUILD_DIR)
    add_custom_command(
      TARGET ${target}
      POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${OBS_BUILD_DIR}/data/obs-plugins/${target}"
      COMMAND "${CMAKE_COMMAND}" -E copy "${resource}" "${OBS_BUILD_DIR}/data/obs-plugins/${target}"
      COMMENT "Copy ${target} resource ${resource} to library directory"
      VERBATIM)
  endif()
  source_group("Resources" FILES "${resource}")
endfunction()
