if(NOT TARGET OBS::obfuscate)
  add_library(obs-obfuscate INTERFACE)
  add_library(OBS::obfuscate ALIAS obs-obfuscate)
  target_sources(obs-obfuscate INTERFACE util/windows/obfuscate.c util/windows/obfuscate.h)
  target_include_directories(obs-obfuscate INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

if(NOT TARGET OBS::comutils)
  add_library(obs-comutils INTERFACE)
  add_library(OBS::COMutils ALIAS obs-comutils)
  target_sources(obs-comutils INTERFACE util/windows/ComPtr.hpp)
  target_include_directories(obs-comutils INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

if(NOT TARGET OBS::winhandle)
  add_library(obs-winhandle INTERFACE)
  add_library(OBS::winhandle ALIAS obs-winhandle)
  target_sources(obs-winhandle INTERFACE util/windows/WinHandle.hpp)
  target_include_directories(obs-winhandle INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

if(NOT TARGET OBS::threading-windows)
  add_library(obs-threading-windows INTERFACE)
  add_library(OBS::threading-windows ALIAS obs-threading-windows)
  target_sources(obs-threading-windows INTERFACE util/threading-windows.h)
  target_include_directories(obs-threading-windows INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

if(NOT TARGET OBS::w32-pthreads)
  add_subdirectory("${CMAKE_SOURCE_DIR}/deps/w32-pthreads" "${CMAKE_BINARY_DIR}/deps/w32-pthreads")
endif()

configure_file(cmake/windows/obs-module.rc.in libobs.rc)

target_sources(
  libobs
  PRIVATE
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
    util/windows/CoTaskMemPtr.hpp
    util/windows/device-enum.c
    util/windows/device-enum.h
    util/windows/HRError.hpp
    util/windows/win-registry.h
    util/windows/win-version.h
    util/windows/window-helpers.c
    util/windows/window-helpers.h
)

target_compile_options(libobs PRIVATE $<$<COMPILE_LANGUAGE:C,CXX>:/EHc->)

set_source_files_properties(
  obs-win-crash-handler.c
  PROPERTIES COMPILE_DEFINITIONS OBS_VERSION="${OBS_VERSION_CANONICAL}"
)

target_link_libraries(
  libobs
  PRIVATE Avrt Dwmapi Dxgi winmm Rpcrt4 OBS::obfuscate OBS::winhandle OBS::COMutils
  PUBLIC OBS::w32-pthreads
)

target_link_options(libobs PRIVATE /IGNORE:4098 /SAFESEH:NO)

set_target_properties(libobs PROPERTIES PREFIX "" OUTPUT_NAME "obs")
