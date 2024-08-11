# OBS CMake Windows helper functions module

# cmake-format: off
# cmake-lint: disable=C0103
# cmake-lint: disable=R0912
# cmake-lint: disable=R0915
# cmake-format: on

include_guard(GLOBAL)

include(helpers_common)

# set_target_properties_obs: Set target properties for use in obs-studio
function(set_target_properties_obs target)
  set(options "")
  set(oneValueArgs "")
  set(multiValueArgs PROPERTIES)
  cmake_parse_arguments(PARSE_ARGV 0 _STPO "${options}" "${oneValueArgs}" "${multiValueArgs}")

  message(DEBUG "Setting additional properties for target ${target}...")

  while(_STPO_PROPERTIES)
    list(POP_FRONT _STPO_PROPERTIES key value)
    set_property(TARGET ${target} PROPERTY ${key} "${value}")
  endwhile()

  get_target_property(target_type ${target} TYPE)

  if(target_type STREQUAL EXECUTABLE)
    if(target STREQUAL obs-browser-helper)
      set(OBS_EXECUTABLE_DESTINATION "${OBS_PLUGIN_DESTINATION}")
    elseif(target STREQUAL inject-helper OR target STREQUAL get-graphics-offsets)
      set(OBS_EXECUTABLE_DESTINATION "${OBS_DATA_DESTINATION}/obs-plugins/win-capture")

      # cmake-format: off
      _target_install_obs(${target} DESTINATION ${OBS_EXECUTABLE_DESTINATION} 32BIT)
      # cmake-format: on
    endif()

    # cmake-format: off
    _target_install_obs(${target} DESTINATION ${OBS_EXECUTABLE_DESTINATION})
    # cmake-format: on

    if(target STREQUAL obs-studio)
      get_property(obs_executables GLOBAL PROPERTY _OBS_EXECUTABLES)
      get_property(obs_modules GLOBAL PROPERTY OBS_MODULES_ENABLED)
      add_dependencies(${target} ${obs_executables} ${obs_modules})
      _bundle_dependencies(${target})
      target_add_resource(${target} "${CMAKE_CURRENT_SOURCE_DIR}/../AUTHORS"
                          "${OBS_DATA_DESTINATION}/obs-studio/authors")
    elseif(target STREQUAL obs-browser-helper)
      set_property(GLOBAL APPEND PROPERTY _OBS_EXECUTABLES ${target})
      return()
    else()
      set_property(GLOBAL APPEND PROPERTY _OBS_EXECUTABLES ${target})
    endif()
  elseif(target_type STREQUAL SHARED_LIBRARY)
    set_target_properties(${target} PROPERTIES VERSION ${OBS_VERSION_MAJOR} SOVERSION ${OBS_VERSION_CANONICAL})

    # cmake-format: off
    _target_install_obs(
      ${target}
        DESTINATION "${OBS_EXECUTABLE_DESTINATION}"
        LIBRARY_DESTINATION "${OBS_LIBRARY_DESTINATION}"
        HEADER_DESTINATION "${OBS_INCLUDE_DESTINATION}")
    # cmake-format: on
  elseif(target_type STREQUAL MODULE_LIBRARY)
    set_target_properties(${target} PROPERTIES VERSION 0 SOVERSION ${OBS_VERSION_CANONICAL})

    if(target STREQUAL libobs-d3d11
       OR target STREQUAL libobs-opengl
       OR target STREQUAL libobs-winrt)
      set(target_destination "${OBS_EXECUTABLE_DESTINATION}")
    elseif(target STREQUAL "obspython" OR target STREQUAL "obslua")
      set(target_destination "${OBS_SCRIPT_PLUGIN_DESTINATION}")
    elseif(target STREQUAL graphics-hook)
      set(target_destination "${OBS_DATA_DESTINATION}/obs-plugins/win-capture")
      target_add_resource(graphics-hook "${CMAKE_CURRENT_SOURCE_DIR}/obs-vulkan64.json" "${target_destination}")
      target_add_resource(graphics-hook "${CMAKE_CURRENT_SOURCE_DIR}/obs-vulkan32.json" "${target_destination}")

      # cmake-format: off
      _target_install_obs(${target} DESTINATION ${target_destination} 32BIT)
      # cmake-format: on
    elseif(target STREQUAL obs-virtualcam-module)
      set(target_destination "${OBS_DATA_DESTINATION}/obs-plugins/win-dshow")

      # cmake-format: off
      _target_install_obs(${target} DESTINATION ${target_destination} 32BIT)
      # cmake-format: on
    else()
      set(target_destination "${OBS_PLUGIN_DESTINATION}")
    endif()

    # cmake-format: off
    _target_install_obs(${target} DESTINATION ${target_destination})
    # cmake-format: on

    if(${target} STREQUAL obspython)
      add_custom_command(
        TARGET ${target}
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E echo "Add obspython import module"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${OBS_OUTPUT_DIR}/$<CONFIG>/${OBS_SCRIPT_PLUGIN_DESTINATION}/"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE_DIR:obspython>/obspython.py"
                "${OBS_OUTPUT_DIR}/$<CONFIG>/${OBS_SCRIPT_PLUGIN_DESTINATION}/"
        COMMENT "")

      install(
        FILES "$<TARGET_FILE_DIR:obspython>/obspython.py"
        DESTINATION "${OBS_SCRIPT_PLUGIN_DESTINATION}"
        COMPONENT Runtime)
    elseif(${target} STREQUAL obs-browser)
      message(DEBUG "Add Chromium Embedded Framework to project for obs-browser plugin...")
      if(TARGET CEF::Library)
        get_target_property(imported_location CEF::Library IMPORTED_LOCATION_RELEASE)

        if(imported_location)
          cmake_path(GET imported_location PARENT_PATH cef_location)
          cmake_path(GET cef_location PARENT_PATH cef_root_location)
          add_custom_command(
            TARGET ${target}
            POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E echo "Add Chromium Embedded Framework to library directory"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${OBS_OUTPUT_DIR}/$<CONFIG>/${target_destination}"
            COMMAND
              "${CMAKE_COMMAND}" -E copy_if_different "${imported_location}" "${cef_location}/chrome_elf.dll"
              "${cef_location}/libEGL.dll" "${cef_location}/libGLESv2.dll" "${cef_location}/snapshot_blob.bin"
              "${cef_location}/v8_context_snapshot.bin" "${OBS_OUTPUT_DIR}/$<CONFIG>/${target_destination}"
            COMMAND
              "${CMAKE_COMMAND}" -E copy_if_different "${cef_root_location}/Resources/chrome_100_percent.pak"
              "${cef_root_location}/Resources/chrome_200_percent.pak" "${cef_root_location}/Resources/icudtl.dat"
              "${cef_root_location}/Resources/resources.pak" "${OBS_OUTPUT_DIR}/$<CONFIG>/${target_destination}/"
            COMMAND "${CMAKE_COMMAND}" -E copy_directory "${cef_root_location}/Resources/locales"
                    "${OBS_OUTPUT_DIR}/$<CONFIG>/${target_destination}/locales"
            COMMENT "")

          install(
            FILES "${imported_location}"
                  "${cef_location}/chrome_elf.dll"
                  "${cef_location}/libEGL.dll"
                  "${cef_location}/libGLESv2.dll"
                  "${cef_location}/snapshot_blob.bin"
                  "${cef_location}/v8_context_snapshot.bin"
                  "${cef_root_location}/Resources/chrome_100_percent.pak"
                  "${cef_root_location}/Resources/chrome_200_percent.pak"
                  "${cef_root_location}/Resources/icudtl.dat"
                  "${cef_root_location}/Resources/resources.pak"
            DESTINATION "${target_destination}"
            COMPONENT Runtime)

          install(
            DIRECTORY "${cef_root_location}/Resources/locales"
            DESTINATION "${target_destination}"
            USE_SOURCE_PERMISSIONS
            COMPONENT Runtime)
        endif()
      endif()
    endif()

    set_property(GLOBAL APPEND PROPERTY OBS_MODULES_ENABLED ${target})
  endif()

  target_link_options(${target} PRIVATE "/PDBALTPATH:$<TARGET_PDB_FILE_NAME:${target}>")
  target_install_resources(${target})

  get_target_property(target_sources ${target} SOURCES)
  set(target_ui_files ${target_sources})
  list(FILTER target_ui_files INCLUDE REGEX ".+\\.(ui|qrc)")
  source_group(
    TREE "${CMAKE_CURRENT_SOURCE_DIR}"
    PREFIX "UI Files"
    FILES ${target_ui_files})

  if(${target} STREQUAL libobs)
    set(target_source_files ${target_sources})
    set(target_header_files ${target_sources})
    list(FILTER target_source_files INCLUDE REGEX ".+\\.(m|c[cp]?p?|swift)")
    list(FILTER target_header_files INCLUDE REGEX ".+\\.h(pp)?")

    source_group(
      TREE "${CMAKE_CURRENT_SOURCE_DIR}"
      PREFIX "Source Files"
      FILES ${target_source_files})
    source_group(
      TREE "${CMAKE_CURRENT_SOURCE_DIR}"
      PREFIX "Header Files"
      FILES ${target_header_files})
  endif()
