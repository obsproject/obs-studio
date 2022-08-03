# Helper function to set up runtime or library targets
function(setup_binary_target target)
  set_target_properties(
    ${target}
    PROPERTIES XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER
               "com.obsproject.${target}"
               XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY
               "${OBS_BUNDLE_CODESIGN_IDENTITY}"
               XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS
               "${CMAKE_SOURCE_DIR}/cmake/bundle/macOS/entitlements.plist")

  set(MACOSX_PLUGIN_BUNDLE_NAME
      "${target}"
      PARENT_SCOPE)
  set(MACOSX_PLUGIN_GUI_IDENTIFIER
      "com.obsproject.${target}"
      PARENT_SCOPE)
  set(MACOSX_PLUGIN_BUNDLE_VERSION
      "${MACOSX_BUNDLE_BUNDLE_VERSION}"
      PARENT_SCOPE)
  set(MACOSX_PLUGIN_SHORT_VERSION_STRING
      "${MACOSX_BUNDLE_SHORT_VERSION_STRING}"
      PARENT_SCOPE)
  set(MACOSX_PLUGIN_EXECUTABLE_NAME
      "${target}"
      PARENT_SCOPE)

  if(${target} STREQUAL libobs)
    setup_framework_target(${target})
    set_property(GLOBAL APPEND PROPERTY OBS_FRAMEWORK_LIST "${target}")
  elseif(NOT ${target} STREQUAL obs-ffmpeg-mux AND NOT ${target} STREQUAL
                                                   mac-dal-plugin)
    set_property(GLOBAL APPEND PROPERTY OBS_FRAMEWORK_LIST "${target}")
  endif()
endfunction()

# Helper function to set-up framework targets on macOS
function(setup_framework_target target)
  set_target_properties(
    ${target}
    PROPERTIES FRAMEWORK ON
               FRAMEWORK_VERSION A
               OUTPUT_NAME "${target}"
               MACOSX_FRAMEWORK_IDENTIFIER "com.obsproject.${target}"
               MACOSX_FRAMEWORK_INFO_PLIST
               "${CMAKE_SOURCE_DIR}/cmake/bundle/macOS/Plugin-Info.plist.in"
               XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER
               "com.obsproject.${target}")

  install(
    TARGETS ${target}
    EXPORT "${target}Targets"
    FRAMEWORK DESTINATION "Frameworks" COMPONENT obs_libraries
    INCLUDES
    DESTINATION Frameworks/$<TARGET_FILE_BASE_NAME:${target}>.framework/Headers
    PUBLIC_HEADER
      DESTINATION
        Frameworks/$<TARGET_FILE_BASE_NAME:${target}>.framework/Headers
      EXCLUDE_FROM_ALL)
endfunction()

# Helper function to set up OBS plugin targets
function(setup_plugin_target target)
  set(MACOSX_PLUGIN_BUNDLE_NAME
      "${target}"
      PARENT_SCOPE)
  set(MACOSX_PLUGIN_GUI_IDENTIFIER
      "com.obsproject.${target}"
      PARENT_SCOPE)
  set(MACOSX_PLUGIN_BUNDLE_VERSION
      "${MACOSX_BUNDLE_BUNDLE_VERSION}"
      PARENT_SCOPE)
  set(MACOSX_PLUGIN_SHORT_VERSION_STRING
      "${MACOSX_BUNDLE_SHORT_VERSION_STRING}"
      PARENT_SCOPE)
  set(MACOSX_PLUGIN_EXECUTABLE_NAME
      "${target}"
      PARENT_SCOPE)
  set(MACOSX_PLUGIN_BUNDLE_TYPE
      "BNDL"
      PARENT_SCOPE)

  set_target_properties(
    ${target}
    PROPERTIES BUNDLE ON
               BUNDLE_EXTENSION "plugin"
               OUTPUT_NAME "${target}"
               MACOSX_BUNDLE_INFO_PLIST
               "${CMAKE_SOURCE_DIR}/cmake/bundle/macOS/Plugin-Info.plist.in"
               XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER
               "com.obsproject.${target}"
               XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY
               "${OBS_BUNDLE_CODESIGN_IDENTITY}"
               XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS
               "${CMAKE_SOURCE_DIR}/cmake/bundle/macOS/entitlements.plist")

  set_property(GLOBAL APPEND PROPERTY OBS_MODULE_LIST "${target}")
  obs_status(ENABLED "${target}")

  install_bundle_resources(${target})
endfunction()

