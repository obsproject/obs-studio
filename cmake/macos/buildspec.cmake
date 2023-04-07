# OBS CMake macOS build dependencies module

# cmake-format: off
# cmake-lint: disable=E1126
# cmake-lint: disable=R0912
# cmake-lint: disable=R0915
# cmake-format: on

# _check_deps_version: Checks for obs-deps VERSION file in prefix paths
macro(_check_deps_version version)
  set(found FALSE)

  foreach(path IN LISTS CMAKE_PREFIX_PATH)
    if(EXISTS "${path}/share/obs-deps/VERSION")
      if(dependency STREQUAL qt6 AND NOT EXISTS "${path}/lib/cmake/Qt6/Qt6Config.cmake")
        set(found FALSE)
        continue()
      endif()

      file(READ "${path}/share/obs-deps/VERSION" _check_version)
      string(REPLACE "\n" "" _check_version "${_check_version}")

      if(_check_version VERSION_EQUAL version)
        set(found TRUE)
        break()
      elseif(_check_version VERSION_LESS version)
        message(AUTHOR_WARNING "Outdated ${label} version detected in ${path}: \n"
                               "Found ${_check_version}, require ${version}")
        list(REMOVE_ITEM CMAKE_PREFIX_PATH "${path}")
        list(APPEND CMAKE_PREFIX_PATH "${path}")
        continue()
      else()
        message(AUTHOR_WARNING "Future ${label} version detected in ${path}: \n"
                               "Found ${_check_version}, require ${version}")
        set(found TRUE)
        break()
      endif()
    endif()
  endforeach()
endmacro()