endfunction()

# _target_install_obs: Helper function to install build artifacts to rundir and install location
function(_target_install_obs target)
  set(options "32BIT")
  set(oneValueArgs "DESTINATION" "LIBRARY_DESTINATION" "HEADER_DESTINATION")
  set(multiValueArgs "")
  cmake_parse_arguments(PARSE_ARGV 0 _TIO "${options}" "${oneValueArgs}" "${multiValueArgs}")

  if(_TIO_32BIT)
    get_target_property(target_type ${target} TYPE)
    if(target_type STREQUAL EXECUTABLE)
      set(suffix exe)
    else()
      set(suffix dll)
    endif()

    cmake_path(RELATIVE_PATH CMAKE_CURRENT_SOURCE_DIR BASE_DIRECTORY "${OBS_SOURCE_DIR}" OUTPUT_VARIABLE project_path)

    set(32bit_project_path "${OBS_SOURCE_DIR}/build_x86/${project_path}")
    set(target_file "${32bit_project_path}/$<CONFIG>/${target}32.${suffix}")
    set(target_pdb_file "${32bit_project_path}/$<CONFIG>/${target}32.pdb")
    set(comment "Copy ${target} (32-bit) to destination")

    install(
      FILES "${32bit_project_path}/$<CONFIG>/${target}32.${suffix}"
      DESTINATION "${_TIO_DESTINATION}"
      COMPONENT Runtime
      OPTIONAL)
  else()
    set(target_file "$<TARGET_FILE:${target}>")
    set(target_pdb_file "$<TARGET_PDB_FILE:${target}>")
    set(comment "Copy ${target} to destination")

    get_target_property(target_type ${target} TYPE)
    if(target_type STREQUAL EXECUTABLE)
      install(TARGETS ${target} RUNTIME DESTINATION "${_TIO_DESTINATION}" COMPONENT Runtime)
    elseif(target_type STREQUAL SHARED_LIBRARY)
      if(NOT _TIO_LIBRARY_DESTINATION)
        set(_TIO_LIBRARY_DESTINATION ${_TIO_DESTINATION})
      endif()
      if(NOT _TIO_HEADER_DESTINATION)
        set(_TIO_HEADER_DESTINATION include)
      endif()
      install(
        TARGETS ${target}
        RUNTIME DESTINATION "${_TIO_DESTINATION}"
        LIBRARY DESTINATION "${_TIO_LIBRARY_DESTINATION}"
                COMPONENT Runtime
                EXCLUDE_FROM_ALL
        PUBLIC_HEADER
          DESTINATION "${_TIO_HEADER_DESTINATION}"
          COMPONENT Development
          EXCLUDE_FROM_ALL)
    elseif(target_type STREQUAL MODULE_LIBRARY)
      install(
        TARGETS ${target}
        LIBRARY DESTINATION "${_TIO_DESTINATION}"
                COMPONENT Runtime
                NAMELINK_COMPONENT Development)
    endif()
  endif()

  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E echo "${comment}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${OBS_OUTPUT_DIR}/$<CONFIG>/${_TIO_DESTINATION}"
    COMMAND "${CMAKE_COMMAND}" -E copy ${target_file} "${OBS_OUTPUT_DIR}/$<CONFIG>/${_TIO_DESTINATION}"
    COMMAND "${CMAKE_COMMAND}" -E $<IF:$<CONFIG:Debug,RelWithDebInfo,Release>,copy,true> ${target_pdb_file}
            "${OBS_OUTPUT_DIR}/$<CONFIG>/${_TIO_DESTINATION}"
    COMMENT ""
    VERBATIM)

  install(
    FILES ${target_pdb_file}
    CONFIGURATIONS RelWithDebInfo Debug Release
    DESTINATION "${_TIO_DESTINATION}"
    COMPONENT Runtime
    OPTIONAL)