# Helper function to set up OBS scripting plugin targets
function(setup_script_plugin_target target)
  set_target_properties(
    ${target}
    PROPERTIES XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER
               "com.obsproject.${target}"
               XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY
               "${OBS_BUNDLE_CODESIGN_IDENTITY}"
               XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS
               "${CMAKE_SOURCE_DIR}/cmake/bundle/macOS/entitlements.plist")

  set_property(GLOBAL APPEND PROPERTY OBS_SCRIPTING_MODULE_LIST "${target}")
  obs_status(ENABLED "${target}")
endfunction()

# Helper function to set up target resources (e.g. L10N files)
function(setup_target_resources target destination)
  install_bundle_resources(${target})
endfunction()

# Helper function to set up plugin resources inside plugin bundle
function(install_bundle_resources target)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data")
    file(GLOB_RECURSE _DATA_FILES "${CMAKE_CURRENT_SOURCE_DIR}/data/*")
    foreach(_DATA_FILE IN LISTS _DATA_FILES)
      file(RELATIVE_PATH _RELATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/data/
           ${_DATA_FILE})
      get_filename_component(_RELATIVE_PATH "${_RELATIVE_PATH}" PATH)
      target_sources(${target} PRIVATE ${_DATA_FILE})
      set_source_files_properties(
        ${_DATA_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION
                                 "Resources/${_RELATIVE_PATH}")
      string(REPLACE "\\" "\\\\" _GROUP_NAME "${_RELATIVE_PATH}")
      source_group("Resources\\${_GROUP_NAME}" FILES ${_DATA_FILE})
    endforeach()
  endif()
endfunction()

# Helper function to set up specific resource files for targets
function(add_target_resource target resource destination)
  target_sources(${target} PRIVATE ${resource})
  set_source_files_properties(${resource} PROPERTIES MACOSX_PACKAGE_LOCATION
                                                     Resources)
endfunction()

# Helper function to set up OBS app target
function(setup_obs_app target)
  set_target_properties(
    ${target}
    PROPERTIES BUILD_WITH_INSTALL_RPATH OFF
               XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY
               "${OBS_BUNDLE_CODESIGN_IDENTITY}"
               XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS
               "${CMAKE_SOURCE_DIR}/cmake/bundle/macOS/entitlements.plist"
               XCODE_SCHEME_ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1")

  install(TARGETS ${target} BUNDLE DESTINATION "." COMPONENT obs_app)

  if(TARGET OBS::browser)
    setup_target_browser(${target})
  endif()

  setup_obs_frameworks(${target})
  setup_obs_modules(${target})
  setup_obs_bundle(${target})
endfunction()

# Helper function to do additional setup for browser source plugin
function(setup_target_browser target)
  get_filename_component(_CEF_FRAMEWORK_NAME "${CEF_LIBRARY}" NAME)

  install(
    DIRECTORY "${CEF_LIBRARY}"
    DESTINATION "Frameworks"
    USE_SOURCE_PERMISSIONS
    COMPONENT obs_browser_dev
    EXCLUDE_FROM_ALL)

  foreach(_CEF_LIBRARY IN ITEMS "libEGL" "libswiftshader_libEGL" "libGLESv2"
                                "libswiftshader_libGLESv2" "libvk_swiftshader")
    set(_COMMAND
        "/usr/bin/codesign --force --sign \\\"${OBS_BUNDLE_CODESIGN_IDENTITY}\\\" $<$<BOOL:${OBS_CODESIGN_LINKER}>:--options linker-signed > \\\"\${CMAKE_INSTALL_PREFIX}/Frameworks/${_CEF_FRAMEWORK_NAME}/Libraries/${_CEF_LIBRARY}.dylib\\\""
    )
    list(APPEND _CEF_CODESIGN_COMMANDS
         "execute_process(COMMAND /bin/sh -c \"${_COMMAND}\")")
  endforeach()

  set(_COMMAND
      "/usr/bin/codesign --force --sign \\\"${OBS_BUNDLE_CODESIGN_IDENTITY}\\\" $<$<BOOL:${OBS_CODESIGN_LINKER}>:--options linker-signed > --deep \\\"\${CMAKE_INSTALL_PREFIX}/Frameworks/${_CEF_FRAMEWORK_NAME}/Chromium Embedded Framework\\\""
  )

  list(APPEND _CEF_CODESIGN_COMMANDS
       "execute_process(COMMAND /bin/sh -c \"${_COMMAND}\")")
  string(REPLACE ";" "\n  " _CEF_CODESIGN_COMMANDS "${_CEF_CODESIGN_COMMANDS}")
  install(
    CODE "${_CEF_CODESIGN_COMMANDS}"
    COMPONENT obs_browser_dev
    EXCLUDE_FROM_ALL)

  foreach(_SUFFIX IN ITEMS "_gpu" "_plugin" "_renderer" "")
    if(TARGET obs-browser-page${_SUFFIX})
      add_executable(OBS::browser-helper${_SUFFIX} ALIAS
                     obs-browser-page${_SUFFIX})
      target_compile_features(obs-browser-page${_SUFFIX} PRIVATE cxx_std_17)
    endif()

    if(TARGET OBS::browser-helper${_SUFFIX})
      add_dependencies(${target} OBS::browser-helper${_SUFFIX})

      install(
        DIRECTORY "$<TARGET_BUNDLE_DIR:OBS::browser-helper${_SUFFIX}>"
        DESTINATION "Frameworks"
        USE_SOURCE_PERMISSIONS
        COMPONENT obs_browser_dev
        EXCLUDE_FROM_ALL)

      set(_COMMAND
          "/usr/bin/codesign --force --sign \\\"${OBS_BUNDLE_CODESIGN_IDENTITY}\\\" $<$<BOOL:${OBS_CODESIGN_LINKER}>:--options linker-signed > \\\"\${CMAKE_INSTALL_PREFIX}/Frameworks/$<TARGET_FILE_NAME:OBS::browser-helper${_SUFFIX}>.app\\\" > /dev/null"
      )

      install(
        CODE "execute_process(COMMAND /bin/sh -c \"${_COMMAND}\")"
        COMPONENT obs_browser_dev
        EXCLUDE_FROM_ALL)
    endif()
  endforeach()

  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMAND
      "${CMAKE_COMMAND}" --install . --config $<CONFIG> --prefix
      $<TARGET_BUNDLE_CONTENT_DIR:${target}> --component obs_browser_dev >
      /dev/null
    COMMENT "Installing Chromium Embedded Framework for development"
    VERBATIM)
endfunction()

# Helper function to set-up OBS frameworks for macOS bundling
function(setup_obs_frameworks target)
  get_property(OBS_FRAMEWORK_LIST GLOBAL PROPERTY OBS_FRAMEWORK_LIST)
  install(
    TARGETS ${OBS_FRAMEWORK_LIST}
    RUNTIME
      DESTINATION "$<TARGET_FILE_BASE_NAME:${target}>.app/Contents/Frameworks/"
      COMPONENT obs_frameworks
    LIBRARY
      DESTINATION "$<TARGET_FILE_BASE_NAME:${target}>.app/Contents/Frameworks/"
      COMPONENT obs_frameworks
    FRAMEWORK
      DESTINATION "$<TARGET_FILE_BASE_NAME:${target}>.app/Contents/Frameworks/"
      COMPONENT obs_frameworks
    PUBLIC_HEADER DESTINATION "${OBS_INCLUDE_DESTINATION}"
                  COMPONENT obs_libraries)
endfunction()

# Helper function to set-up OBS plugins and helper binaries for macOS bundling
function(setup_obs_modules target)

  get_property(OBS_MODULE_LIST GLOBAL PROPERTY OBS_MODULE_LIST)
  list(LENGTH OBS_MODULE_LIST _LEN)
  if(_LEN GREATER 0)
    add_dependencies(${target} ${OBS_MODULE_LIST})

    install(
      TARGETS ${OBS_MODULE_LIST}
      LIBRARY DESTINATION "PlugIns"
              COMPONENT obs_plugin_dev
              EXCLUDE_FROM_ALL)

    install(
      TARGETS ${OBS_MODULE_LIST}
      LIBRARY
        DESTINATION $<TARGET_FILE_BASE_NAME:${target}>.app/Contents/PlugIns
        COMPONENT obs_plugins
        NAMELINK_COMPONENT ${target}_Development)
  endif()

  get_property(OBS_SCRIPTING_MODULE_LIST GLOBAL
               PROPERTY OBS_SCRIPTING_MODULE_LIST)
  list(LENGTH OBS_SCRIPTING_MODULE_LIST _LEN)
  if(_LEN GREATER 0)
    add_dependencies(${target} ${OBS_SCRIPTING_MODULE_LIST})

    install(
      TARGETS ${OBS_SCRIPTING_MODULE_LIST}
      LIBRARY DESTINATION "PlugIns"
              COMPONENT obs_plugin_dev
              EXCLUDE_FROM_ALL)

    if(TARGET obspython)
      install(
        FILES "$<TARGET_FILE_DIR:obspython>/obspython.py"
        DESTINATION "Resources"
        COMPONENT obs_plugin_dev
        EXCLUDE_FROM_ALL)
    endif()

    install(
      TARGETS ${OBS_SCRIPTING_MODULE_LIST}
      LIBRARY DESTINATION $<TARGET_FILE_BASE_NAME:obs>.app/Contents/PlugIns
              COMPONENT obs_scripting_plugins)
  endif()

  if(TARGET obs-ffmpeg-mux)
    add_dependencies(${target} obs-ffmpeg-mux)

    install(TARGETS obs-ffmpeg-mux
            RUNTIME DESTINATION $<TARGET_FILE_BASE_NAME:obs>.app/Contents/MacOS
                    COMPONENT obs_plugins)

    install(
      PROGRAMS $<TARGET_FILE:obs-ffmpeg-mux>
      DESTINATION "MacOS"
      COMPONENT obs_plugin_dev
      EXCLUDE_FROM_ALL)

    set(_COMMAND
        "/usr/bin/codesign --force --sign \\\"${OBS_BUNDLE_CODESIGN_IDENTITY}\\\" $<$<BOOL:${OBS_CODESIGN_LINKER}>:--options linker-signed > \\\"\${CMAKE_INSTALL_PREFIX}/MacOS/$<TARGET_FILE_NAME:obs-ffmpeg-mux>\\\" > /dev/null"
    )

    install(
      CODE "execute_process(COMMAND /bin/sh -c \"${_COMMAND}\")"
      COMPONENT obs_plugin_dev
      EXCLUDE_FROM_ALL)

  endif()

  if(TARGET mac-dal-plugin)
    add_dependencies(${target} mac-dal-plugin)

    install(
      TARGETS mac-dal-plugin
      LIBRARY DESTINATION "Resources"
              COMPONENT obs_plugin_dev
              EXCLUDE_FROM_ALL)
  endif()

  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMAND
      "${CMAKE_COMMAND}" --install .. --config $<CONFIG> --prefix
      $<TARGET_BUNDLE_CONTENT_DIR:${target}> --component obs_plugin_dev >
      /dev/null
    COMMENT "Installing OBS plugins for development"
    VERBATIM)
