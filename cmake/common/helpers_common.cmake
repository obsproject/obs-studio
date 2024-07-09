# OBS CMake common helper functions module

include_guard(GLOBAL)

# message_configuration: Function to print configuration outcome
function(message_configuration)
  include(FeatureSummary)
  feature_summary(WHAT ALL VAR _feature_summary)

  message(DEBUG "${_feature_summary}")

  message(
    NOTICE
    "                      _                   _             _ _       \n"
    "                 ___ | |__  ___       ___| |_ _   _  __| (_) ___  \n"
    "                / _ \\| '_ \\/ __|_____/ __| __| | | |/ _` | |/ _ \\ \n"
    "               | (_) | |_) \\__ \\_____\\__ \\ |_| |_| | (_| | | (_) |\n"
    "                \\___/|_.__/|___/     |___/\\__|\\__,_|\\__,_|_|\\___/ \n"
    "\nOBS:  Application Version: ${OBS_VERSION} - Build Number: ${OBS_BUILD_NUMBER}\n"
    "==================================================================================\n\n"
  )

  get_property(OBS_FEATURES_ENABLED GLOBAL PROPERTY OBS_FEATURES_ENABLED)
  list(SORT OBS_FEATURES_ENABLED COMPARE NATURAL CASE SENSITIVE ORDER ASCENDING)

  if(OBS_FEATURES_ENABLED)
    message(NOTICE "------------------------       Enabled Features           ------------------------")
    foreach(feature IN LISTS OBS_FEATURES_ENABLED)
      message(NOTICE " - ${feature}")
    endforeach()
  endif()

  get_property(OBS_FEATURES_DISABLED GLOBAL PROPERTY OBS_FEATURES_DISABLED)
  list(SORT OBS_FEATURES_DISABLED COMPARE NATURAL CASE SENSITIVE ORDER ASCENDING)

  if(OBS_FEATURES_DISABLED)
    message(NOTICE "------------------------       Disabled Features          ------------------------")
    foreach(feature IN LISTS OBS_FEATURES_DISABLED)
      message(NOTICE " - ${feature}")
    endforeach()
  endif()

  if(ENABLE_PLUGINS)
    get_property(OBS_MODULES_ENABLED GLOBAL PROPERTY OBS_MODULES_ENABLED)
    list(SORT OBS_MODULES_ENABLED COMPARE NATURAL CASE SENSITIVE ORDER ASCENDING)

    if(OBS_MODULES_ENABLED)
      message(NOTICE "------------------------        Enabled Modules           ------------------------")
      foreach(feature IN LISTS OBS_MODULES_ENABLED)
        message(NOTICE " - ${feature}")
      endforeach()
    endif()

    get_property(OBS_MODULES_DISABLED GLOBAL PROPERTY OBS_MODULES_DISABLED)
    list(SORT OBS_MODULES_DISABLED COMPARE NATURAL CASE SENSITIVE ORDER ASCENDING)

    if(OBS_MODULES_DISABLED)
      message(NOTICE "------------------------        Disabled Modules          ------------------------")
      foreach(feature IN LISTS OBS_MODULES_DISABLED)
        message(NOTICE " - ${feature}")
      endforeach()
    endif()
  endif()
  message(NOTICE "----------------------------------------------------------------------------------")
endfunction()

# target_enable_feature: Adds feature to list of enabled application features and sets optional compile definitions
function(target_enable_feature target feature_description)
  set_property(GLOBAL APPEND PROPERTY OBS_FEATURES_ENABLED "${feature_description}")

  if(ARGN)
    target_compile_definitions(${target} PRIVATE ${ARGN})
  endif()
endfunction()

# target_disable_feature: Adds feature to list of disabled application features and sets optional compile definitions
function(target_disable_feature target feature_description)
  set_property(GLOBAL APPEND PROPERTY OBS_FEATURES_DISABLED "${feature_description}")

  if(ARGN)
    target_compile_definitions(${target} PRIVATE ${ARGN})
  endif()
endfunction()

# target_disable: Adds target to list of disabled modules
function(target_disable target)
  set_property(GLOBAL APPEND PROPERTY OBS_MODULES_DISABLED ${target})
endfunction()

