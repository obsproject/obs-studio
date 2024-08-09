cmake_minimum_required(VERSION 3.24...3.25)

legacy_check()

add_library(libobs-winrt-headers INTERFACE)
add_library(OBS::winrt-headers ALIAS libobs-winrt-headers)

target_sources(libobs-winrt-headers INTERFACE winrt-capture.h)
target_include_directories(libobs-winrt-headers INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(libobs-winrt MODULE)
add_library(OBS::libobs-winrt ALIAS libobs-winrt)

target_sources(libobs-winrt PRIVATE winrt-capture.cpp winrt-capture.h winrt-dispatch.cpp winrt-dispatch.h)

target_precompile_headers(
  libobs-winrt
  PRIVATE
  <d3d11.h>
  <DispatcherQueue.h>
  <dwmapi.h>
  <obs-module.h>
  <util/windows/ComPtr.hpp>
  <Windows.Graphics.Capture.Interop.h>
  <windows.graphics.directx.direct3d11.interop.h>
  <winrt/Windows.Foundation.Metadata.h>
  <winrt/Windows.Graphics.Capture.h>
  <winrt/Windows.System.h>)

target_link_libraries(libobs-winrt PRIVATE OBS::libobs OBS::COMutils Dwmapi windowsapp)

# cmake-format: off
set_target_properties_obs(libobs-winrt PROPERTIES FOLDER core)
# cmake-format: on
