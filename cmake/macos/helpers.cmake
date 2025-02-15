# OBS CMake macOS helper functions module

include_guard(GLOBAL)

include(helpers_common)

# set_target_xcode_properties: Sets Xcode-specific target attributes
function(set_target_xcode_properties target)
  set(options "")
  set(oneValueArgs "")
  set(multiValueArgs PROPERTIES)
  cmake_parse_arguments(PARSE_ARGV 0 _STXP "${options}" "${oneValueArgs}" "${multiValueArgs}")

  message(DEBUG "Setting Xcode properties for target ${target}...")

  while(_STXP_PROPERTIES)
    list(POP_FRONT _STXP_PROPERTIES key value)
    set_property(TARGET ${target} PROPERTY XCODE_ATTRIBUTE_${key} "${value}")
  endwhile()
endfunction()

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

  string(TIMESTAMP CURRENT_YEAR "%Y")

  # Target is a GUI or CLI application
  if(target_type STREQUAL EXECUTABLE)
    if(target STREQUAL obs-studio)
      set_target_properties(
        ${target}
        PROPERTIES
          OUTPUT_NAME OBS
          MACOSX_BUNDLE TRUE
          MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macos/Info.plist.in"
          XCODE_EMBED_FRAMEWORKS_REMOVE_HEADERS_ON_COPY YES
          XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY YES
          XCODE_EMBED_PLUGINS_REMOVE_HEADERS_ON_COPY YES
          XCODE_EMBED_PLUGINS_CODE_SIGN_ON_COPY YES
      )

      set_target_xcode_properties(
        ${target}
        PROPERTIES PRODUCT_BUNDLE_IDENTIFIER com.obsproject.obs-studio
                   PRODUCT_NAME OBS
                   ASSETCATALOG_COMPILER_APPICON_NAME AppIcon
                   CURRENT_PROJECT_VERSION ${OBS_BUILD_NUMBER}
                   MARKETING_VERSION ${OBS_VERSION_CANONICAL}
                   GENERATE_INFOPLIST_FILE YES
                   COPY_PHASE_STRIP NO
                   CLANG_ENABLE_OBJC_ARC YES
                   SKIP_INSTALL NO
                   INSTALL_PATH "$(LOCAL_APPS_DIR)"
                   INFOPLIST_KEY_CFBundleDisplayName "OBS Studio"
                   INFOPLIST_KEY_NSHumanReadableCopyright "(c) 2012-${CURRENT_YEAR} Lain Bailey"
                   INFOPLIST_KEY_NSCameraUsageDescription "OBS needs to access the camera to enable camera sources to work."
                   INFOPLIST_KEY_NSMicrophoneUsageDescription "OBS needs to access the microphone to enable audio input."
                   INFOPLIST_KEY_NSAppleEventsUsageDescription "OBS needs to access background events to enable hotkeys while not in focus."
      )

      get_property(obs_dependencies GLOBAL PROPERTY _OBS_DEPENDENCIES)
      add_dependencies(${target} ${obs_dependencies})

      get_property(obs_frameworks GLOBAL PROPERTY _OBS_FRAMEWORKS)
      set_property(TARGET ${target} APPEND PROPERTY XCODE_EMBED_FRAMEWORKS ${obs_frameworks})

      if(SPARKLE_APPCAST_URL AND SPARKLE_PUBLIC_KEY)
        set_property(TARGET ${target} APPEND PROPERTY XCODE_EMBED_FRAMEWORKS ${SPARKLE})
      endif()

      if(TARGET mac-syphon)
        set_property(TARGET ${target} APPEND PROPERTY XCODE_EMBED_FRAMEWORKS ${SYPHON})
      endif()

      get_property(obs_executables GLOBAL PROPERTY _OBS_EXECUTABLES)
      add_dependencies(${target} ${obs_executables})
      foreach(executable IN LISTS obs_executables)
        set_target_xcode_properties(${executable} PROPERTIES INSTALL_PATH
                                    "$(LOCAL_APPS_DIR)/$<TARGET_BUNDLE_DIR_NAME:${target}>/Contents/MacOS"
        )

        add_custom_command(
          TARGET ${target}
          POST_BUILD
          COMMAND
            "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:${executable}>"
            "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/MacOS/"
          COMMENT "Copy ${executable} to application bundle"
        )
      endforeach()

      if(VIRTUALCAM_DEVICE_UUID AND VIRTUALCAM_SOURCE_UUID AND VIRTUALCAM_SINK_UUID)
        set(has_virtualcam_uuids TRUE)
      else()
        set(has_virtualcam_uuids FALSE)
      endif()

      if(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_STYLE STREQUAL Automatic)
        if(has_virtualcam_uuids)
          set(entitlements_file "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macos/entitlements-extension.plist")
        else()
          set(entitlements_file "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macos/entitlements.plist")
        endif()
      else()
        if(has_virtualcam_uuids AND OBS_PROVISIONING_PROFILE)
          set(entitlements_file "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macos/entitlements-extension.plist")
          set_target_properties(
            ${target}
            PROPERTIES XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "${OBS_PROVISIONING_PROFILE}"
          )
          configure_file(cmake/macos/exportOptions-extension.plist.in ${CMAKE_BINARY_DIR}/exportOptions.plist)
        else()
          set(entitlements_file "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macos/entitlements.plist")
          configure_file(cmake/macos/exportOptions.plist.in ${CMAKE_BINARY_DIR}/exportOptions.plist)
        endif()
      endif()

      if(NOT EXISTS "${entitlements_file}")
        message(FATAL_ERROR "Target ${target} is missing an entitlements file in its cmake directory.")
      endif()

      set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${entitlements_file}")

      add_custom_command(
        TARGET ${target}
        POST_BUILD
        COMMAND /bin/ln -fs obs-frontend-api.dylib libobs-frontend-api.1.dylib
        WORKING_DIRECTORY "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Frameworks"
        COMMENT "Create symlink for legacy obs-frontend-api"
      )

      if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macos/qt.conf")
        target_add_resource(${target} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macos/qt.conf")
      endif()

      target_add_resource(${target} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macos/Assets.xcassets")
      target_add_resource(${target} "${CMAKE_CURRENT_SOURCE_DIR}/../AUTHORS")

      if(TARGET obs-dal-plugin)
        add_custom_command(
          TARGET ${target}
          POST_BUILD
          COMMAND
            "${CMAKE_COMMAND}" -E copy_directory "$<TARGET_BUNDLE_DIR:obs-dal-plugin>"
            "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Resources/$<TARGET_BUNDLE_DIR_NAME:obs-dal-plugin>"
          COMMENT "Add OBS DAL plugin to application bundle"
        )
      endif()

      if(TARGET obspython)
        add_custom_command(
          TARGET ${target}
          POST_BUILD
          COMMAND
            "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE_DIR:obspython>/obspython.py"
            "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Resources"
          COMMENT "Add OBS::python import module"
        )
      endif()

      if(
        TARGET mac-camera-extension
        AND (CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_STYLE STREQUAL Automatic OR OBS_PROVISIONING_PROFILE)
      )
        target_enable_feature(mac-camera-extension "macOS CMIO Camera Extension")
        add_custom_command(
          TARGET ${target}
          POST_BUILD
          COMMAND
            "${CMAKE_COMMAND}" -E copy_directory "$<TARGET_BUNDLE_DIR:mac-camera-extension>"
            "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Library/SystemExtensions/$<TARGET_BUNDLE_DIR_NAME:mac-camera-extension>"
          COMMENT "Add Camera Extension to application bundle"
        )
      else()
        target_disable_feature(mac-camera-extension "macOS CMIO Camera Extension")
      endif()

      _bundle_dependencies(${target})

      install(TARGETS ${target} BUNDLE DESTINATION "." COMPONENT Application)
    elseif(${target} STREQUAL mac-camera-extension)
      set_target_properties(${target} PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
      set_property(GLOBAL APPEND PROPERTY _OBS_DEPENDENCIES ${target})
    elseif(${target} STREQUAL obs-ffmpeg-mux)
      if(OBS_CODESIGN_IDENTITY STREQUAL "-")
        set_target_xcode_properties(${target} PROPERTIES ENABLE_HARDENED_RUNTIME NO)
      endif()

      set_target_xcode_properties(${target} PROPERTIES SKIP_INSTALL NO)
      set_property(GLOBAL APPEND PROPERTY _OBS_EXECUTABLES ${target})
      set_property(GLOBAL APPEND PROPERTY _OBS_DEPENDENCIES ${target})
    else()
      set_property(TARGET ${target} PROPERTY XCODE_ATTRIBUTE_SKIP_INSTALL NO)
      set_property(GLOBAL APPEND PROPERTY _OBS_EXECUTABLES ${target})
      set_property(GLOBAL APPEND PROPERTY _OBS_DEPENDENCIES ${target})
      _add_entitlements()
    endif()
  elseif(target_type STREQUAL SHARED_LIBRARY)
    set_target_properties(
      ${target}
      PROPERTIES
        NO_SONAME TRUE
        MACHO_COMPATIBILITY_VERSION 1.0
        MACHO_CURRENT_VERSION ${OBS_VERSION_MAJOR}
        SOVERSION 0
        VERSION 0
    )

    set_target_xcode_properties(
      ${target}
      PROPERTIES DYLIB_COMPATIBILITY_VERSION 1.0
                 DYLIB_CURRENT_VERSION ${OBS_VERSION_MAJOR}
                 PRODUCT_NAME ${target}
                 PRODUCT_BUNDLE_IDENTIFIER com.obsproject.${target}
                 SKIP_INSTALL YES
    )

    get_target_property(is_framework ${target} FRAMEWORK)
    if(is_framework)
      set_target_properties(
        ${target}
        PROPERTIES FRAMEWORK_VERSION A MACOSX_FRAMEWORK_IDENTIFIER com.obsproject.${target}
      )

      set_target_xcode_properties(
        ${target}
        PROPERTIES CODE_SIGN_IDENTITY ""
                   DEVELOPMENT_TEAM ""
                   SKIP_INSTALL YES
                   PRODUCT_NAME ${target}
                   PRODUCT_BUNDLE_IDENTIFIER com.obsproject.${target}
                   CURRENT_PROJECT_VERSION ${OBS_BUILD_NUMBER}
                   MARKETING_VERSION ${OBS_VERSION_CANONICAL}
                   GENERATE_INFOPLIST_FILE YES
                   INFOPLIST_FILE ""
                   INFOPLIST_KEY_CFBundleDisplayName ${target}
                   INFOPLIST_KEY_NSHumanReadableCopyright "(c) 2012-${CURRENT_YEAR} Lain Bailey"
      )
    endif()

    set_property(GLOBAL APPEND PROPERTY _OBS_FRAMEWORKS ${target})
    set_property(GLOBAL APPEND PROPERTY _OBS_DEPENDENCIES ${target})
  elseif(target_type STREQUAL MODULE_LIBRARY)
    if(target STREQUAL obspython)
      set_target_xcode_properties(
        ${target}
        PROPERTIES PRODUCT_NAME ${target}
                   PRODUCT_BUNDLE_IDENTIFIER com.obsproject.${target}
      )
    elseif(target STREQUAL obslua)
      set_target_xcode_properties(
        ${target}
        PROPERTIES PRODUCT_NAME ${target}
                   PRODUCT_BUNDLE_IDENTIFIER com.obsproject.${target}
      )
    elseif(target STREQUAL obs-dal-plugin)
      set_target_properties(${target} PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
      set_property(GLOBAL APPEND PROPERTY _OBS_DEPENDENCIES ${target})
      return()
    else()
      set_target_properties(${target} PROPERTIES BUNDLE TRUE BUNDLE_EXTENSION plugin)

      set_target_xcode_properties(
        ${target}
        PROPERTIES PRODUCT_NAME ${target}
                   PRODUCT_BUNDLE_IDENTIFIER com.obsproject.${target}
                   CURRENT_PROJECT_VERSION ${OBS_BUILD_NUMBER}
                   MARKETING_VERSION ${OBS_VERSION_CANONICAL}
                   GENERATE_INFOPLIST_FILE YES
                   INFOPLIST_KEY_CFBundleDisplayName ${target}
                   INFOPLIST_KEY_NSHumanReadableCopyright "(c) 2012-${CURRENT_YEAR} Lain Bailey"
      )

      if(target STREQUAL obs-browser)
        # Good-enough for now as there are no other variants - in _theory_ we should only add the appropriate variant,
        # but that is only known at project generation and not build system configuration.
        get_target_property(imported_location CEF::Library IMPORTED_LOCATION_RELEASE)
        if(imported_location)
          list(APPEND cef_items "${imported_location}")
        endif()

        foreach(helper IN ITEMS _gpu _plugin _renderer "")
          if(TARGET OBS::browser-helper${helper})
            set_property(GLOBAL APPEND PROPERTY _OBS_DEPENDENCIES OBS::browser-helper${helper})
            list(APPEND cef_items OBS::browser-helper${helper})
          endif()
        endforeach()

        set_property(GLOBAL APPEND PROPERTY _OBS_FRAMEWORKS ${cef_items})
      endif()
    endif()

    set_property(GLOBAL APPEND PROPERTY OBS_MODULES_ENABLED ${target})
    set_property(GLOBAL APPEND PROPERTY _OBS_DEPENDENCIES ${target})
  endif()

  target_install_resources(${target})

  get_target_property(target_sources ${target} SOURCES)
  set(target_ui_files ${target_sources})
  list(FILTER target_ui_files INCLUDE REGEX ".+\\.(ui|qrc)")
  source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "UI Files" FILES ${target_ui_files})

  if(${target} STREQUAL libobs)
    set(target_source_files ${target_sources})
    set(target_header_files ${target_sources})
    list(FILTER target_source_files INCLUDE REGEX ".+\\.(m|c[cp]?p?|swift)")
    list(FILTER target_header_files INCLUDE REGEX ".+\\.h(pp)?")

    source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Source Files" FILES ${target_source_files})
    source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Header Files" FILES ${target_header_files})
  endif()
endfunction()

# _check_entitlements: Macro to check if project ships with entitlements plist
macro(_check_entitlements)
  if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macos/entitlements.plist")
    message(FATAL_ERROR "Target ${target} is missing an entitlements.plist in its cmake directory.")
  endif()
endmacro()

# _add_entitlements: Macro to add entitlements shipped with project
macro(_add_entitlements)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macos/entitlements.plist")
    set_target_xcode_properties(
      ${target}
      PROPERTIES CODE_SIGN_ENTITLEMENTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macos/entitlements.plist"
    )
  endif()
endmacro()

# target_export: Helper function to export target as CMake package
function(target_export target)
  # Exclude CMake package from 'ALL' target
  set(exclude_variant EXCLUDE_FROM_ALL)
  _target_export(${target})
endfunction()

# target_install_resources: Helper function to add resources into bundle
function(target_install_resources target)
  message(DEBUG "Installing resources for target ${target}...")
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data")
    file(GLOB_RECURSE data_files "${CMAKE_CURRENT_SOURCE_DIR}/data/*")
    list(FILTER data_files EXCLUDE REGEX "\\.DS_Store$")
    foreach(data_file IN LISTS data_files)
      cmake_path(
        RELATIVE_PATH
        data_file
        BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/data/"
        OUTPUT_VARIABLE relative_path
      )
      cmake_path(GET relative_path PARENT_PATH relative_path)
      target_sources(${target} PRIVATE "${data_file}")
      set_property(SOURCE "${data_file}" PROPERTY MACOSX_PACKAGE_LOCATION "Resources/${relative_path}")
      source_group("Resources/${relative_path}" FILES "${data_file}")
    endforeach()
  endif()
endfunction()

# target_add_resource: Helper function to add a specific resource to a bundle
function(target_add_resource target resource)
  message(DEBUG "Add resource ${resource} to target ${target} at destination ${destination}...")
  target_sources(${target} PRIVATE "${resource}")
  set_property(SOURCE "${resource}" PROPERTY MACOSX_PACKAGE_LOCATION Resources)
  source_group("Resources" FILES "${resource}")
endfunction()

# _bundle_dependencies: Resolve 3rd party dependencies and add them to macOS app bundle
function(_bundle_dependencies target)
  message(DEBUG "Discover dependencies of target ${target}...")
  set(found_dependencies)
  find_dependencies(TARGET ${target} FOUND_VAR found_dependencies)

  get_property(obs_module_list GLOBAL PROPERTY OBS_MODULES_ENABLED)
  list(LENGTH obs_module_list num_modules)
  if(num_modules GREATER 0)
    add_dependencies(${target} ${obs_module_list})
    set_property(TARGET ${target} APPEND PROPERTY XCODE_EMBED_PLUGINS ${obs_module_list})
    foreach(module IN LISTS obs_module_list)
      find_dependencies(TARGET ${module} FOUND_VAR found_dependencies)
    endforeach()
  endif()

  list(REMOVE_DUPLICATES found_dependencies)

  set(library_paths)
  set(plugins_list)
  file(GLOB sdk_library_paths /Applications/Xcode*.app)
  set(system_library_path "/usr/lib/")

  foreach(library IN LISTS found_dependencies)
    get_target_property(library_type ${library} TYPE)
    get_target_property(is_framework ${library} FRAMEWORK)
    get_target_property(is_imported ${library} IMPORTED)

    if(is_imported)
      get_target_property(imported_location ${library} LOCATION)
      if(NOT imported_location)
        continue()
      endif()

      set(is_xcode_framework FALSE)
      set(is_system_framework FALSE)

      foreach(sdk_library_path IN LISTS sdk_library_paths)
        if(is_xcode_framework)
          break()
        endif()
        cmake_path(IS_PREFIX sdk_library_path "${imported_location}" is_xcode_framework)
      endforeach()
      cmake_path(IS_PREFIX system_library_path "${imported_location}" is_system_framework)

      if(is_system_framework OR is_xcode_framework)
        continue()
      elseif(is_framework)
        file(REAL_PATH "../../.." library_location BASE_DIRECTORY "${imported_location}")
      elseif(NOT library_type STREQUAL "STATIC_LIBRARY")
        if(NOT imported_location MATCHES ".+\\.a")
          set(library_location "${imported_location}")
        else()
          continue()
        endif()
      else()
        continue()
      endif()

      if(library MATCHES "Qt6?::.+")
        find_qt_plugins(COMPONENT ${library} TARGET ${target} FOUND_VAR plugins_list)
      endif()
      list(APPEND library_paths ${library_location})
    elseif(NOT is_imported AND library_type STREQUAL "SHARED_LIBRARY")
      list(APPEND library_paths ${library})
    endif()
  endforeach()

  list(REMOVE_DUPLICATES plugins_list)
  foreach(plugin IN LISTS plugins_list)
    cmake_path(GET plugin PARENT_PATH plugin_path)
    set(plugin_base_dir "${plugin_path}/../")
    cmake_path(SET plugin_stem_dir NORMALIZE "${plugin_base_dir}")
    cmake_path(RELATIVE_PATH plugin_path BASE_DIRECTORY "${plugin_stem_dir}" OUTPUT_VARIABLE plugin_file_name)
    target_sources(${target} PRIVATE "${plugin}")
    set_source_files_properties(
      "${plugin}"
      PROPERTIES MACOSX_PACKAGE_LOCATION "plugins/${plugin_file_name}" XCODE_FILE_ATTRIBUTES "CodeSignOnCopy"
    )
    source_group("Qt plugins" FILES "${plugin}")
  endforeach()

  list(REMOVE_DUPLICATES library_paths)
  set_property(TARGET ${target} APPEND PROPERTY XCODE_EMBED_FRAMEWORKS ${library_paths})
endfunction()
