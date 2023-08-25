option(ENABLE_QSV11 "Build Intel QSV11 Hardware Encoder." TRUE)

if(NOT ENABLE_QSV11)
  obs_status(DISABLED "obs-qsv11")
  return()
endif()

project(obs-qsv11)

add_library(obs-qsv11 MODULE)
add_library(OBS::qsv11 ALIAS obs-qsv11)

find_package(VPL 2.6 REQUIRED)

target_sources(
  obs-qsv11
  PRIVATE obs-qsv11.c
          obs-qsv11-plugin-main.c
          common_utils.cpp
          common_utils.h
          QSV_Encoder.cpp
          QSV_Encoder.h
          QSV_Encoder_Internal.cpp
          QSV_Encoder_Internal.h
          bits/linux_defs.h
          bits/windows_defs.h)

target_link_libraries(obs-qsv11 PRIVATE OBS::libobs VPL::VPL)

if(OS_WINDOWS)
  add_subdirectory(obs-qsv-test)

  target_compile_definitions(obs-qsv11 PRIVATE DX11_D3D)

  set(MODULE_DESCRIPTION "OBS QSV encoder")
  configure_file(${CMAKE_SOURCE_DIR}/cmake/bundle/windows/obs-module.rc.in obs-qsv11.rc)

  target_sources(
    obs-qsv11
    PRIVATE obs-qsv11.rc
            common_directx9.cpp
            common_directx9.h
            common_directx11.cpp
            common_directx11.h
            common_utils_windows.cpp
            device_directx9.cpp
            device_directx9.h)

  target_link_libraries(obs-qsv11 PRIVATE d3d9 d3d11 dxva2 dxgi dxguid)
  target_link_options(obs-qsv11 PRIVATE /IGNORE:4099)

  target_compile_definitions(obs-qsv11 PRIVATE UNICODE _UNICODE _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS)
elseif(OS_LINUX)
  find_package(Libva REQUIRED)

  target_sources(obs-qsv11 PRIVATE common_utils_linux.cpp)

  target_link_libraries(obs-qsv11 PRIVATE Libva::va Libva::drm)
endif()

set_target_properties(obs-qsv11 PROPERTIES FOLDER "plugins/obs-qsv11")

file(GLOB _OBS_QSV11_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.c ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB _OBS_QSV11_HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

source_group("obs-qsv11\\Source Files" FILES ${_OBS_QSV11_SOURCE_FILES})
source_group("obs-qsv11\\Header Files" FILES ${_OBS_QSV11_HEADER_FILES})

setup_plugin_target(obs-qsv11)