# _check_dependencies: Fetch and extract pre-built OBS build dependencies
function(_check_dependencies)
  if(CMAKE_OSX_ARCHITECTURES MATCHES ".+;.+")
    set(arch universal)
  else()
    set(arch ${CMAKE_OSX_ARCHITECTURES})
  endif()

  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/buildspec.json" buildspec)

  # cmake-format: off
  string(JSON deployment_target GET ${buildspec} platformConfig macos-${arch} deploymentTarget)
  string(JSON dependency_data GET ${buildspec} dependencies)
  # cmake-format: on

  if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET
        ${_deployment_target}
        CACHE STRING "Minimum macOS version to target for deployment (at runtime). Newer APIs will be weak-linked."
              FORCE)
  endif()

  set(dependencies_dir "${CMAKE_CURRENT_SOURCE_DIR}/.deps")
  set(prebuilt_filename "macos-deps-VERSION-ARCH-REVISION.tar.xz")
  set(prebuilt_destination "obs-deps-VERSION-ARCH")
  set(qt6_filename "macos-deps-qt6-VERSION-ARCH-REVISION.tar.xz")
  set(qt6_destination "obs-deps-qt6-VERSION-ARCH")
  set(cef_filename "cef_binary_VERSION_macos_ARCH_REVISION.tar.xz")
  set(cef_destination "cef_binary_VERSION_macos_ARCH")

  foreach(dependency IN ITEMS prebuilt qt6 cef)
    # cmake-format: off
    string(JSON data GET ${dependency_data} ${dependency})
    string(JSON version GET ${data} version)
    string(JSON hash GET ${data} hashes macos-${arch})
    string(JSON url GET ${data} baseUrl)
    string(JSON label GET ${data} label)
    string(JSON revision ERROR_VARIABLE error GET ${data} revision macos-${arch})
    # cmake-format: on

    message(STATUS "Setting up ${label}")

    set(file "${${dependency}_filename}")
    set(destination "${${dependency}_destination}")
    string(REPLACE "VERSION" "${version}" file "${file}")
    string(REPLACE "VERSION" "${version}" destination "${destination}")
    string(REPLACE "ARCH" "${arch}" file "${file}")
    string(REPLACE "ARCH" "${arch}" destination "${destination}")
    if(revision)
      string(REPLACE "_REVISION" "_v${revision}" file "${file}")
      string(REPLACE "-REVISION" "-v${revision}" file "${file}")
    else()
      string(REPLACE "_REVISION" "" file "${file}")
      string(REPLACE "-REVISION" "" file "${file}")
    endif()

    set(skip FALSE)
    if(dependency STREQUAL prebuilt OR dependency STREQUAL qt6)
      _check_deps_version(${version})

      if(found)
        set(skip TRUE)
      endif()
    elseif(_dependency STREQUAL sparkle)
      find_library(SPARKLE Sparkle)

      if(NOT ENABLE_SPARKLE OR SPARKLE)
        set(skip TRUE)
      else()
        unset(SPARKLE CACHE)
      endif()
    elseif(dependency STREQUAL vlc)
      if(NOT ENABLE_VLC OR (VLC_PATH AND EXISTS "${VLC_PATH}"))
        set(skip TRUE)
      endif()
    elseif(dependency STREQUAL cef)
      if(NOT ENABLE_BROWSER OR (CEF_ROOT_DIR AND EXISTS "${CEF_ROOT_DIR}"))
        set(skip TRUE)
      endif()
    endif()

    if(skip)
      message(STATUS "Setting up ${label} - skipped")
      continue()
    endif()

    if(dependency STREQUAL qt6 AND "$ENV{CI}")
      # cmake-format: off
      string(JSON hash GET ${buildspec} dependencies qt6 hashes macos-universal)
      # cmake-format: on
      string(REPLACE "${arch}" "universal" file "${file}")
    endif()

    if(dependency STREQUAL cef)
      set(url ${url}/${file})
    else()
      set(url ${url}/${version}/${file})
    endif()

    if(NOT EXISTS "${dependencies_dir}/${file}")
      message(STATUS "Downloading ${url}")
      file(
        DOWNLOAD "${url}" "${dependencies_dir}/${file}"
        STATUS download_status
        EXPECTED_HASH SHA256=${hash})

      list(GET download_status 0 error_code)
      list(GET download_status 1 error_message)
      if(error_code GREATER 0)
        message(STATUS "Downloading ${url} - Failure")
        message(FATAL_ERROR "Unable to download ${url}, failed with error: ${error_message}")
        file(REMOVE "${dependencies_dir}/${file}")
      else()
        message(STATUS "Downloading ${url} - done")
      endif()
    endif()

    if(NOT EXISTS "${dependencies_dir}/${destination}")
      file(MAKE_DIRECTORY "${dependencies_dir}/${destination}")
      if(dependency STREQUAL vlc)
        file(ARCHIVE_EXTRACT INPUT "${dependencies_dir}/${file}" DESTINATION "${dependencies_dir}")
      else()
        file(ARCHIVE_EXTRACT INPUT "${dependencies_dir}/${file}" DESTINATION "${dependencies_dir}/${destination}")
      endif()
      execute_process(COMMAND "xattr" -r -d com.apple.quarantine "${dependencies_dir}/${destination}"
                      RESULT_VARIABLE result COMMAND_ERROR_IS_FATAL ANY)
    endif()

    if(dependency STREQUAL cef)
      set(CEF_ROOT_DIR
          "${dependencies_dir}/${destination}"
          CACHE PATH "CEF Root directory" FORCE)
    elseif(dependency STREQUAL prebuilt)
      set(VLC_PATH
          "${dependencies_dir}/${destination}"
          CACHE PATH "VLC source code directory" FORCE)
      list(APPEND CMAKE_PREFIX_PATH "${dependencies_dir}/${destination}")
    elseif(dependency STREQUAL qt6)
      list(APPEND CMAKE_PREFIX_PATH "${dependencies_dir}/${destination}")
    endif()

    message(STATUS "Setting up ${label} - done")
  endforeach()

  list(REMOVE_DUPLICATES CMAKE_PREFIX_PATH)

  set(CMAKE_PREFIX_PATH
      ${CMAKE_PREFIX_PATH}
      CACHE PATH "CMake prefix search path" FORCE)
endfunction()

_check_dependencies()
