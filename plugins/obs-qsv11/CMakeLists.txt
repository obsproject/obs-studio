cmake_minimum_required(VERSION 3.22...3.25)

legacy_check()

option(ENABLE_QSV11 "Build Intel QSV11 Hardware Encoder." TRUE)
if(NOT ENABLE_QSV11)
  target_disable_feature(obs-qsv11 "Intel QSV11 Hardware Encoder")
  target_disable(obs-qsv11)
  return()
endif()

add_library(obs-qsv11 MODULE)
add_library(OBS::qsv11 ALIAS obs-qsv11)

find_package(VPL 2.6 REQUIRED)

target_sources(
  obs-qsv11
  PRIVATE # cmake-format: sortable
          common_utils.cpp
          common_utils.h
          obs-qsv11-plugin-main.c
          obs-qsv11.c
          QSV_Encoder.cpp
          QSV_Encoder.h
          QSV_Encoder_Internal.cpp
          QSV_Encoder_Internal.h)

target_link_libraries(obs-qsv11 PRIVATE OBS::libobs VPL::VPL)

# cmake-format: off
set_target_properties_obs(obs-qsv11 PROPERTIES FOLDER plugins/obs-qsv11 PREFIX "")
# cmake-format: on

if(OS_WINDOWS)
  add_subdirectory(obs-qsv-test)

  configure_file(cmake/windows/obs-module.rc.in obs-qsv11.rc)
  target_sources(obs-qsv11 PRIVATE obs-qsv11.rc)

  target_compile_definitions(obs-qsv11 PRIVATE DX11_D3D)

  target_sources(obs-qsv11 PRIVATE # cmake-format: sortable
                                   common_directx11.cpp common_directx11.h common_utils_windows.cpp)

  target_link_libraries(obs-qsv11 PRIVATE d3d11 dxgi dxguid)
  target_link_options(obs-qsv11 PRIVATE /IGNORE:4099)
elseif(OS_LINUX)
  find_package(Libva REQUIRED)

  target_sources(obs-qsv11 PRIVATE common_utils_linux.cpp)

  target_link_libraries(obs-qsv11 PRIVATE Libva::va Libva::drm)
endif()