endfunction()

# target_export: Helper function to export target as CMake package
function(target_export target)
  # Exclude CMake package from 'ALL' target
  set(exclude_variant EXCLUDE_FROM_ALL)
  _target_export(${target})

  get_target_property(target_type ${target} TYPE)
  if(NOT target_type STREQUAL INTERFACE_LIBRARY)
    install(
      FILES "$<TARGET_PDB_FILE:${target}>"
      CONFIGURATIONS RelWithDebInfo Debug Release
      DESTINATION "${OBS_EXECUTABLE_DESTINATION}"
      COMPONENT Development
      OPTIONAL)
  endif()
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

    get_property(obs_module_list GLOBAL PROPERTY OBS_MODULES_ENABLED)
    if(target IN_LIST obs_module_list)
      set(target_destination "${OBS_DATA_DESTINATION}/obs-plugins/${target}")
    elseif(target STREQUAL obs-studio)
      set(target_destination "${OBS_DATA_DESTINATION}/obs-studio")
    else()
      set(target_destination "${OBS_DATA_DESTINATION}/${target}")
    endif()

    install(
      DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/data/"
      DESTINATION "${target_destination}"
      USE_SOURCE_PERMISSIONS
      COMPONENT Runtime)

    add_custom_command(
      TARGET ${target}
      POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E echo "Copy ${target} resources to data directory"
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${OBS_OUTPUT_DIR}/$<CONFIG>/${target_destination}"
      COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/data"
              "${OBS_OUTPUT_DIR}/$<CONFIG>/${target_destination}"
      COMMENT ""
      VERBATIM)
  endif()