endfunction()

# Helper function to finalize macOS app bundles
function(setup_obs_bundle target)
  install(
    CODE "
    set(_DEPENDENCY_PREFIX \"${CMAKE_PREFIX_PATH}\")
    set(_BUILD_FOR_DISTRIBUTION \"${BUILD_FOR_DISTRIBUTION}\")
    set(_BUNDLENAME \"$<TARGET_FILE_BASE_NAME:${target}>.app\")
    set(_BUNDLER_COMMAND \"${CMAKE_SOURCE_DIR}/cmake/bundle/macOS/dylibbundler\")
    set(_CODESIGN_IDENTITY \"${OBS_BUNDLE_CODESIGN_IDENTITY}\")
    set(_CODESIGN_ENTITLEMENTS \"${CMAKE_SOURCE_DIR}/cmake/bundle/macOS\")"
    COMPONENT obs_resources)

  if(ENABLE_SPARKLE_UPDATER)

    add_custom_command(
      TARGET ${target}
      POST_BUILD
      COMMAND
        /bin/sh -c
        "plutil -replace SUFeedURL -string https://obsproject.com/osx_update/stable/updates_${CMAKE_OSX_ARCHITECTURES}.xml \"$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Info.plist\""
      VERBATIM)

    add_custom_command(
      TARGET ${target}
      POST_BUILD
      COMMAND
        /bin/sh -c
        "plutil -replace SUPublicDSAKeyFile -string OBSPublicDSAKey.pem \"$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Info.plist\""
      VERBATIM)

    target_sources(
      ${target}
      PRIVATE "${CMAKE_SOURCE_DIR}/cmake/bundle/macOS/OBSPublicDSAKey.pem")
    set_source_files_properties(
      "${CMAKE_SOURCE_DIR}/cmake/bundle/macOS/OBSPublicDSAKey.pem"
      PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    source_group(
      "Resources"
      FILES "${CMAKE_SOURCE_DIR}/cmake/bundle/macOS/OBSPublicDSAKey.pem")

    install(
      DIRECTORY ${SPARKLE}
      DESTINATION $<TARGET_FILE_BASE_NAME:${target}>.app/Contents/Frameworks
      COMPONENT obs_frameworks)
  endif()

  install(SCRIPT "${CMAKE_SOURCE_DIR}/cmake/bundle/macOS/bundleutils.cmake"
          COMPONENT obs_resources)
endfunction()

# Helper function to export target to build and install tree Allows usage of
# `find_package(libobs)` by other build trees
function(export_target target)
  get_target_property(_IS_FRAMEWORK ${target} FRAMEWORK)

  set(OBS_PLUGIN_DESTINATION "")
  set(OBS_DATA_DESTINATION "")

  if(_IS_FRAMEWORK)
    export_framework_target(${target})
  else()
    _export_target(${ARGV})
  endif()
  set_target_properties(
    ${target} PROPERTIES PUBLIC_HEADER
                         "${CMAKE_CURRENT_BINARY_DIR}/${target}_EXPORT.h")
endfunction()

# Helper function to export macOS framework targets
function(export_framework_target)
  set(CMAKE_EXPORT_PACKAGE_REGISTRY OFF)

  include(GenerateExportHeader)
  generate_export_header(${target} EXPORT_FILE_NAME
                         "${CMAKE_CURRENT_BINARY_DIR}/${target}_EXPORT.h")

  target_sources(${target}
                 PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/${target}_EXPORT.h")

  set(TARGETS_EXPORT_NAME "${target}Targets")
  include(CMakePackageConfigHelpers)
  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${target}Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${target}Config.cmake"
    INSTALL_DESTINATION Frameworks/${target}.framework/Resources/cmake
    PATH_VARS OBS_PLUGIN_DESTINATION OBS_DATA_DESTINATION)

  write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${target}ConfigVersion.cmake
    VERSION ${OBS_VERSION_CANONICAL}
    COMPATIBILITY SameMajorVersion)

  export(
    EXPORT ${target}Targets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/${TARGETS_EXPORT_NAME}.cmake"
    NAMESPACE OBS::)

  export(PACKAGE "${target}")

  install(
    EXPORT ${TARGETS_EXPORT_NAME}
    FILE ${TARGETS_EXPORT_NAME}.cmake
    NAMESPACE OBS::
    DESTINATION Frameworks/${target}.framework/Resources/cmake
    COMPONENT obs_libraries
    EXCLUDE_FROM_ALL)

  install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/${target}Config.cmake
          ${CMAKE_CURRENT_BINARY_DIR}/${target}ConfigVersion.cmake
    DESTINATION
      Frameworks/$<TARGET_FILE_BASE_NAME:${target}>.framework/Resources/cmake
    COMPONENT obs_libraries
    EXCLUDE_FROM_ALL)
endfunction()

# Helper function to install header files
function(install_headers target)
  install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/"
    DESTINATION
      $<IF:$<BOOL:$<TARGET_PROPERTY:${target},FRAMEWORK>>,Frameworks/$<TARGET_FILE_BASE_NAME:${target}>.framework/Headers,${OBS_INCLUDE_DESTINATION}>
    COMPONENT obs_libraries
    EXCLUDE_FROM_ALL FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "obs-hevc.h" EXCLUDE
    PATTERN "*-windows.h" EXCLUDE
    PATTERN "*-x11.h" EXCLUDE
    PATTERN "*-wayland.h" EXCLUDE
    PATTERN "audio-monitoring/null" EXCLUDE
    PATTERN "audio-monitoring/win32" EXCLUDE
    PATTERN "audio-monitoring/pulse" EXCLUDE
    PATTERN "util/windows" EXCLUDE
    PATTERN "cmake" EXCLUDE
    PATTERN "pkgconfig" EXCLUDE
    PATTERN "data" EXCLUDE)

  if(ENABLE_HEVC)
    install(
      FILES "${CMAKE_CURRENT_SOURCE_DIR}/obs-hevc.h"
      DESTINATION
        $<IF:$<BOOL:$<TARGET_PROPERTY:${target},FRAMEWORK>>,Frameworks/$<TARGET_FILE_BASE_NAME:${target}>.framework/Headers,${OBS_INCLUDE_DESTINATION}>
      COMPONENT obs_libraries
      EXCLUDE_FROM_ALL)
  endif()

  install(
    FILES "${CMAKE_BINARY_DIR}/config/obsconfig.h"
    DESTINATION
      $<IF:$<BOOL:$<TARGET_PROPERTY:${target},FRAMEWORK>>,Frameworks/$<TARGET_FILE_BASE_NAME:${target}>.framework/Headers,${OBS_INCLUDE_DESTINATION}>
    COMPONENT obs_libraries
    EXCLUDE_FROM_ALL)
endfunction()
