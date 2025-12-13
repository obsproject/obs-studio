# OBS CMake Windows build dependencies module

include_guard(GLOBAL)

include(buildspec_common)

# _handle_qt_cross_compile: Check for and handle cross compiled Qt dependency
function(_handle_qt_cross_compile architecture)
  set(options "")
  set(oneValueArgs DIRECTORY)
  set(multiValueArgs "")
  cmake_parse_arguments(PARSE_ARGV 0 _HQCC "${options}" "${oneValueArgs}" "${multiValueArgs}")

  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/buildspec.json" buildspec)

  string(JSON dependency_data GET ${buildspec} dependencies)
  string(JSON data GET ${dependency_data} qt6)
  string(JSON version GET ${data} version)

  set(qt_build_arch "")
  set(qt_target_arch "")
  set(host_arch "")
  set(platform_name "")
  set(config_has_buildabi FALSE)
  set(qt_cross_compiled FALSE)

  string(REPLACE "VERSION" "${version}" directory "${_HQCC_DIRECTORY}")
  string(TOLOWER "${CMAKE_VS_PLATFORM_NAME}" platform_name)
  string(REPLACE "ARCH" "${platform_name}" qt_arch_location "${directory}")

  file(READ "${qt_arch_location}/mkspecs/qconfig.pri" qt_arch_config)
  string(REGEX MATCH ".+QT_TARGET_BUILDABI = (.+)\n.+" config_has_buildabi "${qt_arch_config}")

  if(config_has_buildabi)
    string(
      REGEX REPLACE
      "host_build {\n[ \t]+QT_ARCH = (x86_64|arm64)\n.+[ \t]+QT_TARGET_ARCH = (x86_64|arm64)\n.+}.+"
      "\\1;\\2"
      host_build_tuple
      "${qt_arch_config}"
    )
    list(GET host_build_tuple 0 qt_build_arch)
    list(GET host_build_tuple 1 qt_target_arch)
    set(qt_cross_compiled TRUE)
  else()
    string(REGEX REPLACE ".*QT_ARCH = (x86_64|arm64)\n.+" "\\1" build_arch "${qt_arch_config}")
    set(qt_build_arch "${build_arch}")
    set(qt_target_arch "${build_arch}")
  endif()

  if(NOT qt_build_arch MATCHES "x86_64|arm64" OR NOT qt_target_arch MATCHES "x86_64|arm64")
    message(FATAL_ERROR "Unable to detect host or target architecture from Qt dependencies in '${qt_arch_location}'")
  endif()

  string(REPLACE "x86_64" "x64" qt_build_arch "${qt_build_arch}")
  string(REPLACE "x86_64" "x64" qt_target_arch "${qt_target_arch}")
  string(REPLACE "AMD64" "x64" architecture "${architecture}")
  string(REPLACE "ARM64" "arm64" architecture "${architecture}")

  if(NOT qt_cross_compiled)
    if(architecture STREQUAL qt_target_arch OR (architecture STREQUAL arm64 AND qt_target_arch STREQUAL x64))
      unset(QT_HOST_PATH CACHE)
      unset(QT_REQUIRE_HOST_PATH_CHECK CACHE)
      return()
    endif()

    set(QT_REQUIRE_HOST_PATH_CHECK TRUE CACHE STRING "Qt Host Tools Check Required" FORCE)
  endif()

  if(NOT DEFINED QT_HOST_PATH)
    string(REPLACE "${qt_target_arch}" "${architecture}" host_tools_directory "${qt_arch_location}")

    if(NOT IS_DIRECTORY "${host_tools_directory}")
      message(
        FATAL_ERROR
        "Required Qt host tools for ${architecture} when building for ${qt_target_arch} not found in '${host_tools_directory}'"
      )
    endif()

    set(QT_HOST_PATH "${host_tools_directory}" CACHE STRING "Qt Host Tools Path" FORCE)
  endif()
endfunction()

# _check_dependencies_windows: Set up Windows slice for _check_dependencies
function(_check_dependencies_windows)
  set(dependencies_dir "${CMAKE_CURRENT_SOURCE_DIR}/.deps")
  set(prebuilt_filename "windows-deps-VERSION-ARCH-REVISION.zip")
  set(prebuilt_destination "obs-deps-VERSION-ARCH")
  set(qt6_filename "windows-deps-qt6-VERSION-ARCH-REVISION.zip")
  set(qt6_destination "obs-deps-qt6-VERSION-ARCH")
  set(cef_filename "cef_binary_VERSION_windows_ARCH_REVISION.zip")
  set(cef_destination "cef_binary_VERSION_windows_ARCH")

  if(CMAKE_VS_PLATFORM_NAME STREQUAL Win32)
    set(arch x86)
    set(dependencies_list prebuilt)
  else()
    string(TOLOWER "${CMAKE_VS_PLATFORM_NAME}" arch)
    set(dependencies_list prebuilt qt6 cef)
  endif()
  set(platform windows-${arch})

  _check_dependencies()

  if(NOT CMAKE_VS_PLATFORM_NAME STREQUAL Win32)
    _handle_qt_cross_compile(${CMAKE_HOST_SYSTEM_PROCESSOR} DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.deps/${qt6_destination}")
  endif()
endfunction()

_check_dependencies_windows()
