if(NOT TARGET OBS::w32-pthreads)
  add_subdirectory("${CMAKE_SOURCE_DIR}/deps/w32-pthreads" "${CMAKE_BINARY_DIR}/deps/w32-pthreads")
endif()

configure_file(cmake/windows/obs-module.rc.in libobs.rc)

add_library(obs-obfuscate INTERFACE)
add_library(OBS::obfuscate ALIAS obs-obfuscate)
target_sources(obs-obfuscate INTERFACE util/windows/obfuscate.c util/windows/obfuscate.h)
target_include_directories(obs-obfuscate INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(obs-comutils INTERFACE)
add_library(OBS::COMutils ALIAS obs-comutils)
target_sources(obs-comutils INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/util/windows/ComPtr.hpp)
target_include_directories(obs-comutils INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(obs-winhandle INTERFACE)
add_library(OBS::winhandle ALIAS obs-winhandle)
target_sources(obs-winhandle INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/util/windows/WinHandle.hpp)
target_include_directories(obs-winhandle INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

target_sources(
  libobs
  PRIVATE # cmake-format: sortable
          audio-monitoring/win32/wasapi-enum-devices.c
          audio-monitoring/win32/wasapi-monitoring-available.c
          audio-monitoring/win32/wasapi-output.c
          audio-monitoring/win32/wasapi-output.h
          libobs.rc
          obs-win-crash-handler.c
          obs-windows.c
          util/pipe-windows.c
          util/platform-windows.c
          util/threading-windows.c
          util/threading-windows.h
          util/windows/device-enum.c
          util/windows/device-enum.h
          util/windows/HRError.hpp
          util/windows/obfuscate.c
          util/windows/obfuscate.h
          util/windows/win-registry.h
          util/windows/win-version.h
          util/windows/window-helpers.c
          util/windows/window-helpers.h)

target_include_directories(
  libobs
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/util/windows>
#  $<INSTALL_INTERFACE:util/windows>
)

target_compile_options(libobs PRIVATE $<$<COMPILE_LANGUAGE:C,CXX>:/EHc->)

set_source_files_properties(obs-win-crash-handler.c PROPERTIES COMPILE_DEFINITIONS
                                                               OBS_VERSION="${OBS_VERSION_CANONICAL}")

target_link_libraries(
  libobs
  PRIVATE Avrt
          Dwmapi
          Dxgi
          winmm
          Rpcrt4
          OBS::obfuscate
          OBS::winhandle
          OBS::COMutils
  PUBLIC OBS::w32-pthreads)

target_link_options(libobs PRIVATE /IGNORE:4098 /SAFESEH:NO)

set_target_properties(libobs PROPERTIES PREFIX "" OUTPUT_NAME "obs")

find_package(LibDataChannel 0.20 REQUIRED)

if(NOT ENABLE_UI)
  set(DEPENDENCY_DLLS
    $<TARGET_FILE:FFmpeg::avcodec>
    $<TARGET_FILE:FFmpeg::avformat>
    $<TARGET_FILE:FFmpeg::avutil>
    $<TARGET_FILE:FFmpeg::swscale>
    $<TARGET_FILE:FFmpeg::swresample>
    $<TARGET_FILE:FFmpeg::avfilter>
    $<TARGET_FILE:FFmpeg::avdevice>
    $<TARGET_FILE:FFmpeg::ffmpegexe>
    $<TARGET_FILE:FFmpeg::ffprobeexe>
    $<TARGET_FILE:Libx264::Libx264>

    "$<TARGET_FILE_DIR:ZLIB::ZLIB>/../bin/zlib.dll"
    "$<TARGET_FILE_DIR:Librist::Librist>/../bin/librist.dll"
    "$<TARGET_FILE_DIR:Libsrt::Libsrt>/../bin/srt.dll"

    "$<TARGET_FILE_DIR:CURL::libcurl>/../bin/libcurl.dll"
    "$<TARGET_FILE_DIR:LibDataChannel::LibDataChannel>/../bin/datachannel.dll"
  )

  set(DEPENDENCY_LIBS
    $<TARGET_FILE:Libsrt::Libsrt>
    $<TARGET_FILE:Librist::Librist>
    $<TARGET_FILE:ZLIB::ZLIB>
    $<TARGET_FILE:CURL::libcurl>
    $<TARGET_FILE:LibDataChannel::LibDataChannel>
  )

  # foreach(DEP_BINARY ${DEPENDENCY_DLLS})
  # message(STATUS "Adding custom command to copy ${DEP_BINARY} to ${OBS_EXECUTABLE_DESTINATION}")

  # add_custom_command(TARGET libobs POST_BUILD
  # COMMAND "${CMAKE_COMMAND}" -E echo "Copying dependencies binaries ${DEP_BINARY} to ${OBS_EXECUTABLE_DESTINATION}"
  # COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DEP_BINARY}" "${OBS_EXECUTABLE_DESTINATION}"
  # COMMENT "."
  # VERBATIM COMMAND_EXPAND_LISTS
  # )
  # endforeach()
  install(FILES ${DEPENDENCY_DLLS} DESTINATION ${OBS_EXECUTABLE_DESTINATION})
  install(FILES ${DEPENDENCY_LIBS} DESTINATION ${OBS_LIBRARY_DESTINATION})
endif()
