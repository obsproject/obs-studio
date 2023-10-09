# OBS common build dependencies module

# cmake-format: off
# cmake-lint: disable=C0103
# cmake-lint: disable=E1126
# cmake-lint: disable=R0912
# cmake-lint: disable=R0915
# cmake-format: on

include_guard(GLOBAL)

# _check_deps_version: Checks for obs-deps VERSION file in prefix paths
function(_check_deps_version version)
  # cmake-format: off
  set(found FALSE PARENT_SCOPE)
  # cmake-format: on

  foreach(path IN LISTS CMAKE_PREFIX_PATH)
    if(EXISTS "${path}/share/obs-deps/VERSION")
      if(dependency STREQUAL qt6 AND NOT EXISTS "${path}/lib/cmake/Qt6/Qt6Config.cmake")
        # cmake-format: off
        set(found FALSE PARENT_SCOPE)
        # cmake-format: on
        continue()
      endif()

      file(READ "${path}/share/obs-deps/VERSION" _check_version)
      string(REPLACE "\n" "" _check_version "${_check_version}")
      string(REPLACE "-" "." _check_version "${_check_version}")
      string(REPLACE "-" "." version "${version}")

      if(_check_version VERSION_EQUAL version)
        # cmake-format: off
        set(found TRUE PARENT_SCOPE)
        # cmake-format: on
        break()
      elseif(_check_version VERSION_LESS version)
        message(AUTHOR_WARNING "Older ${label} version detected in ${path}: \n"
                               "Found ${_check_version}, require ${version}")
        list(REMOVE_ITEM CMAKE_PREFIX_PATH "${path}")
        list(APPEND CMAKE_PREFIX_PATH "${path}")
        # cmake-format: off
        set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} PARENT_SCOPE)
        # cmake-format: on
        continue()
      else()
        message(AUTHOR_WARNING "Newer ${label} version detected in ${path}: \n"
                               "Found ${_check_version}, require ${version}")
        # cmake-format: off
        set(found TRUE PARENT_SCOPE)
        # cmake-format: on
        break()
      endif()
    endif()
  endforeach()
endfunction()

# _check_dependencies: Fetch and extract pre-built OBS build dependencies
function(_check_dependencies)
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/buildspec.json" buildspec)

  # cmake-format: off
  string(JSON dependency_data GET ${buildspec} dependencies)
  # cmake-format: on

  foreach(dependency IN LISTS dependencies_list)
    if(dependency STREQUAL cef AND arch STREQUAL universal)
      if(CMAKE_OSX_ARCHITECTURES MATCHES ".+;.+")
        continue()
      endif()
      set(arch ${CMAKE_OSX_ARCHITECTURES})
      set(platform macos-${arch})
    endif()

    # cmake-format: off
    string(JSON data GET ${dependency_data} ${dependency})
    string(JSON version GET ${data} version)
    string(JSON hash GET ${data} hashes ${platform})
    string(JSON url GET ${data} baseUrl)
    string(JSON label GET ${data} label)
    string(JSON revision ERROR_VARIABLE error GET ${data} revision ${platform})
    # cmake-format: on

    message(STATUS "Setting up ${label} (${arch})")

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

    if(EXISTS "${dependencies_dir}/.dependency_${dependency}_${arch}.sha256")
      file(READ "${dependencies_dir}/.dependency_${dependency}_${arch}.sha256"
           OBS_DEPENDENCY_${dependency}_${arch}_HASH)
    endif()

    set(skip FALSE)
    if(dependency STREQUAL prebuilt OR dependency STREQUAL qt6)
      if(OBS_DEPENDENCY_${dependency}_${arch}_HASH STREQUAL ${hash})
        _check_deps_version(${version})

        if(found)
          set(skip TRUE)
        endif()
      endif()
    elseif(dependency STREQUAL cef)
      if(NOT ENABLE_BROWSER)
        set(skip TRUE)
      elseif(OBS_DEPENDENCY_${dependency}_${arch}_HASH STREQUAL ${hash} AND (CEF_ROOT_DIR AND EXISTS "${CEF_ROOT_DIR}"))
        set(skip TRUE)
      endif()
    endif()

    if(skip)
      message(STATUS "Setting up ${label} (${arch}) - skipped")
      continue()
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

    if(NOT OBS_DEPENDENCY_${dependency}_${arch}_HASH STREQUAL ${hash})
      file(REMOVE_RECURSE "${dependencies_dir}/${destination}")
    endif()

    if(NOT EXISTS "${dependencies_dir}/${destination}")
      file(MAKE_DIRECTORY "${dependencies_dir}/${destination}")
      if(dependency STREQUAL obs-studio)
        file(ARCHIVE_EXTRACT INPUT "${dependencies_dir}/${file}" DESTINATION "${dependencies_dir}")
      else()
        file(ARCHIVE_EXTRACT INPUT "${dependencies_dir}/${file}" DESTINATION "${dependencies_dir}/${destination}")
      endif()
    endif()

    file(WRITE "${dependencies_dir}/.dependency_${dependency}_${arch}.sha256" "${hash}")

    if(dependency STREQUAL prebuilt)
      set(VLC_PATH
          "${dependencies_dir}/${destination}"
          CACHE PATH "VLC source code directory" FORCE)
      list(APPEND CMAKE_PREFIX_PATH "${dependencies_dir}/${destination}")
    elseif(dependency STREQUAL qt6)
      list(APPEND CMAKE_PREFIX_PATH "${dependencies_dir}/${destination}")
    elseif(dependency STREQUAL cef)
      set(CEF_ROOT_DIR
          "${dependencies_dir}/${destination}"
          CACHE PATH "CEF root directory" FORCE)
    endif()

    message(STATUS "Setting up ${label} (${arch}) - done")
  endforeach()

  list(REMOVE_DUPLICATES CMAKE_PREFIX_PATH)

  # cmake-format: off
  set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} CACHE PATH "CMake prefix search path" FORCE)
  # cmake-format: on
endfunction()
