# OBS common build dependencies module

# cmake-format: off
# cmake-lint: disable=E1121
# cmake-lint: disable=E1126
# cmake-lint: disable=R0912
# cmake-lint: disable=R0915
# cmake-format: on

include_guard(GLOBAL)

function(extract_archive file destination)
  # Check if the file exists
  if(NOT EXISTS "${file}")
    message(FATAL_ERROR "File not found: ${file}")
  endif()

  # Determine the file extension
  get_filename_component(extension "${file}" EXT)

  # If it's a .7z file, use 7z to extract
  if("${extension}" STREQUAL ".7z")
    message(STATUS "Extracting .7z archive: ${file}")
    execute_process(
      COMMAND 7z x "${file}" -o"${destination}" -y
      RESULT_VARIABLE _result
      OUTPUT_QUIET
      ERROR_QUIET
    )
    if(NOT _result EQUAL 0)
      message(FATAL_ERROR "Failed to extract .7z archive: ${file}")
    endif()
  else()
    # For other archive types, use file(ARCHIVE_EXTRACT)
    message(STATUS "Extracting archive using CMake: ${file}")
    file(ARCHIVE_EXTRACT INPUT "${file}" DESTINATION "${destination}")
  endif()

  message(STATUS "Extraction complete: ${file} -> ${destination}")
endfunction()

# _check_deps_version: Checks for obs-deps VERSION file in prefix paths
function(_check_deps_version version)
  set(found
      FALSE
      PARENT_SCOPE)
  message(STATUS "Checking CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH}")
  foreach(path IN LISTS CMAKE_PREFIX_PATH)
    if(EXISTS "${path}/share/obs-deps/VERSION")
      message(STATUS "Checking ${path}/share/obs-deps/VERSION")
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
        set(found
            TRUE
            PARENT_SCOPE)
        break()
      elseif(_check_version VERSION_LESS version)
        message(AUTHOR_WARNING "Older ${label} version detected in ${path}: \n"
                               "Found ${_check_version}, require ${version}")
        list(REMOVE_ITEM CMAKE_PREFIX_PATH "${path}")
        list(APPEND CMAKE_PREFIX_PATH "${path}")
        set(CMAKE_PREFIX_PATH
            ${CMAKE_PREFIX_PATH}
            PARENT_SCOPE)
        continue()
      else()
        message(AUTHOR_WARNING "Newer ${label} version detected in ${path}: \n"
                               "Found ${_check_version}, require ${version}")
        set(found
            TRUE
            PARENT_SCOPE)
        break()
      endif()
    endif()
  endforeach()
endfunction()