# _handle_generator_expression_dependency: Helper function to yield dependency from a generator expression
function(_handle_generator_expression_dependency library)
  set(oneValueArgs FOUND_VAR)
  set(multiValueArgs)
  cmake_parse_arguments(var "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  set(${var_FOUND_VAR} "${var_FOUND_VAR}-NOTFOUND")

  message(DEBUG "Checking ${library}...")

  if(library MATCHES "\\$<\\$<PLATFORM_ID:[^>]+>:.+>" OR library MATCHES "\\$<\\$<NOT:\\$<PLATFORM_ID:[^>]+>>:.+>")
    # Platform-dependent generator expression found. Platforms are a comma-separated list of CMake host OS identifiers.
    # Convert to CMake list and check if current host OS is contained in list.
    string(REGEX REPLACE "\\$<.*\\$<PLATFORM_ID:([^>]+)>>?:([^>]+)>" "\\1;\\2" gen_expression "${library}")
    list(GET gen_expression 0 gen_platform)
    list(GET gen_expression 1 gen_library)
    string(REPLACE "," ";" gen_platform "${gen_platform}")

    set(${var_FOUND_VAR} "${var_FOUND_VAR}-SKIP")

    if(library MATCHES "\\$<\\$<NOT:.+>.+>")
      if(NOT CMAKE_SYSTEM_NAME IN_LIST gen_platform)
        set(${var_FOUND_VAR} "${gen_library}")
      endif()
    else()
      if(CMAKE_SYSTEM_NAME IN_LIST gen_platform)
        set(${var_FOUND_VAR} "${gen_library}")
      endif()
    endif()
  elseif(library MATCHES "\\$<\\$<BOOL:[^>]+>:.+>")
    # Boolean generator expression found. Consider parameter a CMake variable that resolves into a CMake-like boolean
    # value for a simple conditional check.
    string(REGEX REPLACE "\\$<\\$<BOOL:([^>]+)>:([^>]+)>" "\\1;\\2" gen_expression "${library}")
    list(GET gen_expression 0 gen_boolean)
    list(GET gen_expression 1 gen_library)

    set(${var_FOUND_VAR} "${var_FOUND_VAR}-SKIP")

    if(${gen_boolean})
      set(${var_FOUND_VAR} "${gen_library}")
    endif()
  elseif(library MATCHES "\\$<TARGET_NAME_IF_EXISTS:[^>]+>")
    # Target-dependent generator expression found. Consider parameter to be a CMake target identifier and check for
    # target existence.
    string(REGEX REPLACE "\\$<TARGET_NAME_IF_EXISTS:([^>]+)>" "\\1" gen_target "${library}")

    set(${var_FOUND_VAR} "${var_FOUND_VAR}-SKIP")

    if(TARGET ${gen_target})
      set(${var_FOUND_VAR} "${gen_target}")
    endif()
  elseif(library MATCHES "\\$<.*Qt6::(EntryPointPrivate|QDarwin.*PermissionPlugin)>")
    set(${var_FOUND_VAR} "${var_FOUND_VAR}-SKIP")
  else()
    # Unknown or unimplemented generator expression found. Abort script run to either add to ignore list or implement
    # detection.
    message(FATAL_ERROR "${library} is an unsupported generator expression for linked libraries.")
  endif()

  if(CMAKE_VERSION VERSION_LESS 3.25)
    set(${var_FOUND_VAR} ${var_FOUND_VAR} PARENT_SCOPE)
  else()
    return(PROPAGATE ${var_FOUND_VAR})
  endif()
endfunction()

# find_dependencies: Check linked interface and direct dependencies of target
function(find_dependencies)
  set(oneValueArgs TARGET FOUND_VAR)
  set(multiValueArgs)
  cmake_parse_arguments(var "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT DEFINED is_root)
    # Root of recursive dependency resolution
    set(is_root TRUE)
    set(nested_depth 0)
  else()
    # Branch of recursive dependency resolution
    set(is_root FALSE)
    math(EXPR nested_depth "${nested_depth}+1")
  endif()

  # * LINK_LIBRARIES are direct dependencies
  # * INTERFACE_LINK_LIBRARIES are transitive dependencies
  get_target_property(linked_libraries ${var_TARGET} LINK_LIBRARIES)
  get_target_property(interface_libraries ${var_TARGET} INTERFACE_LINK_LIBRARIES)
  message(DEBUG "[${nested_depth}] Linked libraries in target ${var_TARGET}: ${linked_libraries}")
  message(DEBUG "[${nested_depth}] Linked interface libraries in target ${var_TARGET}: ${interface_libraries}")

  # Consider CMake targets only
  list(FILTER linked_libraries INCLUDE REGEX ".+::.+")
  list(FILTER interface_libraries INCLUDE REGEX ".+::.+")

  foreach(library IN LISTS linked_libraries interface_libraries)
    if(NOT library)
      continue()
    elseif(library MATCHES "\\$<.*:[^>]+>")
      # Generator expression found
      _handle_generator_expression_dependency(${library} FOUND_VAR found_library)
      if(found_library STREQUAL found_library-SKIP)
        continue()
      elseif(found_library)
        set(library ${found_library})
      endif()
    endif()

    message(DEBUG "[${nested_depth}] Found ${library}...")

    if(NOT library IN_LIST ${var_FOUND_VAR})
      list(APPEND found_libraries ${library})
      # Enter recursive branch
      find_dependencies(TARGET ${library} FOUND_VAR ${var_FOUND_VAR})
    endif()
  endforeach()

  if(NOT is_root)
    # Exit recursive branch
    return(PROPAGATE found_libraries)
  endif()

  list(REMOVE_DUPLICATES found_libraries)
  list(APPEND ${var_FOUND_VAR} ${found_libraries})

  return(PROPAGATE ${var_FOUND_VAR})
endfunction()

# find_qt_plugins: Find and add Qt plugin libraries associated with Qt component to target
function(find_qt_plugins)
  set(oneValueArgs COMPONENT TARGET FOUND_VAR)
  cmake_parse_arguments(var "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  string(REPLACE "::" ";" library_tuple "${var_COMPONENT}")
  list(GET library_tuple 0 library_namespace)
  list(GET library_tuple 1 library_name)

  if(NOT ${library_namespace} MATCHES "Qt6?")
    message(FATAL_ERROR "'find_qt_plugins' has to be called with a valid target from the Qt or Qt6 namespace.")
  endif()

  list(
    APPEND
    qt_plugins_Core
    platforms
    printsupport
    styles
    imageformats
    iconengines
  )
  list(APPEND qt_plugins_Gui platforminputcontexts)
  list(APPEND qt_plugins_Sql sqldrivers)
  list(APPEND qt_plugins_3dRender sceneparsers geometryloaders)
  list(APPEND qt_plugins_3dQuickRender renderplugins)
  list(APPEND qt_plugins_Positioning position)
  list(APPEND qt_plugins_Location geoservices)
  list(APPEND qt_plugins_TextToSpeech texttospeech)
  list(APPEND qt_plugins_WebView webview)

  if(qt_plugins_${library_name})
    get_target_property(library_location ${var_COMPONENT} IMPORTED_LOCATION)
    get_target_property(is_framework ${var_COMPONENT} FRAMEWORK)

    if(is_framework)
      # Resolve Qt plugin location relative to framework binary location on macOS
      set(plugins_location "../../../../../plugins")
      cmake_path(ABSOLUTE_PATH plugins_location BASE_DIRECTORY "${library_location}" NORMALIZE)
    else()
      # Resolve Qt plugin location relative to dynamic library location
      set(plugins_location "../../plugins")
      cmake_path(ABSOLUTE_PATH plugins_location BASE_DIRECTORY "${library_location}" NORMALIZE)
    endif()

    foreach(plugin IN ITEMS ${qt_plugins_${library_name}})
      if(NOT plugin IN_LIST plugins_list)
        if(EXISTS "${plugins_location}/${plugin}")
          # Gather all .dll or .dylib files in given plugin subdirectory
          file(
            GLOB plugin_libraries
            RELATIVE "${plugins_location}/${plugin}"
            "${plugins_location}/${plugin}/*.dylib"
            "${plugins_location}/${plugin}/*.dll"
          )
          message(DEBUG "Found Qt plugin ${plugin} libraries: ${plugin_libraries}")
          foreach(plugin_library IN ITEMS ${plugin_libraries})
            set(plugin_full_path "${plugins_location}/${plugin}/${plugin_library}")
            list(APPEND plugins_list ${plugin_full_path})
          endforeach()
        endif()
      endif()
    endforeach()
  endif()

  set(${var_FOUND_VAR} ${plugins_list})
  return(PROPAGATE ${var_FOUND_VAR})
endfunction()

# target_export: Helper function to export target as CMake package
function(target_export target)
  if(NOT DEFINED exclude_variant)
    set(exclude_variant EXCLUDE_FROM_ALL)
  endif()

  get_target_property(is_framework ${target} FRAMEWORK)
  if(is_framework)
    set(package_destination "Frameworks/${target}.framework/Resources/cmake")
    set(include_destination "Frameworks/${target}.framework/Headers")
  else()
    if(OS_WINDOWS)
      set(package_destination "${OBS_CMAKE_DESTINATION}")
    else()
      set(package_destination "${OBS_CMAKE_DESTINATION}/${target}")
    endif()
    set(include_destination "${OBS_INCLUDE_DESTINATION}")
  endif()

  install(
    TARGETS ${target}
    EXPORT ${target}Targets
    RUNTIME DESTINATION "${OBS_EXECUTABLE_DESTINATION}" COMPONENT Development
    ${exclude_variant}
    LIBRARY DESTINATION "${OBS_LIBRARY_DESTINATION}" COMPONENT Development
    ${exclude_variant}
    ARCHIVE DESTINATION "${OBS_LIBRARY_DESTINATION}" COMPONENT Development
    ${exclude_variant}
    FRAMEWORK DESTINATION Frameworks COMPONENT Development
    ${exclude_variant}
    INCLUDES DESTINATION "${include_destination}"
    PUBLIC_HEADER DESTINATION "${include_destination}" COMPONENT Development
    ${exclude_variant}
  )

  get_target_property(obs_public_headers ${target} OBS_PUBLIC_HEADERS)

  if(obs_public_headers)
    foreach(header IN LISTS obs_public_headers)
      cmake_path(GET header PARENT_PATH header_dir)
      if(header_dir)
        if(NOT ${header_dir} IN_LIST header_dirs)
          list(APPEND header_dirs ${header_dir})
        endif()
        list(APPEND headers_${header_dir} ${header})
      else()
        list(APPEND headers ${header})
      endif()
    endforeach()

    foreach(header_dir IN LISTS header_dirs)
      install(
        FILES ${headers_${header_dir}}
        DESTINATION "${include_destination}/${header_dir}"
        COMPONENT Development
        ${exclude_variant}
      )
    endforeach()

    if(headers)
      install(FILES ${headers} DESTINATION "${include_destination}" COMPONENT Development ${exclude_variant})
    endif()
  endif()

  if(target STREQUAL libobs AND NOT EXISTS "${include_destination}/obsconfig.h")
    install(
      FILES "${CMAKE_BINARY_DIR}/config/obsconfig.h"
      DESTINATION "${include_destination}"
      COMPONENT Development
      ${exclude_variant}
    )
  endif()

  get_target_property(target_type ${target} TYPE)

  if(NOT target_type STREQUAL INTERFACE_LIBRARY)
    message(DEBUG "Generating export header for target ${target} as ${target}_EXPORT.h...")
    include(GenerateExportHeader)
    generate_export_header(${target} EXPORT_FILE_NAME "${target}_EXPORT.h")
    target_sources(${target} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/${target}_EXPORT.h>)

    set_property(TARGET ${target} APPEND PROPERTY PUBLIC_HEADER "${target}_EXPORT.h")
  endif()

  set(TARGETS_EXPORT_NAME ${target}Targets)
  message(
    DEBUG
    "Generating CMake package configuration file ${target}Config.cmake with targets file ${TARGETS_EXPORT_NAME}..."
  )
  include(CMakePackageConfigHelpers)
  configure_package_config_file(
    cmake/${target}Config.cmake.in
    ${target}Config.cmake
    INSTALL_DESTINATION "${package_destination}"
  )

  message(DEBUG "Generating CMake package version configuration file ${target}ConfigVersion.cmake...")
  write_basic_package_version_file(
    "${target}ConfigVersion.cmake"
    VERSION ${OBS_VERSION_CANONICAL}
    COMPATIBILITY SameMajorVersion
  )

  export(EXPORT ${target}Targets FILE "${TARGETS_EXPORT_NAME}.cmake" NAMESPACE OBS::)

  export(PACKAGE ${target})

  install(
    EXPORT ${TARGETS_EXPORT_NAME}
    FILE ${TARGETS_EXPORT_NAME}.cmake
    NAMESPACE OBS::
    DESTINATION "${package_destination}"
    COMPONENT Development
    ${exclude_variant}
  )

  install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/${target}Config.cmake" "${CMAKE_CURRENT_BINARY_DIR}/${target}ConfigVersion.cmake"
    DESTINATION "${package_destination}"
    COMPONENT Development
    ${exclude_variant}
  )

  if(target STREQUAL libobs)
    install(
      FILES "${CMAKE_SOURCE_DIR}/cmake/finders/FindSIMDe.cmake"
      DESTINATION "${package_destination}/finders"
      COMPONENT Development
      ${exclude_variant}
    )
  endif()
endfunction()

# check_uuid: Helper function to check for valid UUID
function(check_uuid uuid_string return_value)
  set(valid_uuid TRUE)
  # gersemi: off
  set(uuid_token_lengths 8 4 4 4 12)
  # gersemi: on
  set(token_num 0)

  string(REPLACE "-" ";" uuid_tokens ${uuid_string})
  list(LENGTH uuid_tokens uuid_num_tokens)

  if(uuid_num_tokens EQUAL 5)
    message(DEBUG "UUID ${uuid_string} is valid with 5 tokens.")
    foreach(uuid_token IN LISTS uuid_tokens)
      list(GET uuid_token_lengths ${token_num} uuid_target_length)
      string(LENGTH "${uuid_token}" uuid_actual_length)
      if(uuid_actual_length EQUAL uuid_target_length)
        string(REGEX MATCH "[0-9a-fA-F]+" uuid_hex_match ${uuid_token})
        if(NOT uuid_hex_match STREQUAL uuid_token)
          set(valid_uuid FALSE)
          break()
        endif()
      else()
        set(valid_uuid FALSE)
        break()
      endif()
      math(EXPR token_num "${token_num}+1")
    endforeach()
  else()
    set(valid_uuid FALSE)
  endif()
  message(DEBUG "UUID ${uuid_string} valid: ${valid_uuid}")

  set(${return_value} ${valid_uuid})
  return(PROPAGATE ${return_value})
endfunction()

# add_obs_plugin: Add plugin subdirectory if host platform is in specified list of supported platforms and architectures
function(add_obs_plugin target)
  set(options WITH_MESSAGE)
  set(oneValueArgs "")
  set(multiValueArgs PLATFORMS ARCHITECTURES)
  cmake_parse_arguments(PARSE_ARGV 0 _AOP "${options}" "${oneValueArgs}" "${multiValueArgs}")

  set(found_platform FALSE)
  list(LENGTH _AOP_PLATFORMS _AOP_NUM_PLATFORMS)

  set(found_architecture FALSE)
  list(LENGTH _AOP_ARCHITECTURES _AOP_NUM_ARCHITECTURES)

  if(_AOP_NUM_PLATFORMS EQUAL 0)
    set(found_platform TRUE)
  else()
    foreach(platform IN LISTS _AOP_PLATFORMS)
      set(check_var_name "OS_${platform}")
      if(${${check_var_name}})
        set(found_platform TRUE)
        break()
      endif()
    endforeach()
  endif()

  if(_AOP_NUM_ARCHITECTURES EQUAL 0)
    set(found_architecture TRUE)
  else()
    foreach(architecture IN LISTS _AOP_ARCHITECTURES)
      if(OS_WINDOWS)
        if("${architecture}" STREQUAL CMAKE_VS_PLATFORM_NAME)
          set(found_architecture TRUE)
        endif()
      elseif(OS_MACOS)
        if(
          "${architecture}" IN_LIST CMAKE_OSX_ARCHITECTURES
          OR "${architecture}" STREQUAL "${CMAKE_HOST_SYSTEM_PROCESSOR}"
        )
          set(found_architecture TRUE)
        endif()
      elseif("${architecture}" STREQUAL CMAKE_SYSTEM_PROCESSOR)
        set(found_architecture TRUE)
      endif()
    endforeach()
  endif()

  if(found_platform AND found_architecture)
    add_subdirectory(${target})
  elseif(_AOP_WITH_MESSAGE)
    add_custom_target(${target} COMMENT "Dummy target for unavailable module ${target}")
    target_disable(${target})
  endif()
endfunction()