endfunction()

# Helper function to add a specific resource to a bundle
function(target_add_resource target resource)
  get_property(obs_module_list GLOBAL PROPERTY OBS_MODULES_ENABLED)
  if(ARGN)
    set(target_destination "${ARGN}")
  elseif(${target} IN_LIST obs_module_list)
    set(target_destination "${OBS_DATA_DESTINATION}/obs-plugins/${target}")
  elseif(target STREQUAL obs-studio)
    set(target_destination "${OBS_DATA_DESTINATION}/obs-studio")
  else()
    set(target_destination "${OBS_DATA_DESTINATION}/${target}")
  endif()

  message(DEBUG "Add resource '${resource}' to target ${target} at destination '${target_destination}'...")

  install(
    FILES "${resource}"
    DESTINATION "${target_destination}"
    COMPONENT Runtime)

  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E echo "Copy ${target} resource ${resource} to library directory"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${OBS_OUTPUT_DIR}/$<CONFIG>/${target_destination}/"
    COMMAND "${CMAKE_COMMAND}" -E copy "${resource}" "${OBS_OUTPUT_DIR}/$<CONFIG>/${target_destination}/"
    COMMENT ""
    VERBATIM)

  source_group("Resources" FILES "${resource}")
endfunction()

# _bundle_dependencies: Resolve third party dependencies and add them to Windows binary directory
function(_bundle_dependencies target)
  message(DEBUG "Discover dependencies of target ${target}...")
  set(found_dependencies)
  find_dependencies(TARGET ${target} FOUND_VAR found_dependencies)

  get_property(obs_module_list GLOBAL PROPERTY OBS_MODULES_ENABLED)
  list(LENGTH obs_module_list num_modules)
  if(num_modules GREATER 0)
    add_dependencies(${target} ${obs_module_list})
    foreach(module IN LISTS obs_module_list)
      find_dependencies(TARGET ${module} FOUND_VAR found_dependencies)
    endforeach()
  endif()

  list(REMOVE_DUPLICATES found_dependencies)
  set(library_paths_DEBUG)
  set(library_paths_RELWITHDEBINFO)
  set(library_paths_RELEASE)
  set(library_paths_MINSIZEREL)
  set(plugins_list)

  foreach(library IN LISTS found_dependencies)
    # CEF needs to be placed in obs-plugins directory on Windows, which is handled already
    if(${library} STREQUAL CEF::Library)
      continue()
    endif()

    get_target_property(library_type ${library} TYPE)
    get_target_property(is_imported ${library} IMPORTED)

    if(is_imported)
      get_target_property(imported_location ${library} IMPORTED_LOCATION)

      foreach(config IN ITEMS RELEASE RELWITHDEBINFO MINSIZEREL DEBUG)
        get_target_property(imported_location_${config} ${library} IMPORTED_LOCATION_${config})
        if(imported_location_${config})
          _check_library_location(${imported_location_${config}})
        elseif(NOT imported_location_${config} AND imported_location_RELEASE)
          _check_library_location(${imported_location_RELEASE})
        else()
          _check_library_location(${imported_location})
        endif()
      endforeach()

      if(library MATCHES "Qt6?::.+")
        find_qt_plugins(COMPONENT ${library} TARGET ${target} FOUND_VAR plugins_list)
      endif()
    endif()
  endforeach()

  foreach(config IN ITEMS DEBUG RELWITHDEBINFO RELEASE MINSIZEREL)
    list(REMOVE_DUPLICATES library_paths_${config})
  endforeach()

  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E echo "Copy dependencies to binary directory (${OBS_EXECUTABLE_DESTINATION})..."
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${OBS_OUTPUT_DIR}/$<CONFIG>/${OBS_EXECUTABLE_DESTINATION}"
    COMMAND "${CMAKE_COMMAND}" -E "$<IF:$<CONFIG:Debug>,copy_if_different,true>"
            "$<$<CONFIG:Debug>:${library_paths_DEBUG}>" "${OBS_OUTPUT_DIR}/$<CONFIG>/${OBS_EXECUTABLE_DESTINATION}"
    COMMAND
      "${CMAKE_COMMAND}" -E "$<IF:$<CONFIG:RelWithDebInfo>,copy_if_different,true>"
      "$<$<CONFIG:RelWithDebInfo>:${library_paths_RELWITHDEBINFO}>"
      "${OBS_OUTPUT_DIR}/$<CONFIG>/${OBS_EXECUTABLE_DESTINATION}"
    COMMAND "${CMAKE_COMMAND}" -E "$<IF:$<CONFIG:Release>,copy_if_different,true>"
            "$<$<CONFIG:Release>:${library_paths_RELEASE}>" "${OBS_OUTPUT_DIR}/$<CONFIG>/${OBS_EXECUTABLE_DESTINATION}"
    COMMAND
      "${CMAKE_COMMAND}" -E "$<IF:$<CONFIG:MinSizeRel>,copy_if_different,true>"
      "$<$<CONFIG:MinSizeRel>:${library_paths_MINSIZEREL}>" "${OBS_OUTPUT_DIR}/$<CONFIG>/${OBS_EXECUTABLE_DESTINATION}"
    COMMENT "."
    VERBATIM COMMAND_EXPAND_LISTS)

  install(
    FILES ${library_paths_DEBUG}
    CONFIGURATIONS Debug
    DESTINATION "${OBS_EXECUTABLE_DESTINATION}"
    COMPONENT Runtime)

  install(
    FILES ${library_paths_RELWITHDEBINFO}
    CONFIGURATIONS RelWithDebInfo
    DESTINATION "${OBS_EXECUTABLE_DESTINATION}"
    COMPONENT Runtime)

  install(
    FILES ${library_paths_RELEASE}
    CONFIGURATIONS Release
    DESTINATION "${OBS_EXECUTABLE_DESTINATION}"
    COMPONENT Runtime)

  install(
    FILES ${library_paths_MINSIZEREL}
    CONFIGURATIONS MinSizeRel
    DESTINATION "${OBS_EXECUTABLE_DESTINATION}"
    COMPONENT Runtime)

  list(REMOVE_DUPLICATES plugins_list)
  foreach(plugin IN LISTS plugins_list)
    message(TRACE "Adding Qt plugin ${plugin}...")

    cmake_path(GET plugin PARENT_PATH plugin_path)
    cmake_path(GET plugin_path STEM plugin_stem)

    list(APPEND plugin_stems ${plugin_stem})

    if(plugin MATCHES ".+d\\.dll$")
      list(APPEND plugin_${plugin_stem}_debug ${plugin})
    else()
      list(APPEND plugin_${plugin_stem} ${plugin})
    endif()
  endforeach()

  list(REMOVE_DUPLICATES plugin_stems)
  foreach(stem IN LISTS plugin_stems)
    set(plugin_list ${plugin_${stem}})
    set(plugin_list_debug ${plugin_${stem}_debug})
    add_custom_command(
      TARGET ${target}
      POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E echo
              "Copy Qt plugins ${stem} to binary directory (${OBS_EXECUTABLE_DESTINATION}/${stem})"
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${OBS_OUTPUT_DIR}/$<CONFIG>/${OBS_EXECUTABLE_DESTINATION}/${stem}"
      COMMAND "${CMAKE_COMMAND}" -E "$<IF:$<CONFIG:Debug>,copy_if_different,true>" "${plugin_list_debug}"
              "${OBS_OUTPUT_DIR}/$<CONFIG>/${OBS_EXECUTABLE_DESTINATION}/${stem}"
      COMMAND "${CMAKE_COMMAND}" -E "$<IF:$<CONFIG:Debug>,true,copy_if_different>" "${plugin_list}"
              "${OBS_OUTPUT_DIR}/$<CONFIG>/${OBS_EXECUTABLE_DESTINATION}/${stem}"
      COMMENT ""
      VERBATIM COMMAND_EXPAND_LISTS)

    install(
      FILES ${plugin_list_debug}
      CONFIGURATIONS Debug
      DESTINATION "${OBS_EXECUTABLE_DESTINATION}/${stem}"
      COMPONENT Runtime)

    install(
      FILES ${plugin_list}
      CONFIGURATIONS RelWithDebInfo Release MinSizeRel
      DESTINATION "${OBS_EXECUTABLE_DESTINATION}/${stem}"
      COMPONENT Runtime)
  endforeach()