# _check_dependencies: Fetch and extract pre-built OBS build dependencies
function(_check_dependencies)
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/buildspec.json" buildspec)

  string(JSON dependency_data GET ${buildspec} dependencies)

  foreach(dependency IN LISTS dependencies_list)
    if(dependency STREQUAL cef AND arch STREQUAL universal)
      if(CMAKE_OSX_ARCHITECTURES MATCHES ".+;.+")
        continue()
      endif()
      set(arch ${CMAKE_OSX_ARCHITECTURES})
      set(platform macos-${arch})
    endif()

    string(JSON data GET ${dependency_data} ${dependency})
    string(JSON version GET ${data} version)
    string(JSON hash GET ${data} hashes ${platform})
    string(JSON url GET ${data} baseUrl)
    string(JSON label GET ${data} label)
    # cmake-format: off
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
    message(STATUS "Working on dependency and version ${dependency} ${version}")
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
    elseif(dependency STREQUAL libmediasoupclient)
      set(url ${url}/${file})
    elseif(dependency STREQUAL webrtc)
      set(url ${url}/${file})
    elseif(dependency STREQUAL grpc)
      set(file "grpc-release-${version}.7z")
      set(url ${url}/${file})
    elseif(dependency STREQUAL openssl)
      set(file "${dependency}-${version}-${arch}.7z")
      set(url ${url}/${file})
    else()
      set(url ${url}/${version}/${file})
    endif()
    
    message(STATUS "Working on url ${url}, file ${file}")

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
      elseif (dependency STREQUAL libmediasoupclient)
        file(ARCHIVE_EXTRACT INPUT "${dependencies_dir}/${file}" DESTINATION "${dependencies_dir}")
      elseif (dependency STREQUAL webrtc)
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
    elseif(dependency STREQUAL libmediasoupclient)
      if(WIN32)
        set(libmediasoupclient_subdir "libmediasoupclient_dist")
      else()
        set(libmediasoupclient_subdir "libmediasoupclient-VERSION-osx-ARCH")
        string(REPLACE "VERSION" "${version}" libmediasoupclient_subdir "${libmediasoupclient_subdir}")
        string(REPLACE "ARCH" "${arch}" libmediasoupclient_subdir "${libmediasoupclient_subdir}")
      endif()
      set(LIBMEDIASOUPCLIENT_PATH
          "${dependencies_dir}/${libmediasoupclient_subdir}"
          CACHE PATH "libmediasoupclient directory" FORCE)

          set(MEDIASOUP_INCLUDE_PATH "${dependencies_dir}/${libmediasoupclient_subdir}/include/mediasoupclient/" CACHE PATH "libmediasoupclient include directory" FORCE)
      if(WIN32)
        set(MEDIASOUP_LIB_PATH "${dependencies_dir}/${libmediasoupclient_subdir}/lib/mediasoupclient.lib" CACHE PATH "libmediasoupclient lib directory" FORCE)
      elseif(APPLE)
        set(MEDIASOUP_LIB_PATH "${dependencies_dir}/${libmediasoupclient_subdir}/lib/libmediasoupclient.a" CACHE PATH "libmediasoupclient lib directory" FORCE)
      endif()

      set(MEDIASOUP_SDP_INCLUDE_PATH "${dependencies_dir}/${libmediasoupclient_subdir}/include/sdptransform" CACHE PATH "libmediasoupclient sdp include directory" FORCE)
      if(WIN32)
        set(MEDIASOUP_SDP_LIB_PATH "${dependencies_dir}/${libmediasoupclient_subdir}/lib/sdptransform.lib" CACHE PATH "libmediasoupclient sdp lib directory" FORCE)
      elseif(APPLE)  
        set(MEDIASOUP_SDP_LIB_PATH "${dependencies_dir}/${libmediasoupclient_subdir}/lib/libsdptransform.a" CACHE PATH "libmediasoupclient sdp lib directory" FORCE)
      endif()
      list(APPEND CMAKE_PREFIX_PATH "${dependencies_dir}/${libmediasoupclient_subdir}")
    elseif(dependency STREQUAL webrtc)
      if(WIN32)
        set(webrtc_subdir "webrtc_dist")
      elseif(APPLE)
        set(webrtc_subdir "webrtc-VERSION-osx-ARCH")
        string(REPLACE "VERSION" "${version}" webrtc_subdir "${webrtc_subdir}")
        string(REPLACE "ARCH" "${arch}" webrtc_subdir "${webrtc_subdir}")
      endif()

      set(WEBRTC_PATH
          "${dependencies_dir}/${webrtc_subdir}"
          CACHE PATH "webrtc directory" FORCE)

      set(WEBRTC_INCLUDE_PATH "${dependencies_dir}/${webrtc_subdir}" CACHE PATH "webrtc include directory" FORCE)
      if(WIN32)
        set(WEBRTC_LIB_PATH "${dependencies_dir}/${webrtc_subdir}/webrtc.lib" CACHE PATH "webrtc lib path" FORCE)
      elseif(APPLE)
        set(WEBRTC_LIB_PATH "${dependencies_dir}/${webrtc_subdir}/libwebrtc.a" CACHE PATH "webrtc lib path" FORCE)
      endif()
      list(APPEND CMAKE_PREFIX_PATH "${dependencies_dir}/${webrtc_subdir}")
    elseif(dependency STREQUAL grpc)
      set(grpc_subdir "grpc-release-${version}")
      set(Protobuf_DIR "${dependencies_dir}/${grpc_subdir}/cmake" CACHE PATH "Protobuf directory" FORCE)
      set(GRPC_PATH "${dependencies_dir}/${grpc_subdir}" CACHE PATH "GRPC directory" FORCE)

      list(APPEND CMAKE_PREFIX_PATH "${GRPC_PATH}")

      message(STATUS "Setting up GRPC_PATH ${GRPC_PATH}")
      message(STATUS "Setting up Protobuf_DIR ${Protobuf_DIR}")
      message(STATUS "Setting up CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH}")
    elseif(dependency STREQUAL openssl)
      set(openssl_subdir "openssl-VERSION-ARCH")
      string(REPLACE "VERSION" "${version}" openssl_subdir "${openssl_subdir}")
      string(REPLACE "ARCH" "${arch}" openssl_subdir "${openssl_subdir}")

      set(OPENSSL_PATH
        "${dependencies_dir}/${openssl_subdir}"
        CACHE PATH "openssl directory" FORCE)

      set(OPENSSL_INCLUDE_PATH "${OPENSSL_PATH}/include" CACHE PATH "openssl include directory" FORCE)
      set(OPENSSL_LIB_PATH "${OPENSSL_PATH}/lib/libssl.a" CACHE PATH "openssl SSL library path" FORCE)
      set(OPENSSL_CRYPTO_LIB_PATH "${OPENSSL_PATH}/lib/libcrypto.a" CACHE PATH "openssl Crypto library path" FORCE)

      # Explicitly set CMake variables for OpenSSL
      set(OPENSSL_ROOT_DIR "${OPENSSL_PATH}" CACHE PATH "OpenSSL root directory" FORCE)
      set(OPENSSL_INCLUDE_DIR "${OPENSSL_INCLUDE_PATH}" CACHE PATH "OpenSSL include directory" FORCE)
      set(OPENSSL_LIBRARIES "${OPENSSL_LIB_PATH};${OPENSSL_CRYPTO_LIB_PATH}" CACHE PATH "OpenSSL libraries" FORCE)

      list(APPEND CMAKE_PREFIX_PATH "${OPENSSL_PATH}")

      message(STATUS "Setting up OPENSSL_PATH ${OPENSSL_PATH}")
      message(STATUS "Setting up OPENSSL_INCLUDE_PATH ${OPENSSL_INCLUDE_PATH}")
      message(STATUS "Setting up OPENSSL_LIB_PATH ${OPENSSL_LIB_PATH}")
      message(STATUS "Setting up OPENSSL_CRYPTO_LIB_PATH ${OPENSSL_CRYPTO_LIB_PATH}")
      message(STATUS "Setting up OPENSSL_ROOT_DIR ${OPENSSL_ROOT_DIR}")
      message(STATUS "Setting up OPENSSL_LIBRARIES ${OPENSSL_LIBRARIES}")
    endif()

    message(STATUS "Finished with file and destination ${file} ${destination}")
    message(STATUS "Setting up ${label} (${arch}) - done")
  endforeach()

  list(REMOVE_DUPLICATES CMAKE_PREFIX_PATH)

  set(CMAKE_PREFIX_PATH
      ${CMAKE_PREFIX_PATH}
      CACHE PATH "CMake prefix search path" FORCE)
endfunction()