endfunction()

# _check_library_location: Check for corresponding DLL given an import library path
macro(_check_library_location location)
  if(library_type STREQUAL "SHARED_LIBRARY")
    set(library_location "${location}")
  else()
    string(STRIP "${location}" location)
    if(location MATCHES ".+lib$")
      cmake_path(GET location FILENAME _dll_name)
      cmake_path(GET location PARENT_PATH _implib_path)
      cmake_path(SET _bin_path NORMALIZE "${_implib_path}/../bin")
      string(REPLACE ".lib" ".dll" _dll_name "${_dll_name}")
      string(REPLACE ".dll" ".pdb" _pdb_name "${_dll_name}")

      find_program(
        _dll_path
        NAMES "${_dll_name}"
        HINTS ${_implib_path} ${_bin_path} NO_CACHE
        NO_DEFAULT_PATH)

      find_program(
        _pdb_path
        NAMES "${_pdb_name}"
        HINTS ${_implib_path} ${_bin_path} NO_CACHE
        NO_DEFAULT_PATH)

      if(_dll_path)
        set(library_location "${_dll_path}")
        set(library_pdb_location "${_pdb_path}")
      else()
        unset(library_location)
        unset(library_pdb_location)
      endif()
      unset(_dll_path)
      unset(_pdb_path)
      unset(_bin_path)
      unset(_implib_path)
      unset(_dll_name)
      unset(_pdb_name)
    else()
      unset(library_location)
      unset(library_pdb_location)
    endif()
  endif()

  if(library_location)
    list(APPEND library_paths_${config} ${library_location})
  endif()
  if(library_pdb_location)
    list(APPEND library_paths_${config} ${library_pdb_location})
  endif()
  unset(location)
  unset(library_location)
  unset(library_pdb_location)
endmacro()
